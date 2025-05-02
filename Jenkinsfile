timestamps {
    node("ubuntu18-agent") {
        def IS_MEMORY_LEAKAGE = 'FALSE'
        catchError {
            checkout scm
            dir_exists = sh (
		        script: "test -d 'tests' && echo 'Y' || echo 'N' ",
                returnStdout: true
            ).trim()

            if (dir_exists == 'N'){
                currentBuild.result= 'FAILURE'
                echo "No tests directory found! Exiting."
                return
            }
            try {
                stage("Prerequisites"){
                    // Change to corresponding CORE_BRANCH as required
                    // e.g. FOGL-xxxx, main etc.
                    sh '''
                        CORE_BRANCH='develop'
                        ${HOME}/buildFledge ${CORE_BRANCH} ${WORKSPACE}
                    '''
                }
            } catch (e) {
                currentBuild.result = 'FAILURE'
                echo "Failed to build Fledge; required to run the tests!"
                return
            }

            try {
                stage("Run Tests") {
                    echo "Executing tests..."
                    sh '''
                        export FLEDGE_ROOT=$HOME/fledge
                        cd tests && cmake . && make -j$(nproc) && \
                        valgrind -v --leak-check=full ./RunTests --gtest_output=xml:test_output.xml 2>&1 | tee valgrind_report.log
                    '''
                    def leakDetected = sh(
                        script: "grep -q '^==[0-9]*==    definitely lost: [1-9][0-9,]* bytes' tests/valgrind_report.log && echo Y || echo N",
                        returnStdout: true
                    ).trim()

                    if (leakDetected == 'Y') {
                        IS_MEMORY_LEAKAGE = 'TRUE'
                        echo "❗ Memory leaks detected!"
                        sh '''
                            grep -A 4 '^==[0-9]*== HEAP SUMMARY:' tests/valgrind_report.log || true
                            grep -A 8 '^==[0-9]*== LEAK SUMMARY:' tests/valgrind_report.log || true
                        '''
                        currentBuild.result = 'FAILURE'
                    } else {
                        echo "✅ No memory leaks detected."
                    }
                    echo "Done."
                }
            } catch (e) {
                currentBuild.result = 'FAILURE'
                echo "Tests failed!"
            }

            try {
                stage("Publish Test Report"){
                    junit "tests/test_output.xml"
                    if (IS_MEMORY_LEAKAGE == 'TRUE') {
                        archiveArtifacts artifacts: 'tests/valgrind_report.log'
                    }
                }
            } catch (e) {
                currentBuild.result = 'FAILURE'
                echo "Failed to generate test reports!"
            }
        }

        stage ("Cleanup"){
            // Add here if any cleanup is required
            echo "Done."
        }
    }
}
