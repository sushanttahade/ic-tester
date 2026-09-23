# Results & Calibration

## Accuracy by mode

| Parameter | Accuracy / Success Rate | Remarks |
|---|---|---|
| Voltage Measurement | 99.2% | Excellent within 0–25V range |
| Resistance Measurement | 98.5% | Very precise with calibration |
| Current Measurement | 96.5% | Stable and noise-resistant |
| Capacitance Measurement | 93.7% | Reliable for general-purpose applications |
| Digital IC Testing | 95% | Logic comparison using truth tables |
| Linear IC Testing | 85–90% | Suitable for basic op-amp validation |

## Calibration methodology

- **Voltage**: ADC readings were initially off due to non-linear response and reference-voltage drift. Calibrated against a precision voltage source + reference DMM at 0.5V/1V/2V/3V; post-calibration accuracy held to ±0.05V across 0–25V (with divider applied).
- **Resistance**: Calibrated with a series of precision resistors; voltage-divider formula tuned via software offset/scaling constants. ±2% for low resistances, ±5% up to 500kΩ.
- **Current**: Factory ACS712 calibration values as baseline, then fine-tuned against DMM readings with a software offset correction (zero-current accuracy) and scaling factor (load-current matching). Error margin <±100mA.
- **Capacitance**: Verified against known capacitors; RC charge/discharge timing constants adjusted in firmware.
- **IC Tester**: Verified against known-working and deliberately faulty digital + linear ICs. Logic thresholds and timing tuned to sharpen pass/fail detection.

## Comparison vs. commercial devices

| Feature | Commercial Multimeter | Commercial IC Tester | This device |
|---|---|---|---|
| Functions | Voltage, current, resistance, continuity, capacitance, inductance | Logic testing only | All multimeter functions + IC testing |
| IC testing | Not available | Digital and linear (limited) | Digital + linear ICs |
| Display | LCD | LCD | OLED |
| Connectivity | No | No | Wi-Fi/BT capable (ESP32) |
| Cost | ₹1000–2000 | ₹1500–3000 | ₹300–500 |
| Flexibility | Fixed functions | Fixed functions | Reprogrammable |

## Test cases (digital IC)

Example — 7400 NAND gate: ESP32 drives all 4 input combinations of (A, B) and reads the output, comparing against expected NAND logic. If all outputs match, IC is declared "Working"; any mismatch flags "Faulty". Same 2ⁿ-combination sweep approach generalizes to AND/OR/NOT and any other 74xx gate defined via a pin-out + truth-table structure.

## Example — linear IC (LM741 op-amp)

Configured as inverting/non-inverting amplifier or unity-gain buffer with known external resistors. A stable DC input is applied; measured Vout is checked against the theoretical Vout = G × Vin within a defined tolerance band to account for analog variation.
