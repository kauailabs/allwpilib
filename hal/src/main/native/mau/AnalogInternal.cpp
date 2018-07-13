/*----------------------------------------------------------------------------*/
/* Copyright (c) 2016-2018 FIRST. All Rights Reserved.                        */
/* Open Source Software - may be modified and shared by FRC teams. The code   */
/* must be accompanied by the FIRST BSD license file in the root directory of */
/* the project.                                                               */
/*----------------------------------------------------------------------------*/

#include "AnalogInternal.h"

#include "HAL/AnalogInput.h"
#include "PortsInternal.h"

namespace hal {
    IndexedHandleResource<HAL_AnalogInputHandle, hal::AnalogPort, kNumAnalogInputs,
            HAL_HandleEnum::AnalogInput> *analogInputHandles;
}

namespace hal {
    namespace init {
        void InitializeAnalogInternal() {
            static IndexedHandleResource<HAL_AnalogInputHandle, hal::AnalogPort,
                    kNumAnalogInputs, HAL_HandleEnum::AnalogInput>
                    aiH;
            analogInputHandles = &aiH;
        }
    }
}
