@echo off
setlocal EnableDelayedExpansion

:: Default settings
set TARGET=esp32
set COM_PORT=COM5

:BEGIN
cls
echo ======================================
echo         WHITE CAT ESP-IDF TOOL
echo ======================================
echo Current Target : %TARGET%
echo Current Port   : %COM_PORT%
echo ======================================
echo 1. Set Target
echo 2. Configure (menuconfig)
echo 3. Build
echo 4. Flash
echo 5. Monitor
echo 6. Check Flash ID
echo 7. Set COM Port
echo 8. Exit
echo ======================================

set /p choose=Select option : 

if "%choose%"=="1" goto setTarget
if "%choose%"=="2" goto configure
if "%choose%"=="3" goto build
if "%choose%"=="4" goto flash
if "%choose%"=="5" goto monitor
if "%choose%"=="6" goto checkflash
if "%choose%"=="7" goto setComPort
if "%choose%"=="8" goto END

echo Invalid selection!
pause
goto BEGIN


:setTarget
cls
echo ======================================
echo Select ESP Target
echo ======================================
echo 1. esp32
echo 2. esp32s3
echo 3. esp32c3
echo 4. esp32s2
echo 5. Back
echo ======================================

set /p targetChoose=Select target : 

if "%targetChoose%"=="1" set TARGET=esp32
if "%targetChoose%"=="2" set TARGET=esp32s3
if "%targetChoose%"=="3" set TARGET=esp32c3
if "%targetChoose%"=="4" set TARGET=esp32s2
if "%targetChoose%"=="5" goto BEGIN

if not "%targetChoose%"=="1" if not "%targetChoose%"=="2" if not "%targetChoose%"=="3" if not "%targetChoose%"=="4" (
    echo Invalid target!
    pause
    goto setTarget
)

echo.
echo Setting target: %TARGET%
idf.py set-target %TARGET%

pause
goto BEGIN


:setComPort
cls
echo ======================================
echo Set COM Port
echo ======================================
echo Current Port : %COM_PORT%
echo.
set /p COM_PORT=Enter COM Port (Example COM5) : 

if "%COM_PORT%"=="" (
    echo Invalid COM Port!
    pause
)

goto BEGIN


:configure
cls
echo ======================================
echo MENUCONFIG
echo ======================================

idf.py menuconfig

pause
goto BEGIN


:build
cls
echo ======================================
echo BUILD 
echo ======================================
echo Target : %TARGET%
echo ======================================
idf.py build

pause
goto BEGIN


:flash
cls
echo ======================================
echo FLASH
echo ======================================
echo Port   : %COM_PORT%
echo Target : %TARGET%
echo ======================================

idf.py -p %COM_PORT% flash

pause
goto BEGIN


:monitor
cls
echo ======================================
echo MONITOR
echo ======================================
echo Port : %COM_PORT%
echo ======================================

idf.py -p %COM_PORT% monitor

pause
goto BEGIN


:checkflash
cls
echo ======================================
echo CHECK FLASH ID
echo ======================================
echo Port : %COM_PORT%
echo ======================================

esptool.py --port %COM_PORT% flash_id

pause
goto BEGIN


:END
echo.
echo ======================================
echo Goodbye White Cat!
echo ======================================
timeout /t 2 >nul
exit