 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Custom chip emulation
  *
  * (c) 1995 Bernd Schmidt, Alessandro Bissacco
  * (c) 2002 - 2021 Toni Wilen
  */

#include "sysdeps.h"

#include "options.h"
#include "memory.h"
#include "custom.h"
#include "newcpu.h"
#include "savestate.h"
#include "blitter.h"
#include "blit.h"

/* we must not change ce-mode while blitter is running.. */
static int blitter_cycle_exact, immediate_blits;
static int blt_statefile_type;

uae_u16 bltcon0, bltcon1;
uae_u32 bltapt, bltbpt, bltcpt, bltdpt;
uae_u32 bltptx;
int bltptxpos, bltptxc;

static int blinea_shift;
static uae_u16 blinea, blineb;
static int blitline, blitfc, blitfill, blitife, blitsing, blitdesc;
static int blitonedot, blitsign, blitlinepixel;
static int blit_add;
static int blit_modadda, blit_modaddb, blit_modaddc, blit_modaddd;
static bool shifter_skip_b, shifter_skip_y;
static bool shifter_skip_b_old, shifter_skip_y_old;
static uae_u16 bltcon0_old, bltcon1_old;
static bool shifter[4], shifter_out, shifter_first;

struct bltinfo blt_info;

static uae_u8 blit_filltable[256][4][2];
uae_u32 blit_masktable[BLITTER_MAX_WORDS];

static int blit_cyclecounter, blit_waitcyclecounter;
static int blit_slowdown;

#ifdef CPUEMU_13
static int blitter_hcounter;
static int blitter_vcounter;
#endif

static long blit_firstline_cycles;
static long blit_first_cycle;
static int blit_last_cycle, blit_dmacount, blit_cyclecount;
static int blit_faulty;
static int blt_delayed_irq;
static uae_u16 ddat1;
static int ddat1use;

static int last_blitter_hpos;

#define BLITTER_STARTUP_CYCLES 2

/*
Blitter Idle Cycle:

Cycles that are free cycles (available for CPU) and
are not used by any other Agnus DMA channel. Blitter
idle cycle is not "used" by blitter, CPU can still use
it normally if it needs the bus.

same in both block and line modes

number of cycles, initial cycle, main cycle
*/

/*

following 4 channel combinations in fill mode have extra
idle cycle added (still requires free bus cycle)

Condition: If D without C: Add extra cycle.

*/

/*
-C-D C-D- ... C-D- --

line draw takes 4 cycles (-C-D)
idle cycles do the same as above, 2 dma fetches
(read from C, write to D, but see below)

Oddities:

- first word is written to address pointed by BLTDPT
but all following writes go to address pointed by BLTCPT!
(some kind of internal copy because all bus cyles are
using normal BLTDDAT)
- BLTDMOD is ignored by blitter (BLTCMOD is used)
- state of D-channel enable bit does not matter!
- disabling A-channel freezes the content of BPLAPT
- C-channel disabled: nothing is written

There is one tricky situation, writing to DFF058 just before
last D write cycle (which is normally free) does not disturb
blitter operation, final D is still written correctly before
blitter starts normally (after 2 idle cycles)

There is at least one demo that does this..

*/

/* Copper pointer to Blitter register copy bug

	1: -d = D (-D)
	2: -c = C (-C)
	3: - (-CD)
	4: - (-B-)
	5: - (-BD)
	6: - (-BC)
	7: -BcD = C, -BCd = D
	8: - (A-)
	9: - (AD)
	A: - (AC)
	B: A (ACD)
	C: - (AB-)
	D: - (ABD-)
	E: - (ABC)
	F: AxBxCxD = -, aBxCxD = A, 

	1FE,8C,RGA,8C

 */

void build_blitfilltable (void)
{
	unsigned int d, fillmask;
	int i;

	for (i = 0; i < BLITTER_MAX_WORDS; i++)
		blit_masktable[i] = 0xFFFF;

	for (d = 0; d < 256; d++) {
		for (i = 0; i < 4; i++) {
			int fc = i & 1;
			uae_u8 data = d;
			for (fillmask = 1; fillmask != 0x100; fillmask <<= 1) {
				uae_u16 tmp = data;
				if (fc) {
					if (i & 2)
						data |= fillmask;
					else
						data ^= fillmask;
				}
				if (tmp & fillmask) fc = !fc;
			}
			blit_filltable[d][i][0] = data;
			blit_filltable[d][i][1] = fc;
		}
	}
}

STATIC_INLINE int canblit (int hpos)
{
	if (!dmaen (DMA_BLITTER))
		return -1;
	if (is_bitplane_dma (hpos))
		return 0;
	if (cycle_line_slot[hpos] & CYCLE_MASK) {
		return 0;
	}
	return 1;
}

static void reset_channel_mods (void)
{
	if (bltptxpos < 0)
		return;
	bltptxpos = -1;
	switch (bltptxc)
	{
		case 1:
		  bltapt = bltptx;
		  break;
		case 2:
		  bltbpt = bltptx;
		  break;
		case 3:
		  bltcpt = bltptx;
		  break;
		case 4:
		  bltdpt = bltptx;
		  break;
	}
}

static void check_channel_mods (int hpos, int ch)
{
	if (bltptxpos != hpos)
		return;
	if (ch == bltptxc) {
		bltptxpos = -1;
	}
}

// blitter interrupt is set (and busy bit cleared) when
// last "main" cycle has been finished, any non-linedraw
// D-channel blit still needs 2 more cycles before final
// D is written (idle cycle, final D write)
//
// According to schematics, AGA has workaround delay circuit
// that adds 2 extra cycles if D is enabled and not line mode.
//
// line draw interrupt triggers when last D is written
// (or cycle where last D write would have been if
// ONEDOT was active)

static bool blitter_interrupt (int done)
{
  blt_info.blit_main = 0;
	if (blt_info.blit_interrupt) {
		return false;
	}
	if (!done && (!blitter_cycle_exact || immediate_blits || currprefs.cpu_model >= 68030 || currprefs.cachesize || currprefs.m68k_speed < 0)) {
		return false;
	}
	blt_info.blit_interrupt = 1;
	send_interrupt (6, (4 + 1) * CYCLE_UNIT);
	blitter_done_notify(blitline);
	return true;
}

static void blitter_done (void)
{
  ddat1use = 0;
  blt_info.blit_finald = 0;
	if (!blitter_interrupt (1)) {
		blitter_done_notify(blitline);
	}
	event2_remevent (ev2_blitter);
	unset_special (SPCFLAG_BLTNASTY);
	blt_info.blitter_dangerous_bpl = 0;
}

