#include <mach/hardware.h>
#include <asm/hardware/gic.h>

#include "mt6575_reg_base.h"
#include "irqs.h"

/* leave disable_fiq as an empty macro */
.macro disable_fiq
.endm

.macro  get_irqnr_preamble, base, tmp
    ldr \base, =GIC_CPU_BASE
.endm

.macro  arch_ret_to_user, tmp1, tmp2
.endm

.macro get_irqnr_and_base, irqnr, irqstat, base, tmp
    ldr \irqstat, [\base, #GIC_CPU_INTACK] /* bits 12-10 = src CPU, 9-0 = int # */
    ldr \tmp, =NR_IRQS
    bic \irqnr, \irqstat, #0x1c00
    cmp \irqnr, #29
    /* if (irqnr < 29) return NO_IRQ; */
    cmpcc \irqnr, \irqnr
    cmpne \irqnr, \tmp
    /* if (irqnr >= NR_IRQS) return NO_IRQ */
    cmpcs \irqnr, \irqnr

    .endm

