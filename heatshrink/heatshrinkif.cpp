#include "heatshrinkif.h"

#include <QFile>
#include <QDebug>
#include <cstring>

HeatshrinkIf::HeatshrinkIf()
{
    hse = new heatshrink_encoder;
    hsd = new heatshrink_decoder;
}

HeatshrinkIf::~HeatshrinkIf()
{
    delete hse;
    delete hsd;
}

QByteArray HeatshrinkIf::encode(QByteArray in)
{
    heatshrink_encoder_reset(hse);

    size_t input_size = in.size();
    size_t comp_sz = input_size + (input_size/2) + 4;
    uint8_t *comp = new uint8_t[comp_sz];
    uint8_t *input = (uint8_t*)in.data();

    memset(comp, 0, comp_sz);

    size_t count = 0;
    uint32_t sunk = 0;
    uint32_t polled = 0;

    while (sunk < input_size) {
        heatshrink_encoder_sink(hse, &input[sunk], input_size - sunk, &count);
        sunk += count;
        if (sunk == input_size) {
            heatshrink_encoder_finish(hse);
        }

        HSE_poll_res pres;
        do {
            pres = heatshrink_encoder_poll(hse, &comp[polled], comp_sz - polled, &count);
            polled += count;
        } while (pres == HSER_POLL_MORE);

        if (sunk == input_size) {
            heatshrink_encoder_finish(hse);
        }
    }

    QByteArray res = QByteArray((char*)comp, polled);
    delete[] comp;

    return res;
}

QByteArray HeatshrinkIf::decode(QByteArray in)
{
    heatshrink_decoder_reset(hsd);

    uint32_t compressed_size = in.size();
    size_t count = 0;
    uint32_t sunk = 0;
    uint32_t polled = 0;
    uint8_t *input = (uint8_t*)in.data();
    QByteArray res;

    while (sunk < compressed_size) {
        heatshrink_decoder_sink(hsd, &input[sunk], compressed_size - sunk, &count);
        sunk += count;
        if (sunk == compressed_size) {
            heatshrink_decoder_finish(hsd);
        }

        HSD_poll_res pres;
        do {
            uint8_t chunk[16];
            pres = heatshrink_decoder_poll(hsd, chunk, 16, &count);
            res.append((char*)chunk, count);
            polled += count;
        } while (pres == HSDR_POLL_MORE);
        if (sunk == compressed_size) {
            heatshrink_decoder_finish(hsd);
        }
    }

    return res;
}

void HeatshrinkIf::test(QString fileName)
{
    qDebug() << sizeof(heatshrink_encoder) << sizeof(heatshrink_decoder);

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Could not open" << fileName;
        return;
    }
    auto dataIn = file.readAll();
    file.close();

    auto comp = encode(dataIn);

    qDebug() << "In:" << dataIn.size() << "Out:" << comp.size() << "Final size:"
             << (100.0 * double(comp.size()) / double(dataIn.size())) << "%";

    auto decomp = decode(comp);
    qDebug() << "DecompSz:" <<  decomp.size();
    qDebug() << "Compare:" << decomp.compare(dataIn);
}
