#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_rom_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stddef.h>
#include <stdio.h>

#define HIGH 1
#define LOW 0

volatile uint32_t led_state = LOW;

static const char *tag = "main";

void on_button_switch(void *arg);

void on_button_switch(void *arg) {
  (void)arg;

  if (led_state == LOW) {
    led_state = HIGH;
  } else {
    led_state = LOW;
  }

  gpio_set_level(GPIO_NUM_15, led_state);
}

static void init(gpio_config_t *button_conf, gpio_config_t *led_conf) {
  ESP_ERROR_CHECK(gpio_config(button_conf));

  ESP_ERROR_CHECK(gpio_config(led_conf));

  ESP_ERROR_CHECK(gpio_install_isr_service(ESP_INTR_FLAG_LEVEL1));

  ESP_ERROR_CHECK(gpio_isr_handler_add(GPIO_NUM_7, on_button_switch, NULL));
}

void app_main(void) {
  gpio_config_t button_conf = {
      .pin_bit_mask = (1ULL << GPIO_NUM_7),
      .mode = GPIO_MODE_INPUT,
      .pull_up_en = GPIO_PULLUP_DISABLE,
      .pull_down_en = GPIO_PULLDOWN_DISABLE,
      .intr_type = GPIO_INTR_ANYEDGE,
  };

  gpio_config_t led_conf = {.pin_bit_mask = (1ULL << GPIO_NUM_15),
                            .mode = GPIO_MODE_OUTPUT,
                            .pull_up_en = GPIO_PULLUP_DISABLE,
                            .pull_down_en = GPIO_PULLDOWN_DISABLE,
                            .intr_type = GPIO_INTR_DISABLE};

  init(&button_conf, &led_conf);

  while (1) {
    ESP_LOGI(tag, "hello!");
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}
