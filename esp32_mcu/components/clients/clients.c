#include "clients.h"

static bool db_opened = false;

clients_status_t open_client_db(void)
{
    if (db_opened)
        return CLIENTS_OK;

    db_opened = true;
    return CLIENTS_ERR_NO_INIT;
}

void close_client_db(void)
{
    if (false == db_opened)
        return;

    db_opened = false;
}