STATIC_INLINE void blitter_maybe_done_early(void)
{
	if (currprefs.chipset_mask & CSMASK_AGA) {
		if (!(bltcon0 & 0x100) || blitline) {
			// immediately done if D disabled or line mode.
			blitter_done();
			return;
		}
	}
	// busy cleared, interrupt generated.
	// last D write still pending if not linemode and D channel active
	if (blitline) {
		blitter_done();
	} else {
		if (ddat1use && (bltcon0 & 0x100)) {
			blt_info.blit_finald = 1 + 2;
			blitter_interrupt(0);
		} else {
			blitter_done();
		}
	}
}

STATIC_INLINE void chipmem_agnus_wput2 (uaecptr addr, uae_u32 w)
{
	//last_custom_value1 = w; blitter writes are not stored
	chipmem_wput_indirect (addr, w);
}

static void blitter_dofast(void)
{
  int i,j;
  uae_u8 *bltadatptr = 0, *bltbdatptr = 0, *bltcdatptr = 0;
  uaecptr bltddatptr = 0;
  uae_u8 mt = bltcon0 & 0xFF;

	blit_masktable[0] = blt_info.bltafwm;
	blit_masktable[blt_info.hblitsize - 1] &= blt_info.bltalwm;

  if (bltcon0 & 0x800) {
	  bltadatptr = &chipmem_bank.baseaddr[bltapt & chipmem_full_mask];
	  bltapt += (blt_info.hblitsize * 2 + blt_info.bltamod) * blt_info.vblitsize;
  }
  if (bltcon0 & 0x400) {
	  bltbdatptr = &chipmem_bank.baseaddr[bltbpt & chipmem_full_mask];
	  bltbpt += (blt_info.hblitsize * 2 + blt_info.bltbmod) * blt_info.vblitsize;
  }
  if (bltcon0 & 0x200) {
	  bltcdatptr = &chipmem_bank.baseaddr[bltcpt & chipmem_full_mask];
	  bltcpt += (blt_info.hblitsize * 2 + blt_info.bltcmod) * blt_info.vblitsize;
  }
  if (bltcon0 & 0x100) {
    bltddatptr = bltdpt;
    bltdpt += (blt_info.hblitsize * 2 + blt_info.bltdmod) * blt_info.vblitsize;
  }

  if (blitfunc_dofast[mt] && !blitfill) {
  	(*blitfunc_dofast[mt])(bltadatptr, bltbdatptr, bltcdatptr, bltddatptr, &blt_info);
	} else {
	  uae_u32 blitbhold = blt_info.bltbhold;
	  uaecptr dstp = 0;

		for (j = 0; j < blt_info.vblitsize; j++) {
	    blitfc = !!(bltcon1 & 0x4);
			for (i = 0; i < blt_info.hblitsize; i++) {
		    uae_u32 bltadat, blitahold;
		    if (bltadatptr) {
		      blt_info.bltadat = bltadat = do_get_mem_word ((uae_u16 *)bltadatptr);
		      bltadatptr += 2;
    		} else
  		    bltadat = blt_info.bltadat;
		    bltadat &= blit_masktable[i];
				blitahold = (((uae_u32)blt_info.bltaold << 16) | bltadat) >> blt_info.blitashift;
				blt_info.bltaold = bltadat;

    		if (bltbdatptr) {
		      uae_u16 bltbdat = do_get_mem_word ((uae_u16 *)bltbdatptr);;
		      bltbdatptr += 2;
					blitbhold = (((uae_u32)blt_info.bltbold << 16) | bltbdat) >> blt_info.blitbshift;
					blt_info.bltbold = bltbdat;
					blt_info.bltbdat = bltbdat;
		    }

    		if (bltcdatptr) {
		      blt_info.bltcdat = do_get_mem_word ((uae_u16 *)bltcdatptr);
		      bltcdatptr += 2;
    		}
    		if (dstp) 
    		  chipmem_agnus_wput2 (dstp, blt_info.bltddat);
    		blt_info.bltddat = blit_func (blitahold, blitbhold, blt_info.bltcdat, mt);
    		if (blitfill) {
		      uae_u16 d = blt_info.bltddat;
		      int ifemode = blitife ? 2 : 0;
		      int fc1 = blit_filltable[d & 255][ifemode + blitfc][1];
		      blt_info.bltddat = (blit_filltable[d & 255][ifemode + blitfc][0]
					  + (blit_filltable[d >> 8][ifemode + fc1][0] << 8));
		      blitfc = blit_filltable[d >> 8][ifemode + fc1][1];
    		}
    		if (blt_info.bltddat)
  		    blt_info.blitzero = 0;
    		if (bltddatptr) {
  		    dstp = bltddatptr;
  		    bltddatptr += 2;
    		}
	    }
	    if (bltadatptr) 
        bltadatptr += blt_info.bltamod;
      if (bltbdatptr) 
        bltbdatptr += blt_info.bltbmod;
	    if (bltcdatptr) 
        bltcdatptr += blt_info.bltcmod;
      if (bltddatptr) 
        bltddatptr += blt_info.bltdmod;
	  }
	  if (dstp)
	    chipmem_agnus_wput2 (dstp, blt_info.bltddat);
	  blt_info.bltbhold = blitbhold;
  }
	blit_masktable[0] = 0xFFFF;
	blit_masktable[blt_info.hblitsize - 1] = 0xFFFF;
  blt_info.blit_main = 0;
}

