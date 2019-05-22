 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Stuff
  *
  * Copyright 1995, 1996 Ed Hanway
  * Copyright 1995-2001 Bernd Schmidt
  */

#ifndef UAE_OPTIONS_H
#define UAE_OPTIONS_H

#include "uae/types.h"

#include "traps.h"

#define UAEMAJOR 4
#define UAEMINOR 2
#define UAESUBREV 1

typedef enum { KBD_LANG_US, KBD_LANG_DK, KBD_LANG_DE, KBD_LANG_SE, KBD_LANG_FR, KBD_LANG_IT, KBD_LANG_ES } KbdLang;

extern uae_s32 version;

#define MAX_PATHS 1

struct multipath {
	TCHAR path[MAX_PATHS][PATH_MAX];
};

#define PATH_NONE -1
#define PATH_FLOPPY 0
#define PATH_CD 1
#define PATH_DIR 2
#define PATH_HDF 3
#define PATH_FS 4
#define PATH_ROM 9

struct strlist {
  struct strlist *next;
  TCHAR *option, *value;
  int unknown;
};

#define MAX_TOTAL_SCSI_DEVICES 1

/* maximum number native input devices supported (single type) */
#define MAX_INPUT_DEVICES 8
/* maximum number of native input device's buttons and axles supported */
#define MAX_INPUT_DEVICE_EVENTS 256
/* 4 different customization settings */
#define MAX_INPUT_SETTINGS 4
#define GAMEPORT_INPUT_SETTINGS 3 // last slot is for gameport panel mappings

#define MAX_INPUT_SUB_EVENT 8
#define MAX_INPUT_SUB_EVENT_ALL 9
#define SPARE_SUB_EVENT 8

#define INTERNALEVENT_COUNT 1

struct uae_input_device {
	TCHAR *name;
	TCHAR *configname;
	uae_s16 eventid[MAX_INPUT_DEVICE_EVENTS][MAX_INPUT_SUB_EVENT_ALL];
	uae_u64 flags[MAX_INPUT_DEVICE_EVENTS][MAX_INPUT_SUB_EVENT_ALL];
	uae_s8 port[MAX_INPUT_DEVICE_EVENTS][MAX_INPUT_SUB_EVENT_ALL];
	uae_s16 extra[MAX_INPUT_DEVICE_EVENTS];
	uae_s8 enabled;
};

#define MAX_JPORTS 4
#define NORMAL_JPORTS 2
#define MAX_JPORT_NAME 128
#define MAX_JPORT_CONFIG 256
struct inputdevconfig {
	TCHAR name[MAX_JPORT_NAME];
	TCHAR configname[MAX_JPORT_CONFIG];
	TCHAR shortid[16];
};
struct jport {
	int id;
	int mode; // 0=def,1=mouse,2=joy,3=anajoy
	int autofire;
	struct inputdevconfig idc;
	bool nokeyboardoverride;
	bool changed;
};
#define JPORT_UNPLUGGED -2
#define JPORT_NONE -1

#define JPORT_AF_NORMAL 1
#define JPORT_AF_TOGGLE 2
#define JPORT_AF_ALWAYS 3

#define MAX_CUSTOM_MEMORY_ADDRS 2

#define CONFIG_TYPE_ALL -1
#define CONFIG_TYPE_DEFAULT 0
#define CONFIG_TYPE_HARDWARE 1
#define CONFIG_TYPE_HOST 2
#define CONFIG_TYPE_NORESET 4
#define CONFIG_BLEN 2560

#define TABLET_OFF 0
#define TABLET_MOUSEHACK 1

struct cdslot
{
	TCHAR name[MAX_DPATH];
	bool inuse;
	bool delayed;
	bool temporary;
	int type;
};
struct floppyslot
{
	TCHAR df[MAX_DPATH];
	int dfxtype;
	bool forcedwriteprotect;
};

struct wh {
	int x, y;
  int width, height;
};

#define MOUNT_CONFIG_SIZE 30
#define UAEDEV_DIR 0
#define UAEDEV_HDF 1

#define HD_LEVEL_SCSI_1 0
#define HD_LEVEL_SCSI_2 1
#define HD_LEVEL_SASI 2
#define HD_LEVEL_SASI_ENHANCED 2
#define HD_LEVEL_SASI_CHS 3

#define HD_LEVEL_ATA_1 0
#define HD_LEVEL_ATA_2 1
#define HD_LEVEL_ATA_2S 2

