#!/bin/bash

set -xue

./src/init_lua-npge.sh

brew update
brew install homebrew/dupes/ncurses

brew install \
    qt4 cmake libzip libzzip boost \
    luajit luabind readline \
    pandoc gnu-tar coreutils binutils

