/*
 * UAE - The Un*x Amiga Emulator
 *
 * Generic interface
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
#include "inputdevice.h"
#include "disk.h"
#include "savestate.h"
#include "rommgr.h"
#include "zfile.h"
#include <SDL.h>
#include "generic_rp9.h"

#ifdef WITH_LOGGING
extern FILE *debugfile;
#endif

int quickstart_start = 1;
int quickstart_model = 0;
int quickstart_conf = 0;
bool host_poweroff = false;

extern void signal_segv(int signum, siginfo_t* info, void*ptr);
extern void signal_buserror(int signum, siginfo_t* info, void*ptr);
extern void signal_term(int signum, siginfo_t* info, void*ptr);
extern void gui_force_rtarea_hdchange(void);

extern void SetLastActiveConfig(const char *filename);

char start_path_data[MAX_DPATH];
char currentDir[MAX_DPATH];

static char config_path[MAX_DPATH];
static char rom_path[MAX_DPATH];
static char rp9_path[MAX_DPATH];
char last_loaded_config[MAX_DPATH] = { '\0' };

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


void fix_apmodes(struct uae_prefs *p)
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
  
	const char *ptr = strstr((char *)filename, ".rp9");
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
#ifdef WITH_LOGGING
    p->leds_on_screen = true;
#endif
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

static void target_shutdown(void)
{
	system("sudo poweroff");
}

int generic_main (int argc, char *argv[])
{
  struct sigaction action;
  
	max_uae_width = 768;
	max_uae_height = 625;

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

  memset(&action, 0, sizeof(action));
  action.sa_sigaction = signal_term;
  action.sa_flags = SA_SIGINFO;
  if(sigaction(SIGTERM, &action, NULL) < 0)
  {
    printf("Failed to set signal handler (SIGTERM).\n");
    abort();
  }

  alloc_AmigaMem();
  RescanROMs();

  real_main (argc, argv);
  
  ClearAvailableROMList();
  romlist_clear();
  free_keyring();
  free_AmigaMem();
  lstMRUDiskList.clear();
  lstMRUCDList.clear();
  rp9_cleanup();
  
  logging_cleanup();

  if(host_poweroff)
	  target_shutdown();

  return 0;
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
