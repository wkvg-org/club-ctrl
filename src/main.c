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

#define W5500_NODE DT_ALIAS(eth_w5500)
static const struct device *w5500_device = DEVICE_DT_GET(W5500_NODE);

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

	return 0;
}
