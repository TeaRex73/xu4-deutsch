var wshShell = WScript.CreateObject("WScript.Shell");
var messageMode = Number(WScript.arguments(0));
var messageText = WScript.arguments(1);
wshShell.Popup(messageText, 0, "Ultima IV", messageMode + 4096);
