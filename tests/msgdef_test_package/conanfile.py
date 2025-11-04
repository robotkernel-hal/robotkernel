import os

from conan import ConanFile


class TestTestConan(ConanFile):
    settings = "os", "compiler", "build_type", "arch"

    self.requires(self.tested_reference_str)

    def test(self):
        pass
