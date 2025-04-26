ARG BASE_IMAGE="ubuntu"
# ARG TAG="18.04"
ARG TAG="22.04"
FROM ${BASE_IMAGE}:${TAG}
WORKDIR /vesc_tool

ARG DEBIAN_FRONTEND=noninteractive
ARG USER_NAME=vesc_tool
ARG USER_UID=1000
ARG USER_GID=1000

RUN groupadd ${USER_NAME} --gid ${USER_GID}\
    && useradd -l -m ${USER_NAME} -u ${USER_UID} -g ${USER_GID} -s /bin/bash

RUN apt-get update && apt-get install --no-install-recommends -y \
    sudo \
    bash-completion \
    python3 python-is-python3 perl git openssl libssl-dev\
    wget curl iputils-ping build-essential \
    make ca-certificates xz-utils bzip2 zip \
    # libqt5printsupport5 libqt5quickwidgets5 libqt5gamepad5 libqt5serialport5 \
    # libqt5bluetooth5 libqt5positioning5 libqt5bluetooth5 libqt5widgets5 qml-module-qtquick-controls \
    # qml-module-qtquick-controls2 qml-module-qtquick-extras qml-module-qt-labs-settings \
    # qtbase5-dev qtchooser qt5-qmake qtbase5-dev-tools \
    # libdrm-dev libgles2-mesa-dev \
    # libxcb-xinerama0-dev "^libxcb.*" \
    # libx11-xcb-dev libglu1-mesa-dev libxrender-dev libxi-dev \
    # flex bison gperf libicu-dev libxslt-dev ruby libssl-dev \
    # libxcursor-dev libxcomposite-dev libxdamage-dev libxrandr-dev \
    # libfontconfig1-dev libcap-dev libxtst-dev libpulse-dev libudev-dev \
    # libpci-dev libnss3-dev libasound2-dev libxss-dev libegl1-mesa-dev \
    # gperf bison libbz2-dev libgcrypt20-dev libdrm-dev libcups2-dev \
    # libatkmm-1.6-dev libasound2-dev libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev \
    # libbluetooth-dev bluetooth blueman bluez libusb-dev libdbus-1-dev bluez-hcidump \
    # bluez-tools
    qtbase5-private-dev qtscript5-dev \
    qml-module-qt-labs-folderlistmodel qml-module-qtquick-extras \
    qml-module-qtquick-controls2 qtbase5-dev qtchooser qt5-qmake qtbase5-dev-tools libqt5quickcontrols2-5 qtquickcontrols2-5-dev \
    qtcreator qtcreator-doc libqt5serialport5-dev qml-module-qt3d qt3d5-dev \
    qtdeclarative5-dev qtconnectivity5-dev qtmultimedia5-dev qtpositioning5-dev \
    libqt5gamepad5-dev qml-module-qt-labs-settings qml-module-qt-labs-platform libqt5svg5-dev

ENV USER=${USER_NAME}

RUN echo "vesc_tool ALL=(ALL) NOPASSWD:ALL" > /etc/sudoers.d/${USER_NAME}
RUN chmod 0440 /etc/sudoers.d/${USER_NAME}

RUN chown -R ${USER_NAME}:${USER_NAME} /${USER_NAME}

USER ${USER_NAME}

# RUN  cd /opt \
#     && sudo mkdir qt5 \
#     && sudo chown $USER qt5 \
#     && git clone https://code.qt.io/qt/qt5.git \
#     && cd qt5 \
#     && git checkout 5.15 \
#     && perl init-repository --module-subset=default,-qtwebkit,-qtwebkit-examples,-qtwebengine 

# RUN sudo mkdir /opt/qt5/build \
#     && cd /opt/qt5/build \
#     && sudo chown $USER /opt/qt5/build \
#     && ../configure -prefix /opt/Qt/5.15-static/ -release -opensource -confirm-license \
#     -static -no-sql-mysql -no-sql-psql -no-sql-sqlite -no-journald -qt-zlib -no-mtdev \
#     -no-gif -qt-libpng -qt-libjpeg -qt-harfbuzz -qt-pcre -no-glib -no-compile-examples \
#     -no-cups -no-iconv -no-tslib -dbus-linked \
#     -no-xcb-xlib -no-eglfs -no-directfb -no-linuxfb -no-kms \
#     -nomake examples -nomake tests -skip qtwebsockets -skip qtwebchannel \
#     -skip qtwebengine -skip qtwayland -skip qtwinextras -skip qtsensors -skip multimedia \
#     -no-libproxy -no-icu -qt-freetype -skip qtimageformats -opengl es2 \
#     && make -j$(nproc) \
#     && sudo make install

CMD ["bash"]