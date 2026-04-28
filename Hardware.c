#include "Hardware.h"

void Buttons_Init(void)
{
    adc_init();
    adc_gpio_init(SWLADDER);
    adc_select_input(2);
}

void LEDs_Init(void)
{
    gpio_init(LED1);
    gpio_init(LED2);
    gpio_init(LED3);
    gpio_init(LED4);

    gpio_set_dir(LED1, GPIO_OUT);
    gpio_set_dir(LED2, GPIO_OUT);
    gpio_set_dir(LED3, GPIO_OUT);
    gpio_set_dir(LED4, GPIO_OUT);

    // LEDs are active low, so setting them high turns them off by default
    gpio_put(LED1, 1);
    gpio_put(LED2, 1);
    gpio_put(LED3, 1);
    gpio_put(LED4, 1);
}