static void blitter_dofast_desc(void)
{
  int i,j;
  uae_u8 *bltadatptr = 0, *bltbdatptr = 0, *bltcdatptr = 0;
  uaecptr bltddatptr = 0;
  uae_u8 mt = bltcon0 & 0xFF;

	blit_masktable[0] = blt_info.bltafwm;
	blit_masktable[blt_info.hblitsize - 1] &= blt_info.bltalwm;

  if (bltcon0 & 0x800) {
	  bltadatptr = &chipmem_bank.baseaddr[bltapt & chipmem_full_mask];
	  bltapt -= (blt_info.hblitsize * 2 + blt_info.bltamod) * blt_info.vblitsize;
  }
  if (bltcon0 & 0x400) {
	  bltbdatptr = &chipmem_bank.baseaddr[bltbpt & chipmem_full_mask];
	  bltbpt -= (blt_info.hblitsize * 2 + blt_info.bltbmod) * blt_info.vblitsize;
  }
  if (bltcon0 & 0x200) {
	  bltcdatptr = &chipmem_bank.baseaddr[bltcpt & chipmem_full_mask];
	  bltcpt -= (blt_info.hblitsize * 2 + blt_info.bltcmod) * blt_info.vblitsize;
  }
  if (bltcon0 & 0x100) {
    bltddatptr = bltdpt;
    bltdpt -= (blt_info.hblitsize * 2 + blt_info.bltdmod) * blt_info.vblitsize;
  }
  if (blitfunc_dofast_desc[mt] && !blitfill) {
		(*blitfunc_dofast_desc[mt])(bltadatptr, bltbdatptr, bltcdatptr, bltddatptr, &blt_info);
	} else {
	  uae_u32 blitbhold = blt_info.bltbhold;
	  uaecptr dstp = 0;

		for (j = 0; j < blt_info.vblitsize; j++) {
			blitfc = !!(bltcon1 & 0x4);
			for (i = 0; i < blt_info.hblitsize; i++) {
				uae_u32 bltadat, blitahold;
				if (bltadatptr) {
		      bltadat = blt_info.bltadat = do_get_mem_word ((uae_u16 *)bltadatptr);
					bltadatptr -= 2;
				} else
					bltadat = blt_info.bltadat;
				bltadat &= blit_masktable[i];
				blitahold = (((uae_u32)bltadat << 16) | blt_info.bltaold) >> blt_info.blitdownashift;
				blt_info.bltaold = bltadat;

				if (bltbdatptr) {
		      uae_u16 bltbdat = do_get_mem_word ((uae_u16 *)bltbdatptr);
					bltbdatptr -= 2;
					blitbhold = (((uae_u32)bltbdat << 16) | blt_info.bltbold) >> blt_info.blitdownbshift;
					blt_info.bltbold = bltbdat;
					blt_info.bltbdat = bltbdat;
				}

			  if (bltcdatptr) {
  		    blt_info.bltcdat = blt_info.bltbdat = do_get_mem_word ((uae_u16 *)bltcdatptr);
					bltcdatptr -= 2;
				}
				if (dstp)
    		  chipmem_agnus_wput2 (dstp, blt_info.bltddat);
    		blt_info.bltddat = blit_func (blitahold, blitbhold, blt_info.bltcdat, mt);
    		if (blitfill) {
  		    uae_u16 d = blt_info.bltddat;
	        int ifemode = blitife ? 2 : 0;
	        int fc1 = blit_filltable[d & 255][ifemode + blitfc][1];
	        blt_info.bltddat = (blit_filltable[d & 255][ifemode + blitfc][0]
				    + (blit_filltable[d >> 8][ifemode + fc1][0] << 8));
	        blitfc = blit_filltable[d >> 8][ifemode + fc1][1];
    		}
    		if (blt_info.bltddat)
  		    blt_info.blitzero = 0;
    		if (bltddatptr) {
  		    dstp = bltddatptr;
  		    bltddatptr -= 2;
    		}
	    }
	    if (bltadatptr) 
        bltadatptr -= blt_info.bltamod;
	    if (bltbdatptr) 
        bltbdatptr -= blt_info.bltbmod;
      if (bltcdatptr) 
        bltcdatptr -= blt_info.bltcmod;
	    if (bltddatptr) 
        bltddatptr -= blt_info.bltdmod;
	  }
	  if (dstp)
	    chipmem_agnus_wput2 (dstp, blt_info.bltddat);
		blt_info.bltbhold = blitbhold;
	}
	blit_masktable[0] = 0xFFFF;
	blit_masktable[blt_info.hblitsize - 1] = 0xFFFF;
  blt_info.blit_main = 0;
}

STATIC_INLINE void blitter_read(void)
{
	if (bltcon0 & 0x200) {
    blt_info.bltcdat = chipmem_wget_indirect(bltcpt);
		last_custom_value1 = blt_info.bltcdat;
	}
}

STATIC_INLINE void blitter_write(void)
{
	if (blt_info.bltddat)
		blt_info.blitzero = 0;
	/* D-channel state has no effect on linedraw, but C must be enabled or nothing is drawn! */
	if (bltcon0 & 0x200) {
    chipmem_wput_indirect (bltdpt, blt_info.bltddat);
	}
}

STATIC_INLINE void blitter_line_incx(void)
{
  if (++blinea_shift == 16) {
  	blinea_shift = 0;
  	bltcpt += 2;
  }
}

STATIC_INLINE void blitter_line_decx(void)
{
  if (blinea_shift-- == 0) {
	  blinea_shift = 15;
	  bltcpt -= 2;
  }
}

STATIC_INLINE void blitter_line_decy(void)
{
  bltcpt -= blt_info.bltcmod;
  blitonedot = 0;
}

STATIC_INLINE void blitter_line_incy(void)
{
  bltcpt += blt_info.bltcmod;
  blitonedot = 0;
}

static void blitter_line (void)
{
	uae_u16 blitahold = (blinea & blt_info.bltafwm) >> blinea_shift;
	uae_u16 blitchold = blt_info.bltcdat;

	blt_info.bltbhold = (blineb & 1) ? 0xFFFF : 0;
	blitlinepixel = !blitsing || (blitsing && !blitonedot);
	blt_info.bltddat = blit_func (blitahold, blt_info.bltbhold, blitchold, bltcon0 & 0xFF);
	blitonedot++;
}

STATIC_INLINE void blitter_line_proc (void)
{
	if (!blitsign) {
		if (bltcon1 & 0x10) {
			if (bltcon1 & 0x8)
				blitter_line_decy ();
			else
				blitter_line_incy ();
		} else {
			if (bltcon1 & 0x8)
				blitter_line_decx ();
			else
				blitter_line_incx ();
		}
	}
	if (bltcon1 & 0x10) {
		if (bltcon1 & 0x4)
			blitter_line_decx ();
		else
			blitter_line_incx ();
	} else {
		if (bltcon1 & 0x4)
			blitter_line_decy ();
		else
			blitter_line_incy ();
	}

  if (bltcon0 & 0x800) {
		if (blitsign)
			bltapt += (uae_s16)blt_info.bltbmod;
		else
			bltapt += (uae_s16)blt_info.bltamod;
	}
	blitsign = 0 > (uae_s16)bltapt;
}

STATIC_INLINE void blitter_nxline(void)
{
	blineb = (blineb << 1) | (blineb >> 15);
}

static void actually_do_blit(void)
{
  if (blitline) {
  	do {
			blitter_read ();
			if (ddat1use)
				bltdpt = bltcpt;
			ddat1use = 1;
      blitter_line ();
			blitter_line_proc();
			blitter_nxline ();
			blt_info.vblitsize--;
			if (blitlinepixel) {
				blitter_write ();
				blitlinepixel = 0;
			}
		} while (blt_info.vblitsize != 0);
	  bltdpt = bltcpt;
	} else {
		if (blitdesc)
			blitter_dofast_desc ();
		else
			blitter_dofast ();
	}
	blt_info.blit_main = 0;
}

