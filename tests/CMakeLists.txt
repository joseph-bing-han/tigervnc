if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  message(FATAL_ERROR "cmake must be invoked with the top level directory")
endif()

# Benchmarking tools
add_subdirectory(perf)

# Unit tests
if(GTest_FOUND)
  add_subdirectory(unit)
endif()

if(NETTLE_FOUND)
  enable_testing()
  add_executable(nettle_hash unit/nettle_hash.cxx)
  target_include_directories(nettle_hash PRIVATE ${CMAKE_SOURCE_DIR}/common ${NETTLE_INCLUDE_DIRS})
  target_link_libraries(nettle_hash ${NETTLE_LIBRARIES})
  add_test(NAME nettle_hash COMMAND nettle_hash)
endif()
