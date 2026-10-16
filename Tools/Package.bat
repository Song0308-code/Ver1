@echo off
rem 패키지를 만든다. 인자로 Development(기본, 로그 있음) 또는 Shipping(배포용)을 준다. 에디터를 닫고 실행할 것.
set ENGINE=C:\Program Files\Epic Games\UE_5.8
set CONFIG=%~1
if "%CONFIG%"=="" set CONFIG=Development
set OUT=%~dp0..\Saved\Packaged
if /I "%CONFIG%"=="Shipping" set OUT=%~dp0..\Saved\PackagedShipping
call "%ENGINE%\Engine\Build\BatchFiles\RunUAT.bat" BuildCookRun -project="%~dp0..\ver1.uproject" -noP4 -platform=Win64 -clientconfig=%CONFIG% -build -cook -stage -pak -archive -archivedirectory="%OUT%" -unattended -utf8output
echo 결과: %OUT%\Windows
pause
