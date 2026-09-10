cmake_minimum_required(VERSION 3.24)
project(EstibordoNavigatorNative VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

if(NOT WIN32)
  message(FATAL_ERROR "Estibordo Navigator Native currently targets Windows.")
endif()

add_executable(EstibordoNavigator WIN32
  src/main.cpp
  src/navigation_state.cpp
  src/nmea_parser.cpp
  src/udp_nmea_receiver.cpp
)

target_include_directories(EstibordoNavigator PRIVATE src)
target_compile_definitions(EstibordoNavigator PRIVATE UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN)
target_link_libraries(EstibordoNavigator PRIVATE ws2_32)

if(MSVC)
  target_compile_options(EstibordoNavigator PRIVATE /W4 /permissive- /EHsc)
endif()

install(TARGETS EstibordoNavigator RUNTIME DESTINATION .)
