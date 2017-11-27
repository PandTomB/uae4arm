/*
 * UAE - The Un*x Amiga Emulator
 *
 * Pandora interface
 *
 */

#include <algorithm>
#include <iostream>
#include <vector>
#include <stdlib.h>
#include <stdarg.h>
#include <asm/sigcontext.h>
#include <signal.h>
#include <dlfcn.h>
#include <execinfo.h>
#include "sysconfig.h"
#include "sysdeps.h"
#include "config.h"
#include "uae.h"
#include "options.h"
#include "threaddep/thread.h"
#include "gui.h"
#include "memory.h"
#include "inputdevice.h"
#include "keyboard.h"
#include "disk.h"
#include "savestate.h"
#include "rtgmodes.h"
#include "rommgr.h"
#include "zfile.h"
#include "gfxboard.h"
#include <SDL.h>
#include "pandora_rp9.h"

#ifdef WITH_LOGGING
extern FILE *debugfile;
#endif

int quickstart_start = 1;
int quickstart_model = 0;
int quickstart_conf = 0;

extern void signal_segv(int signum, siginfo_t* info, void*ptr);
extern void signal_buserror(int signum, siginfo_t* info, void*ptr);
extern void gui_force_rtarea_hdchange(void);

static int delayed_mousebutton = 0;
static int doStylusRightClick;

extern void SetLastActiveConfig(const char *filename);

/* Keyboard */
int customControlMap[SDLK_LAST];

char start_path_data[MAX_DPATH];
char currentDir[MAX_DPATH];

static char config_path[MAX_DPATH];
static char rom_path[MAX_DPATH];
static char rp9_path[MAX_DPATH];
char last_loaded_config[MAX_DPATH] = { '\0' };

static bool cpuSpeedChanged = false;
static int lastCpuSpeed = 600;
int defaultCpuSpeed = 600;

int max_uae_width;
int max_uae_height;


extern "C" int main( int argc, char *argv[] );


void sleep_millis (int ms)
{
  usleep(ms * 1000);
}


void logging_init( void )
{
#ifdef WITH_LOGGING
  static int started;
  static int first;
  char debugfilename[MAX_DPATH];

  if (first > 1) {
  	write_log ("***** RESTART *****\n");
	  return;
  }
  if (first == 1) {
  	if (debugfile)
	    fclose (debugfile);
    debugfile = 0;
  }

	sprintf(debugfilename, "%s/uae4arm_log.txt", start_path_data);
	if(!debugfile)
    debugfile = fopen(debugfilename, "wt");

  first++;
  write_log ( "UAE4ARM Logfile\n\n");
#endif
}

void logging_cleanup( void )
{
#ifdef WITH_LOGGING
  if(debugfile)
    fclose(debugfile);
  debugfile = 0;
#endif
}


void stripslashes (TCHAR *p)
{
	while (_tcslen (p) > 0 && (p[_tcslen (p) - 1] == '\\' || p[_tcslen (p) - 1] == '/'))
		p[_tcslen (p) - 1] = 0;
}
void fixtrailing (TCHAR *p)
{
	if (_tcslen(p) == 0)
		return;
	if (p[_tcslen(p) - 1] == '/' || p[_tcslen(p) - 1] == '\\')
		return;
	_tcscat(p, "/");
}

void getpathpart (TCHAR *outpath, int size, const TCHAR *inpath)
{
	_tcscpy (outpath, inpath);
	TCHAR *p = _tcsrchr (outpath, '/');
	if (p)
		p[0] = 0;
	fixtrailing (outpath);
}
void getfilepart (TCHAR *out, int size, const TCHAR *path)
{
	out[0] = 0;
	const TCHAR *p = _tcsrchr (path, '/');
	if (p)
		_tcscpy (out, p + 1);
	else
		_tcscpy (out, path);
}


uae_u8 *target_load_keyfile (struct uae_prefs *p, const char *path, int *sizep, char *name)
{
  return 0;
}


void target_run (void)
{
  // Reset counter for access violations
  init_max_signals();
}

void target_quit (void)
{
}


static void fix_apmodes(struct uae_prefs *p)
{
  if(p->ntscmode)
  {
    p->gfx_apmode[0].gfx_refreshrate = 60;
    p->gfx_apmode[1].gfx_refreshrate = 60;
  }  
  else
  {
    p->gfx_apmode[0].gfx_refreshrate = 50;
    p->gfx_apmode[1].gfx_refreshrate = 50;
  }  

  p->gfx_apmode[0].gfx_vsync = 2;
  p->gfx_apmode[1].gfx_vsync = 2;
  p->gfx_apmode[0].gfx_vflip = -1;
  p->gfx_apmode[1].gfx_vflip = -1;
  
	fixup_prefs_dimensions (p);
}


