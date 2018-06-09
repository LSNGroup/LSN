CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(RepeaterNode)

SET(CMAKE_CXX_LINK_FLAGS "${CMAKE_CXX_LINK_FLAGS} -L../../../revolver-master/cmake/lib -ludtx")

# includes
SET(includes
    "../../../revolver-master/common/udt/"
    "../"
    "../../ShareDir/"
    "../../RepeaterNode/"
    )
INCLUDE_DIRECTORIES(${includes})

SET(EXECUTABLE_OUTPUT_PATH "${CMAKE_SOURCE_DIR}/bin")

# sources
FILE(GLOB sources_cpp "../../RepeaterNode/*.cpp")

# targets
ADD_EXECUTABLE(RepeaterNode ${sources_cpp})
TARGET_LINK_LIBRARIES(RepeaterNode ShareDir udtx pthread)
