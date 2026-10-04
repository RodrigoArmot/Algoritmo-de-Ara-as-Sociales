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
endif()

set(FUENTES algoritmo/Instancia.cpp algoritmo/Modelo.cpp algoritmo/SSA.cpp)

add_executable(ssa_basico algoritmo/main.cpp ${FUENTES})
# Carpeta datos/ del proyecto: main.cpp la usa por defecto si no se pasan argumentos.
target_compile_definitions(ssa_basico PRIVATE DIR_DATOS="${CMAKE_SOURCE_DIR}/datos")

# Pruebas desactivadas para no confundirlas con el programa. Para activarlas, quitar los '#':
# add_executable(pruebas algoritmo/pruebas.cpp ${FUENTES})
# enable_testing()
# add_test(NAME pruebas COMMAND pruebas ${CMAKE_SOURCE_DIR}/datos/completa)