void target_fixup_options (struct uae_prefs *p)
{
	p->rtgboards[0].rtgmem_type = GFXBOARD_UAE_Z3;

	if(z3base_adr == Z3BASE_REAL) {
  	// map Z3 memory at real address (0x40000000)
  	p->z3_mapping_mode = Z3MAPPING_REAL;
    p->z3autoconfig_start = z3base_adr;
	} else {
	  // map Z3 memory at UAE address (0x10000000)
  	p->z3_mapping_mode = Z3MAPPING_UAE;
    p->z3autoconfig_start = z3base_adr;
	}

  if(p->cs_cd32cd && p->cs_cd32nvram && (p->cs_compatible == CP_GENERIC || p->cs_compatible == 0)) {
    // Old config without cs_compatible, but other cd32-flags
    p->cs_compatible = CP_CD32;
    built_in_chipset_prefs(p);
  }

  if(p->cs_cd32cd && p->cartfile[0]) {
    p->cs_cd32fmv = 1;
  }
  
	p->picasso96_modeflags = RGBFF_CLUT | RGBFF_R5G6B5 | RGBFF_R8G8B8A8;
  p->gfx_resolution = p->gfx_size.width > 600 ? 1 : 0;
  
  if(p->cachesize > 0)
    p->fpu_no_unimplemented = 0;
  else
    p->fpu_no_unimplemented = 1;
  
  fix_apmodes(p);
}


void target_default_options (struct uae_prefs *p, int type)
{
  p->pandora_vertical_offset = OFFSET_Y_ADJUST;
  p->pandora_cpu_speed = defaultCpuSpeed;
  p->pandora_hide_idle_led = 0;
  
  p->pandora_tapDelay = 10;
	p->pandora_customControls = 0;

	p->picasso96_modeflags = RGBFF_CLUT | RGBFF_R5G6B5 | RGBFF_R8G8B8A8;
	
	memset(customControlMap, 0, sizeof(customControlMap));
	
	p->cr[CHIPSET_REFRESH_PAL].locked = true;
	p->cr[CHIPSET_REFRESH_PAL].vsync = 1;

	p->cr[CHIPSET_REFRESH_NTSC].locked = true;
	p->cr[CHIPSET_REFRESH_NTSC].vsync = 1;
	
	p->cr[0].index = 0;
	p->cr[0].horiz = -1;
	p->cr[0].vert = -1;
	p->cr[0].lace = -1;
	p->cr[0].vsync = 1;
	p->cr[0].rate = 50.0;
	p->cr[0].ntsc = 0;
	p->cr[0].locked = true;
	p->cr[0].rtg = true;
	_tcscpy (p->cr[0].label, _T("RTG"));
}


void target_save_options (struct zfile *f, struct uae_prefs *p)
{
  cfgfile_write (f, "pandora.cpu_speed", "%d", p->pandora_cpu_speed);
  cfgfile_write (f, "pandora.hide_idle_led", "%d", p->pandora_hide_idle_led);
  cfgfile_write (f, "pandora.tap_delay", "%d", p->pandora_tapDelay);
  cfgfile_write (f, "pandora.custom_controls", "%d", p->pandora_customControls);
  cfgfile_write (f, "pandora.custom_up", "%d", customControlMap[SDLK_UP]);
  cfgfile_write (f, "pandora.custom_down", "%d", customControlMap[SDLK_DOWN]);
  cfgfile_write (f, "pandora.custom_left", "%d", customControlMap[SDLK_LEFT]);
  cfgfile_write (f, "pandora.custom_right", "%d", customControlMap[SDLK_RIGHT]);
  cfgfile_write (f, "pandora.custom_a", "%d", customControlMap[SDLK_HOME]);
  cfgfile_write (f, "pandora.custom_b", "%d", customControlMap[SDLK_END]);
  cfgfile_write (f, "pandora.custom_x", "%d", customControlMap[SDLK_PAGEDOWN]);
  cfgfile_write (f, "pandora.custom_y", "%d", customControlMap[SDLK_PAGEUP]);
  cfgfile_write (f, "pandora.custom_l", "%d", customControlMap[SDLK_RSHIFT]);
  cfgfile_write (f, "pandora.custom_r", "%d", customControlMap[SDLK_RCTRL]);
  cfgfile_write (f, "pandora.move_y", "%d", p->pandora_vertical_offset - OFFSET_Y_ADJUST);
}


void target_restart (void)
{
  emulating = 0;
  gui_restart();
}


TCHAR *target_expand_environment (const TCHAR *path, TCHAR *out, int maxlen)
{
  if(out == NULL) {
    return strdup(path);
  } else {
    _tcscpy(out, path);
    return out;
  }
}

