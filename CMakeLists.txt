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

find_package(GDAL CONFIG REQUIRED)
find_file(PROJ_DB NAMES proj.db PATH_SUFFIXES share/proj REQUIRED)
configure_file(${PROJ_DB} ${BINARY_DIR}/proj.db COPYONLY)

set(GLM_ENABLE ON)
set(SPDLOG_ENABLE ON)
set(TRACY_ENABLE ON)

add_subdirectory(lib/SDL)
add_subdirectory(lib/behave)
add_subdirectory(lib/cadmium)
add_subdirectory(lib/geo_names)
add_subdirectory(lib/glm)
add_subdirectory(lib/imgui)
add_subdirectory(lib/spdlog)
add_subdirectory(lib/tracy)

add_executable(fire_simulator
    src/console.cpp
    src/fire_fuel_model.cpp
    src/fire_model.cpp
    src/fire_model_logger.cpp
    src/fire_results.cpp
    src/fire_simulator.cpp
    src/main.cpp
    src/service.cpp
    src/service_custom.cpp
    src/service_esa_worldcover.cpp
    src/service_open_topography.cpp
    src/worker.cpp
)
target_include_directories(fire_simulator PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(fire_simulator PRIVATE
    GDAL::GDAL
    SDL3::SDL3
    behave
    cadmium
    geo_names
    glm::glm
    imgui
    spdlog::spdlog
)
set_target_properties(fire_simulator PROPERTIES CXX_STANDARD 23)

# For some reason GDAL and dependencies aren't being placed next to the executable
add_custom_command(TARGET fire_simulator POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_BINARY_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}/$<IF:$<CONFIG:Debug>,debug/bin,bin>
        $<TARGET_FILE_DIR:fire_simulator>
)
