# linuxdeployqt6.py

This Python3 script is like [macdeployqt](https://doc.qt.io/qt-5/macos-deployment.html) and [windeployqt](https://doc.qt.io/qt-5/windows-deployment.html), but for Linux.

It scans the dependencies of a Qt6 application (shared libraries from Qt SDK, Qt plugins, QML modules, Qt resources and translations), and deploys all of them into the specified directory(ies). Then it updates RPATH of each deployed executable and shared library.

Unlike other implementations, this one combines support for Qt6, Qt WebEngine, and QML.

## Usage

```
python3 ./linuxdeployqt6.py -help
usage: linuxdeployqt6.py [-h] [-f] [-n] [-v level] -qtdir path [-qmlscandir path]
                         [-out-dir path] [-out-exe-dir path] [-out-lib-dir path]
                         [-out-plugins-dir path] [-out-qml-dir path] [-out-data-dir path]
                         [-out-translations-dir path] [-no-conf] [-no-exe] [-no-lib]
                         [-no-plugins] [-no-qml] [-no-data] [-no-translations]
                         executable [executable ...]

Unofficial tool to make Linux Qt6 applications self-contained.

operation mode:
  -h, -help             Print help message and exit
  -f, -force            Force overwriting existing files
  -n, -dry-run          Print what is going to be deployed, but don't deploy anything
  -v level, -verbose level
                        Verbosity level

qt options:
  -qtdir path           Qt installation directory (e.g. /opt/Qt/6.2.4/gcc_64)

input options:
  executable            Input executable
  -qmlscandir path      Input directory to scan for qml imports

output options:
  -out-dir path         Output directory (by default directory of first executable)
  -out-exe-dir path     Output directory for executable (by default same as -out-dir)
  -out-lib-dir path     Output directory for libraries (by default same as -out-dir)
  -out-plugins-dir path
                        Output directory for plugins (by default same as -out-dir)
  -out-qml-dir path     Output directory for qml modules (by default same as -out-dir)
  -out-data-dir path    Output directory for data files (by default same as -out-dir)
  -out-translations-dir path
                        Output directory for translations (by default 'translations'
                        inside -out-dir)

deployment options:
  -no-conf              Skip qt.conf deployment
  -no-exe               Skip executable deployment
  -no-lib               Skip libraries deployment
  -no-plugins           Skip plugins deployment
  -no-qml               Skip qml modules deployment
  -no-data              Skip data files deployment
  -no-translations      Skip translations deployment

```

## Example

This invocation will scan dependencies of `<app_executable>` and `<app_qml_dir>` and install them into `<deployment_dir>`, overwriting all existing files.

```
python3 ./linuxdeployqt6.py -force -qtdir /opt/Qt/6.2.4/gcc_64 -out-dir <deployment_dir> \
  -qmlscandir <app_qml_dir> <app_executable>
```

## Alternatives

* [probonopd/linuxdeployqt](https://github.com/probonopd/linuxdeployqt)
* [ddurham2/linuxdeployqt](https://github.com/ddurham2/linuxdeployqt)
* [Larpon/linuxdeployqt.py](https://github.com/Larpon/linuxdeployqt.py)

## Authors

See [here](https://github.com/gavv/linuxdeployqt6.py/graphs/contributors).

## License

[MIT](LICENSE)
