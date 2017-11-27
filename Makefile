PREFIX	=/usr
$(shell ./link_pandora_dirs.sh)

#SDL_BASE = $(PREFIX)/bin/
SDL_BASE = 

NAME   = uae4arm
O      = o
RM     = rm -f
#CC     = gcc
#CXX    = g++
STRIP  = strip
#AS     = as

PROG   = $(NAME)

all: $(PROG)

PANDORA=1

#USE_XFD=1

DEFAULT_CFLAGS = `$(SDL_BASE)sdl-config --cflags`

LDFLAGS = -lSDL -lpthread  -lz -lSDL_image -lpng -lrt
LDFLAGS +=  -lSDL_ttf -lguichan_sdl -lguichan

MORE_CFLAGS += -DGP2X -DPANDORA -DARMV6_ASSEMBLY -DUSE_ARMNEON
MORE_CFLAGS += -DCPU_arm -DARM_ASSEMBLY
#MORE_CFLAGS += -DWITH_LOGGING
#MORE_CFLAGS += -DDEBUG_M68K

MORE_CFLAGS += -Isrc -Isrc/include -fomit-frame-pointer -Wno-unused -Wno-format -Wno-write-strings -DUSE_SDL
MORE_CFLAGS += -fexceptions
MORE_CFLAGS += -msoft-float -ffast-math -D_GLIBCXX_USE_CXX11_ABI=0

ifndef DEBUG
MORE_CFLAGS += -O3 -fexpensive-optimizations
MORE_CFLAGS += -fstrict-aliasing -mstructure-size-boundary=32
MORE_CFLAGS += -fweb -frename-registers -fomit-frame-pointer
#MORE_CFLAGS += -falign-functions=32 -falign-loops -falign-labels -falign-jumps
MORE_CFLAGS += -falign-functions=32
MORE_CFLAGS += -finline -finline-functions -fno-builtin
#MORE_CFLAGS += -S
else
MORE_CFLAGS += -ggdb
endif

CFLAGS  = $(DEFAULT_CFLAGS) $(MORE_CFLAGS)

OBJS =	\
	src/audio.o \
	src/autoconf.o \
	src/blitfunc.o \
	src/blittable.o \
	src/blitter.o \
	src/cfgfile.o \
	src/cia.o \
	src/crc32.o \
	src/custom.o \
	src/disk.o \
	src/drawing.o \
	src/ersatz.o \
	src/expansion.o \
	src/filesys.o \
	src/fpp.o \
	src/fsdb.o \
	src/fsdb_unix.o \
	src/fsusage.o \
	src/gfxutil.o \
	src/hardfile.o \
	src/inputdevice.o \
	src/keybuf.o \
	src/main.o \
	src/memory.o \
	src/missing.o \
	src/native2amiga.o \
	src/savestate.o \
	src/traps.o \
	src/uaelib.o \
	src/uaeresource.o \
	src/zfile.o \
	src/zfile_archive.o \
	src/archivers/7z/7zAlloc.o \
	src/archivers/7z/7zBuffer.o \
	src/archivers/7z/7zCrc.o \
	src/archivers/7z/7zDecode.o \
	src/archivers/7z/7zExtract.o \
	src/archivers/7z/7zHeader.o \
	src/archivers/7z/7zIn.o \
	src/archivers/7z/7zItem.o \
	src/archivers/7z/7zMethodID.o \
	src/archivers/7z/LzmaDecode.o \
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
	src/archivers/wrp/warp.o \
	src/archivers/zip/unzip.o \
	src/machdep/support.o \
	src/osdep/neon_helper.o \
	src/osdep/fsdb_host.o \
	src/osdep/joystick.o \
	src/osdep/keyboard.o \
	src/osdep/inputmode.o \
	src/osdep/picasso96.o \
	src/osdep/writelog.o \
	src/osdep/pandora.o \
	src/osdep/pandora_filesys.o \
	src/osdep/pandora_gui.o \
	src/osdep/pandora_gfx.o \
	src/osdep/pandora_mem.o \
	src/osdep/sigsegv_handler.o \
	src/osdep/menu/menu_config.o \
	src/sounddep/sound.o \
	src/osdep/gui/UaeRadioButton.o \
	src/osdep/gui/UaeDropDown.o \
	src/osdep/gui/UaeCheckBox.o \
	src/osdep/gui/UaeListBox.o \
	src/osdep/gui/InGameMessage.o \
	src/osdep/gui/SelectorEntry.o \
	src/osdep/gui/ShowMessage.o \
	src/osdep/gui/SelectFolder.o \
	src/osdep/gui/SelectFile.o \
	src/osdep/gui/CreateFilesysHardfile.o \
	src/osdep/gui/EditFilesysVirtual.o \
	src/osdep/gui/EditFilesysHardfile.o \
	src/osdep/gui/PanelPaths.o \
	src/osdep/gui/PanelConfig.o \
	src/osdep/gui/PanelCPU.o \
	src/osdep/gui/PanelChipset.o \
	src/osdep/gui/PanelROM.o \
	src/osdep/gui/PanelRAM.o \
	src/osdep/gui/PanelFloppy.o \
	src/osdep/gui/PanelHD.o \
	src/osdep/gui/PanelDisplay.o \
	src/osdep/gui/PanelSound.o \
	src/osdep/gui/PanelInput.o \
	src/osdep/gui/PanelMisc.o \
	src/osdep/gui/PanelSavestate.o \
	src/osdep/gui/main_window.o \
	src/osdep/gui/Navigation.o
ifdef PANDORA
OBJS += src/osdep/gui/sdltruetypefont.o
endif

ifdef USE_XFD
OBJS += src/cpu_small.o \
	src/cpuemu_small.o \
	src/cpustbl_small.o \
	src/archivers/xfd/xfd.o
endif

OBJS += src/newcpu.o
OBJS += src/readcpu.o
OBJS += src/cpudefs.o
OBJS += src/cpustbl.o
OBJS += src/cpuemu_0.o
OBJS += src/cpuemu_4.o
OBJS += src/cpuemu_11.o
OBJS += src/jit/compemu.o
OBJS += src/jit/compemu_fpp.o
OBJS += src/jit/compstbl.o
OBJS += src/jit/compemu_support.o

CPPFLAGS  = $(CFLAGS)

src/osdep/neon_helper.o: src/osdep/neon_helper.s
	$(CXX) -falign-functions=32 -march=armv7-a -mcpu=cortex-a8 -mtune=cortex-a8 -mfpu=neon -mfloat-abi=softfp -Wall -o src/osdep/neon_helper.o -c src/osdep/neon_helper.s

$(PROG): $(OBJS)
	$(CXX) $(CFLAGS) -o $(PROG) $(OBJS) $(LDFLAGS)
ifndef DEBUG
	$(STRIP) $(PROG)
endif

clean:
	$(RM) $(PROG) $(OBJS)
