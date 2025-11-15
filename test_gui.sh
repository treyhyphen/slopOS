#!/bin/bash

# Test script for vibeOS GUI
echo "Testing vibeOS Window Manager..."
echo ""

#!/bin/bash

# Test script for slopOS GUI
echo "Testing slopOS Window Manager..."
gcc src/main.c -o slopos_sim

# Start slopOS and send commands
{
    echo "startgui"
    sleep 1
    echo "newwin Terminal 5 3 30 8"
    sleep 1
    echo "newwin Editor 25 8 35 10"
    sleep 1
    echo "listwin"
    sleep 1
    echo "writewin 2 0 Hello from slopOS!"
    sleep 1
    echo "writewin 2 1 This is a text editor window"
    sleep 1
    echo "writewin 3 0 $ ls -la"
    sleep 1
    echo "writewin 3 1 drwxr-xr-x user home/"
    sleep 1
    echo "exitgui"
    sleep 1
    echo "exit"
} | ./slopos_sim
