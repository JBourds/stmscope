#include "probe.h"
#include "stm32f4xx_hal.h"

ADC_HandleTypeDef hadc1;
DMA_HandleTypeDef hdma_adc1;

static RC adc_init(void);
static RC dma_init(void);

static volatile struct {
    _Bool active_buf : 1;
    _Bool dma_complete : 1;
    volatile u16 *buf;
    usize sz_per_half;
} STATE = {
    .active_buf = 0,
    .dma_complete = 0,
    .buf = NULL,
    .sz_per_half = 0,
};

RC probe_fetch(u16 **buf, usize *sz) {
    if (STATE.buf == NULL) {
        return RC_NOT_INIT;
    }
    *buf = (u16 *)STATE.buf + STATE.sz_per_half;
    return RC_OK;
}

RC probe_init(void) {
    if (dma_init() != RC_OK) {
        return RC_INIT_FAILED;
    }
    if (adc_init() != RC_OK) {
        return RC_INIT_FAILED;
    }
    return RC_OK;
}

RC probe_start(u16 *buf, usize sz) {
    STATE.buf = buf;
    // double buffered but uses a flat buffer, `sz` is size per buffer
    STATE.sz_per_half = sz >> 1;
    if (HAL_ADC_Start_DMA(&hadc1, (u32 *)buf, sz) != HAL_OK) {
        return RC_PROBE_START;
    }
    return RC_OK;
}

static RC dma_init(void) {
    /* DMA controller clock enable */
    __HAL_RCC_DMA2_CLK_ENABLE();

    /* Configure DMA for ADC1 */
    hdma_adc1.Instance = DMA2_Stream0;
    hdma_adc1.Init.Channel = DMA_CHANNEL_0;
    hdma_adc1.Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma_adc1.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_adc1.Init.MemInc = DMA_MINC_ENABLE;
    hdma_adc1.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
    hdma_adc1.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
    hdma_adc1.Init.Mode = DMA_CIRCULAR;
    hdma_adc1.Init.Priority = DMA_PRIORITY_LOW;
    hdma_adc1.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    HAL_DMA_Init(&hdma_adc1);

    /* Associate the DMA handle with the ADC handle */
    __HAL_LINKDMA(&hadc1, DMA_Handle, hdma_adc1);

    /* DMA interrupt init */
    /* DMA2_Stream0_IRQn interrupt configuration */
    HAL_NVIC_SetPriority(DMA2_Stream0_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(DMA2_Stream0_IRQn);

    return RC_OK;
}

static RC adc_init() {
    ADC_ChannelConfTypeDef sConfig = {0};

    hadc1.Instance = ADC1;
    hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
    hadc1.Init.Resolution = ADC_RESOLUTION_12B;
    hadc1.Init.ScanConvMode = DISABLE;
    hadc1.Init.ContinuousConvMode = ENABLE;
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
    hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc1.Init.NbrOfConversion = 1;
    hadc1.Init.DMAContinuousRequests = ENABLE;
    hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
    if (HAL_ADC_Init(&hadc1) != HAL_OK) {
        return RC_INIT_FAILED;
    }

    /** Configure for the selected ADC regular channel its corresponding rank in
     * the sequencer and its sample time. */
    sConfig.Channel = ADC_CHANNEL_0;
    sConfig.Rank = 1;
    sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;
    if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) {
        return RC_INIT_FAILED;
    }

    return RC_OK;
}
