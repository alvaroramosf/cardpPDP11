# cardpPDP11 — PDP-11 Emulator for M5Stack Cardputer

A PDP-11/40–11/24 emulator running on the **M5Stack Cardputer** (ESP32-S3).
Boot Unix V6, RT-11, RSX-11/M, RSTS/E or Ultrix-11 directly from an SD card —
all from a pocket-sized device with a real keyboard.

![Cardputer Logo](Images/Logo.jpg)

> **Fork of** [Isysxp/PDP11-on-the-M5-Core](https://github.com/Isysxp/PDP11-on-the-M5-Core)
> by Ian Schofield. Original work ported from the M5Stack Core2 (touch screen, WiFi)
> to the Cardputer (physical keyboard, no WiFi required). See
> [`docs/UPSTREAM_README.md`](docs/UPSTREAM_README.md) for the original documentation.

---

## Hardware Required

| Component | Details |
|-----------|---------|
| **M5Stack Cardputer** | ESP32-S3, 240×135 ST7789V2 display, 56-key physical keyboard |
| **MicroSD card** | FAT32 formatted; holds disk images and empty disk templates |
| **USB-C cable** | For flashing and serial debug output |

> **Hardware reset:** The physical side button on the Cardputer performs a hard
> ESP32-S3 reset at silicon level — no software needed.

---

## Emulated Hardware

| Component | Emulated Device |
|-----------|----------------|
| CPU | DEC PDP-11/40 (18-bit) + PDP-11/24 (22-bit, non-split I/D, Unibus map) |
| RAM | Up to 248 KB (18-bit mode) |
| FPU | FP11 floating-point unit |
| Instruction set | EIS (Extended Instruction Set) |
| Disk — RK | RK11 / RK05 cartridge disk |
| Disk — RL | RL11 / RL01 and RL02 disk drives (single and multi-volume images) |
| Console | KL11 serial (mapped to Cardputer keyboard + display) |
| Secondary serial | DL11 (available via USB serial at 115200 baud) |
| Real-time clock | KW11-L line frequency clock |

---

## Supported Operating Systems

| Image file | OS | Boot device |
|---|---|---|
| `rk0_v6_DL.rk05` | Unix V6 (with DL11 second user) | RK05 |
| `RT11_V5_CFB.RL02` | RT-11 V5 (with FORTRAN, BASIC, ...) | RL02 |
| `RT11_V5_MUBasic.RL01` | RT-11 V5 + Multi-User BASIC | RL01 |
| `rsts_v9_iss_forth.rl02` | RSTS/E V9 + FORTH | RL02 |
| `rsx11m46-ccc.rl02` | RSX-11/M 4.6 | RL02 |
| `Ultrix-V2-Full.rl02` | Ultrix-11 V2 (2× RL02 volumes) | RL02 |
| `Ultrix-V3.rl02` | Ultrix-11 V3 (2× RL02 volumes) | RL02 |
| `Ultrix_V3_UX24.rl02` | Ultrix-11 V3 (22-bit / 11/24 mode) | RL02 |

All images are SIMH-compatible. See [`docs/Readme.os`](docs/Readme.os) for a
full list of tested images and [`docs/Readme.Ultrix`](docs/Readme.Ultrix) for
Ultrix-specific notes and multi-volume setup instructions.

---

## SD Card Setup

Format a MicroSD card as **FAT32** and copy the following to its **root**:

```
/
├── Empty_RK05.dsk        ← required blank RK05 placeholder
├── Empty_RL01.dsk        ← required blank RL01 placeholder
├── rk0_v6_DL.rk05       ← (any .rk05 or .rl0x images you want to boot)
├── RT11_V5_CFB.RL02
└── ...
```

The firmware scans the root of the SD for files with `.rk05` or `.rl0`
extensions and lists them in the boot menu automatically.

> The `Empty_RK05.dsk` and `Empty_RL01.dsk` files are **required** as
> placeholder images for the inactive drive. Copy them from the `Images/`
> directory in this repository.

---

## Building & Flashing

This project uses **PlatformIO**. Arduino IDE is **not** supported.

### Prerequisites

```bash
# Install PlatformIO Core (if not already installed)
pip install platformio
```

### Compile

```bash
pio run
```

### Flash

Connect the Cardputer via USB-C, then:

```bash
pio run -t upload
```

### Serial Monitor (debug)

```bash
pio device monitor -b 115200
```

The serial output shows SD card info, disk image selection, and CPU state
messages. It also acts as a parallel console terminal (same as the display).

---

## Controls

### Boot Menu / Options

| Key | Action |
|-----|--------|
| `↑` / `↓` (`;` / `.`) | Navigate menu |
| `Enter` | Confirm selection / save |
| `G0` button | Toggle between emulator and options menu |

### Emulator (PDP-11 terminal)

| Key | Action |
|-----|--------|
| Any printable key | Sent directly to PDP-11 console (KL11) |
| `Ctrl`+`letter` | ASCII control code (e.g. `Ctrl+C` → ETX, `Ctrl+D` → EOT) |
| `Del` | Sends DEL (0x7F) — used as rubout in old Unix |
| `Enter` | Sends CR (0x0D) |
| `Tab` | Sends HT (0x09) |
| `G0` button | Enter ODT monitor mode (halt CPU, inspect/modify registers) |

### ODT Monitor Mode

When the CPU is halted (via `G0`), the Cardputer enters an interactive
ODT-style monitor. You can inspect and modify CPU registers (R0–R7, PS)
and memory locations using the standard PDP-11 ODT command syntax.
Press `P` to resume execution or use the menu to select a new disk and reboot.

---

## Options Menu

Press `G0` at any time during emulation to open the options menu:

| Option | Values |
|--------|--------|
| **Terminal color** | Green · Amber · White · Paper (light mode) |
| **Brightness** | 5 levels |
| **Select disk** | Choose from images found on SD |
| **System info** | Free heap, CPU frequency, SD size |

Settings are saved to **NVS** (non-volatile storage) and restored on next boot.

---

## Project Structure

```
cardpPDP11/
├── src/              ← All C++ source files (PlatformIO build)
│   ├── main.cpp      ← Arduino setup(), display canvas, SD scan
│   ├── avr11.cpp     ← Main emulator loop, keyboard polling
│   ├── kb11.cpp/h    ← PDP-11 CPU (instruction decode & execute)
│   ├── unibus.cpp/h  ← Unibus address space
│   ├── kl11.cpp/h    ← Console serial (KL11)
│   ├── dl11.cpp/h    ← Secondary serial (DL11)
│   ├── rk11.cpp/h    ← RK11 disk controller
│   ├── rl11.cpp/h    ← RL11 disk controller
│   ├── fp11.cpp      ← FP11 floating-point unit
│   ├── kt11.cpp/h    ← KT11 memory management unit
│   ├── kw11.cpp/h    ← KW11-L line clock
│   ├── options.cpp/h ← Settings menu, ODT monitor, NVS persistence
│   └── ...
├── Images/           ← SIMH-compatible disk images for the SD card
├── docs/             ← Additional documentation
│   ├── UPSTREAM_README.md  ← Original M5Core2 README (Ian Schofield)
│   ├── Readme.Ultrix       ← Ultrix-11 setup and multi-volume notes
│   └── Readme.os           ← Full list of tested OS images
├── tools/
│   └── patch_options.py    ← Dev utility for code instrumentation
└── platformio.ini    ← PlatformIO build configuration
```

---

## Credits

- **Ian Schofield** ([@Isysxp](https://github.com/Isysxp)) — original PDP-11 emulator
  for the M5Stack Core2, based on [Pico_1140](https://github.com/Isysxp/Pico_1140).
- **Álvaro Ramos** ([@alvaroramosf](https://github.com/alvaroramosf)) — port to the
  M5Stack Cardputer: PlatformIO migration, keyboard/display adaptation, ODT monitor,
  settings menu, and Cardputer-specific optimizations.
- The PDP-11 emulation core traces back to the work of many contributors in the
  retrocomputing community, including Bob Supnik's SIMH project.

---

## License

This project inherits the license terms of the upstream repository. See
[`docs/UPSTREAM_README.md`](docs/UPSTREAM_README.md) for attribution.
