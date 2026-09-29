
/* This code and any associated documentation is provided "as is"

Copyright 2024 Munich Quantum Software Stack Project

Licensed under the Apache License, Version 2.0 with LLVM Exceptions (the
"License"); you may not use this file except in compliance with the License.
You may obtain a copy of the License at

https://github.com/Munich-Quantum-Software-Stack/MQSS-Quantum-Compilation-Suite/blob/develop/LICENSE

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
License for the specific language governing permissions and limitations under
the License.

SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
*/

#pragma once

#include "Utils/DebugUtils.h"
#include "qdmi/client.h"
// #include "qdmi/constants.h"
// #include "qdmi/device.h"
#include "qdmi_example_driver.h"
#include "sc/utils.hpp"

#include <cassert>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <llvm/Support/Error.h>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

// These helpers return llvm::Expected instead of using assert.
// This lets a pass turn a failed QDMI query into a clear error.
// Asserts are compiled out in Release builds so they cannot be trusted.

inline llvm::Error makeQDMIError(const llvm::Twine &what, int ret) {
  return llvm::createStringError(llvm::inconvertibleErrorCode(),
                                 what + " (QDMI error code " +
                                     llvm::Twine(ret) + ")");
}

struct DeviceProperty {
  size_t numQubits = 0;
  CouplingMap cm = {};
};

// A native operation reported by a QDMI device.
// The fields are optional because a device may not report them
// without concrete sites or parameters.
struct OperationInfo {
  std::string name;
  std::optional<size_t> numQubits;
  std::optional<size_t> numParameters;
  std::optional<double> fidelity;
};

inline llvm::Expected<size_t> getDeviceNumQubits(QDMI_Device device) {
  size_t numQubits = 0;
  int ret =
      QDMI_device_query_device_property(device, QDMI_DEVICE_PROPERTY_QUBITSNUM,
                                        sizeof(size_t), &numQubits, nullptr);
  if (ret != QDMI_SUCCESS)
    return makeQDMIError("Could not query the number of qubits", ret);
  return numQubits;
}

inline llvm::Expected<CouplingMap> getDeviceCouplingMap(QDMI_Device device) {
  // Step 1: get the size
  size_t size_ret = 0;
  int ret = QDMI_device_query_device_property(
      device, QDMI_DEVICE_PROPERTY_COUPLINGMAP, 0, nullptr, &size_ret);
  if (ret != QDMI_SUCCESS)
    return makeQDMIError("Could not query the coupling map size", ret);

  MQSS_DEBUG("-->Query coupling map size: " << size_ret << "\n");
  // size_ret = 20 * sizeof(QDMI_Site) for the cxx device
  size_t num_entries = size_ret / sizeof(QDMI_Site); // = 20

  // Step 2: retrieve
  std::vector<QDMI_Site> queired_coupling_map(num_entries);
  ret = QDMI_device_query_device_property(
      device, QDMI_DEVICE_PROPERTY_COUPLINGMAP, size_ret,
      static_cast<void *>(queired_coupling_map.data()), nullptr);
  if (ret != QDMI_SUCCESS)
    return makeQDMIError("Could not query the coupling map", ret);

  MQSS_DEBUG("-->Query coupling map entries: " << queired_coupling_map.size());
  CouplingMap coupling_map_set;
  // Step 3: iterate over pairs
  for (size_t i = 0; i < num_entries; i += 2) {
    QDMI_Site src = queired_coupling_map[i];
    QDMI_Site dst = queired_coupling_map[i + 1];

    // query the index of each site
    uint64_t src_id = 0, dst_id = 0;
    ret = QDMI_device_query_site_property(device, src, QDMI_SITE_PROPERTY_INDEX,
                                          sizeof(uint64_t), &src_id, nullptr);
    if (ret != QDMI_SUCCESS)
      return makeQDMIError("Could not query a coupling map site index", ret);
    ret = QDMI_device_query_site_property(device, dst, QDMI_SITE_PROPERTY_INDEX,
                                          sizeof(uint64_t), &dst_id, nullptr);
    if (ret != QDMI_SUCCESS)
      return makeQDMIError("Could not query a coupling map site index", ret);

    MQSS_DEBUG(src_id << " -> " << dst_id << "\n");
    coupling_map_set.insert({src_id, dst_id});
  }
  return coupling_map_set;
}

