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

This program outputs `H`, `I` and then halts. It uses the **HALT** instruction
(`000000`) and direct output to the KL11 console transmitter (`177564`/`177566`).

### The program (starting at address `001000`)

| Address | Octal word | Instruction | Meaning |
|---------|-----------|-------------|---------|
| 001000 | 012700 | MOV #imm, R0 | Load immediate into R0 |
| 001002 | 177566 | (operand) | KL11 transmit buffer address |
| 001004 | 112037 | MOVB #imm, @#addr | Move immediate byte to absolute address |
| 001006 | 000110 | (byte value) | ASCII 'H' |
| 001010 | 177566 | (address) | KL11 XBUF |
| 001012 | 112037 | MOVB #imm, @#addr | |
| 001014 | 000111 | (byte value) | ASCII 'I' |
| 001016 | 177566 | (address) | KL11 XBUF |
| 001020 | 000000 | HALT | |

### Depositing the program via ODT

Boot the Cardputer with any disk, then press **G0** to enter ODT.
Type exactly as shown (`.` = LF key, `CR` = Enter):

```
@1000/  000000  012700
1002/  000000  177566
1004/  000000  112037
1006/  000000  000110
1010/  000000  177566
1012/  000000  112037
1014/  000000  000111
1016/  000000  177566
1020/  000000  000000 CR
@1000G
HI
[HALTED: ODT] Esc:Resume G0:Menu
@
```

> **Note:** The KL11 status register at `177564` should have bit 7 set (transmitter
> ready) before writing to `177566`. On a freshly halted system it is normally ready.
> If nothing appears, check `177564/` — it should show `177600` or similar (bit 7 set).

---

## Example 2 — Echo Loop

This program reads a character from the KL11 receiver (`177560`/`177562`) and
writes it back to the transmitter (`177564`/`177566`) in an infinite loop.
Press `G0` to halt it when done.

### The program

| Address | Octal | Instruction | Meaning |
|---------|-------|-------------|---------|
| 001000 | 105737 | TSTB @#177560 | Test RX ready bit (bit 7) |
| 001002 | 177560 | (address) | KL11 RCSR |
| 001004 | 100374 | BPL 001000 | Branch back if not ready (bit 7 = 0) |
| 001006 | 113700 | MOVB @#177562, R0 | Read received char into R0 |
| 001010 | 177562 | (address) | KL11 RBUF |
| 001012 | 105737 | TSTB @#177564 | Test TX ready bit |
| 001014 | 177564 | (address) | KL11 XCSR |
| 001016 | 100374 | BPL 001012 | Branch back if not ready |
| 001020 | 110037 | MOVB R0, @#177566 | Transmit the character |
| 001022 | 177566 | (address) | KL11 XBUF |
| 001024 | 000737 | BR 001000 | Loop forever |

### Depositing via ODT

```
@1000/ 000000 105737
1002/ 000000 177560
1004/ 000000 100374
1006/ 000000 113700
1010/ 000000 177562
1012/ 000000 105737
1014/ 000000 177564
1016/ 000000 100374
1020/ 000000 110037
1022/ 000000 177566
1024/ 000000 000737 CR
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
