/*
 * Copyright (C) 2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <errno.h>
#include <math.h>
#include <vector>
#include <sys/socket.h>
#include <cutils/sockets.h>
#include <cutils/log.h>
#include <hardware/hardware.h>
#include <hardware/thermal.h>

#include "ExtThermal.h"

#undef LOG_TAG
#define LOG_TAG "vendor.sprd.hardware.thermal@1.0-impl"
#define SOCKET_NAME "thermald"

namespace vendor {
namespace sprd {
namespace hardware {
namespace thermal {
namespace V1_0 {
namespace implementation {


namespace {

float finalizeTemperature(float temperature) {
    return temperature == UNKNOWN_TEMPERATURE ? NAN : temperature;
}

}

ExtThermal::ExtThermal(thermal_module_t* module) : mModule(module) {}

// Methods from ::android::hardware::thermal::V1_0::IThermal follow.
Return<void> ExtThermal::getTemperatures(getTemperatures_cb _hidl_cb) {
  ThermalStatus status;
  status.code = ThermalStatusCode::SUCCESS;
  hidl_vec<Temperature> temperatures;

  if (!mModule || !mModule->getTemperatures) {
    ALOGI("getTemperatures is not implemented in Thermal HAL.");
    _hidl_cb(status, temperatures);
    return Void();
  }

  ssize_t size = mModule->getTemperatures(mModule, nullptr, 0);
  if (size >= 0) {
    std::vector<temperature_t> list;
    list.resize(size);
    size = mModule->getTemperatures(mModule, list.data(), list.size());
    if (size >= 0) {
      temperatures.resize(list.size());
      for (size_t i = 0; i < list.size(); ++i) {
        switch (list[i].type) {
          case DEVICE_TEMPERATURE_UNKNOWN:
            temperatures[i].type = TemperatureType::UNKNOWN;
            break;
          case DEVICE_TEMPERATURE_CPU:
            temperatures[i].type = TemperatureType::CPU;
            break;
          case DEVICE_TEMPERATURE_GPU:
            temperatures[i].type = TemperatureType::GPU;
            break;
          case DEVICE_TEMPERATURE_BATTERY:
            temperatures[i].type = TemperatureType::BATTERY;
            break;
          case DEVICE_TEMPERATURE_SKIN:
            temperatures[i].type = TemperatureType::SKIN;
            break;
          default:
            ALOGE("Unknown temperature %s type", list[i].name);
            ;
        }
        temperatures[i].name = list[i].name;
        temperatures[i].currentValue = finalizeTemperature(list[i].current_value);
        temperatures[i].throttlingThreshold = finalizeTemperature(list[i].throttling_threshold);
        temperatures[i].shutdownThreshold = finalizeTemperature(list[i].shutdown_threshold);
        temperatures[i].vrThrottlingThreshold =
                finalizeTemperature(list[i].vr_throttling_threshold);
      }
    }
  }
  if (size < 0) {
    status.code = ThermalStatusCode::FAILURE;
    status.debugMessage = strerror(-size);
  }
  _hidl_cb(status, temperatures);
  return Void();
}

Return<void> ExtThermal::getCpuUsages(getCpuUsages_cb _hidl_cb) {
  ThermalStatus status;
  hidl_vec<CpuUsage> cpuUsages;
  status.code = ThermalStatusCode::SUCCESS;

  if (!mModule || !mModule->getCpuUsages) {
    ALOGI("getCpuUsages is not implemented in Thermal HAL");
    _hidl_cb(status, cpuUsages);
    return Void();
  }

  ssize_t size = mModule->getCpuUsages(mModule, nullptr);
  if (size >= 0) {
    std::vector<cpu_usage_t> list;
    list.resize(size);
    size = mModule->getCpuUsages(mModule, list.data());
    if (size >= 0) {
      list.resize(size);
      cpuUsages.resize(size);
      for (size_t i = 0; i < list.size(); ++i) {
        cpuUsages[i].name = list[i].name;
        cpuUsages[i].active = list[i].active;
        cpuUsages[i].total = list[i].total;
        cpuUsages[i].isOnline = list[i].is_online;
      }
    } else {
      status.code = ThermalStatusCode::FAILURE;
      status.debugMessage = strerror(-size);
    }
  }
  if (size < 0) {
    status.code = ThermalStatusCode::FAILURE;
    status.debugMessage = strerror(-size);
  }
  _hidl_cb(status, cpuUsages);
  return Void();
}

Return<void> ExtThermal::getCoolingDevices(getCoolingDevices_cb _hidl_cb) {
  ThermalStatus status;
  status.code = ThermalStatusCode::SUCCESS;
  hidl_vec<CoolingDevice> coolingDevices;

  if (!mModule || !mModule->getCoolingDevices) {
    ALOGI("getCoolingDevices is not implemented in Thermal HAL.");
    _hidl_cb(status, coolingDevices);
    return Void();
  }

  ssize_t size = mModule->getCoolingDevices(mModule, nullptr, 0);
  if (size >= 0) {
    std::vector<cooling_device_t> list;
    list.resize(size);
    size = mModule->getCoolingDevices(mModule, list.data(), list.size());
    if (size >= 0) {
      list.resize(size);
      coolingDevices.resize(list.size());
      for (size_t i = 0; i < list.size(); ++i) {
        switch (list[i].type) {
          case FAN_RPM:
            coolingDevices[i].type = CoolingType::FAN_RPM;
            break;
          default:
            ALOGE("Unknown cooling device %s type", list[i].name);
        }
        coolingDevices[i].name = list[i].name;
        coolingDevices[i].currentValue = list[i].current_value;
      }
    }
  }
  if (size < 0) {
    status.code = ThermalStatusCode::FAILURE;
    status.debugMessage = strerror(-size);
  }
  _hidl_cb(status, coolingDevices);
  return Void();
}

// Methods from IExtThermal follow.
Return<void> ExtThermal::setExtThermal(ExtThermalCmd cmd) {

    int fd;
    char buf[32];
    fd = socket_local_client(SOCKET_NAME, ANDROID_SOCKET_NAMESPACE_ABSTRACT, SOCK_STREAM);
    if (fd < 0) {
        ALOGE("open socket: %s fail: %d", SOCKET_NAME, fd);
        return Void();
    }
    ALOGI("send msg: %d to thermald", cmd);
    sprintf(buf, "%d", cmd);
    send(fd, buf, strlen(buf), 0);
    close(fd);

    return Void();
}

Return<bool> ExtThermal::getExtThermal(ExtThermalCmd cmd) {

    int fd, len;
    char buf[32];
    fd = socket_local_client(SOCKET_NAME, ANDROID_SOCKET_NAMESPACE_ABSTRACT, SOCK_STREAM);
    if (fd < 0) {
        ALOGE("open socket: %s fail: %d", SOCKET_NAME, fd);
        return false;
    }
    ALOGI("send msg: %d to thermald", cmd);
    sprintf(buf, "%d", cmd);
    send(fd, buf, strlen(buf), 0);
    len = recv(fd, buf, sizeof(buf), 0);
    close(fd);
    if (!strncmp(buf, "true", 4)) {
        return true;
    }

    return false;
}

Return<void> ExtThermal::setScenario(const hidl_string& scenario) {
    int fd, len;
    char buf[64];
    fd = socket_local_client(SOCKET_NAME, ANDROID_SOCKET_NAMESPACE_ABSTRACT, SOCK_STREAM);
    if (fd < 0) {
        ALOGE("open socket: %s fail: %d", SOCKET_NAME, fd);
        return Void();
    }
    sprintf(buf, "%d,%s", ExtThermalCmd::THMCMD_SET_SCENARIO, scenario.c_str());
    ALOGI("send msg: %s to thermald", buf);
    send(fd, buf, strlen(buf), 0);
    close(fd);

    return Void();
}

Return<void> ExtThermal::registerThermalCallback(const sp<IThermalCallback>& callback) {
    // TODO implement
    return Void();
}

IExtThermal* HIDL_FETCH_IExtThermal(const char* /* name */) {
  thermal_module_t* module;
  int err = hw_get_module(THERMAL_HARDWARE_MODULE_ID,
                               const_cast<hw_module_t const**>(
                                   reinterpret_cast<hw_module_t**>(&module)));
  if (err || !module) {
    ALOGE("Couldn't load %s module (%s)", THERMAL_HARDWARE_MODULE_ID,
          strerror(-err));
  }

  if (err == 0 && module->common.methods->open) {
    struct hw_device_t* device;
    err = module->common.methods->open(&module->common,
                                       THERMAL_HARDWARE_MODULE_ID, &device);
    if (err) {
      ALOGE("Couldn't open %s module (%s)", THERMAL_HARDWARE_MODULE_ID,
            strerror(-err));
    } else {
      return new ExtThermal(reinterpret_cast<thermal_module_t*>(device));
    }
  }
  return new ExtThermal(module);
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace thermal
}  // namespace hardware
}  // namespace sprd
}  // namespace vendor
