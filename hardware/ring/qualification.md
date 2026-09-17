# Hardware qualification

Prototype specification. All checks below are pending; no bench or enclosure results are recorded. Severity ranks the consequence of leaving an item unresolved, not permission to omit lower-ranked checks.

## Release blockers

| Severity | Finding | Passing evidence |
| --- | --- | --- |
| Critical | U7 mains insulation, PE path, and fault protection are not designed or qualified. Module approvals do not qualify the assembly. | Released schematic/layout and assembly assessment covering insulation coordination, protective bonding, fuse/varistor coordination, branch protection, and fault tests. |
| High | U5's 5 V TVS and series resistor do not establish protection for the Pi GPIO. | Guaranteed voltage/current limits and transient measurements at the GPIO, powered and unpowered; revise the circuit if compliance cannot be demonstrated. Do not rely on undocumented SoC clamp capacity. |
| High | A 6 A source feeds branches whose wiring and contacts may tolerate less fault current. | Selected branch protection or documented current limiting that keeps each contact, wire, and load within its thermal limits during shorts and supply hiccup. Include J8 and HTR1. |
| High | Camera temperature and surface condensation limits are unverified across the outdoor range. | E9/E10 thermal and condensation results, including cold start, solar load, sensor failure, and heater stuck on. Define independent overtemperature protection and operating limits. |
| Medium | Q1's provisional 2N7002 has no guaranteed on-resistance at 3.3 V. | Selected MOSFET with guaranteed drive margin at the minimum GPIO high voltage, plus maximum-current and temperature checks. |
| Medium | Supply margin and audio alignment were inferred without measurements. | Combined rail measurements with derating, and capture/playback delay measurements across stream starts and restarts. |

## Electrical acceptance

Use the combined load: camera autofocus/encoding, Wi-Fi transmission, Bluetooth activity, maximum permitted acknowledgement, both coils, backlight, and heater. Record supply conditions, wiring lengths, instruments, waveforms, temperatures, and results.

| Check | Acceptance criterion |
| --- | --- |
| Wiring | Matches drawings and U2 pad numbers; no 5 V-to-3.3 V connection; 12 V return confined to the strike circuit. |
| Mains insulation | Assess the provisional 6 mm clearance/creepage target in [boards.md](boards.md). Approve test voltages, duration, connections, and component exclusions before testing bare board and complete assembly. Provisional targets: 3 kV AC/60 s and ≥ 100 MΩ at 500 V DC; procedure unapproved. |
| PE bonding | JAC PE to both plates: ≤ 0.1 Ω provisional continuity target, plus an approved fault-current test. Verify JP1 separately and assess bond integrity during servicing; continuity alone does not qualify the PCB/standoff path. |
| Rails | J8 pin 2 and U7 J71: 4.75–5.25 V; 3.3 V within ±5%, during startup and combined load. Record regulator demand and margin against guaranteed limits and module derating; no resets or audio/camera faults. |
| Audio | Left-slot capture/playback at 48 kHz; measure delay after stream restarts, noise, clipping, and rail excursions. Echo return loss ≥ 20 dB before cancellation at default clamps; stable full-duplex browser call; record SPL at 1 m. |
| Buttons | Open GPIO ≥ 1.6 V; closed ≤ 0.9 V; no phantom boot press; every hot-swap position works. |
| Backlight | Off at boot; PWM dims without visible flicker. Current and Q1 temperature meet selected component limits; 36 mA is nominal, not a maximum. |
| Transients | Initial targets: ±4 kV contact/±8 kV air ESD at exposed surfaces; ±1 kV EFT coupled onto W17; ±1 kV line-to-line surge on strike/gate cables. Define assembly mains tests separately from module immunity ratings. |
| Transient result | Powered and unpowered: no damage, back-powering, unintended strike/gate activation, or persistent malfunction; GPIO voltage and current within documented limits. |
| Strike / gate | Strike receives 11–13 V and releases after 1–3 s; gate closes for 0.5–1 s. Neither activates during startup, shutdown, or power interruption. |
| Protection | F0 survives simultaneous cold starts; F1 coordinates with PSU12 hiccup; all shorted branches and coil/heater faults stay within thermal limits. Verify RV1 failure protection. |
| Heater | Q71 drive verified at minimum GPIO high; HTR1 maintains qualified surface temperatures. Off at boot and on sensor fault; duty and upper-temperature limits enforced, including stuck-on failure protection. |

Transient levels are engineering targets. The final installation and applicable product standard determine the qualification plan.

## Environmental acceptance

Define solar irradiance, humidity cycles, RF thresholds, acoustic frequency band, and UV exposure before testing.

| Check | Acceptance criterion |
| --- | --- |
| Sealing | Complete IP65 dust and water testing on the assembled enclosure, including keys, blanks, glands, vent, window, and acoustic ports; no water on U5. |
| Thermal | 24 h at each -20 °C and +50 °C ambient endpoint, with specified solar load. Record camera, Pi, heater, and module temperatures; stay within component ratings and verify image quality, autofocus, and switch operation. |
| Condensation | Humid temperature cycling; no condensation on window or boards. Maintain a validated margin between the coldest relevant surface and local dew point; U6 air temperature alone is insufficient. |
| Radio | Installed Wi-Fi and Bluetooth meet agreed link thresholds, including BLE scanning during a call. |
| Acoustics | Membranes attenuate playback/capture by ≤ 3 dB over the declared band and fixture. |
| Keycaps | Legends remain readable after defined UV/thermal exposure; switches work with gloves. |

## Assembly constraints

Mains cable terminates at JAC behind the partition and cover; installation and upstream protection follow the [electrical interface](variants/rpi02w_cm3/README.md). Use polarized, soldered or latching harness joints with strain relief; no stacked friction-fit housings. Fit C1/C2 at U1 and D1 at the strike coil. Neither speaker terminal connects to ground.

Gasket the microphone to its own membrane-covered opening, at least 100 mm from the decoupled speaker. Fit key gaskets and unused-position blanks. Use rear-entry glands and a PTFE vent, with no drain through the sealed volume. Mask sensitive areas during coating as specified in the [BOM](bom.md). Couple the Pi thermally to the backplate and bond HTR1 to the camera/microphone carrier.

## Enclosure and board work

E4 and E7 remain deferred to casing design. Other open items must close before hardware release.

| Item | Required closure evidence |
| --- | --- |
| E4: relay module | Module schematic, 3.3 V input current, pickup/dropout margin, independent supply sequencing, terminal layout, and fit. Retained wiring remains provisional; no resistor substitution or driver redesign in this step. |
| E7: antenna | Dimensioned RF window and metal keep-out, including PE/harness routing; installed Wi-Fi/Bluetooth measurements. |
| E8: power board | U7 schematic/layout, selected terminals/fuses, cover, mounting and compartment drawings; mains/protection results above. |
| E9: heater | Selected heater, power/placement, Q71 drive, surface-to-air thermal mapping or extra sensing, control limits, and fault protection; thermal/condensation results. |
| E10: camera | Maintain the 0–50 °C camera rating across the outdoor target and solar load; window selection, corner-condition images, and cold autofocus results. Revise the design or rated operating envelope if this cannot be met. |
| E11: keys | Exact switch/keycap parts, faceplate/U5 drawings, gasket and external drainage path, blank inserts, standoff height, and sealing results. |
| Mechanical selection | Released connectors, standoffs, seals, vent, glands, window, and casing depth. |

## Scope

Next: schematic capture and layout of U5/U7. Firmware, checker scripts, CI, infrared illumination, tamper detection, and a solid-state strike driver are outside this step. Reserve space and 12 V capacity for future infrared hardware.
