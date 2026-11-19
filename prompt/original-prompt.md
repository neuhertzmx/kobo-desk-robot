# The original prompt

This is the exact prompt I gave the AI at the start of the project. Everything in Kobo v1 started here. What survived and what didn't is explained in the main [README](../README.md#what-changed-from-the-ais-design).

---

I want to design and build a small desktop robot pet that sits on my desk. I already bought these parts and I want to build it around them:

* ESP32-S3 Touch LCD 1.69" development board (round-corner 240×280 touch display, Waveshare-style board)
* Mini speaker, 8 Ω, 1 W, 20 mm diameter
* Rechargeable LiPo battery, 3.7 V, 250 mAh, 30 × 20 × 5 mm

The pet should show an animated, expressive face on the display, react when I touch the screen, make sounds through the speaker, and run on the battery without a cable for as long as possible. It will be 3D printed (PLA, FDM printer).

Give me:

1. The full mechanical design of the enclosure: overall shape and dimensions, each printed part, wall thickness, how the board, speaker and battery are mounted, how it assembles, and how I access the USB port to charge and program it.
2. The electronics: how everything connects, which pins to use, how the battery is charged, and how the speaker is driven.
3. Estimated battery life, showing the calculation.
4. A bill of materials. If you think I need any extra parts, add them, explain exactly why, and keep them to a minimum.
5. The firmware architecture: libraries, main states of the pet (idle, happy, sleepy, etc.), and a first working code example.

Be specific with numbers. If you're unsure about a detail of my board, say so instead of guessing.

---

## Looking back

The prompt was clear about the parts and the deliverables, but it **didn't ask the AI to check whether the parts were compatible with each other**, for example the battery capacity against the board's charge current. If I wrote it again, I'd add:

> "Before designing anything, check that the parts I bought are compatible with each other (voltages, currents, charge rates) and tell me if something is a risk."
