#!/bin/bash

# Test script for vibeOS with authentication and GUI
echo "Testing vibeOS v2.0 with Authentication..."
echo ""

#!/bin/bash

# Test script for slopOS with authentication and GUI
echo "Testing slopOS v0.1.0 with Authentication..."
gcc src/main.c -o slopos_sim

# Test login and GUI functionality
{
    # Login as admin
    echo "admin"
    echo "slopOS123"
    echo "startgui"
    sleep 1
    echo "newwin UserManager 5 3 40 10"
    sleep 1
    echo "writewin 2 0 User Management System"
    sleep 1
    echo "writewin 2 1 Logged in as: admin"
    sleep 1
    echo "writewin 2 2 Privileges: Administrator"
    sleep 1
    echo "writewin 2 3 "
    sleep 1
    echo "writewin 2 4 Available actions:"
    sleep 1
    echo "writewin 2 5 - Add users"
    sleep 1
    echo "writewin 2 6 - List users"
    sleep 1
    echo "writewin 2 7 - Delete users"
    sleep 1
    echo "exitgui"
    sleep 1
    echo "logout"
} | ./slopos_sim
