# Ring hardware

Hardware specification for the Raspberry Pi Zero 2 W doorbell with Camera Module 3 Wide, I2S audio, two relays, one to four Kailh BOX buttons, and a heated, sealed outdoor enclosure.

Mains enters the 300 × 150 mm enclosure and terminates on the power board U7 inside a partitioned compartment. U7 carries both Class II modules and supplies 5 V for the Pi and an isolated 12 V for the strike. The key board U5 behind the faceplate carries the switches, their protection, and the backlight. Everything else is a bought module or a harness.

| Document | Contents |
| --- | --- |
| [Electrical interface](variants/rpi02w_cm3/README.md) | Power architecture, mains section, pin assignments, boot GPIO state, audio, buttons, strike, gate, heater, thermal, and firmware requirements |
| [Board definitions](boards.md) | U5 key board and U7 power board at connection level, with design rules for the mains section |
| [Assembly BOM](bom.md) | Parts, operating ranges, board contents, population variants, and estimated harness lengths |
| [Qualification](qualification.md) | Acceptance criteria, environmental tests, and deferred enclosure decisions |

This is a prototype specification. Board layouts, relay module verification, antenna keep-out, heater sizing, camera environmental operation, and key sealing remain deferred. Electrical and environmental qualification are pending.

## Wiring

Three drawings show every harness connection; on-board circuits are in the board definitions.

Power and strike, source [wiring-power.yml](wiring-power.yml):

![Ring power and strike](wiring-power.svg)

Audio and camera, source [wiring-audio.yml](wiring-audio.yml):

![Ring audio and camera](wiring-audio.svg)

Key board, relays, heater control, and sensor, source [wiring-io.yml](wiring-io.yml):

![Ring key board, relays, heater control, and sensor](wiring-io.svg)

The SVG files are generated and committed so the drawings render here without tooling; `.gitattributes` marks them as generated. Regenerate all three with WireViz 0.4.1 and Graphviz 14.1.2:

```sh
wireviz -f s hardware/ring/wiring-power.yml hardware/ring/wiring-audio.yml hardware/ring/wiring-io.yml
```

The generated `.bom.tsv` files are partial wire reports. [bom.md](bom.md) is the assembly BOM. Hardware checker scripts and CI are outside this design step.
