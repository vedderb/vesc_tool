/*
    Copyright 2026 Benjamin Vedder	benjamin@vedder.se

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

#include "pollmanager.h"

PollManager::PollManager(QObject *parent, VescInterface *vesc)
    : QObject{parent}, mVesc {vesc}
{
    connect(&mPollTimer, &QTimer::timeout, [this]() {
        if (!mPollList.isEmpty() && mVesc != nullptr) {
            auto id = mPollList.first();
            mPollList.removeAll(id);
            auto commands = mVesc->commands();

            switch (id) {
            case COMM_FW_VERSION:
                commands->getFwVersion();
                break;

            case COMM_GET_VALUES:
                commands->getValues();
                break;

            case COMM_GET_MCCONF:
                commands->getMcconf();
                break;

            case COMM_GET_MCCONF_DEFAULT:
                commands->getMcconfDefault();
                break;

            case COMM_GET_APPCONF:
                commands->getAppConf();
                break;

            case COMM_GET_APPCONF_DEFAULT:
                commands->getAppConfDefault();
                break;

            case COMM_GET_DECODED_PPM:
                commands->getDecodedPpm();
                break;

            case COMM_GET_DECODED_ADC:
                commands->getDecodedAdc();
                break;

            case COMM_GET_DECODED_CHUK:
                commands->getDecodedChuk();
                break;

            case COMM_GET_VALUES_SETUP:
                commands->getValuesSetup();
                break;

            case COMM_GET_IMU_DATA:
                commands->getImuData(0xFFFF);
                break;

            case COMM_BMS_GET_VALUES:
                commands->bmsGetValues();
                break;

            case COMM_GET_STATS:
                commands->getStats(0xFFFFFFFF);
                break;

            default:
                qWarning() << "Unsupported pollmanager ID" << id;
                break;
            }
        }
    });

    mPollTimer.start(10);
}

void PollManager::setVesc(VescInterface *vesc)
{
    mVesc = vesc;
}

void PollManager::pollField(COMM_PACKET_ID id)
{
    mPollList.append(id);
}
