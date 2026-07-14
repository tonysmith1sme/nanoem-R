option(NANOEM_ENABLE_SOKOL_GLES3_COMPAT "Enable building sokol_gles3 with compatibility profile" OFF)
option(NANOEM_ENABLE_SOKOL_WGPU "Enable building sokol_wgpu" OFF)

function(nanoem_emapp_build_sokol_noop)
  if(NOT NANOEM_ENABLE_STATIC_BUNDLE_PLUGIN)
    add_library(sokol_noop ${_library_type}
                ${CMAKE_CURRENT_SOURCE_DIR}/sokol_noop.c
                $<$<BOOL:${WIN32}>:${CMAKE_CURRENT_SOURCE_DIR}/sokol_noop.def>)
    set_property(TARGET sokol_noop PROPERTY FOLDER nanoem/sokol)
    target_include_directories(sokol_noop PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/dependencies)
    set_target_properties(sokol_noop PROPERTIES OUTPUT_NAME sokol_noop PREFIX "" DEFINE_SYMBOL "")
    nanoem_emapp_plugin_install(sokol_noop)
    message(STATUS "[emapp/bundle] setup sokol noop backend renderer plugin")
  endif()
endfunction()

function(nanoem_emapp_build_sokol_glcore33)
  if(NOT (NANOEM_ENABLE_STATIC_BUNDLE_PLUGIN OR (MSVC AND "${NANOEM_TARGET_ARCHITECTURE}" STREQUAL "arm64")))
    add_library(sokol_glcore33 ${_library_type}
                ${CMAKE_CURRENT_SOURCE_DIR}/sokol_glcore33.c
                $<$<BOOL:${WIN32}>:${CMAKE_CURRENT_SOURCE_DIR}/sokol_glcore33.def>)
    set_property(TARGET sokol_glcore33 PROPERTY FOLDER nanoem/sokol)
    target_compile_definitions(sokol_glcore33 PRIVATE SOKOL_DEBUG=$<BOOL:${SOKOL_ENABLE_DEBUG}> SOKOL_EXTERNAL_GL_LOADER)
    target_include_directories(sokol_glcore33 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/dependencies)
    set_target_properties(sokol_glcore33 PROPERTIES OUTPUT_NAME sokol_glcore33 PREFIX "" DEFINE_SYMBOL "")
    nanoem_emapp_plugin_install(sokol_glcore33)
    message(STATUS "[emapp/bundle] setup sokol OpenGL 3.3 Core Profile backend renderer plugin")
  endif()
endfunction()

function(nanoem_emapp_build_sokol_gles3)
  find_library(GLES3_LIBRARY_PATH NAMES GLESv3)
  find_path(GLES3_INCLUDE_DIR NAMES GLES3/gl3.h)
  mark_as_advanced(GLES3_LIBRARY_PATH GLES3_INCLUDE_DIR)
  if((NANOEM_ENABLE_SOKOL_GLES3_COMPAT OR EMSCRIPTEN) OR (GLES3_LIBRARY_PATH AND GLES3_INCLUDE_DIR) AND NOT APPLE)
    add_library(sokol_gles3 ${_library_type}
                ${CMAKE_CURRENT_SOURCE_DIR}/sokol_gles3.c
                $<$<BOOL:${WIN32}>:${CMAKE_CURRENT_SOURCE_DIR}/sokol_gles3.def>)
    set_property(TARGET sokol_gles3 PROPERTY FOLDER nanoem/sokol)
    target_compile_definitions(sokol_gles3 PRIVATE SOKOL_DEBUG=$<BOOL:${SOKOL_ENABLE_DEBUG}> $<$<BOOL:${NANOEM_ENABLE_SOKOL_GLES3_COMPAT}>:NANOEM_ENABLE_SOKOL_GLES3_COMPAT> SOKOL_EXTERNAL_GL_LOADER)
    target_include_directories(sokol_gles3 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/dependencies)
    set_target_properties(sokol_gles3 PROPERTIES OUTPUT_NAME sokol_gles3 PREFIX "" DEFINE_SYMBOL "")
    if(GLES3_INCLUDE_DIR)
      target_include_directories(sokol_gles3 PRIVATE ${GLES3_INCLUDE_DIR})
    endif()
    if(GLES3_LIBRARY_PATH)
      target_link_libraries(sokol_gles3 ${GLES3_LIBRARY_PATH})
    endif()
    nanoem_emapp_plugin_install(sokol_gles3)
    message(STATUS "[emapp/bundle] setup sokol OpenGL ES 3.0 backend renderer plugin")
  endif()
