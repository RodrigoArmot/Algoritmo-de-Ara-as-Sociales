add_library(recocido STATIC RecocidoSimulado.cpp)
target_link_libraries(recocido PUBLIC nucleo)

# Barrido de reproducción de la Tabla 5 de Guerra Ibarra et al.
add_library(barrido STATIC
    barrido/ConfiguracionBarrido.cpp
    barrido/Criterio.cpp
    barrido/EjecutorBarrido.cpp
    barrido/InformeBarrido.cpp
    barrido/ResumenBarrido.cpp
)
target_link_libraries(barrido PUBLIC recocido)

agregar_pruebas(pruebas_recocido
    pruebas/test_recocido.cpp
    pruebas/test_barrido.cpp
)
target_link_libraries(pruebas_recocido PRIVATE recocido barrido)
