# Raspberry Pi Zero W LSM-104F-8

This variant uses a Raspberry Pi Zero W, a MAX98357A amplifier, and an LSM-104F-8 speaker.

## BOM

- 1x Raspberry Pi Zero W
- 1x LSM-104F-8 speaker
- 1x MAX98357A amplifier
- 4x M4x5mm screws
- 4x M2x4mm screws
- 1x 3D printed body
- 1x 3D printed face cover

## Wiring

The harness drawing is [wiring.svg](../../wiring.svg).

The 40-pin GPIO header is J8. Pin 1 is nearest the microSD slot, on the inner row (toward the board center). That pad is square on the underside.

Jumper MAX98357A VIN to SD so the shutdown pin stays high. The boot config uses `dtoverlay=max98357a,no-sdmode`, which does not drive SD from a GPIO. Do not wire GAIN.

| Pi J8 | Net | MAX98357A |
| --- | --- | --- |
| Pin 4 (5V) | `5V` | VIN |
| Pin 4 (5V) | `AMP_SD` | SD (jumper to VIN) |
| Pin 6 (GND) | `GND` | GND |
| Pin 12 (GPIO18) | `I2S_BCLK` | BCLK |
| Pin 35 (GPIO19) | `I2S_LRCLK` | LRC |
| Pin 40 (GPIO21) | `I2S_DIN` | DIN |

| MAX98357A | Net | LSM-104F-8 |
| --- | --- | --- |
| + | `SPK+` | + |
| − | `SPK-` | − |

## Assembly

1. Jumper MAX98357A VIN to SD. Do not wire GAIN.
2. Connect J8 to the MAX98357A using the first table. Pin 1 is nearest the microSD slot.
3. Run speaker leads from MAX98357A + to speaker + and from MAX98357A − to speaker −.
4. Fix the Pi with the 4x M2x4mm screws. Keep the PWR_IN USB port reachable from outside the enclosure.
5. Screw the speaker in with the 4x M4x5mm screws.
6. Press the face cover onto the body.

## Volume

The MAX98357A has no volume register and no volume GPIO; its analog gain is only the GAIN pad. `volume_ring` and `volume_notifications` in Chime config change loudness by adjusting the PCM.

## Interface

BCM is the Broadcom GPIO number. I2S is the digital audio bus into the MAX98357A.

### Electrical

USB 5V enters at PWR_IN (micro USB). The official Raspberry Pi 12.5 W micro-USB supply is 5.1 V, 2.5 A. The MAX98357A can drive 1.8 W typical into 8 Ω at 5 V (10% THD+N).

GAIN is unconnected. `dtoverlay=max98357a,no-sdmode` in `config.txt` means the Pi does not drive SD from a GPIO. SD stays high via the VIN jumper on pin 4.

The speaker (BOM LSM-104F-8, EKULIT LSM-104F/SQ) is 8 Ω ±15%, 3 W nominal.

| Net | J8 pin | BCM | Dir | Voltage | Far end | Owner |
| --- | --- | --- | --- | --- | --- | --- |
| USB 5V in | PWR_IN | — | in | 5 V | Pi | power |
| `5V` | 4 | — | out | 5 V | MAX98357A VIN | chime |
| `AMP_SD` | 4 | — | out | 5 V | MAX98357A SD (jumper from VIN) | chime |
| `GND` | 6 | — | — | 0 V | MAX98357A GND | chime |
| `I2S_BCLK` | 12 | 18 | out | 3.3 V | MAX98357A BCLK | chime |
| `I2S_LRCLK` | 35 | 19 | out | 3.3 V | MAX98357A LRC | chime |
| `I2S_DIN` | 40 | 21 | out | 3.3 V | MAX98357A DIN | chime |
| `SPK+` | — | — | analog | unknown | speaker + | chime |
| `SPK-` | — | — | analog | unknown | speaker − | chime |
| `UART_TX` | 8 | 14 | out | 3.3 V | serial adapter RX | debug |
| `UART_RX` | 10 | 15 | in | 3.3 V | serial adapter TX | debug |

