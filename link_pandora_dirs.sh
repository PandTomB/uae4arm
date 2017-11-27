#!/bin/bash

cd ./src

# We use SDL-threads, so link td-sdl to threaddep
if ! [ -d "threaddep" ]; then
  ln -s td-sdl threaddep
fi

# Link md-pandora to machdep
if ! [ -d "machdep" ]; then
  ln -s md-pandora machdep
fi

# Link od-sound to sounddep
if ! [ -d "sounddep" ]; then
  ln -s sd-pandora sounddep
fi

cd ..
