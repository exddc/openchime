import { readFileSync, writeFileSync } from "node:fs";
import type {
  ElkEdgeSection,
  ElkExtendedEdge,
  ElkLabel,
  ElkNode,
  ElkPoint,
} from "elkjs/lib/elk-api";
import ELK from "elkjs/lib/elk.bundled.js";
import { parse } from "yaml";

// elkjs mistakes Bun's global `self` for a Web Worker scope and skips its CommonJS export.
Reflect.deleteProperty(globalThis, "self");

type Side = "WEST" | "EAST";
type Anchor = "start" | "middle" | "end";
type PinRef = { connector: string; pin: string };
type Wire = { id: string; from: PinRef; to: PinRef; label?: string };
type Connector = {
  name: string;
  title: string[];
  notes: string;
  pins: string[];
  labels: Map<string, string>;
  numbered: boolean;
};
type Model = { connectors: Map<string, Connector>; wires: Wire[] };
type Part = {
  boxWidth: number;
  boxHeight: number;
  west: string[];
  east: string[];
  notes: string[];
};
type Placement = Map<string, { x: number; y: number }>;
type Segment = { net: string; x1: number; y1: number; x2: number; y2: number };
type Rect = { x1: number; y1: number; x2: number; y2: number };
type Attempt = {
  graph: ElkNode;
  segments: Segment[];
  score: number;
  conflicts: [Segment, Segment][];
};

const ROW = 20;
const STUB = 20;
const PAD = 6;
const DOT = 3;
const MARGIN = 20;
const TRACK = 10;
const LOOP_SHIFTS = [30, 20];
const NOTE_WIDTH = 240;
const CONFLICT = 1000;
const STRATEGIES = ["NETWORK_SIMPLEX", "BRANDES_KOEPF", "LINEAR_SEGMENTS"];
const SHIFTS = [0.5, -0.5, 1, -1, 1.5, -1.5, 2, -2].map((rows) => rows * ROW);
const INTERACTIVE = {
  "elk.layered.layering.strategy": "INTERACTIVE",
  "elk.layered.crossingMinimization.strategy": "INTERACTIVE",
  "elk.layered.nodePlacement.strategy": "INTERACTIVE",
};
const FONT = { pin: 11, number: 10, title: 12, note: 10, wire: 10 };
const LEADING = { title: 15, note: 12 };
const COLOR = {
  box: "#333333",
  pin: "#870000",
  text: "#000000",
  title: "#000084",
  note: "#555555",
  wire: "#008700",
  junction: "#cc0000",
};

// Helvetica advance widths for ASCII 32-126, in 1/1000 em.
const GLYPH_WIDTHS = [
  278, 278, 355, 556, 556, 889, 667, 191, 333, 333, 389, 584, 278, 333, 278,
  278, 556, 556, 556, 556, 556, 556, 556, 556, 556, 556, 278, 278, 584, 584,
  584, 556, 1015, 667, 667, 722, 722, 667, 611, 778, 722, 278, 500, 667, 556,
  833, 722, 778, 667, 778, 722, 667, 611, 722, 667, 944, 667, 667, 611, 278,
  278, 278, 469, 556, 333, 556, 556, 500, 556, 556, 278, 556, 556, 222, 222,
  500, 222, 833, 556, 556, 556, 556, 333, 500, 278, 556, 500, 722, 500, 500,
  500, 334, 260, 334, 584,
];

function textWidth(text: string, size: number): number {
  let units = 0;
  for (const char of text) {
    units += GLYPH_WIDTHS[char.charCodeAt(0) - 32] ?? 600;
  }
  return (units * size) / 1000;
}

function wrap(text: string, width: number, size: number): string[] {
  const lines: string[] = [];
  let line = "";
  for (const word of text.split(" ").filter(Boolean)) {
    const next = line ? `${line} ${word}` : word;
    if (line && textWidth(next, size) > width) {
      lines.push(line);
      line = word;
    } else {
      line = next;
    }
  }
  if (line) lines.push(line);
  return lines;
}

