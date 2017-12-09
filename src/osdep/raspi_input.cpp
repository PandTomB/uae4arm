#include "sysconfig.h"
#include "sysdeps.h"
#include "config.h"
#include "options.h"
#include "keyboard.h"
#include "inputdevice.h"
#include <SDL.h>


static int joyXviaCustom = 0;
static int joyYviaCustom = 0;
static int joyButXviaCustom[7] = { 0, 0, 0, 0, 0, 0, 0};
static int mouseBut1viaCustom = 0;
static int mouseBut2viaCustom = 0;


#define MAX_MOUSE_BUTTONS	  2
#define MAX_MOUSE_AXES	    2
#define FIRST_MOUSE_AXIS	  0
#define FIRST_MOUSE_BUTTON	MAX_MOUSE_AXES

static int numMice = 0;


static int init_mouse (void) 
{
  int mouse = open("/dev/input/mouse0", O_RDONLY);
  if(mouse != -1) {
    numMice++;
    close(mouse);
  }
  return numMice;
}

static void close_mouse (void) 
{
}

static int acquire_mouse (int num, int flags) 
{
  if(num >= 0 && num < numMice)
    return 1;
  return 0;
}

static void unacquire_mouse (int num) 
{
}

static int get_mouse_num (void)
{
  return numMice;
}

static TCHAR *get_mouse_friendlyname (int mouse)
{
  if(numMice > 0 && mouse == 0)
    return "Mouse";
  else
    return "";
}

static TCHAR *get_mouse_uniquename (int mouse)
{
  if(numMice > 0 && mouse == 0)
    return "MOUSE0";
  else
    return "";
}

static int get_mouse_widget_num (int mouse)
{
  if(numMice > 0 && mouse == 0) {
    return MAX_MOUSE_AXES + MAX_MOUSE_BUTTONS;
  }
  return 0;
}

static int get_mouse_widget_first (int mouse, int type)
{
  if(numMice > 0 && mouse == 0) {
    switch (type) {
    	case IDEV_WIDGET_BUTTON:
  	    return FIRST_MOUSE_BUTTON;
  	  case IDEV_WIDGET_AXIS:
  	    return FIRST_MOUSE_AXIS;
    	case IDEV_WIDGET_BUTTONAXIS:
    	  return MAX_MOUSE_AXES + MAX_MOUSE_BUTTONS; 
    }
  }
  return -1;
}

static int get_mouse_widget_type (int mouse, int num, TCHAR *name, uae_u32 *code)
{
  if(numMice > 0 && mouse == 0) {
    if (num >= MAX_MOUSE_AXES && num < MAX_MOUSE_AXES + MAX_MOUSE_BUTTONS) {
    	if (name)
  	    sprintf (name, "Button %d", num + 1 - MAX_MOUSE_AXES);
  	  return IDEV_WIDGET_BUTTON;
    } else if (num < MAX_MOUSE_AXES) {
  	  if (name) {
  	    if(num == 0)
  	      sprintf (name, "X Axis");
  	    else if (num == 1)
  	      sprintf (name, "Y Axis");
  	    else
  	      sprintf (name, "Axis %d", num + 1);
  	  }
  	  return IDEV_WIDGET_AXIS;
    }
  }
  return IDEV_WIDGET_NONE;
}

static void read_mouse (void) 
{
  if(currprefs.input_tablet > TABLET_OFF) {
    // Mousehack active
    int x, y;
    SDL_GetMouseState(&x, &y);
	  setmousestate(0, 0, x, 1);
#ifdef USE_SDL2
	  setmousestate(0, 1, y, 1);
#else
	  setmousestate(0, 1, y - OFFSET_Y_ADJUST, 1);
#endif
  }
}


static int get_mouse_flags (int num) 
{
  return 0;
}

struct inputdevice_functions inputdevicefunc_mouse = {
  init_mouse, close_mouse, acquire_mouse, unacquire_mouse, read_mouse,
  get_mouse_num, get_mouse_friendlyname, get_mouse_uniquename,
  get_mouse_widget_num, get_mouse_widget_type,
  get_mouse_widget_first,
  get_mouse_flags
};

