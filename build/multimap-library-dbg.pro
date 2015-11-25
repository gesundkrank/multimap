TEMPLATE = lib
TARGET = multimap-dbg
VERSION = 0.3

COMMON = multimap.pri
!include(../$$COMMON) {
    error("Could not find $$COMMON file")
}

CONFIG += debug

unix {
    multimap.path = /usr/include/multimap
    multimap.files += ../src/cpp/multimap/*.h*
    INSTALLS += multimap

    multimap_internal.path = /usr/include/multimap/internal
    multimap_internal.files += ../src/cpp/multimap/internal/*.h*
    INSTALLS += multimap_internal

    multimap_thirdparty_mt.path = /usr/include/multimap/thirdparty/mt
    multimap_thirdparty_mt.files += ../src/cpp/multimap/thirdparty/mt/*.h*
    INSTALLS += multimap_thirdparty_mt

    multimap_thirdparty_xxhash.path = /usr/include/multimap/thirdparty/xxhash
    multimap_thirdparty_xxhash.files += ../src/cpp/multimap/thirdparty/xxhash/*.h*
    INSTALLS += multimap_thirdparty_xxhash

    target.path = /usr/lib
    INSTALLS += target
}