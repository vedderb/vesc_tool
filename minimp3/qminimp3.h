#ifndef QMINIMP3_H
#define QMINIMP3_H

#include <QObject>
#include <QVector>

struct MiniMp3Dec {
    Q_GADGET

public:
    Q_PROPERTY(QVector<qreal> samples MEMBER samples)
    Q_PROPERTY(int fSamp MEMBER fSamp)

    MiniMp3Dec () {
        fSamp = -1.0;
    }

    QVector<qreal> samples;
    int fSamp;
};

Q_DECLARE_METATYPE(MiniMp3Dec)

class QMiniMp3 : public QObject
{
    Q_OBJECT
public:
    explicit QMiniMp3(QObject *parent = nullptr);
    Q_INVOKABLE MiniMp3Dec decodeFileMono(QString path);

signals:

};

#endif // QMINIMP3_H
