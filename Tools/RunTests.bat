@echo off
rem 게임 규칙 자동화 테스트(Moba.*)를 화면 없이 실행한다. 에디터를 닫고 실행할 것.
set ENGINE=C:\Program Files\Epic Games\UE_5.8
"%ENGINE%\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "%~dp0..\ver1.uproject" -ExecCmds="Automation RunTests Moba; Quit" -unattended -nullrhi -nosplash -nosound -log -abslog="%~dp0..\Saved\Logs\MobaTests.log"
findstr /C:"Test Completed" /C:"EXIT CODE" "%~dp0..\Saved\Logs\MobaTests.log"
pause
