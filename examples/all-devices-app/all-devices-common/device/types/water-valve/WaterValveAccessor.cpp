/*
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

#include "WaterValveAccessor.h"
#include <access/SubjectDescriptor.h>
#include <app-common/zap-generated/cluster-enums.h>
#include <app-common/zap-generated/ids/Attributes.h>
#include <app-common/zap-generated/ids/Clusters.h>
#include <lib/support/CodeUtils.h>
#include <lib/support/logging/CHIPLogging.h>
#include <oob-accessors/OOBDataSerializer.h>

using namespace chip::app::Clusters;

namespace chip::app {

std::optional<CHIP_ERROR> WaterValveAccessor::HandleAction(CharSpan actionName, ByteSpan tlvBuffer)
{
    if (!actionName.data_equal("SetAttribute"_span))
    {
        return std::nullopt;
    }

    auto parseResult = OOBDataSerializer::ParseAttributeRequest(tlvBuffer);
    if (std::holds_alternative<CHIP_ERROR>(parseResult))
    {
        CHIP_ERROR err = std::get<CHIP_ERROR>(parseResult);
        ChipLogError(Support, "Failed to parse attribute request: %" CHIP_ERROR_FORMAT, err.Format());
        return err;
    }

    auto & request = std::get<OOBDataSerializer::AttributeRequest>(parseResult);
    if (request.path.mEndpointId != mDevice.GetEndpointId())
    {
        return std::nullopt;
    }

    Access::SubjectDescriptor subjectDescriptor = {};
    subjectDescriptor.authMode                  = chip::Access::AuthMode::kInternalDeviceAccess;
    AttributeValueDecoder decoder(request.value, subjectDescriptor);

    return SetAttribute(request.path, decoder);
}

std::optional<CHIP_ERROR> WaterValveAccessor::SetAttribute(const ConcreteDataAttributePath & path,
                                                           AttributeValueDecoder & decoder)
{
    switch (path.mClusterId)
    {
    case ValveConfigurationAndControl::Id: {
        switch (path.mAttributeId)
        {
        case ValveConfigurationAndControl::Attributes::CurrentState::Id: {
            uint8_t rawValue = 0;
            CHIP_ERROR err   = decoder.Decode(rawValue);
            if (err != CHIP_NO_ERROR)
            {
                bool boolVal = false;
                ReturnErrorOnFailure(decoder.Decode(boolVal));
                rawValue = boolVal ? 1 : 0;
            }

            auto state = static_cast<ValveConfigurationAndControl::ValveStateEnum>(rawValue);
            if (state == ValveConfigurationAndControl::ValveStateEnum::kClosed)
            {
                ReturnErrorOnFailure(mDevice.ValveConfigurationAndControlCluster().CloseValve());
            }
            else
            {
                // Reopen to the level the valve was last at. Writing CurrentState is only meant to
                // change the state, so it must not silently discard a previously configured level.
                ReturnErrorOnFailure(mDevice.ValveConfigurationAndControlCluster().OpenValve(
                    DataModel::MakeNullable<Percent>(mDevice.LastOpenLevel()), DataModel::NullNullable));
            }
            mDevice.ValveConfigurationAndControlCluster().UpdateCurrentState(state);
            return CHIP_NO_ERROR;
        }
        case ValveConfigurationAndControl::Attributes::CurrentLevel::Id: {
            DataModel::Nullable<Percent> level;
            ReturnErrorOnFailure(decoder.Decode(level));
            if (!level.IsNull())
            {
                if (level.Value() == 0)
                {
                    ReturnErrorOnFailure(mDevice.ValveConfigurationAndControlCluster().CloseValve());
                }
                else
                {
                    ReturnErrorOnFailure(mDevice.ValveConfigurationAndControlCluster().OpenValve(
                        level, DataModel::NullNullable));
                }
            }
            return CHIP_NO_ERROR;
        }
        default:
            return CHIP_IM_GLOBAL_STATUS(UnsupportedWrite);
        }
    }
    default:
        return std::nullopt;
    }
}

} // namespace chip::app
