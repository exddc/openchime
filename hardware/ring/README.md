# Ring hardware

Hardware specification for the Raspberry Pi Zero 2 W doorbell with Camera Module 3 Wide, I2S audio, two relays, one to four Kailh BOX buttons, and a heated, sealed outdoor enclosure.

Prototype architecture: U7 carries two encapsulated mains supplies inside a partitioned compartment; U5 carries the keys and backlight. Electrical and environmental qualification remain open. E4 (relay) and E7 (antenna) stay deferred to casing design.

| Document | Owns |
| --- | --- |
| [Electrical interface](variants/rpi02w_cm3/README.md) | Pin assignments, load budget, device and firmware requirements |
| [Board definitions](boards.md) | U5/U7 circuits, connectors, and PCB targets |
| [Assembly BOM](bom.md) | Parts, quantities, and 300 × 150 mm casing harness estimates |
| [Qualification](qualification.md) | Ranked blockers, acceptance criteria, and remaining design work |

## Wiring

Three drawings show every harness connection; on-board circuits are in the board definitions.

Power and strike, source [wiring-power.yml](wiring-power.yml):

![Ring power and strike](wiring-power.svg)

Audio and camera, source [wiring-audio.yml](wiring-audio.yml):

![Ring audio and camera](wiring-audio.svg)

Key board, relays, heater control, and sensor, source [wiring-io.yml](wiring-io.yml):

![Ring key board, relays, heater control, and sensor](wiring-io.svg)

Regenerate the committed SVGs with WireViz 0.4.1 and Graphviz 15.1.1:

```sh
wireviz -f s hardware/ring/wiring-power.yml hardware/ring/wiring-audio.yml hardware/ring/wiring-io.yml
```

Use [bom.md](bom.md) for procurement; generated wire reports are partial.
