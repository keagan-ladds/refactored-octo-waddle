#ifndef __LORAMAC_SERIALIZER_H__
#define __LORAMAC_SERIALIZER_H__
#include "loramac_message.h"

typedef int loramac_serializer_result_t;

#define LORAMAC_SERIALIZER_ERROR_NPE 1
#define LORAMAC_SERIALIZER_ERROR_BUF_SIZE 2

loramac_serializer_result_t loramac_serialize_join_request(loramac_message_join_request_t *msg);

#endif 
