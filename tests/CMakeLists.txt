find_package(Python3 REQUIRED COMPONENTS Interpreter)

add_executable(zgent_lifecycle_probe runtime_lifecycle_probe.cpp)
target_link_libraries(zgent_lifecycle_probe PRIVATE zgent_core)
if(TARGET cxx_options)
    target_link_libraries(zgent_lifecycle_probe PRIVATE cxx_options)
endif()

foreach(scenario sigint sigterm configuration startup_failure lifecycle)
    add_test(NAME zgent_runtime_${scenario}
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/runtime_test.py
            --executable $<TARGET_FILE:zgent>
            --probe $<TARGET_FILE:zgent_lifecycle_probe>
            --config ${PROJECT_SOURCE_DIR}/doc/zgent_conf.json
            --hpx-config ${PROJECT_SOURCE_DIR}/doc/zgent_hpx.ini
            --scenario ${scenario})
    set_tests_properties(zgent_runtime_${scenario} PROPERTIES TIMEOUT 90)
endforeach()