endfunction()

function(nanoem_emapp_build_sokol_d3d11)
  add_library(sokol_d3d11 ${_library_type}
              ${CMAKE_CURRENT_SOURCE_DIR}/sokol_d3d11.c
              ${CMAKE_CURRENT_SOURCE_DIR}/sokol_d3d11.def)
  set_property(TARGET sokol_d3d11 PROPERTY FOLDER nanoem/sokol)
  target_compile_definitions(sokol_d3d11 PRIVATE SOKOL_DEBUG=$<BOOL:${SOKOL_ENABLE_DEBUG}> SOKOL_STATIC=$<BOOL:${NANOEM_ENABLE_STATIC_BUNDLE_PLUGIN}> SOKOL_EXTERNAL_GL_LOADER)
  target_include_directories(sokol_d3d11 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/dependencies)
  set_target_properties(sokol_d3d11 PROPERTIES OUTPUT_NAME sokol_d3d11 PREFIX "" DEFINE_SYMBOL "")
  target_link_libraries(sokol_d3d11 d3d11 dxgi dxguid)
  nanoem_emapp_plugin_install(sokol_d3d11)
  message(STATUS "[emapp/bundle] setup sokol D3D11 backend renderer plugin")
endfunction()

function(nanoem_emapp_build_sokol_metal)
  add_library(sokol_metal_macos ${_library_type} ${CMAKE_CURRENT_SOURCE_DIR}/sokol_metal_$<IF:$<BOOL:${IOS}>,ios,macos>.m)
  set_property(TARGET sokol_metal_macos PROPERTY FOLDER nanoem/sokol)
  target_compile_definitions(sokol_metal_macos PRIVATE SOKOL_DEBUG=$<BOOL:${SOKOL_ENABLE_DEBUG}>)
  target_include_directories(sokol_metal_macos PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/dependencies)
  set_target_properties(sokol_metal_macos PROPERTIES OUTPUT_NAME sokol_metal_macos PREFIX "" DEFINE_SYMBOL "")
  target_link_libraries(sokol_metal_macos "-framework Metal")
  nanoem_emapp_plugin_install(sokol_metal_macos)
  message(STATUS "[emapp/bundle] setup sokol Metal backend renderer plugin")
endfunction()


