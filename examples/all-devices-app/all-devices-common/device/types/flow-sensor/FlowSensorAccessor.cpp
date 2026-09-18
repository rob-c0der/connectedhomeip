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

#include <access/SubjectDescriptor.h>
#include <device/types/flow-sensor/FlowSensorAccessor.h>
#include <lib/support/CodeUtils.h>
#include <lib/support/logging/CHIPLogging.h>
#include <oob-accessors/OOBDataSerializer.h>

using namespace chip::app::Clusters;

namespace chip::app {

std::optional<CHIP_ERROR> FlowSensorAccessor::HandleAction(CharSpan actionName, ByteSpan tlvBuffer)
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

std::optional<CHIP_ERROR> FlowSensorAccessor::SetAttribute(const ConcreteDataAttributePath & path,
                                                           AttributeValueDecoder & decoder)
{
    // Stop background simulation loop if explicit test values are injected
    mDevice.PauseSimulation();

    switch (path.mClusterId)
    {
    case FlowMeasurement::Id: {
        switch (path.mAttributeId)
        {
        case FlowMeasurement::Attributes::MeasuredValue::Id: {
            DataModel::Nullable<uint16_t> measuredValue;
            ReturnErrorOnFailure(decoder.Decode(measuredValue));
            return mDevice.FlowMeasurementCluster().SetMeasuredValue(measuredValue);
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
