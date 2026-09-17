# Ring assembly BOM

Planning BOM for a 300 mm high × 150 mm wide enclosure. Quantities cover the electrical assembly; mechanical hardware remains subject to enclosure design.

`n` is the number of buttons, from one to four. Components marked provisional require selection or qualification before procurement for production.

## Modules and supplies

| Reference | Quantity | Part / specification | Status |
| --- | --- | --- | --- |
| P1 / J8 | 1 | Raspberry Pi Zero W with 2×20 male header | Selected |
| PSU5 | 1 | Official Raspberry Pi 12.5 W micro-USB supply, 5.1 V / 2.5 A | Indoor installation |
| U1 | 1 | Adafruit MAX98357A breakout, product 3006 | Selected |
| SP1 | 1 | Adafruit enclosed speaker, product 3351, 4 Ω / 3 W | Factory JST-PH lead included |
| U2 | 1 | Adafruit SPH0645 microphone breakout, product 3421 | Selected |
| CAM1 | 1 | Raspberry Pi Camera Module 3 Standard | Selected |
| W6 | 1 | Raspberry Pi Standard-Mini camera cable, 200 mm | Complete cable, not loose wire |
| U3 | 1 | Two-channel 5 V relay module with separate VCC/JD-VCC and active-low inputs | Provisional; E4 deferred |
| U5 | 1 | Custom button interface assembly, [circuit definition](button-input.md) | PCB and connectors pending; parts below are its contents |
| SW1–SWn | n | 16 mm-class IP67 SPST-NO momentary button, gold contacts suitable for ≤ 6 mA switching | Exact part deferred to enclosure design |
| PSU12 | 1 | Mean Well GST18E12-P1J, 12 V / 1.5 A | Enclosed indoor adapter; replaces bare IRM module |
| OPN | 1 | effeff 118, A7 winding, fail-locked, 10–24 V AC/DC | Mechanical strike variant matched to door |

## Passives and interconnect

Resistors are 1%, at least 0.25 W. Ceramic capacitors are X7R, at least 16 V. Electrolytics are low-ESR, at least 10 V, 105 °C rated.

| Reference | Quantity | Part / specification | Placement / status |
| --- | --- | --- | --- |
| C1 | 1 | 470 µF electrolytic | U1 VIN to GND |
| C2 | 1 | 100 nF ceramic | U1 VIN to GND |
| C3 | 1 | 100 µF electrolytic | U3 JD-VCC to GND, after F2 |
| R11–R14 | 4 | 10 kΩ pull-up | U5; fitted on every variant |
| R15–R16 | 2 | 10 kΩ relay pull-up | U5; fitted on every variant |
| U11–U1n | n | Vishay VO617A-3, DIP-4 | U5 button channels |
| R21–R2n | n | 330 Ω LED series resistor | U5 button channels |
| R31–R3n | n | 1 kΩ output series resistor | U5 button channels |
| C11–C1n | n | 100 nF ceramic | U5 GPIO filters |
| D11–D1n | n | Nexperia 1N4148 | U5 supply blocking diodes |
| D21–D2n | n | Nexperia 1N4148 | U5 reverse LED clamps |
| D31–D3n | n | Nexperia PESD5V0S1UL | U5 field-side transient suppression |
| F1 | 1 | 1 A fast fuse, rated ≥ 32 V DC | Fuse and insulated holder selected together; fault coordination pending |
| F2 | 1 | 500 mA hold-current PTC, rated ≥ 16 V | Relay supply; exact part and hot resistance pending E4 |
| D1 | 1 | 1N4007 | Across the strike coil, outside casing |
| J12 / JOUT | 2 sets | Two-pole latching or screw terminal, ≥ 30 V DC / 2 A, accepts 22 AWG | Exact connector and mating parts pending enclosure layout |
| JDC | 1 | Insulated 5.5 × 2.1 mm centre-positive socket-to-terminal adapter, ≥ 24 V DC / 2 A | Indoor connection from PSU12 to installation cable |
| U5 connectors | 1 set | Logic: 9 circuits; field: n+1 circuits; ≥ 12 V / 0.1 A per contact | Footprints, housings, and contacts pending U5 layout |
| J8 branches | 1 set | Soldered, insulated header loom with power and clock splices | No stacked friction-fit housings |
| U1 straps | 2 | Insulated local links: VIN–SD and VIN–GAIN | Up to 20 mm each, 26 AWG |
| U2 strap | 1 | Insulated local link: pad 2–pad 6 | Up to 20 mm, 26 AWG |

