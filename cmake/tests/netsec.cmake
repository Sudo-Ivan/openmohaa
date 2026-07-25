#
# Network security unit tests
#

add_executable(test_netsec
    ${SOURCE_DIR}/qcommon/tests/test_netsec.cpp
    ${SOURCE_DIR}/qcommon/huffman.cpp
    ${SOURCE_DIR}/qcommon/q_shared.c
    ${SOURCE_DIR}/qcommon/q_math.c
    ${SOURCE_DIR}/qcommon/common_light.c
)

target_include_directories(test_netsec PRIVATE
    ${SOURCE_DIR}/qcommon
    ${SOURCE_DIR}
)

target_link_libraries(test_netsec INTERFACE testing)
add_test(NAME test_netsec COMMAND test_netsec)
set_tests_properties(test_netsec PROPERTIES TIMEOUT 15)
