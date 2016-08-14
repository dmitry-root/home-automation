
set(router_SOURCES
	"Config.cpp"
	"Router.cpp"
	"main.cpp"
)

add_executable(dhrouter ${router_SOURCES})
target_link_libraries(dhrouter dhcommon ev pthread)
install(TARGETS dhrouter DESTINATION bin)