function must<K, V>(map: Map<K, V>, key: K): V {
  const value = map.get(key);
  if (value === undefined) throw new Error(`missing ${String(key)}`);
  return value;
}

const portId = (ref: PinRef) => `${ref.connector}:${ref.pin}`;
const pinLabel = (c: Connector, pin: string) => c.labels.get(pin) ?? pin;
const pinY = (row: number) => ROW * 0.75 + row * ROW;
const pointsOf = (section: ElkEdgeSection): ElkPoint[] => [
  section.startPoint,
  ...(section.bendPoints ?? []),
  section.endPoint,
];

function entry(item: unknown, path: string): [string, unknown[]] {
  const entries = item && typeof item === "object" ? Object.entries(item) : [];
  if (entries.length !== 1) {
    throw new Error(`${path}: connection items map one designator to pins`);
  }
  const [name, pins] = entries[0];
  return [name, Array.isArray(pins) ? pins : [pins]];
}

function load(path: string): Model {
  const doc = parse(readFileSync(path, "utf8"));
  const cables: Record<string, Record<string, unknown>> = doc.cables ?? {};
  const specs = Object.entries<Record<string, unknown>>(doc.connectors ?? {});
  const connectors = new Map<string, Connector>();

  for (const [name, spec] of specs) {
    const declared =
      (spec.pins as unknown[]) ??
      Array.from({ length: Number(spec.pincount ?? 0) }, (_, i) => i + 1);
    const pins = declared.map(String);
    const pinlabels = ((spec.pinlabels as unknown[]) ?? []).map(String);
    const kind = [spec.type, spec.subtype].filter(Boolean).join(", ");
    connectors.set(name, {
      name,
      title: [name, kind].filter(Boolean),
      notes: String(spec.notes ?? "")
        .replace(/\s+/g, " ")
        .trim(),
      pins,
      labels: new Map(pins.map((pin, i) => [pin, pinlabels[i] ?? pin])),
      numbered: pins.some(
        (pin, i) => pinlabels[i] !== undefined && pinlabels[i] !== pin,
      ),
    });
  }

  const pinRef = (name: string, value: unknown): PinRef => {
    const connector = connectors.get(name);
    if (!connector) throw new Error(`${path}: unknown connector ${name}`);
    const key = String(value);
    if (connector.pins.includes(key)) return { connector: name, pin: key };
    const byLabel = connector.pins.filter(
      (pin) => pinLabel(connector, pin) === key,
    );
    if (byLabel.length !== 1) {
      throw new Error(`${path}: ${name} has no unique pin ${key}`);
    }
    return { connector: name, pin: byLabel[0] };
  };

  const wireLabel = (name: string, value: unknown): string => {
    const colors = ((cables[name].colors as unknown[]) ?? []).map(String);
    const wirelabels = ((cables[name].wirelabels as unknown[]) ?? []).map(
      String,
    );
    const count = Number(
      cables[name].wirecount ?? Math.max(colors.length, wirelabels.length),
    );
    const index =
      typeof value === "number" ? value - 1 : wirelabels.indexOf(String(value));
    if (index < 0 || index >= count) {
      throw new Error(`${path}: ${name} has no wire ${value}`);
    }
    return [name, colors[index]].filter(Boolean).join(" ");
  };

  const wires: Wire[] = [];
  const add = (wire: Omit<Wire, "id">) =>
    wires.push({ id: `w${wires.length}`, ...wire });
  for (const chain of doc.connections ?? []) {
    const items = (chain as unknown[]).map((item) => entry(item, path));
    items.forEach(([name, pins], i) => {
      if (!(name in cables)) return;
      const left = items[i - 1];
      const right = items[i + 1];
      if (!left || !right || left[0] in cables || right[0] in cables) {
        throw new Error(`${path}: ${name} needs a connector on both sides`);
      }
      if (left[1].length !== pins.length || right[1].length !== pins.length) {
        throw new Error(`${path}: ${name} pin counts differ from its ends`);
      }
      pins.forEach((wire, j) => {
        add({
          from: pinRef(left[0], left[1][j]),
          to: pinRef(right[0], right[1][j]),
          label: wireLabel(name, wire),
        });
      });
    });
  }
  for (const [name, spec] of specs) {
    for (const [a, b] of (spec.loops as unknown[][]) ?? []) {
      add({ from: pinRef(name, a), to: pinRef(name, b) });
    }
  }

  for (const [name, spec] of specs) {
    if (spec.hide_disconnected_pins !== true) continue;
    const connector = must(connectors, name);
    const used = new Set(
      wires
        .flatMap((w) => [w.from, w.to])
        .filter((ref) => ref.connector === name)
        .map((ref) => ref.pin),
    );
    connector.pins = connector.pins.filter((pin) => used.has(pin));
  }
  return { connectors, wires };
}