// Parses the path and prefix conf file format on its own.
// This function is currently unused.
// Fixed here to always return a value.
inline std::optional<std::tuple<std::string, std::string>>
extractQDMIObj(const std::string &conf_path) {
  std::ifstream conf(conf_path);
  std::string line;

  while (std::getline(conf, line)) {
    // Skip empty lines and comments
    if (line.empty() || line[0] == '#')
      continue;

    std::istringstream iss(line);
    std::string path, prefix;

    if (iss >> path >> prefix) {
      MQSS_DEBUG("QDMI Device SO Path: " << path << "\n");
      MQSS_DEBUG(" QDMI Device Prefix: " << prefix << "\n");
      return std::make_tuple(path, prefix);
    }
  }
  return std::nullopt;
}

inline llvm::Expected<std::string> getDeviceName(QDMI_Device device) {
  size_t namesSize = 0;
  int ret = QDMI_device_query_device_property(device, QDMI_DEVICE_PROPERTY_NAME,
                                              0, nullptr, &namesSize);
  if (ret != QDMI_SUCCESS || namesSize == 0)
    return makeQDMIError("Could not query the device name size", ret);

  std::string name(namesSize - 1, '\0');
  ret = QDMI_device_query_device_property(device, QDMI_DEVICE_PROPERTY_NAME,
                                          namesSize, name.data(), nullptr);
  if (ret != QDMI_SUCCESS)
    return makeQDMIError("Could not query the device name", ret);
  return name;
}

inline void PrintDeviceName(QDMI_Device device) {
  auto name = getDeviceName(device);
  if (!name) {
    llvm::consumeError(name.takeError());
    return;
  }
  MQSS_DEBUG("-->QDMI Device Name: " << *name << "\n");
}

// Initializes the QDMI driver and opens a read-only session.
// Returns the first device when device_name is null.
// Otherwise returns the device whose name matches exactly.
inline llvm::Expected<QDMI_Device>
createQDMIDevice(const char *device_conf_path,
                 const char *device_name = nullptr) {
  MQSS_DEBUG("Getting QDMI device...\n");

  setenv("QDMI_CONF", device_conf_path, 1);

  int ret = QDMI_driver_init();
  if (ret != QDMI_SUCCESS)
    return makeQDMIError(llvm::Twine("Could not initialize the QDMI "
                                     "driver from '") +
                             device_conf_path + "'",
                         ret);

  QDMI_Session session = nullptr;
  ret = QDMI_session_alloc(&session);
  if (ret != QDMI_SUCCESS)
    return makeQDMIError("Could not allocate a QDMI session", ret);

  // Empty token = read-only; non-empty token = read/write
  const char *token = "XX12Mayi98"; // read-only
  ret = QDMI_session_set_parameter(session, QDMI_SESSION_PARAMETER_TOKEN,
                                   strlen(token) + 1, token);
  if (ret != QDMI_SUCCESS)
    return makeQDMIError("Could not set the QDMI session token", ret);

  // Initialize QDMI session
  ret = QDMI_session_init(session); // device sessions are created here
  if (ret != QDMI_SUCCESS)
    return makeQDMIError("Could not initialize the QDMI session", ret);

  // Query the number of devices
  size_t size_ret = 0;
  ret = QDMI_session_query_session_property(
      session, QDMI_SESSION_PROPERTY_DEVICES, 0, nullptr, &size_ret);
  if (ret != QDMI_SUCCESS)
    return makeQDMIError("Could not query the number of QDMI devices", ret);

  size_t num_devices = size_ret / sizeof(QDMI_Device);
  std::vector<QDMI_Device> devices(num_devices);
  ret = QDMI_session_query_session_property(
      session, QDMI_SESSION_PROPERTY_DEVICES, size_ret,
      static_cast<void *>(devices.data()), nullptr);
  if (ret != QDMI_SUCCESS)
    return makeQDMIError("Could not query the QDMI devices", ret);

  MQSS_DEBUG("--> QDMI Num Devices: " << devices.size() << "\n");
  if (devices.empty())
    return llvm::createStringError(llvm::inconvertibleErrorCode(),
                                   "The QDMI session reports no devices");

  if (device_name == nullptr) {
    PrintDeviceName(devices.front());
    return devices.front();
  }

  std::string available;
  for (QDMI_Device device : devices) {
    auto name = getDeviceName(device);
    if (!name)
      return name.takeError();
    if (*name == device_name) {
      PrintDeviceName(device);
      return device;
    }
    available += (available.empty() ? "'" : ", '") + *name + "'";
  }
  return llvm::createStringError(llvm::inconvertibleErrorCode(),
                                 llvm::Twine("No QDMI device named '") +
                                     device_name +
                                     "'; available: " + available);
}

