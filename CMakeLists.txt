cmake_minimum_required(VERSION 3.24)
project(fire_simulator)

set(BINARY_DIR ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${BINARY_DIR})
make_directory(${BINARY_DIR})

set(GLM_ENABLE ON)
set(SPDLOG_ENABLE ON)
set(TRACY_ENABLE ON)

find_package(GDAL REQUIRED)

add_subdirectory(lib/behave)
add_subdirectory(lib/cadmium)
add_subdirectory(lib/glm)
add_subdirectory(lib/SDL)
add_subdirectory(lib/imgui)
add_subdirectory(lib/spdlog)
add_subdirectory(lib/tracy)

add_executable(fire_simulator
    src/fire_fuel_model.cpp
    src/fire_model.cpp
    src/fire_model_logger.cpp
    src/fire_simulator.cpp
    src/main.cpp
    src/main.cpp
    src/service.cpp
    src/service_esa_worldcover.cpp
)
target_include_directories(fire_simulator PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(fire_simulator PRIVATE
    GDAL::GDAL
    SDL3::SDL3
    behave
    cadmium
    glm::glm
    imgui
    spdlog::spdlog
)
set_target_properties(fire_simulator PROPERTIES CXX_STANDARD 23)
