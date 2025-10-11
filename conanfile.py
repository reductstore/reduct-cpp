from os.path import join

from conan import ConanFile
from conan.tools.cmake import CMake, CMakeDeps, CMakeToolchain, cmake_layout
from conan.tools.files import copy
from conan.tools.scm import Git


class DriftFrameworkConan(ConanFile):
    name = "reduct-cpp"
    license = "MIT"
    author = "ReductSoftware UG"
    url = "https://github.com/reduct-storage/reduct-cpp"
    description = "Reduct Storage Client SDK for C++"
    topics = ("reduct-storage", "http-client", "http-api")
    settings = "os", "compiler", "build_type", "arch"
    options = {
        "shared": [True, False],
        "fPIC": [True, False],
    }
    default_options = {
        "shared": False,
        "fPIC": True,
        "cpp-httplib/*:with_openssl": True,
        "cpp-httplib/*:with_zlib": True,
    }

    requires = (
        "fmt/9.1.0",
        "cpp-httplib/0.14.3",
        "nlohmann_json/3.11.3",
        "openssl/3.0.13",
        "concurrentqueue/1.0.4",
    )

    def set_version(self):
        if not self.version:
            git = Git(self, self.recipe_folder)
            self.version = git.run("describe --tags --always") + ".local"

    def config_options(self):
        if self.settings.get_safe("os") == "Windows":
            self.options.rm_safe("fPIC")

    def layout(self):
        cmake_layout(self)

    def generate(self):
        deps = CMakeDeps(self)
        deps.generate()
        tc = CMakeToolchain(self)
        tc.generate()

    def export_sources(self):
        if ".local" in self.version:
            copy(
                self,
                "CMakeLists.txt",
                src=self.recipe_folder,
                dst=self.export_sources_folder,
            )
            copy(
                self, "cmake/*", src=self.recipe_folder, dst=self.export_sources_folder
            )
            copy(self, "src/*", src=self.recipe_folder, dst=self.export_sources_folder)

    def source(self):
        if ".local" not in self.version:
            git = Git(self)
            git.clone(
                url="https://github.com/reduct-storage/reduct-cpp.git", target="."
            )
            git.checkout(f"v{self.version}")

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        # headers
        copy(
            self,
            "*.h",
            src=join(self.source_folder, "src", "reduct"),
            dst=join(self.package_folder, "include", "reduct"),
        )

        # binaries (Windows/MSVC uses per-config dirs like Release/Debug)
        copy(
            self,
            "*.lib",
            src=self.build_folder,
            dst=join(self.package_folder, "lib"),
            keep_path=False,
        )
        copy(
            self,
            "*.dll",
            src=self.build_folder,
            dst=join(self.package_folder, "bin"),
            keep_path=False,
        )

        # unix
        copy(
            self,
            "*.so*",
            src=self.build_folder,
            dst=join(self.package_folder, "lib"),
            keep_path=False,
        )
        copy(
            self,
            "*.dylib",
            src=self.build_folder,
            dst=join(self.package_folder, "lib"),
            keep_path=False,
        )
        copy(
            self,
            "*.a",
            src=self.build_folder,
            dst=join(self.package_folder, "lib"),
            keep_path=False,
        )

    def package_info(self):
        self.cpp_info.libs = ["reductcpp"]
        self.cpp_info.set_property("cmake_file_name", "reductcpp")
        self.cpp_info.set_property("cmake_target_name", "reductcpp::reductcpp")
