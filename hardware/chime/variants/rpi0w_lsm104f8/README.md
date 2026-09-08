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

The MAX98357A has no software volume control on this wiring. Hardware gain is the unconnected GAIN pad on the breakout. Set playback level with `volume_bell` and `volume_notifications` in Chime config.
