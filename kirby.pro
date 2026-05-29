# 基础模块
QT       += core gui widgets multimedia

# 包含你的所有代码文件
SOURCES += \
    basicenemy.cpp \
    bossenemy.cpp \
    brainofcthulhu.cpp \
    cake.cpp \
    checkpoint.cpp \
    dukefishron.cpp \
    enemy.cpp \
    icegod.cpp \
    main.cpp \
    mainwindow.cpp \
    gameobject.cpp \
    player.cpp \
    projectile.cpp \
    tile.cpp \
    xuehua.cpp

HEADERS += \
    basicenemy.h \
    bossenemy.h \
    brainofcthulhu.h \
    cake.h \
    checkpoint.h \
    dukefishron.h \
    enemy.h \
    icegod.h \
    mainwindow.h \
    gameobject.h \
    player.h \
    projectile.h \
    tile.h \
    xuehua.h

# 包含你的界面和资源
FORMS += mainwindow.ui
RESOURCES += res.qrc