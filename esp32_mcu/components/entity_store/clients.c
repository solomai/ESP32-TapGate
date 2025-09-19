#include "clients.h"
// Do not use buffering in RAM, work directly with NVM.
// Data should be written into the member provided in the function call.


// NVM namespace. 15 characters are allowed due to NVM key size limitations.
// used partitin nvs_entity
#ifndef CLIENTS_DB_NAMESPACE
#define CLIENTS_DB_NAMESPACE   "clients_entity"
#endif

// NVM namespace. 15 characters are allowed due to NVM key size limitations.
// used partitin nvs_nonce
#ifndef CLIENTS_NONCE_DB_NAMESPACE
#define CLIENTS_NONCE_DB_NAMESPACE   "clients_nonce"
#endif