UART uses `enable_uart=1` and `dtoverlay=disable-bt` in `config.txt`. Getty is on `ttyS0`.

### J8 allocation

| Pin | BCM | Signal | Owner |
| --- | --- | --- | --- |
| 1 | — | 3V3 | power |
| 2 | — | 5V | power |
| 3 | 2 | SDA | free |
| 4 | — | 5V (`5V`, `AMP_SD`) | chime |
| 5 | 3 | SCL | free |
| 6 | — | GND | power |
| 7 | 4 | GPIO4 | free |
| 8 | 14 | TXD (`UART_TX`) | debug |
| 9 | — | GND | power |
| 10 | 15 | RXD (`UART_RX`) | debug |
| 11 | 17 | GPIO17 | free |
| 12 | 18 | PCM_CLK (`I2S_BCLK`) | chime |
| 13 | 27 | GPIO27 | free |
| 14 | — | GND | power |
| 15 | 22 | GPIO22 | free |
| 16 | 23 | GPIO23 | free |
| 17 | — | 3V3 | power |
| 18 | 24 | GPIO24 | free |
| 19 | 10 | MOSI | free |
| 20 | — | GND | power |
| 21 | 9 | MISO | free |
| 22 | 25 | GPIO25 | free |
| 23 | 11 | SCLK | free |
| 24 | 8 | CE0 | free |
| 25 | — | GND | power |
| 26 | 7 | CE1 | free |
| 27 | 0 | ID_SD | do-not-use |
| 28 | 1 | ID_SCL | do-not-use |
| 29 | 5 | GPIO5 | free |
| 30 | — | GND | power |
| 31 | 6 | GPIO6 | free |
| 32 | 12 | GPIO12 | free |
| 33 | 13 | GPIO13 | free |
| 34 | — | GND | power |
| 35 | 19 | PCM_FS (`I2S_LRCLK`) | chime |
| 36 | 16 | GPIO16 | free |
| 37 | 26 | GPIO26 | free |
| 38 | 20 | PCM_DIN | free |
| 39 | — | GND | power |
| 40 | 21 | PCM_DOUT (`I2S_DIN`) | chime |

Pins 27 and 28 are the HAT ID EEPROM I2C bus. Every pin marked `free` is available for use. GPIO18, GPIO19, and GPIO21 are taken.

### Software

Keys and validation stay in `schema/chime_config.json`. Ring publishes into these contracts:

- MQTT `ring_topic` (default `ring/pressed`): Ring reports a press. Any message whose topic matches the filter plays `sound_path`. The payload has no required schema.
- MQTT `heartbeat_topic` (default `chime/heartbeat`): Chime reports it is alive (`alive` or `degraded`).
- HTTPS `chime-webd` on port 8443: pair, then session. See [chime/README.md](../../../../chime/README.md).
- Audio: `aplay` plays the WAV. `volume_ring` / `volume_notifications` scale PCM on the Pi. Amp analog gain is the GAIN strap only.

### Mechanical

PWR_IN must stay reachable with the lid on. Enclosure CAD: [Body.step](../../cad/fusion/exports/Body.step), [Front_Cover.step](../../cad/fusion/exports/Front_Cover.step).

### Datasheets

- [Raspberry Pi Zero W](https://www.raspberrypi.com/products/raspberry-pi-zero-w/)
- [Raspberry Pi 12.5 W micro-USB PSU](https://www.raspberrypi.com/products/micro-usb-power-supply/) (5.1 V, 2.5 A)
- [MAX98357A](https://www.analog.com/media/en/technical-documentation/data-sheets/MAX98357A-MAX98357B.pdf)
- [EKULIT LSM-104F/SQ](https://cdn-reichelt.de/documents/datenblatt/I200/EKULIT-130050.pdf) (8 Ω ±15%, 3 W nominal)
