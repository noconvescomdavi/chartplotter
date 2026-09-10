cmake_minimum_required(VERSION 3.20)
project(estibordo_nv2_core LANGUAGES CXX)

add_library(estibordo_nv2_core STATIC
  src/nv2_reader.cpp
  src/navsdk_bridge.cpp
  src/backend_manager.cpp
)
target_include_directories(estibordo_nv2_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_compile_features(estibordo_nv2_core PUBLIC cxx_std_17)

add_executable(nv2probe src/nv2_probe.cpp)
target_link_libraries(nv2probe PRIVATE estibordo_nv2_core)

add_executable(navsdkprobe src/navsdk_probe.cpp)
target_link_libraries(navsdkprobe PRIVATE estibordo_nv2_core)

add_executable(nv2backendprobe src/backend_probe.cpp)
target_link_libraries(nv2backendprobe PRIVATE estibordo_nv2_core)

enable_testing()
