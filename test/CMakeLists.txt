enable_testing()
file(GLOB_RECURSE CPP_SRC_LIST ./*.cpp)
# generate target for every cpp file
foreach(FILE_PATH ${CPP_SRC_LIST})
    MESSAGE("test :" ${FILE_PATH})
    STRING(REGEX REPLACE ".+/(.+)\\..*" "\\1" FILE_NAME ${FILE_PATH})
    set(TEST_EXE test_${FILE_NAME})
    add_executable(${TEST_EXE} ${FILE_PATH})
    target_link_libraries(${TEST_EXE}  ${DEFAULT_LIB})
	add_test(
        NAME ${TEST_EXE}
        COMMAND ${TEST_EXE}
    )#  ±make test…˙–ß
endforeach()