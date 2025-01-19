from os.path import join

from conan import ConanFile
from conan.tools.cmake import CMake, CMakeDeps, CMakeToolchain, cmake_layout
from conan.tools.files import copy
from conan.tools.scm import Git


class DriftFrameworkConan(ConanFile):
    name = "reduct-cpp"
    license = "MIT"
    author = "Alexey Timin"
    url = "https://github.com/reduct-storage/reduct-cpp"
    description = "Reduct Storage Client SDK for C++"
    topics = ("reduct-storage", "http-client", "http-api")
    settings = "os", "compiler", "build_type", "arch"
    options = {
        "shared": [True, False],
        "fPIC": [True, False],
        "with_chrono": [True, False],
    }
    default_options = {
        "shared": False,
        "fPIC": True,
        "with_chrono": False,
        "cpp-httplib/*:with_openssl": True,
        "cpp-httplib/*:with_zlib": True,
        "date/*:header_only": True,
    }

    requires = (
        "fmt/11.0.2",
        "cpp-httplib/0.16.0",
        "nlohmann_json/3.11.3",
        "openssl/3.2.2",
        "concurrentqueue/1.0.4",
    )

    def set_version(self):
        if not self.version:
            git = Git(self, self.recipe_folder)
            self.version = git.run("describe --tags") + ".local"

    def config_options(self):
        if self.settings.get_safe("os") == "Windows":
            self.options.rm_safe("fPIC")
            self.options.with_chrono = True
        elif (
            self.settings.get_safe("compiler") == "gcc"
            and self.settings.get_safe("compiler.version") >= "14"
        ):
            self.options.with_chrono = True

    def requirements(self):
        if not self.options.with_chrono:
            self.requires("date/3.0.1")

    def layout(self):
        cmake_layout(self)

    def generate(self):
        deps = CMakeDeps(self)
        deps.generate()
        tc = CMakeToolchain(self)
        if self.options.with_chrono:
            tc.variables["REDUCT_CPP_USE_STD_CHRONO"] = "ON"
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
        copy(
            self,
            "*.h",
            dst=join(self.package_folder, "include", "reduct"),
            src=join(self.source_folder, "src", "reduct"),
        )
        copy(
            self,
            "*reductcpp.lib",
            src=join(self.build_folder, "lib"),
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
        copy(
            self,
            "*.so",
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
