@Library('rmc-jenkins-libraries@master') _
import de.dlr.rm.jenkins.ConanBuildConfiguration

def buildConfigurations = ConanBuildConfiguration.permute(
    ['osl42-x86_64-gcc4.8', 'sled11-x86_64-gcc4.8', 'sled11-x86-gcc4.8', 'ubuntu14.04-armhf-gcc4.9'],
    [[build_type: 'Release'], [build_type: 'Debug']],
    [[:]]
)

def parallel = rmcBuild.parallelContext()
parallel.addConfiguration(buildConfigurations)
parallel.stage('build') { config ->
    def ctx = conan.init()

    stage('create') {
        ctx.pkgCreate(config)
    }

    stage('upload') {
        ctx.pkgUpload()
    }
}
