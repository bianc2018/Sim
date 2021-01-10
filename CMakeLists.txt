# CMake 最低版本号要求

cmake_minimum_required (VERSION 2.8)

# 项目信息
project (Sim)

# 指定生成目标
MESSAGE(STATUS "operation system is ${CMAKE_SYSTEM}")  
  
IF (CMAKE_SYSTEM_NAME MATCHES "Linux")  
    MESSAGE(STATUS "current platform: Linux ")  
	target_link_libraries(Demo pthread)
    SET(DEFAULT_LIB pthread)
ELSEIF (CMAKE_SYSTEM_NAME MATCHES "Windows")  
    MESSAGE(STATUS "current platform: Windows")  
ELSEIF (CMAKE_SYSTEM_NAME MATCHES "FreeBSD")  
    MESSAGE(STATUS "current platform: FreeBSD")  
ELSE ()  
    MESSAGE(STATUS "other platform: ${CMAKE_SYSTEM_NAME}")  
ENDIF (CMAKE_SYSTEM_NAME MATCHES "Linux")  
  

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/Sim/include)

add_subdirectory(demo)
