#include "sysconfig.h"
#include "sysdeps.h"

#include "options.h"
#include "threaddep/thread.h"
#include "traps.h"
#include "memory.h"
#include "audio.h"
#include "gfxboard.h"
#include "scsi.h"
#include "cd32_fmv.h"
#include "akiko.h"
#include "gayle.h"
#include "disk.h"
#include "cia.h"
#include "inputdevice.h"
#include "picasso96.h"
#include "blkdev.h"
#include "autoconf.h"
#include "newcpu.h"
#include "savestate.h"
#include "blitter.h"
#include "custom.h"
#include "xwin.h"
#include "bsdsocket.h"
#include "uaeresource.h"
#include "native2amiga.h"
#include "gensound.h"
#include "gui.h"
#ifdef JIT
#include "jit/compemu.h"
#endif

void devices_reset(int hardreset)
{
	gayle_reset (hardreset);
	DISK_reset ();
	CIA_reset ();
	gayle_reset (0);
#ifdef JIT
  compemu_reset ();
#endif
#ifdef AUTOCONFIG
	expamem_reset ();
	rtarea_reset();
#endif
	uae_int_requested = 0;
}


void devices_vsync_pre(void)
{
	blkdev_vsync ();
  CIA_vsync_prehandler();
  inputdevice_vsync ();
  filesys_vsync ();
#ifdef CD32
	cd32_fmv_vsync_handler();
#endif
}

void devices_hsync(void)
{
#ifdef CD32
	AKIKO_hsync_handler ();
	cd32_fmv_hsync_handler();
#endif

  DISK_hsync ();
  if (currprefs.produce_sound)
  	audio_hsync ();
	gayle_hsync ();
}

void devices_rethink(void)
{
  rethink_cias ();
#ifdef CD32
	rethink_akiko ();
	rethink_cd32fmv();
#endif
	rethink_gayle ();
	rethink_uae_int();
}

void devices_update_sync(float svpos, float syncadjust)
{
	cd32_fmv_set_sync(svpos, syncadjust);
}

void reset_all_systems (void)
{
  init_eventtab ();

#ifdef PICASSO96
  picasso_reset ();
#endif
#ifdef FILESYS
  filesys_prepare_reset ();
  filesys_reset ();
#endif
  memory_reset ();
#if defined (BSDSOCKET)
	bsdlib_reset ();
#endif
#ifdef FILESYS
  filesys_start_threads ();
  hardfile_reset ();
#endif
  native2amiga_reset ();
	uae_int_requested = 0;
}

void do_leave_program (void)
{
#ifdef JIT
  compiler_exit();
#endif
  graphics_leave ();
  inputdevice_close ();
  DISK_free ();
  close_sound ();
	dump_counts ();
#ifdef CD32
	akiko_free ();
	cd32_fmv_free();
#endif
 	gui_exit ();
#ifdef USE_SDL
  SDL_Quit ();
#endif
  hardfile_reset();
#ifdef AUTOCONFIG
  expansion_cleanup ();
#endif
#ifdef FILESYS
  filesys_cleanup ();
#endif
#ifdef BSDSOCKET
  bsdlib_reset ();
#endif
	gayle_free ();
	device_func_reset ();
  memory_cleanup ();
  cfgfile_addcfgparam (0);
  machdep_free ();
	rtarea_free();
}

void virtualdevice_init (void)
{
#ifdef AUTOCONFIG
	rtarea_setup ();
#endif
#ifdef FILESYS
	rtarea_init ();
	uaeres_install ();
	hardfile_install ();
#endif
#ifdef AUTOCONFIG
	expansion_init ();
	emulib_install ();
#endif
#ifdef FILESYS
	filesys_install ();
#endif
#if defined (BSDSOCKET)
	bsdlib_install ();
#endif
}

void devices_restore_start(void)
{
	restore_cia_start ();
	restore_blkdev_start();
  changed_prefs.bogomem_size = 0;
  changed_prefs.chipmem_size = 0;
  changed_prefs.fastmem_size = 0;
  changed_prefs.z3fastmem_size = 0;
	changed_prefs.mbresmem_low_size = 0;
	changed_prefs.mbresmem_high_size = 0;
}
