$(shell ./link_pandora_dirs.sh)

NAME   = uae4arm
RM     = rm -f

PROG   = $(NAME)

all: $(PROG)

#DEBUG=1
#TRACER=1

PANDORA=1
#GEN_PROFILE=1
#USE_PROFILE=1

DEFAULT_CFLAGS = $(CFLAGS) `sdl-config --cflags`

MY_LDFLAGS = $(LDFLAGS)
MY_LDFLAGS += -lSDL -lpthread -lz -lSDL_image -lpng -lrt -lxml2 -lFLAC -lmpg123 -ldl -lmpeg2convert -lmpeg2
MY_LDFLAGS +=  -lSDL_ttf -lguichan_sdl -lguichan

MORE_CFLAGS = -DPANDORA -DARMV6_ASSEMBLY -DUSE_ARMNEON -DARMV6T2
MORE_CFLAGS += -DCPU_arm
MORE_CFLAGS += -DWITH_INGAME_WARNING
#MORE_CFLAGS += -DWITH_LOGGING

MORE_CFLAGS += -Isrc/osdep -Isrc -Isrc/include -Isrc/archivers -Isrc/od-pandora
MORE_CFLAGS += -Wno-write-strings -Wno-shift-overflow -Wno-narrowing -DUSE_SDL
MORE_CFLAGS += -msoft-float -fuse-ld=gold -fdiagnostics-color=auto
MORE_CFLAGS += -mstructure-size-boundary=32
MORE_CFLAGS += -falign-functions=32
MORE_CFLAGS += -std=gnu++14

TRACE_CFLAGS = 

ifndef DEBUG
MORE_CFLAGS += -Ofast -pipe -march=armv7-a -mtune=cortex-a8 -mfpu=neon
MORE_CFLAGS += -fweb -frename-registers
MORE_CFLAGS += -funroll-loops -ftracer -funswitch-loops

else
MORE_CFLAGS += -g -rdynamic -funwind-tables -mapcs-frame -DDEBUG -Wl,--export-dynamic
MORE_CFLAGS += -march=armv7-a -mtune=cortex-a8 -mfpu=neon

ifdef TRACER
TRACE_CFLAGS = -DTRACER -finstrument-functions -Wall -rdynamic
endif

endif

ifdef GEN_PROFILE
MORE_CFLAGS += -fprofile-generate=/media/MAINSD/pandora/test -fprofile-arcs -fvpt
endif
ifdef USE_PROFILE
MORE_CFLAGS += -fprofile-use -fbranch-probabilities -fvpt
endif


MY_CFLAGS  = $(MORE_CFLAGS) $(DEFAULT_CFLAGS)

