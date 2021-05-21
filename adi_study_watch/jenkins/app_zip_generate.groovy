// Generates app zip package
def call(String build_mode, String email_list) {
  // "Stage" is for Jenkins to help it organize the steps
  stage("ArchiveZip") {
      node("NRFUTIL") {
        checkout scm
        def gitlab_step = "PS_PM_TESTS"
        def bin_image = "NA"
        gitlabCommitStatus(gitlab_step) {
          try {
            timeout(5) {
              if (build_mode == "debug") {
                unstash "watchv4_nrf52840_debug_bin_stash"
                bin_image = sh (
                script: "find -name *watchv4_nrf52840_debug*.hex",
                returnStdout: true
                ).trim()
              } else {
                unstash "watchv4_nrf52840_release_bin_stash"
                bin_image = sh (
                script: "find -name *watchv4_nrf52840_release*.hex",
                returnStdout: true
                ).trim()
              }
              echo bin_image
              sh "cp ${bin_image} ADI_project.hex"
              bat label: 'AppZipGenerate', script:"""call conda activate py27
              nrfutil pkg generate --hw-version 52 --sd-req 0xAE --application-version 0xff --application ADI_project.hex --key-file nrf5_sdk_15.2.0/adi_study_watch/bootloader/boot_zip/private.pem ADI_project.zip"""
              archiveArtifacts artifacts: 'ADI_project.zip'
              def email_subject = "Perseus Build Artifact"
              emailext attachmentsPattern: 'ADI_project.zip', body: '**** This is an auto-generated mail! ****', subject: email_subject, to: email_list
            }
          } catch(err) {
            echo "Timeout limit exceeded! Failing the job..."
            updateGitlabCommitStatus name: gitlab_step, state: 'failed'
            throw err
          }
        }
      }
    currentBuild.result = 'SUCCESS'
  }
}
return this;