static void blitter_doit (void)
{
	if (blt_info.vblitsize == 0 || (blitline && blt_info.hblitsize != 2)) {
		blitter_done ();
		return;
	}
	actually_do_blit ();
	blitter_done ();
}

void blitter_handler (uae_u32 data)
{
	static int blitter_stuck;

	if (!dmaen (DMA_BLITTER)) {
		event2_newevent (ev2_blitter, 10, 0);
		blitter_stuck++;
		if (blitter_stuck < 20000 || !immediate_blits)
			return; /* gotta come back later. */
		/* "free" blitter in immediate mode if it has been "stuck" ~3 frames
		* fixes some JIT game incompatibilities
		*/
	}
	blitter_stuck = 0;
	if (blit_slowdown > 0 && !immediate_blits) {
		event2_newevent (ev2_blitter, blit_slowdown, 0);
		blit_slowdown = -1;
		return;
	}
	blitter_doit ();
}

#ifdef CPUEMU_13

static void blit_bltset(int con)
{
	if (con & 2) {
		blitdesc = bltcon1 & 2;
		blt_info.blitbshift = bltcon1 >> 12;
		blt_info.blitdownbshift = 16 - blt_info.blitbshift;
	}

	if (con & 1) {
		blt_info.blitashift = bltcon0 >> 12;
		blt_info.blitdownashift = 16 - blt_info.blitashift;
	}

	if (!savestate_state && blt_info.blit_main && (bltcon0_old != bltcon0 || bltcon1_old != bltcon1)) {
		bltcon0_old = bltcon0;
		bltcon1_old = bltcon1;
	}

	blitline = bltcon1 & 1;

	shifter_skip_b = (bltcon0 & 0x400) == 0;
	if (blitline) {
		shifter_skip_y = true;
		blitfill = 0;
	} else {
	  blitfill = (bltcon1 & 0x18) != 0;
		blitfc = !!(bltcon1 & 0x4);
		blitife = !!(bltcon1 & 0x8);
		if ((bltcon1 & 0x18) == 0x18) {
			blitife = 0;
		}
		shifter_skip_y = (bltcon0 & (0x100 | 0x200)) != 0x300;
		// fill mode idle cycle needed?
		if (blitfill && (bltcon0 & (0x100 | 0x200)) == 0x100) {
			shifter_skip_y = false;
		}
	}
	shifter_out = shifter_skip_y ? shifter[2] : shifter[3];

	blit_cyclecount = 4 - (shifter_skip_b + shifter_skip_y);
	blit_dmacount = ((bltcon0 & 0x800) ? 1 : 0) + ((bltcon0 & 0x400) ? 1 : 0) +
		((bltcon0 & 0x200) ? 1 : 0) + ((bltcon0 & 0x100) ? 1 : 0);
}

static int get_current_channel(void)
{
	if (blit_cyclecounter < 0) {
		return 0;
	}

	if (!blit_faulty && blit_cyclecounter > 0) {
		int cnt = 0;
		for (int i = 0; i < 4; i++) {
			if (shifter[i])
				cnt++;
		}
		if (cnt == 0) {
			blit_faulty = 1;
		}
		if (cnt > 1) {
			blit_faulty = 1;
		}
	}

	if (blitline) {
		if (shifter[0]) {
		  // A or idle
			if (blitter_hcounter + 1 == blt_info.hblitsize)
				return 5;
			if (bltcon0 & 0x800)
				return 1;
			return 0;
		}
		// B
		if (shifter[1] && (bltcon0 & 0x400)) {
			return 2;
		}
		// C or D
		if (shifter[2] && (bltcon0 & 0x200)) {
			if (blitter_hcounter + 1 == blt_info.hblitsize)
				return 4;
			return 3;
		}
	} else {
		// order is important when multiple bits in shift register
		// C
		if (shifter[2] && (bltcon0 & 0x200)) {
			return 3;
		}
		// Shift stage 4 active, C enabled and other stage(s) also active:
		// normally would be D but becomes C.
		if (shifter[3] && (bltcon0 & 0x200) && (shifter[0] || shifter[1])) {
			return 3;
		}
		// A
		if (shifter[0] && (bltcon0 & 0x800)) {
			return 1;
		}
		// B
		if (shifter[1] && (bltcon0 & 0x400)) {
			return 2;
		}
		// D only if A, B and C is not currently active
  	if (ddat1use) {
			// if stage 3 and C disabled and D enabled: D
			if (shifter[2] && !(bltcon0 & 0x200) && (bltcon0 & 0x100)) {
				return 4;
			}
			// if stage 4 and C enabled and D enabled: D
			if (shifter[3] && (bltcon0 & 0x200) && (bltcon0 & 0x100)) {
				return 4;
			}
		}
	}
	return 0;
}

STATIC_INLINE uae_u16 blitter_doblit (void)
{
	uae_u32 blitahold;
	uae_u16 bltadat, ddat;
	uae_u8 mt = bltcon0 & 0xFF;

	bltadat = blt_info.bltadat;
	if (blitter_hcounter == 0)
		bltadat &= blt_info.bltafwm;
	if (blitter_hcounter == blt_info.hblitsize - 1)
		bltadat &= blt_info.bltalwm;
	if (blitdesc)
		blitahold = (((uae_u32)bltadat << 16) | blt_info.bltaold) >> blt_info.blitdownashift;
	else
		blitahold = (((uae_u32)blt_info.bltaold << 16) | bltadat) >> blt_info.blitashift;
	blt_info.bltaold = bltadat;

	ddat = blit_func (blitahold, blt_info.bltbhold, blt_info.bltcdat, mt) & 0xFFFF;

	if (blitfill) {
		uae_u16 d = ddat;
		int ifemode = blitife ? 2 : 0;
		int fc1 = blit_filltable[d & 255][ifemode + blitfc][1];
		ddat = (blit_filltable[d & 255][ifemode + blitfc][0]
			+ (blit_filltable[d >> 8][ifemode + fc1][0] << 8));
		blitfc = blit_filltable[d >> 8][ifemode + fc1][1];
	}

	if (ddat)
		blt_info.blitzero = 0;

	return ddat;
}

