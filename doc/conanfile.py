from conan import ConanFile
from conan import tools
from conan.tools.files import mkdir, chdir, copy
from conan.tools.gnu import Autotools, AutotoolsToolchain
import os

class MainProject(ConanFile):
    name = "robotkernel-doc"
    license = "GPLv3"
    author = "Robert Burger <robert.burger@dlr.de>"
    url = f"https://rmc-github.robotic.dlr.de/robotkernel/robotkernel.git"
    description = """"This library provides an Operating System Abstraction Layer
                      (OSAL) for other programs so they do not need to take care
                      about the underlying implementation"""
    settings = "os", "compiler", "build_type", "arch"
    exports_sources = ["*", "!.gitignore", "!bindings"]
    options = {"shared": [True, False],
               "coverage" : [True, False]}
    default_options = {"shared": True,
                       "coverage" : False}

    build_requires = [
        "sphinx-rtd-theme/[~2]@pypi/stable",
        "sphinxcontrib-jquery/[>=4 <5]@pypi/stable",
    ]

    def generate(self):
        tc = AutotoolsToolchain(self)
        tc.generate()

    def build(self):
        autotools = Autotools(self)
        autotools.make(target="html")

    def package(self):
        autotools = Autotools(self)
        autotools.install()

    def package_info(self):
        self.cpp_info.includedirs = ['include']
        self.cpp_info.libs = ["osal"]
        self.cpp_info.bindirs = ['bin']
        self.cpp_info.resdirs = ['share']
