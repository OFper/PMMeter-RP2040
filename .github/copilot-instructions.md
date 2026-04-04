---
name: workspace
version: 1
description: "Project-level guidance for PMMeter-RP2040: an RP2040-based PM2.5 monitor firmware sketch."
---

# PMMeter-RP2040 Workspace Instructions

## What this project is
- A single-file Arduino sketch: `PMMeter-RP2040.ino`.
- Firmware for an RP2040-based board that reads PM2.5 sensor data, formats results, and reports values.
- No existing CI, build scripts, or supporting documentation are included in the repository.
- The sketch is currently empty, so changes should be conservative and focused on the hardware interface and output flow.

## How to help
- Focus on the Arduino/Arduino-compatible RP2040 environment.
- Keep changes simple and hardware-centric: pin mapping, sensor sampling, filtering, display/status logic, and serial output.
- Preserve the single-sketch structure unless splitting into helper files is clearly justified.
- When adding a dependency, prefer standard Arduino libraries and document the required library names.
- Prefer implementing sensor polling, averaging, and alert thresholds in the sketch itself rather than introducing external frameworks.

## Known workflow
- Use the Arduino IDE or `arduino-cli` for compile/upload.
- Example commands (adapt board core and serial port as needed):
  - `arduino-cli compile --fqbn arduino:mbed_rp2040:rpipico PMMeter-RP2040.ino`
  - `arduino-cli upload -p /dev/ttyACM0 --fqbn arduino:mbed_rp2040:rpipico PMMeter-RP2040.ino`

## Questions to ask before making changes
- What PM2.5 sensor model and interface are being used?
- What output method is required: serial, display, buzzer, or LEDs?
- Which RP2040 board variant and pin mapping are intended?
- Should firmware support calibration, averaging, or alert thresholds?
- Is there a preferred sensor update interval or display refresh rate?

## Things to avoid
- Do not assume build or test automation exists in this repository.
- Avoid introducing large new subsystems without explicit user direction.
- Do not change the project structure unless a clear benefit is established.
- Do not add unrelated board support or extra libraries unless needed for the PM2.5 monitor feature.
