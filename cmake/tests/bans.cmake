#
# Ban parsing unit tests (standalone, no engine dependencies)
#

add_executable(test_bans
    ${SOURCE_DIR}/server/tests/test_bans.cpp
)

add_test(NAME test_bans COMMAND test_bans)
set_tests_properties(test_bans PROPERTIES TIMEOUT 15)
