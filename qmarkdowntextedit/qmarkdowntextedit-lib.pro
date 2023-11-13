TARGET = QMarkdownTextedit
TEMPLATE = lib
QT += core gui widgets
CONFIG += c++11 create_prl no_install_prl create_pc

include(qmarkdowntextedit.pri)

TRANSLATIONS += trans/qmarkdowntextedit_de.ts \
                trans/qmarkdowntextedit_zh_CN.ts \
                trans/qmarkdowntextedit_es.ts

isEmpty(PREFIX):PREFIX=$$[QT_INSTALL_PREFIX]
isEmpty(LIBDIR):LIBDIR=$$[QT_INSTALL_LIBS]
isEmpty(HEADERDIR):HEADERDIR=$${PREFIX}/include/$$TARGET/
isEmpty(DSRDIR):DSRDIR=$${PREFIX}/share/$$TARGET

target.path = $${LIBDIR}

headers.files = $$HEADERS
headers.path = $${HEADERDIR}

license.files = LICENSE
license.path = $${DSRDIR}/licenses/

trans.files = trans/*.qm
trans.path = $${DSRDIR}/translations/

QMAKE_PKGCONFIG_NAME = QMarkdownTextedit
QMAKE_PKGCONFIG_DESCRIPTION = C++ Qt QPlainTextEdit widget with markdown highlighting and some other goodies
QMAKE_PKGCONFIG_INCDIR = $${headers.path}
QMAKE_PKGCONFIG_LIBDIR = $${LIBDIR}
QMAKE_PKGCONFIG_DESTDIR = pkgconfig

INSTALLS += target license headers trans