#define BOOTPRI_NOAUTOBOOT -128
#define BOOTPRI_NOAUTOMOUNT -129
#define ISAUTOBOOT(ci) ((ci)->bootpri > BOOTPRI_NOAUTOBOOT)
#define ISAUTOMOUNT(ci) ((ci)->bootpri > BOOTPRI_NOAUTOMOUNT)
struct uaedev_config_info {
	int type;
  TCHAR devname[MAX_DPATH];
  TCHAR volname[MAX_DPATH];
  TCHAR rootdir[MAX_DPATH];
  bool readonly;
	bool lock;
  int bootpri;
  TCHAR filesys[MAX_DPATH];
	int lowcyl;
	int highcyl; // zero if detected from size
	int cyls; // calculated/corrected highcyl
  int surfaces;
  int sectors;
  int reserved;
  int blocksize;
	uae_u64 max_lba;
	int controller_type;
	int controller_type_unit;
	int controller_unit;
	int controller_media_type; // 1 = CF IDE, 0 = normal
	int unit_feature_level;
	int unit_special_flags;
	int pcyls, pheads, psecs;
	int flags;
	int buffers;
	int bufmemtype;
	int stacksize;
	int priority;
	uae_u32 mask;
	int maxtransfer;
	uae_u32 dostype;
	int unit;
	int interleave;
	int sectorsperblock;
	int forceload;
	int uae_unitnum; // mountunit nr
};

struct uaedev_config_data
{
	struct uaedev_config_info ci;
	int configoffset; // HD config entry index
	int unitnum; // scsi unit number (if tape currently)
};

enum { CP_GENERIC = 1, CP_CD32, CP_A500, CP_A500P, CP_A600,
	CP_A1200, CP_A2000, CP_A4000 };

#define IDE_A600A1200 1
#define IDE_A4000 2

#define MAX_CHIPSET_REFRESH 1
#define MAX_CHIPSET_REFRESH_TOTAL (MAX_CHIPSET_REFRESH + 2)
#define CHIPSET_REFRESH_PAL (MAX_CHIPSET_REFRESH + 0)
#define CHIPSET_REFRESH_NTSC (MAX_CHIPSET_REFRESH + 1)
struct chipset_refresh
{
	bool inuse;
	int index;
	bool rtg;
	int horiz;
	int vert;
	int lace;
	int resolution;
	int ntsc;
	double rate;
	TCHAR label[16];
};

#define APMODE_NATIVE 0
#define APMODE_RTG 1

struct apmode
{
	int gfx_refreshrate;
};


#define MAX_DUPLICATE_EXPANSION_BOARDS 1
#define MAX_EXPANSION_BOARDS 20
#define ROMCONFIG_CONFIGTEXT_LEN 256
struct boardromconfig;
struct romconfig
{
	TCHAR romfile[MAX_DPATH];
	TCHAR romident[256];
	uae_u32 board_ram_size;
	struct boardromconfig *back;
};
#define MAX_BOARD_ROMS 2
struct boardromconfig
{
	int device_type;
	int device_num;
	int device_order;
	struct romconfig roms[MAX_BOARD_ROMS];
};
#define MAX_RTG_BOARDS 1
struct rtgboardconfig
{
	int rtgmem_type;
	uae_u32 rtgmem_size;
	int device_order;
};
#define MAX_RAM_BOARDS 1
struct ramboard
{
	uae_u32 size;
	uae_u8 autoconfig[16];
	int device_order;
};
struct expansion_params
{
	int device_order;
};

#define Z3MAPPING_AUTO 0
#define Z3MAPPING_UAE 1
#define Z3MAPPING_REAL 2

struct monconfig
{
	struct wh gfx_size;
};

struct uae_prefs {

  struct strlist *all_lines;

  TCHAR description[256];
  TCHAR info[256];
  int config_version;

	bool socket_emu;

  bool start_gui;

	KbdLang keyboard_lang;

  int produce_sound;
  int sound_stereo;
  int sound_stereo_separation;
  int sound_mixed_stereo_delay;
  int sound_freq;
  int sound_interpol;
  int sound_filter;
  int sound_filter_type;
	int sound_volume_paula;
	int sound_volume_cd;

	bool compfpu;
  int cachesize;
	bool fpu_strict;

	struct monconfig gfx_monitor;
  int gfx_framerate;
	struct apmode gfx_apmode[2];
  int gfx_resolution;
 	int gfx_vresolution;

  bool immediate_blits;
	int waiting_blits;
  unsigned int chipset_mask;
  bool ntscmode;
  double chipset_refreshrate;
	struct chipset_refresh cr[MAX_CHIPSET_REFRESH + 2];
  int collision_level;
  int leds_on_screen;
	int leds_on_screen_mask[2];
  int fast_copper;
  int floppy_speed;
  int floppy_write_length;
	int floppy_auto_ext2;
	int cd_speed;
	int boot_rom;
	int filesys_limit;
	int filesys_max_name;

