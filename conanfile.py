from conans import tools, python_requires, AutoToolsBuildEnvironment
from conans.client.run_environment import RunEnvironment
import os

base = python_requires("conan_template_ln_generator/[~=5]@robotkernel/stable")

class MainProject(base.RobotkernelLNGeneratorConanFile):
    name = "robotkernel"
    description = "robotkernel-5 is a modular, easy configurable hardware abstraction framework"
    exports_sources = ["*", "!.gitignore"] + ["!%s" % x for x in tools.Git().excluded_files()]

    def requirements(self):
        self.requires("libstring_util/[~=1]@common/stable")
        self.requires("yaml-cpp/0.6.2@3rdparty/stable")

    def config_options(self):
        self.options["libstring_util"].shared = True

