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
    exports_sources = [
        "share/ln/message_definitions/*",
    ]

    def build_requirements(self):
        self.build_requires(f"ln_helper/[~=0]@robotkernel/stable")

    def requirements(self):
        self.requires(f"robotkernel/{self.version}@{self.user}/{self.channel}")

    def package(self):
        re = RunEnvironment(self)
        libdirs = self.deps_cpp_info["robotkernel"].lib_paths
        with tools.environment_append(re.vars):
            msgdef_dir = os.path.join(self.build_folder, "share", "ln", "message_definitions")

            for libdir in libdirs:
                for filename in os.listdir(libdir):
                    if filename.endswith("libservice_definitions.so"):
                        self.run("bridge_ln_generator --so_file %s --out %s" % (os.path.join(libdir, filename), msgdef_dir))

        self.copy("*", "share/ln/message_definitions", "share/ln/message_definitions")


    def package_info(self):
        self.env_info.LN_MESSAGE_DEFINITION_DIRS.append(os.path.join(self.package_folder, "message_definitions"))
