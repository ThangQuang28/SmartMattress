@echo off

:BEGIN
echo "......................................................"
echo "1: To set target ..."
echo "2: To Configure ..."
echo "3: To build ..." 
echo "4: To Flash ..."
echo "5: To Moniter ..."
echo "6: To Checking Flash ..."
echo "7: END ..."
set /p choose=Master chose mode to build white cat project : 

if %choose%==1 goto setTarget
if %choose%==2 goto configure
if %choose%==3 goto build
if %choose%==4 goto flash
if %choose%==5 goto moniter
if %choose%==6 goto checkflash
if %choose%==7 goto END
goto :BEGIN

:setTarget
echo ">>> Set target ... #esp32 #esp32s3 #esp32c3 #esp32s2"
idf.py set-target esp32
pause
goto BEGIN

:configure
echo ">>> Configure ..."
idf.py menuconfig
pause
goto BEGIN

:build
echo ">>> Build ..."
idf.py build
pause
goto BEGIN

:flash
">>> Flash ... [idf.py -p [COM] -b [115200] flash]"
idf.py -p COM19 flash
pause
goto BEGIN

:moniter
echo ">>> Moniter ..."
idf.py -p COM19 monitor
pause

:checkflash
echo ">>> Checking Flash .. Please run : esptool.py --port [COM] flash_id"
pause

goto BEGIN

:END
echo -n ": === END ======================================  "