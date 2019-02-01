@Library('rmc-jenkins-libraries@master') _

String defaultProfiles =  'osl42-x86_64 sled11-x86_64-gcc4.x sled11-x86-gcc4.x ubuntu14.04-armhf-gcc4.x ubuntu12.04-armhf-gcc4.x'
String profiles = rmcBuild.findProfiles({ -> defaultProfiles })
println("Building for: $profiles")

def parallelCtx = rmcBuild.parallelContext(profiles)
parallelCtx.checkout()
parallelCtx.generateContext { -> conan.init() }

parallelCtx.stage('create') { vars ->
    conan.pkgCreate(vars.ctx, vars.profile)
}

parallelCtx.stage('upload') { vars ->
    if(vars.ctx.repoContext.isTagFromBranch('master') || vars.ctx.repoContext.isReleaseBranch()){
        conan.pkgUpload(vars.ctx)
    } else {
        println 'Skipping upload of Snapshot'
    }
}
