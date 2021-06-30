// Executes regression tests for Firmware
def call(String test_tags, String build_mode, String QA_TS_Tolerance) {
  // "Stage" is for Jenkins to help it organize the steps
  stage("DevTest") {
    parallel PS_PM_TESTS: {
      node("PERSEUS_QA") {
        checkout scm
        def gitlab_step = "PS_PM_TESTS"
        def bin_image = "jjj"
        gitlabCommitStatus(gitlab_step) {
          stage("FW_Download") {
            try {
              timeout(10) {
                if (build_mode == "release") {
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
                bat label: 'EnterBootloader', script: """call conda activate ${env.py3_qa_test_env}
                python jenkins/enter_bootloader.py"""
                sleep 1
                sh "cp ${bin_image} ADI_project.hex"
                bat label: 'FwDownload', script:"""call conda activate py27
                nrfutil pkg generate --hw-version 52 --sd-req 0xAE --application-version 0xff --application ADI_project.hex --key-file nrf5_sdk_15.2.0/adi_study_watch/bootloader/boot_zip/private.pem ADI_project.zip
                nrfutil dfu usb-serial -pkg ADI_project.zip -p ${env.NRF_COM_PORT}"""
              }
            } catch(err) {
              echo "Timeout limit exceeded! Failing the job..."
              updateGitlabCommitStatus name: gitlab_step, state: 'failed'
              throw err
            }
          }
          stage("Test_Execution"){
            sleep 2
            cur_dir = pwd()
            robot_file_dir = "${cur_dir}\\nrf5_sdk_15.2.0\\validation"
            bat label: 'Execute_Robot_Test', script: """call conda activate ${env.py3_qa_test_env}
            cd ${robot_file_dir}
            robot --variable DUT_COMPORT:${env.NRF_COM_PORT} --variable TS_Tolerance:${QA_TS_Tolerance} --include ${test_tags} qa_test.robot
            exit 0"""
            sh "cp nrf5_sdk_15.2.0/validation/report.html report.html"
            sh "cp nrf5_sdk_15.2.0/validation/log.html log.html"
            sh "cp nrf5_sdk_15.2.0/validation/output.xml output.xml"
            sh "zip perseus_test_results.zip report.html log.html output.xml"
            archiveArtifacts artifacts: 'perseus_test_results.zip'
            // archiveArtifacts artifacts: '*.html'
            // archiveArtifacts artifacts: '*.xml'
            def email_list = "${env.PERSEUS_EMAIL_LIST}"
            def email_subject = "Perseus - QA Regression Report"
            emailext attachmentsPattern: 'perseus_test_results.zip', body: '**** This is an auto-generated mail! ****', subject: email_subject, to: email_list
          }
          stage("Publish_Results") {
            step([$class: 'hudson.plugins.robot.RobotPublisher', 
                  disableArchiveOutput: false, 
                  logFileName: 'log.html', 
                  otherFiles: '', 
                  outputFileName: 'output.xml', 
                  outputPath: '', 
                  passThreshold: 100, 
                  reportFileName: 'report.html', 
                  unstableThreshold: 0]);
          }
        }
      }
    }, failFast: false // parallel
    currentBuild.result = 'SUCCESS'
  }
}
return this;