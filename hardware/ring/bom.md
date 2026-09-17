# Ring assembly BOM

Planning BOM for a 300 mm high × 150 mm wide sealed enclosure. Quantities cover the electrical assembly; mechanical hardware remains subject to enclosure design.

`n` is the number of buttons, from one to four. Components marked provisional require selection or qualification before procurement for production. Operating ranges come from manufacturer data where published; "verify" means no published rating was found.

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
| U5 | 1 | Ring I/O board, custom assembly, [circuit definition](button-input.md) | Per components | PCB and connectors pending; parts below are its contents |
| U6 | 1 | Adafruit SHT40 temperature and humidity breakout, product 4885 | Sensor -40 to +125 °C | Selected |
| HTR1 | 1 | 5 V polyimide film heater, about 3 W, adhesive backing | Verify | Provisional; E9 |
| SW1–SWn | n | 16 mm-class IP67 anti-vandal SPST-NO momentary button, gold contacts suitable for low-current switching | Typically -20 to +55 °C; verify | Exact part deferred to enclosure design |
| PSU5 | 1 | Mean Well IRM-30-5ST, 5 V / 6 A, Class II, screw terminals | -30 to +85 °C with derating above about 50 °C | Selected |
| PSU12 | 1 | Mean Well IRM-30-12ST, 12 V / 2.5 A, Class II, screw terminals | -30 to +85 °C with derating above about 50 °C | Selected |
| OPN | 1 | effeff 118, A7 winding, fail-locked, 10–24 V AC/DC | Verify with manufacturer | Mechanical strike variant matched to door |

## Passives and interconnect

Resistors are 1%, at least 0.25 W. Ceramic capacitors are X7R, at least 16 V. Electrolytics are low-ESR, at least 10 V, 105 °C rated.

| Reference | Quantity | Part / specification | Placement / status |
| --- | --- | --- | --- |
| C1 | 1 | 470 µF electrolytic | U1 VIN to GND |
| C2 | 1 | 100 nF ceramic | U1 VIN to GND |
| C3 | 1 | 100 µF electrolytic | U5, JD-VCC after F2 to the star ground |
| R11–R14 | 4 | 10 kΩ pull-up | U5; fitted on every variant |
| R15–R16 | 2 | 10 kΩ relay pull-up | U5; fitted on every variant |
| U11–U1n | n | Vishay VO617A-3, DIP-4 | U5 button channels |
| R21–R2n | n | 330 Ω LED series resistor | U5 button channels |
| R31–R3n | n | 1 kΩ output series resistor | U5 button channels |
| C11–C1n | n | 100 nF ceramic | U5 GPIO filters |
| D11–D1n | n | Nexperia 1N4148 | U5 supply blocking diodes |
| D21–D2n | n | Nexperia 1N4148 | U5 reverse LED clamps |
| D31–D3n | n | Nexperia PESD5V0S1UL | U5 field-side transient suppression |
| Q1 | 1 | Logic-level N-channel MOSFET, ≥ 20 V, ≥ 2 A, R_DS(on) ≤ 50 mΩ at V_GS 2.5 V, for example AO3400A | U5 heater switch |
| R41 | 1 | 10 kΩ | U5, Q1 gate to ground |
| R42 | 1 | 100 Ω | U5, HEAT to Q1 gate |
| F0 | 1 | 5 × 20 mm, T 2 A, 250 V, with touch-protected holder | Mains compartment, L conductor |
| F1 | 1 | 1 A fast fuse, rated ≥ 32 V DC | Strike branch; fuse and insulated holder selected together |
| F2 | 1 | 500 mA hold-current PTC, rated ≥ 16 V | U5, relay coil supply; exact part pending E4 |
| D1 | 1 | 1N4007 | Across the strike coil, outside casing |
| J_AC | 1 | Three-pole mains terminal, ≥ 250 V / 16 A, touch-safe, accepts 1.5 mm² | Mains compartment; part pending enclosure layout |
| PE1 | 1 set | M4 stud with serrated washers and three ring terminals | Metal backplate |
| JOUT1 / JOUT2 | 2 sets | Two-pole latching or screw terminal, ≥ 30 V DC / 2 A, accepts 22 AWG | Exact connector and mating parts pending enclosure layout |
| U5 connectors | 1 set | Power: 13 circuits, ≥ 5 A on 5V_IN and GND_IN; logic: 9 circuits; field: n+1 circuits | Footprints, housings, and contacts pending U5 layout |
| J8 feed | 1 set | Soldered, insulated loom into pins 2, 4, 6, and 14; splices for pin 1 | No stacked friction-fit housings |
| U1 straps | 2 | Insulated local links: VIN–SD and VIN–GAIN | Up to 20 mm each, 26 AWG |
| U2 strap | 1 | Insulated local link: pad 2–pad 6 | Up to 20 mm, 26 AWG |
| VENT1 | 1 | PTFE pressure-equalization vent, M12, IP67 or better | Enclosure body |
| GL1 | 1 | IP68 cable gland M20 for the mains cable | Rear entry |
| GL2–GL3 | 2 | IP68 cable gland M16 for strike and gate cables | Rear entry |
| MEM1–MEM2 | 2 | Hydrophobic acoustic membrane for the speaker and microphone openings | Faceplate |
| WIN1 | 1 | Optical glass or anti-reflective acrylic camera window with gasket | Faceplate |
| GSK1 | 1 set | Closed-cell EPDM or silicone faceplate and button gaskets | Faceplate |
| TP1 | 1 | Thermal pad, SoC to backplate | Pi mounting |
| CC1 | 1 | Conformal coating for U1, U2, U5, and U6 | Assembly |

