# Mini Speech

Voice-controlled TinyML maze game for the STM32F401RE with OLED output, audio preprocessing, and a small command classifier.

## Overview

- Captures microphone input with the ADC and processes it through STFT-based features.
- Runs a 5-class TinyML model for the commands `down`, `left`, `random`, `right`, and `up`.
- Uses an SSD1306 OLED to show the maze, score bars, and status screens.
- Organizes the firmware into separate modules for display, game logic, hardware setup, and the AI wrapper.

## Build And Upload

This project is built with PlatformIO.

```bash
pio run
pio run --target upload
```

## Author

Silas Loeffler, assisted by GitHub Copilot.

## Notes

The full written report can be summarized here later as a shorter project description.
