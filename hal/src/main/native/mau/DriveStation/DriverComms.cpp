#include "socket.hpp"

#include "DriverComms.hpp"
#include "DriverThreads.h"
#include "include/MauDriveData.h"
#include "MauTime.h"
#include <wpi/priority_mutex.h>

#include <thread>
#include "DriverInternal.h"

static uint8_t sq_1 = 0;
static uint8_t sq_2 = 0;
static uint8_t control = 0;
static uint8_t req = 0;

static mau::comms::_TempJoyData joys[6];
static long long lastDecodeTime;
static double voltage;

static std::thread udpThread;
static std::thread tcpThread;

void mau::comms::start() {
    if (!isRunning) {
        Toast::Net::Socket::socket_init();

        runLock.lock();
        isRunning = true;
        printf("Server Running...\n");
        runLock.unlock();

        udpThread = std::thread(udpProcess);
        udpThread.detach();
        tcpThread = std::thread(tcpProcess);
        tcpThread.detach();
    }
}

void mau::comms::stop() {
    runLock.lock();
    isRunning = false;
    runLock.unlock();
}

//// ----- DriverStation Comms: Encode ----- ////

void mau::comms::encodePacket(char* data) {
    data[0] = sq_1;
    data[1] = sq_2;
    data[2] = 0x01;
    data[3] = control;
    data[4] = 0x10 | 0x20;

    double voltage = voltage;

    data[5] = (uint8_t) (voltage);
    data[6] = (uint8_t) ((voltage * 100 - ((uint8_t) voltage) * 100) * 2.5);
    data[7] = 0;
}

//// ----- DriverStation Comms: Decode ----- ////

bool last = false;
void mau::comms::periodicUpdate() {
    if (mau::vmxGetTime() - lastDecodeTime > 1000) {
        // DS Disconnected
        mau::sharedMemory->updateIsDsAttached(false);
    } else {
        // DS Connected
        mau::sharedMemory->updateIsDsAttached(true);
    }

    for (int joyNum = 0; joyNum < 6; joyNum++) {
        _TempJoyData* tempJoy = &joys[joyNum];
        mau::sharedMemory->updateJoyAxis(joyNum, tempJoy->axis_count, tempJoy->axis);
        mau::sharedMemory->updateJoyPOV(joyNum, tempJoy->pov_count, tempJoy->pov);
        mau::sharedMemory->updateJoyButtons(joyNum, tempJoy->button_count, tempJoy->button_mask);

        tempJoy->has_update = false;
    }

    //	voltage = shared()->power()->get_pdp_voltage();
    voltage = 12;
}

void mau::comms::decodeTcpPacket(char* data, int length) {
    if (data[2] == 0x02) {
        // Joystick Descriptor
        int i = 3;
        while (i < length) {
            uint8_t joyid = data[i++];
            bool xbox = data[i++] == 1;
            uint8_t type = data[i++];
            uint8_t name_length = data[i++];
            int nb_i = i;
            i += name_length;
            uint8_t axis_count = data[i++];
            uint8_t axis_types[16];
            int at_i = i;
            i += axis_count;
            uint8_t button_count = data[i++];
            uint8_t pov_count = data[i++];

            if (type != 255 && axis_count != 255) {
                HAL_JoystickDescriptor desc;
                desc.buttonCount = button_count;
                desc.povCount = pov_count;
                desc.isXbox = xbox;
                desc.type = type;
                if (name_length > 60) {
                    name_length = 60;
                }
                memcpy(desc.name, &data[nb_i], name_length);
                desc.axisCount = axis_count;
                for (int x = 0; x < axis_count; x++) {
                    desc.axisTypes[x] = data[at_i + x];
                }
                mau::sharedMemory->updateJoyDescriptor(joyid, &desc);
            }
        }
    }
}

void mau::comms::decodePacket(char* data, int length) {
    sq_1 = data[0];
    sq_2 = data[1];
    if (data[2] != 0) {
        control = data[3];
        bool test = IS_BIT_SET(control, 0);
        bool auton = IS_BIT_SET(control, 1);
        bool enabled = IS_BIT_SET(control, 2);
        bool fms = IS_BIT_SET(control, 3);
        bool eStop = IS_BIT_SET(control, 7);

        req = data[4];
        bool reboot = IS_BIT_SET(req, 3);
        bool restart = IS_BIT_SET(req, 2);

        if (reboot || restart) {
//            printf("NOTICE: Driver Station Requested Code Restart \n");
            stop();
            exit(0);
        } else if (eStop) {
//            printf("NOTICE: Driver Station Estop \n");
            stop();
            exit(0);
        }
        HAL_AllianceStationID alliance = (HAL_AllianceStationID)data[5];
        int i = 6;
        bool search = true;
        int joy_id = 0;

        while (i < length && search) {
            int struct_size = data[i];
            search = data[i + 1] == 0x0c;
            if (!search) continue;
            _TempJoyData* joy = &joys[joy_id];
            joy->axis_count = data[i + 2];
            for (int ax = 0; ax < joy->axis_count; ax++) {
                joy->axis[ax] = data[i + 2 + ax + 1];
            }
            int b = i + 2 + joy->axis_count + 1;
            joy->button_count = data[b];
            int button_delta = (joy->button_count / 8 + ((joy->button_count % 8 == 0) ? 0 : 1));
            uint32_t total_mask = 0;
            for (int bm = 0; bm < button_delta; bm++) {
                uint8_t m = data[b + bm + 1];
                total_mask = (total_mask << (bm * 8)) | m;
            }
            joy->button_mask = total_mask;
            b += button_delta + 1;
            joy->pov_count = data[b];
            for (int pv = 0; pv < joy->pov_count; pv++) {
                uint8_t a1 = data[b + 1 + (pv * 2)];
                uint8_t a2 = data[b + 1 + (pv * 2) + 1];
                if (a2 < 0) a2 = 256 + a2;
                joy->pov[pv] = (uint16_t) (a1 << 8 | a2);
            }
            joy->has_update = true;
            joy_id++;
            i += struct_size + 1;
        }
        mau::sharedMemory->updateAllianceID(alliance);
        mau::sharedMemory->updateIsEnabled(enabled);
        mau::sharedMemory->updateIsAutonomous(auton);
        mau::sharedMemory->updateIsTest(test);
        mau::sharedMemory->updateEStop(eStop);
        mau::sharedMemory->updateIsFmsAttached(fms);
        periodicUpdate();

        lastDecodeTime = mau::vmxGetTime();
    }
}