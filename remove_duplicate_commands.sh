#!/bin/bash
# Script to remove extracted command implementations from kernel.c

KERNEL_FILE="/workspaces/vibeos/kernel/kernel.c"
BACKUP_FILE="/workspaces/vibeos/kernel/kernel.c.backup"

# Backup the file
cp "$KERNEL_FILE" "$BACKUP_FILE"

echo "Removing extracted command implementations from kernel.c..."

# Use awk to remove specific command functions while keeping network commands
awk '
BEGIN { skip = 0; }

# Start skipping at help command
/^void cmd_help\(void\)/ { skip = 1; next; }

# Stop skipping at network commands section
/^void cmd_ifconfig\(void\)/ { skip = 0; }

# Start skipping again at GUI commands 
/^void cmd_startgui\(void\)/ { skip = 1; next; }

# Stop skipping at command parser section
/^\/\/ ====================/ && /COMMAND PARSER/ { skip = 0; }

# Print line if not skipping
!skip { print; }
' "$KERNEL_FILE" > "$KERNEL_FILE.tmp"

# Replace original with modified version
mv "$KERNEL_FILE.tmp" "$KERNEL_FILE"

echo "Done! Backup saved to $BACKUP_FILE"
echo "Now run: make -f Makefile.kernel clean && make -f Makefile.kernel all"
