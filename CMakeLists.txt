cmake_minimum_required(VERSION 3.20)
project(SSA_Basico LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

if(MINGW)
    add_link_options(-static-libgcc -static-libstdc++)
    get_filename_component(MINGW_BIN "${CMAKE_CXX_COMPILER}" DIRECTORY)
endif()

function(copiar_dll_de_hilos objetivo)
    if(MINGW AND EXISTS "${MINGW_BIN}/libwinpthread-1.dll")
        add_custom_command(TARGET ${objetivo} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different "${MINGW_BIN}/libwinpthread-1.dll"
                "$<TARGET_FILE_DIR:${objetivo}>")
    endif()
endfunction()

# Algoritmo y módulo de ejecución
add_library(algoritmo STATIC
        algoritmo/Instancia.cpp
        algoritmo/FuncionObjetivo.cpp
        algoritmo/OperadorReparacion.cpp
        algoritmo/Arana.cpp
        algoritmo/SSA.cpp
        algoritmo/ModuloEjecucion.cpp)
target_include_directories(algoritmo PUBLIC algoritmo)

add_executable(ssa_basico algoritmo/main.cpp)
target_link_libraries(ssa_basico PRIVATE algoritmo)
# Carpeta datos/ del proyecto
target_compile_definitions(ssa_basico PRIVATE DIR_DATOS="${CMAKE_SOURCE_DIR}/datos")
copiar_dll_de_hilos(ssa_basico)

# Pruebas con Google Test
include(FetchContent)
FetchContent_Declare(googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.17.0.zip
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(pruebas algoritmo/pruebas.cpp)
target_link_libraries(pruebas PRIVATE algoritmo GTest::gtest_main)
target_compile_definitions(pruebas PRIVATE CARPETA_DATOS="${CMAKE_SOURCE_DIR}/datos")
copiar_dll_de_hilos(pruebas)

enable_testing()
include(GoogleTest)
gtest_discover_tests(pruebas)
