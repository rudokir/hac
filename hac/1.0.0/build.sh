#!/bin/bash

# Clear terminal
clear

# ASCII Art Banner
echo -e "\033[1;34m" # Set blue color
cat << "EOF"
 _    _                         
| |  | |                        
| |__| | ___  _ __ ___   ___   
|  __  |/ _ \| '_ ` _ \ / _ \  
| |  | | (_) | | | | | |  __/  
|_|  |_|\___/|_| |_| |_|\___|  
                               
    _                _ _                      
   / \   _ __  _ __ | (_) __ _ _ __   ___ ___
  / _ \ | '_ \| '_ \| | |/ _` | '_ \ / __/ _ \
 / ___ \| |_) | |_) | | | (_| | | | | (_|  __/
/_/   \_\ .__/| .__/|_|_|\__,_|_| |_|\___\___|
        |_|   |_|                              
  _____            _             _ _           
 / ____|          | |           | | |          
| |     ___  _ __ | |_ _ __ ___ | | | ___ _ __ 
| |    / _ \| '_ \| __| '__/ _ \| | |/ _ \ '__|
| |___| (_) | | | | |_| | | (_) | | |  __/ |   
 \_____\___/|_| |_|\__|_|  \___/|_|_|\___|_|   
EOF
echo -e "\033[0m" # Reset color

echo -e "\033[1;32m" # Set green color
echo "Version 1.0 - Smart Home Control System"
echo -e "\033[0m" # Reset color

echo "Initializing Build..."
echo "----------------------------------------------------------------------"

# Build User executable with optimization
echo -e "\033[1;33m[Building]\033[0m User Application"
gcc ./user.c -o User -O2
if [ $? -eq 0 ]; then
    echo -e "\033[1;32m[Success]\033[0m User build completed"
else
    echo -e "\033[1;31m[Error]\033[0m User build failed"
    exit 1
fi
echo "----------------------------------------------------------------------"

# Build Server executable with optimization and pthread support
echo -e "\033[1;33m[Building]\033[0m Server Application"
gcc ./smart_home_server.c -o Server -pthread -O2
if [ $? -eq 0 ]; then
    echo -e "\033[1;32m[Success]\033[0m Server build completed"
else
    echo -e "\033[1;31m[Error]\033[0m Server build failed"
    exit 1
fi
echo "----------------------------------------------------------------------"

echo -e "\033[1;32m"
cat << "EOF"
 _____ _    _ _____ _____ _____ _____ _____ 
|   __| |  | |     |     |   __|   __|   __|
|__   | |  | |   --|   --|   __|__   |__   |
|_____|_____|_____|_____|_____|_____|_____|
EOF
echo -e "\033[0m"

# Display usage instructions
echo -e "\033[1;36mUsage Instructions:\033[0m"
echo "1. Start the server:"
echo "   ./Server"
echo ""
echo "2. Start the client:"
echo "   ./User                  # Connect to localhost"
echo "   ./User hostname         # Connect to specific host"
echo "   ./User hostname b_on    # Connect and send command"
echo ""
echo "Available Commands:"
echo "- b_on    : Turn boiler on"
echo "- b_off   : Turn boiler off"
echo "- sw_on   : Turn light on"
echo "- sw_off  : Turn light off"
echo ""
echo -e "\033[1;32mBuild Complete! System ready to use.\033[0m"
