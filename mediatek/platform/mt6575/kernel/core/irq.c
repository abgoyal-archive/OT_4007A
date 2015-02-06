#include <linux/io.h>
#include <linux/module.h>
#include <asm/mach/irq.h>
#include <asm/hardware/gic.h>
#include <linux/smp.h>
#include <linux/interrupt.h>

#include "mach/mt6575_reg_base.h"
#include "mach/irqs.h"
#include "mach/sync_write.h"

static DEFINE_SPINLOCK(irq_controller_lock);

#define GIC_ICDISER0    (GIC_DIST_BASE + 0x100)
#define GIC_ICDISER1    (GIC_DIST_BASE + 0x104)
#define GIC_ICDISER2    (GIC_DIST_BASE + 0x108)
#define GIC_ICDISER3    (GIC_DIST_BASE + 0x10C)
#define GIC_ICDISER4    (GIC_DIST_BASE + 0x110)

#define GIC_ICDICER0    (GIC_DIST_BASE + 0x180)
#define GIC_ICDICER1    (GIC_DIST_BASE + 0x184)
#define GIC_ICDICER2    (GIC_DIST_BASE + 0x188)
#define GIC_ICDICER3    (GIC_DIST_BASE + 0x18C)
#define GIC_ICDICER4    (GIC_DIST_BASE + 0x190)

/*
 * mt6575_irq_mask: mask one IRQ
 * @irq: IRQ line of the IRQ to mask
 */
void mt6575_irq_mask(unsigned int irq)
{
    unsigned int mask = 1 << (irq % 32);

    spin_lock(&irq_controller_lock);
    writel(mask, GIC_DIST_BASE + GIC_DIST_ENABLE_CLEAR + irq / 32 * 4);
    spin_unlock(&irq_controller_lock);
}

EXPORT_SYMBOL(mt6575_irq_mask);

/*
 * mt6575_irq_unmask: unmask one IRQ
 * @irq: IRQ line of the IRQ to unmask
 */
void mt6575_irq_unmask(unsigned int irq)
{
	unsigned int mask = 1 << (irq % 32);

    spin_lock(&irq_controller_lock);
    writel(mask, GIC_DIST_BASE + GIC_DIST_ENABLE_SET + irq / 32 * 4);
    spin_unlock(&irq_controller_lock);
}

EXPORT_SYMBOL(mt6575_irq_unmask);

/*
 * mt6575_irq_ack: acknowledge the IRQ
 * @irq: IRQ line of the IRQ to acknowledge
 */
void mt6575_irq_ack(unsigned int irq)
{
//    unsigned int mask = 1 << (irq % 32);

    spin_lock(&irq_controller_lock);
//    writel(mask, GIC_DIST_BASE + GIC_DIST_ENABLE_CLEAR + irq / 32 * 4);
    writel(irq, GIC_CPU_BASE + GIC_CPU_EOI);
    spin_unlock(&irq_controller_lock);
}

EXPORT_SYMBOL(mt6575_irq_ack);

/*
 * mt6575_irq_set_sens: set the sensitivity value for the given IRQ line
 * @irq: IRQ ID of the IRQ to set sensitivity
 */
void mt6575_irq_set_sens(unsigned int irq, unsigned int sens)
{
    unsigned long flags;
    unsigned int config;

    local_irq_save(flags);
    if (sens == MT65xx_EDGE_SENSITIVE) {
        config = readl(GIC_DIST_BASE + GIC_DIST_CONFIG + (irq / 16) * 4);
        config |= (0x2 << (irq % 16) * 2);
        mt65xx_reg_sync_writel(config, GIC_DIST_BASE + GIC_DIST_CONFIG + (irq / 16) * 4);
    }else {
        config = readl(GIC_DIST_BASE + GIC_DIST_CONFIG + (irq / 16) * 4);
        config &= ~(0x2 << (irq % 16) * 2);
        mt65xx_reg_sync_writel(config, GIC_DIST_BASE + GIC_DIST_CONFIG + (irq / 16) * 4);
    }

    local_irq_restore(flags);
}

EXPORT_SYMBOL(mt6575_irq_set_sens);