static void blitter_next_cycle(void)
{
  bool tmp[4];
	bool out = false;

	memcpy(tmp, shifter, sizeof(shifter));
	memset(shifter, 0, sizeof(shifter));

	if (shifter_skip_b_old && !shifter_skip_b) {
		// if B skip was disabled: A goes both to B and C
		tmp[1] = tmp[0];
		tmp[2] = tmp[0];
		shifter_skip_b_old = shifter_skip_b;
	} else if (!shifter_skip_b_old && shifter_skip_b) {
		// if B skip was enabled: A goes nowhere
		tmp[0] = false;
		shifter_skip_b_old = shifter_skip_b;
	}

	if (shifter_skip_y_old && !shifter_skip_y) {
		// if Y skip was disbled: X goes both to Y and OUT
		tmp[3] = tmp[2];
		shifter_skip_y_old = shifter_skip_y;
	} else if (!shifter_skip_y_old && shifter_skip_y) {
		// if Y skip was enabled: X goes nowhere
		tmp[2] = false;
		shifter_out = false;
		shifter_skip_y_old = shifter_skip_y;
	}

	if (shifter_out) {
		if (!blitline) {
			ddat1 = blitter_doblit();
			if (bltcon0 & 0x100) {
				ddat1use = true;
			}
		}
		blitter_hcounter++;
		if (blitter_hcounter == blt_info.hblitsize) {
			blitter_hcounter = 0;
			blitter_vcounter++;
			blitfc = !!(bltcon1 & 0x4);
			if (blitter_vcounter == blt_info.vblitsize) {
				shifter_out = false;
				blit_cyclecounter = 0;
				blitter_maybe_done_early();
			}
		}
		shifter[0] = shifter_out;
	}

	if (shifter_first) {
		shifter_first = false;
		shifter[0] = true;
		blitfc = !!(bltcon1 & 0x4);
	} else {
	  if (shifter_skip_b) {
			shifter[2] = tmp[0];
		} else {
			shifter[1] = tmp[0];
			shifter[2] = tmp[1];
		}
		if (shifter_skip_y) {
			out = shifter[2];
		} else {
			shifter[3] = tmp[2];
		  out = shifter[3];
		}
	}
  shifter_out = out;
}

STATIC_INLINE void blitter_doddma_new(int hpos)
{
	chipmem_agnus_wput2(bltdpt, ddat1);
	alloc_cycle(hpos, CYCLE_BLITTER);
	
  if (!blitline) {
	  bltdpt += blit_add;
		if (blitter_hcounter == 0) {
		  bltdpt += blit_modaddd;
	  }
	}
}

STATIC_INLINE void blitter_dodma_new(int ch, int hpos)
{
	uae_u16 dat, reg;
	uae_u32 *addr;
	int mod;

	switch (ch)
	{
	case 1:
		reg = 0x74;
		blt_info.bltadat = dat = chipmem_wget_indirect (bltapt);
		last_custom_value1 = blt_info.bltadat;
		addr = &bltapt;
		mod = blit_modadda;
		alloc_cycle (hpos, CYCLE_BLITTER);
		break;
	case 2:
		reg = 0x72;
		blt_info.bltbdat = dat = chipmem_wget_indirect (bltbpt);
		last_custom_value1 = blt_info.bltbdat;
		addr = &bltbpt;
		mod = blit_modaddb;
		if (blitdesc)
			blt_info.bltbhold = (((uae_u32)blt_info.bltbdat << 16) | blt_info.bltbold) >> blt_info.blitdownbshift;
		else
			blt_info.bltbhold = (((uae_u32)blt_info.bltbold << 16) | blt_info.bltbdat) >> blt_info.blitbshift;
		blt_info.bltbold = blt_info.bltbdat;
		alloc_cycle (hpos, CYCLE_BLITTER);
		break;
	case 3:
		reg = 0x70;
		blt_info.bltcdat = dat = chipmem_wget_indirect (bltcpt);
		last_custom_value1 = blt_info.bltcdat;
		addr = &bltcpt;
		mod = blit_modaddc;
		alloc_cycle (hpos, CYCLE_BLITTER);
		break;
	default:
		abort ();
	}

  if (!blitline) {
		(*addr) += blit_add;
		if (blitter_hcounter + 1 == blt_info.hblitsize) {
			(*addr) += mod;
		}
	}
}

static bool blitter_idle_cycle_register_write(uaecptr addr, uae_u16 v)
{
	addrbank *ab = &get_mem_bank(addr);
	if (ab != &custom_bank)
		return false;
	addr &= 0x1fe;
	if (addr == 0x40) {
		bltcon0 = v;
		blit_bltset(1);
		return true;
	} else if (addr == 0x42) {
		bltcon1 = v;
		blit_bltset(2);
		return true;
	}
	return false;
}

static bool decide_blitter_idle(int lasthpos, int hpos, uaecptr addr, uae_u16 value)
{
	if (addr != 0xffffffff && lasthpos + 1 == hpos) {
		shifter_skip_b_old = shifter_skip_b;
		shifter_skip_y_old = shifter_skip_y;
		return blitter_idle_cycle_register_write(addr, value);
  }
	return false;
}

void decide_blitter (int until_hpos)
{
  decide_blitter_maybe_write(until_hpos, 0xffffffff, 0xffff);
}

