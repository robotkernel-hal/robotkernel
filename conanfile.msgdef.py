import os
from conan import ConanFile
from conan.tools.files import copy, chdir

class MainProject(ConanFile):
    name = "robotkernel_ln_msgdef"
    url = "https://rmc-github.robotic.dlr.de/robotkernel/robotkernel"
    author = "Robert Burger robert.burger@dlr.de"
    license = "GPLv3"
    description = "robotkernel ln message definitions"
    settings = None
    exports_sources = [ "share/*", ]

    tool_requires = ["robotkernel_ln_helper/[*]@robotkernel/stable"]

    generators = "VirtualBuildEnv"
    def build(self):
        svc_def_dir = 'share/service_definitions'
        ln_msg_dir  = 'share/ln/message_definitions'

        svc_def_files = []
        with chdir(self, os.path.join(self.build_folder, svc_def_dir)):
                for (dirpath, _, filenames) in os.walk('.'): 
                    svc_def_files.extend(os.path.join(dirpath, filename) for filename in filenames)

        self.run(f"service_generate --indir {svc_def_dir} --outdir {ln_msg_dir} {' '.join(svc_def_files)}", env = "conanbuild")

    def package(self):
        copy(self, "share/ln/message_definitions/*", self.build_folder, self.package_folder)

    def package_info(self):
        self.env_info.LN_MESSAGE_DEFINITION_DIRS.append(os.path.join(self.package_folder, "share/ln/message_definitions"))
 
