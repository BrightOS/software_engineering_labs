cmake_minimum_required(VERSION 3.12...3.31)
project(mailer-service CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(DownloadUserver)

find_package(userver COMPONENTS core rabbitmq QUIET)
if(NOT userver_FOUND)
    download_userver(TRY_DIR third_party/userver VERSION 2.8)
endif()

userver_setup_environment()

add_library(${PROJECT_NAME}_objs OBJECT
    src/mailer_consumer.hpp src/mailer_consumer.cpp
)
target_link_libraries(${PROJECT_NAME}_objs PUBLIC userver::core userver::rabbitmq)
target_include_directories(${PROJECT_NAME}_objs PUBLIC src)

add_executable(${PROJECT_NAME} src/main.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE ${PROJECT_NAME}_objs)