static void setid (struct uae_input_device *uid, int i, int slot, int sub, int port, int evt, bool gp)
{
	if (gp)
		inputdevice_sparecopy (&uid[i], slot, 0);
  uid[i].eventid[slot][sub] = evt;
  uid[i].port[slot][sub] = port + 1;
}

static void setid_af (struct uae_input_device *uid, int i, int slot, int sub, int port, int evt, int af, bool gp)
{
  setid (uid, i, slot, sub, port, evt, gp);
  uid[i].flags[slot][sub] &= ~ID_FLAG_AUTOFIRE_MASK;
  if (af >= JPORT_AF_NORMAL)
    uid[i].flags[slot][sub] |= ID_FLAG_AUTOFIRE;
}

int input_get_default_mouse (struct uae_input_device *uid, int i, int port, int af, bool gp, bool wheel, bool joymouseswap)
{
  setid (uid, i, ID_AXIS_OFFSET + 0, 0, port, port ? INPUTEVENT_MOUSE2_HORIZ : INPUTEVENT_MOUSE1_HORIZ, gp);
  setid (uid, i, ID_AXIS_OFFSET + 1, 0, port, port ? INPUTEVENT_MOUSE2_VERT : INPUTEVENT_MOUSE1_VERT, gp);
  setid_af (uid, i, ID_BUTTON_OFFSET + 0, 0, port, port ? INPUTEVENT_JOY2_FIRE_BUTTON : INPUTEVENT_JOY1_FIRE_BUTTON, af, gp);
  setid (uid, i, ID_BUTTON_OFFSET + 1, 0, port, port ? INPUTEVENT_JOY2_2ND_BUTTON : INPUTEVENT_JOY1_2ND_BUTTON, gp);

  if (i == 0)
    return 1;
  return 0;
}


static int init_kb (void)
{
  return 1;
}

static void close_kb (void)
{
}

static int acquire_kb (int num, int flags)
{
  return 1;
}

static void unacquire_kb (int num)
{
}

static void read_kb (void)
{
}

static int get_kb_num (void) 
{
  return 1;
}

static TCHAR *get_kb_friendlyname (int kb) 
{
  return strdup("Default Keyboard");
}

static TCHAR *get_kb_uniquename (int kb) 
{
  return strdup("KEYBOARD0");
}

static int get_kb_widget_num (int kb) 
{
  return 255;
}

static int get_kb_widget_first (int kb, int type) 
{
  return 0;
}

static int get_kb_widget_type (int kb, int num, TCHAR *name, uae_u32 *code) 
{
  if(code)
    *code = num;
  return IDEV_WIDGET_KEY;
}

static int get_kb_flags (int num) 
{
  return 0;
}

struct inputdevice_functions inputdevicefunc_keyboard = {
	init_kb, close_kb, acquire_kb, unacquire_kb, read_kb,
	get_kb_num, get_kb_friendlyname, get_kb_uniquename,
	get_kb_widget_num, get_kb_widget_type,
	get_kb_widget_first,
	get_kb_flags
};

int input_get_default_keyboard (int num) 
{
  if (num == 0) {
    return 1;
  }
  return 0;
}


#define MAX_JOY_BUTTONS	  16
#define MAX_JOY_AXES	    8
#define FIRST_JOY_AXIS	  0
#define FIRST_JOY_BUTTON	MAX_JOY_AXES

static int nr_joysticks = -1;
static char JoystickName[MAX_INPUT_DEVICES][80];

static char IsPS3Controller[MAX_INPUT_DEVICES];

static SDL_Joystick* Joysticktable[MAX_INPUT_DEVICES];


static int get_joystick_num (void)
{
	return nr_joysticks;
}

static int init_joystick (void)
{
  //This function is called too many times... we can filter if number of joy is good...
	if (nr_joysticks == SDL_NumJoysticks())
		return 1;

	nr_joysticks = SDL_NumJoysticks();
	if (nr_joysticks > MAX_INPUT_DEVICES)
		nr_joysticks = MAX_INPUT_DEVICES;
	for (int cpt = 0; cpt < nr_joysticks; cpt++)
	{
		Joysticktable[cpt] = SDL_JoystickOpen(cpt);
		if(Joysticktable[cpt] != NULL) {
#ifdef USE_SDL2
		  if(SDL_JoystickNameForIndex(cpt) != NULL)
    		strncpy(JoystickName[cpt], SDL_JoystickNameForIndex(cpt), 80 - 1);
#else
		  if(SDL_JoystickName(cpt) != NULL)
    		strncpy(JoystickName[cpt], SDL_JoystickName(cpt), 80 - 1);
#endif
      else
        sprintf(JoystickName[cpt], "Joystick%d", cpt);
  
  		if (strcmp(JoystickName[cpt], "Sony PLAYSTATION(R)3 Controller") == 0 || strcmp(JoystickName[cpt], "PLAYSTATION(R)3 Controller") == 0)
  			IsPS3Controller[cpt] = 1;
  		else
  			IsPS3Controller[cpt] = 0;
    }
	}

  return 1;
}

