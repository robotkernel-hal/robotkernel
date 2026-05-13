from conan import ConanFile


class MainProject(ConanFile):
    python_requires = "conan_template/[~6]@robotkernel/stable"
    python_requires_extend = "conan_template.RobotkernelConanFile"

    name = "robotkernel"
    description = "robotkernel is a modular, easy configurable hardware abstraction framework"
    exports_sources = ["*", "!.gitignore"]
    tool_requires = ["robotkernel_generator/[~6]@robotkernel/stable"]

    def requirements(self):
        self.requires(f"{self.name}_ln_msgdef/{self.version}@{self.user}/{self.channel}")
        self.requires("yaml-cpp/0.7.0@3rdparty/stable", transitive_headers=True, transitive_libs=True)
