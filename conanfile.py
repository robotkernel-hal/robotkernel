from conans import ConanFile, AutoToolsBuildEnvironment


class MainProject(ConanFile):
    name = "robotkernel"
    license = "GPLv3"
    url = f"https://rmc-github.robotic.dlr.de/robotkernel/{name}"
    description = "robotkernel-5 is a modular, easy configurable hardware abstraction framework"
    settings = "os", "compiler", "build_type", "arch"
    options = {"debug": [True, False]}
    default_options = {"debug": False}
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
        autotools.libs=[]
        autotools.include_paths=[]
        autotools.library_paths=[]
        if self.options.debug:
            autotools.flags = ["-O0", "-g"]
        else:
            autotools.flags = ["-O3"]
        autotools.configure(configure_dir=".")
        autotools.make()

    def package(self):
        autotools = AutoToolsBuildEnvironment(self)
        autotools.install()

    def package_info(self):
        self.cpp_info.includedirs = ['include']
        self.cpp_info.bindirs = ['bin']
        self.cpp_info.resdirs = ['share']
