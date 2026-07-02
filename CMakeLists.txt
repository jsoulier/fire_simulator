cmake_minimum_required(VERSION 3.24)
project(fire_simulator)

set(GLM_ENABLE ON)
set(SPDLOG_ENABLE ON)
set(TRACY_ENABLE ON)

add_subdirectory(lib/behave)
add_subdirectory(lib/cadmium)
add_subdirectory(lib/glm)
add_subdirectory(lib/spdlog)
add_subdirectory(lib/tracy)
add_library(fire_simulator STATIC
    src/fire_fuel_model.cpp
    src/fire_model.cpp
    src/fire_model_logger.cpp
    src/fire_simulator.cpp
)
target_include_directories(fire_simulator PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(fire_simulator PRIVATE
    behave
    cadmium
    glm::glm
    spdlog::spdlog
)
set_target_properties(fire_simulator PROPERTIES CXX_STANDARD 23)
