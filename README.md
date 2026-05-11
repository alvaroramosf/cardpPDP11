# cardpPDP11 — PDP-11 Emulator for M5Stack Cardputer

A PDP-11/40 and PDP-11/23 emulator running on the **M5Stack Cardputer** (ESP32-S3).
Boot Unix V6, RT-11, RSX-11/M or RSTS/E directly from an SD card —
all from a pocket-sized device with a real keyboard.

![Cardputer Logo](Images/Logo.jpg)

> **Fork of** [Isysxp/PDP11-on-the-M5-Core](https://github.com/Isysxp/PDP11-on-the-M5-Core)
> by Ian Schofield. Original work ported from the M5Stack Core2 (touch screen, WiFi)
> to the Cardputer (physical keyboard, no WiFi required). See
> [`docs/UPSTREAM_README.md`](docs/UPSTREAM_README.md) for the original documentation.

---

## Table of Contents
1. [Hardware Required](#hardware-required)
2. [Emulated Hardware](#emulated-hardware)
3. [Supported Operating Systems](#supported-operating-systems)
4. [SD Card Setup](#sd-card-setup)
5. [Booting UNIX V6](#booting-unix-v6)
6. [Building & Flashing](#building--flashing)
7. [Controls](#controls)
8. [Options Menu](#options-menu)
9. [Project Structure](#project-structure)
10. [Changelog](#changelog)
11. [Credits](#credits)

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
| CPU | DEC PDP-11/40 (18-bit Unibus) **or** PDP-11/23 (22-bit Q-Bus, F-11 chip) — selectable |
| RAM | Up to 248 KB (18-bit mode, limited by internal SRAM) |
| FPU | FP11 floating-point unit |
| Instruction set | EIS (Extended Instruction Set) + MUL/DIV/ASH/ASHC |
| Disk — RK | RK11 / RK05 cartridge disk (~2.4 MB) |
| Disk — RL | RL11 / RL01 (~5 MB) and RL02 (~10 MB) disk drives |
| Console | KL11 serial (mapped to Cardputer keyboard + display) |
| Secondary serial | DL11 (available via USB serial at 115200 baud) |
| Real-time clock | KW11-L line frequency clock |

### CPU Model Differences

| Feature | PDP-11/40 | PDP-11/23 (F-11) |
|---|---|---|
| Address bus | 18-bit Unibus | 22-bit Q-Bus |
| MFPT instruction | Illegal (trap 010) | Returns `1` (F-11 ID) |
| 22-bit MMU | OS must activate | Active from reset |
| Compatible images | 18-bit OS builds | 22-bit OS builds |

> **Note:** With only 248 KB of internal SRAM (Cardputer has no PSRAM),
> both models are limited to 248 KB of RAM regardless of address bus width.
> Systems requiring >248 KB (e.g. Ultrix-11, 2.9BSD) will not run.

---

## Supported Operating Systems

### PDP-11/40 images (`/pdp11/pdp11-40/`)

| Image file | OS | Boot device |
|---|---|---|
| `rk0_v6_DL.rk05` | Unix V6 (with DL11 second user) | RK05 |
| `RT11_V5_CFB.RL02` | RT-11 V5 (FORTRAN, BASIC, …) | RL02 |
| `RT11_V5_MUBasic.RL01` | RT-11 V5 + Multi-User BASIC | RL01 |
| `rsts_v9_iss_forth.rl02` | RSTS/E V9 + FORTH | RL02 |
| `rsx11m46-ccc.rl02` | RSX-11/M 4.6 | RL02 |

### PDP-11/23 images (`/pdp11/pdp11-23/`) — require >248 KB RAM

| Image file | OS | Boot device |
|---|---|---|
| `Ultrix-V2-Full.rl02` | Ultrix-11 V2 (2× RL02 volumes) | RL02 |
| `Ultrix-V3.rl02` | Ultrix-11 V3 (2× RL02 volumes) | RL02 |
| `Ultrix_V3_UX24.rl02` | Ultrix-11 V3 (22-bit / 11/24 mode) | RL02 |

> Ultrix images require PSRAM (not available on stock Cardputer) and are
> included for reference / future hardware with PSRAM expansion.

All images are SIMH-compatible. See [`docs/Readme.os`](docs/Readme.os) for a
full list of tested images.

---

## SD Card Setup

Format a MicroSD card as **FAT32** and create the following structure:

```
/
└── pdp11/
    ├── Empty_RK05.dsk        ← required blank RK05 placeholder
    ├── Empty_RL02.dsk        ← required blank RL02 placeholder
    ├── pdp11-40/             ← 18-bit OS images (PDP-11/40 compatible)
    │   ├── rk0_v6_DL.rk05
    │   ├── RT11_V5_CFB.RL02
    │   └── ...
    └── pdp11-23/             ← 22-bit OS images (PDP-11/23 / need >248 KB)
        ├── Ultrix-V3.rl02
        └── ...
```

The firmware scans `/pdp11/` **recursively** for files with `.rk05` or `.rl0x`
extensions. The disk selection menu allows navigating these subdirectories.

> **Tip:** Organize your images by OS or CPU model (e.g., `/pdp11/pdp11-40/unix_v6/`).

---

## Booting UNIX V6

### Single Disk Boot
1. Select your root image (e.g., `unix0_v6_rk_DL.rk05`) in **RK05 Drive 0**.
2. Exit the menu to boot.
3. At the `@` prompt, type `rkunix` and press Enter.
4. When the `login:` prompt appears, type `root` and press Enter.
5. When the `#` prompt appears, you are in!

### Multi-Disk Configuration
To access extra volumes (like `/usr` or `/doc` from SIMH distributions), you should map the images as follows in the **Emulation Settings** menu:

| RK Drive | Image File | Mount Point | Content |
|---|---|---|---|
| **RK0** | `unix0_v6_rk_DL.rk05` | `/` (Root) | Boot kernel and base system |
| **RK1** | `unix1_v6_rk.rk05` | `/usr` | User files, binaries, games |
| **RK2** | `unix2_v6_rk.rk05` | `/doc` | Documentation and source code |
| **RK3** | `unix3_v6_rk.rk05` | (optional) | Extra binaries or source |

#### Mounting Procedure
1. Boot from **RK0** (`rkunix`).
2. Log in as `root`.
3. Create the device nodes for the extra drives (one-time setup):
   ```bash
   /etc/mknod /dev/rk1 b 0 1
   /etc/mknod /dev/rk2 b 0 2
   /etc/mknod /dev/rk3 b 0 3
   ```
4. Mount the volumes:
   ```bash
   /etc/mount /dev/rk1 /usr
   /etc/mount /dev/rk2 /doc
   ```
5. Verify the content with `ls /usr` or `ls /doc`.

---

---

## Building & Flashing

This project uses **PlatformIO**. Arduino IDE is **not** supported.

### Prerequisites

```bash
pip install platformio
```

### Compile

```bash
pio run
```

### Flash

```bash
pio run -t upload
```

### Serial Monitor / USB Console

```bash
pio device monitor -b 115200
```

The USB-C port exposes a CDC serial port serving two purposes:

1. **Debug output** — SD card info, disk selection, free RAM, CPU state on HALT.
2. **Parallel PDP-11 console** — characters from the PC terminal are injected
   into the KL11 receive register.

---

## Controls

### G0 Button

The orange **G0** button (below the display) opens the **Options Menu** directly,
pausing the CPU while the menu is open and resuming on exit.

```
  ┌─────────────┐   G0   ┌─────────────┐
  │  Emulator   │ ──────▶│Options Menu │
  │  (running)  │        │             │
  └─────────────┘        └─────────────┘
         ▲                     │  Esc / G0
         └─────────────────────┘
              (CPU resumes)
```

### Emulator — PDP-11 Terminal

| Key | Action |
|-----|--------|
| Any printable key | Sent directly to PDP-11 console (KL11) |
| `Ctrl`+`letter` | ASCII control code (`Ctrl+C` → ETX, `Ctrl+D` → EOT …) |
| `Del` | Sends DEL (0x7F) — used as rubout in old Unix |
| `Enter` | Sends CR (0x0D) |
| `Tab` | Sends HT (0x09) |

### HALT behaviour

When the PDP-11 executes a `HALT` instruction in kernel mode:
- The CPU enters wait state (emulation pauses)
- Register state is printed to USB serial for debugging
- Press **G0** to open the Options Menu (and optionally reset the machine)

> A hardware-faithful F-11 ODT console interface (PDP-11/23 mode) is planned
> for a future version.

---

## Options Menu

| Option | Values |
|--------|--------|
| **Disk Image** | Navigate folders and select `.rk05` / `.rl0x` images |
| **CPU Model** | PDP-11/40 (18-bit) · PDP-11/23 (22-bit F-11) |
| **Text Colour** | Green (phosphor) · Amber · White · Paper |
| **Brightness** | 5 levels (20 % – 100 %) |
| **System Info** | Version, CPU model, free RAM, SD total/used |
| **Battery Status** | Estimated level and voltage |
| **System Reset** | Soft-reboot PDP-11 with current disk |

Settings are saved to **NVS** and restored on next boot.

---

## Project Structure

```
cardpPDP11/
├── src/              ← All C++ source files (PlatformIO build)
│   ├── main.cpp      ← Arduino setup(), display canvas, SD scan
│   ├── avr11.cpp     ← Main emulator loop, keyboard polling
│   ├── kb11.cpp/h    ← PDP-11 CPU (instruction decode & execute, MFPT, reset)
│   ├── unibus.cpp/h  ← Unibus / Q-Bus address space
│   ├── kl11.cpp/h    ← Console serial (KL11)
│   ├── dl11.cpp/h    ← Secondary serial (DL11)
│   ├── rk11.cpp/h    ← RK11 disk controller
│   ├── rl11.cpp/h    ← RL11 disk controller
│   ├── fp11.cpp      ← FP11 floating-point unit
│   ├── kt11.cpp/h    ← KT11 memory management unit
│   ├── kw11.cpp/h    ← KW11-L line clock
│   └── options.cpp/h ← Settings menu, NVS persistence
├── Images/           ← SIMH-compatible disk images for the SD card
│   ├── Empty_RK05.dsk
│   ├── Empty_RL01.dsk
│   ├── pdp11-40/     ← 18-bit OS images (PDP-11/40)
│   └── pdp11-23/     ← 22-bit OS images (PDP-11/23, need >248 KB RAM)
├── docs/             ← Additional documentation
│   ├── UPSTREAM_README.md
│   ├── Readme.Ultrix
│   └── Readme.os
└── platformio.ini    ← PlatformIO build configuration
```

---

## Changelog

### v0.1.3
- **feat:** Multi-disk support for RK11 (connect up to 4 RK05 drives simultaneously).
- **feat:** Folder navigation in the disk selection menu (recursive browser).
- **feat:** G0 button exit propagation — pressing G0 now exits back to the emulator from any submenu.
- **fix:** Corrected RKDS (status) and RKDA (drive selection) logic for multi-drive compatibility.
- **refactor:** Reorganized UNIX V6 images to separate 11/40 and 11/45 (Supervisor mode) versions.

### v0.1.2
- **feat:** CPU model selector — choose between PDP-11/40 (18-bit) and PDP-11/23 (22-bit F-11) from the Options Menu
- **fix:** RL11 boot ROM had wrong CSR address (`0174400` → `0774400`); RL02/RL01 images now boot correctly
- **fix:** Boot disk verification now checks the actual boot file (RL when booting RL) instead of always checking RK
- **refactor:** Disk images moved to `/pdp11/` on SD card; recursive scan supports subdirectories (`pdp11-40/`, `pdp11-23/`)
- **refactor:** Remove custom ODT monitor (pending hardware-faithful F-11 ODT implementation)
- **refactor:** G0 button opens Options Menu directly (no intermediate ODT layer)

### v0.1.1
- ODT-style monitor mode (register/memory inspect and modify)
- Persistent settings via NVS

### v0.1.0
- Initial port from M5Stack Core2 to Cardputer

---

## Credits

- **Ian Schofield** ([@Isysxp](https://github.com/Isysxp)) — M5Stack Core2 port,
  based on his [Pico_1140](https://github.com/Isysxp/Pico_1140) project (RP2040).
- **Álvaro Ramos** ([@alvaroramosf](https://github.com/alvaroramosf)) — port to
  M5Stack Cardputer: PlatformIO migration, keyboard/display adaptation,
  CPU model selection, settings menu, and Cardputer-specific optimizations.
- **Dave Cheney** — [avr11](https://github.com/dchest/avr11), the PDP-11 emulation
  core for ATmega2560 that this project descends from (via cpp11 → Pico_1140).
- **Julius Schmidt** — original JavaScript PDP-11 simulator that inspired avr11.
- Disk images are compatible with **SIMH** but the emulation core is an
  independent implementation, not a SIMH port.

---

## License

This project inherits the license terms of the upstream repository. See
[`docs/UPSTREAM_README.md`](docs/UPSTREAM_README.md) for attribution.
