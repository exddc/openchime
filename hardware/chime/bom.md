# Chime assembly BOM

BOM for the [rpi02w_lsm104f8](variants/rpi02w_lsm104f8/README.md) variant in the existing 110 × 110 mm housing. “Verify” marks an unconfirmed fit or rating.

## Parts

| Reference | Quantity | Part / specification | Status | Rationale |
| --- | --- | --- | --- | --- |
| P1 / J8 | 1 | Raspberry Pi Zero 2 W with 2×20 male header | Selected; image pending | Same footprint, mounting holes, and PWR_IN position as the shipped Zero W |
| PSU1 | 1 | Raspberry Pi 12.5 W micro-USB supply, 5.1 V / 2.5 A | Selected | Official supply for the Zero 2 W |
| SD1 | 1 | High-endurance or pSLC microSD, 16 GB or more | Class selected; part pending | Write endurance; any temperature grade indoors |
| U1 | 1 | Adafruit MAX98357A breakout, product 3006 | Selected | Pin names and straps match the drawing |
| C1 | 1 | 470 µF electrolytic, ≥ 10 V, low-ESR, 105 °C | Selected | U1 VIN to GND; buffers speaker transients on the Pi 5 V rail |
| C2 | 1 | 100 nF X7R, ≥ 16 V | Selected | U1 VIN to GND |
| SP1 | 1 | EKULIT LSM-104F/SQ, 8 Ω / 3 W | Selected | The housing is built around it |
| BODY | 1 | Printed body, [Body.step](cad/fusion/exports/Body.step) | Selected; verify Zero 2 W fit | |
| COVER | 1 | Printed front cover, [Front_Cover.step](cad/fusion/exports/Front_Cover.step) | Selected | |
| Screws | 4 | M2 × 4 mm, Pi to body | Selected | |
| Screws | 4 | M4 × 5 mm, speaker to body | Selected | |

## Harness length estimate

Estimated cut lengths include 30 mm for termination and movement. Confirm routes against the housing before cutting.

| Cable | Route | Conductors | Length each | Total conductor length |
| --- | --- | --- | --- | --- |
| W1 | J8 pins 4 and 6 to U1 VIN and GND | 2 × 24 AWG | 100 mm | 0.20 m |
| W2 | J8 to U1 I2S | 3 × 26 AWG | 100 mm | 0.30 m |
| W3 | U1 to SP1, twisted | 2 × 24 AWG | 100 mm | 0.20 m |
| W4 | U1 VIN to SD strap | 1 × 26 AWG | 20 mm | 0.02 m |

| Procurement allowance | Quantity |
| --- | --- |
| 24 AWG internal wire | 0.40 m cut; 0.48 m with 20% waste |
| 26 AWG internal wire | 0.32 m cut; 0.38 m with 20% waste |

Solder the J8 loom; friction-fit housings loosen with speaker vibration. Include heat-shrink for every J8 joint and the C1 leads. WireViz lengths are in [wiring.yml](wiring.yml); the generated wire report omits modules and passives, so use this BOM for procurement.
