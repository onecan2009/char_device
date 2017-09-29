TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += main.c \
    char_dev_create.c \
    char_net_platform.c \
    dwmac1000_core.c \
    dwmac1000_dma.c \
    dwmac_lib.c \
    gmac_ctrl.c \
    gmac_init.c \
    net_ctrl.c \
    phy_ctrl.c

include(deployment.pri)
qtcAddDeployment()

HEADERS += \
    char_dev_create.h \
    char_net_platform.h \
    common.h \
    debug.h \
    descs.h \
    dwmac_dma.h \
    gmac_ctrl.h \
    gmac_init.h \
    net_ctrl.h \
    priv.h \
    phy_ctrl.h

INCLUDEPATH+= /home/rk3288-sdk/gpu-rk3288-kernel/include \
              /home/rk3288-sdk/gpu-rk3288-kernel/arch/arm/include

DISTFILES += \
    Makefile

