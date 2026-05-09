# PDP-11 ODT — Command Reference & Examples

**ODT** (Octal Debugging Technique) is the interactive front-panel monitor
built into the PDP-11 architecture. It lets you inspect and modify registers
and memory while the CPU is halted — without an operating system.

On the Cardputer, ODT is accessed by pressing the **G0 button** during
emulation. The CPU halts and the prompt `@` appears on the terminal.

---

## Entering and Leaving ODT

| Action | How |
|---|---|
| Enter ODT | Press **G0** button during emulation |
| Resume emulation | Press **Esc**, **P**, or type an address and **G** |
| Open Options Menu | Press **G0** again while in ODT |

---

## Command Reference

All numbers are in **octal** (base 8).  
Line Feed (`LF`) is typed with the **`.`** (period) key on the Cardputer.

### Open a memory location

```
@address/
```

The monitor prints the current 16-bit word value at that address.

**Example:**
```
@1000/  177570
```
Address `001000` contains the value `177570`.

---

### Open a CPU register

```
@Rn/
```
where `n` is `0` through `7` (R7 = Program Counter).

**Example:**
```
@R0/  000000
@R7/  001000
```

---

### Open the Processor Status Word

```
@RS/
```

**Example:**
```
@RS/  000340
```
(Priority level 7, all condition codes clear.)

---

### Write a value to the open location

After opening a location, type the new octal value followed by **CR** (`Enter`):

```
@R0/  000000  000017
```
(Sets R0 to octal 17 = decimal 15.)

If you press **CR** without typing a value, the location is closed unchanged:

```
@R0/  000017 CR
@
```

---

### Advance to the next location (LF / `.`)

After opening a location, type a value (or nothing) and press **`.`** (LF)
to write and move to the next sequential word:

```
@1000/ 000001
1002/ 000002
1004/ 000003
1006/ 000000 CR
@
```

This is the most efficient way to deposit a sequence of words into memory.

---

### Start execution (Go)

```
@G
```
Resumes the CPU from the current Program Counter (R7).

```
@nnnnnnG
```
Sets PC to *nnnnnn* and starts execution there.

**Example:** Start at address `001000`:
```
@1000G
```

---

### Proceed (continue after a halt)

```
@P
```
Resumes execution at the current PC without changing it. Equivalent to `G`
when no address has been typed.

---

## Summary Table

| Input | Effect |
|---|---|
| `nnnnnn/` | Open memory word at octal address *nnnnnn* |
| `Rn/` | Open register Rn (R0–R7) |
| `RS/` | Open Processor Status Word |
| *value* `CR` | Write *value* to open location, close, show `@` |
| *value* `.` | Write *value* to open location, advance to next |
| `CR` (blank) | Close location without writing |
| `G` | Resume from current PC |
| *addr* `G` | Set PC = *addr*, resume |
| `P` | Proceed (same as `G` with no address) |
| `Esc` | Resume without modifying anything |
| `G0` | Open Options Menu (CPU stays halted) |

---

## Example 1 — Hello World in Machine Code

This program polls the KL11 transmitter ready bit, then writes `H` and `I`
directly to the console output register, and halts.

### Instruction encoding note

`MOVB #byte, @#addr` encodes as **`112737`** (not `112037`):
- `11` = MOVB opcode
- `27` = source: mode 2 (autoincrement), **R7** (PC) = **immediate** next word
- `37` = dest:   mode 3 (deferred),      **R7** (PC) = **absolute** next word

`112037` would mean `MOVB (R0)+, @#addr` — reading *from* R0, not from a
literal byte. That is the bug in the earlier, incorrect listing.

### The program (starting at address `001000`)

| Address | Octal   | Instruction          | Meaning                              |
|---------|---------|----------------------|--------------------------------------|
| 001000  | 105737  | TSTB @#177564        | Test KL11 XCSR bit 7 (TX ready)      |
| 001002  | 177564  | (addr operand)       | KL11 XCSR address                    |
| 001004  | 100375  | BPL 001000           | Loop back if bit 7 = 0 (not ready)   |
| 001006  | 112737  | MOVB #110, @#177566  | Write 'H' (octal 110) to XBUF        |
| 001010  | 000110  | (immediate byte)     | ASCII 'H' = octal 110                |
| 001012  | 177566  | (addr operand)       | KL11 XBUF address                    |
| 001014  | 105737  | TSTB @#177564        | Test XCSR again                      |
| 001016  | 177564  | (addr operand)       | KL11 XCSR                            |
| 001020  | 100375  | BPL 001014           | Loop back if not ready               |
| 001022  | 112737  | MOVB #111, @#177566  | Write 'I' (octal 111) to XBUF        |
| 001024  | 000111  | (immediate byte)     | ASCII 'I' = octal 111                |
| 001026  | 177566  | (addr operand)       | KL11 XBUF                            |
| 001030  | 000000  | HALT                 | Stop                                 |

