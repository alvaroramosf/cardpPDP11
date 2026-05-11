#include <cstdlib>
#include <stdint.h>
#include <stdio.h>
#include "avr11.h"
#include "kb11.h"
#include "unibus.h"
#include "M5Cardputer.h"

extern KB11 cpu;
#define UBM_LNT_LW      32                              /* size in LW */
#define UBM_V_PN        13                              /* page number */
#define UBM_M_PN        037
#define UBM_V_OFF       0                               /* offset */
#define UBM_M_OFF       017777
#define UBM_PAGSIZE     (UBM_M_OFF + 1)                 /* page size */
#define UBM_GETPN(x)    (((x) >> UBM_V_PN) & UBM_M_PN)
#define UBM_GETOFF(x)   ((x) & UBM_M_OFF)


void UNIBUS::write16(uint32_t a, const uint16_t v) {

	if (a & 1) {
		//printf("unibus: write16 to odd address %06o\n", a);
		//cpu.printstate();
		trap(INTBUS);
	}
	if (!(cpu.mmu.SR[3] & 020) && a < MEMSIZE) {
		int page = a >> 15;
		if (!core_pages[page]) { trap(INTBUS); return; }
		core_pages[page][(a & 0x7FFF) >> 1] = v;
		return;
	}

	if ((cpu.mmu.SR[3] & 020) && a < MEMSIZE22) {
		int page = a >> 15;
		if (!core_pages[page]) { trap(INTBUS); return; }
		core_pages[page][(a & 0x7FFF) >> 1] = v;
		return;
	}

	if (a < IOBASE_18BIT && !(cpu.mmu.SR[3] & 020)) {
		trap(INTBUS);
		return;
	}

	if (a < IOBASE_22BIT && (cpu.mmu.SR[3] & 020)) {
		trap(INTBUS);
		return;
	}

	a &= 0777777;

	switch (a & ~077) {
	case 0777400:
		rk11.write16(a, v);
		return;
	case 0774400:
		rl11.write16(a, v);
		return;
	case 0775600:
		switch (a & ~7) {
		case 0775610:
			dl11.write16(a, v);
			return;
		}
		trap(INTBUS);
	case 0776500:
		switch (a & ~7) {
		case 0776500:
			dl11.write16(a, v);
			return;
		}
		trap(INTBUS);
	case 0777500:
		switch (a) {
		case 0777514:
		case 0777516:
			lp11.write16(a, v);
			return;
		case 0777546:
			kw11.write16(a, v);
			return;
		case 0777572:
			cpu.mmu.SR[0] = v;
			return;
		case 0777574:
			cpu.mmu.SR[1] = v;
			return;
		case 0777576:
			// do nothing, SR2 is read only
			return;
		default:
			cons.write16(a, v);
			return;
		}
	case 0772500:
		if (a == 0772516) {
			cpu.mmu.SR[3] = v & 070;
			return;
		}
	case 0770200:
	case 0770300:
		int32 sc, pg;
		pg = (a >> 2) & 037;
		sc = (a & 2) << 3;
		umap[pg] = (umap[pg] & ~(0177777 << sc)) |
			((v & 0177777) << sc);
		umap[pg] = umap[pg] & 017777776;
		return;
	case 0772200:
	case 0772300:
	case 0777600:
		cpu.mmu.write16(a, v);
		return;
	default:
		// Serial.printf("unibus: write to invalid address %06o\r\n", a);
		trap(INTBUS);
	}
	return;
}