inline llvm::Expected<DeviceProperty> getDeviceProperties(QDMI_Device device) {
  auto numQubits = getDeviceNumQubits(device);
  if (!numQubits)
    return numQubits.takeError();
  auto cm = getDeviceCouplingMap(device);
  if (!cm)
    return cm.takeError();

  DeviceProperty properties;
  properties.numQubits = *numQubits;
  properties.cm = std::move(*cm);
  return properties;
}

// Retrieves the device's native operations.
// Uses the same query size then fill pattern as getDeviceCouplingMap.
inline llvm::Expected<std::vector<QDMI_Operation>>
getDeviceOperations(QDMI_Device device) {
  size_t size_ret = 0;
  int ret = QDMI_device_query_device_property(
      device, QDMI_DEVICE_PROPERTY_OPERATIONS, 0, nullptr, &size_ret);
  if (ret != QDMI_SUCCESS)
    return makeQDMIError("Could not query the number of device operations",
                         ret);

  std::vector<QDMI_Operation> operations(size_ret / sizeof(QDMI_Operation));
  if (operations.empty())
    return operations;
  ret = QDMI_device_query_device_property(
      device, QDMI_DEVICE_PROPERTY_OPERATIONS, size_ret,
      static_cast<void *>(operations.data()), nullptr);
  if (ret != QDMI_SUCCESS)
    return makeQDMIError("Could not query the device operations", ret);
  return operations;
}

// Queries an operation property without sites or parameters.
// A device may return QDMI_ERROR_NOTSUPPORTED, which maps to std::nullopt.
// Any other failure is a real error.
template <typename T>
llvm::Expected<std::optional<T>>
queryOperationProperty(QDMI_Device device, QDMI_Operation operation,
                       QDMI_Operation_Property prop) {
  T value{};
  int ret = QDMI_device_query_operation_property(device, operation, 0, nullptr,
                                                 0, nullptr, prop, sizeof(T),
                                                 &value, nullptr);
  if (ret == QDMI_ERROR_NOTSUPPORTED)
    return std::optional<T>();
  if (ret != QDMI_SUCCESS)
    return makeQDMIError("Could not query an operation property", ret);
  return std::optional<T>(value);
}

inline llvm::Expected<OperationInfo>
getOperationInfo(QDMI_Device device, QDMI_Operation operation) {
  OperationInfo info;

  size_t nameSize = 0;
  int ret = QDMI_device_query_operation_property(
      device, operation, 0, nullptr, 0, nullptr, QDMI_OPERATION_PROPERTY_NAME,
      0, nullptr, &nameSize);
  if (ret != QDMI_SUCCESS || nameSize == 0)
    return makeQDMIError("Could not query the operation name size", ret);
  info.name.assign(nameSize - 1, '\0');
  ret = QDMI_device_query_operation_property(
      device, operation, 0, nullptr, 0, nullptr, QDMI_OPERATION_PROPERTY_NAME,
      nameSize, info.name.data(), nullptr);
  if (ret != QDMI_SUCCESS)
    return makeQDMIError("Could not query the operation name", ret);

  auto numQubits = queryOperationProperty<size_t>(
      device, operation, QDMI_OPERATION_PROPERTY_QUBITSNUM);
  if (!numQubits)
    return numQubits.takeError();
  info.numQubits = *numQubits;

  auto numParameters = queryOperationProperty<size_t>(
      device, operation, QDMI_OPERATION_PROPERTY_PARAMETERSNUM);
  if (!numParameters)
    return numParameters.takeError();
  info.numParameters = *numParameters;

  // Fidelity is best effort.
  // A device may only report it for concrete sites.
  auto fidelity = queryOperationProperty<double>(
      device, operation, QDMI_OPERATION_PROPERTY_FIDELITY);
  if (!fidelity)
    return fidelity.takeError();
  info.fidelity = *fidelity;

  MQSS_DEBUG("-->QDMI Operation: " << info.name << "\n");
  return info;
}

// Queries the device's operations and their properties once.
// Callers should cache the result instead of querying per IR operation.
inline llvm::Expected<std::vector<OperationInfo>>
getDeviceNativeGateSet(QDMI_Device device) {
  auto operations = getDeviceOperations(device);
  if (!operations)
    return operations.takeError();

  std::vector<OperationInfo> gate_set;
  gate_set.reserve(operations->size());
  for (QDMI_Operation operation : *operations) {
    auto info = getOperationInfo(device, operation);
    if (!info)
      return info.takeError();
    gate_set.push_back(std::move(*info));
  }
  return gate_set;
}
