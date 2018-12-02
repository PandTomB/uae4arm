#include "sysconfig.h"
#include "sysdeps.h"
#include "config.h"
#include "uae.h"
#include "options.h"
#include "include/memory.h"
#include "inputdevice.h"
#include "keyboard.h"
#include "rtgmodes.h"
#include "savestate.h"
#include "gfxboard.h"

extern bool input_initialize_alldevices (void);

static int delayed_mousebutton = 0;
static int doStylusRightClick = 0;

int defaultCpuSpeed = 600;
static bool cpuSpeedChanged = false;
static int lastCpuSpeed = 600;


static int getDefaultCpuSpeed(void)
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


static void setCpuSpeed()
{
	char speedCmd[128];

  currprefs.pandora_cpu_speed = changed_prefs.pandora_cpu_speed;

	if(currprefs.pandora_cpu_speed != lastCpuSpeed)
	{
		snprintf((char*)speedCmd, 127, "unset DISPLAY; echo y | sudo -n /usr/pandora/scripts/op_cpuspeed.sh %d", currprefs.pandora_cpu_speed);
		system(speedCmd);
		lastCpuSpeed = currprefs.pandora_cpu_speed;
		cpuSpeedChanged = true;
	}
}


static void resetCpuSpeed(void)
{
  if(cpuSpeedChanged)
  {
    lastCpuSpeed = defaultCpuSpeed - 10;
    currprefs.pandora_cpu_speed = changed_prefs.pandora_cpu_speed = defaultCpuSpeed;
    setCpuSpeed();
  }
}


void target_default_options (struct uae_prefs *p, int type)
{
  p->pandora_vertical_offset = OFFSET_Y_ADJUST;
  p->pandora_cpu_speed = defaultCpuSpeed;
  p->pandora_hide_idle_led = 0;
  
  p->pandora_tapDelay = 10;

	p->picasso96_modeflags = RGBFF_CLUT | RGBFF_R5G6B5 | RGBFF_R8G8B8A8;
	
	p->cr[0].index = 0;
	p->cr[0].horiz = -1;
	p->cr[0].vert = -1;
	p->cr[0].lace = -1;
	p->cr[0].resolution = 0;
	p->cr[0].rate = 60.0;
	p->cr[0].ntsc = 1;
	p->cr[0].rtg = true;
	p->cr[0].inuse = true;
	_tcscpy (p->cr[0].label, _T("RTG"));
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
  p->gfx_resolution = p->gfx_monitor.gfx_size.width > 600 ? 1 : 0;
  
  if(p->cachesize > 0)
    p->fpu_no_unimplemented = 0;
  else
    p->fpu_no_unimplemented = 1;
  
  fix_apmodes(p);

	setCpuSpeed();
}


void target_save_options (struct zfile *f, struct uae_prefs *p)
{
  cfgfile_write (f, "pandora.cpu_speed", "%d", p->pandora_cpu_speed);
  cfgfile_write (f, "pandora.hide_idle_led", "%d", p->pandora_hide_idle_led);
  cfgfile_write (f, "pandora.tap_delay", "%d", p->pandora_tapDelay);
  cfgfile_write (f, "pandora.move_y", "%d", p->pandora_vertical_offset - OFFSET_Y_ADJUST);
}


int target_parse_option (struct uae_prefs *p, const char *option, const char *value)
{
  int result = (cfgfile_intval (option, value, "cpu_speed", &p->pandora_cpu_speed, 1)
    || cfgfile_intval (option, value, "hide_idle_led", &p->pandora_hide_idle_led, 1)
    || cfgfile_intval (option, value, "tap_delay", &p->pandora_tapDelay, 1)
    );
  if(!result) {
    result = cfgfile_intval (option, value, "move_y", &p->pandora_vertical_offset, 1);
    if(result)
      p->pandora_vertical_offset += OFFSET_Y_ADJUST;
  }

  return result;
}


static void moveVertical(int value)
{
	changed_prefs.pandora_vertical_offset += value;
	if(changed_prefs.pandora_vertical_offset < -16 + OFFSET_Y_ADJUST)
		changed_prefs.pandora_vertical_offset = -16 + OFFSET_Y_ADJUST;
	else if(changed_prefs.pandora_vertical_offset > 16 + OFFSET_Y_ADJUST)
		changed_prefs.pandora_vertical_offset = 16 + OFFSET_Y_ADJUST;
}


