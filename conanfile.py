from conan import ConanFile
from conan.tools.files import copy
import os

class ComexConan(ConanFile):
   settings = "os", "compiler", "build_type", "arch"
   generators = "CMakeToolchain", "CMakeDeps"
   default_options = {"*:shared": False}

   def requirements(self):
       self.requires("mosquitto/2.0.22-fix",options={"broker": True,"websockets": True,"build_cpp": False})
       self.requires("libcurl/8.12.1")
       self.requires("libarchive/3.7.2")
       self.requires("cairo/1.18.0")
       self.requires("libuv/1.47.0")
       self.requires("libzip/1.11.3")
       self.requires("libiconv/1.17")
       self.requires("libnfs/6.0.2")
       self.requires("libqrencode/4.1.1")
       self.requires("bzip2/1.0.8")
       self.requires("openssl/3.5.2")
       self.requires("podofo/1.0.1")
       self.requires("libsmb2/6.2")
       self.requires("libmagic/5.46-win")
       self.requires("libjpeg/9f")
       self.requires("poco/1.14.2")
       self.requires("zstd/1.5.7", override=True)
       if self.settings.os != "Windows":
          self.requires("libtar/1.2.20")
   
   def generate(self):
       for dep in self.dependencies.values():
           for libdir in dep.cpp_info.libdirs:
               copy(self, "*.a", libdir, os.path.join(self.build_folder, "..","out","lib"))
               copy(self, "*.dylib", libdir, os.path.join(self.build_folder, "..","out","lib"))
               copy(self, "*.dll", libdir, os.path.join(self.build_folder, "..","out","lib"))
           for includedir in dep.cpp_info.includedirs:
               copy(self, "*.h", includedir, os.path.join(self.build_folder, "..","out","include"))
           for bindir in dep.cpp_info.bindirs:
               copy(self, "mosquitto", bindir, os.path.join(self.build_folder, "..","out","bin"))
