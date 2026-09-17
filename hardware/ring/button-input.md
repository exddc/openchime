# Button input circuit

U5 is the custom Ring I/O board. It buffers up to four normally open button contacts, carries the six GPIO pull-ups, forms the 5 V star point, holds F2 and C3 for the relay coils, and switches the heater. Its outputs preserve the active-low J8 button interface.

Each contact drives an optocoupler LED. The corresponding phototransistor pulls a local 3.3 V GPIO node low. The field wire never connects directly to that GPIO node.

The input and output share Pi ground, so this circuit does not provide galvanic system isolation. Its purpose is to separate the field signal voltage from the GPIO signal voltage.

## Connections

Channel n uses U1n, R1n, R2n, R3n, C1n, D1n, D2n, and D3n, where n is 1–4. For example, channel 1 uses U11 and R11/R21/R31.

| Component | Connection |
| --- | --- |
| D1n, 1N4148 blocking diode | Anode to the U5 5 V star; cathode to R2n |
| R2n, 330 Ω / 1% / 0.25 W | D1n cathode to U1n pin 1, LED anode |
| U1n, VO617A-3 | Pin 2, LED cathode, to SWn; pin 3, emitter, to GND; pin 4, collector, to R3n |
| D2n, 1N4148 reverse LED clamp | Anode to U1n pin 2; cathode to pin 1 |
| D3n, PESD5V0S1UL | Cathode to SWn; anode to field return at the loom entry |
| SWn contact | SWn to shared field return when pressed |
| R3n, 1 kΩ / 1% | U1n collector to BUTTONn GPIO node |
| R1n, 10 kΩ / 1% | BUTTONn to Pi 3.3 V |
| C1n, 100 nF | BUTTONn to GND, beside the GPIO connector |

Field return joins the U5 star ground. The star returns to J8 pins 6 and 14 through the power loom and references pin 20 through the logic loom. Route TVS return current directly to the star, away from the GPIO and microphone ground paths.

R15 and R16 are the two relay pull-ups, 10 kΩ to Pi 3.3 V. They connect through U5 RLY1/RLY2 to J8 pins 29/31 and the corresponding U3 inputs.

All six pull-ups remain fitted. Omit the other channel components and field wire for each unused button. Insulate unused loom positions.

## Power distribution and heater switch

PSU5 lands on the 5V_IN and GND_IN terminals. The star fans out on separate terminals to J8 pins 2 and 4 with returns on pins 6 and 14, to U1, to the relay coil supply, and to the heater. PE_LV from stud PE1 bonds to the star at one point.

| Component | Connection |
| --- | --- |
| F2, 500 mA hold PTC | 5 V star to JD-VCC terminal |
| C3, 100 µF | JD-VCC terminal to the star ground |
| Q1, logic-level N-channel MOSFET | Drain to HTR- terminal; source to the star ground; HTR+ terminal to the 5 V star |
| R42, 100 Ω | HEAT terminal to Q1 gate |
| R41, 10 kΩ | Q1 gate to the star ground |

Q1 sees at most 0.6 A. Select a part with R_DS(on) of 50 mΩ or less at a 2.5 V gate drive so that a 3.3 V GPIO fully enhances it.

## Component limits and calculations

The [VO617A datasheet](https://www.vishay.com/docs/83430/vo617a.pdf) specifies a maximum 0.4 V saturation voltage at 5 mA LED current and 1 mA collector current, at 25 °C.

The [1N4148 datasheet](https://assets.nexperia.com/documents/data-sheet/1N4148_1N4448.pdf) specifies a maximum 1 V forward drop at 10 mA. The calculations below use 1.65 V for the optocoupler LED and reserve 0.2 V for the button loop.

| Check | Calculation / design target |
| --- | --- |
| Minimum LED current at 25 °C | (4.75 - 1.0 - 1.65 - 0.2) / 333.3 = 5.70 mA |
| Conservative maximum LED current | 5.25 / 326.7 = 16.1 mA, ignoring diode drops |
| Maximum resistor dissipation | 5.25² / 326.7 = 84.4 mW; 0.25 W resistor selected |
| Maximum static GPIO low, 25 °C model | 0.4 + (3.465 - 0.4) × 1010 / (9900 + 1010) = 0.684 V |
| Capacitor discharge current | Less than 3.465 / 990 = 3.50 mA through R3n |
| GPIO high with contact open | Near the local 3.3 V rail; verify leakage at temperature |

PSU5 holds the 5 V rail within ±2.5 %, so the 4.75 V and 5.25 V corners above are conservative.

The GPIO low target is below the [BCM2835 input-low limit of 0.9 V](https://www.raspberrypi.com/documentation/computers/raspberry-pi.html#voltage-specifications). Temperature margins remain subject to qualification; 25 °C limits are not full-temperature guarantees.

## Transient behavior

The [PESD5V0S1UL](https://assets.nexperia.com/documents/data-sheet/PESD5V0S1UL.pdf) can clamp at 9 V at 1 A and 20 V at 15 A. Those are field-node voltages, not permitted GPIO voltages.

D2n limits reverse voltage across the optocoupler LED during a positive field transient. D1n blocks the resulting path back into the 5 V rail. R2n limits forward LED current during a negative field transient.

The metal front plate bonds to PE, so most button discharges reach earth before the field loom. These paths remove the former reliance on the GPIO's internal protection diodes. Parasitic coupling, return-path inductance, and actual pulse stress still require the powered and unpowered tests in [qualification.md](qualification.md).

## Implementation status

The connection tables specify the circuit. U5 PCB layout, connectors, and measured transient performance are pending. The harness drawings represent U5 as a module; they do not claim a completed PCB design.
