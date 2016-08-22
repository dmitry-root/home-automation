
set(common_SOURCES
	"source/client/Command.cpp"
	"source/client/Dispatcher.cpp"
	"source/proto/Connection.cpp"
	"source/proto/Message.cpp"
	"source/proto/SocketConnection.cpp"
	"source/proto/SocketServer.cpp"
	"source/util/Config.cpp"
	"source/util/EventLoop.cpp"
	"source/util/Log.cpp"
	"source/util/Runtime.cpp"
)

add_library(dhcommon SHARED ${common_SOURCES})
target_link_libraries(dhcommon ev)
install(TARGETS dhcommon DESTINATION lib)


add_executable(dhcommon_test_proto test/test_proto.cpp)
target_link_libraries(dhcommon_test_proto dhcommon gtest)

