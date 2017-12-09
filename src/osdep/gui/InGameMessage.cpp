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

#include "sysconfig.h"
#include "sysdeps.h"
#include "config.h"
#include "options.h"
#include "uae.h"
#include "custom.h"
#include "xwin.h"
#include "drawing.h"
#include "gui.h"
#include "gui_handling.h"


extern SDL_Surface *prSDLScreen;
#ifdef USE_SDL2
extern SDL_Cursor* cursor;
extern void updatedisplayarea();
#endif


static int msg_done = 0;
static gcn::Gui* msg_gui;
static gcn::SDLGraphics* msg_graphics;
static gcn::SDLInput* msg_input;
#ifdef USE_SDL2
static gcn::SDLTrueTypeFont* msg_font;
#else
static gcn::contrib::SDLTrueTypeFont* msg_font;
#endif
static SDL_Event msg_event;

static gcn::Color msg_baseCol;
static gcn::Container* msg_top;
static gcn::Window *wndMsg;
static gcn::Button* cmdDone;
static gcn::TextBox* txtMsg;

static int msgWidth = 260;
static int msgHeight = 100;
int borderSize = 6;

class DoneActionListener : public gcn::ActionListener
{
  public:
    void action(const gcn::ActionEvent& actionEvent)
    {
      msg_done = 1;
    }
};
static DoneActionListener* doneActionListener;

void gui_halt()
{
  msg_top->remove(wndMsg);

  delete txtMsg;
  delete cmdDone;
  delete doneActionListener;
  delete wndMsg;
  
  delete msg_font;
  delete msg_top;
  
  delete msg_gui;
  delete msg_input;
  delete msg_graphics;
}

void checkInput()
{
  //-------------------------------------------------
  // Check user input
  //-------------------------------------------------
  while(SDL_PollEvent(&msg_event))
  {
    if (msg_event.type == SDL_KEYDOWN)
    {
      switch(msg_event.key.keysym.sym)
      {
			  case VK_X:
			  case VK_A:
        case SDLK_RETURN:
          msg_done = 1;
          break;
			  default: 
				  break;
      }
    }

    //-------------------------------------------------
    // Send event to guichan/guisan-controls
    //-------------------------------------------------
    msg_input->pushInput(msg_event);
  }
}

void gui_init(const char* msg)
{
  msg_graphics = new gcn::SDLGraphics();
  msg_graphics->setTarget(prSDLScreen);
  msg_input = new gcn::SDLInput();
  msg_gui = new gcn::Gui();
  msg_gui->setGraphics(msg_graphics);
  msg_gui->setInput(msg_input);
  
  msg_baseCol.r = 192;
  msg_baseCol.g = 192;
  msg_baseCol.b = 208;

  msg_top = new gcn::Container();
  msg_top->setDimension(gcn::Rectangle((prSDLScreen->w - msgWidth) / 2, (prSDLScreen->h - msgHeight) / 2, msgWidth, msgHeight));
  msg_top->setBaseColor(msg_baseCol);
  msg_gui->setTop(msg_top);

  TTF_Init();
#ifdef USE_SDL2
	msg_font = new gcn::SDLTrueTypeFont("data/FreeSans.ttf", 14);
#else
  msg_font = new gcn::contrib::SDLTrueTypeFont("data/FreeSans.ttf", 10);
#endif
  gcn::Widget::setGlobalFont(msg_font);

  doneActionListener = new DoneActionListener();
  
	wndMsg = new gcn::Window("Load");
	wndMsg->setSize(msgWidth, msgHeight);
  wndMsg->setPosition(0, 0);
  wndMsg->setBaseColor(msg_baseCol + 0x202020);
  wndMsg->setCaption("Information");
  wndMsg->setTitleBarHeight(12);

	cmdDone = new gcn::Button("Ok");
	cmdDone->setSize(40, 20);
  cmdDone->setBaseColor(msg_baseCol + 0x202020);
	cmdDone->setId("Done");
  cmdDone->addActionListener(doneActionListener);
  
  txtMsg = new gcn::TextBox(msg);
  txtMsg->setEnabled(false);
  txtMsg->setPosition(6, 6);
  txtMsg->setSize(wndMsg->getWidth() - 16, 46);
  txtMsg->setOpaque(false);
  
  wndMsg->add(txtMsg, 6, 6);
  wndMsg->add(cmdDone, (wndMsg->getWidth() - cmdDone->getWidth()) / 2, wndMsg->getHeight() - 38);

  msg_top->add(wndMsg);
  cmdDone->requestFocus();
}

void InGameMessage(const char *msg)
{
  halt_draw_frame();

	gui_init(msg);
  
  msg_done = 0;
  bool drawn = false;
  while(!msg_done)
  {
		// Poll input
		checkInput();

    // Now we let the Gui object perform its logic.
    msg_gui->logic();
    // Now we let the Gui object draw itself.
    msg_gui->draw();
    // Finally we update the screen.
#ifdef USE_SDL2
		if (!drawn && cursor != NULL)
		{
			SDL_ShowCursor(SDL_ENABLE);
			updatedisplayarea();
		}
#else
    if(!drawn)
      SDL_Flip(prSDLScreen);
#endif
    drawn = true;
  }

	gui_halt();
#ifdef USE_SDL2
	SDL_ShowCursor(SDL_DISABLE);
#endif
}
