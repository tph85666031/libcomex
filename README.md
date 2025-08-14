# libcomex

easy API library of openssl curl zlib mqtt ...  
comex lib requires com (https://github.com/tph85666031/libcom.git)  

How to build:  
1: install conan2  
2: clone https://github.com/tph85666031/conan2_repo.git  
3: run build.sh in conan2_repo dir to update local lib to conan cache  
4: clone libcomex  
5: run "build.sh -L" to build comex library  

build.sh command info:  
./build -c #clean  
./build    #build the library  
./build -L #build the library and build depencency via conan2
./build -d #-d with debug  
./build -u #build unit test bin
