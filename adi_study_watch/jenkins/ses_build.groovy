// This constructs and returns an object which runs the "call" method (i.e. iar_build("my_project.ewp", "Debug", -parallel 4))
def call(String proj_file, String project_config, String project_name, String nop=null) {
  env.GIT_VER = sh (
    script: "git describe --always --dirty",
    returnStdout: true
  ).trim()
  sh "${env.SES_CLI_EXE} -config ${project_config} -project ${project_name} ${proj_file}"
}

return this;

// emBuild -config "Debug" -project "HeartRateCoolidge" watchv4_nrf52840.emProject
// emBuild -config "Debug" -project "watchv4_nrf52840" watchv4_nrf52840.emProject