/*
 * mt6575_irq_set_polarity: set the sensitivity value for the given IRQ line
 * @irq: IRQ ID of the IRQ to set sensitivity
 * polarity ==>  0: low   1: high
 */
void mt6575_irq_set_polarity(unsigned int irq, unsigned int polarity)
{
    /* NOTE:
     * GIC only responds to HIGH level IRQ, if some peripheral is low-level active, its IRQ must be inverted
     */
    u32 offset;
    u32 reg_index;
    u32 value;

    // peripheral device's IRQ line is using GIC's SPI, and line ID >= GIC_PRIVATE_SIGNALS
    if (irq < GIC_PRIVATE_SIGNALS) {
        printk("The Interrupt ID < 32, please check!");

        return;
    }

    offset = (irq - GIC_PRIVATE_SIGNALS) & 0x1F;
    reg_index = (irq - GIC_PRIVATE_SIGNALS) >> 5;
    if (polarity == 0) {
        value = readl(INT_POL_CTL0 + (reg_index * 4));
        value |= (1 << offset); // always invert the incoming IRQ's polarity
        writel(value, (INT_POL_CTL0 + (reg_index * 4)));
    }else {
        value = readl(INT_POL_CTL0 + (reg_index * 4));
        value &= ~(0x1 << offset);
        writel(value, INT_POL_CTL0 + (reg_index * 4));
    }
}

EXPORT_SYMBOL(mt6575_irq_set_polarity);

/*
 * mt6575_irq_mask_all: mask all IRQ lines. (This is ONLY used for the idle current measurement by the factory mode.)
 * @mask: pointer to struct mtk_irq_mask for storing the original mask value.
 * Return 0 for success; return negative values for failure.
 */
int mt6575_irq_mask_all(struct mtk_irq_mask *mask)
{
    spinlock_t lock;
    unsigned long flags;

    if (mask) {
        spin_lock_irqsave(&lock, flags);

        mask->mask0 = readl(GIC_ICDISER0);
        mask->mask1 = readl(GIC_ICDISER1);
        mask->mask2 = readl(GIC_ICDISER2);
        mask->mask3 = readl(GIC_ICDISER3);
        mask->mask4 = readl(GIC_ICDISER4);

        writel(0xFFFFFFFF, GIC_ICDICER0);
        writel(0xFFFFFFFF, GIC_ICDICER1);
        writel(0xFFFFFFFF, GIC_ICDICER2);
        writel(0xFFFFFFFF, GIC_ICDICER3);
        mt65xx_reg_sync_writel(0xFFFFFFFF, GIC_ICDICER4);

        spin_unlock_irqrestore(&lock, flags);

        mask->header = IRQ_MASK_HEADER;
        mask->footer = IRQ_MASK_FOOTER;

        return 0;
    } else {
        return -1;
    }
}

/*
 * mt6575_irq_mask_restore: restore all IRQ lines' masks. (This is ONLY used for the idle current measurement by the factory mode.)
 * @mask: pointer to struct mtk_irq_mask for storing the original mask value.
 * Return 0 for success; return negative values for failure.
 */
int mt6575_irq_mask_restore(struct mtk_irq_mask *mask)
{
    spinlock_t lock;
    unsigned long flags;

    if (!mask) {
        return -1;
    }
    if (mask->header != IRQ_MASK_HEADER) {
        return -1;
    }
    if (mask->footer != IRQ_MASK_FOOTER) {
        return -1;
    }

    spin_lock_irqsave(&lock, flags);

    writel(mask->mask0, GIC_ICDISER0);
    writel(mask->mask1, GIC_ICDISER1);
    writel(mask->mask2, GIC_ICDISER2);
    writel(mask->mask3, GIC_ICDISER3);
    mt65xx_reg_sync_writel(mask->mask4, GIC_ICDISER4);

    spin_unlock_irqrestore(&lock, flags);

    return 0;
}

/*
 * mt6575_irq_set_type: set IRQ type
 * @irq: IRQ number
 * @flow_type: IRQ type
 * Always return 0.
 */
