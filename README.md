# Multifunctional IC Tester

An ESP32-based diagnostic instrument that folds a digital multimeter and a logic/linear IC tester into a single compact device — voltage, auto-ranging resistance, current, capacitance, continuity, and truth-table-driven IC testing, all switchable from an OLED + push-button interface.

Published as *"Multifunctional IC Testor"*, International Journal of Engineering Development and Research (IJEDR), Vol. 13, Issue 4, Nov 2025. Co-authored with Vivekanand Swami, under the guidance of Dr. Lenina S.V.B., Dept. of E&TC, SGGSIE&T Nanded.

## Why this exists

Commercial multimeters measure voltage/current/resistance but can't test ICs. Commercial IC testers are logic-family-specific, bulky, and cost ₹1500–3000+. Neither is affordable or flexible enough for students and field engineers who need both. This device merges the two into one ESP32-driven tool for roughly ₹300–500 in parts, while staying fully reprogrammable — a fixed-function commercial meter can't do that.

## What it does

| Mode | Method | Measured accuracy |
|---|---|---|
| Voltage | Voltage-divider scaling into ESP32's 0–3.3V ADC range | ±0.05V (0–25V range) |
| Resistance | Auto-ranging voltage divider (100kΩ/10kΩ/1kΩ reference legs) | ±2% low range, ±5% up to 500kΩ |
| Current | ACS712 Hall-effect sensor (galvanic isolation) | ±100mA error margin |
| Capacitance | RC charge-time method | 93.7% reliable, nF–low µF range |
| Continuity | GPIO output/input loopback | — |
| Digital IC test | Drives 2ⁿ input combinations, compares against truth table (74xx series: AND/OR/NOT/NAND implemented) | 95% correct logic detection |
| Linear IC test | Configures op-amp (e.g. LM741) in known gain stage, compares measured Vout against calculated Vout | 85–90% |

Full accuracy breakdown and methodology are in `docs/results.md`.

## Why ESP32

Dual-core processor, 12-bit ADC, large GPIO count, and built-in Wi-Fi/BT headroom for future remote monitoring — chosen over simpler MCUs (e.g. Arduino Uno) specifically for ADC resolution and pin count, since IC testing alone needs a wide GPIO bank for the ZIF socket interface.

## Hardware

- ESP32 Dev Module
- SSD1306 OLED (128×64, I²C — SDA: GPIO21, SCL: GPIO22)
- ACS712 current sensor
- Voltage divider network (R1 33kΩ / R2 7.5kΩ for voltage mode)
- Auto-ranging resistor bank (100kΩ / 10kΩ / 1kΩ) for resistance mode
- ZIF/DIP socket wired to dedicated GPIOs for IC-under-test
- 3× tactile push buttons (mode switch, IC-type cycle, test trigger)

Prototype was built and validated on a breadboard/Zero PCB (see `docs/images/prototype.jpg`); dedicated PCB files go in `/hardware/kicad` as that work progresses.

## Firmware

Arduino-framework C++ for ESP32. See `firmware/src/main.cpp`. Current implementation:
- 3 main modes (IC Test / Voltmeter / Ohmmeter) cycled via SWITCH button
- 4 digital IC types implemented (AND, OR, NOT, NAND) cycled via MODE button, extensible via the `ICType` enum + truth-table test functions
- Averaged (10-sample) ADC reads for voltage/resistance stability
- Auto-ranging resistance logic picks whichever reference resistor puts Vout closest to VCC/2 for best measurement accuracy
- OLED live-updates every 500ms in continuous-read modes

## Repo structure

```
ic-tester/
├── firmware/
│   └── src/
│       └── main.cpp        # ESP32 firmware
├── hardware/
│   └── kicad/               # Schematic + PCB design files (in progress)
├── docs/
│   ├── results.md            # Accuracy tables, calibration notes, comparison vs commercial devices
│   ├── circuit-notes.md       # Measurement principle breakdown per mode
│   └── images/
│       └── prototype.jpg
└── tests/
```

## Calibration notes

Every analog mode was calibrated against a reference DMM: known voltages/resistors were applied, raw ADC readings logged, and offset/scaling constants tuned in firmware to correct for ESP32 ADC non-linearity and reference-voltage drift. Full methodology in `docs/results.md`.

## Status

Core firmware (3 modes, 4 digital IC types) working and validated on breadboard. Next: LM741/linear IC test routine, PCB layout, expanded IC truth-table library beyond AND/OR/NOT/NAND.

## License

MIT