int target_parse_option (struct uae_prefs *p, const char *option, const char *value)
{
  int result = (cfgfile_intval (option, value, "cpu_speed", &p->pandora_cpu_speed, 1)
    || cfgfile_intval (option, value, "hide_idle_led", &p->pandora_hide_idle_led, 1)
    || cfgfile_intval (option, value, "tap_delay", &p->pandora_tapDelay, 1)
    || cfgfile_intval (option, value, "custom_controls", &p->pandora_customControls, 1)
    || cfgfile_intval (option, value, "custom_up", &customControlMap[SDLK_UP], 1)
    || cfgfile_intval (option, value, "custom_down", &customControlMap[SDLK_DOWN], 1)
    || cfgfile_intval (option, value, "custom_left", &customControlMap[SDLK_LEFT], 1)
    || cfgfile_intval (option, value, "custom_right", &customControlMap[SDLK_RIGHT], 1)
    || cfgfile_intval (option, value, "custom_a", &customControlMap[SDLK_HOME], 1)
    || cfgfile_intval (option, value, "custom_b", &customControlMap[SDLK_END], 1)
    || cfgfile_intval (option, value, "custom_x", &customControlMap[SDLK_PAGEDOWN], 1)
    || cfgfile_intval (option, value, "custom_y", &customControlMap[SDLK_PAGEUP], 1)
    || cfgfile_intval (option, value, "custom_l", &customControlMap[SDLK_RSHIFT], 1)
    || cfgfile_intval (option, value, "custom_r", &customControlMap[SDLK_RCTRL], 1)
    );
  if(!result) {
    result = cfgfile_intval (option, value, "move_y", &p->pandora_vertical_offset, 1);
    if(result)
      p->pandora_vertical_offset += OFFSET_Y_ADJUST;
  }

  return result;
}


void fetch_datapath (char *out, int size)
{
  strncpy(out, start_path_data, size);
  strncat(out, "/", size);
}


void fetch_saveimagepath (char *out, int size, int dir)
{
  strncpy(out, start_path_data, size);
  strncat(out, "/savestates/", size);
}


void fetch_configurationpath (char *out, int size)
{
  strncpy(out, config_path, size);
}


void set_configurationpath(char *newpath)
{
  strncpy(config_path, newpath, MAX_DPATH);
}


void fetch_rompath (char *out, int size)
{
  strncpy(out, rom_path, size);
}


void set_rompath(char *newpath)
{
  strncpy(rom_path, newpath, MAX_DPATH);
}


void fetch_rp9path (char *out, int size)
{
  strncpy(out, rp9_path, size);
}


void fetch_savestatepath(char *out, int size)
{
  strncpy(out, start_path_data, size);
  strncat(out, "/savestates/", size);
}


void fetch_screenshotpath(char *out, int size)
{
  strncpy(out, start_path_data, size);
  strncat(out, "/screenshots/", size);
}


int target_cfgfile_load (struct uae_prefs *p, const char *filename, int type, int isdefault)
{
  int i;
  int result = 0;

  write_log(_T("target_cfgfile_load(): load file %s\n"), filename);
  
  discard_prefs(p, type);
  default_prefs(p, true, 0);
  
	char *ptr = strstr((char *)filename, ".rp9");
  if(ptr > 0)
  {
    // Load rp9 config
    result = rp9_parse_file(p, filename);
    if(result)
      extractFileName(filename, last_loaded_config);
  }
  else 
	{
  	ptr = strstr((char *)filename, ".uae");
    if(ptr > 0)
    {
      int type = CONFIG_TYPE_HARDWARE | CONFIG_TYPE_HOST;
      result = cfgfile_load(p, filename, &type, 0, 1);
    }
    if(result)
      extractFileName(filename, last_loaded_config);
  }

  if(result)
  {
    for(i=0; i < p->nr_floppies; ++i)
    {
      if(!DISK_validate_filename(p, p->floppyslots[i].df, 0, NULL, NULL, NULL))
        p->floppyslots[i].df[0] = 0;
      disk_insert(i, p->floppyslots[i].df);
      if(strlen(p->floppyslots[i].df) > 0)
        AddFileToDiskList(p->floppyslots[i].df, 1);
    }

    if(!isdefault)
      inputdevice_updateconfig (NULL, p);
  
    SetLastActiveConfig(filename);
  }

  return result;
}


int check_configfile(char *file)
{
  char tmp[MAX_PATH];
  
  FILE *f = fopen(file, "rt");
  if(f)
  {
    fclose(f);
    return 1;
  }
  
  strncpy(tmp, file, MAX_PATH);
	char *ptr = strstr(tmp, ".uae");
	if(ptr > 0)
  {
    *(ptr + 1) = '\0';
    strncat(tmp, "conf", MAX_PATH);
    f = fopen(tmp, "rt");
    if(f)
    {
      fclose(f);
      return 2;
    }
  }

  return 0;
}