uint16_t UNIBUS::read16(uint32_t a) {

	if (a & 1) {
		//printf("unibus: read16 from odd address %06o\n", a);
		//cpu.printstate();
		trap(INTBUS);
		return 0;
	}

	if (!(cpu.mmu.SR[3] & 020) && a < MEMSIZE) {
		int page = a >> 15;
		if (!core_pages[page]) { trap(INTBUS); return 0; }
		return core_pages[page][(a & 0x7FFF) >> 1];
	}

	if ((cpu.mmu.SR[3] & 020) && a < MEMSIZE22) {
		int page = a >> 15;
		if (!core_pages[page]) { trap(INTBUS); return 0; }
		return core_pages[page][(a & 0x7FFF) >> 1];
	}

	if (a < IOBASE_18BIT && !(cpu.mmu.SR[3] & 020)) {
		trap(INTBUS);
		return 0;
	}

	if (a < IOBASE_22BIT && (cpu.mmu.SR[3] & 020)) {
		trap(INTBUS);
		return 0;
	}

	a &= 0777777;

	switch (a & ~077) {
	case 0777400:
		return rk11.read16(a);
	case 0774400:
		return rl11.read16(a);
	case 0775600:
		switch (a & ~7) {
		case 0775610:
			return dl11.read16(a);
		}
		trap(INTBUS);
	case 0776500:
		switch (a & ~7) {
		case 0776500:
			return dl11.read16(a);
		}
		trap(INTBUS);
	case 0777500:
		switch (a) {
		case 0777514:
		case 0777516:
			return lp11.read16(a);
		case 0777546:
			return kw11.read16(a);
		case 0777572:
			return cpu.mmu.SR[0];
		case 0777574:
			return cpu.mmu.SR[1];
		case 0777576:
			return cpu.mmu.SR[2];
		default:
			return cons.read16(a);
		}
	case 0772200:
	case 0772300:
	case 0777600:
		return cpu.mmu.read16(a);
	case 0770200:
	case 0770300:
		int32 pg;
		pg = (a >> 2) & 037;
		return (a & 2) ? ((umap[pg] >> 16) & 077) : (umap[pg] & 0177776);
	case 0772500:
		if (a == 0772516)
			return cpu.mmu.SR[3];
	default:
		// Serial.printf("unibus: read from invalid address %06o\r\n", a);
		trap(INTBUS);
	}
	return 0;
}

uint32_t UNIBUS::remap(uint32_t addr)   // KT24
{
	int32 pg = UBM_GETPN(addr);                              /* map entry */
	int32 off = UBM_GETOFF(addr);                            /* offset */
	int32 uba_last;

	if (!(cpu.mmu.SR[3] & 040))
		return addr;            // No remap

	if (pg != UBM_M_PN)                                     /* last page? */
		uba_last = (umap[pg] + off) & MEMMAX - 1;             /* no, use map */
	else
		uba_last = (IOBASE_22BIT + off) & MEMMAX - 1;            /* yes, use fixed */
	if (addr != uba_last)
		addr = addr;
	return uba_last;
}

void UNIBUS::reset() {
	cons.clearterminal();
	dl11.clearterminal();
	rk11.reset();
	rl11.reset();
	kw11.write16(0777546, 0x00); // disable line clock INTR
	lp11.reset();
	cpu.mmu.SR[0] = 0;
	cpu.mmu.SR[3] = 0;
	memset(umap, 0, sizeof(umap));
	if (core_pages[0])
		return;

    int num_pages22 = MEMSIZE22 / 32768;
    if (MEMSIZE22 % 32768 != 0) num_pages22++;
    bool psram_ok = true;

    // Try to allocate full 22-bit memory in PSRAM first
    for (int i = 0; i < num_pages22; i++) {
        core_pages[i] = (uint16_t*)heap_caps_malloc(32768, MALLOC_CAP_SPIRAM);
        if (!core_pages[i]) {
            psram_ok = false;
            break;
        }
    }

	if (!psram_ok) {
        // Free whatever PSRAM we managed to allocate
        for (int i = 0; i < num_pages22; i++) {
            if (core_pages[i]) heap_caps_free(core_pages[i]);
            core_pages[i] = nullptr;
        }

        Serial.printf("No PSRAM available, falling back to 18-bit (internal SRAM)...\r\n");
        int num_pages18 = MEMSIZE / 32768;
        if (MEMSIZE % 32768 != 0) num_pages18++;

        // Fallback: allocate 18-bit memory in 32KB internal SRAM chunks
        for (int i = 0; i < num_pages18; i++) {
	        core_pages[i] = (uint16_t*)heap_caps_malloc(32768, MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL);
            if (!core_pages[i]) {
		        Serial.printf("RAM alloc failed at 32KB page %d\r\n", i);
                M5Cardputer.Display.fillScreen(BLACK);
                M5Cardputer.Display.setCursor(0, 0);
                M5Cardputer.Display.setTextColor(1, 0);
                M5Cardputer.Display.println("RAM allocation failed!");
		        while (1) delay(100);
            }
        }
	} else {
        Serial.printf("Allocated 22-bit memory in PSRAM.\r\n");
    }
}
