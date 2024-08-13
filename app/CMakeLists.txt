cmake_minimum_required(VERSION 3.30)

add_executable(test_app)
target_sources(test_app
    PRIVATE
    main.cpp
)
target_link_libraries(test_app
    PRIVATE
    tale::engine)

get_filename_component(shader_locations shaders ABSOLUTE)
set_source_files_properties(main.cpp PROPERTIES COMPILE_DEFINITIONS SHADER_SOURCE="${shader_locations}")