void extractFileName(const char * str,char *buffer)
{
	const char *p=str+strlen(str)-1;
	while(*p != '/' && p > str)
		p--;
	p++;
	strncpy(buffer,p, MAX_PATH);
}


void extractPath(char *str, char *buffer)
{
	strncpy(buffer, str, MAX_PATH);
	char *p = buffer + strlen(buffer) - 1;
	while(*p != '/' && p > buffer)
		p--;
	p[1] = '\0';
}


void removeFileExtension(char *filename)
{
  char *p = filename + strlen(filename) - 1;
  while(p > filename && *p != '.')
  {
    *p = '\0';
    --p;
  }
  *p = '\0';
}


void ReadDirectory(const char *path, std::vector<std::string> *dirs, std::vector<std::string> *files)
{
  DIR *dir;
  struct dirent *dent;

  if(dirs != NULL)
    dirs->clear();
  if(files != NULL)
    files->clear();
  
  dir = opendir(path);
  if(dir != NULL)
  {
    while((dent = readdir(dir)) != NULL)
    {
      if(dent->d_type == DT_DIR)
      {
        if(dirs != NULL)
          dirs->push_back(dent->d_name);
      }
      else if (files != NULL)
        files->push_back(dent->d_name);
    }
    if(dirs != NULL && dirs->size() > 0 && (*dirs)[0] == ".")
      dirs->erase(dirs->begin());
    closedir(dir);
  }
  
  if(dirs != NULL)
    std::sort(dirs->begin(), dirs->end());
  if(files != NULL)
    std::sort(files->begin(), files->end());
}


void saveAdfDir(void)
{
	char path[MAX_DPATH];
	int i;
	
	snprintf(path, MAX_DPATH, "%s/conf/adfdir.conf", start_path_data);
	FILE *f=fopen(path,"w");
	if (!f)
	  return;
	  
	char buffer[MAX_DPATH];
	snprintf(buffer, MAX_DPATH, "path=%s\n", currentDir);
	fputs(buffer,f);

	snprintf(buffer, MAX_DPATH, "config_path=%s\n", config_path);
	fputs(buffer, f);

	snprintf(buffer, MAX_DPATH, "rom_path=%s\n", rom_path);
	fputs(buffer, f);

  snprintf(buffer, MAX_DPATH, "ROMs=%d\n", lstAvailableROMs.size());
  fputs(buffer, f);
  for(i=0; i<lstAvailableROMs.size(); ++i)
  {
    snprintf(buffer, MAX_DPATH, "ROMName=%s\n", lstAvailableROMs[i]->Name);
    fputs(buffer, f);
    snprintf(buffer, MAX_DPATH, "ROMPath=%s\n", lstAvailableROMs[i]->Path);
    fputs(buffer, f);
    snprintf(buffer, MAX_DPATH, "ROMType=%d\n", lstAvailableROMs[i]->ROMType);
    fputs(buffer, f);
  }
  
  snprintf(buffer, MAX_DPATH, "MRUDiskList=%d\n", lstMRUDiskList.size());
  fputs(buffer, f);
  for(i=0; i<lstMRUDiskList.size(); ++i)
  {
    snprintf(buffer, MAX_DPATH, "Diskfile=%s\n", lstMRUDiskList[i].c_str());
    fputs(buffer, f);
  }

  snprintf(buffer, MAX_DPATH, "MRUCDList=%d\n", lstMRUCDList.size());
  fputs(buffer, f);
  for(i=0; i<lstMRUCDList.size(); ++i)
  {
    snprintf(buffer, MAX_DPATH, "CDfile=%s\n", lstMRUCDList[i].c_str());
    fputs(buffer, f);
  }

  snprintf(buffer, MAX_DPATH, "Quickstart=%d\n", quickstart_start);
  fputs(buffer, f);

	fclose(f);
	return;
}


void get_string(FILE *f, char *dst, int size)
{
  char buffer[MAX_PATH];
  fgets(buffer, MAX_PATH, f);
  int i = strlen (buffer);
  while (i > 0 && (buffer[i - 1] == '\t' || buffer[i - 1] == ' ' 
  || buffer[i - 1] == '\r' || buffer[i - 1] == '\n'))
	  buffer[--i] = '\0';
  strncpy(dst, buffer, size);
}


static void trimwsa (char *s)
{
  /* Delete trailing whitespace.  */
  int len = strlen (s);
  while (len > 0 && strcspn (s + len - 1, "\t \r\n") == 0)
    s[--len] = '\0';
}


