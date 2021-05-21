echo off
FOR /F %%G IN ('git rev-parse --short HEAD') DO SET commit_id=%%G
FOR /F "tokens=1-3" %%H IN ('git show -s --format^="%%ci" %commit_id%') DO SET date_time=%%H %%I %%J

set ver_file=%1
set proj_config=%2

echo // THIS FILE IS AUTO_GENERATED, DO NOT MODIFY> %ver_file%
echo #include "system_version.h">> %ver_file%
echo const char SYSTEM_BUILD_VERSION[] = "";>> %ver_file%
echo const char SYSTEM_BUILD_USER[] = "ADI";>> %ver_file%
echo const char SYSTEM_BUILD_COMMIT_ID[] = "%commit_id%_%proj_config%";>> %ver_file%
echo const char SYSTEM_BUILD_TIME[] = "%date_time%";>> %ver_file%
echo const uint8_t SYSTEM_BUILD_VERSION_LEN = sizeof(SYSTEM_BUILD_VERSION);>> %ver_file%
echo const uint8_t SYSTEM_BUILD_COMMIT_ID_LEN = sizeof(SYSTEM_BUILD_COMMIT_ID);>> %ver_file%
echo const uint8_t SYSTEM_BUILD_USER_LEN = sizeof(SYSTEM_BUILD_USER);>> %ver_file%
echo const uint8_t SYSTEM_BUILD_TIME_LEN = sizeof(SYSTEM_BUILD_TIME);>> %ver_file%
