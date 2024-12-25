#find_package(Boost 1.83 REQUIRED)

add_library(server)
target_sources(server
        PUBLIC
        FILE_SET CXX_MODULES FILES
        server.cppm
        command_dispatch.cpp
)

get_cmake_property(_variableNames VARIABLES)
list (SORT _variableNames)
foreach (_variableName ${_variableNames})
    message(STATUS "${_variableName}=${${_variableName}}")
endforeach()

add_library(LambdaSnail::server ALIAS server)

target_link_libraries(server PRIVATE LambdaSnail::resp libcuckoo)