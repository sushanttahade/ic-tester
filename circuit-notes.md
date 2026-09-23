# Circuit Notes

## The core constraint: ESP32 ADC is 0–3.3V only

Every analog measurement mode exists to work around this one limit. Any source above 3.3V must be scaled down before it touches an ADC pin, or it risks damaging the chip.

## Voltage measurement

Voltage divider: R1 (33kΩ) and R2 (7.5kΩ) in series between Vin and GND. Midpoint (Vout) goes to an ADC pin, sized so max expected Vin (~30V) lands at 3.3V at the ADC.

```
Vin = Vadc × (R1 + R2) / R2
```

## Resistance measurement

A known reference resistor forms a divider with the unknown resistor. Firmware auto-ranges across three reference legs (100kΩ / 10kΩ / 1kΩ) and picks whichever puts the measured Vout closest to VCC/2 — that's the point of maximum sensitivity on a voltage-divider ADC read.

```
R2 = R1 × Vout / (Vout + Vin)
```

## Current measurement

ACS712 Hall-effect sensor in series with the load. Hall-effect sensing avoids the voltage drop and power loss of a shunt-resistor approach, and gives galvanic isolation between load and ESP32. ADC reads the sensor's analog output, converted to current via its sensitivity spec (e.g. 185mV/A for ACS712-5A).

## Capacitance measurement

RC charge-time method: a GPIO pin discharges the capacitor fully (LOW), then switches HIGH to begin charging through a known resistor. ESP32 times how long the ADC-monitored junction takes to reach a threshold voltage. Capacitance is derived from that charge time and the known resistor value. Works well for nF to low-µF range — few external components needed, which suits MCU-based measurement.

## Continuity

Two GPIOs: one output (driven HIGH), one input. If current flows through the test probes between two points, the input pin reads HIGH → continuity confirmed. Open circuit or high resistance keeps the input LOW.

## IC Tester — digital ICs

GPIOs are split into outputs (drive IC inputs) and inputs (read IC outputs), wired to the IC socket. Firmware iterates through all 2ⁿ input combinations for an N-input gate, compares each measured output against the expected value from the truth table. All combinations must match for a PASS.

Currently implemented: AND, OR, NOT, NAND (see `testAND()`, `testOR()`, `testNOT()`, `testNAND()` in `firmware/src/main.cpp`). Extending to more 74xx gates just means adding another truth-table test function and an `ICType` enum entry.

## IC Tester — linear ICs

Different problem: analog in, analog out. Op-amp is wired into a known configuration (inverting/non-inverting amp, unity-gain buffer). ESP32 applies a known DC input, reads the output via ADC, and checks it against the theoretically expected value (Vout = G × Vin) within a tolerance band — analog variation means you can't expect exact match the way you can with digital logic.

## I²C to the OLED

SSD1306 display, ESP32 as I²C master, display as slave at address 0x3C (or 0x3D fallback). Two wires: SDA (GPIO21), SCL (GPIO22).

## Calibration is what separates a tool from a toy

Physical resistors carry tolerance (5%/1%), the ESP32's actual 3.3V rail typically drifts a little (e.g. 3.28–3.35V), and the ADC itself isn't perfectly linear across its full range. None of that shows up until you check against a reference DMM — calibration means measuring your board's *true* ratios and reference resistor values, then baking those corrections into firmware constants rather than trusting datasheet nominal values.
