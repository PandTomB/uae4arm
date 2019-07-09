ifeq ($(PLATFORM),)
	PLATFORM = rpiA64
endif

ifeq ($(PLATFORM),rpi3)
  CPU_FLAGS += -march=armv8-a -mfpu=neon-fp-armv8 -mfloat-abi=hard -mtune=cortex-a53
  MORE_CFLAGS += -DARMV6T2 -DUSE_ARMNEON -DRASPBERRY -DARM_HAS_DIV -DARMV6_ASSEMBLY
  MORE_CFLAGS += -I/opt/vc/include -I/opt/vc/include/interface/vmcs_host/linux -I/opt/vc/include/interface/vcos/pthreads
  LDFLAGS += -lbcm_host -lvchiq_arm -lvcos -licui18n -licuuc -licudata -llzma -lfreetype -logg -lm -lX11 -L/opt/vc/lib
  PROFILER_PATH = /home/pi/test/uae4arm
  ifeq ($(USE_SDL_VERSION),)
    USE_SDL_VERSION = sdl1
  endif
else ifeq ($(PLATFORM),rpi2)
	CPU_FLAGS += -march=armv7-a -mfpu=neon -mfloat-abi=hard -mtune=cortex-a8
	MORE_CFLAGS += -DARMV6T2 -DUSE_ARMNEON -DRASPBERRY -DARMV6_ASSEMBLY
  MORE_CFLAGS += -I/opt/vc/include -I/opt/vc/include/interface/vmcs_host/linux -I/opt/vc/include/interface/vcos/pthreads
  LDFLAGS += -lbcm_host -lvchiq_arm -lvcos -licui18n -licuuc -licudata -llzma -lfreetype -logg -lm -lX11 -L/opt/vc/lib
  PROFILER_PATH = /home/pi/test/uae4arm
  ifeq ($(USE_SDL_VERSION),)
    USE_SDL_VERSION = sdl1
  endif
else ifeq ($(PLATFORM),rpi1)
	CPU_FLAGS += -march=armv6zk -mfpu=vfp -mfloat-abi=hard
  MORE_CFLAGS += -DRASPBERRY -DARMV6_ASSEMBLY
  MORE_CFLAGS += -I/opt/vc/include -I/opt/vc/include/interface/vmcs_host/linux -I/opt/vc/include/interface/vcos/pthreads
  LDFLAGS += -lbcm_host -lvchiq_arm -lvcos -licui18n -licuuc -licudata -llzma -lfreetype -logg -lm -lX11 -L/opt/vc/lib
  PROFILER_PATH = /home/pi/test/uae4arm
  ifeq ($(USE_SDL_VERSION),)
    USE_SDL_VERSION = sdl1
  endif
else ifeq ($(PLATFORM),Pandora)
  CPU_FLAGS += -march=armv7-a -mfpu=neon -mfloat-abi=softfp -mtune=cortex-a8
  MORE_CFLAGS += -DARMV6T2 -DUSE_ARMNEON  -DARMV6_ASSEMBLY -DPANDORA -msoft-float
  PROFILER_PATH = /media/MAINSD/pandora/test
  ifeq ($(USE_SDL_VERSION),)
    USE_SDL_VERSION = sdl1
  endif
else ifeq ($(PLATFORM),rpiA64sdl1)
  AARCH64 = 1
  CPU_FLAGS += -march=armv8-a -mtune=cortex-a53
  MORE_CFLAGS += -DRASPBERRY -DCPU_AARCH64
  LDFLAGS += -lX11
  PROFILER_PATH = /home/pi/test/uae4arm
  ifeq ($(USE_SDL_VERSION),)
    USE_SDL_VERSION = sdl1
  endif
else ifeq ($(PLATFORM),rpiA64)
  AARCH64 = 1
  CPU_FLAGS += -march=armv8-a -mtune=cortex-a53
  MORE_CFLAGS += -DRASPBERRY -DCPU_AARCH64
  MORE_CFLAGS += -I/opt/vc/include -I/opt/vc/include/interface/vmcs_host/linux -I/opt/vc/include/interface/vcos/pthreads
  LDFLAGS += -lbcm_host -lvchiq_arm -lvcos -licui18n -licuuc -licudata -llzma -lfreetype -logg -lm -lX11  -L/opt/vc/lib
  PROFILER_PATH = /home/pi/test/uae4arm
  ifeq ($(USE_SDL_VERSION),)
    USE_SDL_VERSION = sdl1
  endif
