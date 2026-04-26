#!/bin/bash

VENV="." # set venv path here

VENV_BIN="Scripts" # on windows the executables reside in venv/Scripts (using git-bash)

echo -e "\n\nexecuting $HOME/Qpydict/setup.py with $HOME/$VENV/$VENV_BIN/python\n\n"

export CC=gcc

$HOME/$VENV/$VENV_BIN/python ../setup.py build_ext --build-lib ./shared_lib --build-temp ./build_temp
