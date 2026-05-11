# ODT — On-Line Debugging Technique

> **Status: Removed in v0.1.2**
>
> The custom software ODT monitor has been removed pending a
> hardware-faithful implementation of the F-11 chip's built-in ODT
> (planned for a future version).

## Background

The PDP-11 ODT (On-Line Debugging Technique) is a built-in debug interface
that activates when the CPU halts. Its behaviour differs by model:

| Model | ODT type | Activation |
|---|---|---|
| **PDP-11/40** | No hardware ODT | Physical front panel (lights & switches) |
| **PDP-11/23** (F-11) | Hardware ODT in chip | HALT in kernel mode → ODT via console |

On the PDP-11/23, when the CPU executes `HALT` in kernel mode, the F-11 chip
itself takes over the KL11/DL11 console and presents the `@` prompt. Commands
follow the standard DEC ODT syntax: open memory (`nnnn/`), open registers
(`Rn/`, `RS/`), deposit values, proceed (`P`), go (`G`).

## Current HALT behaviour (v0.1.2)

When the emulated CPU executes `HALT` in kernel mode:

1. The CPU state (registers, PSW, PC) is printed to the USB serial port.
2. The CPU enters wait state (`wtstate = true`).
3. Press **G0** to open the Options Menu — from there you can do a System Reset
   to restart with the same or a different disk image.

## Planned: Hardware-faithful F-11 ODT

A future version will implement the F-11 ODT protocol correctly:

- When `HALT` occurs, the emulator injects `@` into the PDP-11 console terminal
  (visible on the Cardputer screen and USB serial).
- The user types standard ODT commands through the normal keyboard/terminal.
- `P` (Proceed) or `G` (Go) resume execution.
- This is only active in **PDP-11/23 mode** (11/40 has no hardware ODT).
