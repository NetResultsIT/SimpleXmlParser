
CONFIG += debug debug_and_release
TARGET = xmlparsetest
DEPENDPATH += . paramparser_class ../simplexmlparser_class
INCLUDEPATH += . paramparser_class ../simplexmlparser_class

# Input
HEADERS += paramparser_class/nrparamparser.h \
           ../simplexmlparser_class/SimpleXmlParser.h
SOURCES += main.cpp \
           paramparser_class/nrparamparser.cpp \
           ../simplexmlparser_class/SimpleXmlParser.cpp

unix {
TEMPLATE = app
}

win32 {
TEMPLATE = vcapp
}
