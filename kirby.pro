# 基础模块
QT       += core gui widgets multimedia

# 包含你的所有代码文件
SOURCES += \
    apple.cpp \
    goal.cpp \
    basicenemy.cpp \
    bossenemy.cpp \
    brainofcthulhu.cpp \
    cake.cpp \
    checkpoint.cpp \
    crate.cpp \
    dukefishron.cpp \
    enemy.cpp \
    icegod.cpp \
    main.cpp \
    mainwindow.cpp \
    gameobject.cpp \
    player.cpp \
    projectile.cpp \
    star.cpp \
    tile.cpp \
    xuehua.cpp

HEADERS += \
    apple.h \
    goal.h \
    basicenemy.h \
    bossenemy.h \
    brainofcthulhu.h \
    cake.h \
    checkpoint.h \
    crate.h \
    dukefishron.h \
    enemy.h \
    icegod.h \
    mainwindow.h \
    gameobject.h \
    player.h \
    projectile.h \
    star.h \
    tile.h \
    xuehua.h

# 包含你的界面和资源
FORMS += mainwindow.ui
RESOURCES += res.qrc