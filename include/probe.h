#ifndef INCLUDE_PROBE_H
#define INCLUDE_PROBE_H
#include "defs.h"

#include "stm32f4xx_hal.h"

extern ADC_HandleTypeDef hadc1;
extern DMA_HandleTypeDef hdma_adc1;

RC probe_init(void);
RC probe_start(u16 *buf, usize sz);
RC probe_fetch(u16 **buf, usize *sz);

void HAL_ADC_MspInit(ADC_HandleTypeDef *hadc);

#endif // INCLUDE_PROBE_H
