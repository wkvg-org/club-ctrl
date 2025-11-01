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
#include <zephyr/drivers/i2c.h>
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

// Relays
#define I2C_NODE DT_ALIAS(i2c0)
static const struct device *i2c_device = DEVICE_DT_GET(I2C_NODE);

/****************************************************** The macro defines the TCA9554PWR information ******************************************************/

#define TCA9554_ADDRESS 0x20	  // TCA9554PWR I2C address
#define TCA9554_INPUT_REG 0x00	  // Input register,input level
#define TCA9554_OUTPUT_REG 0x01	  // Output register, high and low level output
#define TCA9554_Polarity_REG 0x02 // The Polarity Inversion register (register 2) allows polarity inversion of pins defined as inputs by the Configuration register.
#define TCA9554_CONFIG_REG 0x03	  // Configuration register, mode configuration

#define Low 0
#define High 1
#define EXIO_PIN1 1
#define EXIO_PIN2 2
#define EXIO_PIN3 3
#define EXIO_PIN4 4
#define EXIO_PIN5 5
#define EXIO_PIN6 6
#define EXIO_PIN7 7
#define EXIO_PIN8 8

/*****************************************************  Operation register REG   ****************************************************/
uint8_t Read_REG(uint8_t REG);				  // Read the value of the TCA9554PWR register REG
uint8_t Write_REG(uint8_t REG, uint8_t Data); // Write Data to the REG register of the TCA9554PWR
/********************************************************** Set EXIO mode **********************************************************/
void Mode_EXIO(uint8_t Pin, uint8_t State); // Set the mode of the TCA9554PWR Pin. The default is Output mode (output mode or input mode). State: 0= Output mode 1= input mode
void Mode_EXIOS(uint8_t PinState);			// Set the mode of the 7 pins from the TCA9554PWR with PinState
/********************************************************** Read EXIO status **********************************************************/
uint8_t Read_EXIO(uint8_t Pin);	 // Read the level of the TCA9554PWR Pin
uint8_t Read_EXIOS(uint8_t REG); // Read the level of all pins of TCA9554PWR, the default read input level state, want to get the current IO output state, pass the parameter TCA9554_OUTPUT_REG, such as Read_EXIOS(TCA9554_OUTPUT_REG);
/********************************************************** Set the EXIO output status **********************************************************/
bool Set_EXIO(uint8_t Pin, uint8_t State); // Sets the level state of the Pin without affecting the other pins
bool Set_EXIOS(uint8_t PinState);		   // Set 7 pins to the PinState state such as :PinState=0x23, 0010 0011 state (the highest bit is not used)
/********************************************************** Flip EXIO state **********************************************************/
bool Set_Toggle(uint8_t Pin); // Flip the level of the TCA9554PWR Pin
/********************************************************* TCA9554PWR Initializes the device ***********************************************************/
void TCA9554PWR_Init(uint8_t PinMode, uint8_t PinState); // Set the seven pins to PinState state, for example :PinState=0x23, 0010 0011 State (the highest bit is not used) (Output mode or input mode) 0= Output mode 1= Input mode. The default value is output mode