function assignSides({ connectors, wires }: Model): Map<string, Side> {
  const side = new Map<string, Side>();
  const external = new Set<string>();
  const straps = wires.filter((w) => w.from.connector === w.to.connector);
  for (const w of wires) {
    if (w.from.connector === w.to.connector) continue;
    for (const [ref, value] of [
      [w.from, "EAST"],
      [w.to, "WEST"],
    ] as const) {
      external.add(portId(ref));
      if (!side.has(portId(ref))) side.set(portId(ref), value);
    }
  }
  for (let changed = true; changed; ) {
    changed = false;
    for (const s of straps) {
      const a = side.get(portId(s.from));
      const b = side.get(portId(s.to));
      if (a && !b) side.set(portId(s.to), a);
      if (b && !a) side.set(portId(s.from), b);
      if (!a !== !b) changed = true;
    }
  }
  for (const c of connectors.values()) {
    const known = c.pins.map((pin) =>
      side.get(portId({ connector: c.name, pin })),
    );
    c.pins.forEach((pin, i) => {
      if (known[i]) return;
      let found: Side = "WEST";
      for (let d = 1; d < c.pins.length; d++) {
        const neighbour = known[i - d] ?? known[i + d];
        if (neighbour) {
          found = neighbour;
          break;
        }
      }
      side.set(portId({ connector: c.name, pin }), found);
    });
  }
  for (const s of straps) {
    const a = portId(s.from);
    const b = portId(s.to);
    if (side.get(a) === side.get(b)) continue;
    if (!external.has(b)) side.set(b, must(side, a));
    else if (!external.has(a)) side.set(a, must(side, b));
  }
  return side;
}

function partOf(c: Connector, side: Map<string, Side>): Part {
  const onSide = (s: Side) =>
    c.pins.filter((pin) => side.get(portId({ connector: c.name, pin })) === s);
  const west = onSide("WEST");
  const east = onSide("EAST");
  const widest = (pins: string[]) =>
    Math.max(0, ...pins.map((pin) => textWidth(pinLabel(c, pin), FONT.pin)));
  const gap = west.length && east.length ? 4 * PAD : 0;
  const inner = widest(west) + widest(east) + gap + 2 * PAD;
  const boxWidth = Math.max(60, Math.ceil(inner / 10) * 10);
  const rows = Math.max(west.length, east.length, 1);
  return {
    boxWidth,
    boxHeight: rows * ROW + ROW / 2,
    west,
    east,
    notes: wrap(c.notes, Math.max(boxWidth, NOTE_WIDTH), FONT.note),
  };
}

