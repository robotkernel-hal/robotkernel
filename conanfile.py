from conans import ConanFile, tools

class MainProject(ConanFile):
    python_requires = "conan_template_ln_generator/[~=5 >=5.0.7]@robotkernel/stable"
    python_requires_extend = "conan_template_ln_generator.RobotkernelLNGeneratorConanFile"

    name = "robotkernel"
    description = "robotkernel is a modular, easy configurable hardware abstraction framework"
    exports_sources = ["*", "!.gitignore"] + ["!%s" % x for x in tools.Git().excluded_files()]

    def requirements(self):
        self.requires("libstring_util/[~=1]@common/stable")
        self.requires("yaml-cpp/0.6.2@3rdparty/stable")

    def config_options(self):
        self.options.generate_ln_mds = True
        self.options["libstring_util"].shared = True

