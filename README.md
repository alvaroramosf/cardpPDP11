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

### Serial Monitor / USB Console

```bash
pio device monitor -b 115200
```

The Cardputer's USB-C port exposes a **CDC serial port** (USB Serial/JTAG).
It serves two purposes simultaneously:

1. **Debug output** — SD card info, disk image selection, free RAM, CPU state.
2. **Parallel PDP-11 console** — every character typed on the physical keyboard
   is also echoed here, and characters sent from the PC terminal are injected
   directly into the PDP-11 KL11 receive register. You can use any terminal
   emulator (e.g. `minicom`, `screen`, PuTTY) at **115200 8N1**.

This means you can operate the PDP-11 entirely from a PC terminal over USB
without touching the Cardputer keyboard.

---

## Controls

### G0 Button — Mode Switching

The orange **G0** button (below the display) is the main mode-switching button.
It cycles through three states:

```
  ┌─────────────┐   G0   ┌─────────────┐   G0   ┌─────────────┐
  │  Emulator   │ ──────▶│  ODT Monitor│ ──────▶│Options Menu │
  │  (running)  │        │  (CPU halt) │        │             │
  └─────────────┘        └─────────────┘        └─────────────┘
         ▲                     │  Esc                  │  Esc / G0
         └─────────────────────┴───────────────────────┘
                              Resume
```

| From state | Press | Result |
|---|---|---|
| **Emulator** | `G0` | CPU halts → enters ODT monitor |
| **ODT Monitor** | `G0` | Opens Options Menu (CPU stays halted) |
| **ODT Monitor** | `Esc` or `P` | Resumes emulation (CPU un-halts) |
| **Options Menu** | `Esc` or `G0` | Saves settings, resumes emulation |

### Emulator — PDP-11 Terminal

| Key | Action |
|-----|--------|
| Any printable key | Sent directly to PDP-11 console (KL11) |
| `Ctrl`+`letter` | ASCII control code (`Ctrl+C` → ETX, `Ctrl+D` → EOT …) |
| `Del` | Sends DEL (0x7F) — used as rubout in old Unix |
| `Enter` | Sends CR (0x0D) |
| `Tab` | Sends HT (0x09) |

### ODT Monitor — Commands

The ODT prompt is `@`. Commands follow standard PDP-11 ODT syntax:

| Input | Effect |
|---|---|
| `nnnnnn/` | Open memory address *nnnnnn* (octal), display its value |
| `Rn/` | Open register Rn (R0–R7) |
| `RS/` | Open Processor Status Word (PSW) |
| `value CR` | Write *value* to the open location and close it |
| `value LF` (`.`) | Write *value* and advance to next location |
| `CR` (empty) | Close location without writing |
| `G` / `nnnG` | Go: resume from current PC (or set PC=*nnn* first) |
| `P` | Proceed: resume execution at current PC |
| `Esc` | Resume execution without changing any register |

See [`docs/ODT_Guide.md`](docs/ODT_Guide.md) for a full command reference
and worked examples with machine-code programs.

---

## Options Menu

Press `G0` while in ODT monitor to open the options menu (CPU remains halted):

| Option | Values |
|--------|--------|
| **Disk Image** | Choose from all `.rk05` / `.rl0x` images found on SD |
| **Text Colour** | Green (phosphor) · Amber · White · Paper (light on dark) |
| **Brightness** | 5 levels (20 % – 100 %) |
| **System Info** | Version, free RAM, SD total/used |
| **Battery Status** | Estimated level and voltage |
| **System Reset** | Soft-reboot PDP-11 with the currently selected disk |

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

- **Ian Schofield** ([@Isysxp](https://github.com/Isysxp)) — M5Stack Core2 port,
  based on his [Pico_1140](https://github.com/Isysxp/Pico_1140) project (RP2040).
- **Álvaro Ramos** ([@alvaroramosf](https://github.com/alvaroramosf)) — port to the
  M5Stack Cardputer: PlatformIO migration, keyboard/display adaptation, ODT monitor,
  settings menu, and Cardputer-specific optimizations.
- **Dave Cheney** — [avr11](https://github.com/dchest/avr11), the PDP-11 emulation
  core for ATmega2560 that this project descends from (via cpp11 → Pico_1140).
- **Julius Schmidt** — original JavaScript PDP-11 simulator that inspired avr11.
- Disk images are compatible with **SIMH** (Bob Supnik et al.) but the emulation
  core is an independent implementation, not a SIMH port.

---

## License

This project inherits the license terms of the upstream repository. See
[`docs/UPSTREAM_README.md`](docs/UPSTREAM_README.md) for attribution.
