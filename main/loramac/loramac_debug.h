#ifndef __LORAMAC_DEBUG_H__
#define __LORAMAC_DEBUG_H__

#include "loramac_message.h"

void loramac_debug_dump_join_request(loramac_message_join_request_t *msg);
void loramac_debug_dump_join_accept(loramac_message_join_accept_t *msg);

#endif