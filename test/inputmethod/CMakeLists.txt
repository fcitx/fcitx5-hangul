add_custom_target(copy-im DEPENDS hangul.conf.in-fmt)
add_custom_command(TARGET copy-im COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_BINARY_DIR}/src/hangul.conf ${CMAKE_CURRENT_BINARY_DIR}/hangul.conf)
