/*
 *
 *    Copyright (c) 2026 Project CHIP Authors
 *    All rights reserved.
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

#include <app/persistence/DefaultAttributePersistenceProvider.h>
#include <data-model-providers/codedriven/CodeDrivenDataModelProvider.h>
#include <device/types/soil-sensor/impl/IncreasingMoistureSoilSensor.h>
#include <lib/core/TLV.h>
#include <lib/support/TestPersistentStorageDelegate.h>
#include <oob-accessors/OOBDataSerializer.h>
#include <device/types/soil-sensor/SoilSensorAccessor.h>
#include <platform/CHIPDeviceLayer.h>
#include <pw_unit_test/framework.h>

using namespace chip;
using namespace chip::app;
using namespace chip::app::Clusters;

class TestSoilSensorAccessor : public ::testing::Test
{
public:
    static void SetUpTestSuite()
    {
        ASSERT_EQ(chip::Platform::MemoryInit(), CHIP_NO_ERROR);
        ASSERT_EQ(chip::DeviceLayer::PlatformMgr().InitChipStack(), CHIP_NO_ERROR);
    }
    static void TearDownTestSuite()
    {
        chip::DeviceLayer::PlatformMgr().Shutdown();
        chip::Platform::MemoryShutdown();
    }

protected:
    TestPersistentStorageDelegate mStorage;
    DefaultAttributePersistenceProvider mAttrStorage;
    CodeDrivenDataModelProvider mProvider;
    IncreasingMoistureSoilSensor mDevice;

    TestSoilSensorAccessor() : mProvider(mStorage, mAttrStorage)
    {
        EXPECT_EQ(mAttrStorage.Init(&mStorage), CHIP_NO_ERROR);
    }

    void SetUp() override
    {
        EXPECT_EQ(mDevice.Register(1, mProvider), CHIP_NO_ERROR);
    }

    void TearDown() override
    {
        mDevice.Unregister(mProvider);
    }
};

// Test 1: Setting 4200 (Percent100ths) changes moisture to 42%
TEST_F(TestSoilSensorAccessor, SetMoisture_ScaledValue4200)
{
    SoilSensorAccessor accessor(mDevice);
    ConcreteDataAttributePath path(1, SoilMeasurement::Id, SoilMeasurement::Attributes::SoilMoistureMeasuredValue::Id);

    uint8_t buffer[64];
    TLV::TLVWriter writer;
    writer.Init(buffer);
    EXPECT_EQ(writer.Put(TLV::AnonymousTag(), static_cast<uint16_t>(4200)), CHIP_NO_ERROR);
    EXPECT_EQ(writer.Finalize(), CHIP_NO_ERROR);

    TLV::TLVReader reader;
    reader.Init(buffer, writer.GetLengthWritten());
    EXPECT_EQ(reader.Next(), CHIP_NO_ERROR);

    auto buildResult = OOBDataSerializer::BuildSetAttributeRequest(path, reader);
    ASSERT_FALSE(std::holds_alternative<CHIP_ERROR>(buildResult));

    auto & requestBuffer = std::get<ReadOnlyBuffer<uint8_t>>(buildResult);
    auto status          = accessor.HandleAction("SetAttribute"_span, ByteSpan(requestBuffer.data(), requestBuffer.size()));
    ASSERT_TRUE(status.has_value());
    EXPECT_EQ(status.value(), CHIP_NO_ERROR);

    auto moisture = mDevice.SoilMeasurementCluster().GetSoilMoistureMeasuredValue();
    ASSERT_FALSE(moisture.IsNull());
    EXPECT_EQ(moisture.Value(), 42);
}

// Test 1b: Setting 42 (standard Percent) changes moisture to 42%
TEST_F(TestSoilSensorAccessor, SetMoisture_DirectValue42)
{
    SoilSensorAccessor accessor(mDevice);
    ConcreteDataAttributePath path(1, SoilMeasurement::Id, SoilMeasurement::Attributes::SoilMoistureMeasuredValue::Id);

    uint8_t buffer[64];
    TLV::TLVWriter writer;
    writer.Init(buffer);
    EXPECT_EQ(writer.Put(TLV::AnonymousTag(), static_cast<uint8_t>(42)), CHIP_NO_ERROR);
    EXPECT_EQ(writer.Finalize(), CHIP_NO_ERROR);

    TLV::TLVReader reader;
    reader.Init(buffer, writer.GetLengthWritten());
    EXPECT_EQ(reader.Next(), CHIP_NO_ERROR);

    auto buildResult = OOBDataSerializer::BuildSetAttributeRequest(path, reader);
    ASSERT_FALSE(std::holds_alternative<CHIP_ERROR>(buildResult));

    auto & requestBuffer = std::get<ReadOnlyBuffer<uint8_t>>(buildResult);
    auto status          = accessor.HandleAction("SetAttribute"_span, ByteSpan(requestBuffer.data(), requestBuffer.size()));
    ASSERT_TRUE(status.has_value());
    EXPECT_EQ(status.value(), CHIP_NO_ERROR);

    auto moisture = mDevice.SoilMeasurementCluster().GetSoilMoistureMeasuredValue();
    ASSERT_FALSE(moisture.IsNull());
    EXPECT_EQ(moisture.Value(), 42);
}

// Test 2: Setting 0 changes temp to 0 deg C
TEST_F(TestSoilSensorAccessor, SetTemperature_ZeroDegrees)
{
    SoilSensorAccessor accessor(mDevice);
    ConcreteDataAttributePath path(1, TemperatureMeasurement::Id, TemperatureMeasurement::Attributes::MeasuredValue::Id);

    uint8_t buffer[64];
    TLV::TLVWriter writer;
    writer.Init(buffer);
    EXPECT_EQ(writer.Put(TLV::AnonymousTag(), static_cast<int16_t>(0)), CHIP_NO_ERROR);
    EXPECT_EQ(writer.Finalize(), CHIP_NO_ERROR);

    TLV::TLVReader reader;
    reader.Init(buffer, writer.GetLengthWritten());
    EXPECT_EQ(reader.Next(), CHIP_NO_ERROR);

    auto buildResult = OOBDataSerializer::BuildSetAttributeRequest(path, reader);
    ASSERT_FALSE(std::holds_alternative<CHIP_ERROR>(buildResult));

    auto & requestBuffer = std::get<ReadOnlyBuffer<uint8_t>>(buildResult);
    auto status          = accessor.HandleAction("SetAttribute"_span, ByteSpan(requestBuffer.data(), requestBuffer.size()));
    ASSERT_TRUE(status.has_value());
    EXPECT_EQ(status.value(), CHIP_NO_ERROR);

    auto temp = mDevice.TemperatureMeasurementCluster().GetMeasuredValue();
    ASSERT_FALSE(temp.IsNull());
    EXPECT_EQ(temp.Value(), 0);
}

// Test 2b: Setting 2150 (21.50 deg C from CUJ 02)
TEST_F(TestSoilSensorAccessor, SetTemperature_2150Degrees)
{
    SoilSensorAccessor accessor(mDevice);
    ConcreteDataAttributePath path(1, TemperatureMeasurement::Id, TemperatureMeasurement::Attributes::MeasuredValue::Id);

    uint8_t buffer[64];
    TLV::TLVWriter writer;
    writer.Init(buffer);
    EXPECT_EQ(writer.Put(TLV::AnonymousTag(), static_cast<int16_t>(2150)), CHIP_NO_ERROR);
    EXPECT_EQ(writer.Finalize(), CHIP_NO_ERROR);

    TLV::TLVReader reader;
    reader.Init(buffer, writer.GetLengthWritten());
    EXPECT_EQ(reader.Next(), CHIP_NO_ERROR);

    auto buildResult = OOBDataSerializer::BuildSetAttributeRequest(path, reader);
    ASSERT_FALSE(std::holds_alternative<CHIP_ERROR>(buildResult));

    auto & requestBuffer = std::get<ReadOnlyBuffer<uint8_t>>(buildResult);
    auto status          = accessor.HandleAction("SetAttribute"_span, ByteSpan(requestBuffer.data(), requestBuffer.size()));
    ASSERT_TRUE(status.has_value());
    EXPECT_EQ(status.value(), CHIP_NO_ERROR);

    auto temp = mDevice.TemperatureMeasurementCluster().GetMeasuredValue();
    ASSERT_FALSE(temp.IsNull());
    EXPECT_EQ(temp.Value(), 2150);
}

// Test 3: Setting 24 changes battery to 12% (24 half-percent units)
TEST_F(TestSoilSensorAccessor, SetBattery_24HalfPercent)
{
    SoilSensorAccessor accessor(mDevice);
    ConcreteDataAttributePath path(1, PowerSource::Id, PowerSource::Attributes::BatPercentRemaining::Id);

    uint8_t buffer[64];
    TLV::TLVWriter writer;
    writer.Init(buffer);
    EXPECT_EQ(writer.Put(TLV::AnonymousTag(), static_cast<uint8_t>(24)), CHIP_NO_ERROR);
    EXPECT_EQ(writer.Finalize(), CHIP_NO_ERROR);

    TLV::TLVReader reader;
    reader.Init(buffer, writer.GetLengthWritten());
    EXPECT_EQ(reader.Next(), CHIP_NO_ERROR);

    auto buildResult = OOBDataSerializer::BuildSetAttributeRequest(path, reader);
    ASSERT_FALSE(std::holds_alternative<CHIP_ERROR>(buildResult));

    auto & requestBuffer = std::get<ReadOnlyBuffer<uint8_t>>(buildResult);
    auto status          = accessor.HandleAction("SetAttribute"_span, ByteSpan(requestBuffer.data(), requestBuffer.size()));
    ASSERT_TRUE(status.has_value());
    EXPECT_EQ(status.value(), CHIP_NO_ERROR);

    auto battery = mDevice.PowerSourceCluster().GetBatPercentRemaining();
    ASSERT_FALSE(battery.IsNull());
    EXPECT_EQ(battery.Value(), 24);
}

// Test 4: Setting null triggers probe fault
TEST_F(TestSoilSensorAccessor, SetMoisture_NullProbeFault)
{
    SoilSensorAccessor accessor(mDevice);
    ConcreteDataAttributePath path(1, SoilMeasurement::Id, SoilMeasurement::Attributes::SoilMoistureMeasuredValue::Id);

    uint8_t buffer[64];
    TLV::TLVWriter writer;
    writer.Init(buffer);
    EXPECT_EQ(writer.PutNull(TLV::AnonymousTag()), CHIP_NO_ERROR);
    EXPECT_EQ(writer.Finalize(), CHIP_NO_ERROR);

    TLV::TLVReader reader;
    reader.Init(buffer, writer.GetLengthWritten());
    EXPECT_EQ(reader.Next(), CHIP_NO_ERROR);

    auto buildResult = OOBDataSerializer::BuildSetAttributeRequest(path, reader);
    ASSERT_FALSE(std::holds_alternative<CHIP_ERROR>(buildResult));

    auto & requestBuffer = std::get<ReadOnlyBuffer<uint8_t>>(buildResult);
    auto status          = accessor.HandleAction("SetAttribute"_span, ByteSpan(requestBuffer.data(), requestBuffer.size()));
    ASSERT_TRUE(status.has_value());
    EXPECT_EQ(status.value(), CHIP_NO_ERROR);

    auto moisture = mDevice.SoilMeasurementCluster().GetSoilMoistureMeasuredValue();
    EXPECT_TRUE(moisture.IsNull());
}

// Test 5: Pauses simulation on write
TEST_F(TestSoilSensorAccessor, SimulationPausedOnWrite)
{
    SoilSensorAccessor accessor(mDevice);
    ConcreteDataAttributePath path(1, SoilMeasurement::Id, SoilMeasurement::Attributes::SoilMoistureMeasuredValue::Id);

    uint8_t buffer[64];
    TLV::TLVWriter writer;
    writer.Init(buffer);
    EXPECT_EQ(writer.Put(TLV::AnonymousTag(), static_cast<uint8_t>(42)), CHIP_NO_ERROR);
    EXPECT_EQ(writer.Finalize(), CHIP_NO_ERROR);

    TLV::TLVReader reader;
    reader.Init(buffer, writer.GetLengthWritten());
    EXPECT_EQ(reader.Next(), CHIP_NO_ERROR);

    auto buildResult = OOBDataSerializer::BuildSetAttributeRequest(path, reader);
    ASSERT_FALSE(std::holds_alternative<CHIP_ERROR>(buildResult));

    auto & requestBuffer = std::get<ReadOnlyBuffer<uint8_t>>(buildResult);
    auto status          = accessor.HandleAction("SetAttribute"_span, ByteSpan(requestBuffer.data(), requestBuffer.size()));
    ASSERT_TRUE(status.has_value());
    EXPECT_EQ(status.value(), CHIP_NO_ERROR);

    // Call TimerFired to ensure simulation does not change values
    mDevice.TimerFired();

    auto moisture = mDevice.SoilMeasurementCluster().GetSoilMoistureMeasuredValue();
    ASSERT_FALSE(moisture.IsNull());
    EXPECT_EQ(moisture.Value(), 42);
}

// Test 6: Wrong action or wrong endpoint
TEST_F(TestSoilSensorAccessor, UnhandledActionOrEndpoint)
{
    SoilSensorAccessor accessor(mDevice);

    // Wrong action
    uint8_t dummy[4] = { 0 };
    auto actionStatus = accessor.HandleAction("UnknownAction"_span, ByteSpan(dummy, sizeof(dummy)));
    EXPECT_FALSE(actionStatus.has_value());

    // Wrong endpoint (e.g. endpoint 2)
    ConcreteDataAttributePath path(2, SoilMeasurement::Id, SoilMeasurement::Attributes::SoilMoistureMeasuredValue::Id);
    uint8_t buffer[64];
    TLV::TLVWriter writer;
    writer.Init(buffer);
    EXPECT_EQ(writer.Put(TLV::AnonymousTag(), static_cast<uint8_t>(50)), CHIP_NO_ERROR);
    EXPECT_EQ(writer.Finalize(), CHIP_NO_ERROR);

    TLV::TLVReader reader;
    reader.Init(buffer, writer.GetLengthWritten());
    EXPECT_EQ(reader.Next(), CHIP_NO_ERROR);

    auto buildResult = OOBDataSerializer::BuildSetAttributeRequest(path, reader);
    ASSERT_FALSE(std::holds_alternative<CHIP_ERROR>(buildResult));

    auto & requestBuffer = std::get<ReadOnlyBuffer<uint8_t>>(buildResult);
    auto epStatus        = accessor.HandleAction("SetAttribute"_span, ByteSpan(requestBuffer.data(), requestBuffer.size()));
    EXPECT_FALSE(epStatus.has_value());
}
