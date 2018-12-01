#ifdef USE_SDL2
#include <guisan.hpp>
#include <SDL_ttf.h>
#include <guisan/sdl.hpp>
#include <guisan/sdl/sdltruetypefont.hpp>
#else
#include <guichan.hpp>
#include <SDL/SDL_ttf.h>
#include <guichan/sdl.hpp>
#include "sdltruetypefont.hpp"
#endif
#include "SelectorEntry.hpp"
#include "UaeDropDown.hpp"

#include "sysconfig.h"
#include "sysdeps.h"
#include "config.h"
#include "options.h"
#include "include/memory.h"
#include "uae.h"
#include "autoconf.h"
#include "filesys.h"
#include "gui.h"
#include "gui_handling.h"
#include "keyboard.h"
#include "inputdevice.h"
#include "GenericListModel.h"


static int total_devices;


static const char *mousespeed_list[] = { ".25", ".5", "1x", "2x", "4x" };
static const int mousespeed_values[] = { 2, 5, 10, 20, 40 };

static gcn::Label *lblPort0;
static gcn::Label *lblPort1;

static gcn::Label* lblMouseSpeed;
static gcn::Label* lblMouseSpeedInfo;
static gcn::Slider* sldMouseSpeed;
static gcn::Label* lblAutofireRate;
static gcn::Label* lblAutofireRateInfo;
static gcn::Slider* sldAutofireRate;
  
static gcn::Label *lblNumLock;
static gcn::UaeDropDown* cboKBDLed_num;
static gcn::Label *lblScrLock;
static gcn::UaeDropDown* cboKBDLed_scr;
static gcn::Label *lblCapLock;
static gcn::UaeDropDown* cboKBDLed_cap;

static const char *inputport_list[12] = { "<none>", "Keyboard Layout A", "Keyboard Layout B", "Keyboard Layout C", NULL };
static GenericListModel ctrlPortList;

static const char *inputmode_list[] = { "Default", "Mouse", "Joystick", "CD32 pad" };
static const int inputmode_val[] = { JSEM_MODE_DEFAULT, JSEM_MODE_MOUSE, JSEM_MODE_JOYSTICK, JSEM_MODE_JOYSTICK_CD32 };
static GenericListModel ctrlPortModeList(inputmode_list, 4);

const char *autofireValues[] = { "No autofire", "Autofire", "Autofire (toggle)", "Autofire (always)" };
static GenericListModel autofireList(autofireValues, 4);

static gcn::UaeDropDown* cboPorts[4] = { NULL, NULL, NULL, NULL };
static gcn::UaeDropDown* cboPortModes[4] = { NULL, NULL, NULL, NULL };
static gcn::UaeDropDown* cboAutofires[4] = { NULL, NULL, NULL, NULL };


static const char *listValues[] = { "none", "POWER", "DF0", "DF1", "DF2", "DF3", "DF*", "HD", "CD" };
static GenericListModel KBDLedList(listValues, 9);

static const int ControlKey_SDLKeyValues[] = { 0, SDLK_F11, SDLK_F12 };
static const char *ControlKeyValues[] = { "------------------", "F11", "F12" };
static GenericListModel ControlKeyList(ControlKeyValues, 3);

static int GetControlKeyIndex(int key)
{
	int ControlKey_SDLKeyValues_Length = sizeof(ControlKey_SDLKeyValues) / sizeof(int);
	for (int i = 0; i < (ControlKey_SDLKeyValues_Length + 1); ++i) {
		if (ControlKey_SDLKeyValues[i] == key)
			return i;
	}
	return 0; // Default: no key
}

static const int ControlButton_SDLButtonValues[] = { -1, 0, 1, 2, 3 };
static const char *ControlButtonValues[] = { "------------------", "JoyButton0", "JoyButton1", "JoyButton2", "JoyButton3" };
static GenericListModel ControlButtonList(ControlButtonValues, 5);

