/*
    Copyright 2016 - 2017 Benjamin Vedder	benjamin@vedder.se

    This file is part of VESC Tool.

    VESC Tool is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    VESC Tool is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
    */

#include "digitalfiltering.h"
#include <cmath>
#include <QDebug>

DigitalFiltering::DigitalFiltering()
{
}

// Found at http://paulbourke.net/miscellaneous//dft/
// Dir: 0: Forward, != 0: Reverse
// m: 2^m points
// real: Real part
// imag: Imaginary part
void DigitalFiltering::fft(int dir, int m, double *real, double *imag)
{
    long n,i,i1,j,k,i2,l,l1,l2;
    double c1,c2,tx,ty,t1,t2,u1,u2,z;

    // Calculate the number of points
    n = 1 << m;

    // Do the bit reversal
    i2 = n >> 1;
    j = 0;
    for (i=0;i<n-1;i++) {
        if (i < j) {
            tx = real[i];
            ty = imag[i];
            real[i] = real[j];
            imag[i] = imag[j];
            real[j] = tx;
            imag[j] = ty;
        }
        k = i2;
        while (k <= j) {
            j -= k;
            k >>= 1;
        }
        j += k;
    }

    // Compute the FFT
    c1 = -1.0;
    c2 = 0.0;
    l2 = 1;
    for (l=0;l<m;l++) {
        l1 = l2;
        l2 <<= 1;
        u1 = 1.0;
        u2 = 0.0;
        for (j=0;j < l1;j++) {
            for (i=j;i < n;i += l2) {
                i1 = i + l1;
                t1 = u1 * real[i1] - u2 * imag[i1];
                t2 = u1 * imag[i1] + u2 * real[i1];
                real[i1] = real[i] - t1;
                imag[i1] = imag[i] - t2;
                real[i] += t1;
                imag[i] += t2;
            }
            z =  u1 * c1 - u2 * c2;
            u2 = u1 * c2 + u2 * c1;
            u1 = z;
        }
        c2 = sqrt((1.0 - c1) / 2.0);
        if (dir) {
            c2 = -c2;
        }
        c1 = sqrt((1.0 + c1) / 2.0);
    }

    // Scaling for reverse transform
    if (dir) {
        for (i=0;i < n;i++) {
            real[i] /= n;
            imag[i] /= n;
        }
    }
}

// Found at http://paulbourke.net/miscellaneous//dft/
void DigitalFiltering::dft(int dir, int len, double *real, double *imag) {
    long i,k;
    double arg;
    double cosarg, sinarg;

    if(dir) {
        dir = 1;
    } else {
        dir = -1;
    }

    double *x2 = new double[len];
    double *y2 = new double[len];

    for (i=0;i < len;i++) {
        x2[i] = 0;
        y2[i] = 0;
        arg = -(double)dir * 2.0 * M_PI * (double)i / (double)len;
        for (k=0;k<len;k++) {
            cosarg = cos(k * arg);
            sinarg = sin(k * arg);
            x2[i] += (real[k] * cosarg - imag[k] * sinarg);
            y2[i] += (real[k] * sinarg + imag[k] * cosarg);
        }
    }

    // Copy the data back
    if (dir == 1) {
        for (i=0;i<len;i++) {
            real[i] = x2[i] / (double)len;
            imag[i] = y2[i] / (double)len;
        }
    } else {
        for (i=0;i<len;i++) {
            real[i] = x2[i];
            imag[i] = y2[i];
        }
    }

    delete[] x2;
    delete[] y2;
}

void DigitalFiltering::fftshift(double *data, int len)
{
    for (int i = 0;i < (len / 2);i++) {
        double r1 = data[i];
        double r2 = data[len/2 + i];

        data[i] = r2;
        data[len / 2 + i] = r1;
    }
}

void DigitalFiltering::hamming(double *data, int len)
{
    if (len % 2 == 0) {
        for (int i = 0;i < (len / 2);i++) {
            double val = 0.54 - 0.46 * cos((2.0 * M_PI * (double)i)/(double)(len - 1));
            data[i] *= val;
            data[len - i - 1] *= val;
        }
    } else {
        for (int i = 0;i < len;i++) {
            data[i] *= 0.54 - 0.46 * cos((2.0 * M_PI * (double)i)/(double)(len - 1));
        }
    }
}