function buildGraph(
  model: Model,
  parts: Map<string, Part>,
  options: Record<string, string>,
  placement?: Placement,
): ElkNode {
  const children = [...model.connectors.values()].map((c): ElkNode => {
    const part = must(parts, c.name);
    const width = part.boxWidth + 2 * STUB;
    const labels: ElkLabel[] = [
      {
        id: `${c.name}#title`,
        text: c.title.join(" "),
        width: Math.max(...c.title.map((line) => textWidth(line, FONT.title))),
        height: c.title.length * LEADING.title,
        layoutOptions: { "elk.nodeLabels.placement": "OUTSIDE V_TOP H_CENTER" },
      },
    ];
    if (part.notes.length) {
      const widest = Math.max(
        ...part.notes.map((line) => textWidth(line, FONT.note)),
      );
      labels.push({
        id: `${c.name}#notes`,
        text: c.notes,
        width: STUB + widest,
        height: part.notes.length * LEADING.note,
        layoutOptions: {
          "elk.nodeLabels.placement": "OUTSIDE V_BOTTOM H_LEFT",
        },
      });
    }
    const port = (pin: string, row: number, s: Side) => ({
      id: portId({ connector: c.name, pin }),
      x: s === "WEST" ? 0 : width,
      y: pinY(row),
      width: 0,
      height: 0,
      layoutOptions: { "elk.port.side": s },
    });
    return {
      id: c.name,
      ...placement?.get(c.name),
      width,
      height: part.boxHeight,
      labels,
      ports: [
        ...part.west.map((pin, i) => port(pin, i, "WEST")),
        ...part.east.map((pin, i) => port(pin, i, "EAST")),
      ],
      layoutOptions: { "elk.portConstraints": "FIXED_POS" },
    };
  });

  const edges = model.wires.map(
    (w): ElkExtendedEdge => ({
      id: w.id,
      sources: [portId(w.from)],
      targets: [portId(w.to)],
      labels: w.label
        ? [
            {
              id: `${w.id}#label`,
              text: w.label,
              width: textWidth(w.label, FONT.wire),
              height: FONT.wire + 2,
              layoutOptions: { "elk.edgeLabels.placement": "HEAD" },
            },
          ]
        : [],
    }),
  );

  return {
    id: "root",
    layoutOptions: {
      "elk.algorithm": "layered",
      "elk.direction": "RIGHT",
      "elk.edgeRouting": "ORTHOGONAL",
      "elk.layered.considerModelOrder.strategy": "NODES_AND_EDGES",
      "elk.spacing.nodeNode": "30",
      "elk.layered.spacing.nodeNodeBetweenLayers": "40",
      "elk.spacing.edgeEdge": String(TRACK),
      "elk.layered.spacing.edgeEdgeBetweenLayers": String(TRACK),
      "elk.layered.spacing.edgeNodeBetweenLayers": "20",
      "elk.spacing.edgeLabel": "2",
      "elk.spacing.labelNode": "4",
      ...options,
    },
    children,
    edges,
  };
}

function netsOf(model: Model): Map<string, string> {
  const parent = new Map<string, string>();
  const find = (key: string): string => {
    const up = parent.get(key) ?? key;
    if (up === key) return key;
    const root = find(up);
    parent.set(key, root);
    return root;
  };
  for (const w of model.wires) {
    parent.set(find(portId(w.from)), find(portId(w.to)));
  }
  return new Map(model.wires.map((w) => [w.id, find(portId(w.from))]));
}

function segmentsOf(graph: ElkNode, nets: Map<string, string>): Segment[] {
  const segments: Segment[] = [];
  for (const edge of graph.edges ?? []) {
    const net = must(nets, edge.id);
    for (const section of edge.sections ?? []) {
      const points = pointsOf(section);
      for (let i = 1; i < points.length; i++) {
        const [a, b] = [points[i - 1], points[i]];
        if (a.x !== b.x || a.y !== b.y) {
          segments.push({ net, x1: a.x, y1: a.y, x2: b.x, y2: b.y });
        }
      }
    }
  }
  return segments;
}

