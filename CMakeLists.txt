#CMAKE最小版本要求,可修改
#BUILD_TYPE=release,debug  UNIT_TEST=true,false
CMAKE_MINIMUM_REQUIRED(VERSION 3.0)

PROJECT("comex")

MESSAGE("CMAKE_SYSTEM_VERSION: ${CMAKE_SYSTEM_VERSION}")
MESSAGE("CMAKE_SYSTEM_PROCESSOR: ${CMAKE_SYSTEM_PROCESSOR}")
MESSAGE("CMAKE_SYSTEM_NAME: ${CMAKE_SYSTEM_NAME}")
MESSAGE("CMAKE_SYSTEM: ${CMAKE_SYSTEM}")
MESSAGE("CMAKE_LIBRARY_ARCHITECTURE: ${CMAKE_LIBRARY_ARCHITECTURE}")

IF (MSVC)
  ADD_DEFINITIONS("-DWIN32_LEAN_AND_MEAN")
  ADD_DEFINITIONS("-DLIBARCHIVE_STATIC")
  ADD_DEFINITIONS("-DLIBMOSQUITTO_STATIC")
  ADD_DEFINITIONS("-DPODOFO_STATIC")
  ADD_DEFINITIONS("-D_WIN32_WINNT=0x0501")
ENDIF()

IF ("${UNIT_TEST}" STREQUAL "true")
  ADD_DEFINITIONS("-DUNIT_TEST")
ENDIF()

IF ("${BUILD_TYPE}" STREQUAL "Debug")
  ADD_DEFINITIONS("-D__DEBUG__")
  IF (MSVC)
    ADD_COMPILE_OPTIONS( /MDd)
  ELSE()
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
  ENDIF()
ELSE()
  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2")
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2")
  IF (MSVC)
    ADD_COMPILE_OPTIONS( /MD)
  ENDIF()
ENDIF()

set(CMAKE_CXX_STANDARD 17)

#-Woverloaded-virtual
#C/C++文件编译选项,无需修改
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -std=c99 -fwrapv")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

IF (NOT APPLE)
  IF (UNIX)
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wl,--copy-dt-needed-entries -fPIC -fvisibility=hidden -Wl,-Bsymbolic")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wl,--copy-dt-needed-entries -fPIC -fvisibility=hidden -Wl,-Bsymbolic")
  ENDIF()
ENDIF()

IF (APPLE)
  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC -fvisibility=hidden -framework SystemConfiguration -framework Foundation -framework CoreFoundation -Wl,-hidden-lcrypto -Wl,-hidden-lssl -Wl,-hidden-lz -Wl,-hidden-lbz2 -Wl,-hidden-ltar -Wl,-hidden-lzip -Wl,-hidden-larchive -Wl,-hidden-lsmb2 -Wl,-hidden-lnfs -Wl,-hidden-lcurl -Wl,-hidden-lwebsockets -Wl,-hidden-lmosquitto -Wl,-x")
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -fvisibility=hidden -framework SystemConfiguration -framework Foundation -framework CoreFoundation -Wl,-hidden-lcrypto -Wl,-hidden-lssl -Wl,-hidden-lz -Wl,-hidden-lbz2 -Wl,-hidden-ltar -Wl,-hidden-lzip -Wl,-hidden-larchive -Wl,-hidden-lsmb2 -Wl,-hidden-lnfs -Wl,-hidden-lcurl -Wl,-hidden-lwebsockets -Wl,-hidden-lmosquitto -Wl,-x")
ENDIF()

#头文件搜索路径,可修改
INCLUDE_DIRECTORIES(
    ${PROJECT_SOURCE_DIR}/src
	${PROJECT_SOURCE_DIR}/src/export
	${PROJECT_SOURCE_DIR}/out/include
    ${PROJECT_SOURCE_DIR}/../libcom/src/export
)

#库文件搜索路径,可修改
LINK_DIRECTORIES(
	${PROJECT_SOURCE_DIR}
	${PROJECT_SOURCE_DIR}/out/lib
    ${PROJECT_SOURCE_DIR}/../libcom/out
)