bool decide_blitter_maybe_write(int until_hpos, uaecptr addr, uae_u16 value)
{
	bool written = false;
	int hsync = until_hpos < 0;
	int hpos = last_blitter_hpos;

	if (hsync && blt_delayed_irq) {
		if (blt_delayed_irq > 0)
			blt_delayed_irq--;
		if (blt_delayed_irq <= 0) {
			blt_delayed_irq = 0;
			send_interrupt(6, 2 * CYCLE_UNIT);
		}
	}

	if (until_hpos < 0) {
		until_hpos = maxhpos;
	}

	if (immediate_blits) {
	  if (!blt_info.blit_main) {
			return false;
	  }
		if (dmaen (DMA_BLITTER)) {
			blitter_doit();
		}
		return false;
	}

	if (!blt_info.blit_main && !blt_info.blit_finald) {
		goto end;
	}

	if (!blitter_cycle_exact) {
		return false;
	}

	while (hpos < until_hpos) {
		int c = get_current_channel();

		for (;;) {
			int v = canblit(hpos);

		  // final D idle cycle
			// does not need free bus
  		if (blt_info.blit_finald > 1) {
				blt_info.blit_finald--;
			}

			// copper bltsize write needs one cycle (any cycle) delay
			// does not need free bus
			if (blit_waitcyclecounter) {
				blit_waitcyclecounter = 0;
				break;
			}

			if (v <= 0) {
				break;
			}

		  if (blt_info.blit_finald == 1) {
				// final D write
				blitter_doddma_new(hpos);
				blitter_done();
				break;
			}
			
		  if (blt_info.blit_main) {
				blit_cyclecounter++;
				if (blit_cyclecounter == 0) {
					shifter_first = true;
				}

				blt_info.got_cycle = 1;

				if (c == 0) {

					written = decide_blitter_idle(hpos, until_hpos, addr, value);

				} else if (c == 1 && blitline) { // line 1/4 (A, free)

					written = decide_blitter_idle(hpos, until_hpos, addr, value);

				} else if (c == 3 && blitline) { // line 2/4 (C)

					blt_info.bltcdat = chipmem_wget_indirect(bltcpt);
					last_custom_value1 = blt_info.bltcdat;
					alloc_cycle(hpos, CYCLE_BLITTER);

				} else if (c == 5 && blitline) { // line 3/4 (free)

					blitter_line();

					written = decide_blitter_idle(hpos, until_hpos, addr, value);

				} else if (c == 4 && blitline) { // line 4/4 (D)

					if (ddat1use)
						bltdpt = bltcpt;
					ddat1use = 1;

					blitter_line_proc();
					blitter_nxline();

					/* onedot mode and no pixel = bus write access is skipped */
					if (blitlinepixel) {
						if (blt_info.bltddat)
							blt_info.blitzero = 0;
						chipmem_wput_indirect(bltdpt, blt_info.bltddat);
						alloc_cycle(hpos, CYCLE_BLITTER);
						blitlinepixel = 0;
					}
					bltdpt = bltcpt;

				} else {
					// normal mode A to D

					if (c == 4) {
						blitter_doddma_new(hpos);
					} else {
						blitter_dodma_new(c, hpos);
					}
				}

				blitter_next_cycle();

			  // check this after end check because last D write won't cause any problems.
			  check_channel_mods (hpos, c);
			}
			break;
		}

		hpos++;
	}
end:
	last_blitter_hpos = until_hpos;
 	reset_channel_mods ();
	if (hsync) {
		last_blitter_hpos = 0;
	}

	return written;
}
#else
void decide_blitter (int hpos) { }
#endif

static void blitter_force_finish(void)
{
  uae_u16 odmacon;
  if (!blt_info.blit_main && !blt_info.blit_finald)
    return;
  /* blitter is currently running
   * force finish (no blitter state support yet)
   */
  odmacon = dmacon;
  dmacon |= DMA_MASTER | DMA_BLITTER;
	if (blitter_cycle_exact && !immediate_blits) {
		int rounds = 10000;
	  while (blt_info.blit_main || blt_info.blit_finald && rounds > 0) {
			memset (cycle_line_slot, 0, sizeof cycle_line_slot);
			decide_blitter (-1);
			rounds--;
		}
		if (rounds == 0)
			write_log (_T("blitter froze!?\n"));
	} else {
    actually_do_blit ();
	}
	blitter_done ();
  dmacon = odmacon;
}

static void blit_modset (void)
{
	int mult;

	blit_add = blitdesc ? -2 : 2;
	mult = blitdesc ? -1 : 1;
	blit_modadda = mult * blt_info.bltamod;
	blit_modaddb = mult * blt_info.bltbmod;
	blit_modaddc = mult * blt_info.bltcmod;
	blit_modaddd = mult * blt_info.bltdmod;
}

void reset_blit (int bltcon)
{
	if (bltcon & 1)
		blinea_shift = bltcon0 >> 12;
	if (bltcon & 2)
		blitsign = bltcon1 & 0x40;
  if (!blt_info.blit_main && !blt_info.blit_finald)
  	return;
	if (bltcon)
	  blit_bltset (bltcon);
	blit_modset ();
}

static bool waitingblits (void)
{
  // crazy large blit size? don't wait.. (Vital / Mystic)
	if (blt_info.vblitsize * blt_info.hblitsize * 2 > 2 * 1024 * 1024) {
    return false;
  }

	while ((blt_info.blit_main || blt_info.blit_finald) && dmaen (DMA_BLITTER)) {
		x_do_cycles (8 * CYCLE_UNIT);
	}
	if (!blt_info.blit_main && !blt_info.blit_finald)
		return true;
	return false;
}

static void blitter_start_init (void)
{
  blit_faulty = 0;
	blt_info.blitzero = 1;

	blit_bltset (1 | 2);
	shifter_skip_b_old = shifter_skip_b;
	shifter_skip_y_old = shifter_skip_y;
	blit_modset ();
	ddat1use = 0;
	blt_info.blit_interrupt = 0;

  blt_info.bltaold = 0;
  blt_info.bltbold = 0;

	if (blitline) {
		blinea = blt_info.bltadat;
		blineb = (blt_info.bltbdat >> blt_info.blitbshift) | (blt_info.bltbdat << (16 - blt_info.blitbshift));
		blitonedot = 0;
		blitlinepixel = 0;
		blitsing = bltcon1 & 0x2;
	}

	if (!(dmacon & DMA_BLITPRI) && blt_info.nasty_cnt >= BLIT_NASTY_CPU_STEAL_CYCLE_COUNT) {
		blt_info.wait_nasty = 1;
	} else {
		blt_info.wait_nasty = 0;
	}
}

void do_blitter(int hpos, int copper)
{
	int cycles;

  bltcon0_old = bltcon0;
	bltcon1_old = bltcon1;

	blitter_cycle_exact = currprefs.blitter_cycle_exact;
	immediate_blits = currprefs.immediate_blits;
	blt_info.got_cycle = 0;
	blit_firstline_cycles = blit_first_cycle = get_cycles ();
	blit_last_cycle = 0;
	blit_cyclecounter = 0;
	blt_info.blit_pending = 1;

	blitter_start_init ();

	if (blitline) {
		cycles = blt_info.vblitsize * blt_info.hblitsize;
	} else {
		cycles = blt_info.vblitsize * blt_info.hblitsize;
		blit_firstline_cycles = blit_first_cycle + (blit_cyclecount * blt_info.hblitsize) * CYCLE_UNIT + cpu_cycles;
	}

	blit_slowdown = 0;

 	unset_special (SPCFLAG_BLTNASTY);
	if (dmaen(DMA_BLITPRI)) {
    set_special (SPCFLAG_BLTNASTY);
	}

	if (dmaen(DMA_BLITTER)) {
		blt_info.blit_main = 1;
		blt_info.blit_pending = 0;
	}

	blit_waitcyclecounter = 0;

	if (blitter_cycle_exact) {
		if (immediate_blits) {
			if (dmaen(DMA_BLITTER)) {
				blitter_doit ();
			}
			return;
		}
		blitter_hcounter = 0;
		blitter_vcounter = 0;
		blit_cyclecounter = -BLITTER_STARTUP_CYCLES;
		blit_waitcyclecounter = copper;
		blt_info.blit_pending = 0;
		blt_info.blit_main = 1;
    return;
	}

	if (blt_info.vblitsize == 0 || (blitline && blt_info.hblitsize != 2)) {
		if (dmaen(DMA_BLITTER)) {
			blitter_done ();
		}
		return;
	}

	if (dmaen (DMA_BLITTER)) {
		blt_info.got_cycle = 1;
	}

	if (immediate_blits) {
		if (dmaen(DMA_BLITTER)) {
      blitter_doit ();
    }
    return;
	}

  blit_cyclecounter = cycles * blit_cyclecount; 
  event2_newevent (ev2_blitter, blit_cyclecounter, 0);
}

