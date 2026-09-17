# Raspberry Pi Zero 2 W with Camera Module 3 Wide

Pin assignments and operating requirements. Circuits are in [boards.md](../../boards.md), parts and harness lengths in the [BOM](../../bom.md), and pending verification in [qualification](../../qualification.md).

## Power architecture

The installation cable terminates on U7 inside a partitioned mains compartment. Two encapsulated modules supply the loads; the Pi generates 3.3 V.

| Rail | Source | Consumers |
| --- | --- | --- |
| 5 V | PSU5 on U7, Mean Well IRM-30-5, 5 V / 6 A | Pi through J8 pin 2, U1 amplifier, U3 relay coils through F2, U5 backlight LEDs, HTR1 heater through Q71 |
| 12 V | PSU12 on U7, Mean Well IRM-30-12, 12 V / 2.5 A | Strike through F1 and relay contact 1; headroom reserved for a later infrared array |
| 3.3 V | Pi onboard regulator | Camera, U2 microphone, U3 relay logic, U6 sensor, U5 button pull-ups |

[IRM-30 ratings and derating curves](https://www.meanwell.com/Upload/PDF/IRM-30/IRM-30-SPEC.PDF) apply to each module. U7 distributes 5 V radially; logic grounds provide additional return paths. JP1 bonds the 5 V return to PE. The 12 V return connects only to the strike circuit.

Mains layout and protective bonding are defined in [boards.md](../../boards.md). Installation requires a qualified electrician, a three-core 1.5 mm² supply cable through a strain-relieved IP68 gland, and an approved upstream protection plan. The provisional installation assumption is a breaker of at most 10 A and a 30 mA RCD; suitability remains part of the electrical assessment.

## J8 interface

J8 uses physical pin numbers; BCM identifies GPIOs. Pin 1 is nearest microSD on the inner row, marked by a square pad underneath.

5 V enters the Pi on pin 2 and returns on pin 6 through 20 AWG conductors from U7. Use a polarized harness and U7 as the only source. The proposed path lacks branch overcurrent and reverse-polarity protection; protection coordination is a release blocker.

| J8 | BCM | Net | Endpoint |
| --- | --- | --- | --- |
| 1 | n/a | 3V3 | U2 supply, U5 pull-ups, U6 supply; three branches spliced within 30 mm of J8 |
| 2 | n/a | 5V_IN | U7 J71 |
| 3 | 2 | I2C_SDA | U6 SDA |
| 5 | 3 | I2C_SCL | U6 SCL |
| 6 | n/a | GND_IN | U7 J71 |
| 9 | n/a | GND_MIC | U2 GND |
| 11 | 17 | BUTTON1 | U5 J51, switch position 1 |
| 12 | 18 | I2S_BCLK | U1 BCLK and U2 BCLK |
| 13 | 27 | BUTTON2 | U5 J51, switch position 2 |
| 14 | n/a | GND_HTR | U7 J76 |
| 15 | 22 | BUTTON3 | U5 J51, switch position 3 |
| 16 | 23 | BUTTON4 | U5 J51, switch position 4 |
| 17 | n/a | 3V3_RLY | U3 VCC |
| 18 | 24 | HEATER | U7 J76, Q71 gate network |
| 20 | n/a | GND_BTN | U5 J51 |
| 25 | n/a | GND_ENV | U6 GND |
| 29 | 5 | RELAY1 | U3 IN1, strike |
| 31 | 6 | RELAY2 | U3 IN2, gate |
| 32 | 12 | LED_PWM | U5 J51, backlight dimmer |
| 35 | 19 | I2S_LRCLK | U1 LRC and U2 LRCL |
| 38 | 20 | I2S_MIC | U2 DOUT |
| 40 | 21 | I2S_DIN | U1 DIN |

Required `config.txt` GPIO settings:

```
gpio=17,27,22,23=ip,pu
gpio=5,6=op,dh
gpio=24=op,dl
gpio=12=op,dl
```

These settings do not guarantee safe relay states during power sequencing. E4 must verify startup, shutdown, and loss of either supply. R71 and R41 pull the heater and backlight gates low while GPIOs float.

Reserve pins 7 (GPIO4) and 36 (GPIO16) for audio-overlay compatibility. Pins 27 and 28 remain unwired because the camera uses the corresponding I2C interface. Pins 8 and 10 carry the bench console on the mini-UART, because Bluetooth stays enabled and owns the PL011. Pin 4 and the remaining ground pins are free.

## Audio and camera

| Device | Required configuration |
| --- | --- |
| U1, Adafruit MAX98357A 3006 | VIN to SD selects left playback; VIN to GAIN selects 6 dB; supply from U7 J72 with C1 and C2 at VIN |
| U2, Adafruit SPH0645 3421 | 3.3 V supply; header pad 6 (SEL) to pad 2 (GND) selects left capture |
| SP1, Adafruit 3351 | 4 Ω / 3 W; twisted speaker pair, maximum 100 mm; foam or rubber decoupling |
| CAM1, Camera Module 3 Wide | 200 mm Standard-Mini ribbon; Pi Zero 22-pin to camera 15-pin CSI; 102° horizontal field of view; infrared-cut filter |

U2 header pads are 1: 3V, 2: GND, 3: BCLK, 4: DOUT, 5: LRCL, 6: SEL. Source: [Adafruit breakout schematic](https://github.com/adafruit/Adafruit-I2S-Microphone-Breakout-PCB/blob/master/Adafruit%20I2S%20Mic%20SPK0415HM4H.sch).

Splice shared I2S clocks within 30 mm of J8. Each I2S branch is at most 200 mm. SPK+ and SPK- are bridge outputs; neither speaker terminal connects to ground. Keep this pair separate from microphone wiring and CSI. [Amplifier pinouts](https://learn.adafruit.com/adafruit-max98357-i2s-class-d-mono-amp/pinouts).

Capture and playback share the I2S clock at 48 kHz. Echo cancellation must measure and track their buffer delay, including after stream restarts; a shared clock does not guarantee sample alignment.

Separate microphone and speaker ports by at least 100 mm. Decouple the speaker mechanically; seal the microphone bottom port to its own gasketed, membrane-covered opening. Target echo return loss before cancellation: ≥ 20 dB.

The [camera product brief](https://datasheets.raspberrypi.com/camera/camera-module-3-product-brief.pdf) specifies 0–50 °C operation and a 102° horizontal/120° diagonal field of view. Enclosure operation remains open under E10.

## Controls and outputs

| Interface | Requirement |
| --- | --- |
| Buttons | U5 positions 1–4 map to BUTTON1–4 and MQTT suffixes 1–4. Populate consecutively from the top; circuitry and plate geometry are in [boards.md](../../boards.md). |
| Relays | U3 VCC from J8 pin 17; active-low IN1/IN2 from GPIO5/6; JD-VCC from U7 J74 through F2, with C3; coil return to U7; JD-VCC jumper removed. Module qualification remains deferred under E4. |
| Strike | effeff 118 A7 winding, 12 V DC, 1–3 s pulse. PSU12 → F1 → U3 COM1/NO1 → JOUT1. D1 at the coil: cathode to STRIKE+, anode to STRIKE-. Exclude the D1 winding. |
| Gate | U3 COM2/NO2 to JOUT2, potential-free contact; design limit 30 V DC / 1 A, pulse 0.5–1 s. |
| Heater | HTR1 from U7 J75; GPIO24 through J76 drives Q71; R71 holds it off when the GPIO floats. |
| Environment | U6 SHT40 at I2C1 address 0x44, beside the camera window; VIN from 3.3 V, breakout 3V output unconnected. |

The strike draws nominally 280 mA at 12 V (43 Ω); require 11–13 V at the energized coil. Size the installation cable for supply tolerance, fuse, contact, and cable drop. [Manufacturer electrical data](https://dach.assaabloy.com/de/en/downloadportal/download/435-model-118?inline=1).

U6 measures local air, not the coldest surface. Heater control needs a qualified relationship between that reading and window, camera, and board temperatures, or additional sensing. Heater capacity, temperature limits, sensor-fault response, and independent overtemperature protection remain open under E9.

## Power budget

No load or temperature measurements exist. Values below are planning inputs, not measured peaks or proof of supply margin.

| Rail / load | Planning input |
| --- | --- |
| 5 V: Pi board | 0.7 A allocation; verify camera, encoder, Wi-Fi, and Bluetooth transients |
| 3.3 V: camera | 0.3 A allocation; measure CSI demand and regulator input power before adding to the 5 V budget |
| 5 V: amplifier | Supply current unmeasured; speaker current below is not supply current |
| 5 V: relay coils | 0.18 A assumed for two 0.45 W coils; E4 |
| 5 V: heater | 0.6 A nominal for 3 W |
| 5 V: backlight | 36 mA nominal with four LEDs |
| 3.3 V: button pull-ups | 1.32 mA nominal; 1.40 mA at +5% supply and -1% resistance, excluding internal pulls |
| Other loads | Include microphone, relay inputs, sensor, indicator, and regulator losses in measurements |
| 12 V: strike | 0.28 A nominal; verify cold coil; 0.5 A reserved for future infrared |
| Mains inrush | 45 A typical cold-start per module at 230 V; verify both modules against F0 |

Measure combined 5 V demand before claiming margin against PSU5's 6 A rating. Apply the manufacturer's voltage, temperature, and altitude derating at the measured module ambient.

For a sinusoidal PCM signal, the [MAX98357A datasheet](https://www.analog.com/media/en/technical-documentation/data-sheets/MAX98357A-MAX98357B.pdf) gives output level as input dBFS + 2.1 dB + gain.

| Mode, 6 dB gain | Calculated speaker voltage | Calculated current into 4 Ω | Calculated power |
| --- | --- | --- | --- |
| Talkback, -6 dBFS | 1.27 V RMS | 0.32 A RMS / 0.45 A peak | 0.41 W |
| Acknowledgement, -3 dBFS | 1.80 V RMS | 0.45 A RMS / 0.64 A peak | 0.81 W |

Qualify sound pressure and echo return loss at the configured output clamps.

## Thermal limits

The outdoor target is -20 to +50 °C, while the camera is rated 0 to +50 °C. Enclosure depth, thermal resistance, solar absorption, and heater capacity remain unverified. No internal temperature rise can yet be claimed. E9/E10 require cold-start and hot-soak measurements, surface condensation checks, and a defined operating policy when component limits cannot be maintained.

## Firmware interface requirements

Requirements only; firmware and overlays are outside this step.

| Interface | Required behavior |
| --- | --- |
| Platform | Zero 2 W image; Bluetooth enabled; bench console on the mini-UART; hardware watchdog serviced; read-only root with RAM logs; brown-out flag logging |
| Boot GPIO state | `config.txt` directives: buttons input with pull-up, GPIO5 and GPIO6 output high, GPIO24 and GPIO12 output low |
| Audio | One duplex sound card: PCM_DOUT playback, PCM_DIN capture; no SD GPIO. Standard I2S, 48 kHz, two 32-bit slots, 3.072 MHz BCLK; S32_LE capture/playback in the left slot. |
| Intercom | Full duplex with acoustic echo cancellation using the delay-aligned playback reference; automatic voice switching only as fallback; configurable output clamps, defaults -6 dBFS talkback and -3 dBFS acknowledgement |
| Test endpoint | WebRTC endpoint with Opus and hardware H.264; signaling over the MQTT broker; browser test page for phone and PC |
| Camera | IMX708 Wide through libcamera; 1280 × 720 initial target; fixed lens position fallback within the qualified temperature range |
| Buttons | Active low; ≥ 50 ms debounce and 3 s repeat suppression per input; state ignored for 3 s after boot |
| Backlight | Hardware PWM on GPIO12; off at boot; configurable level and schedule; brief flash as press acknowledgement |
| Relay outputs | High when idle, during startup, and on shutdown; strike low pulse 1–3 s; gate low pulse 0.5–1 s; maximum on-time; no output commands until time sync and authentication are ready |
| Heater | GPIO24 enables HTR1; off at boot and on sensor fault; duty and temperature limits; maintain qualified surface dew-point margin and camera range under E9/E10 |
| Environment sensor | SHT40 on I2C1 at 0x44; temperature and humidity logged |
| Button notification | `ring/<unit>/pressed/1` through `ring/<unit>/pressed/4`; QoS 1, retain false |
| Status | `ring/<unit>/status` retained with a last-will message |
| Output commands | `ring/<unit>/cmd/#` with signed payload and nonce over the existing TLS |
| Bluetooth unlock | LE Secure Connections plus application challenge-response with a per-device key and rolling counter; RSSI threshold with hysteresis and dwell; intent gating; rate limit; audit log |
| Local fallback | Acknowledgement sound plays when the broker is unreachable |

The Chime `ring_topic` filter must match the chosen unit and suffix, for example `ring/<unit>/pressed/1` or `ring/+/pressed/#`. The shipped exact-match `ring/pressed` does not match suffixed topics.

## Enclosure constraints

The planning envelope is 300 mm high × 150 mm wide; depth and mounting positions remain open. Assembly sealing target is IP65 with a pressure-equalization vent and no drain holes through the sealed volume.

| Part | Planning envelope / constraint |
| --- | --- |
| [Pi Zero 2 W](https://www.raspberrypi.com/products/raspberry-pi-zero-2-w/) | 65 × 30 mm; retain access to CSI and microSD; thermal pad from the SoC to the backplate; plastic RF window over the antenna corner with no metal behind it |
| Camera Module 3 Wide | Approximately 25 × 24 × 12.4 mm; autofocus clearance; optical window with gasket and light trap; space and a 12 V feed reserved for a later infrared window |
| [MAX98357A](https://www.adafruit.com/product/3006) | 19.4 × 17.8 × 3 mm; within 100 mm cable route of the speaker |
| [SPH0645 breakout](https://www.adafruit.com/product/3421) | 16.7 × 12.7 × 1.8 mm; bottom port, gasket, membrane; at least 100 mm from the speaker grille; within 200 mm of J8 |
| [Adafruit 3351](https://www.adafruit.com/product/3351) | 70 × 30 × 17 mm; vertical long axis; decoupled mounting; verify against the purchased unit |
| Relay module | Reserve 55 × 45 × 22 mm provisionally; E4 |
| U5 key board | About 50 × 130 mm behind the lower face on M3 standoffs from the front plate; switch cutouts, gasket sheet, and keycap clearance per E11 |
| U7 power board | About 160 × 90 mm on metal standoffs inside the rear mains compartment; 30 mm module height plus cover |
| U6 sensor | 25.4 × 17.8 mm beside the camera window |
| HTR1 | About 50 × 50 mm bonded to the camera and microphone carrier |
| Entries | One IP68 M20 gland for mains, two M16 glands for strike and gate; PTFE vent M12 |

Layout from the top: camera and U6, microphone, Pi with U1, speaker, key board behind the lower face. The mains compartment with U7 sits at the rear bottom behind its partition. HTR1 bonds to the carrier that holds the camera and the microphone.

Materials are UV-stable polycarbonate, ASA, or coated aluminium with stainless fasteners; no PLA or PETG. Coating locations are specified in the BOM; keep microphone ports, sensor openings, optics, connectors, and switch sockets clear. Antenna keep-out geometry and installed RF performance remain deferred under E7.
