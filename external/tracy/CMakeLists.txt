include(FetchContent)

FetchContent_Declare (
        tracy
        GIT_REPOSITORY https://github.com/wolfpld/tracy.git
        GIT_TAG master
        GIT_SHALLOW TRUE
        GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(tracy)

    if(USE_TRACY)
        add_compile_definitions(TRACY_ENABLE)
    else()
        #add_compile_definitions(TRACY_ENABLE 0)
        remove_definitions(TRACY_ENABLE)
        set(TRACY_ENABLE OFF)
    endif()

target_link_libraries(redis-like PUBLIC TracyClient)
#    target_link_libraries(networking PUBLIC TracyClient)