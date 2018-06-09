CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(ShareDir)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")

# includes
SET(includes
    "../../../revolver-master/common/udt/"
    "../"
    "../../ShareDir/"
    )
INCLUDE_DIRECTORIES(${includes})

# sources
FILE(GLOB sources_cpp "../../ShareDir/*.cpp")
FILE(GLOB sources_c   "../../ShareDir/*.c")

# targets
ADD_LIBRARY (ShareDir STATIC ${sources_cpp} ${sources_c})
