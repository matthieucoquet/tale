cmake_minimum_required(VERSION 3.26)

add_executable(test_app)
target_sources(test_app
    PRIVATE
    main.cpp
)
target_link_libraries(test_app
    PRIVATE
    tale::engine)