static bool handle_internal_functions(int sdlkeycode, int sdlmodifier)
{
  int i;
  
  if (sdlkeycode == SDLK_LCTRL) { // Select key
    inputdevice_add_inputcode (AKS_ENTERGUI, 1, NULL);
    return true;
  }

  if(sdlmodifier == (KMOD_RCTRL | KMOD_RSHIFT)) {
    // Left and right shoulder
    switch(sdlkeycode) {
      case SDLK_UP:  // Left and right shoulder + UP -> move display up
        moveVertical(1);
        return true;
  				        
      case SDLK_DOWN:  // Left and right shoulder + DOWN -> move display down
        moveVertical(-1);
        return true;
  				        
      case SDLK_1:
      case SDLK_2:
      case SDLK_3:
      case SDLK_4:
      case SDLK_5:
      case SDLK_6:
        // Change display height direct
        changed_prefs.gfx_monitor.gfx_size.height = amigaheight_values[sdlkeycode - SDLK_1];
        update_display(&changed_prefs);
        return true;

      case SDLK_9:  // Select next lower display height
  			for(i=0; i<AMIGAHEIGHT_COUNT; ++i) {
  			  if(currprefs.gfx_monitor.gfx_size.height == amigaheight_values[i]) {
  		      if(i > 0)
  		        changed_prefs.gfx_monitor.gfx_size.height = amigaheight_values[i - 1];
  		      else
  		        changed_prefs.gfx_monitor.gfx_size.height = amigaheight_values[AMIGAHEIGHT_COUNT - 1];
  		      break;
  		    }
  			}
  			update_display(&changed_prefs);
        return true;

      case SDLK_0:  // Select next higher display height
  			for(i=0; i<AMIGAHEIGHT_COUNT; ++i) {
  			  if(currprefs.gfx_monitor.gfx_size.height == amigaheight_values[i]) {
  		      if(i < AMIGAHEIGHT_COUNT - 1)
  		        changed_prefs.gfx_monitor.gfx_size.height = amigaheight_values[i + 1];
  		      else
  		        changed_prefs.gfx_monitor.gfx_size.height = amigaheight_values[0];
  		      break;
  		    }
  			}
  			update_display(&changed_prefs);
        return true;

      case SDLK_w:  // Select next display width
  			for(i=0; i<AMIGAWIDTH_COUNT; ++i) {
  			  if(currprefs.gfx_monitor.gfx_size.width == amigawidth_values[i]) {
  		      if(i < AMIGAWIDTH_COUNT - 1)
  		        changed_prefs.gfx_monitor.gfx_size.width = amigawidth_values[i + 1];
  		      else
  		        changed_prefs.gfx_monitor.gfx_size.width = amigawidth_values[0];
  		      break;
  		    }
  			}
  			update_display(&changed_prefs);
        return true;

      case SDLK_r:
  		  // Toggle resolution (lores/hires)
  		  if(currprefs.gfx_monitor.gfx_size.width > 600)
  		    changed_prefs.gfx_monitor.gfx_size.width = currprefs.gfx_monitor.gfx_size.width / 2;
  		  else
  		    changed_prefs.gfx_monitor.gfx_size.width = currprefs.gfx_monitor.gfx_size.width * 2;
  			update_display(&changed_prefs);
        return true;
    }
  }

  if(sdlmodifier == KMOD_RSHIFT)
  {
    // Left shoulder
    switch(sdlmodifier) {
      case SDLK_d:  // Left shoulder + d -> toggle "Status line"
        changed_prefs.leds_on_screen = !changed_prefs.leds_on_screen;
        return true;
        
      case SDLK_f:  // Left shoulder + f -> toggle frameskip
        changed_prefs.gfx_framerate ? changed_prefs.gfx_framerate = 0 : changed_prefs.gfx_framerate = 1;
        set_config_changed();
        return true;
        
      case SDLK_s:  // Left shoulder + s -> store savestate
        savestate_initsave(savestate_fname, 2, 0, false);
  			save_state (savestate_fname, "...");
        savestate_state = STATE_DOSAVE;
        return true;
        
      case SDLK_l:  // Left shoulder + l -> load savestate
      	{
      		FILE *f=fopen(savestate_fname, "rb");
      		if(f)
      		{
      			fclose(f);
      			savestate_state = STATE_DORESTORE;
      		}
      	}
        return true;

#ifdef ACTION_REPLAY
      case SDLK_a:  // Left shoulder + a -> activate cardridge (Action Replay)
	      if(currprefs.cartfile[0] != '\0') {
		      inputdevice_add_inputcode (AKS_FREEZEBUTTON, 1, NULL);
          return true;
	      }
      	break;
#endif
    }
  }

  return false;
}


