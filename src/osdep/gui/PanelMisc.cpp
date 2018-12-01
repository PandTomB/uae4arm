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
#include "UaeCheckBox.hpp"

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


static gcn::UaeCheckBox* chkStatusLine;
static gcn::UaeCheckBox* chkHideIdleLed;
static gcn::UaeCheckBox* chkShowGUI;
#ifdef PANDORA
static gcn::Label* lblPandoraSpeed;
static gcn::Label* lblPandoraSpeedInfo;
static gcn::Slider* sldPandoraSpeed;
#endif
static gcn::UaeCheckBox* chkBSDSocket;
static gcn::UaeCheckBox* chkMasterWP;


static void RefreshPanelMisc(void)
{
  chkStatusLine->setSelected(workprefs.leds_on_screen);
  chkHideIdleLed->setSelected(workprefs.pandora_hide_idle_led);
  chkShowGUI->setSelected(workprefs.start_gui);

#ifdef PANDORA
  TCHAR tmp[20];
  sldPandoraSpeed->setValue(workprefs.pandora_cpu_speed);
  snprintf(tmp, 20, "%d MHz", workprefs.pandora_cpu_speed);
  lblPandoraSpeedInfo->setCaption(tmp);
#endif
  
  chkBSDSocket->setSelected(workprefs.socket_emu);
  chkMasterWP->setSelected(workprefs.floppy_read_only);
}


class MiscActionListener : public gcn::ActionListener
{
  public:
    void action(const gcn::ActionEvent& actionEvent)
    {
      if (actionEvent.getSource() == chkStatusLine)
        workprefs.leds_on_screen = chkStatusLine->isSelected();
      
      else if (actionEvent.getSource() == chkHideIdleLed)
        workprefs.pandora_hide_idle_led = chkHideIdleLed->isSelected();

      else if (actionEvent.getSource() == chkShowGUI)
        workprefs.start_gui = chkShowGUI->isSelected();

      else if (actionEvent.getSource() == chkBSDSocket)
        workprefs.socket_emu = chkBSDSocket->isSelected();
        
      else if (actionEvent.getSource() == chkMasterWP) {
        workprefs.floppy_read_only = chkMasterWP->isSelected();
      }

#ifdef PANDORA
      else if (actionEvent.getSource() == sldPandoraSpeed)
      {
        int newspeed = (int) sldPandoraSpeed->getValue();
        newspeed = newspeed - (newspeed % 20);
        if(workprefs.pandora_cpu_speed != newspeed)
        {
          workprefs.pandora_cpu_speed = newspeed;
          RefreshPanelMisc();
        }
      }
#endif
    }
};
static MiscActionListener* miscActionListener;


