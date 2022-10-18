// Docs: https://github.com/jenkinsci/gitlab-plugin#scripted-pipeline-jobs

pipeline {
    agent none
    
    post {
        failure {
            updateGitlabCommitStatus name: 'build_linux64', state: 'failed'
        }
        success {
            updateGitlabCommitStatus name: 'build_linux64', state: 'success'
        }
        aborted {
            updateGitlabCommitStatus name: 'build_linux64', state: 'canceled'
        }
    }

    options {
        gitLabConnection ('LVTK-GitLab')
        gitlabBuilds (builds: [ 
            'build_linux64', 'test_linux64', 'deploy_linux64' 
        ])
    }

    triggers {
        gitlab(
            triggerOnPush: true, 
            triggerOnMergeRequest: true, 
            branchFilterType: 'All')
    }

    stages {
        stage('build_linux64') {
            agent {
                label 'linux64'
            }
            steps {
                updateGitlabCommitStatus name: 'build_linux64', state: 'pending'
                updateGitlabCommitStatus name: 'build_linux64', state: 'running'
                sh 'sh tools/lindeploy/jenkins.sh'
                updateGitlabCommitStatus name: 'build_linux64', state: 'success'
            }
        }

        stage('test_linux64') {
            agent {
                label 'linux64'
            }
            steps {
                updateGitlabCommitStatus name: 'test_linux64', state: 'pending'
                updateGitlabCommitStatus name: 'test_linux64', state: 'running'
                echo "Testing..."
                updateGitlabCommitStatus name: 'test_linux64', state: 'success'
            }
        }

        stage('deploy_linux64') {
            agent {
                label 'linux64'
            }
            steps {
                updateGitlabCommitStatus name: 'deploy_linux64', state: 'pending'
                updateGitlabCommitStatus name: 'deploy_linux64', state: 'running'
                echo "Deploying..."
                updateGitlabCommitStatus name: 'deploy_linux64', state: 'success'
            }
        }
    }
}
