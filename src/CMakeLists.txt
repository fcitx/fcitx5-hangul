set( fcitx_hangul_sources
    engine.cpp
    )

add_fcitx5_addon(hangul ${fcitx_hangul_sources})
target_link_libraries(hangul Fcitx5::Core Fcitx5::Config ${HANGUL_TARGET})
install(TARGETS hangul DESTINATION "${CMAKE_INSTALL_LIBDIR}/fcitx5")
fcitx5_translate_desktop_file(hangul.conf.in hangul.conf)
configure_file(hangul-addon.conf.in.in hangul-addon.conf.in)
fcitx5_translate_desktop_file("${CMAKE_CURRENT_BINARY_DIR}/hangul-addon.conf.in" hangul-addon.conf)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/hangul.conf" DESTINATION "${FCITX_INSTALL_PKGDATADIR}/inputmethod" COMPONENT config)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/hangul-addon.conf" RENAME hangul.conf DESTINATION "${FCITX_INSTALL_PKGDATADIR}/addon" COMPONENT config)
