cmake_minimum_required (VERSION 3.6)

project(fcitx5-hangul VERSION 5.1.0)

find_package(ECM REQUIRED 1.0.0)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})
include(FeatureSummary)
include(GNUInstallDirs)
include(ECMUninstallTarget)

find_package(Fcitx5Core 5.0.2 REQUIRED)
find_package(Gettext REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(Hangul IMPORTED_TARGET "libhangul>=0.0.12" REQUIRED)

include("${FCITX_INSTALL_CMAKECONFIG_DIR}/Fcitx5Utils/Fcitx5CompilerSettings.cmake")
add_definitions(-DFCITX_GETTEXT_DOMAIN=\"fcitx5-hangul\" -D_GNU_SOURCE)
fcitx5_add_i18n_definition()

add_subdirectory(po)
add_subdirectory(src)
add_subdirectory(data)
add_subdirectory(list)

fcitx5_translate_desktop_file(org.fcitx.Fcitx5.Addon.Hangul.metainfo.xml.in
                              org.fcitx.Fcitx5.Addon.Hangul.metainfo.xml XML)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/org.fcitx.Fcitx5.Addon.Hangul.metainfo.xml" DESTINATION ${CMAKE_INSTALL_DATADIR}/metainfo)
feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
