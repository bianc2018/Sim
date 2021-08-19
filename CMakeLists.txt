# CMake 最低版本号要求

cmake_minimum_required (VERSION 3.0)

# 项目信息
project (Sim)

#导入openssl
find_package(OpenSSL REQUIRED)
#OpenSSL_FOUND
IF(OpenSSL_FOUND)
	set(DEP_LIBS
		OpenSSL::Crypto OpenSSL::SSL  # modern
		#${OPENSSL_SSL_LIBRARY} ${OPENSSL_CRYPTO_LIBRARY}  # old style
	)
	add_definitions(-DSIM_USE_OPENSSL)
	MESSAGE(STATUS "OpenSSL_FOUND SSL_LIBRARY: ${OPENSSL_SSL_LIBRARY} ,CRYPTO_LIBRARY: ${OPENSSL_CRYPTO_LIBRARY}")  
ELSE(OpenSSL_FOUND)
	MESSAGE(WARNING "OpenSSL_NotFOUND ")  
ENDIF(OpenSSL_FOUND)

# 指定生成目标
MESSAGE(STATUS "operation system is ${CMAKE_SYSTEM}")  
 
IF (CMAKE_SYSTEM_NAME MATCHES "Linux")  
    MESSAGE(STATUS "current platform: Linux ")  
	list(APPEND DEP_LIBS pthread)
ELSEIF (CMAKE_SYSTEM_NAME MATCHES "Windows")  
    MESSAGE(STATUS "current platform: Windows")  
ELSEIF (CMAKE_SYSTEM_NAME MATCHES "FreeBSD")  
    MESSAGE(STATUS "current platform: FreeBSD")  
ELSE ()  
    MESSAGE(STATUS "other platform: ${CMAKE_SYSTEM_NAME}")  
ENDIF (CMAKE_SYSTEM_NAME MATCHES "Linux")  
  

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/Sim/include)

MESSAGE(STATUS "DEP_LIBS: ${DEP_LIBS}") 

add_subdirectory(demo)
option(BUILD_TESTS "Build tests." TRUE)
if (BUILD_TESTS)
    enable_testing()
    add_subdirectory ("test")
endif()