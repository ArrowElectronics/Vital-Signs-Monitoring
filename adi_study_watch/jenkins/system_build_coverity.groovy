// Build IAR/SES with coverity analysis
def call(Object builder, String build_mode, String builder_name) {
  // "Stage" is for Jenkins to help it organize the steps
  stage("Build") {
    if (builder_name == "IAR"){
      if (build_mode == "debug") {
          // Build IAR for PM
          builder("./nrf5_sdk_15.2.0/adi_study_watch/algo/pedometer/iar/pedometer.ewp", "Debug", env.COVERITY_STREAM_PERSEUS_HR)
          sleep 5
          builder("./nrf5_sdk_15.2.0/adi_study_watch/algo/ppg_loop1_algo/iar/heart_rate_coolidge.ewp", "Debug", env.COVERITY_STREAM_PERSEUS_HR)
          sleep 15
          builder("./nrf5_sdk_15.2.0/adi_study_watch/app/nRF52840_app/iar/watchv4_nrf52840.ewp", "freertos", env.COVERITY_STREAM_PERSEUS)

          // Rename the output file based on the git hash
          sh "mv nrf5_sdk_15.2.0/adi_study_watch/app/nRF52840_app/iar/Debug/Exe/watchv4.hex nrf5_sdk_15.2.0/adi_study_watch/app/nRF52840_app/iar/Debug/Exe/watchv4_nrf52840_debug_${env.GIT_VER}.hex"
          // "Stash" the binary so it can be "unstashed" for the tests/
          stash includes: "nrf5_sdk_15.2.0/adi_study_watch/app/nRF52840_app/iar/Debug/Exe/watchv4_nrf52840_debug_${env.GIT_VER}.hex", name: "watchv4_nrf52840_debug_bin_stash"
          // Archive all the artifacts.. This makes them available for download through jenkins
          archiveArtifacts artifacts: 'nrf5_sdk_15.2.0/adi_study_watch/app/nRF52840_app/iar/Debug/Exe/*.hex', onlyIfSuccessful: true
      } else {
        // Build IAR for PM
        builder("./nrf5_sdk_15.2.0/adi_study_watch/algo/pedometer/iar/pedometer.ewp", "Release", env.COVERITY_STREAM_PERSEUS_HR)
        sleep 5
        builder("./nrf5_sdk_15.2.0/adi_study_watch/algo/ppg_loop1_algo/iar/heart_rate_coolidge.ewp", "Release", env.COVERITY_STREAM_PERSEUS_HR)
        sleep 15
        builder("./nrf5_sdk_15.2.0/adi_study_watch/app/nRF52840_app/iar/watchv4_nrf52840.ewp", "Release", env.COVERITY_STREAM_PERSEUS)

        // Rename the output file based on the git hash
        sh "mv nrf5_sdk_15.2.0/adi_study_watch/app/nRF52840_app/iar/Release/Exe/watchv4.hex nrf5_sdk_15.2.0/adi_study_watch/app/nRF52840_app/iar/Release/Exe/watchv4_nrf52840_release_${env.GIT_VER}.hex"
        // "Stash" the binary so it can be "unstashed" for the tests/
        stash includes: "nrf5_sdk_15.2.0/adi_study_watch/app/nRF52840_app/iar/Release/Exe/watchv4_nrf52840_release_${env.GIT_VER}.hex", name: "watchv4_nrf52840_release_bin_stash"
        // Archive all the artifacts.. This makes them available for download through jenkins
        archiveArtifacts artifacts: 'nrf5_sdk_15.2.0/adi_study_watch/app/nRF52840_app/iar/Release/Exe/*.hex', onlyIfSuccessful: true
      }
    } else {
      if (build_mode == "debug") {
        // Build SES for PM
        builder("./nrf5_sdk_15.2.0/adi_study_watch/app/nRF52840_app/ses/watchv4_nrf52840.emProject", "Debug", "HeartRateCoolidge", env.COVERITY_STREAM_PERSEUS_HR)
        sleep 5
        builder("./nrf5_sdk_15.2.0/adi_study_watch/app/nRF52840_app/ses/watchv4_nrf52840.emProject", "Debug", "watchv4_nrf52840", env.COVERITY_STREAM_PERSEUS)

        // Rename the output file based on the git hash
        //sh "mv nrf5_sdk_15.2.0/adi_study_watch/algo/ppg_loop1_algo/package/debug/lib_adpd_m4.a nrf5_sdk_15.2.0/adi_study_watch/algo/ppg_loop1_algo/package/debug/HeartRate_debug${env.GIT_VER}.a"
        //archiveArtifacts artifacts: 'nrf5_sdk_15.2.0/adi_study_watch/algo/ppg_loop1_algo/package/debug/*.a', onlyIfSuccessful: true

        // Rename the output file based on the git hash
        sh "mv nrf5_sdk_15.2.0/adi_study_watch/app/nRF52840_app/ses/Output/Debug/Exe/watchv4_nrf52840.hex nrf5_sdk_15.2.0/adi_study_watch/app/nRF52840_app/ses/Output/Debug/Exe/watchv4_nrf52840_debug_${env.GIT_VER}.hex"
        // "Stash" the binary so it can be "unstashed" for the tests/
        stash includes: "nrf5_sdk_15.2.0/adi_study_watch/app/nRF52840_app/ses/Output/Debug/Exe/watchv4_nrf52840_debug_${env.GIT_VER}.hex", name: "watchv4_nrf52840_debug_bin_stash"
        // Archive all the artifacts.. This makes them available for download through jenkins
        archiveArtifacts artifacts: 'nrf5_sdk_15.2.0/adi_study_watch/app/nRF52840_app/ses/Output/Debug/Exe/*.hex', onlyIfSuccessful: true
      } else {
        // Build SES for PM
        builder("./nrf5_sdk_15.2.0/adi_study_watch/app/nRF52840_app/ses/watchv4_nrf52840.emProject", "Release", "HeartRateCoolidge", env.COVERITY_STREAM_PERSEUS_HR)
        sleep 5
        builder("./nrf5_sdk_15.2.0/adi_study_watch/app/nRF52840_app/ses/watchv4_nrf52840.emProject", "Release", "pedometer", env.COVERITY_STREAM_PERSEUS)
        sleep 5
        builder("./nrf5_sdk_15.2.0/adi_study_watch/app/nRF52840_app/ses/watchv4_nrf52840.emProject", "Release", "watchv4_nrf52840", env.COVERITY_STREAM_PERSEUS)

        // Rename the output file based on the git hash
        //sh "mv nrf5_sdk_15.2.0/adi_study_watch/algo/ppg_loop1_algo/package/release/lib_adpd_m4.a nrf5_sdk_15.2.0/adi_study_watch/algo/ppg_loop1_algo/package/release/HeartRate_release${env.GIT_VER}.a"
        //archiveArtifacts artifacts: 'nrf5_sdk_15.2.0/adi_study_watch/algo/ppg_loop1_algo/package/release/*.a', onlyIfSuccessful: true

        // Rename the output file based on the git hash
        sh "mv nrf5_sdk_15.2.0/adi_study_watch/app/nRF52840_app/ses/Output/Release/Exe/watchv4_nrf52840.hex nrf5_sdk_15.2.0/adi_study_watch/app/nRF52840_app/ses/Output/Release/Exe/watchv4_nrf52840_release_${env.GIT_VER}.hex"
        // "Stash" the binary so it can be "unstashed" for the tests/
        stash includes: "nrf5_sdk_15.2.0/adi_study_watch/app/nRF52840_app/ses/Output/Release/Exe/watchv4_nrf52840_release_${env.GIT_VER}.hex", name: "watchv4_nrf52840_release_bin_stash"
        // Archive all the artifacts.. This makes them available for download through jenkins
        archiveArtifacts artifacts: 'nrf5_sdk_15.2.0/adi_study_watch/app/nRF52840_app/ses/Output/Release/Exe/*.hex', onlyIfSuccessful: true
      }
    }
    currentBuild.result = 'SUCCESS'
  }
}
return this;