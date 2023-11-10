import os

from conan import ConanFile

class TestTestConan(ConanFile):
    settings = "os", "compiler", "build_type", "arch"

    def test(self):
        pass