void loadAdfDir(void)
{
	char path[MAX_DPATH];
  int i;

	strncpy(currentDir, start_path_data, MAX_DPATH);
	snprintf(config_path, MAX_DPATH, "%s/conf/", start_path_data);
	snprintf(rom_path, MAX_DPATH, "%s/kickstarts/", start_path_data);
	snprintf(rp9_path, MAX_DPATH, "%s/rp9/", start_path_data);

	snprintf(path, MAX_DPATH, "%s/conf/adfdir.conf", start_path_data);
  struct zfile *fh;
  fh = zfile_fopen (path, _T("r"), ZFD_NORMAL);
  if (fh) {
    char linea[CONFIG_BLEN];
    TCHAR option[CONFIG_BLEN], value[CONFIG_BLEN];
    int numROMs, numDisks, numCDs;
    int romType = -1;
    char romName[MAX_PATH] = { '\0' };
    char romPath[MAX_PATH] = { '\0' };
    char tmpFile[MAX_PATH];
    
    while (zfile_fgetsa (linea, sizeof (linea), fh) != 0) {
    	trimwsa (linea);
    	if (strlen (linea) > 0) {
  	    if (!cfgfile_separate_linea (path, linea, option, value))
      		continue;
        
        if(cfgfile_string(option, value, "ROMName", romName, sizeof(romName))
        || cfgfile_string(option, value, "ROMPath", romPath, sizeof(romPath))
        || cfgfile_intval(option, value, "ROMType", &romType, 1)) {
          if(strlen(romName) > 0 && strlen(romPath) > 0 && romType != -1) {
            AvailableROM *tmp = new AvailableROM();
            strncpy(tmp->Name, romName, sizeof(tmp->Name));
            strncpy(tmp->Path, romPath, sizeof(tmp->Path));
            tmp->ROMType = romType;
            lstAvailableROMs.push_back(tmp);
            strncpy(romName, "", sizeof(romName));
            strncpy(romPath, "", sizeof(romPath));
            romType = -1;
          }
        } else if (cfgfile_string(option, value, "Diskfile", tmpFile, sizeof(tmpFile))) {
          FILE *f = fopen(tmpFile, "rb");
          if(f != NULL) {
            fclose(f);
            lstMRUDiskList.push_back(tmpFile);
          }
        } else if (cfgfile_string(option, value, "CDfile", tmpFile, sizeof(tmpFile))) {
          FILE *f = fopen(tmpFile, "rb");
          if(f != NULL) {
            fclose(f);
            lstMRUCDList.push_back(tmpFile);
          }
        } else {
          cfgfile_string(option, value, "path", currentDir, sizeof(currentDir));
          cfgfile_string(option, value, "config_path", config_path, sizeof(config_path));
          cfgfile_string(option, value, "rom_path", rom_path, sizeof(rom_path));
          cfgfile_intval(option, value, "ROMs", &numROMs, 1);
          cfgfile_intval(option, value, "MRUDiskList", &numDisks, 1);
          cfgfile_intval(option, value, "MRUCDList", &numCDs, 1);
          cfgfile_intval(option, value, "Quickstart", &quickstart_start, 1);
        }
    	}
    }
    zfile_fclose (fh);
  }
}


int currVSyncRate = 0;
bool SetVSyncRate(int hz)
{
	char cmd[64];
  
  if(currVSyncRate != hz && (hz == 50 || hz == 60))
  {
    snprintf((char*)cmd, 64, "sudo /usr/pandora/scripts/op_lcdrate.sh %d", hz);
    system(cmd);
    currVSyncRate = hz;
    return true;
  }
  return false;
}

void setCpuSpeed()
{
	char speedCmd[128];

  currprefs.pandora_cpu_speed = changed_prefs.pandora_cpu_speed;

	if(currprefs.pandora_cpu_speed != lastCpuSpeed)
	{
		snprintf((char*)speedCmd, 128, "unset DISPLAY; echo y | sudo -n /usr/pandora/scripts/op_cpuspeed.sh %d", currprefs.pandora_cpu_speed);
		system(speedCmd);
		lastCpuSpeed = currprefs.pandora_cpu_speed;
		cpuSpeedChanged = true;
	}
	if(changed_prefs.ntscmode != currprefs.ntscmode)
	{
		if(changed_prefs.ntscmode)
			SetVSyncRate(60);
		else
			SetVSyncRate(50);
		fix_apmodes(&changed_prefs);
	}
}


int getDefaultCpuSpeed(void)
{
  int speed = 600;
  FILE* f = fopen ("/etc/pandora/conf/cpu.conf", "rt");
  if(f)
  {
    char line[128];
    for(int i=0; i<6; ++i)
    {
      fscanf(f, "%s\n", &line);
      if(strncmp(line, "default:", 8) == 0)
      {
        int value = 0;
        sscanf(line, "default:%d", &value);
        if(value > 500 && value < 1200)
        {
          speed = value;
        }
      }
    }
    fclose(f);
  }
  return speed;
}


