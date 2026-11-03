# This file was automatically generated for projects
# without default 'CMakeLists.txt' file.

FILE(GLOB_RECURSE app_sources 
    ${CMAKE_CURRENT_LIST_DIR}/*.c 
    ${CMAKE_CURRENT_LIST_DIR}/*.cpp
)

set(RFBP_DIR "${CMAKE_CURRENT_LIST_DIR}/../../libs/rfbp")


set(RFBP_SOURCES
    "${RFBP_DIR}/src/speck.cpp"
    "${RFBP_DIR}/src/packet.cpp"
    "${RFBP_DIR}/src/telemetry.cpp"
    "${RFBP_DIR}/src/fsm.cpp"
)

idf_component_register(
    SRCS 
        ${app_sources}
        ${RFBP_SOURCES}
    INCLUDE_DIRS 
        "." 
        "modules" 
        "${RFBP_DIR}/lib"
)