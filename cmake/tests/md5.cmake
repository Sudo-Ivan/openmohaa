#
# MD5 unit tests (standalone, no engine dependencies)
#

add_executable(test_md5
    ${SOURCE_DIR}/fgame/tests/test_md5.cpp
    ${SOURCE_DIR}/fgame/md5.cpp
)

target_include_directories(test_md5 PRIVATE
    ${SOURCE_DIR}/fgame
)

add_test(NAME test_md5 COMMAND test_md5)
set_tests_properties(test_md5 PROPERTIES TIMEOUT 15)
