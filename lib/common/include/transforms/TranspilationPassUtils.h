
/* This code and any associated documentation is provided "as is"

Copyright 2024 Munich Quantum Software Stack Project

Licensed under the Apache License, Version 2.0 with LLVM Exceptions (the
"License"); you may not use this file except in compliance with the License.
You may obtain a copy of the License at

https://github.com/Munich-Quantum-Software-Stack/passes/blob/develop/LICENSE

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
License for the specific language governing permissions and limitations under
the License.

SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
*************************************************************************
  author Akshay Bhosale
  date   June 2026
  version 1.0
*************************************************************************/

#include "QdmiDriver.h"
#include "driver/qdmi_example_driver.h"
#include "include/transforms/PassUtils.h"
#include "qdmi/client.h"
#include "qdmi/constants.h"
#include "qdmi/device.h"
#include "sc/utils.hpp"

#include <cassert>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

struct DeviceProperty {
  int numQubits = -223;
  CouplingMap cm = {};
};

static int getDeviceNumQubits(QDMI_Device device) {

  // Step 1: get the size
  size_t size_ret = 0;
  int ret = -223;
  int numQubits;

  ret = QDMI_device_query_device_property(
      device, QDMI_DEVICE_PROPERTY_QUBITSNUM,
      sizeof(size_t), // ← size of the buffer you're providing
      &numQubits,     // ← pointer to receive the value directly
      nullptr         // ← size_ret not needed);
  );
  assert(ret == QDMI_SUCCESS);
  return numQubits;
}
static CouplingMap getDeviceCouplingMap(QDMI_Device device) {
  // Step 1: get the size
  size_t size_ret = 0;
  int ret = -223;
  ret = QDMI_device_query_device_property(
      device, QDMI_DEVICE_PROPERTY_COUPLINGMAP, 0, nullptr, &size_ret);

  MQSS_DEBUG("-->Query coupling map size: " << size_ret << "\n");
  assert(ret == QDMI_SUCCESS);
  // size_ret = 20 * sizeof(QDMI_Site) for the cxx device
  size_t num_entries = size_ret / sizeof(QDMI_Site); // = 20
  size_t num_pairs = num_entries / 2;                // = 10

  // Step 2: retrieve
  std::vector<QDMI_Site> queired_coupling_map(num_entries);
  ret = QDMI_device_query_device_property(
      device, QDMI_DEVICE_PROPERTY_COUPLINGMAP, size_ret,
      static_cast<void *>(queired_coupling_map.data()), nullptr);

  MQSS_DEBUG("-->Query coupling map entries: " << queired_coupling_map.size());
  assert(ret == QDMI_SUCCESS);
  CouplingMap coupling_map_set;
  // Step 3: iterate over pairs
  for (size_t i = 0; i < num_entries; i += 2) {
    QDMI_Site src = queired_coupling_map[i];
    QDMI_Site dst = queired_coupling_map[i + 1];

    // query the index of each site
    uint64_t src_id = 0, dst_id = 0;
    QDMI_device_query_site_property(device, src, QDMI_SITE_PROPERTY_INDEX,
                                    sizeof(uint64_t), &src_id, nullptr);
    QDMI_device_query_site_property(device, dst, QDMI_SITE_PROPERTY_INDEX,
                                    sizeof(uint64_t), &dst_id, nullptr);

    MQSS_DEBUG(src_id << " -> " << dst_id << "\n");
    coupling_map_set.insert({src_id, dst_id});
  }
  return coupling_map_set;
}

static std::tuple<string, string> extractQDMIObj(std::string conf_path) {
  std::ifstream conf(conf_path);
  std::string line;

  while (std::getline(conf, line)) {
    // Skip empty lines and comments
    if (line.empty() || line[0] == '#')
      continue;

    std::istringstream iss(line);
    std::string path, prefix;

    if (iss >> path >> prefix) {
      MQSS_DEBUG("QDMI Device SO Path: " << path);
      MQSS_DEBUG(" QDMI Device Prefix: " << prefix);
      return std::make_tuple(path, prefix);
    }
  }
}

// Load the device library dynamically
static std::pair<std::reference_wrapper<qdmi_main_driver::Driver>, QDMI_Device>
addDynamicDeviceLibrary(const std::string &libName, const std::string &prefix) {

  qdmi_main_driver::DeviceSessionConfig config;
  // If connecting to a remote device, use:
  // config.baseUrl = "http://localhost:8080";
  // config.token = "test_token";
  // config.authUrl = "https://auth.example.com";
  // config.username = "user";
  // config.password = "pass";
  // config.custom1 = "value1";
  // config.custom2 = "value2";
  // config.custom3 = "value3";
  // config.custom4 = "value4";
  // config.custom5 = "value5";

  auto &driver = qdmi_main_driver::Driver::get();

  auto *device = driver.addDynamicDeviceLibrary(libName, prefix, config);
  size_t namesSize = 0;
  size_t ret = 0;
  ret = QDMI_device_query_device_property(device, QDMI_DEVICE_PROPERTY_NAME, 0,
                                          nullptr, &namesSize);

  assert(ret == QDMI_SUCCESS);
  std::string name(namesSize - 1, '\0');
  ret = QDMI_device_query_device_property(device, QDMI_DEVICE_PROPERTY_NAME,
                                          namesSize, name.data(), nullptr);

  assert(ret == QDMI_SUCCESS);
  return std::make_pair(std::ref(driver), device);
}

static QDMI_Device createQDMIDevice(const char *device_conf_path) {
  QDMI_Job job = nullptr;

  MQSS_DEBUG("Initializing QDMI driver...\n");
  auto [libName, prefix] = extractQDMIObj(device_conf_path);
  auto [driver_ref, device] = addDynamicDeviceLibrary(libName, prefix);
  return device;
}

static DeviceProperty getDeviceProperties(QDMI_Device Device) {

  DeviceProperty Properties;
  Properties.numQubits = getDeviceNumQubits(Device);
  Properties.cm = getDeviceCouplingMap(Device);

  return Properties;
}
