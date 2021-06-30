@echo off

if "%1" == "" goto USAGE_M4

if not exist %1 goto USAGE_M4
if not exist "%1\lib_adpd_m4.a" goto USAGE_M4

if exist "%1\tmp" rd "%1\tmp" /s /q
    
mkdir "%1\tmp"
copy "%1\lib_adpd_m4.a" "%1\tmp"
if exist %2 copy "%2" "%1\tmp"
if exist %3 copy "%3" "%1\tmp"
if exist %4 copy "%4" "%1\tmp"
if exist %5 copy "%5" "%1\tmp"
if exist %6 copy "%6" "%1\tmp"

:EXTRACT_M4
cd "%1\tmp"
iarchive --extract lib_adpd_m4.a
if exist %2 iarchive --extract %2
if exist %3 iarchive --extract %3
if exist %4 iarchive --extract %4
if exist %5 iarchive --extract %5
if exist %6 iarchive --extract %6

:PROCESS_M4
set "files="
for %%F in ("*.o") do call set files=%%files%% "%%F"
echo Files: %files%    

iarchive --create ..\lib_adpd_m4_%1.a %files%

cd ..\..
rd "%1\tmp" /s /q

copy ..\adpd400x_lib.h adpd400x_lib.h

goto END

:USAGE_M4
echo.
echo Usage: %0 BasePath HRMLib ToolboxLib
echo Example: makelib.cmd debug lib_adi_vsm_hrv_m3.a libadi_vsm_sqi_m4f.a AGClibM3_Debug.a lib_adi_vsm_hrm_m3.a lib_adi_vsm_toolbox_m3.a

:END
