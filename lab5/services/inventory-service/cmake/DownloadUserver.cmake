include_guard(GLOBAL)

function(download_userver)
  set(OPTIONS)
  set(ONE_VALUE_ARGS TRY_DIR VERSION GIT_TAG)
  set(MULTI_VALUE_ARGS)
  cmake_parse_arguments(
      ARG "${OPTIONS}" "${ONE_VALUE_ARGS}" "${MULTI_VALUE_ARGS}" ${ARGN}
  )

  if(ARG_TRY_DIR)
    get_filename_component(ARG_TRY_DIR "${ARG_TRY_DIR}" REALPATH)
    if(EXISTS "${ARG_TRY_DIR}")
      message(STATUS "Using userver from ${ARG_TRY_DIR}")
      add_subdirectory("${ARG_TRY_DIR}" third_party/userver)
      return()
    endif()
  endif()

  cmake_minimum_required(VERSION 3.21)

  set(CPM_DOWNLOAD_VERSION 0.40.2)
  if(NOT DEFINED CPM_DOWNLOAD_ALL AND NOT DEFINED CPM_SOURCE_CACHE)
    set(CPM_DOWNLOAD_ALL FALSE)
  endif()

  set(CPM_DOWNLOAD_LOCATION "${CMAKE_BINARY_DIR}/cmake/CPM_${CPM_DOWNLOAD_VERSION}.cmake")
  if(NOT EXISTS "${CPM_DOWNLOAD_LOCATION}")
    message(STATUS "Downloading CPM.cmake to ${CPM_DOWNLOAD_LOCATION}")
    file(DOWNLOAD
        "https://github.com/cpm-cmake/CPM.cmake/releases/download/v${CPM_DOWNLOAD_VERSION}/CPM.cmake"
        "${CPM_DOWNLOAD_LOCATION}"
    )
  endif()
  include("${CPM_DOWNLOAD_LOCATION}")

  if(NOT DEFINED ARG_VERSION AND NOT DEFINED ARG_GIT_TAG)
    set(ARG_GIT_TAG develop)
  endif()

  if(NOT DEFINED CPM_USE_NAMED_CACHE_DIRECTORIES)
    set(CPM_USE_NAMED_CACHE_DIRECTORIES ON)
  endif()

  CPMAddPackage(
      NAME userver
      GITHUB_REPOSITORY userver-framework/userver
      VERSION ${ARG_VERSION}
      GIT_TAG ${ARG_GIT_TAG}
      ${ARG_UNPARSED_ARGUMENTS}
  )
endfunction()
