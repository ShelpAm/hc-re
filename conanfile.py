from conan import ConanFile

class HCReRecipe(ConanFile):
    name = "hc-re"
    version = "0.1"
    settings = "os", "compiler", "build_type", "arch"
    default_options = {
        "spdlog/*:use_std_fmt": True
    }
    requires = (
        "nlohmann_json/3.12.0",
        "spdlog/1.16.0",
        "cli11/2.6.0",
        "boost/1.89.0",
        "cpp-httplib/0.27.0",
        "gtest/1.17.0",
        "cppcodec/0.2"
    )
    generators = (
        "CMakeDeps",
        "CMakeToolchain"
    )
