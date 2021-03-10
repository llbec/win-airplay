project(win-airplay)

if(!WIN32)
return()
endif()

find_package(FFmpeg REQUIRED COMPONENTS avcodec avutil)
include_directories(${FFMPEG_INCLUDE_DIRS})

include_directories(SYSTEM 
	"${CMAKE_SOURCE_DIR}/libobs"
	"${CMAKE_SOURCE_DIR}/libobs/util"
)
#include_directories("${CMAKE_CURRENT_SOURCE_DIR}/libs")

#if(MSVC)
#	set(WINAIRPLAY_PLATFORM_SOURCES
#		w32-pthreads)
#endif()

set(AIRPLAY_LIBS
	"${CMAKE_CURRENT_SOURCE_DIR}/libs/postproc.lib"
	"${CMAKE_CURRENT_SOURCE_DIR}/libs/fdk-aac.lib"
	"${CMAKE_CURRENT_SOURCE_DIR}/libs/mycrypt.lib"
	"${CMAKE_CURRENT_SOURCE_DIR}/libs/AirPlay.lib"
)
#link_directories("libs")

FILE(GLOB win_airplay_SOURCES 
	*.c
	*.cpp
	${WINAIRPLAY_PLATFORM_SOURCES}
)

FILE(GLOB win_airplay_HEADERS
	*.h
	*.hpp
)

add_library(win-airplay MODULE
	${win_airplay_SOURCES}
	${win_airplay_HEADERS}
)

target_link_libraries(win-airplay
	${WINAIRPLAY_PLATFORM_DEPS}
	libobs
	${AIRPLAY_LIBS}
	${FFMPEG_LIBRARIES}
	w32-pthreads
	ws2_32)

set_target_properties(win-airplay PROPERTIES FOLDER "plugins/win-airplay")

install_obs_plugin_with_data(win-airplay data)
