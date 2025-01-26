include(FetchContent)

include(FetchContent)
FetchContent_Populate(
        cli11_proj
        QUIET
        GIT_REPOSITORY https://github.com/CLIUtils/CLI11.git
        GIT_TAG main
        #SOURCE_DIR     cli11_proj
)

#add_subdirectory(${cli11_proj_SOURCE_DIR} ${cli11_proj_SOURCE_DIR}/build)
#target_link_libraries(redis-like PRIVATE CLI11::CLI11)

add_library(cli11 INTERFACE)
target_include_directories(cli11 INTERFACE ${cli11_proj_SOURCE_DIR}/include)
