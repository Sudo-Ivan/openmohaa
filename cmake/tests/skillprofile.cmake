#
# Unit tests
#

add_executable(test_skillprofile
    ${SOURCE_DIR}/qcommon/tests/test_skillprofile.cpp
    ${SOURCE_DIR}/qcommon/q_shared.c
    ${SOURCE_DIR}/qcommon/common_light.c
)

target_link_libraries(test_skillprofile INTERFACE testing)
add_test(NAME test_skillprofile COMMAND test_skillprofile)
set_tests_properties(test_skillprofile PROPERTIES TIMEOUT 15)
