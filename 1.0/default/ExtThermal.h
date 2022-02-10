#ifndef VENDOR_SPRD_HARDWARE_THERMAL_V1_0_EXTTHERMAL_H
#define VENDOR_SPRD_HARDWARE_THERMAL_V1_0_EXTTHERMAL_H

#include <vendor/sprd/hardware/thermal/1.0/IExtThermal.h>
#include <hidl/MQDescriptor.h>
#include <hidl/Status.h>
#include <hardware/thermal.h>

namespace vendor {
namespace sprd {
namespace hardware {
namespace thermal {
namespace V1_0 {
namespace implementation {

using ::android::hardware::thermal::V1_0::CoolingDevice;
using ::android::hardware::thermal::V1_0::CpuUsage;
using ::android::hardware::thermal::V1_0::Temperature;
using ::android::hardware::thermal::V1_0::ThermalStatus;
using ::android::hardware::thermal::V1_0::ThermalStatusCode;
using ::android::hardware::thermal::V1_0::TemperatureType;
using ::android::hardware::thermal::V1_0::CoolingType;
using ::android::hardware::thermal::V1_1::IThermalCallback;
using ::vendor::sprd::hardware::thermal::V1_0::ExtThermalCmd;

using ::android::hardware::hidl_array;
using ::android::hardware::hidl_memory;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::sp;

struct ExtThermal : public IExtThermal {
    ExtThermal(thermal_module_t* module);
    // Methods from ::android::hardware::thermal::V1_0::IThermal follow.
    Return<void> getTemperatures(getTemperatures_cb _hidl_cb) override;
    Return<void> getCpuUsages(getCpuUsages_cb _hidl_cb) override;
    Return<void> getCoolingDevices(getCoolingDevices_cb _hidl_cb) override;
    Return<void> registerThermalCallback(const sp<IThermalCallback>& callback);

    // Methods from IExtThermal follow.
    Return<void> setExtThermal(ExtThermalCmd cmd) override;
    Return<bool> getExtThermal(ExtThermalCmd cmd) override;
    Return<void> setScenario(const hidl_string& scenario) override;
    private:
        thermal_module_t* mModule;
};

// FIXME: most likely delete, this is only for passthrough implementations
extern "C" IExtThermal* HIDL_FETCH_IExtThermal(const char* name);

}  // namespace implementation
}  // namespace V1_0
}  // namespace thermal
}  // namespace hardware
}  // namespace sprd
}  // namespace vendor

#endif  // VENDOR_SPRD_HARDWARE_THERMAL_V1_0_EXTTHERMAL_H