int mt6575_irq_set_type(unsigned int irq, unsigned int flow_type)
{
    if (flow_type & (IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING)) {
        mt6575_irq_set_sens(irq, MT65xx_EDGE_SENSITIVE);
        mt6575_irq_set_polarity(irq, (flow_type & IRQF_TRIGGER_FALLING) ? 0 : 1);
        irq_desc[irq].handle_irq = handle_edge_irq;
    } else if (flow_type & (IRQF_TRIGGER_HIGH | IRQF_TRIGGER_LOW)) {
        mt6575_irq_set_sens(irq, MT65xx_LEVEL_SENSITIVE);
        mt6575_irq_set_polarity(irq, (flow_type & IRQF_TRIGGER_LOW) ? 0 : 1);
        irq_desc[irq].handle_irq = handle_level_irq;
    }

    return 0;
}

static struct irq_chip mt6575_irq_chip = {
    .disable = mt6575_irq_mask,
    .enable = mt6575_irq_unmask,
    .ack = mt6575_irq_ack,
    .mask = mt6575_irq_mask,
    .unmask = mt6575_irq_unmask,
    .set_type = mt6575_irq_set_type,
};

static void mt6575_gic_dist_init(void)
{
//    unsigned int max_irq, i;
    unsigned int i;
    u32 cpumask = 1 << smp_processor_id();

    cpumask |= cpumask << 8;
    cpumask |= cpumask << 16;

	writel(0, GIC_DIST_BASE + GIC_DIST_CTRL);

#if 0   // Discard using GIC register to count IRQ#
    /*
     * Find out how many interrupts are supported.
     */
    max_irq = readl(GIC_DIST_BASE + GIC_DIST_CTR) & 0x1F;
    max_irq = (max_irq + 1) * 32;

    /*
     * The GIC only supports up to 1020 interrupt sources.
     * Limit this to either the architected maximum, or the
     * platform maximum.
     */
    if (max_irq > max(1020, NR_IRQS)) {
        max_irq = max(1020, NR_IRQS);
    }
#endif

    /*
     * Set all global interrupts to be level triggered, active low.
     */
    for (i = 32; i < (MT6575_NR_SPI + 32); i += 16) {
        writel(0, GIC_DIST_BASE + GIC_DIST_CONFIG + i * 4 / 16);
    }

    /*
     * Set all global interrupts to this CPU only.
     */
    for (i = 32; i < (MT6575_NR_SPI + 32); i += 4) {
        writel(cpumask, GIC_DIST_BASE + GIC_DIST_TARGET + i * 4 / 4);
    }

    /*
     * Set priority on all interrupts.
     */
    for (i = 0; i < NR_MT6575_IRQ_LINE; i += 4) {
        writel(0xA0A0A0A0, GIC_DIST_BASE + GIC_DIST_PRI + i * 4 / 4);
    }

	/*
	 * Disable all interrupts.
	 */
	for (i = 0; i < NR_MT6575_IRQ_LINE; i += 32) {
        writel(0xFFFFFFFF, GIC_DIST_BASE + GIC_DIST_ENABLE_CLEAR + i * 4 / 32); 
    }

    /*
     * Setup the Linux IRQ subsystem.
     */
    for (i = GIC_PPI_OFFSET; i < NR_MT6575_IRQ_LINE; i++) {
        set_irq_chip(i, &mt6575_irq_chip);
        set_irq_handler(i, handle_level_irq);
        set_irq_flags(i, IRQF_VALID | IRQF_PROBE);
    }

    writel(1, GIC_DIST_BASE + GIC_DIST_CTRL);
}

static void mt6575_gic_cpu_init(void)
{
    writel(0xF0, GIC_CPU_BASE + GIC_CPU_PRIMASK);
    writel(1, GIC_CPU_BASE + GIC_CPU_CTRL);
}

void __init mt6575_init_irq(void)
{
    mt6575_gic_dist_init();
    mt6575_gic_cpu_init();
}

int mt6575_irq_is_active(const unsigned int irq)
{
    const unsigned int iActive = readl(GIC_DIST_BASE + 0x200 + irq / 32 * 4);

    return iActive & (1 << (irq % 32)) ? 1 : 0;
}

EXPORT_SYMBOL(mt6575_irq_is_active);
