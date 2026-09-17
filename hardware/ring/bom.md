# Ring assembly BOM

Planning BOM for a 300 mm high × 150 mm wide sealed enclosure. Quantities cover the electrical assembly; mechanical hardware remains subject to enclosure design.

`n` is the number of buttons, from one to four. Components marked provisional require selection or qualification before procurement for production. Operating ranges come from manufacturer data where published; "verify" means no published rating was found.

Two custom boards exist, the key board U5 and the power board U7. Their circuit definitions are in [boards.md](boards.md); their parts are listed below as board contents.

## Modules and supplies

| Reference | Quantity | Part / specification | Operating range | Status |
| --- | --- | --- | --- | --- |
| P1 / J8 | 1 | Raspberry Pi Zero 2 W with 2×20 male header | -20 to +70 °C | Selected |
| SD1 | 1 | Industrial microSD, pSLC or high-endurance, 16 GB or more | -25 to +85 °C required | Class selected; part pending |
| U1 | 1 | Adafruit MAX98357A breakout, product 3006 | IC -40 to +85 °C; breakout unrated | Selected |
| SP1 | 1 | Adafruit enclosed speaker, product 3351, 4 Ω / 3 W | Verify | Factory JST-PH lead included |
| U2 | 1 | Adafruit SPH0645 microphone breakout, product 3421 | IC -40 to +85 °C; breakout unrated | Selected |
| CAM1 | 1 | Raspberry Pi Camera Module 3 Wide | 0 to +50 °C | Selected; enclosure operation under E10 |
| W14 | 1 | Raspberry Pi Standard-Mini camera cable, 200 mm | | Complete cable, not loose wire |
| U3 | 1 | Two-channel 5 V relay module with separate VCC/JD-VCC and active-low inputs | Songle coil -25 to +70 °C; verify module | Retained; E4 open |
| U5 | 1 | Ring key board, custom PCB, [definition](boards.md) | Per components | Schematic and layout pending |
| U6 | 1 | Adafruit SHT40 temperature and humidity breakout, product 4885 | Sensor -40 to +125 °C | Selected |
| U7 | 1 | Ring power board, custom PCB, [definition](boards.md) | Per components | Schematic and layout pending; E8 |
| HTR1 | 1 | 5 V polyimide film heater, about 3 W, adhesive backing | Verify | Provisional; E9 |
| SW1–SWn | n | Kailh BOX tactile switch, IP56, MX stem, 3-pin plate mount | Verify | Selected; heavier BOX variants acceptable |
| KC1–KCn | n | MX-compatible relegendable keycap, 1u or 2u | Verify UV stability | Part pending E11 |
| PSU5 | 1 | Mean Well IRM-30-5, PCB pins, 5 V / 6 A, Class II | -30 to +85 °C with derating above about 50 °C | Selected; on U7 |
| PSU12 | 1 | Mean Well IRM-30-12, PCB pins, 12 V / 2.5 A, Class II | -30 to +85 °C with derating above about 50 °C | Selected; on U7 |
| OPN | 1 | effeff 118, A7 winding, fail-locked, 10–24 V AC/DC | Verify with manufacturer | Mechanical strike variant matched to door |

## U5 key board contents

Resistors are 1%, at least 0.25 W or 0603 at 0.1 W where marked SMD. Ceramic capacitors are X7R, at least 16 V.

| Reference | Quantity | Part / specification | Placement |
| --- | --- | --- | --- |
| SK1–SK4 | 4 | Kailh MX hot-swap socket | All positions fitted |
| R11–R14 | 4 | 10 kΩ, SMD | Switch node pull-up to 3V3 |
| R21–R24 | 4 | 1 kΩ, SMD | Switch node to BUTTONn |
| C11–C14 | 4 | 100 nF, SMD | BUTTONn to GND beside J51 |
| D11–D14 | 4 | Nexperia PESD5V0S1BA bidirectional TVS, SOD-323 | Switch node to GND beside the socket |
| LED1–LED4 | 4 | White SMD LED, top mount, for the switch housing window | Under each switch |
| R31–R34 | 4 | 220 Ω, SMD | LED cathode to the Q1 drain node |
| Q1 | 1 | 2N7002 or equivalent logic-level N-channel MOSFET, SOT-23 | Backlight low-side switch |
| R41 | 1 | 10 kΩ, SMD | Q1 gate to GND |
| R42 | 1 | 100 Ω, SMD | LED_PWM to Q1 gate |
| J51 | 1 | 7-circuit latching connector, ≥ 1 A per contact, 26 AWG | Logic loom from J8 |
| J52 | 1 | 2-circuit latching connector, ≥ 1 A per contact, 22 AWG | LED supply from U7 |
| Standoffs | 4 | M3, height per E11 plate stack | Board to front plate |

