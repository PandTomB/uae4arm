 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Unix file system handler for AmigaDOS
  *
  * Copyright 1997 Bernd Schmidt
  */

struct hardfiledata {
    uae_u64 virtsize; // virtual size
    uae_u64 offset;
    int nrcyls;
    int secspertrack;
    int surfaces;
    int reservedblocks;
    int blocksize;
    FILE *fd;
    int readonly;
    int flags;
    TCHAR vendor_id[8 + 1];
    TCHAR product_id[16 + 1];
    TCHAR product_rev[4 + 1];
    TCHAR device_name[256];
    /* geometry from possible RDSK block */
    int cylinders;
    int sectors;
    int heads;
    int unitnum;

    int drive_empty;
};

#define HD_CONTROLLER_UAE 0
#define HD_CONTROLLER_IDE0 1
#define HD_CONTROLLER_IDE1 2
#define HD_CONTROLLER_IDE2 3
#define HD_CONTROLLER_IDE3 4
#define HD_CONTROLLER_SCSI0 5
#define HD_CONTROLLER_SCSI1 6
#define HD_CONTROLLER_SCSI2 7
#define HD_CONTROLLER_SCSI3 8
#define HD_CONTROLLER_SCSI4 9
#define HD_CONTROLLER_SCSI5 10
#define HD_CONTROLLER_SCSI6 11
#define HD_CONTROLLER_PCMCIA_SRAM 12
#define HD_CONTROLLER_PCMCIA_IDE 13

#define FILESYS_VIRTUAL 0
#define FILESYS_HARDFILE 1
#define FILESYS_HARDFILE_RDB 2
#define FILESYS_HARDDRIVE 3
#define FILESYS_CD 4

#define MAX_FILESYSTEM_UNITS 30

struct uaedev_mount_info;
extern struct uaedev_mount_info options_mountinfo;

extern struct hardfiledata *get_hardfile_data (int nr);
extern int get_native_path(uae_u32 lock, TCHAR *out);
extern void hardfile_do_disk_change (struct uaedev_config_info *uci, int insert);
