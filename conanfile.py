from conans import ConanFile


class BeyondFunctionsRecipe(ConanFile):
    name = "beyond-functions"
    version = "0.0.1"
    license = "MIT"
    url = "https://beyond-engine.github.io/functions/"
    description = "A C++17 implementation of various type erased callable types."
    # No settings/options are necessary, this is header only
    exports_sources = "include/*"
    no_copy_source = True

    def package(self):
        self.copy("*.hpp")
