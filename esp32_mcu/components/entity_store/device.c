#include "device.h"
// Do not use buffering in RAM, work directly with NVM.
// Data should be written into the member provided in the function call.


// NVM namespace. 15 characters are allowed due to NVM key size limitations.
// used partitin nvs_entity
#ifndef DEVICE_DB_NAMESPACE
#define DEVICE_DB_NAMESPACE   "device_entity"
#endif