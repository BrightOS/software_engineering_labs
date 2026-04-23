cmake_minimum_required(VERSION 3.12...3.31)
project(booking-service CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(DownloadUserver)

find_package(userver COMPONENTS core mongo QUIET)
if(NOT userver_FOUND)
    download_userver(TRY_DIR third_party/userver VERSION 2.8)
endif()

userver_setup_environment()

find_package(jwt-cpp 0.7.2 QUIET)
if(NOT jwt-cpp_FOUND)
    include(FetchContent)
    FetchContent_Declare(jwt-cpp
        GIT_REPOSITORY https://github.com/Thalhammer/jwt-cpp.git
        GIT_TAG v0.7.2)
    FetchContent_MakeAvailable(jwt-cpp)
endif()

add_library(${PROJECT_NAME}_objs OBJECT
    src/storage.hpp src/storage.cpp
    src/booking_handler.hpp src/booking_handler.cpp
)
target_link_libraries(${PROJECT_NAME}_objs PUBLIC userver::core userver::mongo jwt-cpp::jwt-cpp)
target_include_directories(${PROJECT_NAME}_objs PUBLIC src ${CMAKE_CURRENT_SOURCE_DIR}/shared)

add_executable(${PROJECT_NAME} src/main.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE ${PROJECT_NAME}_objs)