static int GetControlButtonIndex(int button)
{
	int ControlButton_SDLButtonValues_Length = sizeof(ControlButton_SDLButtonValues) / sizeof(int);
	for (int i = 0; i < (ControlButton_SDLButtonValues_Length + 1); ++i) {
		if (ControlButton_SDLButtonValues[i] == button)
			return i;
	}
	return 0; // Default: no key
}


static void RefreshPanelGamePort(void)
{
  int i, idx;
  TCHAR tmp[100];

  // Set current device in port 0
  idx = inputdevice_getjoyportdevice (0, workprefs.jports[0].id);
	if (idx >= 0)
		idx += 1;
	else
    idx = 0;
	if (idx >= total_devices)
		idx = 0;
  cboPorts[0]->setSelected(idx); 

  for(i = 0; i < 4; ++i) {
    if(workprefs.jports[0].mode == inputmode_val[i]) {
      cboPortModes[0]->setSelected(i);
      break;
    }
  }
  
  // Set current device in port 1
  idx = inputdevice_getjoyportdevice (1, workprefs.jports[1].id);
	if (idx >= 0)
		idx += 1;
	else
    idx = 0;
	if (idx >= total_devices)
		idx = 0;
  cboPorts[1]->setSelected(idx); 

  for(i = 0; i < 4; ++i) {
    if(workprefs.jports[1].mode == inputmode_val[i]) {
      cboPortModes[1]->setSelected(i);
      break;
    }
  }

  cboAutofires[0]->setSelected(workprefs.jports[0].autofire);
  cboAutofires[1]->setSelected(workprefs.jports[1].autofire);

  for(i = 0; i < 5; ++i) {
    if(workprefs.input_joymouse_multiplier == mousespeed_values[i]) {
      sldMouseSpeed->setValue(i);
      lblMouseSpeedInfo->setCaption(mousespeed_list[i]);
      break;
    }
  }

  sldAutofireRate->setValue(workprefs.input_autofire_linecnt / 100);
  _stprintf (tmp, _T("%d lines"), workprefs.input_autofire_linecnt);
  lblAutofireRateInfo->setCaption(tmp);

	cboKBDLed_num->setSelected(workprefs.kbd_led_num);
	cboKBDLed_scr->setSelected(workprefs.kbd_led_scr);
}


static void values_to_dialog(void)
{
	inputdevice_updateconfig (NULL, &workprefs);

  RefreshPanelGamePort();
}


static void values_from_dialog(int changedport, bool reset)
{
	int i;
	int changed = 0;
	int prevport;

  if(reset)
    inputdevice_compa_clear (&workprefs, changedport);

	for (i = 0; i < MAX_JPORTS; i++) {
	  if (i >= 2)
	    continue; // parallel port not yet available
	  
		int prevport = workprefs.jports[i].id;
		int max = JSEM_LASTKBD + inputdevice_get_device_total (IDTYPE_JOYSTICK);
		if (i < 2)
			max += inputdevice_get_device_total (IDTYPE_MOUSE);
    int id = cboPorts[i]->getSelected() - 1;
		if (id < 0) {
			workprefs.jports[i].id = JPORT_NONE;
		} else if (id >= max) {
			workprefs.jports[i].id = JPORT_NONE;
		} else if (id < JSEM_LASTKBD) {
			workprefs.jports[i].id = JSEM_KBDLAYOUT + id;
		} else if (id >= JSEM_LASTKBD + inputdevice_get_device_total (IDTYPE_JOYSTICK)) {
			workprefs.jports[i].id = JSEM_MICE + id - inputdevice_get_device_total (IDTYPE_JOYSTICK) - JSEM_LASTKBD;
		} else {
			workprefs.jports[i].id = JSEM_JOYS + id - JSEM_LASTKBD;
		}

		if(cboPortModes[i] != NULL)
		  workprefs.jports[i].mode = inputmode_val[cboPortModes[i]->getSelected()];
		if(cboAutofires[i] != NULL)
		  workprefs.jports[i].autofire = cboAutofires[i]->getSelected();

		if (workprefs.jports[i].id != prevport)
			changed = 1;
	}

  if (changed)
    inputdevice_validate_jports (&workprefs, changedport, NULL);        

	RefreshPanelGamePort();

  inputdevice_config_change();
  if(reset)
    inputdevice_forget_unplugged_device(changedport);
}


