# Hardware qualification

Status: prototype design; no electrical bench results or enclosure qualification recorded.

## Electrical acceptance

| Check | Acceptance criterion | Status |
| --- | --- | --- |
| Wiring | Continuity matches terminal names and U2 pad numbers; no 5 V-to-3.3 V connection; strike return connects only to PSU12 and JOUT1 | Pending assembly |
| Mains safety | PE continuity from the supply PE to the front plate and PE1 at 0.1 Ω or less; insulation resistance from mains terminals to SELV and PE of 100 MΩ or more at 500 V DC; 6 mm creepage and clearance verified on the assembled compartment | Pending assembly |
| Pi rails | 4.75–5.25 V at J8 pins 2 and 4 and at U5 under startup and combined load; 3.3 V within ±5%; no resets or audio/camera faults | Pending measurement |
| Load case | Camera autofocus + encoding + Wi-Fi transmission + maximum permitted acknowledgement + both coils + all buttons + heater | Pending measurement |
| Regulator margin | Record camera and onboard rail demand, peaks, temperature, and justified margin against the applicable regulator limits | Pending measurement |
| Audio | Correct left-slot capture/playback at 48 kHz; measured capture-to-playback offset; noise, clipping, and rail excursions | Pending firmware and bench |
| Intercom | Echo return loss of 20 dB or better before cancellation at the default clamps; stable full-duplex conversation with the browser test page; sound pressure at 1 m recorded | Pending firmware and bench |
| Buttons | Open GPIO ≥ 1.6 V; closed GPIO ≤ 0.9 V; LED current ≥ 5 mA; verify rail and temperature corners | Pending U5 prototype |
| Field transients | Initial target: ±4 kV contact and ±8 kV air ESD at accessible buttons and the front plate, plus ±1 kV EFT through a coupling clamp on the field loom | Pending lab validation |
| Surge | ±1 kV line-to-line on the strike and gate cables; mains immunity per the module specification | Pending lab validation |
| Transient result | Powered and unpowered: no damage, supply back-powering, unintended strike or gate activation, or persistent malfunction; GPIO stays within published device limits | Pending lab validation |
| Strike | 11–13 V at the energized coil; release after the commanded 1–3 s pulse; repeated operation and power interruption | Pending assembly |
| Gate output | Contact closes for the commanded 0.5–1 s pulse; open otherwise, including boot and shutdown | Pending assembly |
| Fault protection | Output short and coil fault remain within wire/component thermal limits; F1 clears against PSU12 hiccup behavior; F0 holds two simultaneous cold starts | Pending selected fuse test |
| Heater | HTR1 raises the camera carrier above 0 °C at -20 °C ambient; off at boot; duty limit honoured; dew point tracked from U6 | Pending prototype |

The ESD, EFT, and surge levels are initial engineering targets, not a compliance claim. Final installation conditions determine the applicable product test plan.

Record test supply settings, wiring lengths, instrument setup, waveforms, temperatures, and results. Typical datasheet curves do not substitute for guaranteed limits or measurements.

## Environmental acceptance

| Check | Acceptance criterion | Status |
| --- | --- | --- |
| Sealing | IP65 spray test on the assembled enclosure; no ingress at glands, vent, window, buttons, or acoustic ports | Pending enclosure |
| Thermal soak | 24 h at -20 °C and at +50 °C ambient with a solar lamp on the front; all functions operate; internal temperatures at the camera, Pi, and modules recorded | Pending enclosure |
| Condensation | Temperature cycling with humid air; no condensation on the camera window or boards; heater control keeps the internal air above the dew point | Pending enclosure |
| Radio | Wi-Fi RSSI at the installation site survey; Bluetooth scanning during a live call; both meet the firmware thresholds through the RF window | Pending enclosure |
| Acoustic ports | Membranes cost no more than 3 dB on playback and capture | Pending enclosure |

## Assembly requirements

| Item | Requirement |
| --- | --- |
| Mains | Only J_AC, F0, PSU5, and PSU12 in the partitioned compartment; double-insulated mains wire; PE to PE1, then to the front plate and the U5 star; installation by a qualified electrician behind a 10 A breaker and a 30 mA residual-current device |
| Harness | Soldered or latching production joints; strain relief at cable entries; no stacked Dupont housings; polarized 5 V feed into J8 pins 2, 4, 6, and 14 |
| Speaker | Verify polarity from terminal markings; ferrules suitable for the actual factory lead; no SPK terminal-to-ground connection; decoupled mounting |
| Microphone | Bottom port gasketed to its own opening behind a membrane; at least 100 mm from the speaker grille |
| Passives | C1/C2 at amplifier VIN; F2 and C3 on U5; D1 physically across the strike coil |
| Moisture | IP65 sealed body with a PTFE vent, gasketed faceplate, glands at the rear entry, conformal coating on every board; no drain holes through the sealed volume |
| Thermal | Thermal pad from the SoC to the backplate; heater bonded to the camera and microphone carrier; light-coloured or shaded front preferred |

## Deferred to enclosure design

| Review item | Deferred work | Closure evidence |
| --- | --- | --- |
| E4: relay module | Verify the retained module: input current at 3.3 V VCC, pickup and dropout margins, startup and power-loss behaviour, terminal layout, footprint | Schematic, measured input current, margins, and mechanical fit |
| E7: antenna | Define the RF window over the Zero 2 W antenna and the dimensioned metal keep-out; route the front plate, PE bond, and harness around it | Board-relative enclosure drawing and installed Wi-Fi and Bluetooth measurements |
| E8: mains compartment | Partition geometry, terminal and fuse-holder parts, creepage and clearance, gland positions, module mounting | Compartment drawing, part numbers, and the insulation and PE measurements above |
| E9: heater | Heater part, power, placement, and the dew-point control limits | Thermal soak and condensation results |
| E10: camera environment | Operation of the 0 to 50 °C rated camera inside the sealed enclosure across -20 to +50 °C ambient with solar load; window material and autofocus behaviour when cold | Thermal soak results and image checks at the corners |
| Mechanical selection | Buttons, U5 connectors, mounting hardware, sealing parts, vent, glands, window, and casing depth | Released mechanical BOM and drawings |

Relay wiring is provisional. No relay resistor substitution or driver redesign is approved in this step.

## Scope exclusions

Firmware, automated pin checks, CI regeneration checks for the drawings, infrared illumination, tamper detection, and a solid-state strike driver are outside this hardware step. The interface specification preserves the requirements needed by later firmware work; the enclosure reserves space and a 12 V feed for the infrared upgrade.
