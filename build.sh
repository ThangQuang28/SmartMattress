#!/bin/bash

printf_mode() {
    echo "1: To set target ..."
    echo "2: To Configure ..."
    echo "3: To build ..." 
    echo "4: To Flash ..."
    echo "5: To Moniter ..."
}

echo "=== White cat project ============================================================== >"
echo -n ">>> Setup path of tool and library ..."
echo ""
. $HOME/iot/devices/esp32/esp-idf/export.sh
while true; do
    printf_mode
    echo -n ": Master chose mode to build white cat project >> "
    read mode

    case $mode in

    1 )
        echo -n ">>> Set target ..."
        echo ""
        idf.py set-target esp32
        ;;
    2 )
        echo -n ">>> Configure ..."
        echo ""
        idf.py menuconfig
        ;;
    3 )
        echo -n ">>> Build ..."
        echo ""
        idf.py build
        ;;

    4 )
        echo -n ">>> Flash ..."
        echo ""
        # idf.py flash # 
        idf.py -p /dev/ttyUSB1 flash
        ;;
    5 )
        echo -n ">>> Moniter ..."
        echo ""
        idf.py monitor #idf.py -p PORT monitor
        ;;
    *)
        ;;
    esac
    echo ""
    echo "--- Next step ------------------------------------------"
done