const near = (a: number, b: number) => Math.abs(a - b) < 0.01;
const horizontal = (s: Segment) => near(s.y1, s.y2);
const span = (s: Segment): [number, number, number] =>
  horizontal(s)
    ? [s.y1, Math.min(s.x1, s.x2), Math.max(s.x1, s.x2)]
    : [s.x1, Math.min(s.y1, s.y2), Math.max(s.y1, s.y2)];

function directions(s: Segment, x: number, y: number): string[] {
  const [line, lo, hi] = span(s);
  const [along, across] = horizontal(s) ? [x, y] : [y, x];
  if (!near(across, line) || along < lo - 0.01 || along > hi + 0.01) return [];
  const [back, forward] = horizontal(s) ? ["W", "E"] : ["N", "S"];
  return [
    ...(along > lo + 0.01 ? [back] : []),
    ...(along < hi - 0.01 ? [forward] : []),
  ];
}

function junctions(segments: Segment[]): ElkPoint[] {
  const seen = new Set<string>();
  const result: ElkPoint[] = [];
  for (const s of segments) {
    for (const [x, y] of [
      [s.x1, s.y1],
      [s.x2, s.y2],
    ]) {
      const key = `${s.net}|${x.toFixed(2)}|${y.toFixed(2)}`;
      if (seen.has(key)) continue;
      seen.add(key);
      const dirs = new Set(
        segments
          .filter((t) => t.net === s.net)
          .flatMap((t) => directions(t, x, y)),
      );
      if (dirs.size >= 3) result.push({ x, y });
    }
  }
  return result;
}

function analyze(graph: ElkNode, nets: Map<string, string>): Attempt {
  const segments = segmentsOf(graph, nets);
  const conflicts: [Segment, Segment][] = [];
  let crossings = 0;
  for (let i = 0; i < segments.length; i++) {
    for (let j = i + 1; j < segments.length; j++) {
      const [a, b] = [segments[i], segments[j]];
      if (a.net === b.net) continue;
      const [la, loa, hia] = span(a);
      const [lb, lob, hib] = span(b);
      if (horizontal(a) === horizontal(b)) {
        const gap = Math.max(loa, lob) - Math.min(hia, hib);
        const collinear = near(la, lb) && gap < (horizontal(a) ? 3 * ROW : ROW);
        const crowded = Math.abs(la - lb) < TRACK - 0.01 && gap < 0;
        if (collinear || crowded) conflicts.push([a, b]);
      } else if (
        lb > loa - 0.01 &&
        lb < hia + 0.01 &&
        la > lob - 0.01 &&
        la < hib + 0.01
      ) {
        const inside =
          lb > loa + 0.01 &&
          lb < hia - 0.01 &&
          la > lob + 0.01 &&
          la < hib - 0.01;
        if (inside) crossings++;
        else conflicts.push([a, b]);
      }
    }
  }
  const score = conflicts.length * CONFLICT + crossings * 10 + segments.length;
  return { graph, segments, score, conflicts };
}

function widenLoops(
  graph: ElkNode,
  loops: Set<string>,
  shift: number,
): ElkNode {
  const copy = structuredClone(graph);
  for (const edge of copy.edges ?? []) {
    if (!loops.has(edge.id)) continue;
    for (const section of edge.sections ?? []) {
      const bends = section.bendPoints ?? [];
      if (bends.length !== 2 || !near(bends[0].x, bends[1].x)) continue;
      const out = Math.sign(bends[0].x - section.startPoint.x);
      for (const p of bends) p.x += out * shift;
    }
  }
  return copy;
}

function textRect(
  x: number,
  y: number,
  width: number,
  size: number,
  anchor: Anchor,
): Rect {
  const left = x - { start: 0, middle: width / 2, end: width }[anchor];
  return { x1: left, y1: y - size * 0.8, x2: left + width, y2: y + size * 0.2 };
}

