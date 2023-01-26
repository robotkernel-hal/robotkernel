from conans import ConanFile, tools

class MainProject(ConanFile):
    python_requires = "conan_template/[~=5]@robotkernel/stable"
    python_requires_extend = "conan_template.RobotkernelConanFile"

    name = "robotkernel"
    description = "robotkernel is a modular, easy configurable hardware abstraction framework"
    exports_sources = ["*", "!.gitignore"] + ["!%s" % x for x in tools.Git().excluded_files()]

    def requirements(self):
        self.requires(f"{self.name}_ln_msgdef/{self.version}@{self.user}/{self.channel}")
        self.requires("robotkernel_service_helper/[*]@robotkernel/stable")
        self.requires("libstring_util/[~=1]@common/stable")
        self.requires("yaml-cpp/0.6.2@3rdparty/stable")

    def package_id(self):
        # set to full_package_mode because those can be either shared or not
        self.info.requires["libstring_util"].full_package_mode()
        self.info.requires["yaml-cpp"].full_package_mode()
