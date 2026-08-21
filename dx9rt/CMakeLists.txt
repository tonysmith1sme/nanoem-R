# dx9rt - Direct3D 9 semantics runtime for MME effect execution
add_library(dx9rt STATIC
  ${CMAKE_CURRENT_SOURCE_DIR}/src/StateVector.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/src/Types.cc)
set_property(TARGET dx9rt PROPERTY CXX_STANDARD 11)
target_include_directories(dx9rt PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
set_property(TARGET dx9rt PROPERTY FOLDER dx9rt)

option(DX9RT_ENABLE_TEST "Enable building dx9rt unit test option." OFF)
if(DX9RT_ENABLE_TEST AND EXISTS ${PROJECT_SOURCE_DIR}/dependencies/catch2/single_include/catch2/catch.hpp)
  add_executable(dx9rt_test ${CMAKE_CURRENT_SOURCE_DIR}/test/main.cc
                            ${CMAKE_CURRENT_SOURCE_DIR}/test/state_vector.cc)
  set_property(TARGET dx9rt_test PROPERTY CXX_STANDARD 11)
  set_property(TARGET dx9rt_test APPEND PROPERTY INCLUDE_DIRECTORIES
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/dependencies/catch2/single_include)
  target_link_libraries(dx9rt_test dx9rt)
  set_property(TARGET dx9rt_test PROPERTY FOLDER dx9rt)
  if(COMMAND catch_discover_tests)
    catch_discover_tests(dx9rt_test)
  endif()
endif()
