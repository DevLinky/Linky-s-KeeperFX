cmake_minimum_required(VERSION 3.20)


if( NOT EXISTS ${CMAKE_SOURCE_DIR}/deps/enet )
	file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/deps/enet)
	if( NOT EXISTS ${CMAKE_SOURCE_DIR}/deps/enet-mingw32.tar.gz )
		file(DOWNLOAD https://github.com/dkfans/kfx-deps/releases/download/initial/enet-mingw32.tar.gz ${CMAKE_SOURCE_DIR}/deps/enet-mingw32.tar.gz SHOW_PROGRESS)
	endif()

	execute_process(COMMAND ${CMAKE_COMMAND} -E tar xzf ${CMAKE_SOURCE_DIR}/deps/enet-mingw32.tar.gz WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/deps/enet)
endif()

if( NOT EXISTS ${CMAKE_SOURCE_DIR}/deps/zlib )
	file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/deps/zlib)
	if( NOT EXISTS ${CMAKE_SOURCE_DIR}/deps/zlib-mingw32.tar.gz )
		file(DOWNLOAD https://github.com/dkfans/kfx-deps/releases/download/initial/zlib-mingw32.tar.gz ${CMAKE_SOURCE_DIR}/deps/zlib-mingw32.tar.gz SHOW_PROGRESS)
	endif()

	execute_process(COMMAND ${CMAKE_COMMAND} -E tar xzf ${CMAKE_SOURCE_DIR}/deps/zlib-mingw32.tar.gz WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/deps/zlib)
endif()

if( NOT EXISTS ${CMAKE_SOURCE_DIR}/deps/spng )
	file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/deps/spng)
	if( NOT EXISTS ${CMAKE_SOURCE_DIR}/deps/spng-mingw32.tar.gz )
		file(DOWNLOAD https://github.com/dkfans/kfx-deps/releases/download/initial/spng-mingw32.tar.gz ${CMAKE_SOURCE_DIR}/deps/spng-mingw32.tar.gz SHOW_PROGRESS)
	endif()

	execute_process(COMMAND ${CMAKE_COMMAND} -E tar xzf ${CMAKE_SOURCE_DIR}/deps/spng-mingw32.tar.gz WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/deps/spng)
endif()

if( NOT EXISTS ${CMAKE_SOURCE_DIR}/deps/astronomy )
	file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/deps/astronomy)
	if( NOT EXISTS ${CMAKE_SOURCE_DIR}/deps/astronomy-mingw32.tar.gz )
		file(DOWNLOAD https://github.com/dkfans/kfx-deps/releases/download/initial/astronomy-mingw32.tar.gz ${CMAKE_SOURCE_DIR}/deps/astronomy-mingw32.tar.gz SHOW_PROGRESS)
	endif()

	execute_process(COMMAND ${CMAKE_COMMAND} -E tar xzf ${CMAKE_SOURCE_DIR}/deps/astronomy-mingw32.tar.gz WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/deps/astronomy)
endif()

if( NOT EXISTS ${CMAKE_SOURCE_DIR}/deps/centijson )
	file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/deps/centijson)
	if( NOT EXISTS ${CMAKE_SOURCE_DIR}/deps/centijson-mingw32.tar.gz )
		file(DOWNLOAD https://github.com/dkfans/kfx-deps/releases/download/initial/centijson-mingw32.tar.gz ${CMAKE_SOURCE_DIR}/deps/centijson-mingw32.tar.gz SHOW_PROGRESS)
	endif()

	execute_process(COMMAND ${CMAKE_COMMAND} -E tar xzf ${CMAKE_SOURCE_DIR}/deps/centijson-mingw32.tar.gz WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/deps/centijson)
endif()

if(NOT EXISTS ${CMAKE_SOURCE_DIR}/deps/ffmpeg)
	file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/deps/ffmpeg)
	if(NOT EXISTS ${CMAKE_SOURCE_DIR}/deps/ffmpeg-mingw32.tar.gz)
		file(DOWNLOAD https://github.com/dkfans/kfx-deps/releases/download/initial/ffmpeg-mingw32.tar.gz ${CMAKE_SOURCE_DIR}/deps/ffmpeg-mingw32.tar.gz SHOW_PROGRESS)
	endif()

	execute_process(COMMAND ${CMAKE_COMMAND} -E tar xzf ${CMAKE_SOURCE_DIR}/deps/ffmpeg-mingw32.tar.gz WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/deps/ffmpeg)
endif()

## enet (replaced by enet6 below)

## spng
add_library(spng_static STATIC IMPORTED)
set_target_properties(spng_static PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/spng/libspng.a)
set_target_properties(spng_static PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/spng/include)
target_link_libraries(keeperfx PUBLIC spng_static)
target_link_libraries(keeperfx_hvlog PUBLIC spng_static)

## centijson
add_library(centijson_static STATIC IMPORTED)
set_target_properties(centijson_static PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/centijson/libjson.a)
set_target_properties(centijson_static PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/centijson/include)
target_link_libraries(keeperfx PUBLIC centijson_static)
target_link_libraries(keeperfx_hvlog PUBLIC centijson_static)

## astronomy.
add_library(astronomy_static STATIC IMPORTED)
set_target_properties(astronomy_static PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/astronomy/libastronomy.a)
set_target_properties(astronomy_static PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/astronomy/include)
target_link_libraries(keeperfx PUBLIC astronomy_static)
target_link_libraries(keeperfx_hvlog PUBLIC astronomy_static)

## zlib
add_library(zlib_static STATIC IMPORTED)
set_target_properties(zlib_static PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/zlib/libz.a)
set_target_properties(zlib_static PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/zlib/include)
target_link_libraries(keeperfx PUBLIC zlib_static)
target_link_libraries(keeperfx_hvlog PUBLIC zlib_static)

# Add minizip
add_library(minizip_static STATIC IMPORTED)
set_target_properties(minizip_static PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/zlib/libminizip.a)
set_target_properties(minizip_static PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/zlib/include)
target_link_libraries(minizip_static INTERFACE zlib_static)
target_link_libraries(keeperfx PUBLIC minizip_static)
target_link_libraries(keeperfx_hvlog PUBLIC minizip_static)

## centitoml.
add_library(centitoml OBJECT "centitoml/toml_api.c")
target_link_libraries(centitoml PUBLIC centijson_static)
target_include_directories(centitoml INTERFACE "centitoml")
target_link_libraries(keeperfx PUBLIC centitoml)
target_link_libraries(keeperfx_hvlog PUBLIC centitoml)

## ffmpeg
add_library(libavcodec_static STATIC IMPORTED)
set_target_properties(libavcodec_static PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/ffmpeg/libavcodec/libavcodec.a)
set_target_properties(libavcodec_static PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/ffmpeg)

add_library(libavformat_static STATIC IMPORTED)
set_target_properties(libavformat_static PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/ffmpeg/libavformat/libavformat.a)
set_target_properties(libavformat_static PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/ffmpeg)

add_library(libavutil_static STATIC IMPORTED)
set_target_properties(libavutil_static PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/ffmpeg/libavutil/libavutil.a)
set_target_properties(libavutil_static PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/ffmpeg)

add_library(libswresample_static STATIC IMPORTED)
set_target_properties(libswresample_static PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/ffmpeg/libswresample/libswresample.a)
set_target_properties(libswresample_static PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/ffmpeg)

target_link_libraries(keeperfx PUBLIC libavformat_static libavcodec_static libswresample_static libavutil_static bcrypt)
target_link_libraries(keeperfx_hvlog PUBLIC libavformat_static libavcodec_static libswresample_static libavutil_static bcrypt)

## luajit
add_library(luajit_static STATIC IMPORTED)
set_target_properties(luajit_static PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/luajit/lib/libluajit.a)
set_target_properties(luajit_static PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/luajit/include)
target_link_libraries(keeperfx PUBLIC luajit_static)
target_link_libraries(keeperfx_hvlog PUBLIC luajit_static)

## enet6
add_library(enet6_static STATIC IMPORTED)
set_target_properties(enet6_static PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/enet6/lib/libenet6.a)
set_target_properties(enet6_static PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/enet6/include)
target_link_libraries(enet6_static INTERFACE ws2_32 winmm)
target_link_libraries(keeperfx PUBLIC enet6_static)
target_link_libraries(keeperfx_hvlog PUBLIC enet6_static)

## openal
find_library(OPENAL_DYNAMIC_LIBRARY NAMES openal libopenal.dll.a
    PATHS "C:/msys64/mingw32/lib"
    NO_DEFAULT_PATH)
if (OPENAL_DYNAMIC_LIBRARY)
    add_library(openal_dynamic UNKNOWN IMPORTED)
    set_target_properties(openal_dynamic PROPERTIES IMPORTED_LOCATION ${OPENAL_DYNAMIC_LIBRARY})
    set_target_properties(openal_dynamic PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/openal/include)
    target_link_libraries(keeperfx PUBLIC openal_dynamic)
    target_link_libraries(keeperfx_hvlog PUBLIC openal_dynamic)
else ()
    add_library(gthr_win32_compat OBJECT "gthr_win32_compat.c")
    target_link_libraries(keeperfx PUBLIC gthr_win32_compat)
    target_link_libraries(keeperfx_hvlog PUBLIC gthr_win32_compat)
    add_library(openal_static STATIC IMPORTED)
    set_target_properties(openal_static PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/openal/libOpenAL32.a)
    set_target_properties(openal_static PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/openal/include)
    target_compile_definitions(keeperfx PUBLIC AL_LIBTYPE_STATIC=1)
    target_compile_definitions(keeperfx_hvlog PUBLIC AL_LIBTYPE_STATIC=1)
    target_link_libraries(openal_static INTERFACE ole32 uuid)
    target_link_libraries(keeperfx PUBLIC openal_static)
    target_link_libraries(keeperfx_hvlog PUBLIC openal_static)
endif ()

## miniupnpc
add_library(miniupnpc_static STATIC IMPORTED)
set_target_properties(miniupnpc_static PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/miniupnpc/libminiupnpc.a)
set_target_properties(miniupnpc_static PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/miniupnpc/include)
target_link_libraries(miniupnpc_static INTERFACE iphlpapi)
target_link_libraries(keeperfx PUBLIC miniupnpc_static)
target_link_libraries(keeperfx_hvlog PUBLIC miniupnpc_static)

## libnatpmp
add_library(libnatpmp_static STATIC IMPORTED)
set_target_properties(libnatpmp_static PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/libnatpmp/libnatpmp.a)
set_target_properties(libnatpmp_static PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/libnatpmp/include)
target_link_libraries(libnatpmp_static INTERFACE iphlpapi)
target_link_libraries(keeperfx PUBLIC libnatpmp_static)
target_link_libraries(keeperfx_hvlog PUBLIC libnatpmp_static)

## libcurl
add_library(libcurl_static STATIC IMPORTED)
set_target_properties(libcurl_static PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/libcurl/lib/libcurl.a)
set_target_properties(libcurl_static PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/libcurl/include)
target_link_libraries(libcurl_static INTERFACE wldap32 crypt32 secur32 iphlpapi)
target_link_libraries(keeperfx PUBLIC libcurl_static)
target_link_libraries(keeperfx_hvlog PUBLIC libcurl_static)
