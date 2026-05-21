cmake_minimum_required(VERSION 3.12...3.31)
project(auth-service CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(DownloadUserver)

find_package(userver COMPONENTS core postgresql QUIET)
if(NOT userver_FOUND)
    download_userver(TRY_DIR third_party/userver VERSION 2.8)
endif()

userver_setup_environment()

find_package(jwt-cpp 0.7.2 QUIET)
if(NOT jwt-cpp_FOUND)
    include(FetchContent)
    FetchContent_Declare(
        jwt-cpp
        GIT_REPOSITORY https://github.com/Thalhammer/jwt-cpp.git
        GIT_TAG v0.7.2
    )
    FetchContent_MakeAvailable(jwt-cpp)
endif()

add_library(${PROJECT_NAME}_objs OBJECT
    src/storage.hpp
    src/storage.cpp
    src/user_utils.hpp
    src/auth_handler.hpp
    src/auth_handler.cpp
    src/user_handler.hpp
    src/user_handler.cpp
)
target_link_libraries(${PROJECT_NAME}_objs PUBLIC userver::core userver::postgresql jwt-cpp::jwt-cpp)
target_include_directories(${PROJECT_NAME}_objs PUBLIC src ${CMAKE_CURRENT_SOURCE_DIR}/shared)

add_executable(${PROJECT_NAME} src/main.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE ${PROJECT_NAME}_objs)

include(GNUInstallDirs)

if(DEFINED ENV{PREFIX})
    message(STATUS "Set install prefix: $ENV{PREFIX}")
    file(TO_CMAKE_PATH "$ENV{PREFIX}" PREFIX_PATH)
    set(CMAKE_INSTALL_PREFIX "${PREFIX_PATH}")
endif()

file(GLOB CONFIGS_FILES ${CMAKE_CURRENT_SOURCE_DIR}/configs/*.yaml)

install(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_BINDIR}
        COMPONENT ${PROJECT_NAME})
install(FILES ${CONFIGS_FILES}
        DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/${PROJECT_NAME}
        COMPONENT ${PROJECT_NAME})
