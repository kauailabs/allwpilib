/*----------------------------------------------------------------------------*/
/* Copyright (c) 2017-2018 FIRST. All Rights Reserved.                        */
/* Open Source Software - may be modified and shared by FRC teams. The code   */
/* must be accompanied by the FIRST BSD license file in the root directory of */
/* the project.                                                               */
/*----------------------------------------------------------------------------*/

#include <AHRS.h>
#include "HALInitializer.h"
#include "hal/Accelerometer.h"
#include "MauInternal.h"

namespace hal {
    namespace init {
        void InitializeAccelerometer() {}
    }
}

extern "C" {
    void HAL_SetAccelerometerActive(HAL_Bool active) {
        // Always active: no-op
    }

    void HAL_SetAccelerometerRange(HAL_AccelerometerRange range) {
        // VMX-pi's Accelerometer range is fixed at 2G
    }

    double HAL_GetAccelerometerX(void) {
        return mau::vmxIMU->GetRawAccelX();
    }

    double HAL_GetAccelerometerY(void) {
        return mau::vmxIMU->GetRawAccelY();
    }

    double HAL_GetAccelerometerZ(void) {
        return mau::vmxIMU->GetRawAccelZ();
    }
}
