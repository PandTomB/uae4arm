 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Interface to the Tcl/Tk GUI
  *
  * Copyright 1996 Bernd Schmidt
  */

#ifndef UAE_GUI_H
#define UAE_GUI_H

#include "uae/types.h"

extern int gui_init (void);
extern int gui_update (void);
extern void gui_exit (void);
extern void gui_led (int, int, int);
extern void gui_filename (int, const TCHAR *);
extern void gui_flicker_led (int, int, int);
extern void gui_display (void);

#define LED_CD_ACTIVE 1
#define LED_CD_ACTIVE2 2
#define LED_CD_AUDIO 4

#define LED_POWER 0
#define LED_DF0 1
#define LED_DF1 2
#define LED_DF2 3
#define LED_DF3 4
#define LED_HD 5
#define LED_CD 6
#define LED_FPS 7
#define LED_CPU 8
#define LED_SND 9
#define LED_NET 10
#define LED_MAX 11

struct gui_info_drive {
  bool drive_motor;    /* motor on off */
  uae_u8 drive_track;    /* rw-head track */
  bool drive_writing;  /* drive is writing */
  TCHAR df[256];		    /* inserted image */
  uae_u32 crc32;		    /* crc32 of image */
};

struct gui_info
{
  bool powerled;          /* state of power led */
  uae_s8 drive_side;			/* floppy side */
  uae_s8 hd;			        /* harddrive */
  uae_s8 cd;			        /* CD */
  int cpu_halted;
  int fps, idle;
	struct gui_info_drive drives[4];
};
#define NUM_LEDS (LED_MAX)
#define VISIBLE_LEDS (LED_MAX)

extern struct gui_info gui_data;

void notify_user (int msg);
void notify_user_parms (int msg, const TCHAR *parms, ...);
int translate_message (int msg, TCHAR *out);

typedef enum {
	NUMSG_NEEDEXT2, // 0
	NUMSG_NOROM,
	NUMSG_NOROMKEY,
	NUMSG_KSROMCRCERROR,
	NUMSG_KSROMREADERROR,
	NUMSG_NOEXTROM, // 5
	NUMSG_KS68EC020,
	NUMSG_KS68020,
	NUMSG_KS68030,
	NUMSG_ROMNEED,
	NUMSG_EXPROMNEED, // 10
	NUMSG_NOZLIB,
	NUMSG_STATEHD,
	NUMSG_NOCAPS,
	NUMSG_OLDCAPS,
	NUMSG_KICKREP, // 15
	NUMSG_KICKREPNO,
	NUMSG_KS68030PLUS,
	NUMSG_NOMEMORY,
	NUMSG_LAST
} notify_user_msg;

#endif /* UAE_GUI_H */
