/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>
#include <stdio.h>

#define LOG_LEVEL 4
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main);

#include <zephyr/kernel.h>
#include <zephyr/drivers/led_strip.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/sys/util.h>

#define STRIP_NODE DT_ALIAS(led_strip)

#if DT_NODE_HAS_PROP(DT_ALIAS(led_strip), chain_length)
#define STRIP_NUM_PIXELS DT_PROP(DT_ALIAS(led_strip), chain_length)
#else
#error Unable to determine length of LED strip
#endif

#define DELAY_TIME K_MSEC(500)

#define RGB(_r, _g, _b) {.r = (_r), .g = (_g), .b = (_b)}

static const struct led_rgb colors[] = {
	RGB(0xff, 0x00, 0x00), /* red */
	RGB(0x00, 0xff, 0x00), /* green */
	RGB(0x00, 0x00, 0xff), /* blue */
};
static struct led_rgb pixels[STRIP_NUM_PIXELS];

static const struct device *const strip = DEVICE_DT_GET(STRIP_NODE);

/* The devicetree node identifier for the "buzzer" alias. */
#define BUZZER_NODE DT_ALIAS(buzzer)

/*
 * A build error on this line means your board is unsupported.
 * See the sample documentation for information on how to fix this.
 */
static const struct gpio_dt_spec buzzer = GPIO_DT_SPEC_GET(BUZZER_NODE, gpios);

#define USB_SERIAL_NODE DT_ALIAS(usb_serial)
static const struct device *esp_usb_serial_device = DEVICE_DT_GET(USB_SERIAL_NODE);

/*
 * Get button configuration from the devicetree boot-btn alias. This is mandatory.
 */
#define SW0_NODE DT_ALIAS(boot_btn)
#if !DT_NODE_HAS_STATUS_OKAY(SW0_NODE)
#error "Unsupported board: boot-btn devicetree alias is not defined"
#endif
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET_OR(SW0_NODE, gpios, {0});
static struct gpio_callback button_cb_data;
void button_pressed(const struct device *dev, struct gpio_callback *cb,
					uint32_t pins)
{

	size_t color = 0;
	int rc;
	printk("Button pressed at %" PRIu32 "\n", k_cycle_get_32());
	/* If we have an LED, match its state to the button's. */
	int val = gpio_pin_get_dt(&button);

	if (val > 0)
	{
		printk("Button pressed");
		gpio_pin_toggle_dt(&buzzer);
		for (size_t cursor = 0; cursor < ARRAY_SIZE(pixels); cursor++)
		{
			memset(&pixels, 0x00, sizeof(pixels));
			memcpy(&pixels[cursor], &colors[color], sizeof(struct led_rgb));

			rc = led_strip_update_rgb(strip, pixels, STRIP_NUM_PIXELS);
			if (rc)
			{
				LOG_ERR("couldn't update strip: %d", rc);
			}

			k_sleep(DELAY_TIME);
		}

		color = (color + 1) % ARRAY_SIZE(colors);
	}
}
/* Ethernet */
#define W5500_NODE DT_ALIAS(eth_w5500)
static const struct device *w5500_device = DEVICE_DT_GET(W5500_NODE);

/*  SD Card  */
#include <zephyr/storage/disk_access.h>
#include <zephyr/fs/fs.h>
#include <ff.h>

static int lsdir(const char *path);

static FATFS fat_fs;
/* mounting info */
static struct fs_mount_t mp = {
	.type = FS_FATFS,
	.fs_data = &fat_fs,
};

/*
 *  Note the fatfs library is able to mount only strings inside _VOLUME_STRS
 *  in ffconf.h
 */
static const char *disk_mount_pt = "/SD:";

static int lsdir(const char *path)
{
	int res;
	struct fs_dir_t dirp;
	static struct fs_dirent entry;

	fs_dir_t_init(&dirp);

	/* Verify fs_opendir() */
	res = fs_opendir(&dirp, path);
	if (res)
	{
		printk("Error opening dir %s [%d]\n", path, res);
		return res;
	}

	printk("\nListing dir %s ...\n", path);
	for (;;)
	{
		/* Verify fs_readdir() */
		res = fs_readdir(&dirp, &entry);

		/* entry.name[0] == 0 means end-of-dir */
		if (res || entry.name[0] == 0)
		{
			break;
		}

		if (entry.type == FS_DIR_ENTRY_DIR)
		{
			printk("[DIR ] %s\n", entry.name);
		}
		else
		{
			printk("[FILE] %s (size = %zu)\n",
				   entry.name, entry.size);
		}
	}

	/* Verify fs_closedir() */
	fs_closedir(&dirp);

	return res;
}

