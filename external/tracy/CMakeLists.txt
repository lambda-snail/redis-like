if(USE_TRACY)
    FetchContent_Declare (
            tracy
            GIT_REPOSITORY https://github.com/wolfpld/tracy.git
            GIT_TAG master
            GIT_SHALLOW TRUE
            GIT_PROGRESS TRUE
    )
    FetchContent_MakeAvailable(tracy)

    add_compile_definitions(TRACY_ENABLE)

    target_link_libraries(redis-like PUBLIC TracyClient)
endif()