void resetCpuSpeed(void)
{
  if(cpuSpeedChanged)
  {
    lastCpuSpeed = defaultCpuSpeed - 10;
    currprefs.pandora_cpu_speed = changed_prefs.pandora_cpu_speed = defaultCpuSpeed;
    setCpuSpeed();
  }
}


void target_addtorecent (const TCHAR *name, int t)
{
}


void target_reset (void)
{
}


bool target_can_autoswitchdevice(void)
{
	return true;
}


uae_u32 emulib_target_getcpurate (uae_u32 v, uae_u32 *low)
{
  *low = 0;
  if (v == 1) {
    *low = 1e+9; /* We have nano seconds */
    return 0;
  } else if (v == 2) {
    int64_t time;
    struct timespec ts;
    clock_gettime (CLOCK_MONOTONIC, &ts);
    time = (((int64_t) ts.tv_sec) * 1000000000) + ts.tv_nsec;
    *low = (uae_u32) (time & 0xffffffff);
    return (uae_u32)(time >> 32);
  }
  return 0;
}

int main (int argc, char *argv[])
{
  struct sigaction action;
  
	max_uae_width = 768;
	max_uae_height = 270;

  defaultCpuSpeed = getDefaultCpuSpeed();
  
  // Get startup path
	getcwd(start_path_data, MAX_DPATH);
	loadAdfDir();
  rp9_init();

  snprintf(savestate_fname, MAX_PATH, "%s/saves/default.ads", start_path_data);
	logging_init ();
  
  memset(&action, 0, sizeof(action));
  action.sa_sigaction = signal_segv;
  action.sa_flags = SA_SIGINFO;
  if(sigaction(SIGSEGV, &action, NULL) < 0)
  {
    printf("Failed to set signal handler (SIGSEGV).\n");
    abort();
  }
  if(sigaction(SIGILL, &action, NULL) < 0)
  {
    printf("Failed to set signal handler (SIGILL).\n");
    abort();
  }

  memset(&action, 0, sizeof(action));
  action.sa_sigaction = signal_buserror;
  action.sa_flags = SA_SIGINFO;
  if(sigaction(SIGBUS, &action, NULL) < 0)
  {
    printf("Failed to set signal handler (SIGBUS).\n");
    abort();
  }

  alloc_AmigaMem();
  RescanROMs();

  keyboard_settrans();  
  real_main (argc, argv);
  
  ClearAvailableROMList();
  romlist_clear();
  free_keyring();
  free_AmigaMem();
  lstMRUDiskList.clear();
  lstMRUCDList.clear();
  rp9_cleanup();
  
  logging_cleanup();

  return 0;
}


