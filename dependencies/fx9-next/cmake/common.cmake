function(fx9next_cmake_get_install_path _name _base_path _debug_path _release_path)
  if(DEFINED ENV{FX9_BUILD_DEPENDENCIES_DIRECTORY})
    set(base_path $ENV{FX9_BUILD_DEPENDENCIES_DIRECTORY}/${_name})
  elseif(DEFINED ENV{NANOEM_BUILD_DEPENDENCIES_DIRECTORY})
    set(base_path $ENV{NANOEM_BUILD_DEPENDENCIES_DIRECTORY}/${_name})
  else()
    set(base_path ${PROJECT_SOURCE_DIR}/dependencies/${_name})
  endif()
  get_filename_component(base_path ${base_path} ABSOLUTE)
  if(DEFINED ENV{FX9_TARGET_SYSTEM_NAME})
    set(_system_name $ENV{FX9_TARGET_SYSTEM_NAME})
  else()
    string(TOLOWER ${CMAKE_SYSTEM_NAME} _system_name)
  endif()
  if(DEFINED ENV{NANOEM_TARGET_COMPILER})
    set(_target_compiler $ENV{NANOEM_TARGET_COMPILER})
  elseif(DEFINED FX9_TARGET_COMPILER)
    set(_target_compiler ${FX9_TARGET_COMPILER})
  else()
    set(_target_compiler clang)
  endif()
  if(DEFINED ENV{NANOEM_TARGET_ARCHITECTURES})
    set(_arch $ENV{NANOEM_TARGET_ARCHITECTURES})
  elseif(DEFINED ENV{NANOEM_TARGET_ARCHITECTURE})
    set(_arch $ENV{NANOEM_TARGET_ARCHITECTURE})
  elseif(DEFINED FX9_TARGET_ARCHITECTURE)
    set(_arch ${FX9_TARGET_ARCHITECTURE})
  else()
    set(_arch ${NANOEM_TARGET_ARCHITECTURE})
  endif()
  set(${_base_path} ${base_path} PARENT_SCOPE)
  set(common_prefix_path ${base_path}/out/${_system_name}/${_target_compiler}/${_arch})
  set(${_debug_path} ${common_prefix_path}/debug/install-root PARENT_SCOPE)
  set(${_release_path} ${common_prefix_path}/release/install-root PARENT_SCOPE)
endfunction()

if(NOT SPIRV_CROSS_CORE_LIBRARY_RELEASE)
  fx9next_cmake_get_install_path("spirv-cross" SPIRV_CROSS_BASE_PATH SPIRV_CROSS_INSTALL_PATH_DEBUG SPIRV_CROSS_INSTALL_PATH_RELEASE)
  find_library(SPIRV_CROSS_CORE_LIBRARY_DEBUG NAMES spirv-cross-cored spirv-cross-core PATH_SUFFIXES lib PATHS ${SPIRV_CROSS_INSTALL_PATH_DEBUG} NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH)
  find_library(SPIRV_CROSS_CPP_LIBRARY_DEBUG NAMES spirv-cross-cppd spirv-cross-cpp PATH_SUFFIXES lib PATHS ${SPIRV_CROSS_INSTALL_PATH_DEBUG} NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH)
  find_library(SPIRV_CROSS_GLSL_LIBRARY_DEBUG NAMES spirv-cross-glsld spirv-cross-glsl PATH_SUFFIXES lib PATHS ${SPIRV_CROSS_INSTALL_PATH_DEBUG} NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH)
  find_library(SPIRV_CROSS_HLSL_LIBRARY_DEBUG NAMES spirv-cross-hlsld spirv-cross-hlsl PATH_SUFFIXES lib PATHS ${SPIRV_CROSS_INSTALL_PATH_DEBUG} NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH)
  find_library(SPIRV_CROSS_MSL_LIBRARY_DEBUG NAMES spirv-cross-msld spirv-cross-msl PATH_SUFFIXES lib PATHS ${SPIRV_CROSS_INSTALL_PATH_DEBUG} NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH)
  find_library(SPIRV_CROSS_CORE_LIBRARY_RELEASE NAMES spirv-cross-core PATH_SUFFIXES lib PATHS ${SPIRV_CROSS_INSTALL_PATH_RELEASE} NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH)
  find_library(SPIRV_CROSS_CPP_LIBRARY_RELEASE NAMES spirv-cross-cpp PATH_SUFFIXES lib PATHS ${SPIRV_CROSS_INSTALL_PATH_RELEASE} NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH)
  find_library(SPIRV_CROSS_GLSL_LIBRARY_RELEASE NAMES spirv-cross-glsl PATH_SUFFIXES lib PATHS ${SPIRV_CROSS_INSTALL_PATH_RELEASE} NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH)
  find_library(SPIRV_CROSS_HLSL_LIBRARY_RELEASE NAMES spirv-cross-hlsl PATH_SUFFIXES lib PATHS ${SPIRV_CROSS_INSTALL_PATH_RELEASE} NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH)
  find_library(SPIRV_CROSS_MSL_LIBRARY_RELEASE NAMES spirv-cross-msl PATH_SUFFIXES lib PATHS ${SPIRV_CROSS_INSTALL_PATH_RELEASE} NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH)
  find_path(SPIRV_CROSS_INCLUDE_DIR NAMES spirv_cross/spirv_cross.hpp PATH_SUFFIXES include PATHS ${SPIRV_CROSS_INSTALL_PATH_RELEASE} NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH)