endif

ifeq ($(USE_SDL_VERSION),)
  USE_SDL_VERSION = sdl1
endif

NAME   = uae4arm
RM     = rm -f
CC     ?= gcc
CXX    ?= g++
STRIP  ?= strip

PROG   = $(NAME)

ifeq ($(USE_SDL_VERSION),sdl1)
all: $(PROG)
else
all: guisan $(PROG)
endif

guisan:
	$(MAKE) -C src/guisan

#DEBUG=1
#TRACER=1

#GEN_PROFILE=1
#USE_PROFILE=1

ifeq ($(USE_SDL_VERSION),sdl1)
SDL_CFLAGS = `sdl-config --cflags`
else
SDL_CFLAGS = `sdl2-config --cflags --libs`
endif

DEFS += `xml2-config --cflags`
DEFS += -DCPU_arm
DEFS += -DUSE_SDL
#DEFS += -DWITH_LOGGING

ifeq ($(USE_SDL_VERSION),sdl2)
MORE_CFLAGS += -Isrc/guisan/include -DUSE_SDL2
endif
MORE_CFLAGS += -Isrc -Isrc/osdep -Isrc/include -Isrc/archivers
MORE_CFLAGS += -Wno-write-strings -Wno-shift-overflow -Wno-narrowing
MORE_CFLAGS += -fuse-ld=gold -fdiagnostics-color=auto
MORE_CFLAGS += -falign-functions

LDFLAGS += -lpthread -lz -lpng -lrt -lxml2 -lFLAC -lmpg123 -ldl -lmpeg2convert -lmpeg2
ifeq ($(USE_SDL_VERSION),sdl1)
LDFLAGS += -lSDL -lSDL_image -lSDL_ttf -lguichan_sdl -lguichan
else
LDFLAGS += -lSDL2 -lSDL2_image -lSDL2_ttf -lguisan -Lsrc/guisan/lib
endif

TRACE_CFLAGS = 

ifndef DEBUG
ifdef AARCH64
  MORE_CFLAGS += -O3 -pipe
  MORE_CFLAGS += -funroll-loops
else
  MORE_CFLAGS += -O3 -pipe
  MORE_CFLAGS += -funroll-loops -ftracer
endif
else
  MORE_CFLAGS += -O2
  MORE_CFLAGS += -g -rdynamic -funwind-tables -DDEBUG -Wl,--export-dynamic
  ifdef TRACER
    TRACE_CFLAGS = -DTRACER -finstrument-functions -Wall -rdynamic
  endif
endif

ASFLAGS += $(CPU_FLAGS) -falign-functions

CXXFLAGS += $(SDL_CFLAGS) $(CPU_FLAGS) $(DEFS) $(MORE_CFLAGS)

ifdef GEN_PROFILE
MORE_CFLAGS += -fprofile-generate=$(PROFILER_PATH) -fprofile-arcs -fvpt
endif
ifdef USE_PROFILE
MORE_CFLAGS += -fprofile-use -fprofile-correction -fbranch-probabilities -fvpt
endif


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
	src/devices.o \
	src/disk.o \
	src/diskutil.o \
	src/dlopen.o \
	src/drawing.o \
	src/events.o \
	src/expansion.o \
  src/fdi2raw.o \
	src/filesys.o \
	src/flashrom.o \
	src/fpp.o \
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
	src/osdep/bsdsocket_host.o \
	src/osdep/caps/generic_caps.o \
	src/osdep/cda_play.o \
	src/osdep/charset.o \
	src/osdep/fsdb_host.o \
	src/osdep/keyboard.o \
	src/osdep/mp3decoder.o \
	src/osdep/picasso96.o \
	src/osdep/writelog.o \
	src/osdep/generic_hardfile.o \
	src/osdep/generic_target.o \
	src/osdep/generic_filesys.o \
	src/osdep/generic_gui.o \
	src/osdep/generic_rp9.o \
	src/osdep/generic_mem.o \
	src/osdep/sigsegv_handler.o \
	src/osdep/gui/GenericListModel.o \
	src/osdep/gui/UaeRadioButton.o \
	src/osdep/gui/UaeDropDown.o \
	src/osdep/gui/UaeCheckBox.o \
	src/osdep/gui/UaeListBox.o \
	src/osdep/gui/SelectorEntry.o \
	src/osdep/gui/ShowHelp.o \
	src/osdep/gui/ShowMessage.o \
	src/osdep/gui/SelectFolder.o \
	src/osdep/gui/SelectFile.o \
	src/osdep/gui/CreateFilesysHardfile.o \
	src/osdep/gui/EditFilesysVirtual.o \
	src/osdep/gui/EditFilesysHardfile.o \
	src/osdep/gui/PanelPaths.o \
	src/osdep/gui/PanelQuickstart.o \
	src/osdep/gui/PanelConfig.o \
	src/osdep/gui/PanelCPU.o \
	src/osdep/gui/PanelChipset.o \
	src/osdep/gui/PanelROM.o \
	src/osdep/gui/PanelRAM.o \
	src/osdep/gui/PanelFloppy.o \
	src/osdep/gui/PanelHD.o \
	src/osdep/gui/PanelInput.o \
	src/osdep/gui/PanelDisplay.o \
	src/osdep/gui/PanelSound.o \
	src/osdep/gui/PanelMisc.o \
	src/osdep/gui/PanelSavestate.o \
	src/osdep/gui/main_window.o \
	src/osdep/gui/Navigation.o
  

