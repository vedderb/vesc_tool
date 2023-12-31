#include "qminimp3.h"
#include <QFile>
#include <QDebug>

//#define MINIMP3_NO_SIMD
#define MINIMP3_FLOAT_OUTPUT
#define MINIMP3_IMPLEMENTATION
#include "minimp3_ex.h"

QMiniMp3::QMiniMp3(QObject *parent)
    : QObject{parent}
{

}

MiniMp3Dec QMiniMp3::decodeFileMono(QString path)
{
    MiniMp3Dec res;

    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        qWarning() << "Could not open MP3 file";
        return res;
    }

    if (!path.endsWith("mp3", Qt::CaseInsensitive)) {
        qWarning() << "Invalid file ending";
        return res;
    }

    if (f.size() > 100e6) {
        qWarning() << "Too large file";
        return res;
    }

    QByteArray mp3Data = f.readAll();
    f.close();

    mp3dec_t mp3d;
    mp3dec_file_info_t info;
    memset(&info, 0, sizeof(info));

    mp3dec_load_buf(&mp3d, (uint8_t*)mp3Data.data(), mp3Data.size(), &info, 0, 0);

    res.fSamp = info.hz;

    for (int i = 0;i < info.samples;i++) {
        if (i % info.channels == 0) {
            res.samples.append(info.buffer[i]);
        }
    }

    free(info.buffer);

    return res;
}
