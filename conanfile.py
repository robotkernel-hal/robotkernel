from conan import ConanFile


class MainProject(ConanFile):
    python_requires = "conan_template/[~5]@robotkernel/stable"
    python_requires_extend = "conan_template.RobotkernelConanFile"

    name = "robotkernel"
    description = "robotkernel is a modular, easy configurable hardware abstraction framework"
    exports_sources = ["*", "!.gitignore"]
    tool_requires = ["robotkernel_service_helper/[~6]@robotkernel/unstable"]

    def source(self):
        self.run(f"sed 's/AC_INIT(.*/AC_INIT([robotkernel], [{self.version}], [{self.author}])/' -i configure.ac")

    def requirements(self):
        self.requires(f"{self.name}_ln_msgdef/{self.version}@{self.user}/{self.channel}")
        self.requires("libstring_util/[~1]@common/stable", transitive_headers=True, transitive_libs=True)
        self.requires("yaml-cpp/0.7.0@3rdparty/stable", transitive_headers=True, transitive_libs=True)