## U7 power board contents

Resistors are 1%, at least 0.25 W or 0603 at 0.1 W where marked SMD. Electrolytics are low-ESR, at least 10 V, 105 °C rated.

| Reference | Quantity | Part / specification | Placement |
| --- | --- | --- | --- |
| JAC | 1 | 3-pole PCB screw terminal, ≥ 300 V / 16 A, accepts 1.5 mm² | Mains inlet: L, N, PE |
| FH0 / F0 | 1 | 5 × 20 mm PCB fuse holder with cover; fuse T 2 A, 250 V | L conductor |
| RV1 | 1 | Varistor S14K275 | L to N after F0; fit for exposed installations |
| FH1 / F1 | 1 | 5 × 20 mm PCB fuse holder; fuse 1 A fast, rated ≥ 32 V DC | 12 V positive |
| F2 | 1 | 500 mA hold-current PTC, rated ≥ 16 V | 5 V rail to JD-VCC; exact part pending E4 |
| C3 | 1 | 100 µF electrolytic | JD-VCC to SELV ground |
| Q71 | 1 | AO3400A or equivalent logic-level N-channel MOSFET, ≥ 20 V, ≥ 2 A, R_DS(on) ≤ 50 mΩ at V_GS 2.5 V | Heater low-side switch |
| R71 | 1 | 10 kΩ, SMD | Q71 gate to SELV ground |
| R72 | 1 | 100 Ω, SMD | HEATER to Q71 gate |
| LED71 / R73 | 1 each | Green SMD LED and 1 kΩ | 5 V present indicator |
| JP1 | 1 | 0 Ω link or solder jumper | PE track to SELV ground |
| J71 | 1 | 2-circuit terminal, ≥ 5 A per contact, 20 AWG | 5V_PI, GND_PI |
| J72–J75, J77 | 5 | 2-circuit terminals, ≥ 3 A per contact, 22 AWG | Amplifier, LED supply, relay coil, heater, 12 V |
| J76 | 1 | 2-circuit latching logic connector, 26 AWG | Heater control |
| PE1 | 1 set | Plated M4 hole, metal standoff, serrated washer; ring terminal pad for the front-plate bond | PE bonding |
| Cover | 1 | Clip-on insulating cover over the mains section, 94V-0 | Mains section |
| Standoffs | 4 | M4 metal standoffs to the backplate; PE1 is one of them | Board mounting |

## Off-board passives and interconnect

| Reference | Quantity | Part / specification | Placement / status |
| --- | --- | --- | --- |
| C1 | 1 | 470 µF electrolytic, low-ESR, 105 °C | U1 VIN to GND |
| C2 | 1 | 100 nF ceramic | U1 VIN to GND |
| D1 | 1 | 1N4007 | Across the strike coil, outside casing |
| JOUT1 / JOUT2 | 2 sets | Two-pole latching or screw terminal, ≥ 30 V DC / 2 A, accepts 22 AWG | Exact connector and mating parts pending enclosure layout |
| J8 loom | 1 set | Soldered, insulated leads into the header; splices for pin 1 | No stacked friction-fit housings |
| U1 straps | 2 | Insulated local links: VIN–SD and VIN–GAIN | Up to 20 mm each, 26 AWG |
| U2 strap | 1 | Insulated local link: pad 2–pad 6 | Up to 20 mm, 26 AWG |
| GSK2 | 1 | Silicone gasket sheet, 0.5 mm, with switch cutouts | Between the front plate and the switch flanges; E11 |
| VENT1 | 1 | PTFE pressure-equalization vent, M12, IP67 or better | Enclosure body |
| GL1 | 1 | IP68 cable gland M20 for the mains cable | Rear entry |
| GL2–GL3 | 2 | IP68 cable gland M16 for strike and gate cables | Rear entry |
| MEM1–MEM2 | 2 | Hydrophobic acoustic membrane for the speaker and microphone openings | Faceplate |
| WIN1 | 1 | Optical glass or anti-reflective acrylic camera window with gasket | Faceplate |
| GSK1 | 1 set | Closed-cell EPDM or silicone faceplate gaskets | Faceplate |
| TP1 | 1 | Thermal pad, SoC to backplate | Pi mounting |
| CC1 | 1 | Conformal coating for U1, U2, U5, U6, and the U7 low-voltage side | Assembly |

