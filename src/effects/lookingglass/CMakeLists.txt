#######################################
# Effect

set(lookingglass_SOURCES
    lookingglass.cpp
    lookingglass.qrc
    main.cpp
)

kconfig_add_kcfg_files(lookingglass_SOURCES
    lookingglassconfig.kcfgc
)

kwin4_add_effect_module(kwin4_effect_lookingglass ${lookingglass_SOURCES})
target_link_libraries(kwin4_effect_lookingglass PRIVATE
    kwineffects
    kwinglutils

    KF5::ConfigGui
    KF5::ConfigWidgets
    KF5::GlobalAccel
    KF5::I18n
)

#######################################
# Config
if (KWIN_BUILD_KCMS)
    set(kwin_lookingglass_config_SRCS lookingglass_config.cpp)
    ki18n_wrap_ui(kwin_lookingglass_config_SRCS lookingglass_config.ui)
    kconfig_add_kcfg_files(kwin_lookingglass_config_SRCS lookingglassconfig.kcfgc)

    kwin_add_effect_config(kwin_lookingglass_config ${kwin_lookingglass_config_SRCS})

    target_link_libraries(kwin_lookingglass_config
        KF5::ConfigWidgets
        KF5::CoreAddons
        KF5::GlobalAccel
        KF5::I18n
        KF5::XmlGui
        KWinEffectsInterface
    )
endif()