#依赖库,无需修改,静态库的依赖顺序为依赖的库写在后面
IF (UNIX OR APPLE)
SET(PROJECT_LIB
	${PROJECT_LIB}
	com
	pthread
    dl
    m
	-Wl,--start-group
    ${PROJECT_SOURCE_DIR}/out/lib/libmosquitto.a
	${PROJECT_SOURCE_DIR}/out/lib/libwebsockets.a
	${PROJECT_SOURCE_DIR}/out/lib/libcurl.a
	${PROJECT_SOURCE_DIR}/out/lib/libpodofo.a
	${PROJECT_SOURCE_DIR}/out/lib/libpodofo_private.a
	${PROJECT_SOURCE_DIR}/out/lib/libcairo.a
	${PROJECT_SOURCE_DIR}/out/lib/libfontconfig.a
	${PROJECT_SOURCE_DIR}/out/lib/libfreetype.a
	${PROJECT_SOURCE_DIR}/out/lib/libarchive.a
	${PROJECT_SOURCE_DIR}/out/lib/libtiff.a
	${PROJECT_SOURCE_DIR}/out/lib/libxml2.a
    ${PROJECT_SOURCE_DIR}/out/lib/libbz2.a
    ${PROJECT_SOURCE_DIR}/out/lib/libtar.a
    ${PROJECT_SOURCE_DIR}/out/lib/libzip.a
    ${PROJECT_SOURCE_DIR}/out/lib/libsmb2.a
    ${PROJECT_SOURCE_DIR}/out/lib/libnfs.a
	${PROJECT_SOURCE_DIR}/out/lib/libssl.a
    ${PROJECT_SOURCE_DIR}/out/lib/libcrypto.a
	${PROJECT_SOURCE_DIR}/out/lib/libiconv.a
	${PROJECT_SOURCE_DIR}/out/lib/libexpat.a
    ${PROJECT_SOURCE_DIR}/out/lib/libjbig.a
	${PROJECT_SOURCE_DIR}/out/lib/libidn.a
	${PROJECT_SOURCE_DIR}/out/lib/libharfbuzz.a
    ${PROJECT_SOURCE_DIR}/out/lib/libbrotlidec.a 
    ${PROJECT_SOURCE_DIR}/out/lib/libbrotlienc.a
	${PROJECT_SOURCE_DIR}/out/lib/libbrotlicommon.a
	${PROJECT_SOURCE_DIR}/out/lib/libunistring.a
	${PROJECT_SOURCE_DIR}/out/lib/libjpeg.a
	${PROJECT_SOURCE_DIR}/out/lib/libpng.a
	${PROJECT_SOURCE_DIR}/out/lib/libmagic.a
	${PROJECT_SOURCE_DIR}/out/lib/liblzma.a
	${PROJECT_SOURCE_DIR}/out/lib/libz.a
	${PROJECT_SOURCE_DIR}/out/lib/libqrencode.a
	${PROJECT_SOURCE_DIR}/out/lib/libpixman-1.a
	libstdc++fs.a
	-Wl,--end-group
)
ELSE()
ADD_COMPILE_OPTIONS( /W3 /wd4819 /wd4251 /wd4200 /wd4996)
SET(PROJECT_LIB
	${PROJECT_LIB}
    com.lib
    archive.lib
    bz2.lib
    cares.lib
    charset.lib
    iconv.lib
    libnfs.lib
    libcrypto.lib
    libcurl.lib
    libssl.lib
    libxml2.lib
    lz4.lib
    lzma.lib
    mosquitto_static.lib
    pthreadVC3.lib
    smb2.lib
    libuv.lib
    websockets.lib
    zip.lib
    zlib.lib
    zstd.lib
	libexpatMD.lib
	pixman-1.lib
	brotlicommon.lib
	brotlidec.lib
	brotlienc.lib
	fontconfig.lib
	freetype.lib
	jpeg.lib
	turbojpeg.lib
	libpng16.lib
	podofo.lib
	podofo_private.lib
	tiff.lib
	qrencode.lib
	cairo.lib
	tre.lib
	magic.lib
	msvcrt.lib
	shlwapi.lib
	Crypt32.lib
)
ENDIF()

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

#src文件夹下的所有源文件,无需修改
#非递归方式，不允许存在子文件夹
AUX_SOURCE_DIRECTORY(./src PROJECT_SRC)
IF ("${UNIT_TEST}" STREQUAL "true")
AUX_SOURCE_DIRECTORY(./src/test PROJECT_SRC)
ENDIF()
#递归方式(允许存在子文件夹)
#FILE(GLOB_RECURSE PROJECT_SRC ${PROJECT_SOURCE_DIR}/*.cpp ${PROJECT_SOURCE_DIR}/*.c)

SET(CMAKE_SKIP_BUILD_RPATH FALSE)
SET(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
SET(CMAKE_INSTALL_RPATH "$ORIGIN:$ORIGIN/../../libcom/out:$ORIGIN/lib")

#无需改动
IF (MSVC)
    SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG   ${PROJECT_SOURCE_DIR}/out)
    SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${PROJECT_SOURCE_DIR}/out)
    SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG   ${PROJECT_SOURCE_DIR}/out)
    SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${PROJECT_SOURCE_DIR}/out)
    SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG   ${PROJECT_SOURCE_DIR}/out)
    SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${PROJECT_SOURCE_DIR}/out)
ENDIF()

IF ("${UNIT_TEST}" STREQUAL "true")
  SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/out)
  ADD_EXECUTABLE(${PROJECT_NAME} ${PROJECT_SRC})
  IF(MSVC)
    SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES LINK_FLAGS "/NODEFAULTLIB:LIBCMT /NODEFAULTLIB:MSVCRT /INCREMENTAL:NO /LTCG")
  ENDIF()
ELSE()
  IF (UNIX OR APPLE)
    SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/out)
  ENDIF()
  
  ADD_LIBRARY(${PROJECT_NAME}_static STATIC ${PROJECT_SRC})
  IF(MSVC)
    SET_TARGET_PROPERTIES(${PROJECT_NAME}_static PROPERTIES OUTPUT_NAME ${PROJECT_NAME}_static LINK_FLAGS "/NODEFAULTLIB:LIBCMT /NODEFAULTLIB:MSVCRT /INCREMENTAL:NO /LTCG")
  ELSE()
    SET_TARGET_PROPERTIES(${PROJECT_NAME}_static PROPERTIES OUTPUT_NAME ${PROJECT_NAME})
  ENDIF()
  SET_TARGET_PROPERTIES(${PROJECT_NAME}_static PROPERTIES CLEAN_DIRECT_OUTPUT 1)
  
  ADD_LIBRARY(${PROJECT_NAME} SHARED ${PROJECT_SRC})
  SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES OUTPUT_NAME ${PROJECT_NAME})
  SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES CLEAN_DIRECT_OUTPUT 1)
  
  FILE(COPY ${PROJECT_SOURCE_DIR}/src/export/ DESTINATION ${PROJECT_SOURCE_DIR}/out/include)
ENDIF()
STRING(STRIP "${PROJECT_LIB}" PROJECT_LIB)
TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${PROJECT_LIB})