	int cs_compatible;
	int cs_ciaatod;
	int cs_rtc;
	bool cs_ksmirror_e0;
	bool cs_ksmirror_a8;
	bool cs_ciaoverlay;
	bool cs_cd32cd;
	bool cs_cd32c2p;
	bool cs_cd32nvram;
	bool cs_cd32fmv;
	int cs_cd32nvram_size;
	int cs_ide;
	bool cs_pcmcia;
	int cs_fatgaryrev;
	int cs_ramseyrev;
	bool cs_df0idhw;
	bool cs_ciatodbug;
	bool cs_z3autoconfig;
	bool cs_bytecustomwritebug;
	int cs_unmapped_space;
	int cs_ciatype[2];

	struct boardromconfig expansionboard[MAX_EXPANSION_BOARDS];

  TCHAR romfile[MAX_DPATH];
	TCHAR romident[256];
  TCHAR romextfile[MAX_DPATH];
	TCHAR romextident[256];
	TCHAR flashfile[MAX_DPATH];
	TCHAR cartfile[MAX_DPATH];
	TCHAR cartident[256];
	int cart_internal;
	struct cdslot cdslots[MAX_TOTAL_SCSI_DEVICES];

	struct multipath path_floppy;
	struct multipath path_hardfile;
	struct multipath path_rom;
	struct multipath path_cd;

  int m68k_speed;
  int cpu_model;
  int fpu_model;
  bool cpu_compatible;
	bool fpu_no_unimplemented;
  bool address_space_24;
  int picasso96_modeflags;

	uae_u32 z3autoconfig_start;
	struct ramboard z3fastmem[MAX_RAM_BOARDS];
	struct ramboard fastmem[MAX_RAM_BOARDS];
  uae_u32 chipmem_size;
  uae_u32 bogomem_size;
	uae_u32 mbresmem_low_size;
	uae_u32 mbresmem_high_size;
	struct rtgboardconfig rtgboards[MAX_RTG_BOARDS];
	uae_u32 custom_memory_addrs[MAX_CUSTOM_MEMORY_ADDRS];
	uae_u32 custom_memory_sizes[MAX_CUSTOM_MEMORY_ADDRS];
	uae_u32 custom_memory_mask[MAX_CUSTOM_MEMORY_ADDRS];
	int uaeboard;
	int uaeboard_order;

	int z3_mapping_mode;

  int mountitems;
  struct uaedev_config_data mountconfig[MOUNT_CONFIG_SIZE];

  int nr_floppies;
  struct floppyslot floppyslots[4];
	bool floppy_read_only;
	bool harddrive_read_only;

  /* Target specific options */
  
#ifdef PANDORA
  int pandora_cpu_speed;
  int pandora_tapDelay;
#endif
    
#ifdef RASPBERRY
  int gfx_correct_aspect;
  int gfx_fullscreen_ratio;
  int kbd_led_num;
  int kbd_led_scr;
  int kbd_led_cap;
#endif

  /* input */

	struct jport jports[MAX_JPORTS];
	int input_selected_setting;
  int input_joymouse_multiplier;
	int input_joymouse_deadzone;
	int input_joystick_deadzone;
	int input_joymouse_speed;
	int input_analog_joystick_mult;
	int input_analog_joystick_offset;
	int input_autofire_linecnt;
	int input_mouse_speed;
  int input_tablet;
	int input_keyboard_type;
	struct uae_input_device joystick_settings[MAX_INPUT_SETTINGS][MAX_INPUT_DEVICES];
	struct uae_input_device mouse_settings[MAX_INPUT_SETTINGS][MAX_INPUT_DEVICES];
	struct uae_input_device keyboard_settings[MAX_INPUT_SETTINGS][MAX_INPUT_DEVICES];
	struct uae_input_device internalevent_settings[MAX_INPUT_SETTINGS][INTERNALEVENT_COUNT];
	TCHAR input_config_name[GAMEPORT_INPUT_SETTINGS][256];
};

extern int config_changed;
extern void config_check_vsync (void);
extern void set_config_changed (void);

extern void cfgfile_write (struct zfile *, const TCHAR *option, const TCHAR *format,...);
extern void cfgfile_dwrite (struct zfile *, const TCHAR *option, const TCHAR *format,...);
extern void cfgfile_target_write (struct zfile *, const TCHAR *option, const TCHAR *format,...);
extern void cfgfile_target_dwrite (struct zfile *, const TCHAR *option, const TCHAR *format,...);

extern void cfgfile_write_bool (struct zfile *f, const TCHAR *option, bool b);
extern void cfgfile_target_write_bool (struct zfile *f, const TCHAR *option, bool b);
extern void cfgfile_target_dwrite_bool (struct zfile *f, const TCHAR *option, bool b);

