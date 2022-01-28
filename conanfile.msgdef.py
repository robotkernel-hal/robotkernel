import os
from conans import tools, ConanFile, AutoToolsBuildEnvironment
from conans.client.run_environment import RunEnvironment

class MainProject(ConanFile):
    name = "robotkernel_ln_msgdef"
    url = "https://rmc-github.robotic.dlr.de/robotkernel/robotkernel"
    author = "Robert Burger robert.burger@dlr.de"
    license = "GPLv3"
    description = "robotkernel ln message definitions"
    settings = None
    exports_sources = [ "share/robotkernel/service_definitions/*", ]

    def build_requirements(self):
        self.build_requires(f"robotkernel_ln_helper/[*]@robotkernel/stable")

    def package(self):
        svc_def_dir = 'share/robotkernel/service_definitions'
        ln_msg_dir  = 'share/ln/message_definitions'

        re = RunEnvironment(self)
        
        with tools.environment_append(re.vars):

            svc_def_files = []
            with tools.chdir(os.path.join(self.build_folder, svc_def_dir)):
                for (dirpath, dirnames, filenames) in os.walk('.'): 
                    svc_def_files.extend(os.path.join(dirpath, filename) for filename in filenames)

            print("service_generate --indir %s --outdir %s %s" % (svc_def_dir, ln_msg_dir, " ".join(svc_def_files)))
            self.run("service_generate --indir %s --outdir %s %s" % (svc_def_dir, ln_msg_dir, " ".join(svc_def_files)))

        self.copy("*", ln_msg_dir, ln_msg_dir)

    def package_info(self):
        self.env_info.LN_MESSAGE_DEFINITION_DIRS.append(os.path.join(self.package_folder, "message_definitions"))
 
