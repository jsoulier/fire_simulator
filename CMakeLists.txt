cmake_minimum_required(VERSION 3.24)
project(fire_simulator)

if(WIN32)
    set(VCPKG_TARGET_TRIPLET x64-windows-static)
elseif(UNIX)
    set(VCPKG_TARGET_TRIPLET x64-linux)
endif()
set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/lib/vcpkg/scripts/buildsystems/vcpkg.cmake")

set(GLM_ENABLE ON)
set(SPDLOG_ENABLE ON)
set(TRACY_ENABLE ON)

find_package(GDAL CONFIG REQUIRED)
find_file(PROJ_DB NAMES proj.db PATH_SUFFIXES share/proj REQUIRED)
configure_file(${PROJ_DB} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/proj.db COPYONLY)

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
