cmake_minimum_required(VERSION 3.20)
project(estibordo_nv2_core LANGUAGES CXX)

add_library(estibordo_nv2_core STATIC
  src/nv2_reader.cpp
  src/navsdk_bridge.cpp
  src/backend_manager.cpp
)
target_include_directories(estibordo_nv2_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_compile_features(estibordo_nv2_core PUBLIC cxx_std_17)
set_target_properties(estibordo_nv2_core PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_executable(nv2probe src/nv2_probe.cpp)
target_link_libraries(nv2probe PRIVATE estibordo_nv2_core)

add_executable(navsdkprobe src/navsdk_probe.cpp)
target_link_libraries(navsdkprobe PRIVATE estibordo_nv2_core)

add_executable(nv2backendprobe src/backend_probe.cpp)
target_link_libraries(nv2backendprobe PRIVATE estibordo_nv2_core)

# When this directory is copied below OpenCPN/plugins it is discovered by the
# upstream *_pi glob. Build the actual chart provider only in that context.
if(TARGET ocpn::opencpn)
  add_library(estibordo_nv2_pi SHARED
    src/estibordo_nv2_pi.cpp
    src/estibordo_nv2_pi.h
    src/nv2_chart.cpp
    src/nv2_chart.h
  )
  target_compile_definitions(estibordo_nv2_pi PRIVATE MAKING_PLUGIN)
  target_compile_features(estibordo_nv2_pi PRIVATE cxx_std_17)
  target_include_directories(estibordo_nv2_pi PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/include
  )
  target_link_libraries(estibordo_nv2_pi PRIVATE estibordo_nv2_core ocpn::opencpn)
  install(TARGETS estibordo_nv2_pi
    RUNTIME DESTINATION plugins
    LIBRARY DESTINATION lib/opencpn
  )
endif()

enable_testing()
