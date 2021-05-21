ECHO OFF
@REM robot -t "AD8233 Noise Test" watch_test.robot
robot watch_test.robot

if exist output_dir.tmp (goto VALID_OUTPUT) else ( goto NO_OUTPUT)

:VALID_OUTPUT
set /p output_dir=<output_dir.tmp
ECHO Results Directory: %output_dir%
ECHO Copying reports to Shared Drive...
if exist report.html (xcopy report.html "%output_dir%" /i )
if exist report.html (xcopy log.html "%output_dir%" /i )
if exist report.html (xcopy output.xml "%output_dir%" /i )
goto END

:NO_OUTPUT
ECHO "Unable to copy Report and Log files to shared drive!")
goto END

:END
ECHO 'Done'