function(nanoem_emapp_build_sokol_wgpu)
  if(NANOEM_ENABLE_SOKOL_WGPU)
    set(DAWN_BASE_PATH ${PROJECT_SOURCE_DIR}/dependencies/dawn)
    # Support both classic Dawn layout and install-root / system layouts.
    find_path(WEBGPU_INCLUDE_DIR
      NAMES webgpu/webgpu.h dawn/webgpu.h webgpu.h
      PATH_SUFFIXES
        include
        out/Release/gen/src/include
        gen/include
      PATHS
        ${DAWN_BASE_PATH}
        ${DAWN_BASE_PATH}/install-root
        $ENV{DAWN_ROOT}
        $ENV{WEBGPU_ROOT}
    )
    find_library(WEBGPU_DAWN_NATIVE_LIBRARY_PATH
      NAMES dawn_native webgpu_dawn dawn
      PATH_SUFFIXES lib out/Release out/Release/lib
      PATHS ${DAWN_BASE_PATH} ${DAWN_BASE_PATH}/install-root $ENV{DAWN_ROOT} $ENV{WEBGPU_ROOT})
    find_library(WEBGPU_DAWN_PROC_LIBRARY_PATH
      NAMES dawn_proc webgpu_dawn dawn
      PATH_SUFFIXES lib out/Release out/Release/lib
      PATHS ${DAWN_BASE_PATH} ${DAWN_BASE_PATH}/install-root $ENV{DAWN_ROOT} $ENV{WEBGPU_ROOT})
    find_library(WEBGPU_DAWN_WIRE_LIBRARY_PATH
      NAMES dawn_wire
      PATH_SUFFIXES lib out/Release out/Release/lib
      PATHS ${DAWN_BASE_PATH} ${DAWN_BASE_PATH}/install-root $ENV{DAWN_ROOT} $ENV{WEBGPU_ROOT})
    if(WEBGPU_INCLUDE_DIR AND WEBGPU_DAWN_NATIVE_LIBRARY_PATH)
      add_library(sokol_wgpu ${_library_type} ${CMAKE_CURRENT_SOURCE_DIR}/sokol_wgpu.c)
      set_property(TARGET sokol_wgpu PROPERTY FOLDER nanoem/sokol)
      target_compile_definitions(sokol_wgpu PRIVATE SOKOL_DEBUG=$<BOOL:${SOKOL_ENABLE_DEBUG}> SOKOL_EXTERNAL_GL_LOADER)
      target_include_directories(sokol_wgpu PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/dependencies ${WEBGPU_INCLUDE_DIR})
      set_target_properties(sokol_wgpu PROPERTIES OUTPUT_NAME sokol_wgpu PREFIX "" DEFINE_SYMBOL "")
      set(_wgpu_libs ${WEBGPU_DAWN_NATIVE_LIBRARY_PATH})
      if(WEBGPU_DAWN_PROC_LIBRARY_PATH)
        list(APPEND _wgpu_libs ${WEBGPU_DAWN_PROC_LIBRARY_PATH})
      endif()
      if(WEBGPU_DAWN_WIRE_LIBRARY_PATH)
        list(APPEND _wgpu_libs ${WEBGPU_DAWN_WIRE_LIBRARY_PATH})
      endif()
      target_link_libraries(sokol_wgpu ${_wgpu_libs})
      nanoem_emapp_plugin_install(sokol_wgpu)
      set(NANOEM_HAS_WGPU ON CACHE BOOL "WGPU/Dawn backend available" FORCE)
      message(STATUS "[emapp/bundle] setup sokol WGPU backend renderer plugin")
    else()
      set(NANOEM_HAS_WGPU OFF CACHE BOOL "WGPU/Dawn backend available" FORCE)
      message(WARNING "[emapp/bundle] NANOEM_ENABLE_SOKOL_WGPU=ON but Dawn/WebGPU was not found under dependencies/dawn; sokol_wgpu skipped")
    endif()
  else()
    set(NANOEM_HAS_WGPU OFF CACHE BOOL "WGPU/Dawn backend available" FORCE)
  endif()
endfunction()

function(nanoem_emapp_build_sokol)
  nanoem_emapp_plugin_set_library_type()
  nanoem_emapp_build_sokol_noop()
  nanoem_emapp_build_sokol_glcore33()
  nanoem_emapp_build_sokol_gles3()
  nanoem_emapp_build_sokol_wgpu()
  if(WIN32)
    nanoem_emapp_build_sokol_d3d11()
    if(TARGET sokol_glcore33)
      target_link_libraries(sokol_glcore33 opengl32)
    endif()
    if(TARGET sokol_gles3)
      target_link_libraries(sokol_gles3 opengl32)
    endif()
  elseif(APPLE)
    nanoem_emapp_build_sokol_metal()
  endif()
endfunction()

nanoem_emapp_build_sokol()
