/*
 *
 *    Copyright (c) 2026 Project CHIP Authors
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */
#pragma once

#include <lib/core/CHIPError.h>

namespace chip {
namespace app {

/**
 * Interface for simulated devices that support lifecycle control (pause and resume).
 * Device types representing pure Matter models do not implement this interface directly;
 * only simulated device implementations implement it.
 */
class SimulationControl
{
public:
    virtual ~SimulationControl() = default;

    virtual void PauseSimulation()          = 0;
    virtual CHIP_ERROR ResumeSimulation()   = 0;
    virtual bool IsSimulationPaused() const = 0;
};

} // namespace app
} // namespace chip
