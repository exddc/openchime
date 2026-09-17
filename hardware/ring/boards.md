# Board definitions

Two custom boards exist. U5, the key board, sits behind the faceplate and carries the switches. U7, the power board, sits in the rear mains compartment and carries both mains modules. Everything else is a bought module or a harness.

Both boards are defined here at connection level. Schematic capture, layout, and connector part selection are the next hardware step.

## U5 key board

U5 holds up to four Kailh BOX tactile switches in hot-swap sockets, one pull-up, series resistor, capacitor, and transient suppressor per channel, one white backlight LED per switch, and a single PWM-controlled MOSFET for the backlight. Its outputs are active-low GPIO signals.

The metal front plate is the switch plate: 1.5 mm thick, 14 × 14 mm cutouts at 25.4 mm vertical pitch, board surface 5 mm behind the plate front. Populated positions count from the top.

| Element | Specification |
| --- | --- |
| SW1–SWn | Kailh BOX tactile, IP56, MX-compatible stem, 3-pin plate mount |
| SK1–SK4 | Kailh MX hot-swap sockets; all four fitted so positions can be populated later |
| KC1–KCn | MX-compatible relegendable keycaps, 1u or 2u, name label under a clear cover |
| LED1–LEDn | White SMD LED in the switch housing window, top mount |
| J51 | 7-circuit latching connector: 3V3, GND, BUTTON1, BUTTON2, BUTTON3, BUTTON4, LED_PWM |
| J52 | 2-circuit latching connector: 5V_LED, GND_LED |
| Board | About 50 × 130 mm, four M3 mounting points to standoffs on the front plate |

Channel n uses SWn, SKn, R1n, R2n, C1n, D1n, LEDn, and R3n. For example, channel 1 uses SW1, SK1, R11, R21, C11, D11, LED1, and R31.

| Component | Connection |
| --- | --- |
| SWn through SKn | Switch node to GND when pressed |
| R1n, 10 kΩ / 1% | Switch node to 3V3 |
| D1n, PESD5V0S1BA bidirectional TVS | Switch node to GND, beside the socket |
| R2n, 1 kΩ / 1% | Switch node to BUTTONn at J51 |
| C1n, 100 nF | BUTTONn to GND, beside J51 |
| LEDn | Anode to 5V_LED; cathode to R3n |
| R3n, 220 Ω | LEDn cathode to the Q1 drain node |
| Q1, 2N7002 or equivalent logic-level N-channel MOSFET | Drain to the LED return node; source to GND |
| R42, 100 Ω | LED_PWM at J51 to the Q1 gate |
| R41, 10 kΩ | Q1 gate to GND |

All four pull-ups, capacitors, and suppressors remain fitted on every variant. Omit the switch, keycap, and LED for each unused position.

| Check | Calculation / design target |
| --- | --- |
| Contact current | 3.3 / 10 000 = 330 µA; gold crosspoint contacts required for this dry circuit |
| GPIO low, contact closed | Below 0.1 V through R2n with the GPIO input high impedance |
| GPIO high, contact open | Near 3.3 V; internal pull-up may stay enabled |
| RC at the GPIO | 1 kΩ × 100 nF = 0.1 ms; firmware debounce of 50 ms dominates |
| LED current | (5.0 - 3.0) / 220 = 9 mA per LED; 36 mA for four |
| Q1 dissipation | Below 5 mW at 36 mA |
| TVS | PESD5V0S1BA clamps the switch node; R2n limits the residual current into the SoC clamp diodes |

The switch housings protrude through the PE-bonded front plate, so most discharges reach earth before the board. D1n and R2n handle the remainder. Water management at the keycaps is enclosure open item E11.

## U7 power board

U7 carries the mains terminal, fuse, surge element, both Mean Well IRM-30 PCB-mount modules, protective-earth bonding, the 5 V and 12 V distribution terminals, the relay coil PTC, and the heater MOSFET.

| Element | Specification |
| --- | --- |
| JAC | 3-pole PCB screw terminal, 7.5 mm pitch or wider, ≥ 300 V / 16 A, accepts 1.5 mm²: L, N, PE |
| F0 | 5 × 20 mm PCB fuse holder with cover; fuse T 2 A, 250 V, in L |
| RV1 | Metal-oxide varistor S14K275 across L and N after F0; fit for exposed installations |
| PSU5 | Mean Well IRM-30-5, PCB pins, 5 V / 6 A |
| PSU12 | Mean Well IRM-30-12, PCB pins, 12 V / 2.5 A |
| PE1 | Plated M4 mounting hole on the PE track, fastened to a metal standoff of the backplate with a serrated washer |
| PE_FRONT | Ring-terminal pad on the PE track for the front-plate bond |
| JP1 | 0 Ω link from the PE track to the SELV ground; fitted |
| J71 | 5V_PI, GND_PI; 2-circuit, ≥ 5 A per contact, 20 AWG |
| J72 | 5V_AMP, GND_AMP |
| J73 | 5V_LED, GND_LED |
| J74 | JD-VCC, GND_RLY; JD-VCC through F2 |
| J75 | HTR+, HTR-; HTR- switched by Q71 |
| J76 | HEATER, GND_HTR; 2-circuit latching logic connector |
| J77 | 12V_FUSED, 12V_RTN; 12V_FUSED through F1 |
| Board | About 160 × 90 mm on metal standoffs; mains section under a clip-on insulating cover |

| Component | Connection |
| --- | --- |
| F0 | JAC L to the fused L node |
| RV1 | Fused L node to N |
| PSU5, PSU12 | AC/L to the fused L node; AC/N to N |
| PSU5 outputs | +V to the 5 V rail; -V to the SELV ground |
| PSU12 outputs | +V to F1; -V to 12V_RTN at J77; not connected to the SELV ground |
| F1, 1 A fast fuse in a PCB holder | PSU12 +V to 12V_FUSED at J77 |
| F2, 500 mA hold PTC | 5 V rail to JD-VCC at J74 |
| C3, 100 µF | JD-VCC at J74 to the SELV ground |
| Q71, AO3400A or equivalent logic-level N-channel MOSFET | Drain to HTR- at J75; source to the SELV ground; HTR+ at J75 to the 5 V rail |
| R72, 100 Ω | HEATER at J76 to the Q71 gate |
| R71, 10 kΩ | Q71 gate to the SELV ground |
| LED71 with R73, 1 kΩ | 5 V rail to the SELV ground; service indicator |
| JP1 | PE track to the SELV ground, single point |

Design rules for the mains section:

| Rule | Value |
| --- | --- |
| Creepage and clearance, mains to SELV and mains to PE | At least 6 mm on the board, with a milled slot under each module between its AC and DC pins |
| Board material | FR4 with CTI of 175 or better; no SELV copper under the module primary side |
| Mains copper | 1 mm track width or more at 35 µm for 2 A; keep-out to the board edge of 3 mm or more |
| Cover | Clip-on insulating cover over JAC, F0, RV1, and the module primaries; removable only with the enclosure open |
| Marking | Mains section outlined and labelled on the silkscreen; fuse rating printed beside F0 |

The 12 V rail stays isolated: its return goes only to J77 and the strike. The 5 V rail's ground is the SELV ground, bonded once to PE at JP1.

## Implementation status

These tables define both boards. Schematics, layouts, connector part numbers, and measured performance are pending. The harness drawings represent U5 and U7 as modules with logical terminal names.
