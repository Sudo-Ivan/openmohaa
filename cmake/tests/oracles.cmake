#
# Oracle tests - verify deterministic functions against known-good reference values
#

add_executable(test_oracles
    ${SOURCE_DIR}/qcommon/tests/test_oracles.cpp
    ${SOURCE_DIR}/qcommon/q_shared.c
    ${SOURCE_DIR}/qcommon/q_math.c
    ${SOURCE_DIR}/qcommon/crc.c
    ${SOURCE_DIR}/qcommon/common_light.c
)

target_include_directories(test_oracles PRIVATE
    ${SOURCE_DIR}/qcommon
    ${SOURCE_DIR}
)

target_link_libraries(test_oracles INTERFACE testing)
add_test(NAME test_oracles COMMAND test_oracles)
set_tests_properties(test_oracles PROPERTIES TIMEOUT 15)