// This functions returns the Amiga key code (AK_...) and maybe changes modifier
static int translate_pandora_keys(int sdlkeycode, int *sdlmodifier)
{
  if (*sdlmodifier == KMOD_RCTRL) {
    // Right shoulder button pressed
    switch(sdlkeycode) {
      // Right shoulder + dPad -> cursor keys
      case SDLK_UP:
        *sdlmodifier = KMOD_NONE;
        return AK_UP;
      case SDLK_DOWN:
        *sdlmodifier = KMOD_NONE;
        return AK_DN;
      case SDLK_LEFT:
        *sdlmodifier = KMOD_NONE;
        return AK_LF;
      case SDLK_RIGHT:
        *sdlmodifier = KMOD_NONE;
        return AK_RT;

      case SDLK_HOME: // Right shoulder + button A -> CTRL
        *sdlmodifier = KMOD_NONE;
        return AK_CTRL;

      case SDLK_END:  // Right shoulder + button B -> left ALT
        *sdlmodifier = KMOD_NONE;
        return AK_LALT;

      case SDLK_PAGEDOWN: // Right shoulder + button X -> HELP
        *sdlmodifier = KMOD_NONE;
        return AK_HELP;
    }
  }

  if(*sdlmodifier == KMOD_LSHIFT) {
    // Shift button pressed
    switch(sdlkeycode) {
      case SDLK_2:              // '{'
        *sdlmodifier = KMOD_SHIFT;
        return AK_LBRACKET;
      
      case SDLK_3:              // '}'
        *sdlmodifier = KMOD_SHIFT;
        return AK_RBRACKET;
      
      case SDLK_4:              // '~'
        *sdlmodifier = KMOD_SHIFT;
        return AK_BACKQUOTE;

      case SDLK_9:              // '['
        *sdlmodifier = KMOD_NONE;
        return AK_LBRACKET;

      case SDLK_0:              // ']'
        *sdlmodifier = KMOD_NONE;
        return AK_RBRACKET;
    }
  }
  
  switch(sdlkeycode) {
    case SDLK_PAGEUP: // button Y -> Space
      *sdlmodifier = KMOD_NONE;
      return AK_SPC;

    case SDLK_EXCLAIM:
      *sdlmodifier = KMOD_SHIFT;
      return AK_1;

    case SDLK_QUOTEDBL:
      *sdlmodifier = KMOD_SHIFT;
      return AK_QUOTE;

    case SDLK_HASH:
      *sdlmodifier = KMOD_SHIFT;
      return AK_3;

    case SDLK_DOLLAR:
      *sdlmodifier = KMOD_SHIFT;
      return AK_4;

    case SDLK_AMPERSAND:
      *sdlmodifier = KMOD_SHIFT;
      return AK_7;

    case SDLK_LEFTPAREN:
      *sdlmodifier = KMOD_SHIFT;
      return AK_9;

    case SDLK_RIGHTPAREN:
      *sdlmodifier = KMOD_SHIFT;
      return AK_0;

    case SDLK_ASTERISK:
      *sdlmodifier = KMOD_SHIFT;
      return AK_8;

    case SDLK_PLUS:
      *sdlmodifier = KMOD_SHIFT;
      return AK_EQUAL;

    case SDLK_COLON:
      *sdlmodifier = KMOD_SHIFT;
      return AK_SEMICOLON;

    case SDLK_QUESTION:
      *sdlmodifier = KMOD_SHIFT;
      return AK_SLASH;

    case SDLK_AT:
      *sdlmodifier = KMOD_SHIFT;
      return AK_2;

    case SDLK_CARET:
      *sdlmodifier = KMOD_SHIFT;
      return AK_6;

    case SDLK_UNDERSCORE:
      *sdlmodifier = KMOD_SHIFT;
      return AK_MINUS;
          
    case 124: // code for '|'
      *sdlmodifier = KMOD_SHIFT;
      return AK_BACKSLASH;
  }  

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
  int sdlmouse = get_sdlmouse();
  int sdlkbd = get_sdlkbd();

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
  		  if (handle_internal_functions(rEvent.key.keysym.sym, rEvent.key.keysym.mod))
  		    continue;
        
  		  if (sdlkbd < 0)
  		    continue;
  		  
  		  switch(rEvent.key.keysym.sym)
  		  {
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
  				  modifier = rEvent.key.keysym.mod;
  				  keycode = translate_pandora_keys(rEvent.key.keysym.sym, &modifier);
  				  if(keycode) {
				      if(modifier == KMOD_SHIFT)
  				      inputdevice_do_keyboard(AK_LSH, 1);
  				    else
  				      inputdevice_do_keyboard(AK_LSH, 0);
				      inputdevice_do_keyboard(keycode, 1);
  				  } else {
			        inputdevice_translatekeycode(sdlkbd, rEvent.key.keysym.sym, 1, false);
				    }
            				    				  
  				  break;
				}
        break;
        
  	  case SDL_KEYUP:
  		  if (sdlkbd < 0)
  		    continue;

  	    switch(rEvent.key.keysym.sym) {
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
  				  modifier = rEvent.key.keysym.mod;
  				  keycode = translate_pandora_keys(rEvent.key.keysym.sym, &modifier);
  				  if(keycode) {
				      inputdevice_do_keyboard(keycode, 0);
				      if(modifier == KMOD_SHIFT)
  				      inputdevice_do_keyboard(AK_LSH, 0);
            } else {
	  		      inputdevice_translatekeycode(sdlkbd, rEvent.key.keysym.sym, 0, true);
				    }
  				  break;
  	    }
  	    break;
  	    
  	  case SDL_MOUSEBUTTONDOWN:
        if(sdlmouse >= 0) {
    	    if(rEvent.button.button == SDL_BUTTON_LEFT) {
    	      if(currprefs.input_tablet > TABLET_OFF && !doStylusRightClick) {
    	        // Delay mousebutton, we need new position first...
    	        delayed_mousebutton = currprefs.pandora_tapDelay << 1;
    	      } else {
    	        setmousebuttonstate (sdlmouse, doStylusRightClick, 1);
      	    }
    	    }
    	    else if(rEvent.button.button == SDL_BUTTON_RIGHT)
    	      setmousebuttonstate (sdlmouse, 1, 1);
        }
  	    break;

  	  case SDL_MOUSEBUTTONUP:
        if(sdlmouse >= 0) {
    	    if(rEvent.button.button == SDL_BUTTON_LEFT) {
  	        setmousebuttonstate (sdlmouse, doStylusRightClick, 0);
          }
    	    else if(rEvent.button.button == SDL_BUTTON_RIGHT)
    	      setmousebuttonstate (sdlmouse, 1, 0);
        }
  	    break;
  	    
  		case SDL_MOUSEMOTION:
  		  if(currprefs.input_tablet == TABLET_OFF) {
          if(sdlmouse >= 0) {
  			    int x, y;
    		    int mouseScale = currprefs.input_joymouse_multiplier / 2;
            x = rEvent.motion.xrel;
    				y = rEvent.motion.yrel;

    				if(rEvent.motion.x == 0 && x > -4)
    					x = -4;
    				if(rEvent.motion.y == 0 && y > -4)
    					y = -4;
    				if(rEvent.motion.x == currprefs.gfx_monitor.gfx_size.width - 1 && x < 4)
    					x = 4;
    				if(rEvent.motion.y == currprefs.gfx_monitor.gfx_size.height - 1 && y < 4)
    					y = 4;

  				  setmousestate(sdlmouse, 0, x * mouseScale, 0);
        	  setmousestate(sdlmouse, 1, y * mouseScale, 0);
          }
        }
        break;
		}
	}
	return got;
}


int main (int argc, char *argv[])
{
  defaultCpuSpeed = getDefaultCpuSpeed();
  
  if(!input_initialize_alldevices()) {
		printf("Failed to initialize input devices.\n");
		abort();
	};
  
  generic_main(argc, argv);
  
	resetCpuSpeed();
}
