#путь к корневому каталогу проекта
PROJECT_ROOT_PATH = $${PWD}/

#корневые папки
# directory of common files
COMMONDIR       =       common
# derectory to search additional include files
DEFSDIR         =       defs


#BUILD_FLAG = release
BUILD_FLAG = debug

# версия
VERSION = \\\"'0.9.9.0'\\\"
DEFINES += "VERSION_NUMBER=$${VERSION}"

# путь к библиотекам
#LIBS_PATH = $${PROJECT_ROOT_PATH}/lib

# путь к заголовочным файлам
INC_PATH = $${PROJECT_ROOT_PATH}/defs/

# пути сборки
BUILD_PATH = $${PROJECT_ROOT_PATH}/build/$${BUILD_FLAG}/$${TARGET}/
UI_DIR = $${BUILD_PATH}/ui/
MOC_DIR = $${BUILD_PATH}/moc/
OBJECTS_DIR = $${BUILD_PATH}/obj/

# путь к каталогу с бинарниками
BIN_PATH = $${PROJECT_ROOT_PATH}/bin/$${BUILD_FLAG}/

# путь, в который будет помещен готовый исполняемый файл
DESTDIR = $${BIN_PATH}/

#HEADERS += \
#    $$PWD/Common/stringconstants.h \
#    $$PWD/Common/devices/abstractdevice.h \
#    $$PWD/Common/devices/universaldevice.h

#SOURCES += \
#    $$PWD/Common/devices/abstractdevice.cpp \
#    $$PWD/Common/devices/universaldevice.cpp

