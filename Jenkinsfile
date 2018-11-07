@Library('rmc-jenkins-libraries@master') _

String profiles = rmcBuild.findProfiles()
println("Building for: $profiles")

def parallelCtx = rmcBuild.parallelContext(profiles)
parallelCtx.checkout()
parallelCtx.generateContext { -> conan.init() }

parallelCtx.stage('create') { vars ->
    conan.pkgCreate(vars.ctx, vars.profile)
}

parallelCtx.stage('upload') { vars ->
    conan.pkgUpload(vars.ctx)
}
