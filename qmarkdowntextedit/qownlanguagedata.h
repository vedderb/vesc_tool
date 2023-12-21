/*
 * MIT License
 *
 * Copyright (c) 2019-2021 Waqar Ahmed -- <waqar.17a@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#ifndef QOWNLANGUAGEDATA_H
#define QOWNLANGUAGEDATA_H

#include <QMultiHash>

/* ------------------------
 * TEMPLATE FOR LANG DATA
 * -------------------------
 *
 * loadXXXData, where XXX is the language
 * keywords are the language keywords e.g, const
 * types are built-in types i.e, int, char, var
 * literals are words like, true false
 * builtin are the library functions
 * other can contain any other thing, for e.g, in cpp it contains the
 preprocessor

    static const QMultiHash<char, QLatin1String> xxx_keywords = {
    };

    static const QMultiHash<char, QLatin1String> xxx_types = {
    };

    static const QMultiHash<char, QLatin1String> xxx_literals = {
    };

    static const QMultiHash<char, QLatin1String> xxx_builtin = {
    };

    static const QMultiHash<char, QLatin1String> xxx_other = {
    };

*/

/**********************************************************/
/* C/C++ Data *********************************************/
/**********************************************************/
void loadCppData(QMultiHash<char, QLatin1String> &typess,
                 QMultiHash<char, QLatin1String> &keywordss,
                 QMultiHash<char, QLatin1String> &builtins,
                 QMultiHash<char, QLatin1String> &literalss,
                 QMultiHash<char, QLatin1String> &others);

/**********************************************************/
/* Shell Data *********************************************/
/**********************************************************/
void loadShellData(QMultiHash<char, QLatin1String> &types,
                   QMultiHash<char, QLatin1String> &keywords,
                   QMultiHash<char, QLatin1String> &builtin,
                   QMultiHash<char, QLatin1String> &literals,
                   QMultiHash<char, QLatin1String> &other);

/**********************************************************/
/* JS Data *********************************************/
/**********************************************************/
void loadJSData(QMultiHash<char, QLatin1String> &types,
                QMultiHash<char, QLatin1String> &keywords,
                QMultiHash<char, QLatin1String> &builtin,
                QMultiHash<char, QLatin1String> &literals,
                QMultiHash<char, QLatin1String> &other);

/**********************************************************/
/* JS Data *********************************************/
/**********************************************************/
void loadNixData(QMultiHash<char, QLatin1String> &types,
                QMultiHash<char, QLatin1String> &keywords,
                QMultiHash<char, QLatin1String> &builtin,
                QMultiHash<char, QLatin1String> &literals,
                QMultiHash<char, QLatin1String> &other);

/**********************************************************/
/* PHP Data *********************************************/
/**********************************************************/
void loadPHPData(QMultiHash<char, QLatin1String> &types,
                 QMultiHash<char, QLatin1String> &keywords,
                 QMultiHash<char, QLatin1String> &builtin,
                 QMultiHash<char, QLatin1String> &literals,
                 QMultiHash<char, QLatin1String> &other);

/**********************************************************/
/* QML Data *********************************************/
/**********************************************************/
void loadQMLData(QMultiHash<char, QLatin1String> &types,
                 QMultiHash<char, QLatin1String> &keywords,
                 QMultiHash<char, QLatin1String> &builtin,
                 QMultiHash<char, QLatin1String> &literals,
                 QMultiHash<char, QLatin1String> &other);

/**********************************************************/
/* Python Data *********************************************/
/**********************************************************/
void loadPythonData(QMultiHash<char, QLatin1String> &types,
                    QMultiHash<char, QLatin1String> &keywords,
                    QMultiHash<char, QLatin1String> &builtin,
                    QMultiHash<char, QLatin1String> &literals,
                    QMultiHash<char, QLatin1String> &other);

/********************************************************/
/***   Rust DATA      ***********************************/
/********************************************************/
void loadRustData(QMultiHash<char, QLatin1String> &types,
                  QMultiHash<char, QLatin1String> &keywords,
                  QMultiHash<char, QLatin1String> &builtin,
                  QMultiHash<char, QLatin1String> &literals,
                  QMultiHash<char, QLatin1String> &other);

/********************************************************/
/***   Java DATA      ***********************************/
/********************************************************/
void loadJavaData(QMultiHash<char, QLatin1String> &types,
                  QMultiHash<char, QLatin1String> &keywords,
                  QMultiHash<char, QLatin1String> &builtin,
                  QMultiHash<char, QLatin1String> &literals,
                  QMultiHash<char, QLatin1String> &other);

