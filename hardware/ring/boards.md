# Board definitions

Connection-level specification for the two custom boards. The Clapper is the key board behind the faceplate, named for the part of a bell that strikes it. The Yoke is the power board in the mains compartment, named for the beam that carries a bell and its load. Schematics, layouts, and connector parts are pending. Parts and quantities are in the [BOM](bom.md).

## Clapper key board

The Clapper carries one to four hot-swap switches with active-low GPIO outputs.

The metal front plate is the switch plate: 1.5 mm thick, 14 × 14 mm cutouts at 25.4 mm vertical pitch, board surface 5 mm behind the plate front. Populated positions count from the top.

| Element | Specification |
| --- | --- |
| SW1–SWn | Kailh BOX tactile, MX stem, 3-pin plate mount; exact part must meet dry-contact and IP56 requirements |
| SK1–SK4 | Kailh MX hot-swap sockets; all four fitted so positions can be populated later |
| KC1–KCn | MX-compatible relegendable keycaps, 1u or 2u, name label under a clear cover |
| JC1 | 6-circuit latching connector: 3V3, GND, BUTTON1, BUTTON2, BUTTON3, BUTTON4 |
| Board | About 50 × 130 mm, four M3 mounting points to standoffs on the front plate |

Channel n uses SWn, SKn, R1n, R2n, C1n, and D1n. For example, channel 1 uses SW1, SK1, R11, R21, C11, and D11.

| Component | Connection |
| --- | --- |
| SWn through SKn | Switch node to GND when pressed |
| R1n, 10 kΩ / 1% | Switch node to 3V3 |
| D1n, PESD5V0S1BA bidirectional TVS | Switch node to GND, beside the socket |
| R2n, 1 kΩ / 1% | Switch node to BUTTONn at JC1 |
| C1n, 100 nF | BUTTONn to GND, beside JC1 |

Fit all four sockets and input networks. Populate switches and keycaps only at positions 1 through n; blank unused faceplate openings.

At 3.3 V, contact current is 330 µA; select contacts rated for this dry circuit. With internal GPIO pulls disabled, nominal RC constants are 0.1 ms on press and 1.1 ms on release. Firmware provides 50 ms debounce.

Protection remains unqualified: the [PESD5V0S1BA](https://assets.nexperia.com/documents/data-sheet/PESD5V0S1BA.pdf) permits 10 V clamping at 1 A. A series resistor and the metal faceplate do not establish safe GPIO voltage or injection current.

## Yoke power board

The Yoke distributes mains to two encapsulated supplies and low voltage to the loads. Class II describes the supply modules; it does not classify the complete, PE-bonded assembly.

| Element | Specification |
| --- | --- |
| JAC | 3-pole PCB screw terminal, 7.5 mm pitch or wider, ≥ 300 V / 16 A, accepts 1.5 mm²: L, N, PE |
| F0 | 5 × 20 mm PCB fuse holder with cover; fuse T 2 A, 250 V, in L |
| RV1 | Metal-oxide varistor S14K275 across L and N after F0; fit for exposed installations |
| PSU5 | Mean Well IRM-30-5, PCB pins, 5 V / 6 A |
| PSU12 | Mean Well IRM-30-12, PCB pins, 12 V / 2.5 A |
| PE1 | Plated M4 mounting hole on the PE track, fastened to a metal standoff of the backplate with a serrated washer |
| PE_FRONT | Ring-terminal pad on the PE track for the front-plate bond |
| JP1 | 0 Ω link from the PE track to the 5 V return; fitted |
| JY1 | 5V_PI, GND_PI; 2-circuit, ≥ 5 A per contact, 20 AWG |
| JY2 | 5V_AMP, GND_AMP |
| JY3 | JD-VCC, GND_RLY; JD-VCC through F2 |
| JY4 | 12V_FUSED, 12V_RTN; 12V_FUSED through F1 |
| Board | About 160 × 90 mm on metal standoffs; mains section under a clip-on insulating cover |

| Component | Connection |
| --- | --- |
| F0 | JAC L to the fused L node |
| RV1 | Fused L node to N |
| PSU5, PSU12 | AC/L to the fused L node; AC/N to N |
| PSU5 outputs | +V to the 5 V rail; -V to the 5 V return |
| PSU12 outputs | +V to F1; -V to 12V_RTN at JY4; not connected to the 5 V return |
| F1, 1 A fast fuse in a PCB holder | PSU12 +V to 12V_FUSED at JY4 |
| F2, 500 mA hold PTC | 5 V rail to JD-VCC at JY3 |
| C3, 100 µF | JD-VCC at JY3 to the 5 V return |
| LED1 with R1, 1 kΩ | 5 V rail to the 5 V return; service indicator |
| JP1 | PE track to the 5 V return, single point |

Provisional layout targets, subject to insulation coordination for the installation, applicable product standard, altitude, pollution degree, and material group:

| Rule | Value |
| --- | --- |
| Creepage and clearance, mains to low voltage and mains to PE | At least 6 mm on the board, with a milled slot under each module between its AC and DC pins |
| Board material | FR4 with CTI of 175 or better; no low-voltage copper under the module primary side |
| Mains copper | 1 mm track width or more at 35 µm for 2 A; keep-out to the board edge of 3 mm or more |
| Cover | Clip-on insulating cover over JAC, F0, RV1, and the module primaries; removable only with the enclosure open |
| Marking | Mains section outlined and labelled on the silkscreen; fuse rating printed beside F0 |

The 12 V return connects only to JY4 and the strike. JP1 bonds the 5 V return to PE, so this rail is not SELV; [PELV classification](https://psu.deltaww.com/en/industry-know-how/what-is-the-difference-between-selv-pelv-and-es1-in-ac-dc-power-supplies) depends on the assembly assessment and applicable standard. Clapper JC1 GND shares the board return through the Pi; assess shared-current voltage drops during layout.

The 6 A supply does not establish protection for each branch. Coordinate protection with the J8 contact, harness, connectors, and supply fault response before energizing an assembled prototype. Fuse and PE-path verification remain open.
