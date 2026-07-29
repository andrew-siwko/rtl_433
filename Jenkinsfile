pipeline {
    agent { label 'orangepi' }

    options {
        timestamps()
        disableConcurrentBuilds()
    }

    environment {
        BUILD_DIR = 'build'
        REGISTRY = 'kregistry.siwko.org:5000'
        IMAGE = "${REGISTRY}/rtl_433"
    }

    stages {
        stage('Checkout') {
            steps {
                checkout scm
            }
        }

        // This itakes 20 seconds
        // stage('Install Dependencies') {
        //     steps {
        //         sh '''
        //             id
        //             sudo apt-get update -q -y
        //             sudo apt-get install -q -y --no-install-recommends \
        //                 cmake build-essential pkg-config ccache \
        //                 libusb-1.0-0-dev librtlsdr-dev libsoapysdr-dev
        //         '''
        //     }
        // }

        stage('Configure') {
            steps {
                sh "echo ${BUILD_NUMBER} > BUILD_NUMBER"
                sh "cmake -B ${BUILD_DIR} -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER_LAUNCHER=ccache"
            }
        }

        stage('Build') {
            steps {
                sh "cmake --build ${BUILD_DIR} -- -j\$(nproc)"
            }
        }

        stage('Test') {
            steps {
                sh """
                    cmake --build ${BUILD_DIR} --target test || \
                    { cat ${BUILD_DIR}/Testing/Temporary/LastTest.log; exit 1; }
                """
            }
        }

        stage('Deploy') {
            steps {
                sh """
                    mkdir -p /opt/rtl_433/bin
                    cp ${BUILD_DIR}/src/rtl_433 /opt/rtl_433/bin/rtl_433
                """
                archiveArtifacts artifacts: "${BUILD_DIR}/src/rtl_433", fingerprint: true
                stash name: 'rtl433-bin', includes: "${BUILD_DIR}/src/rtl_433"
            }
        }

        stage('Build & Push Image') {
            agent { label 'docker-builder' }
            steps {
                checkout scm
                unstash 'rtl433-bin'
                sh """
                    cp ${BUILD_DIR}/src/rtl_433 rtl_433
                    docker buildx build --platform linux/arm64 --push \
                        -t ${IMAGE}:${BUILD_NUMBER} -t ${IMAGE}:latest .
                """
            }
            post {
                always {
                    cleanWs()
                }
            }
        }
    }

    post {
        always {
            cleanWs(patterns: [
                [pattern: "${BUILD_DIR}/**", type: 'EXCLUDE'],
                [pattern: '.git/**', type: 'EXCLUDE']
            ])
        }
        success {
            build job: 'kubernetes-rtl-433-sender', wait: false
        }
        failure {
            mail to: 'asiwko@siwko.org',
                 subject: "rtl_433 Build #${env.BUILD_NUMBER} Failed",
                 body: "Check console output at ${env.BUILD_URL}"   
        }
    }
}