/********************************************************/
/***   C# DATA      *************************************/
/********************************************************/
void loadCSharpData(QMultiHash<char, QLatin1String> &types,
                    QMultiHash<char, QLatin1String> &keywords,
                    QMultiHash<char, QLatin1String> &builtin,
                    QMultiHash<char, QLatin1String> &literals,
                    QMultiHash<char, QLatin1String> &other);

/********************************************************/
/***   Go DATA      *************************************/
/********************************************************/
void loadGoData(QMultiHash<char, QLatin1String> &types,
                QMultiHash<char, QLatin1String> &keywords,
                QMultiHash<char, QLatin1String> &builtin,
                QMultiHash<char, QLatin1String> &literals,
                QMultiHash<char, QLatin1String> &other);

/********************************************************/
/***   V DATA      **************************************/
/********************************************************/
void loadVData(QMultiHash<char, QLatin1String> &types,
               QMultiHash<char, QLatin1String> &keywords,
               QMultiHash<char, QLatin1String> &builtin,
               QMultiHash<char, QLatin1String> &literals,
               QMultiHash<char, QLatin1String> &other);

/********************************************************/
/***   SQL DATA      ************************************/
/********************************************************/
void loadSQLData(QMultiHash<char, QLatin1String> &types,
                 QMultiHash<char, QLatin1String> &keywords,
                 QMultiHash<char, QLatin1String> &builtin,
                 QMultiHash<char, QLatin1String> &literals,
                 QMultiHash<char, QLatin1String> &other);

/********************************************************/
/***   JSON DATA      ***********************************/
/********************************************************/
void loadJSONData(QMultiHash<char, QLatin1String> &types,
                  QMultiHash<char, QLatin1String> &keywords,
                  QMultiHash<char, QLatin1String> &builtin,
                  QMultiHash<char, QLatin1String> &literals,
                  QMultiHash<char, QLatin1String> &other);

/********************************************************/
/***   CSS DATA      ***********************************/
/********************************************************/
void loadCSSData(QMultiHash<char, QLatin1String> &types,
                 QMultiHash<char, QLatin1String> &keywords,
                 QMultiHash<char, QLatin1String> &builtin,
                 QMultiHash<char, QLatin1String> &literals,
                 QMultiHash<char, QLatin1String> &other);

/********************************************************/
/***   Typescript DATA  *********************************/
/********************************************************/
void loadTypescriptData(QMultiHash<char, QLatin1String> &types,
                        QMultiHash<char, QLatin1String> &keywords,
                        QMultiHash<char, QLatin1String> &builtin,
                        QMultiHash<char, QLatin1String> &literals,
                        QMultiHash<char, QLatin1String> &other);

/********************************************************/
/***   YAML DATA  ***************************************/
/********************************************************/
void loadYAMLData(QMultiHash<char, QLatin1String> &types,
                  QMultiHash<char, QLatin1String> &keywords,
                  QMultiHash<char, QLatin1String> &builtin,
                  QMultiHash<char, QLatin1String> &literals,
                  QMultiHash<char, QLatin1String> &other);

/********************************************************/
/***   VEX DATA  ****************************************/
/********************************************************/
void loadVEXData(QMultiHash<char, QLatin1String> &types,
                 QMultiHash<char, QLatin1String> &keywords,
                 QMultiHash<char, QLatin1String> &builtin,
                 QMultiHash<char, QLatin1String> &literals,
                 QMultiHash<char, QLatin1String> &other);

/********************************************************/
/***   CMake DATA  **************************************/
/********************************************************/
void loadCMakeData(QMultiHash<char, QLatin1String> &types,
                   QMultiHash<char, QLatin1String> &keywords,
                   QMultiHash<char, QLatin1String> &builtin,
                   QMultiHash<char, QLatin1String> &literals,
                   QMultiHash<char, QLatin1String> &other);

/********************************************************/
/***   Make DATA  ***************************************/
/********************************************************/
void loadMakeData(QMultiHash<char, QLatin1String> &types,
                  QMultiHash<char, QLatin1String> &keywords,
                  QMultiHash<char, QLatin1String> &builtin,
                  QMultiHash<char, QLatin1String> &literals,
                  QMultiHash<char, QLatin1String> &other);
/********************************************************/
/***   Forth DATA  **************************************/
/********************************************************/
void loadForthData(QMultiHash<char, QLatin1String> &types,
                   QMultiHash<char, QLatin1String> &keywords,
                   QMultiHash<char, QLatin1String> &builtin,
                   QMultiHash<char, QLatin1String> &literals,
                   QMultiHash<char, QLatin1String> &other);
#endif
