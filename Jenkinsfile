@Library('rmc-jenkins-libraries@master') _

String defaultProfiles =  'osl42-x86_64-gcc4.8 sled11-x86_64-gcc4.8 sled11-x86-gcc4.8 ubuntu14.04-armhf-gcc4.9'

def parallelCtx = rmcBuild.parallelContext({ -> defaultProfiles })
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
