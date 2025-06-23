import os

from conan import ConanFile, conan_version
from conan.tools.files import chdir, copy
from conan.tools.scm import Version


class MainProject(ConanFile):
    name = "robotkernel_ln_msgdef"
    url = "https://rmc-github.robotic.dlr.de/robotkernel/robotkernel"
    author = "Robert Burger robert.burger@dlr.de"
    license = "GPLv3"
    description = "robotkernel ln message definitions"
    settings = None
    exports_sources = ["*"]

    tool_requires = ["robotkernel_ln_helper/[*]@robotkernel/stable"]

    generators = "VirtualBuildEnv"

    def build(self):
        ln_msg_dir = "share/ln/message_definitions"

        svc_def_files = []
        with chdir(self, self.build_folder):
            for dirpath, _, filenames in os.walk("./robotkernel"):
                svc_def_files.extend(os.path.join(dirpath, filename) for filename in filenames)

        self.run(f"service_generate --indir . --outdir {ln_msg_dir} {' '.join(svc_def_files)}", env="conanbuild")

    def package(self):
        copy(self, "share/ln/message_definitions/*", self.build_folder, self.package_folder)

    def package_info(self):
        self.cpp_info.includedirs = []
        msg_def_dir = os.path.join(self.package_folder, "share/ln/message_definitions")
        if conan_version < Version("2.0.0"):
            self.env_info.LN_MESSAGE_DEFINITION_DIRS.append(msg_def_dir)
        self.runenv_info.append_path("LN_MESSAGE_DEFINITION_DIRS", msg_def_dir)
        self.buildenv_info.append_path("LN_MESSAGE_DEFINITION_DIRS", msg_def_dir)
