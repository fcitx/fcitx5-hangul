cmake_minimum_required (VERSION 3.6)

project(fcitx5-hangul)

find_package(ECM REQUIRED 1.0.0)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})
include(FeatureSummary)
include(GNUInstallDirs)
include(ECMUninstallTarget)

find_package(Fcitx5Core REQUIRED)
find_package(Gettext REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(Hangul IMPORTED_TARGET "libhangul>=0.0.12" REQUIRED)

include("${FCITX_INSTALL_CMAKECONFIG_DIR}/Fcitx5Utils/Fcitx5CompilerSettings.cmake")
add_definitions(-DFCITX_GETTEXT_DOMAIN=\"fcitx5-hangul\" -D_GNU_SOURCE)

add_subdirectory(po)
add_subdirectory(src)
add_subdirectory(data)
add_subdirectory(list)

fcitx5_translate_desktop_file(org.fcitx.fcitx5-hangul.metainfo.xml.in
                              org.fcitx.fcitx5-hangul.metainfo.xml XML)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/org.fcitx.fcitx5-hangul.metainfo.xml" DESTINATION ${CMAKE_INSTALL_DATADIR}/metainfo)
feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
