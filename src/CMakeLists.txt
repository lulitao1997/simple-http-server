
SET(glog_DIR "third_party/")
find_package(glog REQUIRED)
add_executable(SERVER main.cpp connection.cpp server.cpp parser.cpp)
add_library(HTTP_PARSER third_party/http_parser.c)
include_directories(./third_party)
target_link_libraries(SERVER ${GLOG_LIBRARY} HTTP_PARSER)