void InitPanelMisc(const struct _ConfigCategory& category)
{
  miscActionListener = new MiscActionListener();

	chkStatusLine = new gcn::UaeCheckBox("Status Line");
	chkStatusLine->setId("StatusLine");
  chkStatusLine->addActionListener(miscActionListener);

	chkHideIdleLed = new gcn::UaeCheckBox("Hide idle led");
	chkHideIdleLed->setId("HideIdle");
  chkHideIdleLed->addActionListener(miscActionListener);

	chkShowGUI = new gcn::UaeCheckBox("Show GUI on startup");
	chkShowGUI->setId("ShowGUI");
  chkShowGUI->addActionListener(miscActionListener);

#ifdef PANDORA
	lblPandoraSpeed = new gcn::Label("Pandora Speed:");
  lblPandoraSpeed->setSize(110, LABEL_HEIGHT);
  lblPandoraSpeed->setAlignment(gcn::Graphics::RIGHT);
  sldPandoraSpeed = new gcn::Slider(500, 1260);
  sldPandoraSpeed->setSize(200, SLIDER_HEIGHT);
  sldPandoraSpeed->setBaseColor(gui_baseCol);
	sldPandoraSpeed->setMarkerLength(20);
	sldPandoraSpeed->setStepLength(20);
	sldPandoraSpeed->setId("PandSpeed");
  sldPandoraSpeed->addActionListener(miscActionListener);
  lblPandoraSpeedInfo = new gcn::Label("1000 MHz");
#endif

	chkBSDSocket = new gcn::UaeCheckBox("bsdsocket.library");
  chkBSDSocket->setId("BSDSocket");
  chkBSDSocket->addActionListener(miscActionListener);

	chkMasterWP = new gcn::UaeCheckBox("Master floppy write protection");
  chkMasterWP->setId("MasterWP");
  chkMasterWP->addActionListener(miscActionListener);
  
	int posY = DISTANCE_BORDER;
  category.panel->add(chkStatusLine, DISTANCE_BORDER, posY);
  posY += chkStatusLine->getHeight() + DISTANCE_NEXT_Y;
  category.panel->add(chkHideIdleLed, DISTANCE_BORDER, posY);
  posY += chkHideIdleLed->getHeight() + DISTANCE_NEXT_Y;
  category.panel->add(chkShowGUI, DISTANCE_BORDER, posY);
  posY += chkShowGUI->getHeight() + DISTANCE_NEXT_Y;
#ifdef PANDORA
  category.panel->add(lblPandoraSpeed, DISTANCE_BORDER, posY);
  category.panel->add(sldPandoraSpeed, DISTANCE_BORDER + lblPandoraSpeed->getWidth() + 8, posY);
  category.panel->add(lblPandoraSpeedInfo, sldPandoraSpeed->getX() + sldPandoraSpeed->getWidth() + 12, posY);
  posY += sldPandoraSpeed->getHeight() + DISTANCE_NEXT_Y;
#endif
  category.panel->add(chkBSDSocket, DISTANCE_BORDER, posY);
  posY += chkBSDSocket->getHeight() + DISTANCE_NEXT_Y;
  category.panel->add(chkMasterWP, DISTANCE_BORDER, posY);
  posY += chkMasterWP->getHeight() + DISTANCE_NEXT_Y;
  
  RefreshPanelMisc();
}


void ExitPanelMisc(const struct _ConfigCategory& category)
{
  category.panel->clear();
  
  delete chkStatusLine;
  delete chkHideIdleLed;
  delete chkShowGUI;
#ifdef PANDORA
  delete lblPandoraSpeed;
  delete sldPandoraSpeed;
  delete lblPandoraSpeedInfo;
#endif
  delete chkBSDSocket;
  delete chkMasterWP;

  delete miscActionListener;
}


bool HelpPanelMisc(std::vector<std::string> &helptext)
{
  helptext.clear();
  helptext.push_back("\"Status Line\" shows/hides the status line indicator. During emulation, you can show/hide this by pressing left");
  helptext.push_back("shoulder and 'd'. The first value in the status line shows the idle time of the Pandora CPU in %, the second value");
  helptext.push_back("is the current frame rate. When you have a HDD in your Amiga emulation, the HD indicator shows read (blue) and");
  helptext.push_back("write (red) access to the HDD. The next values are showing the track number for each disk drive and indicates");
  helptext.push_back("disk access.");
  helptext.push_back("");
  helptext.push_back("When you deactivate the option \"Show GUI on startup\" and use this configuration by specifying it with the");
  helptext.push_back("command line parameter \"-config=<file>\", the emulation starts directly without showing the GUI.");
  helptext.push_back("");
#ifdef PANDORA
  helptext.push_back("Set the speed for the Pandora CPU to overclock it for games which need more power. Be careful with this");
  helptext.push_back("parameter.");
  helptext.push_back("");
#endif	
  helptext.push_back("\"bsdsocket.library\" enables network functions (i.e. for web browsers in OS3.9).");
  helptext.push_back("");
  helptext.push_back("\"Master floppy drive protection\" will disable all write access to floppy disks.");
  return true;
}
