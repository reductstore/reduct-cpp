from os.path import join

from conan import ConanFile
from conan.tools.cmake import CMake, cmake_layout
from conan.tools.files import copy
from conan.tools.scm import Git


class DriftFrameworkConan(ConanFile):
    name = "reduct-cpp"
    version = "1.13.0"
    license = "MIT"
    author = "Alexey Timin"
    url = "https://github.com/reduct-storage/reduct-cpp"
    description = "Reduct Storage Client SDK for C++"
    topics = ("reduct-storage", "http-client", "http-api")
    settings = "os", "compiler", "build_type", "arch"
    options = {"shared": [True, False], "fPIC": [True, False]}
    default_options = {
        "shared": False,
        "fPIC": True,
        "cpp-httplib/*:with_openssl": True,
        "cpp-httplib/*:with_zlib": True,
        "date/*:header_only": True,
    }
    generators = "CMakeDeps", "CMakeToolchain"

    requires = (
        "fmt/11.0.2",
        "cpp-httplib/0.16.0",
        "nlohmann_json/3.11.3",
        "openssl/3.2.2",
        "concurrentqueue/1.0.4",
        "date/3.0.1",
    )

    def config_options(self):
        if self.settings.get_safe("os") == "Windows":
            self.options.rm_safe("fPIC")

    def layout(self):
        cmake_layout(self)

    def source(self):
        git = Git(self)
        git.clone(url="https://github.com/reduct-storage/reduct-cpp.git", target=".")
        git.checkout(f"v{self.version}")

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        copy(
            self,
            "*.h",
            dst=join(self.package_folder, "include", "reduct"),
            src=join(self.source_folder, "src", "reduct"),
        )
        copy(
            self,
            "*reductcpp.lib",
            dst=join(self.package_folder, "lib"),
            keep_path=False,
        )
        copy(self, "*.dll", dst=join(self.package_folder, "bin"), keep_path=False)
        copy(self, "*.so", dst=join(self.package_folder, "lib"), keep_path=False)
        copy(self, "*.dylib", dst=join(self.package_folder, "lib"), keep_path=False)
        copy(self, "*.a", dst=join(self.package_folder, "lib"), keep_path=False)

    def package_info(self):
        self.cpp_info.libs = ["reductcpp"]