class GamePortActionListener : public gcn::ActionListener
{
  public:
    void action(const gcn::ActionEvent& actionEvent)
    {
      if (actionEvent.getSource() == cboPorts[0] || actionEvent.getSource() == cboPortModes[0]) {
        values_from_dialog(0, true);
      }
      
      else if (actionEvent.getSource() == cboPorts[1] || actionEvent.getSource() == cboPortModes[1]) {
        values_from_dialog(1, true);
      }
      
      else if (actionEvent.getSource() == cboAutofires[0]) {
        values_from_dialog(0, false);
      }
      
      else if (actionEvent.getSource() == cboAutofires[1]) {
        values_from_dialog(1, false);
      }
      
 	    else if (actionEvent.getSource() == sldMouseSpeed) {
    		workprefs.input_joymouse_multiplier = mousespeed_values[(int)(sldMouseSpeed->getValue())];
    		RefreshPanelGamePort();
    	}

 	    else if (actionEvent.getSource() == sldAutofireRate) {
    		workprefs.input_autofire_linecnt = (int)(sldAutofireRate->getValue()) * 100;
    		RefreshPanelGamePort();
    	}

      else if (actionEvent.getSource() == cboKBDLed_num)
        workprefs.kbd_led_num = cboKBDLed_num->getSelected();

      else if (actionEvent.getSource() == cboKBDLed_scr)
  			workprefs.kbd_led_scr = cboKBDLed_scr->getSelected();
    }
};
static GamePortActionListener* gameportActionListener;


