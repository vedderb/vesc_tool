#ifndef HEATSHRINKIF_H
#define HEATSHRINKIF_H

#include "heatshrink_encoder.h"
#include "heatshrink_decoder.h"
#include <QString>

class HeatshrinkIf
{
public:
    HeatshrinkIf();
    ~HeatshrinkIf();

    QByteArray encode(QByteArray in);
    QByteArray decode(QByteArray in);
    void test(QString fileName);

private:
    heatshrink_encoder *hse;
    heatshrink_decoder *hsd;
};

#endif // HEATSHRINKIF_H
