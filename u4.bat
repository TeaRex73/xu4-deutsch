@echo off
cd /d "%~dp0"
if not exist "%APPDATA%\xu4\nul" mkdir "%APPDATA%\xu4"
copy /y .\xu4.cfg "%PPDATA%\xu4"
if not exist .\ultima4.zip goto download
attrib -s -h -r "%TEMP%\u4zipsiz.txt"
del /f "%TEMP%\u4zipsiz.txt"
call :size .\ultima4.zip >"%TEMP%\u4zipsiz.txt"
set /p FILESIZE=<"%TEMP%\u4zipsiz.txt"
del /f "%TEMP%\u4zipsiz.txt"
if not "%FILESIZE%"=="529099" goto download
goto nodownload
:download
msg Console /w /time:9999 "Die originale englischsprachige MS-DOS-Version von Ultima IV wird jetzt heruntergeladen..."
attrib -s -h -r .\ultima4.zip
del /f .\ultima4.zip
powershell -c "& {(New-Object System.Net.WebClient).DownloadFile('https://ultima.thatfleminggent.com/ultima4.zip','ultima4.zip')}"
attrib -s -h -r "%TEMP%\u4zipsiz.txt"
del /f "%TEMP%\u4zipsiz.txt"
call :size .\ultima4.zip >"%TEMP%\u4zipsiz.txt"
set /p FILESIZE=<"%TEMP%\u4zipsiz.txt"
del /f "%TEMP%\u4zipsiz.txt"
if not "%FILESIZE%"=="529099" goto error
msg Console /w /time:9999 "Herunterladen war erfolgreich! Klicke erneut auf Ultima IV, um zu spielen."
goto :eof
:nodownload
set SDL_VIDEODRIVER=windib
start /b /min .\u4.exe -f
goto :eof
:error
msg Console /w /time:9999 "Herunterladen war nicht erfolgreich! Finde im Internet die Datei ultima4.zip, die 529.099 Bytes gross ist, und kopiere sie in den versteckten Unterordner Daten!"
goto :eof
:size
if exist "%1" goto cont
echo 0
goto :eof
:cont
echo %~z1
goto :eof
