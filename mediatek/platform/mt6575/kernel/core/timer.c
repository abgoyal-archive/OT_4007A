#include <asm/mach/time.h>

#include "mach/timer.h"

extern struct mt65xx_clock mt6575_gpt;

struct mt65xx_clock *mt6575_clocks[] =
{
    &mt6575_gpt,
};

static void __init mt6575_timer_init(void)
{
    int i;
    struct mt65xx_clock *clock;
    int ret;

    for (i = 0; i < ARRAY_SIZE(mt6575_clocks); i++) {
        clock = mt6575_clocks[i];

        clock->init_func();

        if (clock->clocksource.name) {
            ret = clocksource_register(&(clock->clocksource));
            if (ret) {
                printk(KERN_ERR "mt6575_timer_init: clocksource_register failed for %s\n", clock->clocksource.name);
            }
        }

        ret = setup_irq(clock->irq.irq, &(clock->irq));
        if (ret) {
            printk(KERN_ERR "mt6575_timer_init: setup_irq failed for %s\n", clock->irq.name);
        }

        if (clock->clockevent.name)
            clockevents_register_device(&(clock->clockevent));
    }
}

struct sys_timer mt6575_timer = {
    .init = mt6575_timer_init,
};

