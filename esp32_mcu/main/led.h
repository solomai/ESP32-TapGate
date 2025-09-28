#include "driver/gpio.h"

#ifndef BLUE_LED_PIN
#define BLUE_LED_PIN GPIO_NUM_2
#endif

void turn_on_blue_led()
{
    gpio_set_level(BLUE_LED_PIN, 1);
}

void turn_off_blue_led()
{
    gpio_set_level(BLUE_LED_PIN, 0);
}

void blue_led_init()
{
    gpio_reset_pin(BLUE_LED_PIN);
    gpio_set_direction(BLUE_LED_PIN, GPIO_MODE_OUTPUT);
    turn_off_blue_led();
}