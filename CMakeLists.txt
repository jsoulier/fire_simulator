cmake_minimum_required(VERSION 3.24)
project(fire_simulator)

option(USE_VENDORED "Use vendored libraries" ON)
option(USE_TRACY "Enable Tracy profiling zones" OFF)
set(GLM_BUILD_LIBRARY OFF)
set(USE_SPDLOG ON)

if(USE_VENDORED)
    add_subdirectory(lib/behave)
    add_subdirectory(lib/cadmium)
    add_subdirectory(lib/glm)
    add_subdirectory(lib/spdlog)
    if(USE_TRACY)
        add_subdirectory(lib/tracy)
    endif()
endif()

add_library(fire_simulator STATIC
    src/fire_fuel_model.cpp
    src/fire_model.cpp
    src/fire_model_logger.cpp
    src/fire_simulator.cpp
)
target_include_directories(fire_simulator PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(fire_simulator PRIVATE behave cadmium glm::glm spdlog::spdlog)
target_compile_definitions(fire_simulator PRIVATE GLM_ENABLE_EXPERIMENTAL)
set_target_properties(fire_simulator PROPERTIES CXX_STANDARD 23)

if(USE_TRACY)
    target_link_libraries(fire_simulator PRIVATE Tracy::TracyClient)
    target_compile_definitions(fire_simulator PRIVATE USE_TRACY=1)
endif()
