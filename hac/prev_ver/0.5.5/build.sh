#!/bin/bash

clear
echo "Initializing Build..."
echo " "
echo "----------------------------------------------------------------------"
echo "Building User"
gcc ./User.c -o User
echo "Done building User"
echo " "
echo "----------------------------------------------------------------------"
echo "Building Server"
gcc ./SmartHomeServer.c -o Server -pthread
echo "Done building Server"
echo " "
echo "----------------------------------------------------------------------"
echo "Done Building All."