function hits(r: Rect, s: Segment): boolean {
  const [line, lo, hi] = span(s);
  return horizontal(s)
    ? line > r.y1 && line < r.y2 && hi > r.x1 - 2 && lo < r.x2 + 2
    : line > r.x1 - 2 && line < r.x2 + 2 && hi > r.y1 && lo < r.y2;
}

const overlaps = (a: Rect, b: Rect) =>
  a.x1 < b.x2 + 2 && b.x1 < a.x2 + 2 && a.y1 < b.y2 && b.y1 < a.y2;

function placeLabel(
  edge: ElkExtendedEdge,
  label: ElkLabel,
  loop: boolean,
  segments: Segment[],
  placed: Rect[],
): [number, number, Anchor] {
  const section = edge.sections?.at(-1);
  const points = section ? pointsOf(section) : [];
  const [end, before, corner] = [points.at(-1), points.at(-2), points.at(-3)];
  const width = label.width ?? 0;
  const free = (x: number, y: number, anchor: Anchor) => {
    const r = textRect(x, y, width, FONT.wire, anchor);
    return (
      !segments.some((s) => hits(r, s)) && !placed.some((p) => overlaps(r, p))
    );
  };
  if (end && before && near(end.y, before.y)) {
    const away = end.x > before.x ? -1 : 1;
    const anchor = away < 0 ? "end" : "start";
    const beside = loop && corner !== undefined && near(corner.x, before.x);
    const origin = beside ? before.x : end.x;
    const reach = beside ? 4 * width : Math.abs(end.x - before.x) - width - 3;
    const rows = [end.y - 3, end.y + FONT.wire + 1];
    if (beside) rows.unshift(end.y + FONT.wire * 0.35);
    for (const y of rows) {
      for (let offset = 3; offset <= reach; offset += 2) {
        const x = origin + away * offset;
        if (free(x, y, anchor)) return [x, y, anchor];
      }
    }
  }
  return [label.x ?? 0, (label.y ?? 0) + (label.height ?? 0) - 2, "start"];
}

const num = (v: number) => String(Math.round(v * 10) / 10);
const xml = (text: string) =>
  text.replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;");

function wirePaths(segments: Segment[]): string[] {
  type Line = {
    net: string;
    flat: boolean;
    line: number;
    spans: [number, number][];
  };
  const lines = new Map<string, Line>();
  for (const s of segments) {
    const [line, lo, hi] = span(s);
    const key = `${s.net}|${horizontal(s)}|${line.toFixed(2)}`;
    const found = lines.get(key) ?? {
      net: s.net,
      flat: horizontal(s),
      line,
      spans: [],
    };
    found.spans.push([lo, hi]);
    lines.set(key, found);
  }
  const byNet = new Map<string, string[]>();
  for (const { net, flat, line, spans } of lines.values()) {
    spans.sort((a, b) => a[0] - b[0]);
    const merged: [number, number][] = [];
    for (const [lo, hi] of spans) {
      const last = merged.at(-1);
      if (last && lo <= last[1] + 0.01) last[1] = Math.max(last[1], hi);
      else merged.push([lo, hi]);
    }
    const d = merged.map(([lo, hi]) =>
      flat
        ? `M${num(lo)} ${num(line)}H${num(hi)}`
        : `M${num(line)} ${num(lo)}V${num(hi)}`,
    );
    byNet.set(net, [...(byNet.get(net) ?? []), ...d]);
  }
  return [...byNet.values()].map((d) => `<path d="${d.join("")}"/>`);
}

