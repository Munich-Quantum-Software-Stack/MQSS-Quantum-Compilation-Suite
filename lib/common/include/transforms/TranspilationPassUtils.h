

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
#include <utility>
#include <vector>

#define QDMI_CONF_PATH "cxx_qdmi.conf"

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

  std::cout << "-->Query coupling map size: " << size_ret << "\n";
  assert(ret == QDMI_SUCCESS);
  // size_ret = 20 * sizeof(QDMI_Site) for the cxx device
  size_t num_entries = size_ret / sizeof(QDMI_Site); // = 20
  size_t num_pairs = num_entries / 2;                // = 10

  // Step 2: retrieve
  std::vector<QDMI_Site> queired_coupling_map(num_entries);
  ret = QDMI_device_query_device_property(
      device, QDMI_DEVICE_PROPERTY_COUPLINGMAP, size_ret,
      static_cast<void *>(queired_coupling_map.data()), nullptr);

  std::cout << "-->Query coupling map entries: " << queired_coupling_map.size()
            << "\n";
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

static QDMI_Device createQDMIDevice(const char *device_conf_path) {
  QDMI_Job job = nullptr;
  int num_shots = 1000;

  MQSS_DEBUG("Initializing QDMI driver...\n");

  setenv("QDMI_CONF", device_conf_path, 1);

  MQSS_DEBUG("CWD: " << std::filesystem::current_path() << "\n");
  MQSS_DEBUG("QDMI_CONF: " << std::getenv("QDMI_CONF") << "\n");

  int ret = QDMI_driver_init();
  assert(ret == QDMI_SUCCESS);

  QDMI_Session session = nullptr;
  ret = QDMI_session_alloc(&session);
  assert(ret == QDMI_SUCCESS);

  // Empty token = read-only; non-empty token = read/write
  const char *token = "XX12Mayi98"; // read-only
  ret = QDMI_session_set_parameter(session, QDMI_SESSION_PARAMETER_TOKEN,
                                   strlen(token) + 1, token);
  assert(ret == QDMI_SUCCESS);

  // Initialize QDMI session
  ret = QDMI_session_init(session); // device sessions are created here
  MQSS_DEBUG("QDMI_session_init() : " << ret << " session = " << session
                                      << "\n");
  assert(ret == QDMI_SUCCESS);

  // Query the number of devices
  size_t size_ret = 0;
  ret = QDMI_session_query_session_property(
      session, QDMI_SESSION_PROPERTY_DEVICES, 0, nullptr, &size_ret);

  assert(ret == QDMI_SUCCESS);

  size_t num_devices = size_ret / sizeof(QDMI_Device);
  std::vector<QDMI_Device> devices(num_devices);
  ret = QDMI_session_query_session_property(
      session, QDMI_SESSION_PROPERTY_DEVICES, size_ret,
      static_cast<void *>(devices.data()), nullptr);

  MQSS_DEBUG("--> QDMI Num Devices: " << devices.size() << "\n");
  assert(ret == QDMI_SUCCESS);

  // Create a Job
  QDMI_Device dev = devices[0];

  return dev;
}

static DeviceProperty getDeviceProperties(QDMI_Device Device) {

  DeviceProperty Properties;
  Properties.numQubits = getDeviceNumQubits(Device);
  Properties.cm = getDeviceCouplingMap(Device);

  return Properties;
}

// const auto format = QDMI_PROGRAM_FORMAT_QIRBASESTRING;
// QDMI_job_set_parameter(job, QDMI_JOB_PARAMETER_PROGRAMFORMAT,
//                        sizeof(QDMI_Program_Format), &format);
// QDMI_job_set_parameter(job, QDMI_JOB_PARAMETER_PROGRAM,
//                        TEST_CIRCUIT.size() + 1, TEST_CIRCUIT.c_str());
// if (num_shots > 0) {
//   QDMI_job_set_parameter(job, QDMI_JOB_PARAMETER_SHOTSNUM, sizeof(size_t),
//                          &num_shots);
// }
// QDMI_job_submit(job);
// QDMI_job_wait(job, 0);
// // Teardown (in reverse order)
// QDMI_session_free(session);
// QDMI_driver_shutdown(); // dlclose() happens here

// return job;
