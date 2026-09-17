# Raspberry Pi Zero W with Camera Module 3

Electrical interface specification for hardware, enclosure, and firmware engineers. Component quantities and harness estimates are in the [assembly BOM](../../bom.md).

## Power architecture

The doorbell enclosure accepts 5 V for the Pi and isolated 12 V for the strike. Both mains adapters remain indoors in a dry, accessible location.

| Supply | Selected source | Connection |
| --- | --- | --- |
| Pi 5 V | Official Raspberry Pi 12.5 W micro-USB supply, 5.1 V / 2.5 A | PWR_IN, micro-USB |
| Strike 12 V | Mean Well GST18E12-P1J, 12 V / 1.5 A, Class II | Indoor 5.5 × 2.1 mm centre-positive socket adapter, installation cable, then J12 |

The [GST18E specification](https://www.meanwell.com/Upload/PDF/GST18E/GST18E-SPEC.PDF) defines the adapter ratings and connector polarity. The IRM-10-12 and its exposed mains wiring are excluded from this revision.

J12 is the two-pole strike-supply inlet. F1 is a 1 A fuse between J12 positive and relay COM1. JOUT connects relay NO1 and J12 return to the external strike cable.

The strike return remains separate from Pi GND. Neither relay channel may switch mains. The relay contact design limit is 30 V DC / 1 A.

## J8 interface

J8 numbers are physical header pins. BCM numbers identify GPIOs. Pins 2 and 4 share the Pi 5 V rail; pins 1 and 17 share its 3.3 V rail.

| J8 | BCM | Net | Endpoint |
| --- | --- | --- | --- |
| 1 | n/a | 3V3 | U2 supply; U5 supply for six pull-ups |
| 2 | n/a | 5V_RLY | F2, then U3 JD-VCC |
| 4 | n/a | 5V | U1 VIN; U5 button LED supply |
| 6 | n/a | GND | U1 GND |
| 9 | n/a | GND_MIC | U2 GND |
| 11 | 17 | BUTTON1 | U5 GPIO1, button position 1 |
| 12 | 18 | I2S_BCLK | U1 BCLK and U2 BCLK |
| 13 | 27 | BUTTON2 | U5 GPIO2, button position 2 |
| 14 | n/a | GND_RLY | U3 GND |
| 15 | 22 | BUTTON3 | U5 GPIO3, button position 3 |
| 16 | 23 | BUTTON4 | U5 GPIO4, button position 4 |
| 17 | n/a | 3V3_RLY | U3 VCC |
| 20 | n/a | GND_BTN | U5 ground and button return |
| 29 | 5 | RELAY1 | U3 IN1, strike; U5 pull-up |
| 31 | 6 | RELAY2 | U3 IN2, auxiliary; U5 pull-up |
| 35 | 19 | I2S_LRCLK | U1 LRC and U2 LRCL |
| 38 | 20 | I2S_MIC | U2 DOUT |
| 40 | 21 | I2S_DIN | U1 DIN |

Reserve pins 7 (GPIO4) and 36 (GPIO16) for audio-overlay compatibility. Pins 27 and 28 remain unwired because the camera uses the corresponding I2C interface.

Pins 8 and 10 remain available for bench UART. Pins 3 and 5, the SPI pins, and other unassigned GPIOs remain free.

## Audio and camera

| Device | Required configuration |
| --- | --- |
| U1, Adafruit MAX98357A 3006 | VIN to SD selects left playback; VIN to GAIN selects 6 dB |
| U2, Adafruit SPH0645 3421 | 3.3 V supply; header pad 6 (SEL) to pad 2 (GND) selects left capture |
| SP1, Adafruit 3351 | 4 Ω / 3 W; twisted speaker pair, maximum 100 mm |
| CAM1, Camera Module 3 Standard | 200 mm Standard-Mini ribbon; Pi Zero 22-pin to camera 15-pin CSI |

U2 header pads are 1: 3V, 2: GND, 3: BCLK, 4: DOUT, 5: LRCL, 6: SEL. Source: [Adafruit breakout schematic](https://github.com/adafruit/Adafruit-I2S-Microphone-Breakout-PCB/blob/master/Adafruit%20I2S%20Mic%20SPK0415HM4H.sch).

U1 and U3 terminals use their silkscreen names in the harness drawing. U5 uses logical net names until its connector footprint is selected.

C1 (470 µF) and C2 (100 nF) connect directly across U1 VIN and GND. Splice shared I2S clocks within 30 mm of J8. Each I2S branch is at most 150 mm.

SPK+ and SPK- are bridge outputs. Neither speaker terminal connects to ground. Keep this pair separate from microphone wiring and CSI. [Amplifier pinouts](https://learn.adafruit.com/adafruit-max98357-i2s-class-d-mono-amp/pinouts).

## Buttons and relays

U5 buffers the button contacts using the [button input circuit](../../button-input.md). Each press produces a low GPIO input. Populate channels consecutively from the top of the faceplate.

| Position | Net | Future MQTT suffix | Population |
| --- | --- | --- | --- |
| 1 | BUTTON1 | 1 | All variants |
| 2 | BUTTON2 | 2 | 2–4 buttons |
| 3 | BUTTON3 | 3 | 3–4 buttons |
| 4 | BUTTON4 | 4 | 4 buttons |

All four GPIO pull-ups remain fitted when channels are omitted. The field loom carries SW1–SW4 and a shared return; it has no direct GPIO connection.

Relay module selection remains provisional until enclosure design. Retain the planned 3.3 V VCC, separate 5 V JD-VCC, common Pi ground, and active-low IN1/IN2 interface. The JD-VCC jumper must be absent.

F2 is a 500 mA hold-current PTC in the JD-VCC branch. C3 (100 µF) connects from JD-VCC after F2 to U3 GND. R15/R16 provide 10 kΩ pull-ups for IN1/IN2 on U5.

The module, input resistors, any driver changes, and operating margins remain unresolved under E4 in [qualification.md](../../qualification.md). These are not approved by a room-temperature switching test.

## Strike

The strike is an effeff 118 with the A7, 10–24 V AC/DC winding. This revision operates it at 12 V DC with a 1–3 s pulse. The D1 winding is excluded.

At 12 V DC, the A7 nominal current is 280 mA and resistance is 43 Ω. Its continuous-duty range is 11–13 V DC. [Manufacturer electrical data](https://dach.assaabloy.com/de/en/downloadportal/download/435-model-118?inline=1).

D1, a 1N4007, connects across the coil at the strike: cathode to STRIKE+, anode to STRIKE-. This diode is outside the doorbell casing when the strike is remote.

Require 11–13 V at the energized strike. External cable length is installation-specific; select its gauge after accounting for adapter tolerance, connectors, fuse, contacts, and cable voltage drop.

## Power budget

No system current or temperature measurements exist yet. The following values distinguish calculations from provisional allocations.

| Rail / load | Current | Basis |
| --- | --- | --- |
| 3.3 V: microphone | 1 mA allocation | 0.6 mA typical in the [Knowles specification](https://www.knowles.com/docs/default-source/model-downloads/sph0645lm4h-b-datasheet-rev-c.pdf) |
| 3.3 V: six 10 kΩ pull-ups | ≤ 1.98 mA nominal; ≤ 2.10 mA at +5% supply, -1% resistance | Upper bound with all six nodes at ground |
| 3.3 V: relay logic | 13 mA provisional allocation | Module-dependent; E4 remains open |
| 3.3 V: camera + autofocus | 300 mA provisional allocation | Peak current requires measurement at CSI |
| 3.3 V: onboard Pi loads | Unknown | No remaining regulator margin claimed |
| 5 V: button inputs | ≤ 65 mA for four channels | Conservative resistor-only bound; see button circuit |
| 5 V: two relay coils | 180 mA provisional allocation | Assumes two 0.45 W coils |
| 5 V: entire assembly | 1.5 A average planning allocation | Unverified; includes camera, Wi-Fi, audio, coils, and buttons |
| 12 V: strike | 280 mA nominal | A7 at 12 V DC; tolerance and cold-coil current require verification |

The Pi adapter rating is 2.5 A; that rating alone does not establish transient voltage at J8. C1 and C3 are initial component selections pending load-step measurements.

For a sinusoidal PCM signal, the [MAX98357A datasheet](https://www.analog.com/media/en/technical-documentation/data-sheets/MAX98357A-MAX98357B.pdf) gives output level as input dBFS + 2.1 dB + gain.

| Mode, 6 dB gain | Calculated speaker voltage | Calculated current into 4 Ω | Calculated power |
| --- | --- | --- | --- |
| Talkback, -6 dBFS | 1.27 V RMS | 0.32 A RMS / 0.45 A peak | 0.41 W |
| Acknowledgement, -3 dBFS | 1.80 V RMS | 0.45 A RMS / 0.64 A peak | 0.81 W |

These are nominal resistive-load calculations, not Pi supply-current measurements. Speaker impedance, amplifier losses, rail voltage, and waveform affect the actual load.

## Firmware interface requirements

This step defines hardware requirements only. Firmware, device-tree overlays, and automated hardware checks are not implemented here.

| Interface | Required behavior |
| --- | --- |
| Audio clock | Shared 48 kHz frame clock, two 32-bit slots, standard I2S; 3.072 MHz BCLK |
| Audio device | One sound-card configuration for PCM_DOUT playback and PCM_DIN capture; no SD GPIO |
| Channel format | Left-channel playback and capture, S32_LE; mono source routed to the left slot |
| Talkback | Half-duplex; PCM clamp at -6 dBFS |
| Local acknowledgement | PCM clamp at -3 dBFS |
| Camera | IMX708 through libcamera; 1280 × 720 initial target pending performance measurement |
| Buttons | Active low; ≥ 50 ms debounce and 3 s repeat suppression per input |
| Relay outputs | High when idle, during startup, and on shutdown; strike low pulse for 1–3 s |
| Button notification | ring/pressed/1 through ring/pressed/4; QoS 0, retain false |

The Chime subscription must match the chosen suffix or `ring/pressed/#`. Its existing `ring/pressed` topic does not match suffixed topics.

## Enclosure constraints

The planning envelope is 300 mm high × 150 mm wide; depth and mounting positions remain open. Harness estimates require the Pi, amplifier, and microphone to form a compact cluster.

| Part | Planning envelope / constraint |
| --- | --- |
| [Pi Zero W](https://www.raspberrypi.com/products/raspberry-pi-zero-w/) | 65 × 30 mm; retain access to PWR_IN, CSI, and microSD |
| [Camera Module 3](https://www.raspberrypi.com/documentation/accessories/camera.html) | Approximately 25 × 24 × 11.5 mm; autofocus clearance and window required |
| [MAX98357A](https://www.adafruit.com/product/3006) | 19.4 × 17.8 × 3 mm; within 100 mm cable route of speaker |
| [SPH0645 breakout](https://www.adafruit.com/product/3421) | 16.7 × 12.7 × 1.8 mm; bottom port, membrane, drain, gasket; ≥ 40 mm from speaker grille as an initial layout target |
| [Adafruit 3351](https://www.adafruit.com/product/3351) | 70 × 30 × 17 mm; vertical long axis; verify mounting dimensions against purchased unit |
| Relay module | Reserve 55 × 45 × 22 mm provisionally; selection deferred under E4 |
| U5 button board | Reserve 70 × 40 mm provisionally; connector and component placement pending |
| Buttons | 16 mm-class IP67 SPST-NO with gold contacts; exact part selected during enclosure design |

Assembly sealing target is IP54. Solar heating, condensation, and operating temperature require qualification. Antenna keep-out geometry and installed RF performance remain deferred under E7.
