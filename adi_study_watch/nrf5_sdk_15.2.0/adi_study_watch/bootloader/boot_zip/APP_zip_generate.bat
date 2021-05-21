ECHO OFF
IF [%1]==[] GOTO EMPTY

:VALID
SET build_mode=%1
GOTO PROCEED

:EMPTY
SET build_mode=debug


:PROCEED
IF %build_mode%==debug (copy ..\..\app\nRF52840_app\ses\Output\Debug\Exe\watchv4_nrf52840.hex .\ADI_project.hex) ELSE (copy ..\..\app\nRF52840_app\ses\Output\Release\Exe\watchv4_nrf52840.hex .\ADI_project.hex)
nrfutil pkg generate --hw-version 52 --sd-req 0xAE --application-version 0xff --application ADI_project.hex --key-file private.pem ADI_project.zip
pause