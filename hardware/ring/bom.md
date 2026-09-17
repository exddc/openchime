# Ring assembly BOM

Prototype BOM for a 300 × 150 mm enclosure; depth and mechanical parts remain open. `n` is the button count, 1–4. Provisional parts require selection or qualification; “verify” marks an unconfirmed operating range. U5/U7 circuits are defined in [boards.md](boards.md).

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
| SW1–SWn | n | Kailh BOX tactile, MX stem, 3-pin plate mount; IP56 and dry-contact rating required | Verify | Family selected; exact variant pending |
| KC1–KCn | n | MX-compatible relegendable keycap, 1u or 2u | Verify UV stability | Part pending E11 |
| PSU5 | 1 | Mean Well IRM-30-5, PCB pins, 5 V / 6 A, Class II | -30 to +85 °C; apply datasheet derating | Selected; on U7 |
| PSU12 | 1 | Mean Well IRM-30-12, PCB pins, 12 V / 2.5 A, Class II | -30 to +85 °C; apply datasheet derating | Selected; on U7 |
| OPN | 1 | effeff 118, A7 winding, fail-locked, 10–24 V AC/DC | Verify with manufacturer | Mechanical strike variant matched to door |

## U5 key board contents

Resistors are 1%, at least 0.25 W or 0603 at 0.1 W where marked SMD. Ceramic capacitors are X7R, at least 16 V.

| Reference | Quantity | Part / specification |
| --- | --- | --- |
| SK1–SK4 | 4 | Kailh MX hot-swap socket |
| R11–R14 | 4 | 10 kΩ, SMD |
| R21–R24 | 4 | 1 kΩ, SMD |
| C11–C14 | 4 | 100 nF, SMD |
| D11–D14 | 4 | Nexperia PESD5V0S1BA bidirectional TVS, SOD-323 |
| LED1–LEDn | n | White SMD LED, top mount, for the switch housing window |
| R31–R3n | n | 220 Ω, SMD |
| Q1 | 1 | N-channel MOSFET with guaranteed on-resistance at ≤ 3.3 V; 2N7002 provisional |
| R41 | 1 | 10 kΩ, SMD |
| R42 | 1 | 100 Ω, SMD |
| J51 | 1 | 7-circuit latching connector, ≥ 1 A per contact, 26 AWG |
| J52 | 1 | 2-circuit latching connector, ≥ 1 A per contact, 22 AWG |
| Standoffs | 4 | M3, height per E11 plate stack |

## U7 power board contents

Resistors are 1%, at least 0.25 W or 0603 at 0.1 W where marked SMD. Electrolytics are low-ESR, at least 10 V, 105 °C rated.

| Reference | Quantity | Part / specification |
| --- | --- | --- |
| JAC | 1 | 3-pole PCB screw terminal, ≥ 300 V / 16 A, accepts 1.5 mm² |
| FH0 / F0 | 1 | 5 × 20 mm PCB fuse holder with cover; fuse T 2 A, 250 V |
| RV1 | 1 | Varistor S14K275; fit for exposed installations |
| FH1 / F1 | 1 | 5 × 20 mm PCB fuse holder; fuse 1 A fast, rated ≥ 32 V DC |
| F2 | 1 | 500 mA hold-current PTC, rated ≥ 16 V; exact part pending E4 |
| C3 | 1 | 100 µF electrolytic |
| Q71 | 1 | AO3400A or equivalent logic-level N-channel MOSFET, ≥ 20 V, ≥ 2 A, R_DS(on) ≤ 50 mΩ at V_GS 2.5 V |
| R71 | 1 | 10 kΩ, SMD |
| R72 | 1 | 100 Ω, SMD |
| LED71 / R73 | 1 each | Green SMD LED and 1 kΩ |
| JP1 | 1 | 0 Ω link or solder jumper |
| J71 | 1 | 2-circuit terminal, ≥ 5 A per contact, 20 AWG |
| J72–J75, J77 | 5 | 2-circuit terminals, ≥ 3 A per contact, 22 AWG |
| J76 | 1 | 2-circuit latching logic connector, 26 AWG |
| PE1 | 1 set | Plated M4 hole, metal standoff, serrated washer; ring terminal pad for the front-plate bond |
| Cover | 1 | Clip-on insulating cover over the mains section, 94V-0 |
| Standoffs | 4 | M4 metal standoffs to backplate; one is PE1 |

## Off-board passives and interconnect

| Reference | Quantity | Part / specification | Placement / status |
| --- | --- | --- | --- |
| C1 | 1 | 470 µF electrolytic, ≥ 10 V, low-ESR, 105 °C | U1 VIN to GND |
| C2 | 1 | 100 nF X7R, ≥ 16 V | U1 VIN to GND |
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
| CC1 | 1 | Coating for exposed PCB areas on U1, U2, U5, U6 and U7 low-voltage side | Mask ports, sensor, optics, connectors, and sockets; qualify compatibility |

Loose-wire insulation is rated at least 60 V and for the qualified enclosure temperature. No loose mains wire exists inside the enclosure; the installation cable terminates on JAC. Include ferrules, heat-shrink, strain relief, and fasteners after connector and casing selection.

## Harness length estimate

Estimated cut lengths include 30–50 mm for termination and movement. Layout assumes camera/sensor/microphone at the top, Pi/amplifier centrally, speaker and keys below, and U7 at the rear bottom. Confirm routes against enclosure drawings before cutting.

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

W17 has seven conductors for every button count.

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

WireViz lengths are in [power](wiring-power.yml), [audio](wiring-audio.yml), and [I/O](wiring-io.yml). Generated wire reports omit boards, modules, and passives; use this BOM for procurement. This is not a production release.