void blitter_check_start (void)
{
  if (blt_info.blit_pending && !blt_info.blit_main) {
		blt_info.blit_pending = 0;
		blt_info.blit_main = 1;
	  blitter_start_init ();
	  if (immediate_blits) {
		  blitter_doit ();
    }
  }
}

void maybe_blit (int hpos, int hack)
{
	reset_channel_mods ();

	if (!blt_info.blit_main) {
		decide_blitter(hpos);
		return;
	}

	if (savestate_state)
		return;

	if (dmaen (DMA_BLITTER) && (currprefs.cpu_model >= 68020 || !currprefs.cpu_memory_cycle_exact)) {
		bool doit = false;
		if (currprefs.waiting_blits) { // automatic
			if (blit_dmacount == blit_cyclecount && (regs.spcflags & SPCFLAG_BLTNASTY))
				doit = true;
			else if (currprefs.m68k_speed < 0)
				doit = true;
		}
		if (doit) {
			if (waitingblits ())
				return;
		}
	}

	if (blitter_cycle_exact) {
		decide_blitter (hpos);
		return;
	}

  if (hack == 1 && (int)get_cycles() - (int)blit_firstline_cycles < 0)
  	return;

  blitter_handler (0);
}

void check_is_blit_dangerous (uaecptr *bplpt, int planes, int words)
{
	blt_info.blitter_dangerous_bpl = 0;
	if ((!blt_info.blit_main && !blt_info.blit_finald) || !blitter_cycle_exact)
		return;
	// too simple but better than nothing
	for (int i = 0; i < planes; i++) {
		uaecptr bpl = bplpt[i];
		uaecptr dpt = bltdpt & chipmem_bank.mask;
		if (dpt >= bpl - 2 * words && dpt < bpl + 2 * words) {
			blt_info.blitter_dangerous_bpl = 1;
			return;
		}
	}
}

int blitnasty (void)
{
	int cycles, ccnt;
	if (!blt_info.blit_main)
		return 0;
	if (!dmaen (DMA_BLITTER))
		return 0;
	if (blitter_cycle_exact) {
		blitter_force_finish();
		return -1;
	}
	if (blit_last_cycle >= blit_cyclecount && blit_dmacount == blit_cyclecount)
		return 0;
	cycles = (get_cycles () - blit_first_cycle) / CYCLE_UNIT;
	ccnt = 0;
	while (blit_last_cycle + blit_cyclecount < cycles) {
		ccnt += blit_dmacount;
		blit_last_cycle += blit_cyclecount;
	}
	return ccnt;
}

/* very approximate emulation of blitter slowdown caused by bitplane DMA */
void blitter_slowdown (int ddfstrt, int ddfstop, int totalcycles, int freecycles)
{
  static int oddfstrt, oddfstop, ototal, ofree;
  static int slow;

  if (!totalcycles || ddfstrt < 0 || ddfstop < 0)
  	return;
  if (ddfstrt != oddfstrt || ddfstop != oddfstop || totalcycles != ototal || ofree != freecycles) {
  	int linecycles = ((ddfstop - ddfstrt + totalcycles - 1) / totalcycles) * totalcycles;
  	int freelinecycles = ((ddfstop - ddfstrt + totalcycles - 1) / totalcycles) * freecycles;
	  int dmacycles = (linecycles * blit_dmacount) / blit_cyclecount;
	  oddfstrt = ddfstrt;
	  oddfstop = ddfstop;
	  ototal = totalcycles;
	  ofree = freecycles;
	  slow = 0;
	  if (dmacycles > freelinecycles)
	    slow = dmacycles - freelinecycles;
  }
  if (blit_slowdown < 0 || blitline)
	  return;
  blit_slowdown += slow;
}

void blitter_reset (void)
{
	bltptxpos = -1;
}

#ifdef SAVESTATE

void restore_blitter_finish (void)
{
  if (blt_statefile_type == 0) {
	  blt_info.blit_interrupt = 1;
  	if (blt_info.blit_pending) {
		  write_log (_T("blitter was started but DMA was inactive during save\n"));
	  }
	  if (blt_delayed_irq < 0) {
		  if (intreq & 0x0040)
			  blt_delayed_irq = 3;
		  intreq &= ~0x0040;
  	}
	} else {
		last_blitter_hpos = 0;
		blit_modset();
	}
}

uae_u8 *restore_blitter (uae_u8 *src)
{
  uae_u32 flags = restore_u32();

	blt_statefile_type = 0;
	blt_delayed_irq = 0;
	blt_info.blit_pending = 0;
	blt_info.blit_finald = 0;
	blt_info.blit_main = 0;
	if (flags & 4) {
    if (!(flags & 1)) {
			blt_info.blit_pending = 1;
		}
  }
	if (flags & 2) {
		write_log (_T("blitter was force-finished when this statefile was saved\n"));
		write_log (_T("contact the author if restored program freezes\n"));
		// there is a problem. if system ks vblank is active, we must not activate
		// "old" blit's intreq until vblank is handled or ks 1.x thinks it was blitter
		// interrupt..
		blt_delayed_irq = -1;
	}
  return src;
}

uae_u8 *save_blitter (int *len, uae_u8 *dstptr)
{
  uae_u8 *dstbak,*dst;
  int forced;

  forced = 0;
  if (blt_info.blit_main || blt_info.blit_finald) {
  	write_log (_T("blitter is active, forcing immediate finish\n"));
	  /* blitter is active just now but we don't have blitter state support yet */
		blitter_force_finish();
	  forced = 2;
  }
  if (dstptr)
  	dstbak = dst = dstptr;
  else
    dstbak = dst = xmalloc (uae_u8, 16);
  save_u32(((blt_info.blit_main || blt_info.blit_finald) ? 0 : 1) | forced | 4);
  *len = dst - dstbak;
  return dstbak;
}

