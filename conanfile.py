from conans import ConanFile, AutoToolsBuildEnvironment


class MainProject(ConanFile):
    name = "robotkernel"
    license = "GPLv3"
    url = "https://rmc-github.robotic.dlr.de/robotkernel/robotkernel"
    description = "robotkernel-5 is a modular, easy configurable hardware abstraction framework"
    settings = "os", "compiler", "build_type", "arch"
    scm = {
        "type": "git",
        "url": "auto",
        "revision": "auto",
        "submodule": "recursive",
    }

    generators = "pkg_config"
    requires = "libstring_util/[~=1.1]@common/unstable", "yaml-cpp/0.6.1@jbeder/stable"

    def build(self):
        self.run("autoreconf -if")
        autotools = AutoToolsBuildEnvironment(self)
        autotools.configure(configure_dir=".", host=self.settings.arch )
        autotools.make()

    def package(self):
        autotools = AutoToolsBuildEnvironment(self)
        autotools.install()

    def package_info(self):
        self.cpp_info.includedirs = ['include']
        self.cpp_info.bindirs = ['bin']
        self.cpp_info.resdirs = ['share']
