SET(pwd ${CMAKE_CURRENT_LIST_DIR})

SET(src "")
AUX_SOURCE_DIRECTORY(${pwd} src)
add_executable(sea-of-thieves ${src})
message(STATUS "minecraft added ${src}")

target_link_libraries(sea-of-thieves ${stdgl_libraries})
FIND_PACKAGE(JPEG REQUIRED)
TARGET_LINK_LIBRARIES(sea-of-thieves ${JPEG_LIBRARIES})
