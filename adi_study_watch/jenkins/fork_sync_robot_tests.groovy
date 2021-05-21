// Executes regression tests for Firmware
def call(String test_tags, String build_mode) {
  stage("DevTest") {
    def gitlab_step = "PS_PM_TESTS"
    def bin_image = "jjj"
    gitlabCommitStatus(gitlab_step) {
      stage("FW_Download") {
        try {
          timeout(10) {
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
            bat label: 'EnterBootloader', script: '''call conda activate py27
            python jenkins/enter_bootloader.py'''
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
        robot_file_dir = "${cur_dir}\\nrf5_sdk_15.2.0\\adi_study_watch\\tst\\robot_framework"
        bat label: 'Execute_Robot_Test', script: """call conda activate py27
        cd ${robot_file_dir}
        robot --variable DUT_COMPORT:${env.NRF_COM_PORT} m2m2_tests.txt
        exit 0"""
        sh "cp nrf5_sdk_15.2.0/adi_study_watch/tst/robot_framework/report.html report.html"
        sh "cp nrf5_sdk_15.2.0/adi_study_watch/tst/robot_framework/log.html log.html"
        sh "cp nrf5_sdk_15.2.0/adi_study_watch/tst/robot_framework/output.xml output.xml"
        sh "zip perseus_test_results.zip report.html log.html output.xml"
        archiveArtifacts artifacts: 'perseus_test_results.zip'
        // archiveArtifacts artifacts: '*.html'
        // archiveArtifacts artifacts: '*.xml'
        def email_list = "${env.PERSEUS_EMAIL_LIST}"
        def email_subject = "Perseus - Firmware Regression Report"
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
        bat label: 'Check_Test_Status', script: """call conda activate py27
        python jenkins/check_robot_test_status.py
        exit %errorlevel%"""
      }
    }
  }
}
return this;