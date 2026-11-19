# Kobo — an AI-designed desk pet robot

Kobo is a small desk pet built on the Waveshare **ESP32-S3 Touch LCD 1.69"** board.

The idea behind the project: I asked an AI to design a desk robot around parts I had already bought. Then I checked its design as a mechanical engineer, measured what it got wrong, redesigned it in SolidWorks and built it. This repo has everything: the original prompt, the CAD, the firmware and the mistakes.

> **Status:** v1 — finished. It works, but it's not perfect. See [Known issues](#known-issues).

📺 Full build series on YouTube: [@neuhertz](https://www.youtube.com/@neuhertz)

- Episode 1 — [The AI design](https://youtu.be/xmR51z2DC2A)
- Episode 2 — [My AI-designed robot was overcharging its battery](https://youtu.be/8F7hFOqv2Bs)
- Episode 3 — Assembly and the honest result *(link coming soon)*

---

## What it does

| Action | Kobo's reaction |
|---|---|
| Tap the screen | Happy eyes + chirp |
| Hold for 1 s | Heart eyes + melody |
| Tap 5 times fast | Gets angry |
| 20 s without touching | Gets sleepy and yawns |
| 45 s without touching | Falls asleep (Zzz, snores, dims the screen) |
| Touch it while asleep | Wakes up surprised |

It also blinks and looks around on its own.

---

## Repo structure

```
kobo-desk-robot/
├── cad/        STL files for printing + STEP assembly
├── KoboFirmware/  KoboFirmware.ino (open this folder in Arduino IDE)
├── prompt/     The original prompt I gave the AI
└── README.md
```

---

## Hardware

| Part | Notes |
|---|---|
| Waveshare ESP32-S3 Touch LCD 1.69" | ST7789V2 display (240×280), CST816T touch, built-in passive buzzer (GPIO42). Use the **new** board version (name printed on the PCB) |
| USB-C cable | Kobo v1 runs on USB power only |
| 3D printed enclosure | PLA, FDM. 3 parts, see below |

### Printed parts (`/cad`)

| File | Part |
|---|---|
| `kobo-body.stl` | Main body |
| `kobo-head.stl` | Head (holds the board and screen) |
| `kobo-button.stl` | Button actuator |
| `kobo-enclosure.step` | Full enclosure assembly, for editing (board not included) |

Suggested settings: PLA, 0.2 mm layers, 3 walls, 20 % infill. **Print a tolerance test first:** printed holes usually come out 0.1–0.3 mm smaller than designed.

The 3D model of the board itself is available from [Waveshare](https://www.waveshare.com/wiki/ESP32-S3-Touch-LCD-1.69).

### What changed from the AI's design

The AI's design was a good starting point, but these points did not survive real measurements:

- **No battery.** The board's charger is set to up to 2 A (R25 = 82 kΩ). I measured about **0.63 A going into a 250 mAh cell**, roughly 2.5× what that battery should take. I removed it instead of risking it.
- **No external speaker.** The built-in buzzer is enough for beeps, chirps and short melodies, and saves space and wiring.
- **Real dimensions.** The AI assumed a 10 mm module; it's **10.7 mm**. The visible screen area is **not centered** on the glass (top margin 2.58 mm, bottom 5.12 mm).
- **Screen window.** Changed to **29.8 × 34.4 mm, R5.9**, with a lip that stops the glass. A window the size of the glass lets the board slide through.
- **USB-C slot.** Changed to an oblong **12 × 8 mm, R4** slot, for an 11 × 7 mm plug.

---

## Firmware

### Requirements
- Arduino IDE with **esp32 by Espressif** (tested on core 3.3.x)
- Library: **GFX Library for Arduino** (by moononournation)

### Board settings (Tools menu)
| Setting | Value |
|---|---|
| Board | **ESP32S3 Dev Module** |
| USB CDC On Boot | Enabled |
| PSRAM | OPI PSRAM |
| Flash Size | 16MB |

> ⚠️ If you get `'digitalPinToGPIONumber' is not a type`, you selected the wrong board, or Pin Numbering is set to "By Arduino pin". Select **ESP32S3 Dev Module**, or set Pin Numbering to **By GPIO number (legacy)**.

### Pins used
| Function | GPIO |
|---|---|
| LCD DC / CS / SCK / MOSI / RST / Backlight | 4 / 5 / 6 / 7 / 8 / 15 |
| Touch SDA / SCL / RST | 11 / 10 / 13 |
| Buzzer | 42 |
| Power hold (SYS_EN) | 41 |

### Customizing
At the top of `KoboFirmware.ino` you can change:
- `TIEMPO_SUENO` / `TIEMPO_DORMIR`: how long until it gets sleepy or falls asleep
- `RONQUIDOS`: set it to `false` for silent sleep
- The `SONIDOS` section: each sound is a list of `{frequency_Hz, duration_ms}`

*(Variable names and comments are in Spanish, my native language.)*

---

## Known issues

I'm documenting these because they're part of the project:

- **Cracked screen.** I damaged the display during assembly. It still works, but there's a white line, and colors leave streaks where they cross it.
- **Button holes too tight.** 3D printed holes come out smaller than designed. I opened them with a drill and broke part of the case. Next time: print a tolerance test first, and open holes by hand.
- **USB only.** No battery means Kobo has to stay plugged in, which limits it a lot as a desk pet.
- **Simple behavior.** It reacts and has moods, but it doesn't do much more than that.

---

## Lessons learned

1. **Calculate before you buy.** Check the battery capacity against the charger current *before* ordering parts.
2. **If an AI writes your code, learn enough to read it.**
3. **What you get from an AI depends on how you ask.** The full prompt is in [`/prompt`](prompt/).
4. **Design for 3D printing tolerances**, especially in small assemblies.

---

## What's next

Kobo v1 is the base for a bigger robot: a larger screen, a properly sized battery, real speakers and maybe a small on-device AI model.

---

## License

- **Code** (`/KoboFirmware`): [MIT](LICENSE)
- **CAD and STL files** (`/cad`): [CC BY-NC 4.0](https://creativecommons.org/licenses/by-nc/4.0/). Use them, modify them and share them with credit, but not for commercial purposes.

Made by **Neuhertz** · [YouTube](https://www.youtube.com/@neuhertz) · neuhertzmx@gmail.com
