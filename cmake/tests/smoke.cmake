#
# Smoke tests - verify core components initialize and function without crashing
#

add_executable(test_smoke
    ${SOURCE_DIR}/qcommon/tests/test_smoke.cpp
    ${SOURCE_DIR}/qcommon/q_shared.c
    ${SOURCE_DIR}/qcommon/q_math.c
    ${SOURCE_DIR}/qcommon/crc.c
    ${SOURCE_DIR}/qcommon/common_light.c
)

target_include_directories(test_smoke PRIVATE
    ${SOURCE_DIR}/qcommon
    ${SOURCE_DIR}
)

target_link_libraries(test_smoke INTERFACE testing)
add_test(NAME test_smoke COMMAND test_smoke)
set_tests_properties(test_smoke PROPERTIES TIMEOUT 15)
