# Raspberry Pi Zero 2 W with Camera Module 3 Wide

Electrical interface specification for hardware, enclosure, and firmware engineers. Component quantities and harness estimates are in the [assembly BOM](../../bom.md). The two custom boards are defined in [boards.md](../../boards.md).

## Power architecture

230 V AC enters the enclosure and lands on the power board U7 inside a partitioned mains compartment. U7 carries both Class II encapsulated modules and generates every low-voltage rail. No mains conductor exists outside U7.

| Rail | Source | Consumers |
| --- | --- | --- |
| 5 V | PSU5 on U7, Mean Well IRM-30-5, 5 V / 6 A | Pi through J8 pin 2, U1 amplifier, U3 relay coils through F2, U5 backlight LEDs, HTR1 heater through Q71 |
| 12 V | PSU12 on U7, Mean Well IRM-30-12, 12 V / 2.5 A | Strike through F1 and relay contact 1; headroom reserved for a later infrared array |
| 3.3 V | Pi onboard regulator | Camera, U2 microphone, U3 relay logic, U6 sensor, U5 button pull-ups |

The [IRM-30 specification](https://www.meanwell.com/Upload/PDF/IRM-30/IRM-30-SPEC.PDF) states Class II construction without an earth pin, 4.2 kV AC input-to-output withstand, ±2.5 % output tolerance, 45 A cold-start inrush at 230 V, and a -30 to +85 °C working range with derating above about 50 °C. The PCB-mount style measures 69.5 × 39 × 24 mm.

The 12 V rail is isolated from the 5 V rail. Its return connects only to J77 on U7 and the strike and never touches Pi ground.

U7 is the 5 V star point. Its terminals fan out to the Pi, the amplifier, the relay coil supply, the key board backlight, and the heater, and every 5 V return comes back to U7.

### Mains section

| Item | Requirement |
| --- | --- |
| Supply cable | 230 V AC, three cores with protective earth, NYY-J or H07RN-F 3G1.5 in conduit; IP68 gland at the rear entry; drip loop and strain relief; terminated on the U7 mains terminal |
| Compartment | Partition between the mains and low-voltage sides; only U7 inside; the U7 mains section additionally wears its clip-on cover; at least 6 mm creepage and clearance from mains copper to any SELV part or metal |
| F0 | 5 × 20 mm, T 2 A, 250 V, in the covered PCB holder on U7, in the L conductor ahead of both modules |
| RV1 | S14K275 varistor across L and N after F0; fit for exposed installations |
| PE | Supply PE to the U7 PE track; the track bonds to the metal backplate through the PE1 mounting standoff, to the metal front plate through a 1.0 mm² green-yellow conductor, and to the SELV ground through the link JP1; PE1 is the single chassis bonding point |
| Installation | Permanently connected by a qualified electrician; upstream circuit breaker of 10 A or less plus a 30 mA residual-current device; isolate the circuit before opening the enclosure |

The PE bond gives faceplate and keycap discharges a path that bypasses the electronics. The module isolation does not depend on PE.

## J8 interface

J8 numbers are physical header pins. BCM numbers identify GPIOs. The Zero 2 W header matches the Zero W.

5 V enters the Pi on pin 2 and returns on pin 6 through 20 AWG conductors from U7. This path has no fuse or reverse-polarity protection, so the harness is polarized and U7 is the only source.

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

Boot state comes from the firmware GPIO directives in `config.txt`, which the bootloader applies before the kernel starts:

```
gpio=17,27,22,23=ip,pu
gpio=5,6=op,dh
gpio=24=op,dl
gpio=12=op,dl
```

GPIO5 and GPIO6 also power up with internal pull-ups, so the active-low relay inputs stay off even before the directives apply. GPIO24 and GPIO12 power up with internal pull-downs and both MOSFET gates carry their own pull-down resistors, so the heater and backlight stay off until firmware enables them. The button inputs read high through the U5 pull-ups from power-on; firmware ignores button state for the first 3 s after boot.

Reserve pins 7 (GPIO4) and 36 (GPIO16) for audio-overlay compatibility. Pins 27 and 28 remain unwired because the camera uses the corresponding I2C interface. Pins 8 and 10 carry the bench console on the mini-UART, because Bluetooth stays enabled and owns the PL011. Pin 4 and the remaining ground pins are free.

## Audio and camera

| Device | Required configuration |
| --- | --- |
| U1, Adafruit MAX98357A 3006 | VIN to SD selects left playback; VIN to GAIN selects 6 dB; supply from U7 J72 with C1 and C2 at VIN |
| U2, Adafruit SPH0645 3421 | 3.3 V supply; header pad 6 (SEL) to pad 2 (GND) selects left capture |
| SP1, Adafruit 3351 | 4 Ω / 3 W; twisted speaker pair, maximum 100 mm; foam or rubber decoupling |
| CAM1, Camera Module 3 Wide | 200 mm Standard-Mini ribbon; Pi Zero 22-pin to camera 15-pin CSI; 102° horizontal field of view; infrared-cut filter |

U2 header pads are 1: 3V, 2: GND, 3: BCLK, 4: DOUT, 5: LRCL, 6: SEL. Source: [Adafruit breakout schematic](https://github.com/adafruit/Adafruit-I2S-Microphone-Breakout-PCB/blob/master/Adafruit%20I2S%20Mic%20SPK0415HM4H.sch).

U1 and U3 terminals use their silkscreen names in the harness drawings. U5 and U7 use logical terminal names until their connectors are selected.

Splice shared I2S clocks within 30 mm of J8. Each I2S branch is at most 200 mm. SPK+ and SPK- are bridge outputs; neither speaker terminal connects to ground. Keep this pair separate from microphone wiring and CSI. [Amplifier pinouts](https://learn.adafruit.com/adafruit-max98357-i2s-class-d-mono-amp/pinouts).

The Pi drives capture and playback from one I2S clock, so the far-end reference for echo cancellation is sample-aligned. Firmware keeps 48 kHz end to end without resampling and measures the fixed capture-to-playback offset once.

Full-duplex acoustics: the microphone port sits at least 100 mm from the speaker grille at the opposite end of the faceplate, the enclosed speaker mounts on decoupling, and the microphone bottom port seals to its own faceplate opening through a gasket and hydrophobic membrane. Target echo return loss before cancellation is 20 dB or better.

The [Camera Module 3 product brief](https://datasheets.raspberrypi.com/camera/camera-module-3-product-brief.pdf) rates operation at 0 to 50 °C and lists the Wide variant at 102° horizontal and 120° diagonal. Operation inside the sealed enclosure across the outdoor range is open item E10.

## Buttons

The buttons are Kailh BOX tactile switches on the key board U5, seated in hot-swap sockets, with MX-compatible relegendable keycaps. The switches are rated IP56 for dust and splash. The metal front plate is the switch plate, and a press reads low on the corresponding GPIO through the U5 pull-up, series resistor, and suppressor described in [boards.md](../../boards.md).

| Element | Specification |
| --- | --- |
| Switches | Kailh BOX tactile, MX stem, 3-pin plate mount; heavier BOX variants are acceptable for glove use |
| Plate | Metal front plate, 1.5 mm, 14 × 14 mm cutouts at 25.4 mm vertical pitch, board surface 5 mm behind the plate front |
| Keycaps | MX-compatible relegendable, 1u or 2u, name label under a clear cover |
| Backlight | One white SMD LED per switch through the housing window, about 9 mA each, dimmed together by hardware PWM on GPIO12 |
| Interface | 3.3 V pull-up of 10 kΩ, 1 kΩ series, 100 nF, bidirectional TVS per channel on U5; active low |
| Debounce | Firmware, at least 50 ms, plus 3 s repeat suppression |

Contact current is about 330 µA, a dry circuit that suits the gold crosspoint contacts. With the contact closed, the GPIO sees less than 0.1 V. The switch housings protrude through the PE-bonded plate, so most discharges reach earth before the board. Water that passes a keycap must not reach the board or the enclosure interior; the gasket sheet, keycap, and drainage concept is open item E11.

| Position | Net | MQTT suffix | Population |
| --- | --- | --- | --- |
| 1 | BUTTON1 | 1 | All variants |
| 2 | BUTTON2 | 2 | 2–4 buttons |
| 3 | BUTTON3 | 3 | 3–4 buttons |
| 4 | BUTTON4 | 4 | 4 buttons |

Populate positions consecutively from the top of the faceplate. All four sockets, pull-ups, capacitors, and suppressors are fitted on every variant; unused positions receive a blank plate insert.

## Relays, heater, and sensor

The relay module is retained. VCC is 3.3 V from J8 pin 17, IN1 and IN2 are active low from GPIO5 and GPIO6, JD-VCC comes from U7 J74 through F2 with C3, the coil return goes to U7, and the JD-VCC jumper is absent. Contact 1 switches the strike; contact 2 is the potential-free gate output on JOUT2. Module selection, input current at 3.3 V, and pickup margin remain open under E4 in [qualification.md](../../qualification.md).

Q71 on U7 switches HTR1 from the 5 V rail. GPIO24 high turns the heater on through J76; R71 holds the gate low when the GPIO floats. Firmware runs the heater against the U6 dew point and a 0 °C floor at the camera, with a duty limit.

U6 is an Adafruit SHT40 breakout on I2C1, address 0x44, mounted beside the camera window. It supplies temperature and relative humidity for heater control and condensation logging.

## Strike

The strike is an effeff 118 with the A7, 10–24 V AC/DC winding. This revision operates it at 12 V DC from PSU12 with a 1–3 s pulse. The D1 winding is excluded.

At 12 V DC, the A7 nominal current is 280 mA and resistance is 43 Ω. Its continuous-duty range is 11–13 V DC. [Manufacturer electrical data](https://dach.assaabloy.com/de/en/downloadportal/download/435-model-118?inline=1).

F1 on U7 fuses the 12 V positive at 1 A. J77 feeds relay contact 1; the contact's normally open terminal feeds JOUT1. D1, a 1N4007, connects across the coil at the strike: cathode to STRIKE+, anode to STRIKE-. This diode is outside the doorbell casing when the strike is remote.

Require 11–13 V at the energized strike. External cable length is installation-specific; select its gauge after accounting for module tolerance, fuse, contacts, and cable voltage drop.

## Gate output

JOUT2 carries U3 COM2 and NO2 as a potential-free contact for the gate controller start input. The design limit is 30 V DC / 1 A. Firmware pulses it for 0.5–1 s.

## Power budget

No system current or temperature measurements exist yet. The following values distinguish calculations from provisional allocations.

| Rail / load | Current | Basis |
| --- | --- | --- |
| 5 V: Zero 2 W board | 0.7 A peak, 0.35 A typical | Raspberry Pi recommends a 2.5 A supply; typical bare-board active current is about 350 mA |
| 5 V: camera through the Pi 3.3 V regulator | 0.3 A allocation at 3.3 V | Peak current requires measurement at CSI |
| 5 V: amplifier at -3 dBFS | 0.65 A peak, about 0.25 A average | MAX98357A calculation below |
| 5 V: two relay coils | 0.18 A | Assumes two 0.45 W coils |
| 5 V: HTR1 | 0.6 A when on | 3 W pad |
| 5 V: U5 backlight | 0.036 A | Four LEDs at 9 mA |
| 5 V: U6, U7 indicator, gate networks | < 0.01 A | |
| 5 V: peak sum | About 2.5 A | PSU5 rated 6 A; about 40 % load, inside the derating curve at any ambient |
| 3.3 V: U5 pull-ups | ≤ 1.3 mA | Four 10 kΩ with all contacts closed |
| 12 V: strike | 0.28 A | A7 at 12 V DC; cold-coil current requires verification |
| 12 V: reserved | 0.5 A | Later infrared array |
| Mains | About 0.05 A at 230 V typical; 45 A cold-start inrush per module | IRM-30 specification |

For a sinusoidal PCM signal, the [MAX98357A datasheet](https://www.analog.com/media/en/technical-documentation/data-sheets/MAX98357A-MAX98357B.pdf) gives output level as input dBFS + 2.1 dB + gain.

| Mode, 6 dB gain | Calculated speaker voltage | Calculated current into 4 Ω | Calculated power |
| --- | --- | --- | --- |
| Talkback, -6 dBFS | 1.27 V RMS | 0.32 A RMS / 0.45 A peak | 0.41 W |
| Acknowledgement, -3 dBFS | 1.80 V RMS | 0.45 A RMS / 0.64 A peak | 0.81 W |

Both clamps are firmware settings, not constants. Speaker level trades against echo cancellation margin; qualification measures sound pressure and echo return loss at the chosen values.

## Thermal

The sealed enclosure sheds heat only through its walls.

| Source | Continuous heat |
| --- | --- |
| Zero 2 W under load | 2.5 W |
| PSU5 losses at 6 W load, 83 % efficiency | 1.2 W |
| PSU12 idle and strike pulses | 0.3 W |
| Amplifier during a call | 1 W, bursty |
| HTR1 when on | 3 W |

About 5 W in a 300 × 150 × 60 mm enclosure gives a 5 to 7 K rise over ambient. A dark front in direct sun adds 20 to 30 K. With the heater on, winter self-heating reaches about 10 K. Both seasons exceed the camera rating without measures; E10 covers them.

## Firmware interface requirements

This step defines hardware requirements only. Firmware, device-tree overlays, and automated hardware checks are not implemented here.

| Interface | Required behavior |
| --- | --- |
| Platform | Zero 2 W image; Bluetooth enabled; bench console on the mini-UART; hardware watchdog serviced; read-only root with RAM logs; brown-out flag logging |
| Boot GPIO state | `config.txt` directives: buttons input with pull-up, GPIO5 and GPIO6 output high, GPIO24 and GPIO12 output low |
| Audio clock | Shared 48 kHz frame clock, two 32-bit slots, standard I2S; 3.072 MHz BCLK |
| Audio device | One sound-card configuration for PCM_DOUT playback and PCM_DIN capture; no SD GPIO |
| Channel format | Left-channel playback and capture, S32_LE; mono source routed to the left slot |
| Intercom | Full duplex with acoustic echo cancellation using the sample-aligned reference; automatic voice switching only as fallback; configurable output clamps, defaults -6 dBFS talkback and -3 dBFS acknowledgement |
| Test endpoint | WebRTC endpoint with Opus and hardware H.264; signaling over the MQTT broker; browser test page for phone and PC |
| Camera | IMX708 Wide through libcamera; 1280 × 720 initial target; fixed lens position fallback when cold |
| Buttons | Active low; ≥ 50 ms debounce and 3 s repeat suppression per input; state ignored for 3 s after boot |
| Backlight | Hardware PWM on GPIO12; off at boot; configurable level and schedule; brief flash as press acknowledgement |
| Relay outputs | High when idle, during startup, and on shutdown; strike low pulse 1–3 s; gate low pulse 0.5–1 s; maximum on-time; no output commands until time sync and authentication are ready |
| Heater | GPIO24 high enables HTR1 through Q71; control against U6 dew point and a 0 °C floor; duty limit; off at boot |
| Environment sensor | SHT40 on I2C1 at 0x44; temperature and humidity logged |
| Button notification | ring/<unit>/pressed/1 through ring/<unit>/pressed/4; QoS 1, retain false |
| Status | ring/<unit>/status retained with a last-will message |
| Output commands | ring/<unit>/cmd/# with signed payload and nonce over the existing TLS |
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

Materials are UV-stable polycarbonate, ASA, or coated aluminium with stainless fasteners; no PLA or PETG. Every board receives conformal coating outside its connectors and switch sockets. Antenna keep-out geometry and installed RF performance remain deferred under E7.
