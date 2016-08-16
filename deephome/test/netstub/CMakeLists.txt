add_executable(dhnetstub "netstub.cpp")
target_link_libraries(dhnetstub dhcommon pthread)
install(TARGETS dhnetstub DESTINATION bin)
install(FILES dhrouter-test.conf DESTINATION test)