void InitPanelGamePort(const struct _ConfigCategory& category)
{
  int j;

  total_devices = 1 + JSEM_LASTKBD;
	for (j = 0; j < inputdevice_get_device_total (IDTYPE_JOYSTICK) && total_devices < 12; j++, total_devices++)
	  inputport_list[total_devices] = inputdevice_get_device_name (IDTYPE_JOYSTICK, j);
	for (j = 0; j < inputdevice_get_device_total (IDTYPE_MOUSE) && total_devices < 12; j++, total_devices++)
		inputport_list[total_devices] = inputdevice_get_device_name (IDTYPE_MOUSE, j);

  ctrlPortList.clear();
  for(j = 0; j < total_devices; ++j)
    ctrlPortList.add(inputport_list[j]);
  
  gameportActionListener = new GamePortActionListener();

  lblPort0 = new gcn::Label("Port 1:");
  lblPort0->setSize(100, LABEL_HEIGHT);
  lblPort0->setAlignment(gcn::Graphics::RIGHT);
	cboPorts[0] = new gcn::UaeDropDown(&ctrlPortList);
  cboPorts[0]->setSize(160, DROPDOWN_HEIGHT);
  cboPorts[0]->setBaseColor(gui_baseCol);
  cboPorts[0]->setId("cboPort0");
  cboPorts[0]->addActionListener(gameportActionListener);

  lblPort1 = new gcn::Label("Port 2:");
  lblPort1->setSize(100, LABEL_HEIGHT);
  lblPort1->setAlignment(gcn::Graphics::RIGHT);
	cboPorts[1] = new gcn::UaeDropDown(&ctrlPortList);
  cboPorts[1]->setSize(160, DROPDOWN_HEIGHT);
  cboPorts[1]->setBaseColor(gui_baseCol);
  cboPorts[1]->setId("cboPort1");
  cboPorts[1]->addActionListener(gameportActionListener);

	cboPortModes[0] = new gcn::UaeDropDown(&ctrlPortModeList);
  cboPortModes[0]->setSize(80, DROPDOWN_HEIGHT);
  cboPortModes[0]->setBaseColor(gui_baseCol);
  cboPortModes[0]->setId("cboPortMode0");
  cboPortModes[0]->addActionListener(gameportActionListener);

	cboPortModes[1] = new gcn::UaeDropDown(&ctrlPortModeList);
  cboPortModes[1]->setSize(80, DROPDOWN_HEIGHT);
  cboPortModes[1]->setBaseColor(gui_baseCol);
  cboPortModes[1]->setId("cboPortMode1");
  cboPortModes[1]->addActionListener(gameportActionListener);

	cboAutofires[0] = new gcn::UaeDropDown(&autofireList);
  cboAutofires[0]->setSize(120, DROPDOWN_HEIGHT);
  cboAutofires[0]->setBaseColor(gui_baseCol);
  cboAutofires[0]->setId("cboAutofire0");
  cboAutofires[0]->addActionListener(gameportActionListener);

	cboAutofires[1] = new gcn::UaeDropDown(&autofireList);
  cboAutofires[1]->setSize(120, DROPDOWN_HEIGHT);
  cboAutofires[1]->setBaseColor(gui_baseCol);
  cboAutofires[1]->setId("cboAutofire1");
  cboAutofires[1]->addActionListener(gameportActionListener);

	lblMouseSpeed = new gcn::Label("Mouse Speed:");
  lblMouseSpeed->setSize(100, LABEL_HEIGHT);
  lblMouseSpeed->setAlignment(gcn::Graphics::RIGHT);
  sldMouseSpeed = new gcn::Slider(0, 4);
  sldMouseSpeed->setSize(110, SLIDER_HEIGHT);
  sldMouseSpeed->setBaseColor(gui_baseCol);
	sldMouseSpeed->setMarkerLength(20);
	sldMouseSpeed->setStepLength(1);
	sldMouseSpeed->setId("MouseSpeed");
  sldMouseSpeed->addActionListener(gameportActionListener);
  lblMouseSpeedInfo = new gcn::Label(".25");

	lblAutofireRate = new gcn::Label("Autofire rate:");
  lblAutofireRate->setSize(82, LABEL_HEIGHT);
  lblAutofireRate->setAlignment(gcn::Graphics::RIGHT);
  sldAutofireRate = new gcn::Slider(1, 40);
  sldAutofireRate->setSize(117, SLIDER_HEIGHT);
  sldAutofireRate->setBaseColor(gui_baseCol);
	sldAutofireRate->setMarkerLength(20);
	sldAutofireRate->setStepLength(1);
	sldAutofireRate->setId("AutofireRate");
  sldAutofireRate->addActionListener(gameportActionListener);
  lblAutofireRateInfo = new gcn::Label("XXXX lines.");

//  lblNumLock = new gcn::Label("NumLock LED:");
//  lblNumLock->setSize(100, LABEL_HEIGHT);
//  lblNumLock->setAlignment(gcn::Graphics::RIGHT);
//  cboKBDLed_num = new gcn::UaeDropDown(&KBDLedList);
//  cboKBDLed_num->setSize(150, DROPDOWN_HEIGHT);
//  cboKBDLed_num->setBaseColor(gui_baseCol);
//  cboKBDLed_num->setId("numlock");
//  cboKBDLed_num->addActionListener(gameportActionListener);

//  lblCapLock = new gcn::Label("CapsLock LED:");
//  lblCapLock->setSize(100, LABEL_HEIGHT);
//  lblCapLock->setAlignment(gcn::Graphics::RIGHT);
//  cboKBDLed_cap = new gcn::UaeDropDown(&KBDLedList);
//  cboKBDLed_cap->setSize(150, DROPDOWN_HEIGHT);
//  cboKBDLed_cap->setBaseColor(gui_baseCol);
//  cboKBDLed_cap->setId("capslock");
//  cboKBDLed_cap->addActionListener(gameportActionListener);

//  lblScrLock = new gcn::Label("ScrollLock LED:");
//  lblScrLock->setSize(100, LABEL_HEIGHT);
//  lblScrLock->setAlignment(gcn::Graphics::RIGHT);
//  cboKBDLed_scr = new gcn::UaeDropDown(&KBDLedList);
//  cboKBDLed_scr->setSize(150, DROPDOWN_HEIGHT);
//  cboKBDLed_scr->setBaseColor(gui_baseCol);
//  cboKBDLed_scr->setId("scrolllock");
//  cboKBDLed_scr->addActionListener(gameportActionListener);

  int posY = DISTANCE_BORDER;
  category.panel->add(lblPort0, DISTANCE_BORDER, posY);
  category.panel->add(cboPorts[0], DISTANCE_BORDER + lblPort0->getWidth() + 8, posY);
  category.panel->add(cboPortModes[0], 300, posY);
  category.panel->add(cboAutofires[0], 390, posY);
  posY += cboPorts[0]->getHeight() + DISTANCE_NEXT_Y;
  category.panel->add(lblPort1, DISTANCE_BORDER, posY);
  category.panel->add(cboPorts[1], DISTANCE_BORDER + lblPort1->getWidth() + 8, posY);
  category.panel->add(cboPortModes[1], 300, posY);
  category.panel->add(cboAutofires[1], 390, posY);
  posY += cboPorts[1]->getHeight() + DISTANCE_NEXT_Y;

  category.panel->add(lblMouseSpeed, DISTANCE_BORDER, posY);
  category.panel->add(sldMouseSpeed, DISTANCE_BORDER + lblMouseSpeed->getWidth() + 9, posY);
  category.panel->add(lblMouseSpeedInfo, sldMouseSpeed->getX() + sldMouseSpeed->getWidth() + 12, posY);
  category.panel->add(lblAutofireRate, 300, posY);
  category.panel->add(sldAutofireRate, 300 + lblAutofireRate->getWidth() + 9, posY);
  category.panel->add(lblAutofireRateInfo, sldAutofireRate->getX() + sldAutofireRate->getWidth() + 12, posY);
  posY += sldAutofireRate->getHeight() + DISTANCE_NEXT_Y;

//  category.panel->add(lblNumLock, DISTANCE_BORDER, posY);
//	category.panel->add(cboKBDLed_num, DISTANCE_BORDER + lblNumLock->getWidth() + 8, posY);
//  category.panel->add(lblCapLock, lblNumLock->getX() + lblNumLock->getWidth() + DISTANCE_NEXT_X, posY);
//  category.panel->add(cboKBDLed_cap, cboKBDLed_num->getX() + cboKBDLed_num->getWidth() + DISTANCE_NEXT_X, posY);
//	category.panel->add(lblScrLock, cboKBDLed_num->getX() + cboKBDLed_num->getWidth() + DISTANCE_NEXT_X, posY);
//	category.panel->add(cboKBDLed_scr, lblScrLock->getX() + lblScrLock->getWidth() + 8, posY);
//  posY += cboKBDLed_scr->getHeight() + DISTANCE_NEXT_Y;

  values_to_dialog();
}


void ExitPanelGamePort(const struct _ConfigCategory& category)
{
  category.panel->clear();
  
  delete lblPort0;
  delete cboPorts[0];
  delete lblPort1;
  delete cboPorts[1];
  
  delete cboPortModes[0];  
  delete cboPortModes[1];
  delete cboAutofires[0];
  delete cboAutofires[1];
  delete lblMouseSpeed;
  delete sldMouseSpeed;
  delete lblMouseSpeedInfo;
  delete lblAutofireRate;
  delete sldAutofireRate;
  delete lblAutofireRateInfo;

  delete lblCapLock;
  delete lblScrLock;
  delete lblNumLock;
  delete cboKBDLed_num;
  delete cboKBDLed_cap;
  delete cboKBDLed_scr;

  delete gameportActionListener;
}


bool HelpPanelGamePort(std::vector<std::string> &helptext)
{
  helptext.clear();
  helptext.push_back("You can select the control type for both ports and the rate for autofire.");
  helptext.push_back("");
  helptext.push_back("Set the emulated mouse speed to .25x, .5x, 1x, 2x and 4x to slow down or speed up the mouse.");
  return true;
}