Loose-wire insulation is rated at least 60 V and for the qualified enclosure temperature. No loose mains wire exists inside the enclosure; the installation cable terminates on JAC. Include ferrules, heat-shrink, strain relief, and fasteners after connector and casing selection.

## Harness length estimate

Lengths are cut allowances per conductor, including approximately 30–50 mm for termination and service movement. They are estimates, not released manufacturing dimensions.

Assume the camera and U6 occupy the upper face, the microphone sits below them at least 100 mm from the speaker grille, the Pi with U1 forms a central cluster, the speaker sits below that cluster, the key board sits behind the lower face, and U7 sits in the rear bottom compartment.

| Cable | Route | Conductors | Length each | Total conductor length |
| --- | --- | --- | --- | --- |
| WAC | Installation cable to U7 JAC | 3 × 1.5 mm² | Installation-specific | Excluded |
| W1 | U7 PE_FRONT to the front plate | 1 × 1.0 mm² | 250 mm | 0.25 m |
| W2 | U7 J71 to J8 pins 2 and 6 | 2 × 20 AWG | 200 mm | 0.40 m |
| W3 | U7 J72 to U1 supply | 2 × 22 AWG | 200 mm | 0.40 m |
| W4 | U7 J74 to relay coil supply | 2 × 22 AWG | 200 mm | 0.40 m |
| W5 | U7 J75 to heater | 2 × 22 AWG | 350 mm | 0.70 m |
| W6 | U7 J73 to U5 J52 | 2 × 22 AWG | 100 mm | 0.20 m |
| W7 | U7 J77 12 V fused to relay COM1 | 1 × 22 AWG | 150 mm | 0.15 m |
| W8 | Relay NO1 to JOUT1 positive | 1 × 22 AWG | 100 mm | 0.10 m |
| W9 | U7 J77 12 V return to JOUT1 return | 1 × 22 AWG | 150 mm | 0.15 m |
| W10 | Relay COM2 and NO2 to JOUT2 | 2 × 22 AWG | 100 mm | 0.20 m |
| W11 | J8 to amplifier I2S | 3 × 26 AWG | 100 mm | 0.30 m |
| W12 | U1 to speaker | 2 × factory lead | 100 mm | 0.20 m, included with SP1 |
| W13 | J8 to microphone | 5 × 26 AWG | 200 mm | 1.00 m |
| W14 | CSI camera ribbon | Complete 22-to-15-pin cable | 200 mm | 1 cable |
| W15 / W16 | U1 VIN to SD / GAIN | 2 × 26 AWG | 20 mm | 0.04 m |
| W17 | J8 to U5 J51 | 7 × 26 AWG | 250 mm | 1.75 m |
| W18 | J8 to relay logic | 3 × 26 AWG | 200 mm | 0.60 m |
| W19 | J8 to U6 sensor | 4 × 26 AWG | 250 mm | 1.00 m |
| W20 | J8 to U7 J76 heater control | 2 × 26 AWG | 200 mm | 0.40 m |
| U2 local strap | Microphone GND to SEL | 1 × 26 AWG | 20 mm | 0.02 m |

The button count no longer changes the harness: W17 always carries all seven conductors, and the switches sit on U5.

| Procurement allowance | Any button count |
| --- | --- |
| 1.0 mm² green-yellow | 0.25 m cut; 0.30 m with 20% waste |
| 20 AWG internal wire | 0.40 m cut; 0.48 m with 20% waste |
| 22 AWG internal wire | 2.30 m cut; 2.76 m with 20% waste |
| 26 AWG internal wire, including 60 mm of local straps | 5.11 m cut; 6.13 m with 20% waste |
| Speaker lead | 100 mm pair from included factory lead |
| Camera ribbon | One 200 mm cable |

Allow approximately 9.7 m of loose wire, distributed across the listed gauges and colours. Factory speaker and camera cables are additional.

The mains supply cable, strike cable, and gate cable are outside this internal-wire estimate. Their routes require an installation survey; the strike cable also requires voltage-drop verification against the 11–13 V window.

## Drawing scope

Three WireViz sources hold the harness lengths: [wiring-power.yml](wiring-power.yml), [wiring-audio.yml](wiring-audio.yml), and [wiring-io.yml](wiring-io.yml). Their generated TSV files exclude modules, boards, and passives and model CSI as one logical connection. Use this document for procurement.

Board layouts, connector part numbers, mechanical items, and qualification results remain open. This BOM supports prototype planning, not a production release.