static void close_joystick (void)
{
	for (int cpt = 0; cpt < nr_joysticks; cpt++)
	{
		SDL_JoystickClose(Joysticktable[cpt]);
	}
  nr_joysticks = -1;
}

static int acquire_joystick (int num, int flags)
{
  if(num >= 0 && num < nr_joysticks)
    return 1;
  return 0;
}

static void unacquire_joystick (int num)
{
}

static TCHAR *get_joystick_friendlyname (int joy)
{
	return JoystickName[joy];
}

static TCHAR *get_joystick_uniquename (int joy)
{
	if (joy == 0)
		return "JOY0";
	if (joy == 1)
		return "JOY1";
	if (joy == 2)
		return "JOY2";
	if (joy == 3)
		return "JOY3";
	if (joy == 4)
		return "JOY4";
	if (joy == 5)
		return "JOY5";
	if (joy == 6)
		return "JOY6";

	return "JOY7";
}

static int get_joystick_widget_num (int joy)
{
  if(joy >= 0 && joy < nr_joysticks) {
    return SDL_JoystickNumAxes(Joysticktable[joy]) + SDL_JoystickNumButtons(Joysticktable[joy]);
  }
  return 0;
}

static int get_joystick_widget_first (int joy, int type)
{
  if(joy >= 0 && joy < nr_joysticks) {
    switch (type) {
    	case IDEV_WIDGET_BUTTON:
  	    return FIRST_JOY_BUTTON;
  	  case IDEV_WIDGET_AXIS:
  	    return FIRST_JOY_AXIS;
    	case IDEV_WIDGET_BUTTONAXIS:
    	  return MAX_JOY_AXES + MAX_JOY_BUTTONS; 
    }
  }
  return -1;
}

static int get_joystick_widget_type (int joy, int num, TCHAR *name, uae_u32 *code)
{
  if(joy >= 0 && joy < nr_joysticks) {
    if (num >= MAX_JOY_AXES && num < MAX_JOY_AXES + MAX_JOY_BUTTONS) {
    	if (name) {
  	    switch(num)
  	    {
  	      case FIRST_JOY_BUTTON:
            sprintf (name, "Button X/CD32 red");
  	        break;
  	      case FIRST_JOY_BUTTON + 1:
  	        sprintf (name, "Button B/CD32 blue");
  	        break;
  	      case FIRST_JOY_BUTTON + 2:
  	        sprintf (name, "Button A/CD32 green");
  	        break;
  	      case FIRST_JOY_BUTTON + 3:
  	        sprintf (name, "Button Y/CD32 yellow");
  	        break;
  	      case FIRST_JOY_BUTTON + 4:
  	        sprintf (name, "CD32 start");
  	        break;
  	      case FIRST_JOY_BUTTON + 5:
  	        sprintf (name, "CD32 ffw");
  	        break;
  	      case FIRST_JOY_BUTTON + 6:
  	        sprintf (name, "CD32 rwd");
  	        break;
  	    }
  	  }
  	  return IDEV_WIDGET_BUTTON;
    } else if (num < MAX_JOY_AXES) {
  	  if (name) {
  	    if(num == 0)
  	      sprintf (name, "X Axis");
  	    else if (num == 1)
  	      sprintf (name, "Y Axis");
  	    else
  	      sprintf (name, "Axis %d", num + 1);
  	  }
  	  return IDEV_WIDGET_AXIS;
    }
  }
  return IDEV_WIDGET_NONE;
}

static int get_joystick_flags (int num)
{
  return 0;
}


