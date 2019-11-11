import os

from conans import ConanFile, CMake, tools
from conans.client.run_environment import RunEnvironment

class TestTestConan(ConanFile):
    settings = "os", "compiler", "build_type", "arch"

    def test(self):
        if not tools.cross_building(self.settings):
            re = RunEnvironment(self)
            with tools.environment_append(re.vars):
                self.run("robotkernel")
        else:
            self.output.warn("Skipping run cross built package")
