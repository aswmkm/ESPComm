QT += widgets serialport charts

TARGET = client
TEMPLATE = app

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    settingsdialog.cpp \
    console.cpp \
    chart.cpp

HEADERS += \
    mainwindow.h \
    settingsdialog.h \
    console.h \
    chart.h

FORMS += \
    mainwindow.ui \
    settingsdialog.ui

RESOURCES += \
    client.qrc

INSTALLS += target
