/*----------------------------------------------------------------------------*/
/* Copyright (c) 2017-2018 FIRST. All Rights Reserved.                        */
/* Open Source Software - may be modified and shared by FRC teams. The code   */
/* must be accompanied by the FIRST BSD license file in the root directory of */
/* the project.                                                               */
/*----------------------------------------------------------------------------*/

#include "HAL/DriverStation.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#include <wpi/condition_variable.h>
#include <wpi/mutex.h>

#include "HALInitializer.h"
#include "DriveStation/include/DriverStationInternal.h"
#include "DriveStation/include/MauDriveData.h"

static wpi::mutex msgMutex;
static wpi::condition_variable* newDSDataAvailableCond;
static wpi::mutex newDSDataAvailableMutex;
static int newDSDataAvailableCounter{0};

namespace hal {
    namespace init {
        void InitializeDriverStation() {
            static wpi::condition_variable nddaC;
            newDSDataAvailableCond = &nddaC;
        }
    }
}

using namespace hal;

extern "C" {
    int32_t HAL_SendError(HAL_Bool isError, int32_t errorCode, HAL_Bool isLVCode,
                          const char* details, const char* location, const char* callStack, HAL_Bool printMsg) {
        // Avoid flooding console by keeping track of previous 5 error
        // messages and only printing again if they're longer than 1 second old.
        static constexpr int KEEP_MSGS = 5;
        std::lock_guard<wpi::mutex> lock(msgMutex);
        static std::string prevMsg[KEEP_MSGS];
        static std::chrono::time_point<std::chrono::steady_clock>
                prevMsgTime[KEEP_MSGS];
        static bool initialized = false;
        if (!initialized) {
            for (int i = 0; i < KEEP_MSGS; i++) {
                prevMsgTime[i] =
                        std::chrono::steady_clock::now() - std::chrono::seconds(2);
            }
            initialized = true;
        }

        auto curTime = std::chrono::steady_clock::now();
        int i;
        for (i = 0; i < KEEP_MSGS; ++i) {
            if (prevMsg[i] == details) break;
        }
        int retval = 0;
        if (i == KEEP_MSGS || (curTime - prevMsgTime[i]) >= std::chrono::seconds(1)) {
            printMsg = true;
            if (printMsg) {
                if (location && location[0] != '\0') {
                    std::fprintf(stderr, "%s at %s: ", isError ? "Error" : "Warning",
                                 location);
                }
                std::fprintf(stderr, "%s\n", details);
                if (callStack && callStack[0] != '\0') {
                    std::fprintf(stderr, "%s\n", callStack);
                }
            }
            if (i == KEEP_MSGS) {
                // replace the oldest one
                i = 0;
                auto first = prevMsgTime[0];
                for (int j = 1; j < KEEP_MSGS; ++j) {
                    if (prevMsgTime[j] < first) {
                        first = prevMsgTime[j];
                        i = j;
                    }
                }
                prevMsg[i] = details;
            }
            prevMsgTime[i] = curTime;
        }
        return retval;
    }

    int32_t HAL_GetControlWord(HAL_ControlWord* controlWord) {
        // controlWord->enabled = SimDriverStationData->GetEnabled();
        // controlWord->autonomous = SimDriverStationData->GetAutonomous();
        // controlWord->test = SimDriverStationData->GetTest();
        // controlWord->eStop = SimDriverStationData->GetEStop();
        // controlWord->fmsAttached = SimDriverStationData->GetFmsAttached();
        // controlWord->dsAttached = SimDriverStationData->GetDsAttached();
        return 0;
    }

    HAL_AllianceStationID HAL_GetAllianceStation(int32_t* status) {
        *status = 0;
        return *mau::sharedMemory->readAllianceID();
    }

    int32_t HAL_GetJoystickAxes(int32_t joystickNum, HAL_JoystickAxes* axes) {
        axes = mau::sharedMemory->readJoyAxes(joystickNum);
        return axes->count;
    }

    int32_t HAL_GetJoystickPOVs(int32_t joystickNum, HAL_JoystickPOVs* povs) {
        // SimDriverStationData->GetJoystickPOVs(joystickNum, povs);
        povs = mau::sharedMemory->readJoyPOVs(joystickNum);
        return povs->count;
    }

    int32_t HAL_GetJoystickButtons(int32_t joystickNum, HAL_JoystickButtons* buttons) {
        // SimDriverStationData->GetJoystickButtons(joystickNum, buttons);
        buttons = mau::sharedMemory->readJoyButtons(joystickNum);
        return buttons->count;
    }

    /**
     * Retrieve the Joystick Descriptor for particular slot
     * @param desc [out] descriptor (data transfer object) to fill in.  desc is
     * filled in regardless of success. In other words, if descriptor is not
     * available, desc is filled in with default values matching the init-values in
     * Java and C++ Driverstation for when caller requests a too-large joystick
     * index.
     *
     * @return error code reported from Network Comm back-end.  Zero is good,
     * nonzero is bad.
     */
    int32_t HAL_GetJoystickDescriptor(int32_t joystickNum, HAL_JoystickDescriptor* desc) {
        desc = mau::sharedMemory->readJoyDescriptor(joystickNum);
        return 0;
    }

    HAL_Bool HAL_GetJoystickIsXbox(int32_t joystickNum) {
        return mau::sharedMemory->readJoyDescriptor(joystickNum)->isXbox;
    }

    int32_t HAL_GetJoystickType(int32_t joystickNum) {
        return mau::sharedMemory->readJoyDescriptor(joystickNum)->type;
    }

    char* HAL_GetJoystickName(int32_t joystickNum) {
        return mau::sharedMemory->readJoyDescriptor(joystickNum)->name;
    }

    void HAL_FreeJoystickName(char* name) {
        std::free(name);
    }

    int32_t HAL_GetJoystickAxisType(int32_t joystickNum, int32_t axis) {
        return mau::sharedMemory->readJoyDescriptor(joystickNum)->axisTypes[axis];
    }

    int32_t HAL_SetJoystickOutputs(int32_t joystickNum, int64_t outputs, int32_t leftRumble, int32_t rightRumble) {
        // SimDriverStationData->SetJoystickOutputs(joystickNum, outputs, leftRumble, rightRumble);
        return 0;
    }

    double HAL_GetMatchTime(int32_t* status) {
        return 0;
    }

    int HAL_GetMatchInfo(HAL_MatchInfo* info) {
        info = mau::sharedMemory->readMatchInfo();
        return 0;
    }

    void HAL_FreeMatchInfo(HAL_MatchInfo* info) {
        std::free(info->eventName);
        std::free(info->gameSpecificMessage);
        info->eventName = nullptr;
        info->gameSpecificMessage = nullptr;
    }

// ----- Driver Station:  ----- //

void HAL_ReleaseDSMutex(void);
bool HAL_IsNewControlData(void);
void HAL_WaitForDSData(void);
HAL_Bool HAL_WaitForDSDataTimeout(double timeout);
void HAL_InitializeDriverStation(void);

// ----- HAL Data: Read ----- //

    void HAL_ObserveUserProgramStarting(void) {
        // HALSIM_SetProgramStarted();
    }

    void HAL_ObserveUserProgramDisabled(void) {
        // TODO
    }

    void HAL_ObserveUserProgramAutonomous(void) {
        // TODO
    }

    void HAL_ObserveUserProgramTeleop(void) {
        // TODO
    }

    void HAL_ObserveUserProgramTest(void) {
        // TODO
    }
}  // extern "C"