OBJS =	\
	src/akiko.o \
	src/ar.o \
	src/aros.rom.o \
	src/audio.o \
	src/autoconf.o \
	src/blitfunc.o \
	src/blittable.o \
	src/blitter.o \
	src/blkdev.o \
	src/blkdev_cdimage.o \
	src/bsdsocket.o \
	src/calc.o \
	src/cd32_fmv.o \
	src/cd32_fmv_genlock.o \
	src/cdrom.o \
	src/cfgfile.o \
	src/cia.o \
	src/crc32.o \
	src/custom.o \
	src/def_icons.o \
	src/devices.o \
	src/disk.o \
	src/diskutil.o \
	src/drawing.o \
	src/events.o \
	src/expansion.o \
  src/fdi2raw.o \
	src/filesys.o \
	src/flashrom.o \
	src/fpp.o \
	src/fpp_native.o \
	src/fpp_softfloat.o \
	src/softfloat/softfloat.o \
	src/softfloat/softfloat_decimal.o \
	src/softfloat/softfloat_fpsp.o \
	src/fsdb.o \
	src/fsdb_unix.o \
	src/fsusage.o \
	src/gayle.o \
	src/gfxboard.o \
	src/gfxutil.o \
	src/hardfile.o \
	src/hrtmon.rom.o \
	src/ide.o \
	src/inputdevice.o \
	src/keybuf.o \
	src/main.o \
	src/memory.o \
	src/native2amiga.o \
	src/rommgr.o \
	src/rtc.o \
	src/savestate.o \
	src/scsi.o \
	src/statusline.o \
	src/traps.o \
	src/uaelib.o \
	src/uaeresource.o \
	src/zfile.o \
	src/zfile_archive.o \
	src/archivers/7z/7zAlloc.o \
	src/archivers/7z/7zBuf.o \
	src/archivers/7z/7zCrc.o \
	src/archivers/7z/7zCrcOpt.o \
	src/archivers/7z/7zDec.o \
	src/archivers/7z/7zIn.o \
	src/archivers/7z/7zStream.o \
  src/archivers/7z/Bcj2.o \
	src/archivers/7z/Bra.o \
	src/archivers/7z/Bra86.o \
	src/archivers/7z/LzmaDec.o \
	src/archivers/7z/Lzma2Dec.o \
  src/archivers/7z/BraIA64.o \
  src/archivers/7z/Delta.o \
  src/archivers/7z/Sha256.o \
  src/archivers/7z/Xz.o \
  src/archivers/7z/XzCrc64.o \
  src/archivers/7z/XzDec.o \
	src/archivers/dms/crc_csum.o \
	src/archivers/dms/getbits.o \
	src/archivers/dms/maketbl.o \
	src/archivers/dms/pfile.o \
	src/archivers/dms/tables.o \
	src/archivers/dms/u_deep.o \
	src/archivers/dms/u_heavy.o \
	src/archivers/dms/u_init.o \
	src/archivers/dms/u_medium.o \
	src/archivers/dms/u_quick.o \
	src/archivers/dms/u_rle.o \
	src/archivers/lha/crcio.o \
	src/archivers/lha/dhuf.o \
	src/archivers/lha/header.o \
	src/archivers/lha/huf.o \
	src/archivers/lha/larc.o \
	src/archivers/lha/lhamaketbl.o \
	src/archivers/lha/lharc.o \
	src/archivers/lha/shuf.o \
	src/archivers/lha/slide.o \
	src/archivers/lha/uae_lha.o \
	src/archivers/lha/util.o \
	src/archivers/lzx/unlzx.o \
	src/archivers/mp2/kjmp2.o \
	src/archivers/wrp/warp.o \
	src/archivers/zip/unzip.o \
	src/machdep/support.o \
	src/od-pandora/neon_helper.o \
	src/od-pandora/bsdsocket_host.o \
	src/od-pandora/cda_play.o \
	src/od-pandora/charset.o \
	src/od-pandora/fsdb_host.o \
	src/od-pandora/hardfile_pandora.o \
	src/od-pandora/keyboard.o \
	src/od-pandora/mp3decoder.o \
	src/od-pandora/picasso96.o \
	src/od-pandora/writelog.o \
	src/od-pandora/pandora.o \
	src/od-pandora/pandora_filesys.o \
	src/od-pandora/pandora_input.o \
	src/od-pandora/pandora_gui.o \
	src/od-pandora/pandora_rp9.o \
	src/od-pandora/pandora_gfx.o \
	src/od-pandora/pandora_mem.o \
	src/od-pandora/sigsegv_handler.o \
	src/sounddep/sound.o \
	src/od-pandora/gui/UaeRadioButton.o \
	src/od-pandora/gui/UaeDropDown.o \
	src/od-pandora/gui/UaeCheckBox.o \
	src/od-pandora/gui/UaeListBox.o \
	src/od-pandora/gui/InGameMessage.o \
	src/od-pandora/gui/SelectorEntry.o \
	src/od-pandora/gui/ShowHelp.o \
	src/od-pandora/gui/ShowMessage.o \
	src/od-pandora/gui/SelectFolder.o \
	src/od-pandora/gui/SelectFile.o \
	src/od-pandora/gui/CreateFilesysHardfile.o \
	src/od-pandora/gui/EditFilesysVirtual.o \
	src/od-pandora/gui/EditFilesysHardfile.o \
	src/od-pandora/gui/PanelPaths.o \
	src/od-pandora/gui/PanelQuickstart.o \
	src/od-pandora/gui/PanelConfig.o \
	src/od-pandora/gui/PanelCPU.o \
	src/od-pandora/gui/PanelChipset.o \
	src/od-pandora/gui/PanelROM.o \
	src/od-pandora/gui/PanelRAM.o \
	src/od-pandora/gui/PanelFloppy.o \
	src/od-pandora/gui/PanelHD.o \
	src/od-pandora/gui/PanelDisplay.o \
	src/od-pandora/gui/PanelSound.o \
	src/od-pandora/gui/PanelInput.o \
	src/od-pandora/gui/PanelMisc.o \
	src/od-pandora/gui/PanelSavestate.o \
	src/od-pandora/gui/main_window.o \
	src/od-pandora/gui/Navigation.o
