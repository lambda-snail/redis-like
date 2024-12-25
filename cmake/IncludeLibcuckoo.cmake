include(FetchContent)

fetchcontent_declare(
    libcuckoo
    GIT_REPOSITORY  https://github.com/efficient/libcuckoo
    GIT_TAG         v0.3.1
)

FetchContent_MakeAvailable(libcuckoo)

#add_library(libcuckoo INTERFACE)
#target_include_directories(libcuckoo INTERFACE ${libcuckoo_SOURCE_DIR})