static void read_joystick (void)
{
	for (int joyid = 0; joyid < MAX_JPORTS; joyid++) {
		if (jsem_isjoy(joyid, &currprefs) != -1)
		{
			// Now we handle real SDL joystick...
			int hostjoyid = currprefs.jports[joyid].id - JSEM_JOYS;
			int hat = SDL_JoystickGetHat(Joysticktable[hostjoyid], 0);
			int val = SDL_JoystickGetAxis(Joysticktable[hostjoyid], 0);

			if (hat & SDL_HAT_RIGHT)
				setjoystickstate(hostjoyid, 0, 32767, 32767);
			else 
			if (hat & SDL_HAT_LEFT)
				setjoystickstate(hostjoyid, 0, -32767, 32767);
			else
				setjoystickstate(hostjoyid, 0, val, 32767);
			val = SDL_JoystickGetAxis(Joysticktable[hostjoyid], 1);
			if (hat & SDL_HAT_UP)
				setjoystickstate(hostjoyid, 1, -32767, 32767);
			else
			if (hat & SDL_HAT_DOWN) 
				setjoystickstate(hostjoyid, 1, 32767, 32767);
			else
				setjoystickstate(hostjoyid, 1, val, 32767);
                        
      // cd32 red, blue, green, yellow
			setjoybuttonstate(hostjoyid, 0, (SDL_JoystickGetButton(Joysticktable[hostjoyid], 0) & 1));
			setjoybuttonstate(hostjoyid, 1, (SDL_JoystickGetButton(Joysticktable[hostjoyid], 1) & 1));
			setjoybuttonstate(hostjoyid, 2, (SDL_JoystickGetButton(Joysticktable[hostjoyid], 2) & 1));
			setjoybuttonstate(hostjoyid, 3, (SDL_JoystickGetButton(Joysticktable[hostjoyid], 3) & 1));

			// cd32 start, ffw, rwd
			setjoybuttonstate(hostjoyid, 4, (SDL_JoystickGetButton(Joysticktable[hostjoyid], 4) & 1));
			setjoybuttonstate(hostjoyid, 5, (SDL_JoystickGetButton(Joysticktable[hostjoyid], 5) & 1));
			setjoybuttonstate(hostjoyid, 6, (SDL_JoystickGetButton(Joysticktable[hostjoyid], 6) & 1));

			if (IsPS3Controller[hostjoyid])
			{
        // cd32 red, blue, green, yellow
				setjoybuttonstate(hostjoyid, 0, (SDL_JoystickGetButton(Joysticktable[hostjoyid], 14) & 1)); // south
        setjoybuttonstate(hostjoyid, 1, (SDL_JoystickGetButton(Joysticktable[hostjoyid], 13) & 1)); // east
        setjoybuttonstate(hostjoyid, 2, (SDL_JoystickGetButton(Joysticktable[hostjoyid], 15) & 1)); // west
        setjoybuttonstate(hostjoyid, 3, (SDL_JoystickGetButton(Joysticktable[hostjoyid], 12) & 1)); // north
 
        // cd32  rwd, ffw, start
        setjoybuttonstate(hostjoyid, 4, (SDL_JoystickGetButton(Joysticktable[hostjoyid], 10) & 1)); // left shoulder
        setjoybuttonstate(hostjoyid, 5, (SDL_JoystickGetButton(Joysticktable[hostjoyid], 11) & 1)); // right shoulder
        setjoybuttonstate(hostjoyid, 6, (SDL_JoystickGetButton(Joysticktable[hostjoyid], 3) & 1));  // start
 
        // mouse left and 'space'
        setjoybuttonstate(hostjoyid, 7, (SDL_JoystickGetButton(Joysticktable[hostjoyid], 8) & 1)); // left trigger
        setjoybuttonstate(hostjoyid, 8, (SDL_JoystickGetButton(Joysticktable[hostjoyid], 9) & 1)); // right trigger
                                
				// Simulate a top with button 4
				if (SDL_JoystickGetButton(Joysticktable[hostjoyid], 4))
					setjoystickstate(hostjoyid, 1, -32767, 32767);
				// Simulate a right with button 5
				if (SDL_JoystickGetButton(Joysticktable[hostjoyid], 5))
					setjoystickstate(hostjoyid, 0, 32767, 32767);
				// Simulate a bottom with button 6
				if (SDL_JoystickGetButton(Joysticktable[hostjoyid], 6))
					setjoystickstate(hostjoyid, 1, 32767, 32767);
				// Simulate a left with button 7
				if (SDL_JoystickGetButton(Joysticktable[hostjoyid], 7))
					setjoystickstate(hostjoyid, 0, -32767, 32767);
			}
		}
  }
}

