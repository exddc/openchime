# Hardware qualification

Status: prototype design; no electrical bench results or enclosure qualification recorded.

## Electrical acceptance

| Check | Acceptance criterion | Status |
| --- | --- | --- |
| Wiring | Continuity matches terminal names and U2 pad numbers; no 5 V-to-3.3 V connection; strike return separate from Pi GND | Pending assembly |
| Pi rails | 4.75–5.25 V at J8 and U5 under startup and combined load; 3.3 V within ±5%; no resets or audio/camera faults | Pending measurement |
| Load case | Camera autofocus + encoding + Wi-Fi transmission + maximum permitted acknowledgement + both coils + all buttons | Pending measurement |
| Regulator margin | Record camera and onboard rail demand, peaks, temperature, and justified margin against the applicable regulator limits | Pending measurement |
| Audio | Correct left-slot capture/playback at 48 kHz; measured sample alignment, noise, clipping, and rail excursions | Pending firmware and bench |
| Buttons | Open GPIO ≥ 1.6 V; closed GPIO ≤ 0.9 V; LED current ≥ 5 mA; verify rail and temperature corners | Pending U5 prototype |
| Field transients | Initial target: ±4 kV contact and ±8 kV air ESD at accessible buttons, plus ±1 kV EFT through a coupling clamp on the field loom | Pending lab validation |
| Transient result | Powered and unpowered: no damage, supply back-powering, unintended strike activation, or persistent malfunction; GPIO stays within published device limits | Pending lab validation |
| Strike | 11–13 V at the energized coil; release after the commanded 1–3 s pulse; check repeated operation and power interruption | Pending assembly |
| Fault protection | Output short and coil fault remain within wire/component thermal limits; document F1 clearing and adapter protection behavior | Pending selected fuse test |

The ESD and EFT levels are initial engineering targets, not a compliance claim. Final installation conditions determine the applicable product test plan.

Record test supply settings, wiring lengths, instrument setup, waveforms, temperatures, and results. Typical datasheet curves do not substitute for guaranteed limits or measurements.

## Assembly requirements

| Item | Requirement |
| --- | --- |
| Power | Keep both mains adapters indoors; bring only 5 V and 12 V into the doorbell |
| Harness | Soldered or latching production joints; strain relief at cable entries; no stacked Dupont housings |
| Speaker | Verify polarity from terminal markings; ferrules suitable for the actual factory lead; no SPK terminal-to-ground connection |
| Passives | C1/C2 at amplifier VIN; C3 after relay PTC; D1 physically across the strike coil |
| Moisture | IP54 assembly target, protected acoustic port, drain path, and condensation control |

## Deferred to enclosure design

| Review item | Deferred work | Closure evidence |
| --- | --- | --- |
| E4: relay module | Select the module and PCB revision; resolve input network, any driver, terminal layout, coil protection, and footprint | Schematic, measured input current, pickup/dropout margins, startup/power-loss behavior, and mechanical fit |
| E7: antenna | Define the actual antenna location and dimensioned keep-out; route metalwork and harness around it | Board-relative enclosure drawing and installed wireless measurements |
| Mechanical selection | Select buttons, U5 connectors, mounting hardware, sealing parts, and casing depth | Released mechanical BOM and drawings |

Relay wiring is provisional. No relay resistor substitution or driver redesign is approved in this step. Antenna placement remains an explicit open item; the harness estimate does not resolve it.

## Scope exclusions

Firmware, automated pin checks, CI, transformer-to-5 V conversion, and infrared illumination are outside this hardware step. The interface specification preserves the requirements needed by later firmware work.