Loose-wire insulation is rated at least 60 V and for the qualified enclosure temperature. Include ferrules, heat-shrink, strain relief, and fasteners after connector and casing selection.

## Harness length estimate

Lengths are cut allowances per conductor, including approximately 30–50 mm for termination and service movement. They are estimates, not released manufacturing dimensions.

Assume the camera occupies the upper face, the speaker and audio electronics form a central cluster, and buttons occupy the lower face. Place the microphone near that cluster's lower edge.

The Pi-to-microphone route must fit within 150 mm. If the microphone moves to the bottom of the full-height casing, move the Pi/audio cluster with it or revise the interconnect design.

| Cable | Route | Conductors | Length each | Total conductor length |
| --- | --- | --- | --- | --- |
| W1 | J8 to U1 supply | 2 × 22 AWG | 100 mm | 0.20 m |
| W2 | U1 to speaker | 2 × factory lead | 100 mm | 0.20 m, included with SP1 |
| W3 | J8 to microphone | 5 × 26 AWG | 150 mm | 0.75 m |
| W4 | J8 to relay logic | 4 × 26 AWG | 200 mm | 0.80 m |
| W5 | U5 to button contacts and return | (n+1) × 26 AWG | 300 mm | 0.60 / 0.90 / 1.20 / 1.50 m for 1 / 2 / 3 / 4 buttons |
| W6 | CSI camera ribbon | Complete 22-to-15-pin cable | 200 mm | 1 cable |
| W7 | J8 to amplifier I2S | 3 × 26 AWG | 100 mm | 0.30 m |
| W8 | J8 5 V to relay PTC | 1 × 22 AWG | 50 mm | 0.05 m |
| W9 | J12 positive to F1 | 1 × 22 AWG | 100 mm | 0.10 m |
| W10 | F1 to relay COM1 | 1 × 22 AWG | 100 mm | 0.10 m |
| W11 | Relay NO1 to JOUT positive | 1 × 22 AWG | 100 mm | 0.10 m |
| W12 | J12 return to JOUT return | 1 × 22 AWG | 100 mm | 0.10 m |
| W13 | Relay PTC to JD-VCC | 1 × 22 AWG | 100 mm | 0.10 m |
| W14 | J8 to U5 logic, power, and relay pull-ups | 9 × 26 AWG | 100 mm | 0.90 m |
| W15 / W16 | U1 VIN to SD / GAIN | 2 × 26 AWG | 20 mm | 0.04 m |
| U2 local strap | Microphone GND to SEL | 1 × 26 AWG | 20 mm | 0.02 m |

W5 uses a conservative 300 mm allowance for every populated signal and the shared return. Trim individual branches during enclosure fitting. W14 retains all four GPIO connections for the permanent pull-ups.

| Procurement allowance | 1-button assembly | 4-button assembly |
| --- | --- | --- |
| 22 AWG internal wire | 0.75 m cut; 0.90 m with 20% waste | 0.75 m cut; 0.90 m with 20% waste |
| 26 AWG internal wire, including 60 mm of local straps | 3.41 m cut; 4.10 m with 20% waste | 4.31 m cut; 5.18 m with 20% waste |
| Speaker lead | 100 mm pair from included factory lead | Same |
| Camera ribbon | One 200 mm cable | Same |

Allow approximately 5.0 m of loose wire for one button or 6.1 m for four buttons, distributed across the listed gauges and colours. Factory speaker and camera cables are additional.

The official micro-USB supply lead, indoor 12 V adapter lead, wall cable, and strike cable are outside this internal-wire estimate. Their routes require an installation survey and voltage-drop verification.

## Drawing scope

[wiring.yml](wiring.yml) contains the four-button harness lengths. Its generated TSV excludes modules and passives and models CSI as one logical connection. Use this document for procurement.

Final connector part numbers, mechanical items, U5 fabrication details, and qualification results remain open. This BOM supports prototype planning, not a production release.