// totally non-real-blitter-like state save but better than nothing..

uae_u8 *restore_blitter_new (uae_u8 *src)
{
	uae_u8 state, tmp;

	blt_statefile_type = 1;
	blitter_cycle_exact = restore_u8 ();
	if (blitter_cycle_exact & 2) {
		blt_statefile_type = 2;
		blitter_cycle_exact = 1;
	}

	state = restore_u8 ();

	blit_first_cycle = restore_u32 ();
	blit_last_cycle = restore_u32 ();
	blit_waitcyclecounter = restore_u32 ();
	restore_u32();
	/*blit_maxcyclecounter =*/ restore_u32 ();
	blit_firstline_cycles = restore_u32 ();
	blit_cyclecounter = restore_u32 ();
	blit_slowdown = restore_u32 ();

	blitter_hcounter = restore_u16();
	restore_u16();
	blitter_vcounter = restore_u16();
	restore_u16();
	/* blit_ch = */ restore_u8 ();
	restore_u8();
	restore_u8();
	restore_u8();
	blt_info.blit_finald = restore_u8();
	blitfc = restore_u8 ();
	blitife = restore_u8 ();

	blt_info.blitbshift = restore_u8 ();
	blt_info.blitdownbshift = restore_u8 ();
	blt_info.blitashift = restore_u8 ();
	blt_info.blitdownashift = restore_u8 ();

	ddat1use = restore_u8 ();
	restore_u8();
	ddat1 = restore_u16 ();
	restore_u16();

	blitline = restore_u8 ();
	blitfill = restore_u8 ();
	blinea = restore_u16 ();
	blineb = restore_u16 ();
	blinea_shift = restore_u8 ();
	blitonedot = restore_u8 ();
	blitlinepixel = restore_u8 ();
	blitsing = restore_u8 ();
	blt_info.blit_interrupt = restore_u8 ();
	blt_delayed_irq = restore_u8 ();
	blt_info.blitzero = restore_u8 ();
	blt_info.got_cycle = restore_u8 ();

	blit_faulty = restore_u8 ();
	restore_u8();
	restore_u8();

	if (restore_u16 () != 0x1234) {
		write_log (_T("blitter state restore error\n"));
	}

	blt_info.blitter_nasty = restore_u8 ();
  tmp = restore_u8();
	if (blt_statefile_type < 2) {
		tmp = 0;
		blt_info.blit_finald = 0;
	} else {
		shifter[0] = (tmp & 1) != 0;
		shifter[1] = (tmp & 2) != 0;
		shifter[2] = (tmp & 4) != 0;
		shifter[3] = (tmp & 8) != 0;
		blt_info.blit_finald = restore_u8();
		/* blit_ovf = */restore_u8();
	}

	blt_info.blit_main = 0;
	blt_info.blit_finald = 0;
	blt_info.blit_pending = 0;

	if (!blitter_cycle_exact) {
	  if (state > 0)
			do_blitter(0, 0);
	} else {
		// if old blitter active state file:
		// stop blitter. We can't support them anymore.
		if (state == 2 && blt_statefile_type < 2) {
			blt_info.blit_pending = 0;
			blt_info.blit_main = 0;
		} else {
			if (state == 1) {
				blt_info.blit_pending = 1;
			} else if (state == 2) {
				blt_info.blit_main = 1;
			}
			if (blt_info.blit_finald) {
				blt_info.blit_main = 0;
			}
			if (blt_statefile_type == 2) {
				blit_bltset(0);
			}
		}
	}
	return src;
}

uae_u8 *save_blitter_new (int *len, uae_u8 *dstptr)
{
	uae_u8 *dstbak,*dst;
	if (dstptr)
		dstbak = dst = dstptr;
	else
		dstbak = dst = xmalloc (uae_u8, 256);

	uae_u8 state;
	save_u8 (blitter_cycle_exact ? 3 : 0);
	if (!blt_info.blit_main && !blt_info.blit_finald) {
		state = 0;
	} else if (blt_info.blit_pending) {
		state = 1;
	} else {
		state = 2;
	}
	save_u8 (state);

	if (blt_info.blit_main || blt_info.blit_finald) {
		write_log (_T("BLITTER active while saving state\n"));
	}

	save_u32 (blit_first_cycle);
	save_u32 (blit_last_cycle);
	save_u32 (blit_waitcyclecounter);
	save_u32(0); //(blit_startcycles);
	save_u32 (0 /*blit_maxcyclecounter*/);
	save_u32 (blit_firstline_cycles);
	save_u32 (blit_cyclecounter);
	save_u32 (blit_slowdown);

	save_u16(blitter_hcounter);
	save_u16(0); //(blitter_hcounter2);
	save_u16(blitter_vcounter);
	save_u16(0); //(blitter_vcounter2);
	save_u8 (0 /*blit_ch*/);
	save_u8 (blit_dmacount);
	save_u8(blit_cyclecount);
	save_u8(0); //(blit_nod);
	save_u8(blt_info.blit_finald);
	save_u8 (blitfc);
	save_u8 (blitife);

	save_u8 (blt_info.blitbshift);
	save_u8 (blt_info.blitdownbshift);
	save_u8 (blt_info.blitashift);
	save_u8 (blt_info.blitdownashift);

	save_u8 (ddat1use);
	save_u8(0); //(ddat2use);
	save_u16 (ddat1);
	save_u16(0); //(ddat2);

	save_u8 (blitline);
	save_u8 (blitfill);
	save_u16 (blinea);
	save_u16 (blineb);
	save_u8 (blinea_shift);
	save_u8 (blitonedot);
	save_u8 (blitlinepixel);
	save_u8 (blitsing);
	save_u8 (blt_info.blit_interrupt);
	save_u8 (blt_delayed_irq);
	save_u8 (blt_info.blitzero);
	save_u8 (blt_info.got_cycle);
	
	save_u8 (blit_faulty);
	save_u8(0); //(original_ch);
	save_u8(0); //(get_cycle_diagram_type (blit_diag));

	save_u16 (0x1234);

	save_u8 (blt_info.blitter_nasty);
	save_u8((shifter[0] ? 1 : 0) | (shifter[1] ? 2 : 0) | (shifter[2] ? 4 : 0) | (shifter[3] ? 8 : 0));
	save_u8(blt_info.blit_finald);
	save_u8(0 /*blit_ovf*/);

	*len = dst - dstbak;
	return dstbak;
}

#endif /* SAVESTATE */
