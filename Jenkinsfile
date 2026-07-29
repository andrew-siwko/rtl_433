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

        stage('Deploy') {
            steps {
                sh """
                    mkdir -p /opt/rtl_433/bin
                    cp ${BUILD_DIR}/src/rtl_433 /opt/rtl_433/bin/rtl_433
                """
                archiveArtifacts artifacts: "${BUILD_DIR}/src/rtl_433", fingerprint: true
            }
        }

        stage('Build & Push Image') {
            agent { label 'docker-builder' }
            steps {
                checkout scm
                sh """
                    docker build -t ${IMAGE}:${BUILD_NUMBER} -t ${IMAGE}:latest .
                    docker push ${IMAGE}:${BUILD_NUMBER}
                    docker push ${IMAGE}:latest
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
            cleanWs()
        }
    }
}
