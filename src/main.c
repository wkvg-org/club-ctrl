/*
 * Copyright (C) 2025 - 2026 Wolfenbütteler Kultur- und Veranstaltungsgesellschaft gemeinnützige UG (haftungsbeschränkt)
 *
 * SPDX-License-Identifier: GPL-2.0-only
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
#include <zephyr/drivers/buzzer.h>
#include <zephyr/sys/util.h>

#include "modules/output/dmx/dmx.h"
#include "modules/output/dmx/fixture.h"
#include "modules/output/sound/sound.h"
#include "modules/show/show.h"
#include "modules/net/udp_server.h"

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

#define BUZZER_NODE DT_ALIAS(buzzer0)
static const struct device *const buzzer_dev = DEVICE_DT_GET(BUZZER_NODE);

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
		buzzer_beep(buzzer_dev, 100);
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

#define W5500_NODE DT_ALIAS(eth_w5500)
static const struct device *w5500_device = DEVICE_DT_GET(W5500_NODE);

// RTC
#define RTC_NODE DT_ALIAS(rtc)
static const struct device *rtc_device = DEVICE_DT_GET(RTC_NODE);

// TCA
#define TCA_NODE DT_ALIAS(tca)
static const struct device *tca_device = DEVICE_DT_GET(TCA_NODE);

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

	if (!device_is_ready(buzzer_dev))
	{
		LOG_ERR("buzzer not ready");
		return 0;
	}

	ret = dmx_init();
	if (ret != 0) {
		LOG_ERR("dmx_init failed: %d", ret);
	}

	ret = sound_init();
	if (ret != 0) {
		LOG_ERR("sound_init failed: %d", ret);
	} else {
		sound_play(startup_chime, startup_chime_len);
	}

	show_init();

	udp_server_init();

	if (device_is_ready(w5500_device))
	{
		LOG_INF("Ethernet ready");
	}
	else
	{
		LOG_INF("Ethernet not ready");
		return 0;
	}

	if (device_is_ready(rtc_device))
	{
		LOG_INF("rtc ready");
	}
	else
	{
		LOG_INF("rtc not ready");
		return 0;
	}

	if (device_is_ready(tca_device))
	{
		LOG_INF("tca ready");
	}
	else
	{
		LOG_INF("tca not ready");
		return 0;
	}

	return 0;
}
