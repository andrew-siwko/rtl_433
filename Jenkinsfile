pipeline {
    agent { label 'orangepi' }

    options {
        timestamps()
        disableConcurrentBuilds()
    }

    environment {
        BUILD_DIR = 'build'
    }

    stages {
        stage('Checkout') {
            steps {
                checkout scm
            }
        }

        stage('Install Dependencies') {
            steps {
                sh '''
                    id
                    sudo apt-get update -q -y
                    sudo apt-get install -q -y --no-install-recommends \
                        cmake build-essential pkg-config \
                        libusb-1.0-0-dev librtlsdr-dev libsoapysdr-dev
                '''
            }
        }

        stage('Configure') {
            steps {
                sh "cmake -B ${BUILD_DIR} -DCMAKE_BUILD_TYPE=Release"
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
    }

    post {
        always {
            cleanWs()
        }
    }
}
