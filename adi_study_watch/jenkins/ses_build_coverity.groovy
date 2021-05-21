// This constructs and returns an object which runs the "call" method (i.e. iar_build("my_project.ewp", "Debug", -parallel 4))
def call(String proj_file, String project_config, String project_name, String coverity_stream=null) {
  env.GIT_VER = sh (
    script: "git describe --always --dirty",
    returnStdout: true
  ).trim()
  sh "${env.COVERITY_BUILD_EXE} --dir ./coverity_output --config ./jenkins/coverity/ses_coverity_config.xml ${env.SES_CLI_EXE} -config ${project_config} -project ${project_name} ${proj_file}"
  
  if (coverity_stream != null) {
    sh "${env.COVERITY_ANALYZE_EXE} --dir ./coverity_output --config ./jenkins/coverity/ses_coverity_config.xml --security --concurrency --enable-constraint-fpp --enable-fnptr --enable-virtual --enable-callgraph-metrics -j auto"
    withCredentials([usernamePassword(credentialsId: 'jenkins-coverity-1', passwordVariable: 'COVERITY_PASSWORD', usernameVariable: 'COVERITY_USER')]) {
      sh "echo 'Running Coverity Commit. Hiding output to protect the credentials.'&&set +x"
      sh "${env.COVERITY_COMMIT_EXE} --dir ./coverity_output --host ${env.COVERITY_SERVER_URL} --dataport 9090 --user ${COVERITY_USER} --password ${COVERITY_PASSWORD} --stream ${coverity_stream} --scm git"
      sh "set -x"
      sh "${env.COVERITY_FORMAT_ERRORS_EXE} --dir ./coverity_output --html-output ./coverity_html"
      sh "zip -r coverity_results_${env.GIT_VER}.zip ./coverity_html"
      def email_list = "${env.COVERITY_EMAIL_LIST}"
      def email_subject = "Perseus Coverity results:${coverity_stream}:${env.GIT_VER}"
      emailext attachmentsPattern: 'coverity_results_*.zip', body: '', subject: email_subject, to: email_list
      archiveArtifacts artifacts: 'coverity_results_*.zip', onlyIfSuccessful: true
    }
  }
}

return this;

// emBuild -config "Debug" -project "HeartRateCoolidge" watchv4_nrf52840.emProject
// emBuild -config "Debug" -project "watchv4_nrf52840" watchv4_nrf52840.emProject