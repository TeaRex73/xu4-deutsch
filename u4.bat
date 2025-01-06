@echo off
cd /d "%~dp0"
if not exist "%APPDATA%\xu4\nul" mkdir "%APPDATA%\xu4"
copy /y .\xu4.cfg "%PPDATA%\xu4"
if not exist .\ultima4.zip goto download
attrib -s -h -r "%TEMP%\u4zipsiz.txt" 2>nul
del /f "%TEMP%\u4zipsiz.txt" 2>nul
call :size .\ultima4.zip >"%TEMP%\u4zipsiz.txt"
set /p FILESIZE=<"%TEMP%\u4zipsiz.txt"
del /f "%TEMP%\u4zipsiz.txt"
if not "%FILESIZE%"=="529099" goto download
goto nodownload
:download
msg Console /w /time:9999 "Die originale englischsprachige MS-DOS-Version von Ultima IV wird jetzt heruntergeladen..."
attrib -s -h -r .\ultima4.zip 2>nul
del /f .\ultima4.zip 2>nul
powershell -c "& {(New-Object System.Net.WebClient).DownloadFile('https://ultima.thatfleminggent.com/ultima4.zip','ultima4.zip')}"
attrib -s -h -r "%TEMP%\u4zipsiz.txt" 2>nul
del /f "%TEMP%\u4zipsiz.txt" 2>nul
call :size .\ultima4.zip >"%TEMP%\u4zipsiz.txt"
set /p FILESIZE=<"%TEMP%\u4zipsiz.txt"
del /f "%TEMP%\u4zipsiz.txt"
if not "%FILESIZE%"=="529099" goto error
msg Console /w /time:9999 "Herunterladen war erfolgreich! Klicke erneut auf Ultima IV, um die Dokumentation lesbar zu machen!"
goto :eof
:nodownload
cd "..\Ultima IV Dokumentation"
if not exist Box.pdf goto decode
if not exist Geschichte.pdf goto decode
if not exist Karte.pdf goto goto decode
if not exist Referenz.pdf goto decode
if not exist Weisheit.pdf goto decode
goto nodecode
:decode
msg Console /w /time:9999 "Die Dokumentation wird nun lesbar gemacht..."
for %%I in (*.xor) do ..\Daten\xorbin.exe %%I ..\Daten\ultima4.zip > %%~dpnI
if not exist Box.pdf goto error2
if not exist Geschichte.pdf goto error2
if not exist Karte.pdf goto error2
if not exist Referenz.pdf goto error2
if not exist Weisheit.pdf goto error2
del /f /q *.xor 2>nul
cd /d "%~dp0"
msg Console /w /time:9999 "Lesbarmachen war erfolgreich, bitte lies die PDF-Dateien im Ordner Ultima IV Dokumentation! Klicke nochmal auf Ultima IV, um zu spielen!"
goto :eof
:nodecode
cd /d "%~dp0"
set SDL_VIDEODRIVER=windib
start /b /min .\u4.exe -f
goto :eof
:error
cd /d "%~dp0"
msg Console /w /time:9999 "Herunterladen war nicht erfolgreich! Finde im Internet die Datei ultima4.zip, die 529.099 Bytes gross ist, und kopiere sie in den versteckten Unterordner Daten!"
goto :eof
:error2
cd /d "%~dp0"
msg Console /w /time:9999 "Die Dokumentation konnte nicht lesbar gemacht werden! Bitte kontaktiere die Entwickler und melde den Fehler!"
goto :eof
:size
if exist "%1" goto cont
echo 0
goto :eof
:cont
echo %~z1
goto :eof