endif()

if(NOT DEFINED FX9NEXT_PATH)
  get_filename_component(FX9NEXT_PATH ${CMAKE_CURRENT_LIST_DIR}/.. ABSOLUTE)
endif()
if(NOT DEFINED FX9_DEPENDENCIES_PATH)
  get_filename_component(FX9_DEPENDENCIES_PATH ${FX9NEXT_PATH}/.. ABSOLUTE)
endif()
if(NOT DEFINED FX9_PROTOBUF_PATH)
  get_filename_component(FX9_PROTOBUF_PATH ${FX9NEXT_PATH}/../../emapp/src/protoc ABSOLUTE)
endif()

if(NOT TARGET fx9_protobuf)
  file(GLOB EFFECT_PROTO_SOURCES ${FX9_PROTOBUF_PATH}/effect*.pb-c.c)
  add_library(fx9_protobuf ${EFFECT_PROTO_SOURCES})
  set_property(TARGET fx9_protobuf PROPERTY FOLDER dependencies/effect)
  set_property(TARGET fx9_protobuf APPEND PROPERTY INCLUDE_DIRECTORIES ${FX9_DEPENDENCIES_PATH}/protobuf-c)
  if(TARGET protobuf-c)
    target_link_libraries(fx9_protobuf protobuf-c)
  endif()
endif()

set(FX9NEXT_SOURCES
  ${FX9NEXT_PATH}/src/AST.cc
  ${FX9NEXT_PATH}/src/Compiler.cc
  ${FX9NEXT_PATH}/src/Encoding.cc
  ${FX9NEXT_PATH}/src/Lexer.cc
  ${FX9NEXT_PATH}/src/Parser.cc
  ${FX9NEXT_PATH}/src/Pipeline.cc
  ${FX9NEXT_PATH}/src/Preprocessor.cc
  ${FX9NEXT_PATH}/src/ProductWriter.cc
  ${FX9NEXT_PATH}/src/RenderState.cc
  ${FX9NEXT_PATH}/src/SpirvEmitter.cc
  ${FX9NEXT_PATH}/src/Translator.cc
  ${FX9NEXT_PATH}/src/Type.cc)

add_library(fx9next ${FX9NEXT_SOURCES})
set_property(TARGET fx9next PROPERTY FOLDER dependencies)
set_property(TARGET fx9next PROPERTY CXX_STANDARD 11)
set_property(TARGET fx9next APPEND PROPERTY INCLUDE_DIRECTORIES
  ${FX9NEXT_PATH}/include
  ${SPIRV_CROSS_INCLUDE_DIR}
  ${FX9_PROTOBUF_PATH}
  ${FX9_DEPENDENCIES_PATH}/protobuf-c)
set_property(TARGET fx9next APPEND PROPERTY COMPILE_DEFINITIONS
  $<$<BOOL:${WIN32}>:_CRT_SECURE_NO_WARNINGS=1>)
target_link_libraries(fx9next fx9_protobuf
  optimized ${SPIRV_CROSS_CORE_LIBRARY_RELEASE}
  optimized ${SPIRV_CROSS_CPP_LIBRARY_RELEASE}
  optimized ${SPIRV_CROSS_GLSL_LIBRARY_RELEASE}
  optimized ${SPIRV_CROSS_HLSL_LIBRARY_RELEASE}
  optimized ${SPIRV_CROSS_MSL_LIBRARY_RELEASE}
  debug ${SPIRV_CROSS_CORE_LIBRARY_DEBUG}
  debug ${SPIRV_CROSS_CPP_LIBRARY_DEBUG}
  debug ${SPIRV_CROSS_GLSL_LIBRARY_DEBUG}
  debug ${SPIRV_CROSS_HLSL_LIBRARY_DEBUG}
  debug ${SPIRV_CROSS_MSL_LIBRARY_DEBUG})
if(NOT WIN32)
  find_package(Iconv QUIET)
  if(Iconv_FOUND)
    target_link_libraries(fx9next Iconv::Iconv)
  else()
    target_link_libraries(fx9next iconv)
  endif()
endif()