> **BPL offset:** at 001004, next PC = 001006, target = 001000.
> Displacement = (001000 − 001006) / 2 = −3 words = 0375 (8-bit two's complement).
> `BPL 0375` = `100375`. Same offset applies at 001020 → 001014.

### Depositing via ODT

Boot the Cardputer (any disk), press **G0** to enter ODT.
Use **`.`** (period) as the LF key to advance to the next word:

```
@1000/ 000000 105737
1002/ 000000 177564
1004/ 000000 100375
1006/ 000000 112737
1010/ 000000 000110
1012/ 000000 177566
1014/ 000000 105737
1016/ 000000 177564
1020/ 000000 100375
1022/ 000000 112737
1024/ 000000 000111
1026/ 000000 177566
1030/ 000000 000000 CR
@1000G
HI
[HALTED: ODT] Esc:Resume G0:Menu
@
```

---

## Example 2 — Echo Loop

This program reads characters from the KL11 receiver and echoes them back.
Press **G0** to halt.

### The program

| Address | Octal  | Instruction          | Meaning                              |
|---------|--------|----------------------|--------------------------------------|
| 001000  | 105737 | TSTB @#177560        | Test KL11 RCSR bit 7 (RX done)       |
| 001002  | 177560 | (addr operand)       | KL11 RCSR                            |
| 001004  | 100375 | BPL 001000           | Loop back if no char received        |
| 001006  | 113700 | MOVB @#177562, R0    | Read received byte into R0           |
| 001010  | 177562 | (addr operand)       | KL11 RBUF                            |
| 001012  | 105737 | TSTB @#177564        | Test XCSR bit 7 (TX ready)           |
| 001014  | 177564 | (addr operand)       | KL11 XCSR                            |
| 001016  | 100375 | BPL 001012           | Loop back if transmitter busy        |
| 001020  | 110037 | MOVB R0, @#177566    | Write byte in R0 to XBUF             |
| 001022  | 177566 | (addr operand)       | KL11 XBUF                            |
| 001024  | 000765 | BR 001000            | Branch always back to top            |

> **BR offset:** at 001024, next PC = 001026, target = 001000.
> Displacement = (001000 − 001026) / 2 = −11 words = 0365 (8-bit two's complement).
> `BR 0365` = `000400 | 0365` = `000765`.

### Depositing via ODT

```
@1000/ 000000 105737
1002/ 000000 177560
1004/ 000000 100375
1006/ 000000 113700
1010/ 000000 177562
1012/ 000000 105737
1014/ 000000 177564
1016/ 000000 100375
1020/ 000000 110037
1022/ 000000 177566
1024/ 000000 000765 CR
@1000G
```

Now anything you type on the keyboard is echoed back immediately.
Press **G0** to halt and return to ODT.

---


## KL11 Register Map (Console Terminal)

| Address (octal) | Name | Bit 7 | Purpose |
|---|---|---|---|
| `177560` | RCSR | RX Done | Set when a character has been received |
| `177562` | RBUF | — | Received character (read to clear RCSR bit 7) |
| `177564` | XCSR | TX Ready | Set when transmitter is ready for next char |
| `177566` | XBUF | — | Write character here to transmit |

---

## Tips

- Numbers are always **octal**. `10` means 8 decimal, `100` means 64 decimal.
- R7 is the **Program Counter**. Inspect it with `R7/` to see where execution halted.
- R6 is the **Stack Pointer**. The current stack frame can help diagnose crashes.
- The PSW (Processor Status Word) encodes: priority (bits 7–5), T (bit 4),
  N (bit 3), Z (bit 2), V (bit 1), C (bit 0).
- If you accidentally modify memory you did not intend to, press the **side button**
  to hard-reset the Cardputer and start fresh.
