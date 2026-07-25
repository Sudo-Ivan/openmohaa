#
# Adversarial tests - probe for edge-case bugs, UB, and crashes
#

add_executable(test_adversarial
    ${SOURCE_DIR}/qcommon/tests/test_adversarial.cpp
    ${SOURCE_DIR}/qcommon/q_shared.c
    ${SOURCE_DIR}/qcommon/q_math.c
    ${SOURCE_DIR}/qcommon/common_light.c
)

target_include_directories(test_adversarial PRIVATE
    ${SOURCE_DIR}/qcommon
    ${SOURCE_DIR}
)

target_link_libraries(test_adversarial INTERFACE testing)
add_test(NAME test_adversarial COMMAND test_adversarial)
set_tests_properties(test_adversarial PROPERTIES TIMEOUT 15)
