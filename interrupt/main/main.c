#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_rom_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stddef.h>
#include <stdio.h>
#include "esp_timer.h"

#define HIGH 1
#define LOW 0

volatile uint32_t motor_state = LOW;
static const char *tag = "main";
int64_t last_valid_time = 0;

void on_button_switch(void *arg);

void on_button_switch(void *arg) 
{
  (void)arg;
  int64_t time = esp_timer_get_time();

  if (time - last_valid_time > 50000)
  {
    if (gpio_get_level(GPIO_NUM_6) == LOW)
    {
      if (motor_state == LOW) 
      {
        motor_state = HIGH;
      } 
      else 
      {
        motor_state = LOW;
      } 
      last_valid_time = esp_timer_get_time();
    }
  }


  gpio_set_level(GPIO_NUM_3, motor_state);
}

static void init(gpio_config_t *button_conf, gpio_config_t *motor_conf) {
  ESP_ERROR_CHECK(gpio_config(button_conf));

  ESP_ERROR_CHECK(gpio_config(motor_conf));

  ESP_ERROR_CHECK(gpio_install_isr_service(ESP_INTR_FLAG_LEVEL1));

  ESP_ERROR_CHECK(gpio_isr_handler_add(GPIO_NUM_6, on_button_switch, NULL));
}

void app_main(void) {
  gpio_config_t button_conf = {
      .pin_bit_mask = (1ULL << GPIO_NUM_6),
      .mode = GPIO_MODE_INPUT,
      .pull_up_en = GPIO_PULLUP_DISABLE,
      .pull_down_en = GPIO_PULLDOWN_DISABLE,
      .intr_type = GPIO_INTR_ANYEDGE,
  };

  gpio_config_t motor_conf = {.pin_bit_mask = (1ULL << GPIO_NUM_3),
                            .mode = GPIO_MODE_OUTPUT,
                            .pull_up_en = GPIO_PULLUP_DISABLE,
                            .pull_down_en = GPIO_PULLDOWN_DISABLE,
                            .intr_type = GPIO_INTR_DISABLE};

  init(&button_conf, &motor_conf);

  while (1) {
    ESP_LOGI(tag, "hello!");
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}