Loose-wire insulation is rated at least 60 V on the low-voltage side and 300/500 V double-insulated on the mains side, and for the qualified enclosure temperature. Include ferrules, heat-shrink, strain relief, and fasteners after connector and casing selection.

## Harness length estimate

Lengths are cut allowances per conductor, including approximately 30–50 mm for termination and service movement. They are estimates, not released manufacturing dimensions.

Assume the camera and U6 occupy the upper face, the microphone sits below them at least 100 mm from the speaker grille, the Pi with U1 and U5 forms a central cluster, the speaker sits below that cluster, buttons occupy the lower face, and the mains compartment sits at the rear bottom.

| Cable | Route | Conductors | Length each | Total conductor length |
| --- | --- | --- | --- | --- |
| WM1 | J_AC L to F0 | 1 × 0.75 mm² | 100 mm | 0.10 m |
| WM2, WM3 | F0 to PSU5 and PSU12 L | 2 × 0.75 mm² | 150 mm | 0.30 m |
| WM4, WM5 | J_AC N to PSU5 and PSU12 N | 2 × 0.75 mm² | 150 mm | 0.30 m |
| WM6 | J_AC PE to PE1 | 1 × 1.0 mm² | 150 mm | 0.15 m |
| WM7 | PE1 to U5 star | 1 × 1.0 mm² | 200 mm | 0.20 m |
| WM8 | PE1 to front plate | 1 × 1.0 mm² | 250 mm | 0.25 m |
| W1 | PSU5 to U5 | 2 × 20 AWG | 150 mm | 0.30 m |
| W2 | U5 to J8 pins 2, 4, 6, 14 | 4 × 22 AWG | 100 mm | 0.40 m |
| W3 | U5 to U1 supply | 2 × 22 AWG | 100 mm | 0.20 m |
| W4 | U5 to relay coil supply | 2 × 22 AWG | 200 mm | 0.40 m |
| W5 | U5 to heater | 2 × 22 AWG | 250 mm | 0.50 m |
| W6 | PSU12 positive to F1 | 1 × 22 AWG | 100 mm | 0.10 m |
| W7 | F1 to relay COM1 | 1 × 22 AWG | 100 mm | 0.10 m |
| W8 | Relay NO1 to JOUT1 positive | 1 × 22 AWG | 100 mm | 0.10 m |
| W9 | PSU12 return to JOUT1 return | 1 × 22 AWG | 150 mm | 0.15 m |
| W10 | Relay COM2 and NO2 to JOUT2 | 2 × 22 AWG | 100 mm | 0.20 m |
| W11 | J8 to amplifier I2S | 3 × 26 AWG | 100 mm | 0.30 m |
| W12 | U1 to speaker | 2 × factory lead | 100 mm | 0.20 m, included with SP1 |
| W13 | J8 to microphone | 5 × 26 AWG | 200 mm | 1.00 m |
| W14 | CSI camera ribbon | Complete 22-to-15-pin cable | 200 mm | 1 cable |
| W15 / W16 | U1 VIN to SD / GAIN | 2 × 26 AWG | 20 mm | 0.04 m |
| W17 | J8 to U5 logic, pull-ups, and heater control | 9 × 26 AWG | 100 mm | 0.90 m |
| W18 | J8 to relay logic | 3 × 26 AWG | 200 mm | 0.60 m |
| W19 | U5 to button contacts and return | (n+1) × 26 AWG | 300 mm | 0.60 / 0.90 / 1.20 / 1.50 m for 1 / 2 / 3 / 4 buttons |
| W20 | J8 to U6 sensor | 4 × 26 AWG | 250 mm | 1.00 m |
| U2 local strap | Microphone GND to SEL | 1 × 26 AWG | 20 mm | 0.02 m |

W19 uses a conservative 300 mm allowance for every populated signal and the shared return. Trim individual branches during enclosure fitting. W17 retains all four GPIO connections for the permanent pull-ups.

| Procurement allowance | 1-button assembly | 4-button assembly |
| --- | --- | --- |
| 0.75 mm² mains wire, brown and blue | 0.70 m cut; 0.84 m with 20% waste | Same |
| 1.0 mm² green-yellow | 0.60 m cut; 0.72 m with 20% waste | Same |
| 20 AWG internal wire | 0.30 m cut; 0.36 m with 20% waste | Same |
| 22 AWG internal wire | 2.15 m cut; 2.58 m with 20% waste | Same |
| 26 AWG internal wire, including 60 mm of local straps | 4.46 m cut; 5.35 m with 20% waste | 5.36 m cut; 6.43 m with 20% waste |
| Speaker lead | 100 mm pair from included factory lead | Same |
| Camera ribbon | One 200 mm cable | Same |

Allow approximately 10 m of loose wire for one button or 11 m for four buttons, distributed across the listed gauges and colours. Factory speaker and camera cables are additional.

The mains supply cable, strike cable, and gate cable are outside this internal-wire estimate. Their routes require an installation survey; the strike cable also requires voltage-drop verification against the 11–13 V window.

## Drawing scope

Three WireViz sources hold the four-button harness lengths: [wiring-power.yml](wiring-power.yml), [wiring-audio.yml](wiring-audio.yml), and [wiring-io.yml](wiring-io.yml). Their generated TSV files exclude modules and passives and model CSI as one logical connection. Use this document for procurement.

Final connector part numbers, mechanical items, U5 fabrication details, and qualification results remain open. This BOM supports prototype planning, not a production release.