extern void cfgfile_write_str (struct zfile *f, const TCHAR *option, const TCHAR *value);
extern void cfgfile_dwrite_str (struct zfile *f, const TCHAR *option, const TCHAR *value);
extern void cfgfile_target_write_str (struct zfile *f, const TCHAR *option, const TCHAR *value);
extern void cfgfile_target_dwrite_str (struct zfile *f, const TCHAR *option, const TCHAR *value);

extern struct uaedev_config_data *add_filesys_config (struct uae_prefs *p, int index, struct uaedev_config_info*);
extern void uci_set_defaults (struct uaedev_config_info *uci, bool rdb);

extern void error_log (const TCHAR*, ...);
extern void default_prefs (struct uae_prefs *, bool, int);
extern void discard_prefs (struct uae_prefs *, int);
extern void copy_prefs(struct uae_prefs *src, struct uae_prefs *dst);
extern int bip_a500 (struct uae_prefs *p, int rom);
extern int bip_a500plus (struct uae_prefs *p, int rom);
extern int bip_a1200 (struct uae_prefs *p, int rom);
extern int bip_a2000 (struct uae_prefs *p, int rom);
extern int bip_a4000 (struct uae_prefs *p, int rom);
extern int bip_cd32 (struct uae_prefs *p, int rom);

int parse_cmdline_option (struct uae_prefs *, TCHAR, const TCHAR *);

extern int cfgfile_separate_linea (const TCHAR *filename, char *line, TCHAR *line1b, TCHAR *line2b);
extern int cfgfile_yesno (const TCHAR *option, const TCHAR *value, const TCHAR *name, bool *location);
extern int cfgfile_intval (const TCHAR *option, const TCHAR *value, const TCHAR *name, int *location, int scale);
extern int cfgfile_strval (const TCHAR *option, const TCHAR *value, const TCHAR *name, int *location, const TCHAR *table[], int more);
extern int cfgfile_string (const TCHAR *option, const TCHAR *value, const TCHAR *name, TCHAR *location, int maxsz);

extern TCHAR *target_expand_environment (const TCHAR *path, TCHAR *out, int maxlen);
extern int target_parse_option (struct uae_prefs *, const TCHAR *option, const TCHAR *value);
extern void target_save_options (struct zfile*, struct uae_prefs *);
extern void target_default_options (struct uae_prefs *, int type);
extern void target_fixup_options (struct uae_prefs *);
extern int target_cfgfile_load (struct uae_prefs *, const TCHAR *filename, int type, int isdefault);
extern void cfgfile_resolve_path_out_load(const TCHAR *path, TCHAR *out, int size, int type);
extern void cfgfile_resolve_path_load(TCHAR *path, int size, int type);
extern void cfgfile_resolve_path_out_save(const TCHAR *path, TCHAR *out, int size, int type);

extern struct uae_prefs *cfgfile_open(const TCHAR *filename, int *type);
extern void cfgfile_close(struct uae_prefs *p);
extern int cfgfile_load (struct uae_prefs *p, const TCHAR *filename, int *type, int userconfig);
extern int cfgfile_save (struct uae_prefs *p, const TCHAR *filename, int);
extern void cfgfile_parse_line (struct uae_prefs *p, TCHAR *, int);
extern int cfgfile_parse_option (struct uae_prefs *p, const TCHAR *option, TCHAR *value, int);
extern int cfgfile_get_description (struct uae_prefs *p, const TCHAR *filename, TCHAR *description, int *type);
extern uae_u32 cfgfile_uaelib (TrapContext *ctx, int mode, uae_u32 name, uae_u32 dst, uae_u32 maxlen);
extern uae_u32 cfgfile_uaelib_modify (TrapContext *ctx, uae_u32 mode, uae_u32 parms, uae_u32 size, uae_u32 out, uae_u32 outsize);
extern void cfgfile_addcfgparam (TCHAR *);
extern int built_in_prefs (struct uae_prefs *p, int model, int config, int compa, int romcheck);
extern int built_in_chipset_prefs (struct uae_prefs *p);
extern void fixup_prefs_dimensions (struct uae_prefs *prefs);
extern void fixup_prefs (struct uae_prefs *prefs, bool userconfig);
extern void fixup_cpu (struct uae_prefs *prefs);
extern void cfgfile_compatibility_romtype(struct uae_prefs *p);
extern void cfgfile_compatibility_rtg(struct uae_prefs *p);
extern bool cfgfile_createconfigstore(struct uae_prefs *p);

extern void check_prefs_changed_custom (void);
extern void check_prefs_changed_cpu (void);
extern void check_prefs_changed_audio (void);
extern int check_prefs_changed_gfx (void);

extern struct uae_prefs currprefs, changed_prefs;

extern int machdep_init (void);
extern void machdep_free (void);

#endif /* UAE_OPTIONS_H */
