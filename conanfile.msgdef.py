import os, sys, re
from conans import tools, ConanFile, AutoToolsBuildEnvironment
from conans.client.run_environment import RunEnvironment

class MainProject(ConanFile):
    name = "robotkernel_ln_msgdef"
    url = "https://rmc-github.robotic.dlr.de/robotkernel/robotkernel"
    author = "Robert Burger robert.burger@dlr.de"
    license = "GPLv3"
    description = "robotkernel ln message definitions"
    settings = None
    generators = "pkg_config"
    exports_sources = ["*", "!.gitignore"] + ["!%s" % x for x in tools.Git().excluded_files()]

    def build_requirements(self):
        self.build_requires(f"ln_helper/[~=0]@robotkernel/stable")
        self.build_requires("libstring_util/[~=1]@common/stable")
        self.build_requires("yaml-cpp/0.6.2@3rdparty/stable")

    def build(self):
        self.run("autoreconf -if")
        autotools = AutoToolsBuildEnvironment(self)
        autotools.configure(configure_dir=".")
        autotools.make()
        
        re = RunEnvironment(self)
        with tools.environment_append(re.vars):
            msgdef_dir = os.path.join(self.build_folder, "message_definitions/robotkernel")
            filename = os.path.join(self.build_folder, "src/main/cpp/.libs/libservice_definitions.so")
            self.run("bridge_ln_generator --so_file %s --out %s" % (filename, msgdef_dir))

    def package(self):
        self.copy("*", "message_definitions", "message_definitions")

    def package_info(self):
        self.env_info.LN_MESSAGE_DEFINITION_DIRS.append(os.path.join(self.package_folder, "message_definitions"))
