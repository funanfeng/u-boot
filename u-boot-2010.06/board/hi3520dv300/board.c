/*
 * (C) Copyright 2002
 * Sysgo Real-Time Solutions, GmbH <www.elinos.com>
 * Marius Groeger <mgroeger@sysgo.de>
 *
 * (C) Copyright 2002
 * David Mueller, ELSOFT AG, <d.mueller@elsoft.ch>
 *
 * (C) Copyright 2003
 * Texas Instruments, <www.ti.com>
 * Kshitij Gupta <Kshitij@ti.com>
 *
 * (C) Copyright 2004
 * ARM Ltd.
 * Philippe Robin, <philippe.robin@arm.com>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <config.h>
#include <common.h>
#include <asm/io.h>
#include <asm/sizes.h>
#include <asm/arch/platform.h>

static int boot_media = BOOT_MEDIA_UNKNOW;

#if defined(CONFIG_SHOW_BOOT_PROGRESS)
void show_boot_progress(int progress)
{
	printf("Boot reached stage %d\n", progress);
}
#endif

#define COMP_MODE_ENABLE ((unsigned int)0x0000EAEF)

static inline void delay(unsigned long loops)
{
	__asm__ volatile ("1:\n"
		"subs %0, %1, #1\n"
		"bne 1b" : "=r" (loops) : "0" (loops));
}
/* get uboot start media. */
int get_boot_media(void)
{
	return boot_media;
}

void mdelay(unsigned long msec)
{
	while(msec > 0) {
		msec --;
		udelay(1000);
	}
}

#define IO_CONF_BASE     (0x120f0000)
#define IO_CONF_REG(U)   (IO_CONF_BASE+((U)*0x0004))
#define GPIO0_BASE       (0x12150000)
#define GPIO_BASE(X)     (GPIO0_BASE + (X)*0x10000)
#define BUZZER_GPIO_ADDR GPIO_BASE(0)
#define BUZZER_PIN       (3)
void buzzer_notify(unsigned long msecs, unsigned long times)
{
	unsigned long tmpRD = 0;

	tmpRD  = __raw_readl(BUZZER_GPIO_ADDR+0x400); //set buzzer_pin to ouput
	tmpRD |= (1<<BUZZER_PIN); //set pin register
	__raw_writel(tmpRD, BUZZER_GPIO_ADDR+0x400);

	while(times --) {
		__raw_writel(0xFF, BUZZER_GPIO_ADDR + ((1<<BUZZER_PIN)<<2));//buzzer pin out enable
		mdelay(msecs);
		__raw_writel(0x00, BUZZER_GPIO_ADDR + ((1<<BUZZER_PIN)<<2));//buzzer pin out disable
		mdelay(msecs*1.5);
	}

	tmpRD  = __raw_readl(BUZZER_GPIO_ADDR+0x400); //set buzzer_pin to ouput
	tmpRD &= ~(1<<BUZZER_PIN); //clear pin register
	__raw_writel(tmpRD, BUZZER_GPIO_ADDR+0x400);
}

void boot_flag_init(void)
{
	unsigned int regval, device_type;

	/* get boot device type */
	regval = __raw_readl(SYS_CTRL_REG_BASE + REG_SYSSTAT);
	device_type = (regval >> 8) & 0x1;

	switch (device_type) {
	/* spi nor device */
	case 0:
		boot_media = BOOT_MEDIA_SPIFLASH;
		break;
	/* spi nand device */
	case 1:
		boot_media = BOOT_MEDIA_NAND;
		break;
	default:
		boot_media = BOOT_MEDIA_SPIFLASH;
		break;
	}
}

/*
 * Miscellaneous platform dependent initialisations
 */
int board_init(void)
{
	unsigned long reg;
	/* set uart clk from XTAL OSC 24M */
	reg = readl(CRG_REG_BASE + PERI_CRG33);
	reg &= ~UART_CKSEL_MASK;
	reg |= UART_CKSEL_24M;
	writel(reg, CRG_REG_BASE + PERI_CRG33);

	DECLARE_GLOBAL_DATA_PTR;

	gd->bd->bi_arch_number = MACH_TYPE_HI3521A;
	gd->bd->bi_boot_params = CFG_BOOT_PARAMS;
	gd->flags = 0;

	buzzer_notify(55, 1);

	boot_flag_init();

	return 0;
}

int misc_init_r(void)
{
#ifdef CONFIG_RANDOM_ETHADDR
	random_init_r();
#endif
	setenv("verify", "n");

#ifdef CONFIG_AUTO_UPDATE
	extern int do_auto_update(void);
#ifdef CFG_MMU_HANDLEOK
	dcache_stop();
#endif
	do_auto_update();
#ifdef CFG_MMU_HANDLEOK
	dcache_start();
#endif
#endif

	return 0;

}

int dram_init(void)
{
	DECLARE_GLOBAL_DATA_PTR;
	gd->bd->bi_dram[0].start = CFG_DDR_PHYS_OFFSET;
	gd->bd->bi_dram[0].size = CFG_DDR_SIZE;

	return 0;
}