void DigitalFiltering::zeroPad(double *data, double *result, int dataLen, int resultLen)
{
    for (int i = 0;i < resultLen;i++) {
        if (i < dataLen) {
            result[i] = data[i];
        } else {
            result[i] = 0;
        }
    }
}

int DigitalFiltering::whichPowerOfTwo(unsigned int number)
{
    unsigned int powersOfTwo[32] =
    {1,2,4,8,16,32,64,128,256,512,1024,2048,4096,8192,16384,32768,
     65536,131072,262144,524288,1048576,2097152,4194304,8388608,
     16777216,33554432,67108864,134217728,268435456,536870912,
     1073741824UL,2147483648UL};

    int exponent = 0;
    while (powersOfTwo[exponent] < number && exponent < 31) {
        exponent++;
    }

    return exponent;
}

QVector<double> DigitalFiltering::filterSignal(const QVector<double> &signal, const QVector<double> &filter, bool padAfter)
{
    QVector<double> result;
    int taps = filter.size();

    for (int i = 0;i < taps / 2;i++) {
        result.append(0.0);
    }

    if (!padAfter) {
        for (int i = 0;i < taps / 2;i++) {
            result.append(0.0);
        }
    }

    for (int i = 0;i < signal.size() - taps;i++) {
        double coeff = 0;
        for (int j = 0;j < taps;j++) {
            coeff += signal[i + j] * filter[j];
        }
        result.append(coeff);
    }

    if (padAfter) {
        for (int i = 0;i < taps / 2;i++) {
            result.append(0.0);
        }
    }

    return result;
}

QVector<double> DigitalFiltering::generateFirFilter(double f_break, int bits, bool useHamming)
{
    int taps = 1 << bits;
    double imag[taps];
    double filter_vector[taps];

    for(int i = 0;i < taps;i++) {
        if (i < (int)((double)taps * f_break)) {
            filter_vector[i] = 1.0;
        } else {
            filter_vector[i] = 0.0;
        }
        imag[i] = 0;
    }

    for (int i = 0;i < taps / 2;i++) {
        filter_vector[taps - i - 1] = filter_vector[i];
    }

    fft(1, bits, filter_vector, imag);
    fftshift(filter_vector, taps);

    if (useHamming) {
        hamming(filter_vector, taps);
    }

    QVector<double> result;
    for(int i = 0;i < taps;i++) {
        result.append(filter_vector[i]);
    }

    return result;
}

QVector<double> DigitalFiltering::fftWithShift(QVector<double> &signal, int resultBits, bool scaleByLen)
{
    QVector<double> result;
    int taps = signal.size();
    int resultLen = 1 << resultBits;

    double *signal_vector = new double[resultLen];
    double *imag = new double[resultLen];

    if (resultLen < taps) {
        int sizeDiffHalf = (taps - resultLen) / 2;
        signal.remove(0, sizeDiffHalf);
        signal.resize(resultLen);

        for(int i = 0;i < resultLen;i++) {
            signal_vector[i] = signal[i];
            imag[i] = 0;
        }
    } else {
        int sizeDiffHalf = (resultLen - taps) / 2;
        for(int i = 0;i < resultLen;i++) {
            if (i < sizeDiffHalf) {
                signal_vector[i] = 0;
            } else if (i < (taps + sizeDiffHalf)) {
                signal_vector[i] = signal[i - sizeDiffHalf];
            } else {
                signal_vector[i] = 0;
            }
            imag[i] = 0;
        }
    }

    fftshift(signal_vector, resultLen);
    fft(0, resultBits, signal_vector, imag);

    double div_factor = scaleByLen ? (double)taps : 1.0;
    for(int i = 0;i < resultLen;i++) {
        result.append(fabs(signal_vector[i]) / div_factor);
    }

    delete[] signal_vector;
    delete[] imag;

    return result;
}
