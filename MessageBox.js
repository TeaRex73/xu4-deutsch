var wshShell = WScript.CreateObject("WScript.Shell");
var messageText = WScript.arguments(0);
wshShell.Popup(messageText, 0, "Ultima IV", 4096);

