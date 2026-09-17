# Hardware qualification

Status: prototype design; no electrical bench results or enclosure qualification recorded.

## Electrical acceptance

| Check | Acceptance criterion | Status |
| --- | --- | --- |
| Wiring | Continuity matches terminal names and U2 pad numbers; no 5 V-to-3.3 V connection; the 12 V return connects only to U7 J77 and JOUT1 | Pending assembly |
| U7 mains section | Creepage and clearance of 6 mm or more from mains copper to SELV copper and to PE, measured on the bare board; dielectric strength 3 kV AC for 60 s from mains to SELV and from mains to PE with RV1 removed, before first power-up; insulation resistance of 100 MΩ or more at 500 V DC | Pending bare board |
| PE bonding | Continuity of 0.1 Ω or less from the JAC PE terminal to the backplate standoff, to the front plate, and to the SELV ground through JP1 | Pending assembly |
| Pi rails | 4.75–5.25 V at J8 pin 2 and at U7 J71 under startup and combined load; 3.3 V within ±5%; no resets or audio/camera faults | Pending measurement |
| Load case | Camera autofocus + encoding + Wi-Fi transmission + maximum permitted acknowledgement + both coils + backlight + heater | Pending measurement |
| Regulator margin | Record camera and onboard rail demand, peaks, temperature, and justified margin against the applicable regulator limits | Pending measurement |
| Audio | Correct left-slot capture/playback at 48 kHz; measured capture-to-playback offset; noise, clipping, and rail excursions | Pending firmware and bench |
| Intercom | Echo return loss of 20 dB or better before cancellation at the default clamps; stable full-duplex conversation with the browser test page; sound pressure at 1 m recorded | Pending firmware and bench |
| Buttons | Open GPIO ≥ 1.6 V through the U5 pull-up; closed GPIO ≤ 0.9 V; no phantom press during boot; every hot-swap position accepts and releases a switch | Pending U5 prototype |
| Backlight | PWM dims from off to full without visible flicker; off at boot; 36 mA or less at full | Pending U5 prototype |
| Field transients | Initial target: ±4 kV contact and ±8 kV air ESD at the keycaps and the front plate, plus ±1 kV EFT through a coupling clamp on W17 | Pending lab validation |
| Surge | ±1 kV line-to-line on the strike and gate cables; mains immunity per the module specification | Pending lab validation |
| Transient result | Powered and unpowered: no damage, supply back-powering, unintended strike or gate activation, or persistent malfunction; GPIO stays within published device limits | Pending lab validation |
| Strike | 11–13 V at the energized coil; release after the commanded 1–3 s pulse; repeated operation and power interruption | Pending assembly |
| Gate output | Contact closes for the commanded 0.5–1 s pulse; open otherwise, including boot and shutdown | Pending assembly |
| Fault protection | Output short and coil fault remain within wire/component thermal limits; F1 clears against PSU12 hiccup behavior; F0 holds two simultaneous cold starts | Pending selected fuse test |
| Heater | Q71 switches fully from a 3.3 V GPIO; HTR1 raises the camera carrier above 0 °C at -20 °C ambient; off at boot; duty limit honoured; dew point tracked from U6 | Pending prototype |

The ESD, EFT, and surge levels are initial engineering targets, not a compliance claim. Final installation conditions determine the applicable product test plan.

Record test supply settings, wiring lengths, instrument setup, waveforms, temperatures, and results. Typical datasheet curves do not substitute for guaranteed limits or measurements.

## Environmental acceptance

| Check | Acceptance criterion | Status |
| --- | --- | --- |
| Sealing | IP65 spray test on the assembled enclosure; no ingress at glands, vent, window, keycaps, or acoustic ports; no water on U5 after the test | Pending enclosure |
| Thermal soak | 24 h at -20 °C and at +50 °C ambient with a solar lamp on the front; all functions operate, including switch feel; internal temperatures at the camera, Pi, and U7 modules recorded | Pending enclosure |
| Condensation | Temperature cycling with humid air; no condensation on the camera window or boards; heater control keeps the internal air above the dew point | Pending enclosure |
| Radio | Wi-Fi RSSI at the installation site survey; Bluetooth scanning during a live call; both meet the firmware thresholds through the RF window | Pending enclosure |
| Acoustic ports | Membranes cost no more than 3 dB on playback and capture | Pending enclosure |
| Keycaps | Legends readable after the UV and thermal soaks; switches actuate with gloves | Pending enclosure |

## Assembly requirements

| Item | Requirement |
| --- | --- |
| Mains | Installation cable terminated on U7 JAC inside the partitioned compartment; U7 mains cover fitted; PE to the standoff, the front plate, and JP1; installation by a qualified electrician behind a 10 A breaker and a 30 mA residual-current device |
| Harness | Soldered or latching production joints; strain relief at cable entries; no stacked Dupont housings; polarized 20 AWG 5 V feed into J8 pins 2 and 6; no inline components |
| Speaker | Verify polarity from terminal markings; ferrules suitable for the actual factory lead; no SPK terminal-to-ground connection; decoupled mounting |
| Microphone | Bottom port gasketed to its own opening behind a membrane; at least 100 mm from the speaker grille |
| Key board | Switches seated in the sockets through the front plate with the gasket sheet in place; keycaps fitted last; blank inserts in unused positions |
| Passives | C1/C2 at amplifier VIN; D1 physically across the strike coil |
| Moisture | IP65 sealed body with a PTFE vent, gasketed faceplate, glands at the rear entry, conformal coating on every board outside connectors and sockets; no drain holes through the sealed volume |
| Thermal | Thermal pad from the SoC to the backplate; heater bonded to the camera and microphone carrier; light-coloured or shaded front preferred |

## Deferred to enclosure design

| Review item | Deferred work | Closure evidence |
| --- | --- | --- |
| E4: relay module | Verify the retained module: input current at 3.3 V VCC, pickup and dropout margins, startup and power-loss behaviour, terminal layout, footprint | Schematic, measured input current, margins, and mechanical fit |
| E7: antenna | Define the RF window over the Zero 2 W antenna and the dimensioned metal keep-out; route the front plate, PE bond, and harness around it | Board-relative enclosure drawing and installed Wi-Fi and Bluetooth measurements |
| E8: power board | U7 schematic and layout: creepage slots, cover, terminal and fuse-holder parts, standoff positions, compartment fit | Released U7 design files and the bare-board measurements above |
| E9: heater | Heater part, power, placement, Q71 drive margin, and the dew-point control limits | Thermal soak, condensation results, and measured Q71 on-state at 0.6 A |
| E10: camera environment | Operation of the 0 to 50 °C rated camera inside the sealed enclosure across -20 to +50 °C ambient with solar load; window material and autofocus behaviour when cold | Thermal soak results and image checks at the corners |
| E11: key sealing | Front-plate cutouts, gasket sheet, keycap choice, water path from the keycap gap to the outside, blank inserts, U5 standoff height | Faceplate drawing, U5 outline, and the sealing test above |
| Mechanical selection | Connectors for U5 and U7, standoffs, sealing parts, vent, glands, window, and casing depth | Released mechanical BOM and drawings |

Relay wiring is provisional. No relay resistor substitution or driver redesign is approved in this step.

## Scope exclusions

Firmware, automated pin checks, CI regeneration checks for the drawings, infrared illumination, tamper detection, and a solid-state strike driver are outside this hardware step. Schematic capture and layout of U5 and U7 are the next hardware step; this specification fixes their interfaces. The enclosure reserves space and a 12 V feed for the infrared upgrade.