struct inputdevice_functions inputdevicefunc_joystick = {
	init_joystick, close_joystick, acquire_joystick, unacquire_joystick,
	read_joystick, get_joystick_num, get_joystick_friendlyname, get_joystick_uniquename,
	get_joystick_widget_num, get_joystick_widget_type,
	get_joystick_widget_first,
	get_joystick_flags
};

int input_get_default_joystick (struct uae_input_device *uid, int num, int port, int af, int mode, bool gp, bool joymouseswap)
{
  int h, v;

  h = port ? INPUTEVENT_JOY2_HORIZ : INPUTEVENT_JOY1_HORIZ;
  v = port ? INPUTEVENT_JOY2_VERT : INPUTEVENT_JOY1_VERT;

  setid (uid, num, ID_AXIS_OFFSET + 0, 0, port, h, gp);
  setid (uid, num, ID_AXIS_OFFSET + 1, 0, port, v, gp);

  setid_af (uid, num, ID_BUTTON_OFFSET + 0, 0, port, port ? INPUTEVENT_JOY2_FIRE_BUTTON : INPUTEVENT_JOY1_FIRE_BUTTON, af, gp);
  setid (uid, num, ID_BUTTON_OFFSET + 1, 0, port, port ? INPUTEVENT_JOY2_2ND_BUTTON : INPUTEVENT_JOY1_2ND_BUTTON, gp);
  setid (uid, num, ID_BUTTON_OFFSET + 2, 0, port, port ? INPUTEVENT_JOY2_3RD_BUTTON : INPUTEVENT_JOY1_3RD_BUTTON, gp);

  if (mode == JSEM_MODE_JOYSTICK_CD32) {
    setid_af (uid, num, ID_BUTTON_OFFSET + 0, 0, port, port ? INPUTEVENT_JOY2_CD32_RED : INPUTEVENT_JOY1_CD32_RED, af, gp);
    setid (uid, num, ID_BUTTON_OFFSET + 1, 0, port, port ? INPUTEVENT_JOY2_CD32_BLUE : INPUTEVENT_JOY1_CD32_BLUE, gp);
    setid (uid, num, ID_BUTTON_OFFSET + 2, 0, port, port ? INPUTEVENT_JOY2_CD32_GREEN : INPUTEVENT_JOY1_CD32_GREEN, gp);
    setid (uid, num, ID_BUTTON_OFFSET + 3, 0, port, port ? INPUTEVENT_JOY2_CD32_YELLOW : INPUTEVENT_JOY1_CD32_YELLOW, gp);
    setid (uid, num, ID_BUTTON_OFFSET + 4, 0, port, port ? INPUTEVENT_JOY2_CD32_RWD : INPUTEVENT_JOY1_CD32_RWD, gp);
    setid (uid, num, ID_BUTTON_OFFSET + 5, 0, port, port ? INPUTEVENT_JOY2_CD32_FFW : INPUTEVENT_JOY1_CD32_FFW, gp);
    setid (uid, num, ID_BUTTON_OFFSET + 6, 0, port, port ? INPUTEVENT_JOY2_CD32_PLAY : INPUTEVENT_JOY1_CD32_PLAY, gp);

    // mouse left and 'space' events (Not real but very useful?)
    setid(uid, num, ID_BUTTON_OFFSET + 7, 0, port, port ? INPUTEVENT_JOY1_FIRE_BUTTON : INPUTEVENT_JOY2_FIRE_BUTTON, gp);
    setid(uid, num, ID_BUTTON_OFFSET + 8, 0, port, port ? INPUTEVENT_KEY_SPACE : INPUTEVENT_KEY_SPACE, gp);
  }
  if (num >= 0 && num < nr_joysticks) {
    return 1;
  }
  return 0;
}

int input_get_default_joystick_analog (struct uae_input_device *uid, int num, int port, int af, bool joymouseswap) 
{
  return 0;
}
