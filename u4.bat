@echo off
cd /D "%~dp0" >nul 2>nul
if not exist %AppData%\xu4\nul mkdir %AppData%\xu4 >nul 2>nul
copy /y .\xu4.cfg %AppData%\xu4 >nul 2>nul
set SDL_VIDEODRIVER=windib
start /b /min .\u4.exe -f