int handle_msgpump (void)
{
	int got = 0;
  SDL_Event rEvent;
  int keycode;
  int modifier;
  int handled = 0;
  int i;
  
  if(delayed_mousebutton) {
    --delayed_mousebutton;
    if(delayed_mousebutton == 0)
      setmousebuttonstate (0, 0, 1);
  }
  
	while (SDL_PollEvent(&rEvent)) {
		got = 1;
		
		switch (rEvent.type)
		{
  		case SDL_QUIT:
  			uae_quit();
  			break;
  			
  		case SDL_KEYDOWN:
  		  switch(rEvent.key.keysym.sym)
  		  {
  		    case SDLK_LCTRL: // Select key
  		      inputdevice_add_inputcode (AKS_ENTERGUI, 1);
  		      break;

				  case SDLK_LSHIFT: // Shift key
            inputdevice_do_keyboard(AK_LSH, 1);
            break;
            
				  case SDLK_RSHIFT: // Left shoulder button
				  case SDLK_RCTRL:  // Right shoulder button
  					if(currprefs.input_tablet > TABLET_OFF) {
  					  // Holding left or right shoulder button -> stylus does right mousebutton
  					  doStylusRightClick = 1;
            }
            // Fall through...
            
  				default:
  				  if(currprefs.pandora_customControls) {
  				    keycode = customControlMap[rEvent.key.keysym.sym];
  				    if(keycode < 0) {
  				      // Simulate mouse or joystick
  				      SimulateMouseOrJoy(keycode, 1);
  				      break;
  				    }
  				    else if(keycode > 0) {
  				      // Send mapped key press
  				      inputdevice_do_keyboard(keycode, 1);
  				      break;
  				    }
  				  }
  				    
				    // Handle Pandora specific functions
				    if(rEvent.key.keysym.mod == (KMOD_RCTRL | KMOD_RSHIFT)) {
              // Left and right shoulder
  				    switch(rEvent.key.keysym.sym) {
  				      case SDLK_UP:  // Left and right shoulder + UP -> move display up
  				        moveVertical(1);
  				        handled = 1;
  				        break;
  				        
  				      case SDLK_DOWN:  // Left and right shoulder + DOWN -> move display down
  				        moveVertical(-1);
  				        handled = 1;
  				        break;
  				        
  				      case SDLK_1:
  				      case SDLK_2:
  				      case SDLK_3:
  				      case SDLK_4:
  				      case SDLK_5:
  				      case SDLK_6:
  				        // Change display height direct
  				        changed_prefs.gfx_size.height = amigaheight_values[rEvent.key.keysym.sym - SDLK_1];
  				        update_display(&changed_prefs);
  				        handled = 1;
  				        break;

                case SDLK_9:  // Select next lower display height
            			for(i=0; i<AMIGAHEIGHT_COUNT; ++i)
            			{
            			  if(currprefs.gfx_size.height == amigaheight_values[i])
            		    {
            		      if(i > 0)
            		        changed_prefs.gfx_size.height = amigaheight_values[i - 1];
            		      else
            		        changed_prefs.gfx_size.height = amigaheight_values[AMIGAHEIGHT_COUNT - 1];
            		      break;
            		    }
            			}
            			update_display(&changed_prefs);
  				        handled = 1;
  				        break;

                case SDLK_0:  // Select next higher display height
            			for(i=0; i<AMIGAHEIGHT_COUNT; ++i)
            			{
            			  if(currprefs.gfx_size.height == amigaheight_values[i])
            		    {
            		      if(i < AMIGAHEIGHT_COUNT - 1)
            		        changed_prefs.gfx_size.height = amigaheight_values[i + 1];
            		      else
            		        changed_prefs.gfx_size.height = amigaheight_values[0];
            		      break;
            		    }
            			}
            			update_display(&changed_prefs);
  				        handled = 1;
  				        break;

                case SDLK_w:  // Select next display width
            			for(i=0; i<AMIGAWIDTH_COUNT; ++i)
            			{
            			  if(currprefs.gfx_size.width == amigawidth_values[i])
            		    {
            		      if(i < AMIGAWIDTH_COUNT - 1)
            		        changed_prefs.gfx_size.width = amigawidth_values[i + 1];
            		      else
            		        changed_prefs.gfx_size.width = amigawidth_values[0];
            		      break;
            		    }
            			}
            			update_display(&changed_prefs);
  				        handled = 1;
  				        break;

                case SDLK_r:
            		  // Toggle resolution (lores/hires)
            		  if(currprefs.gfx_size.width > 600)
            		    changed_prefs.gfx_size.width = currprefs.gfx_size.width / 2;
            		  else
            		    changed_prefs.gfx_size.width = currprefs.gfx_size.width * 2;
            			update_display(&changed_prefs);
  				        handled = 1;
  				        break;

              }              
            }
				    else if(rEvent.key.keysym.mod == KMOD_RSHIFT)
			      {
			        // Left shoulder
  				    switch(rEvent.key.keysym.sym) {
  				      case SDLK_c:  // Left shoulder + c -> toggle "Custom control"
  				        currprefs.pandora_customControls = !currprefs.pandora_customControls;
  				        changed_prefs.pandora_customControls = currprefs.pandora_customControls;
  				        handled = 1;
  				        break;
  				        
  				      case SDLK_d:  // Left shoulder + d -> toggle "Status line"
  			          changed_prefs.leds_on_screen = !changed_prefs.leds_on_screen;
  				        handled = 1;
  				        break;
  				        
  				      case SDLK_f:  // Left shoulder + f -> toggle frameskip
  				        changed_prefs.gfx_framerate ? changed_prefs.gfx_framerate = 0 : changed_prefs.gfx_framerate = 1;
  				        set_config_changed();
  				        handled = 1;
				          break;
				          
				        case SDLK_s:  // Left shoulder + s -> store savestate
                  savestate_initsave(savestate_fname, 2, 0, false);
            			save_state (savestate_fname, "...");
				          savestate_state = STATE_DOSAVE;
  				        handled = 1;
				          break;
				          
				        case SDLK_l:  // Left shoulder + l -> load savestate
                	{
                		FILE *f=fopen(savestate_fname, "rb");
                		if(f)
                		{
                			fclose(f);
                			savestate_state = STATE_DORESTORE;
                		}
                	}
  				        handled = 1;
                	break;

#ifdef ACTION_REPLAY
                case SDLK_a:  // Left shoulder + a -> activate cardridge (Action Replay)
        		      if(currprefs.cartfile[0] != '\0') {
          		      inputdevice_add_inputcode (AKS_FREEZEBUTTON, 1);
    				        handled = 1;
    				      }
                	break;
#endif
  			      }
				    }

            if(!handled) {
    				  modifier = rEvent.key.keysym.mod;
    				  keycode = translate_pandora_keys(rEvent.key.keysym.sym, &modifier);
    				  if(keycode)
    				  {
  				      if(modifier == KMOD_SHIFT)
    				      inputdevice_do_keyboard(AK_LSH, 1);
    				    else
    				      inputdevice_do_keyboard(AK_LSH, 0);
  				      inputdevice_do_keyboard(keycode, 1);
    				  } else {
  				      inputdevice_translatekeycode(0, rEvent.key.keysym.sym, 1);
  				    }
            }
            				    				  
  				  break;
				}
        break;
        
  	  case SDL_KEYUP:
  	    switch(rEvent.key.keysym.sym)
  	    {
  		    case SDLK_LCTRL: // Select key
  		      break;

				  case SDLK_LSHIFT: // Shift key
            inputdevice_do_keyboard(AK_LSH, 0);
            break;
            
				  case SDLK_RSHIFT: // Left shoulder button
				  case SDLK_RCTRL:  // Right shoulder button
  					if(currprefs.input_tablet > TABLET_OFF) {
  					  // Release left or right shoulder button -> stylus does left mousebutton
    					doStylusRightClick = 0;
            }
            // Fall through...
  				
  				default:
  				  if(currprefs.pandora_customControls) {
  				    keycode = customControlMap[rEvent.key.keysym.sym];
  				    if(keycode < 0) {
  				      // Simulate mouse or joystick
  				      SimulateMouseOrJoy(keycode, 0);
  				      break;
  				    }
  				    else if(keycode > 0) {
  				      // Send mapped key release
  				      inputdevice_do_keyboard(keycode, 0);
  				      break;
  				    }
  				  }

  				  modifier = rEvent.key.keysym.mod;
  				  keycode = translate_pandora_keys(rEvent.key.keysym.sym, &modifier);
  				  if(keycode)
  				  {
				      inputdevice_do_keyboard(keycode, 0);
				      if(modifier == KMOD_SHIFT)
  				      inputdevice_do_keyboard(AK_LSH, 0);
            } else {
				      inputdevice_translatekeycode(0, rEvent.key.keysym.sym, 0);
				    }
  				  break;
  	    }
  	    break;
  	    
  	  case SDL_MOUSEBUTTONDOWN:
        if(currprefs.jports[0].id == JSEM_MICE || currprefs.jports[1].id == JSEM_MICE) {
    	    if(rEvent.button.button == SDL_BUTTON_LEFT) {
    	      if(currprefs.input_tablet > TABLET_OFF && !doStylusRightClick) {
    	        // Delay mousebutton, we need new position first...
    	        delayed_mousebutton = currprefs.pandora_tapDelay << 1;
    	      } else {
      	      setmousebuttonstate (0, doStylusRightClick, 1);
      	    }
    	    }
    	    else if(rEvent.button.button == SDL_BUTTON_RIGHT)
    	      setmousebuttonstate (0, 1, 1);
        }
  	    break;

  	  case SDL_MOUSEBUTTONUP:
        if(currprefs.jports[0].id == JSEM_MICE || currprefs.jports[1].id == JSEM_MICE) {
    	    if(rEvent.button.button == SDL_BUTTON_LEFT) {
  	        setmousebuttonstate (0, doStylusRightClick, 0);
          }
    	    else if(rEvent.button.button == SDL_BUTTON_RIGHT)
    	      setmousebuttonstate (0, 1, 0);
        }
  	    break;
  	    
  		case SDL_MOUSEMOTION:
  		  if(currprefs.input_tablet == TABLET_OFF) {
          if(currprefs.jports[0].id == JSEM_MICE || currprefs.jports[1].id == JSEM_MICE) {
  			    int x, y;
    		    int mouseScale = currprefs.input_joymouse_multiplier / 2;
            x = rEvent.motion.xrel;
    				y = rEvent.motion.yrel;
    				if(rEvent.motion.x == 0 && x > -4)
    					x = -4;
    				if(rEvent.motion.y == 0 && y > -4)
    					y = -4;
    				if(rEvent.motion.x == currprefs.gfx_size.width - 1 && x < 4)
    					x = 4;
    				if(rEvent.motion.y == currprefs.gfx_size.height - 1 && y < 4)
    					y = 4;
  				  setmousestate(0, 0, x * mouseScale, 0);
        	  setmousestate(0, 1, y * mouseScale, 0);
          }
        }
        break;
		}
	}
	return got;
}


static uaecptr clipboard_data;

void amiga_clipboard_die (void)
{
}

void amiga_clipboard_init (void)
{
}

void amiga_clipboard_task_start (uaecptr data)
{
  clipboard_data = data;
}

uae_u32 amiga_clipboard_proc_start (void)
{
  return clipboard_data;
}

void amiga_clipboard_got_data (uaecptr data, uae_u32 size, uae_u32 actual)
{
}

int amiga_clipboard_want_data (void)
{
  return 0;
}

void clipboard_vsync (void)
{
}
