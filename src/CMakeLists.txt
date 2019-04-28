
find_package(glog REQUIRED)
add_executable(SERVER main.cpp connection.cpp server.cpp utils.cpp parser.cpp)
target_link_libraries(SERVER glog::glog)