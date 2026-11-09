#include "esp_adc/adc_continuous.h" // adc
#include "esp_attr.h"               // IRAM_ATTR
#include "esp_cpu.h"                // wait for interrupt
#include "esp_err.h"                // error check
#include "esp_log.h"                // log
#include "esp_task_wdt.h"           // watchdog timer
#include <stdbool.h>                // bool
#include <stddef.h>                 // provides NULL
#include <stdint.h>                 // uint....

#define POTENTIOMETER ADC_CHANNEL_9

#define SAMPLE_RATE_HZ 20000
#define FRAME_BYTES 256 /* 64 conversions per frame */

static adc_continuous_handle_t adc;

// a frame is a full buffer that the DMA has filled
/* Shared between on_frame (writer) and the main loop (reader). */
static volatile bool frame_ready = false;
static volatile uint32_t frame_average = 0;

static const char *tag = "adc";

/* Runs in interrupt context each time DMA finishes a frame. */
static bool IRAM_ATTR on_frame(adc_continuous_handle_t handle,
                               const adc_continuous_evt_data_t *edata,
                               void *user_data) {
  const adc_digi_output_data_t *samples =
      (const adc_digi_output_data_t *)
          edata->conv_frame_buffer; // samples. ESP fills it with other
                                    // information so it has to be decoded, you
                                    // get about a 1/4th of the numbers
                                    // read struct for info
  uint32_t count = edata->size / sizeof(adc_digi_output_data_t);
  // how count is calculated. the SIZE of the list divided by the SIZE of each
  // item in the list

  uint32_t sum = 0, valid = 0;
  for (uint32_t i = 0; i < count; i++) {
    if (samples[i].type2.channel == POTENTIOMETER) {
      sum += samples[i].type2.data; // taking data out
      valid++;                      // counts the amount of valid samples
    }
  }

  if (valid > 0) {
    frame_average = sum / valid; // averages readings into 1
    frame_ready = true;
  }

  return false; /* no task was woken, so no context switch is needed */
}

static void adc_init(void) {
  adc_continuous_handle_cfg_t handle_cfg = {
      /* The driver requires this pool, but frames are consumed in on_frame. */
      .max_store_buf_size = FRAME_BYTES, // amount of bytes in a frame
      .conv_frame_size =
          FRAME_BYTES, // size of what frames should be converted into
  };
  ESP_ERROR_CHECK(adc_continuous_new_handle(&handle_cfg, &adc));

  adc_digi_pattern_config_t pattern = {
      .atten = ADC_ATTEN_DB_12,
      .channel = POTENTIOMETER,     /* GPIO10 */
      .unit = ADC_UNIT_1,           // ADC unit 1. there are 2 ADC units
      .bit_width = ADC_BITWIDTH_12, // 12 bit ADC
  };
  adc_continuous_config_t config = {
      .pattern_num =
          1, // a pattern is the amount of channels the ADC is reading. in this
             // case its only reading 1 pin but it can round robin multiple
             // round robin = rotate through pins
      .adc_pattern = &pattern,
      .sample_freq_hz =
          SAMPLE_RATE_HZ, // sampling rate. sampling rate is split amongst the
                          // amount of pins that are being read
      .conv_mode =
          ADC_CONV_SINGLE_UNIT_1, // only converts info via ADC1. read constant
  };
  ESP_ERROR_CHECK(adc_continuous_config(adc, &config));

  adc_continuous_evt_cbs_t callbacks = {
      .on_conv_done = on_frame}; // set of callbacks. read struct for what else
                                 // can be done in terms of callbacks
  // callback = function called after something happens
  ESP_ERROR_CHECK(
      adc_continuous_register_event_callbacks(adc, &callbacks, NULL));
  // set up callbacks
}

void app_main(void) {
  /* The loop below never blocks, so the idle task on this core never runs and
   * would trip the task watchdog. */
  ESP_ERROR_CHECK(esp_task_wdt_deinit());
  // remove the watchdog timer

  adc_init();
  ESP_ERROR_CHECK(adc_continuous_start(adc));
  // start adc

  while (1) {
    while (!frame_ready) {
      esp_cpu_wait_for_intr();
      // tell the cpu in the main loop to wait for an interrupt to occur
    }
    frame_ready = false;

    ESP_LOGI(tag, "average raw = %lu", (unsigned long)frame_average);
  }
}