function svg(
  model: Model,
  parts: Map<string, Part>,
  { graph, segments }: Attempt,
): string {
  const bounds = {
    x1: Number.POSITIVE_INFINITY,
    y1: Number.POSITIVE_INFINITY,
    x2: Number.NEGATIVE_INFINITY,
    y2: Number.NEGATIVE_INFINITY,
  };
  const grow = (r: Rect) => {
    bounds.x1 = Math.min(bounds.x1, r.x1);
    bounds.y1 = Math.min(bounds.y1, r.y1);
    bounds.x2 = Math.max(bounds.x2, r.x2);
    bounds.y2 = Math.max(bounds.y2, r.y2);
  };
  const texts: string[] = [];
  const text = (
    x: number,
    y: number,
    value: string,
    size: number,
    fill: string,
    anchor: Anchor = "start",
  ) => {
    grow(textRect(x, y, textWidth(value, size), size, anchor));
    const attrs = [`x="${num(x)}"`, `y="${num(y)}"`, `font-size="${size}"`];
    if (fill !== COLOR.text) attrs.push(`fill="${fill}"`);
    if (anchor !== "start") attrs.push(`text-anchor="${anchor}"`);
    texts.push(`<text ${attrs.join(" ")}>${xml(value)}</text>`);
  };

  const boxes: string[] = [];
  const stubs: string[] = [];
  for (const node of graph.children ?? []) {
    const c = must(model.connectors, node.id);
    const part = must(parts, node.id);
    const x = node.x ?? 0;
    const y = node.y ?? 0;
    const boxX = x + STUB;
    const boxRight = boxX + part.boxWidth;
    boxes.push(
      `<rect x="${num(boxX)}" y="${num(y)}" width="${num(part.boxWidth)}" height="${num(part.boxHeight)}"/>`,
    );
    grow({ x1: x, y1: y, x2: boxRight + STUB, y2: y + part.boxHeight });
    const pins = (list: string[], s: Side) => {
      list.forEach((pin, row) => {
        const py = y + pinY(row);
        const [outer, inner] =
          s === "WEST" ? [x, boxX] : [boxRight + STUB, boxRight];
        stubs.push(
          `<line x1="${num(outer)}" y1="${num(py)}" x2="${num(inner)}" y2="${num(py)}"/>`,
        );
        if (c.numbered) {
          const middle = (outer + inner) / 2;
          text(middle, py - 3, pin, FONT.number, COLOR.pin, "middle");
        }
        const label = pinLabel(c, pin);
        if (s === "WEST")
          text(inner + PAD, py + 4, label, FONT.pin, COLOR.text);
        else text(inner - PAD, py + 4, label, FONT.pin, COLOR.text, "end");
      });
    };
    pins(part.west, "WEST");
    pins(part.east, "EAST");
    for (const label of node.labels ?? []) {
      const lx = x + (label.x ?? 0);
      const ly = y + (label.y ?? 0);
      if (label.id === `${c.name}#title`) {
        const center = lx + (label.width ?? 0) / 2;
        c.title.forEach((line, i) => {
          const baseline = ly + (i + 1) * LEADING.title - 3;
          text(center, baseline, line, FONT.title, COLOR.title, "middle");
        });
      } else {
        part.notes.forEach((line, i) => {
          const baseline = ly + (i + 1) * LEADING.note - 2;
          text(lx + STUB, baseline, line, FONT.note, COLOR.note);
        });
      }
    }
  }

  for (const s of segments) {
    grow({
      x1: Math.min(s.x1, s.x2),
      y1: Math.min(s.y1, s.y2),
      x2: Math.max(s.x1, s.x2),
      y2: Math.max(s.y1, s.y2),
    });
  }
  const dots = junctions(segments);
  const placed = dots.map((p) => ({
    x1: p.x - DOT,
    y1: p.y - DOT,
    x2: p.x + DOT,
    y2: p.y + DOT,
  }));
  const wires = new Map(model.wires.map((w) => [w.id, w]));
  for (const edge of graph.edges ?? []) {
    const wire = must(wires, edge.id);
    const loop = wire.from.connector === wire.to.connector;
    for (const label of edge.labels ?? []) {
      const [x, y, anchor] = placeLabel(edge, label, loop, segments, placed);
      placed.push(textRect(x, y, label.width ?? 0, FONT.wire, anchor));
      text(x, y, label.text ?? "", FONT.wire, COLOR.title, anchor);
    }
  }

  const left = num(bounds.x1 - MARGIN);
  const top = num(bounds.y1 - MARGIN);
  const width = num(bounds.x2 - bounds.x1 + 2 * MARGIN);
  const height = num(bounds.y2 - bounds.y1 + 2 * MARGIN);
  return [
    `<svg xmlns="http://www.w3.org/2000/svg" width="${width}" height="${height}" viewBox="${left} ${top} ${width} ${height}" font-family="Arial, Helvetica, sans-serif">`,
    `<rect x="${left}" y="${top}" width="${width}" height="${height}" fill="#ffffff"/>`,
    `<g fill="none" stroke="${COLOR.wire}" stroke-width="1.2" stroke-linecap="square">`,
    ...wirePaths(segments),
    "</g>",
    `<g stroke="${COLOR.pin}">`,
    ...stubs,
    "</g>",
    `<g fill="#ffffff" stroke="${COLOR.box}">`,
    ...boxes,
    "</g>",
    `<g fill="${COLOR.junction}">`,
    ...dots.map(
      (p) => `<circle cx="${num(p.x)}" cy="${num(p.y)}" r="${DOT}"/>`,
    ),
    "</g>",
    ...texts,
    "</svg>",
    "",
  ].join("\n");
}