/*****************************************************  Operation register REG   ****************************************************/
uint8_t Read_REG(uint8_t REG) // Read the value of the TCA9554PWR register REG
{
	uint8_t result = i2c_write(i2c_device, &REG, 1, TCA9554_ADDRESS);
	if (result != 0)
	{
		printf("Data Transfer Failure !!!\r\n");
	}
	uint8_t bitsStatus;
	i2c_read(i2c_device, &bitsStatus, 1, TCA9554_ADDRESS);
	return bitsStatus;
}
uint8_t Write_REG(uint8_t REG, uint8_t Data) // Write Data to the REG register of the TCA9554PWR
{
	uint8_t buf[2] = {REG, Data};
	uint8_t result = i2c_write(i2c_device, buf, 2, TCA9554_ADDRESS);
	if (result != 0)
	{
		printf("Data write failure!!!\r\n");
		return -1;
	}
	return 0;
}
/********************************************************** Set EXIO mode **********************************************************/
void Mode_EXIO(uint8_t Pin, uint8_t State) // Set the mode of the TCA9554PWR Pin. The default is Output mode (output mode or input mode). State: 0= Output mode 1= input mode
{
	uint8_t bitsStatus = Read_REG(TCA9554_CONFIG_REG);
	uint8_t Data = (0x01 << (Pin - 1)) | bitsStatus;
	uint8_t result = Write_REG(TCA9554_CONFIG_REG, Data);
	if (result != 0)
	{
		printf("I/O Configuration Failure !!!\r\n");
	}
}
void Mode_EXIOS(uint8_t PinState) // Set the mode of the 7 pins from the TCA9554PWR with PinState
{
	uint8_t result = Write_REG(TCA9554_CONFIG_REG, PinState);
	if (result != 0)
	{
		printf("I/O Configuration Failure !!!\r\n");
	}
}
/********************************************************** Read EXIO status **********************************************************/
uint8_t Read_EXIO(uint8_t Pin) // Read the level of the TCA9554PWR Pin
{
	uint8_t inputBits = Read_REG(TCA9554_INPUT_REG);
	uint8_t bitStatus = (inputBits >> (Pin - 1)) & 0x01;
	return bitStatus;
}
uint8_t Read_EXIOS(uint8_t REG) // Read the level of all pins of TCA9554PWR, the default read input level state, want to get the current IO output state, pass the parameter TCA9554_OUTPUT_REG, such as Read_EXIOS(TCA9554_OUTPUT_REG);
{
	uint8_t inputBits = Read_REG(REG);
	return inputBits;
}

/********************************************************** Set the EXIO output status **********************************************************/
bool Set_EXIO(uint8_t Pin, uint8_t State) // Sets the level state of the Pin without affecting the other pins
{
	uint8_t Data;
	if (State < 2 && Pin < 9 && Pin > 0)
	{
		uint8_t bitsStatus = Read_EXIOS(TCA9554_OUTPUT_REG);
		if (State == 1)
			Data = (0x01 << (Pin - 1)) | bitsStatus;
		else if (State == 0)
			Data = (~(0x01 << (Pin - 1))) & bitsStatus;
		uint8_t result = Write_REG(TCA9554_OUTPUT_REG, Data);
		if (result != 0)
		{
			printf("Failed to set GPIO!!!\r\n");
			return 0;
		}
		return 1;
	}
	else
	{
		printf("Parameter error, please enter the correct parameter!\r\n");
		return 0;
	}
}
bool Set_EXIOS(uint8_t PinState) // Set 7 pins to the PinState state such as :PinState=0x23, 0010 0011 state (the highest bit is not used)
{
	uint8_t result = Write_REG(TCA9554_OUTPUT_REG, PinState);
	if (result != 0)
	{
		printf("Failed to set GPIO!!!\r\n");
		return 0;
	}
	return 1;
}
/********************************************************** Flip EXIO state **********************************************************/
bool Set_Toggle(uint8_t Pin) // Flip the level of the TCA9554PWR Pin
{
	uint8_t bitsStatus = Read_EXIO(Pin);
	uint8_t result = Set_EXIO(Pin, (bool)!bitsStatus);
	if (!result)
	{
		printf("Failed to Toggle GPIO!!!\r\n");
		return 0;
	}
	return 1;
}
/********************************************************* TCA9554PWR Initializes the device ***********************************************************/
void TCA9554PWR_Init(uint8_t PinMode, uint8_t PinState) // Set the seven pins to PinState state, for example :PinState=0x23, 0010 0011 State  (Output mode or input mode) 0= Output mode 1= Input mode. The default value is output mode
{
	Set_EXIOS(PinState);
	Mode_EXIOS(PinMode);
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

	if (device_is_ready(i2c_device))
	{
		LOG_INF("i2c ready");
	}
	else
	{
		LOG_INF("i2c not ready");
		return 0;
	}

	TCA9554PWR_Init(0x00, 0x00);
	/*
		while (1)
		{
			Set_EXIOS(0b01010101);
			k_msleep(1000);
			Set_EXIOS(0b10101010);
			k_msleep(1000);
		}
	*/
	return 0;
}