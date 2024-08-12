#ifndef __LORAMAC_PARSER_H__
#define __LORAMAC_PARSER_H__
#include "loramac_message.h"
#include "loramac.h"
typedef int loramac_parser_error_t;

#define LORAMAC_OK 0
#define LORAMAC_ERR -1

loramac_parser_error_t loramac_parse_join_accept(loramac_message_join_accept_t *msg);
loramac_err_t loramac_parse_frame(loramac_frame_t *frame);

#endif