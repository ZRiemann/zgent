add_library(zgent_core STATIC
    app.cpp
    hpx_runtime.cpp
)

target_include_directories(zgent_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(zgent_core PUBLIC
    zpp::zpp_nng
    zpp::hpx
)

if(TARGET cxx_options)
    target_link_libraries(zgent_core PRIVATE cxx_options)
endif()

add_executable(zgent
    main.cpp
)

target_link_libraries(zgent PRIVATE
    zgent_core
)

if(TARGET cxx_options)
    target_link_libraries(zgent PRIVATE cxx_options)
endif()

if(COMMAND add_run_target)
    add_run_target(zgent ARGS
        ${PROJECT_SOURCE_DIR}/doc/zgent_conf.json zgent
        --hpx:config=${PROJECT_SOURCE_DIR}/doc/zgent_hpx.ini)
endif()
