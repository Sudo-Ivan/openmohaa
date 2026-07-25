#
# Acceptance tests - verify feature-level behavior of combined subsystems
#

add_executable(test_acceptance
    ${SOURCE_DIR}/qcommon/tests/test_acceptance.cpp
    ${SOURCE_DIR}/qcommon/q_shared.c
    ${SOURCE_DIR}/qcommon/q_math.c
    ${SOURCE_DIR}/qcommon/crc.c
    ${SOURCE_DIR}/qcommon/common_light.c
)

target_include_directories(test_acceptance PRIVATE
    ${SOURCE_DIR}/qcommon
    ${SOURCE_DIR}
)

target_link_libraries(test_acceptance INTERFACE testing)
add_test(NAME test_acceptance COMMAND test_acceptance)
set_tests_properties(test_acceptance PROPERTIES TIMEOUT 15)
