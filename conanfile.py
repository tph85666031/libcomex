from conan import ConanFile

class ComexConan(ConanFile):
   settings = "os", "compiler", "build_type", "arch"
   generators = "CMakeToolchain", "CMakeDeps"
   default_options = {"*:shared": False}

   def requirements(self):
      self.requires("mosquitto/2.0.21")
      self.requires("libcurl/8.12.1")
      self.requires("libarchive/3.7.2")
      self.requires("cairo/1.18.0")
      self.requires("libuv/1.49.2")
      self.requires("libzip/1.11.3")
      self.requires("libnfs/6.0.2")
      self.requires("libqrencode/4.1.1")
      self.requires("bzip2/1.0.8")
      self.requires("libtar/1.2.20")
      self.requires("libmagic/5.45")
      self.requires("openssl/3.5.2")
      self.requires("podofo/0.10.4")
      self.requires("libsmb2/6.2")
      self.requires("libjpeg/9f", override=True) 
        
   def imports(self):
      self.copy("*", dst="out/bin", src="bin")
      self.copy("*.a", dst="out/lib", src="lib")