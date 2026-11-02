@echo off
rem Bam dup de mo web app dem thong minh qua cap USB (khong can doi WiFi).
chcp 65001 >nul
set PY=D:\Espressif\python_env\idf5.4_py3.11_env\Scripts\python.exe
if not exist "%PY%" set PY=python
set PYTHONIOENCODING=utf-8
"%PY%" "%~dp0web_usb_bridge.py" %*
pause