ifdef PANDORA
OBJS += src/od-pandora/gui/sdltruetypefont.o
endif

OBJS += src/newcpu.o
OBJS += src/newcpu_common.o
OBJS += src/readcpu.o
OBJS += src/cpudefs.o
OBJS += src/cpustbl.o
OBJS += src/cpuemu_0.o
OBJS += src/cpuemu_4.o
OBJS += src/cpuemu_11.o
OBJS += src/cpuemu_40.o
OBJS += src/cpuemu_44.o
OBJS += src/jit/compemu.o
OBJS += src/jit/compstbl.o
OBJS += src/jit/compemu_fpp.o
OBJS += src/jit/compemu_support.o

src/od-pandora/neon_helper.o: src/od-pandora/neon_helper.s
	$(CXX) -falign-functions=32 -mcpu=cortex-a8 -mtune=cortex-a8 -mfpu=neon -mfloat-abi=softfp -Wall -o src/od-pandora/neon_helper.o -c src/od-pandora/neon_helper.s

src/trace.o: src/trace.c
	$(CC) $(MORE_CFLAGS) -c src/trace.c -o src/trace.o

.cpp.o:
	$(CXX) $(MY_CFLAGS) $(TRACE_CFLAGS) -c $< -o $@

.cpp.s:
	$(CXX) $(MY_CFLAGS) -S -c $< -o $@

$(PROG): $(OBJS) src/trace.o
ifndef DEBUG
	$(CXX) $(MY_CFLAGS) -o $(PROG) $(OBJS) $(MY_LDFLAGS)
	$(STRIP) $(PROG)
else
	$(CXX) $(MY_CFLAGS) -o $(PROG) $(OBJS) src/trace.o $(MY_LDFLAGS)
endif

ASMS = \
	src/audio.s \
	src/autoconf.s \
	src/blitfunc.s \
	src/blitter.s \
	src/cia.s \
	src/custom.s \
	src/disk.s \
	src/drawing.s \
	src/events.s \
	src/expansion.s \
	src/filesys.s \
	src/fpp.s \
	src/fsdb.s \
	src/fsdb_unix.s \
	src/fsusage.s \
	src/gfxutil.s \
	src/hardfile.s \
	src/inputdevice.s \
	src/keybuf.s \
	src/main.s \
	src/memory.s \
	src/native2amiga.s \
	src/traps.s \
	src/uaelib.s \
	src/uaeresource.s \
	src/zfile.s \
	src/zfile_archive.s \
	src/machdep/support.s \
	src/od-pandora/picasso96.s \
	src/od-pandora/pandora.s \
	src/od-pandora/pandora_gfx.s \
	src/od-pandora/pandora_mem.s \
	src/od-pandora/sigsegv_handler.s \
	src/sounddep/sound.s \
	src/newcpu.s \
	src/newcpu_common.s \
	src/readcpu.s \
	src/cpudefs.s \
	src/cpustbl.s \
	src/cpuemu_0.s \
	src/cpuemu_4.s \
	src/cpuemu_11.s \
	src/jit/compemu.s \
	src/jit/compemu_fpp.s \
	src/jit/compstbl.s \
	src/jit/compemu_support.s

genasm: $(ASMS)


clean:
	$(RM) $(PROG) $(OBJS) $(ASMS)

delasm:
	$(RM) $(ASMS)

bootrom:
	od -v -t xC -w8 src/filesys |tail -n +5 | sed -e "s,^.......,," -e "s,[0123456789abcdefABCDEF][0123456789abcdefABCDEF],db(0x&);,g" > src/filesys_bootrom.cpp
	touch src/filesys.cpp
