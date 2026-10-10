#ifndef SRC_CAN_CAN_BUS_PARSER_H_
#define SRC_CAN_CAN_BUS_PARSER_H_

#include <stdint.h>
#include "stm32f4xx_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

void can_parse_msg(CAN_RxHeaderTypeDef *header, uint8_t *data);

#ifdef __cplusplus
}
#endif

#endif /* SRC_CAN_CAN_BUS_PARSER_H_ */
