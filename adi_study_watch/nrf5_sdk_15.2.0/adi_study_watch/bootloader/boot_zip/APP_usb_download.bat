IF [%1]==[] GOTO EMPTY

:VALID
SET com_port=%1
GOTO PROCEED

:EMPTY
SET com_port=COM30
ECHO Proceeding with hardcoded COM Port: %com_port%


:PROCEED
nrfutil dfu usb-serial -pkg ADI_project.zip -p %com_port%
pause