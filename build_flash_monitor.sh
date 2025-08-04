# BScode sucks lol
. ~/Workspace/binaries/esp-idf-v5.5/export.sh

idf.py build
idf.py flash -p /dev/ttyUSB0
idf.py monitor -p /dev/ttyUSB0
