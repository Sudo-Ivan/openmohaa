#
# Unit tests
#

add_executable(test_deferredsave
    ${SOURCE_DIR}/qcommon/tests/test_deferredsave.cpp
    ${SOURCE_DIR}/corepp/lz77.cpp
    ${SOURCE_DIR}/qcommon/q_shared.c
    ${SOURCE_DIR}/qcommon/common_light.c
)

target_link_libraries(test_deferredsave INTERFACE testing)
add_test(NAME test_deferredsave COMMAND test_deferredsave)
set_tests_properties(test_deferredsave PROPERTIES TIMEOUT 15)
