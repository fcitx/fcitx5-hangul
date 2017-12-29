install(FILES symbol.txt DESTINATION ${FCITX_INSTALL_PKGDATADIR}/hangul/)
install(FILES hangul.png RENAME fcitx-hangul.png DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/64x64/apps/)
install(FILES hanja_active.png RENAME fcitx-hanja-active.png DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/48x48/status/)
install(FILES hanja_inactive.png RENAME fcitx-hanja-inactive.png DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/48x48/status/)