ifeq ($(USE_SDL_VERSION),sdl1)
OBJS += src/osdep/gui/sdltruetypefont.o
endif

ifeq ($(PLATFORM),Pandora)
OBJS += src/osdep/pandora.o
OBJS += src/osdep/pandora_gfx.o
OBJS += src/osdep/pandora_input.o
OBJS += src/sounddep/pandora_sound.o
OBJS += src/osdep/gui/PanelGamePortPandora.o
else
OBJS += src/osdep/raspi.o
ifeq ($(USE_SDL_VERSION),sdl2)
OBJS += src/osdep/raspi_gfxsdl2.o
else ifeq ($(PLATFORM),rpiA64sdl1)
OBJS += src/osdep/raspi_gfxsdl1.o
else
OBJS += src/osdep/raspi_gfx.o
endif
OBJS += src/osdep/raspi_input.o
OBJS += src/sounddep/sound_sdl.o
OBJS += src/osdep/gui/PanelGamePortRaspi.o
endif

ifdef AARCH64
	OBJS += src/osdep/aarch64_helper.o
else ifeq ($(PLATFORM),rpi1)
	OBJS += src/osdep/arm_helper.o
else
	OBJS += src/osdep/neon_helper.o
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

src/osdep/neon_helper.o: src/osdep/neon_helper.s
	$(CXX) $(ASFLAGS) -Wall -o src/osdep/neon_helper.o -c src/osdep/neon_helper.s

src/osdep/arm_helper.o: src/osdep/arm_helper.s
	$(CXX) $(ASFLAGS) -Wall -o src/osdep/arm_helper.o -c src/osdep/arm_helper.s

src/osdep/aarch64_helper.o: src/osdep/aarch64_helper.s
	$(CXX) $(ASFLAGS) -Wall -o src/osdep/aarch64_helper.o -c src/osdep/aarch64_helper.s

ifdef TRACER
src/trace.o: src/trace.c
	$(CC) $(MORE_CFLAGS) -c src/trace.c -o src/trace.o

OBJS += src/trace.o
endif

.cpp.o:
	$(CXX) $(CXXFLAGS) $(TRACE_CFLAGS) -c $< -o $@

.cpp.s:
	$(CXX) $(CXXFLAGS) -S -c $< -o $@

$(PROG): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(PROG) $(OBJS) $(LDFLAGS)
ifndef DEBUG
	$(STRIP) $(PROG)
endif

ASMS = \
	src/ar.s \
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
	src/osdep/picasso96.s \
	src/osdep/sigsegv_handler.s \
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
	$(RM) $(PROG) $(OBJS) $(ASMS) src/trace.o

delasm:
	$(RM) $(ASMS)

bootrom:
	od -v -t xC -w8 src/filesys |tail -n +5 | sed -e "s,^.......,," -e "s,[0123456789abcdefABCDEF][0123456789abcdefABCDEF],db(0x&);,g" > src/filesys_bootrom.cpp
	touch src/filesys.cpp
