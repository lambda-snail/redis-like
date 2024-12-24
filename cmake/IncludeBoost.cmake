include(FetchContent)

#FetchContent_Declare(
#        Boost
#        URL https://github.com/boostorg/boost/releases/download/boost-1.84.0/boost-1.84.0.tar.xz
#        URL_MD5 893b5203b862eb9bbd08553e24ff146a
#        DOWNLOAD_EXTRACT_TIMESTAMP ON
#)

set(BOOST_ENABLE_CMAKE ON)
FetchContent_Declare(
        Boost
        GIT_REPOSITORY https://github.com/boostorg/boost.git
        GIT_TAG boost-1.84.0
)

set(BOOST_INCLUDE_LIBRARIES unordered core)
FetchContent_MakeAvailable(Boost)

#add_subdirectory(${boost_unordered_SOURCE_DIR} EXCLUDE_FROM_ALL)

#add_subdirectory(deps/boost)