// This constructs and returns an object which runs the "call" method (i.e. iar_build("my_project.ewp", "Debug", -parallel 4))
def call(String proj_file, String project_config, String nop=null) {
  env.GIT_VER = sh (
    script: "git describe --always --dirty",
    returnStdout: true
  ).trim()
  sh "${env.IAR_832_CLI_EXE} ${proj_file} -build ${project_config} -parallel 4"
}

return this;


// "C:\Program Files (x86)\IAR Systems\Embedded Workbench 8.2\common\bin\IarBuild.exe" ".\nrf5_sdk_15.2.0\adi_study_watch\app\nRF52840_app\iar\watchv4_nrf52840.ewp" -build Release/freertos -parallel 4
// "C:\Program Files (x86)\IAR Systems\Embedded Workbench 8.2\common\bin\IarBuild.exe" ".\nrf5_sdk_15.2.0\adi_study_watch\app\nRF52840_app\iar\watchv4_nrf52840.ewp" -clean Release -parallel 4
// "C:\Program Files (x86)\IAR Systems\Embedded Workbench 8.2\common\bin\IarBuild.exe" ".\nrf5_sdk_15.2.0\adi_study_watch\app\nRF52840_app\iar\watchv4_nrf52840.ewp" -make Release -parallel 4
// "C:\Program Files (x86)\IAR Systems\Embedded Workbench 8.2\common\bin\IarBuild.exe" ".\nrf5_sdk_15.2.0\adi_study_watch\algo\pedometer\iar\pedometer.ewp" -build Release/Debug -parallel 4
// "C:\Program Files (x86)\IAR Systems\Embedded Workbench 8.2\common\bin\IarBuild.exe" ".\nrf5_sdk_15.2.0\adi_study_watch\algo\ppg_loop1_algo\iar\heart_rate_coolidge.ewp" -build Release/Debug -parallel 4