async function render(path: string): Promise<string> {
  const model = load(path);
  const side = assignSides(model);
  const parts = new Map(
    [...model.connectors.values()].map((c) => [c.name, partOf(c, side)]),
  );
  const nets = netsOf(model);
  const elk = new ELK();
  const attempt = async (
    options: Record<string, string>,
    placement?: Placement,
  ) => {
    const graph = await elk.layout(
      buildGraph(model, parts, options, placement),
    );
    return analyze(graph, nets);
  };

  let best: Attempt | undefined;
  for (const strategy of STRATEGIES) {
    const next = await attempt({
      "elk.layered.nodePlacement.strategy": strategy,
    });
    if (!best || next.score < best.score) best = next;
  }
  if (!best) throw new Error(`${path}: no layout`);

  // ELK can run two nets along one line, which reads as a connection; nudge single parts until none do.
  while (best.conflicts.length) {
    let next: Attempt = best;
    const nodes = best.graph.children ?? [];
    for (const moved of nodes) {
      for (const shift of SHIFTS) {
        const placement: Placement = new Map(
          nodes.map((n) => [
            n.id,
            { x: n.x ?? 0, y: (n.y ?? 0) + (n === moved ? shift : 0) },
          ]),
        );
        const candidate = await attempt(INTERACTIVE, placement);
        if (candidate.score < next.score) next = candidate;
      }
    }
    if (next === best) break;
    best = next;
  }

  const loops = new Set(
    model.wires
      .filter((w) => w.from.connector === w.to.connector)
      .map((w) => w.id),
  );
  for (const shift of LOOP_SHIFTS) {
    const widened = analyze(widenLoops(best.graph, loops, shift), nets);
    if (widened.conflicts.length <= best.conflicts.length) {
      best = widened;
      break;
    }
  }

  if (best.conflicts.length) {
    throw new Error(
      `${path}: ${best.conflicts.length} wire pairs still share a line`,
    );
  }
  return svg(model, parts, best);
}

const inputs = process.argv.slice(2);
if (inputs.length === 0) {
  console.error("usage: bun scripts/wiring/render.ts <wiring.yml>...");
  process.exit(2);
}
for (const input of inputs) {
  if (!/\.ya?ml$/.test(input)) {
    console.error(`${input}: expected a .yml or .yaml file`);
    process.exit(2);
  }
}
for (const input of inputs) {
  const output = input.replace(/\.ya?ml$/, ".svg");
  writeFileSync(output, await render(input));
  console.log(`wrote ${output}`);
}
