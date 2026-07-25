#
# Unit tests
#

add_executable(test_sp_stealth
    ${SOURCE_DIR}/qcommon/tests/test_sp_stealth.cpp
    ${SOURCE_DIR}/qcommon/sp_stealth_rear.c
    ${SOURCE_DIR}/qcommon/q_shared.c
    ${SOURCE_DIR}/qcommon/common_light.c
)

target_link_libraries(test_sp_stealth INTERFACE testing)
add_test(NAME test_sp_stealth COMMAND test_sp_stealth)
set_tests_properties(test_sp_stealth PROPERTIES TIMEOUT 15)
