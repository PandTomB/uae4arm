Read changelog.txt for the history of development of UAE4ARM for Pandora. 

This version is compiled and tested with Raspbian Stretch (32 bit) and Ubuntu Mate (64 bit).

Requirements:
 - guichan 0.8.1
 - libpng16
 - libSDL 1.2
 - libSDL_image
 - libSDL_ttf
 - libmpeg2
 - libmpg123
 - libxml2
 - libFLAC
 - capsimg from https://github.com/FrodeSolheim/capsimg

Install the libs to run emulator on Raspberry Pi:

      sudo apt-get install libsdl1.2debian
      sudo apt-get install libsdl-image1.2
      sudo apt-get install libsdl-gfx1.2
      sudo apt-get install libsdl-ttf2.0
      sudo apt-get install libmpg123
      sudo apt-get install libguichan-sdl
      sudo apt-get install libxml2
      sudo apt-get install mpeg2dec

Install the packages to compile uae4arm on Raspberry Pi:

      sudo apt-get install libsdl-dev
      sudo apt-get install libguichan-dev
      sudo apt-get install libsdl-ttf2.0-dev
      sudo apt-get install libsdl-gfx1.2-dev
      sudo apt-get install libxml2-dev
      sudo apt-get install libflac-dev
      sudo apt-get install libmpg123-dev
      sudo apt-get install libmpeg2-4-dev

Aarch64:
To run on Ubuntu mate, dispmanx is required:

      git clone https://github.com/raspberrypi/userland.git
      cd userland
      buildme --aarch64

Use dsp as audio driver:

      export SDL_AUDIODRIVER=dsp


Credits
 - Toni Wilen (WinUAE)
 - Frode Solheim (FS-UAE)
 - Chui (first version of uae4all)
 - Pickle (initial Pandora port of uae4all)
 - AnotherGuest (uae4all 2.0, based on Symbian sources of uae4all)
 - john4p (maintainer of uae4all 2.0 for Pandora)
 - Lyubomyr Lisen (Android port, bug reports, new GUI for uae4all2)
 - Jens Heitmann (idea and first versions of JIT in ARAnyM project)
 - Chips (Raspberry Pi port of uae4arm)
 - Midwan (Amiberry)
 - There are some more people who added code, but it’s hard to get everyone…
 
