include_directories(include)
include_directories(.)

add_library(gtest SHARED src/gtest-all.cc)
target_link_libraries(gtest pthread)
