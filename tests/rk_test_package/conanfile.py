import os

from conan import ConanFile
from conan.tools.build import can_run


class TestTestConan(ConanFile):
    settings = "os", "compiler", "build_type", "arch"

    generators = "VirtualRunEnv"

    def test(self):
        if can_run(self):
            self.run("robotkernel", env="conanrun")
        else:
            self.output.warn("Skipping run cross built package")
