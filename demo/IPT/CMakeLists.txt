# all cpp files
add_definitions(-DUSING_SIM_LOGGER)
enable_testing()
add_executable(IptService "Ipt.cpp")
target_link_libraries(IptService  ${DEP_LIBS})