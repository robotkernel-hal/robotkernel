import os

from conans import ConanFile, CMake, tools
from conans.client.run_environment import RunEnvironment

class TestTestConan(ConanFile):
    settings = "os", "compiler", "build_type", "arch"

    def test(self):
        pass