static int mount_sd_card(void)
{
	/* raw disk i/o */
	static const char *disk_pdrv = "SD";
	uint64_t memory_size_mb;
	uint32_t block_count;
	uint32_t block_size;

	if (disk_access_init(disk_pdrv) != 0)
	{
		LOG_ERR("Storage init ERROR!");
		return -1;
	}

	if (disk_access_ioctl(disk_pdrv,
						  DISK_IOCTL_GET_SECTOR_COUNT, &block_count))
	{
		LOG_ERR("Unable to get sector count");
		return -1;
	}
	LOG_INF("Block count %u", block_count);

	if (disk_access_ioctl(disk_pdrv,
						  DISK_IOCTL_GET_SECTOR_SIZE, &block_size))
	{
		LOG_ERR("Unable to get sector size");
		return -1;
	}
	printk("Sector size %u\n", block_size);

	memory_size_mb = (uint64_t)block_count * block_size;
	printk("Memory Size(MB) %u\n", (uint32_t)(memory_size_mb >> 20));

	mp.mnt_point = disk_mount_pt;

	int res = fs_mount(&mp);

	if (res == FR_OK)
	{
		printk("Disk mounted.\n");
		lsdir(disk_mount_pt);
	}
	else
	{
		printk("Failed to mount disk - trying one more time\n");
		res = fs_mount(&mp);
		if (res != FR_OK)
		{
			printk("Error mounting disk.\n");
			return -1;
		}
	}

	return 0;
}

int main(void)
{
	if (device_is_ready(esp_usb_serial_device))
	{
		LOG_INF("ESP USB Serial ready");
	}
	else
	{
		LOG_INF("ESP USB Serial not ready");
		return 0;
	}
	printk("hello world");

	int ret;

	if (!gpio_is_ready_dt(&button))
	{
		printk("Error: button device %s is not ready\n", button.port->name);
		return 0;
	}

	ret = gpio_pin_configure_dt(&button, GPIO_INPUT);
	if (ret != 0)
	{
		printk("Error %d: failed to configure %s pin %d\n", ret, button.port->name, button.pin);
		return 0;
	}

	ret = gpio_pin_interrupt_configure_dt(&button, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret != 0)
	{
		printk("Error %d: failed to configure interrupt on %s pin %d\n", ret, button.port->name, button.pin);
		return 0;
	}

	gpio_init_callback(&button_cb_data, button_pressed, BIT(button.pin));
	gpio_add_callback(button.port, &button_cb_data);
	printk("Set up button at %s pin %d\n", button.port->name, button.pin);

	if (device_is_ready(strip))
	{
		LOG_INF("Found LED strip device %s", strip->name);
	}
	else
	{
		LOG_ERR("LED strip device %s is not ready", strip->name);
		return 0;
	}

	if (!gpio_is_ready_dt(&buzzer))
	{
		return 0;
	}

	ret = gpio_pin_configure_dt(&buzzer, GPIO_OUTPUT_ACTIVE);
	if (ret < 0)
	{
		return 0;
	}

	if (device_is_ready(w5500_device))
	{
		LOG_INF("Ethernet ready");
	}
	else
	{
		LOG_INF("Ethernet not ready");
		return 0;
	}

	/** SD card mount test **/
	if (mount_sd_card())
	{
		printk("Failed to mount SD card\n");
		return -1;
	}
	else
	{
		printk("Successfully mounted SD card\n");
	}

	char file_data_buffer[200];
	struct fs_file_t data_filp;
	fs_file_t_init(&data_filp);

	ret = fs_unlink("/SD:/test_data.txt");

	ret = fs_open(&data_filp, "/SD:/test_data.txt", FS_O_WRITE | FS_O_CREATE);
	if (ret)
	{
		printk("%s -- failed to create file (err = %d)\n", __func__, ret);
		return -2;
	}
	else
	{
		printk("%s - successfully created file\n", __func__);
	}

	sprintf(file_data_buffer, "hello world!\n");
	ret = fs_write(&data_filp, file_data_buffer, strlen(file_data_buffer));
	fs_close(&data_filp);

	while (1)
	{
		k_sleep(K_MSEC(1));
	}

	return 0;
}
