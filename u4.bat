@echo off
if x%OS%x==xWindows_NTx goto winnt
goto nowinnt
:winnt
verify other 2>nul
setlocal enableextensions
if errorlevel 1 goto oldwindows
setlocal
setlocal disabledelayedexpansion
for /f "tokens=3-7 delims=[.] " %%I in ('ver') do @(if %%I==XP (set OS_VER_ORG=%%K.%%L) else (if %%J geq 10 (set OS_VER_ORG=%%J.%%K.%%L) else (set OS_VER_ORG=%%J.%%K)))
set OS_VER=%OS_VER_ORG%
if %OS_VER_ORG:~0,1% gtr 3 set OS_VER=0%OS_VER_ORG%
if %OS_VER% lss 06.1 goto oldwindows
cd /d "%~dp0"
if not exist "%APPDATA%\xu4" mkdir "%APPDATA%\xu4" >nul 2>nul
copy /y .\xu4.cfg "%APPDATA%\xu4" >nul 2>nul
if not exist .\ultima4.zip goto download
attrib -s -h -r "%TEMP%\u4zipsiz.txt" >nul 2>nul
del /f "%TEMP%\u4zipsiz.txt" >nul 2>nul
call :size .\ultima4.zip >"%TEMP%\u4zipsiz.txt"
set /p FILESIZE=<"%TEMP%\u4zipsiz.txt"
del /f "%TEMP%\u4zipsiz.txt" >nul 2>nul
if not "%FILESIZE%"=="529099" goto download
goto nodownload
:download
cd /d "%~dp0"
cscript //NoLogo //E:jscript MessageBox.js 64 "Die originale MS-DOS-Version von Ultima IV wird jetzt heruntergeladen..."
set TRY=1
:nexttry
call mirrors.bat
set /a m=%RANDOM% %% %MAXMIRROR% + 1
set MIRROR=MIRROR%m%
setlocal enabledelayedexpansion
call :getfile !%MIRROR%!
if errorlevel 1 goto error
setlocal disabledelayedexpansion
cd /d "%~dp0"
cscript //NoLogo //E:jscript MessageBox.js 64 "Herunterladen war erfolgreich! Klicke erneut auf Ultima IV, um die Dokumentation lesbar zu machen!"
exit /b 0
:error
setlocal disabledelayedexpansion
set /a TRY=%TRY% + 1
if %TRY% leq 10 goto :nexttry
set TRY=1
:nexttry2
call mirrors.bat
set /a m=%RANDOM% %% %MAXMIRROR% + 1
set MIRROR=MIRROR%m%
setlocal enabledelayedexpansion
call :getfile2 !%MIRROR%!
if errorlevel 1 goto error2
setlocal disabledelayedexpansion
cd /d "%~dp0"
cscript //NoLogo //E:jscript MessageBox.js 64 "Herunterladen war erfolgreich! Klicke erneut auf Ultima IV, um die Dokumentation lesbar zu machen!"
exit /b 0
:error2
setlocal disabledelayedexpansion
set /a TRY=%TRY% + 1
if %TRY% leq 10 goto :nexttry2
cd /d "%~dp0"
cscript //NoLogo //E:jscript MessageBox.js 48 "Herunterladen war nicht erfolgreich! Lade Ultima IV von https://ultima.thatfleminggent.com/ultima4.zip selbst herunter und kopiere die zip-Datei, ohne sie zu entpacken, in den Unterordner Daten!"
exit /b 1
:nodownload
cd "..\Ultima IV Dokumentation"
if not exist Box.pdf goto decode
if not exist Geschichte.pdf goto decode
if not exist Karte.pdf goto goto decode
if not exist Referenz.pdf goto decode
if not exist Weisheit.pdf goto decode
goto nodecode
:decode
cd /d "%~dp0"
cscript //NoLogo //E:jscript MessageBox.js 64 "Die Dokumentation wird nun lesbar gemacht..."
cd "..\Ultima IV Dokumentation"
for %%I in (*.xor) do ..\Daten\xorbin.exe %%I ..\Daten\ultima4.zip > %%~dpnI
if not exist Box.pdf goto error2
if not exist Geschichte.pdf goto error
if not exist Karte.pdf goto error2
if not exist Referenz.pdf goto error2
if not exist Weisheit.pdf goto error2
del /f /q *.xor >nul 2>nul
cd /d "%~dp0"
cscript //NoLogo //E:jscript MessageBox.js 64 "Lesbarmachen war erfolgreich, bitte lies die PDF-Dateien im Ordner Ultima IV Dokumentation! Klicke nochmal auf Ultima IV, um zu spielen!"
exit /b 0
:nodecode
cd /d "%~dp0"
set SDL_VIDEODRIVER=windib
start /b .\u4.exe -f
exit /b 0
:error2
cd /d "%~dp0"
cscript //NoLogo //E:jscript MessageBox.js 16 "Die Dokumentation konnte nicht lesbar gemacht werden! Bitte kontaktiere die Entwickler und melde den Fehler!"
exit /b 1
:getfile
attrib -s -h -r .\ultima4.zip >nul 2>nul
del /f .\ultima4.zip >nul 2>nul
powershell -c "Invoke-WebRequest -Uri '%1distfiles/59/ultima4.zip' -OutFile 'ultima4.zip'"
attrib -s -h -r "%TEMP%\u4zipsiz.txt" >nul 2>nul
del /f "%TEMP%\u4zipsiz.txt" >nul 2>nul
call :size .\ultima4.zip >"%TEMP%\u4zipsiz.txt"
set /p FILESIZE=<"%TEMP%\u4zipsiz.txt"
del /f "%TEMP%\u4zipsiz.txt" >nul 2>nul
if "%FILESIZE%"=="529099" exit /b 0
exit /b 1
:getfile2
attrib -s -h -r .\ultima4.zip >nul 2>nul
del /f .\ultima4.zip >nul 2>nul
powershell -c "& {(New-Object System.Net.WebClient).DownloadFile('%1distfiles/59/ultima4.zip','ultima4.zip')}"
attrib -s -h -r "%TEMP%\u4zipsiz.txt" >nul 2>nul
del /f "%TEMP%\u4zipsiz.txt" >nul 2>nul
call :size .\ultima4.zip >"%TEMP%\u4zipsiz.txt"
set /p FILESIZE=<"%TEMP%\u4zipsiz.txt"
del /f "%TEMP%\u4zipsiz.txt" >nul 2>nul
if "%FILESIZE%"=="529099" exit /b 0
exit /b 1
:size
if exist %1 goto cont
echo 0
exit /b
:cont
echo %~z1
exit /b
:nowinnt
if exist c:\windows\command\cscript.exe goto oldwindows
echo.
echo *****************************************************
echo * Ultima IV Deutsch erfordert mindestens Windows 7! *
echo *****************************************************
echo.
goto end
:oldwindows
if exist Daten\MessageBox.js cd Daten
cscript //NoLogo MessageBox.js 16 "Ultima IV Deutsch erfordert mindestens Windows 7!"
goto end
:end
