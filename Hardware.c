#include "Hardware.h"

void Buttons_Init(void)
{
    // same as setting them using: gpio_set_function(SW1, GPIO_FUNC_SIO) (SIO == Simple Input/Output)
    gpio_init(SW1);
    gpio_init(SW2);
    gpio_init(SW3);
    gpio_init(SW4);

    gpio_set_dir(SW1, GPIO_IN);
    gpio_set_dir(SW2, GPIO_IN);
    gpio_set_dir(SW3, GPIO_IN);
    gpio_set_dir(SW4, GPIO_IN);

    // pull-up since buttons are active low
    gpio_pull_up(SW1);
    gpio_pull_up(SW2);
    gpio_pull_up(SW3);
    gpio_pull_up(SW4);
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

    gpio_put(LED1, 1);
    gpio_put(LED2, 1);
    gpio_put(LED3, 1);
    gpio_put(LED4, 1);
}