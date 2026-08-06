/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * DMX512 transmitter on UART1 (RS485 half-duplex, auto-DE transceiver).
 *
 * Each frame is: start code + 512 slots, then a hardware BREAK appended
 * by the ESP32 UART (conf0.txd_brk) after the last slot drains from TX
 * FIFO. BREAK at end-of-frame N == start-of-frame N+1 for the receiver.
 * No trigger byte, no baud switching, no GPIO signal-matrix tricks.
 */

#include "dmx.h"

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/logging/log.h>

#include <hal/uart_ll.h>

LOG_MODULE_REGISTER(dmx, LOG_LEVEL_INF);

#define DMX_UART_NODE DT_ALIAS(dmx_uart)
BUILD_ASSERT(DT_NODE_HAS_STATUS_OKAY(DMX_UART_NODE),
	     "dmx-uart alias missing or disabled");

#define DMX_UART_NUM   1
#define DMX_BAUD       250000
#define DMX_BREAK_BITS 30
#define DMX_MAB_US     20
#define DMX_PERIOD_MS  25

static const struct device *const dmx_uart = DEVICE_DT_GET(DMX_UART_NODE);
static uint8_t universe[DMX_UNIVERSE_SIZE];
static struct k_mutex universe_lock;

static void dmx_tx_frame(void)
{
	uint8_t snapshot[DMX_UNIVERSE_SIZE];
	uart_dev_t *hw = UART_LL_GET_HW(DMX_UART_NUM);

	k_mutex_lock(&universe_lock, K_FOREVER);
	memcpy(snapshot, universe, sizeof(snapshot));
	k_mutex_unlock(&universe_lock);

	while (!uart_ll_is_tx_idle(hw)) {
		k_yield();
	}
	uart_ll_tx_break(hw, 0);
	k_busy_wait(DMX_MAB_US);

	uart_poll_out(dmx_uart, 0x00);
	for (size_t i = 0; i < DMX_UNIVERSE_SIZE; i++) {
		uart_poll_out(dmx_uart, snapshot[i]);
	}
	uart_ll_tx_break(hw, DMX_BREAK_BITS);
}

static void dmx_tx_thread_fn(void *a, void *b, void *c)
{
	ARG_UNUSED(a); ARG_UNUSED(b); ARG_UNUSED(c);
	while (true) {
		dmx_tx_frame();
		k_msleep(DMX_PERIOD_MS);
	}
}

#define DMX_TX_STACK_SIZE 1536
#define DMX_TX_PRIO       K_PRIO_COOP(7)
K_THREAD_STACK_DEFINE(dmx_tx_stack, DMX_TX_STACK_SIZE);
static struct k_thread dmx_tx_thread_data;

int dmx_init(void)
{
	static bool started;
	if (started) {
		return 0;
	}
	if (!device_is_ready(dmx_uart)) {
		return -ENODEV;
	}

	const struct uart_config cfg = {
		.baudrate  = DMX_BAUD,
		.parity    = UART_CFG_PARITY_NONE,
		.stop_bits = UART_CFG_STOP_BITS_2,
		.data_bits = UART_CFG_DATA_BITS_8,
		.flow_ctrl = UART_CFG_FLOW_CTRL_NONE,
	};
	int err = uart_configure(dmx_uart, &cfg);
	if (err) {
		return err;
	}

	/* Invert TXD polarity. Waveshare board's RS485 transceiver A/B pin
	 * mapping vs. DMX-standard XLR pin 2/3 was ambiguous; this makes
	 * every UART bit inverted before it hits the differential driver.
	 * If DMX works after this flip, transceiver was convention-swapped. */
	uart_dev_t *hw = UART_LL_GET_HW(DMX_UART_NUM);
	uart_ll_inverse_signal(hw, UART_SIGNAL_TXD_INV);

	k_mutex_init(&universe_lock);

	k_thread_create(&dmx_tx_thread_data, dmx_tx_stack,
			K_THREAD_STACK_SIZEOF(dmx_tx_stack),
			dmx_tx_thread_fn, NULL, NULL, NULL,
			DMX_TX_PRIO, 0, K_NO_WAIT);
	k_thread_name_set(&dmx_tx_thread_data, "dmx_tx");

	started = true;
	LOG_INF("dmx tx up: 250k 8N2, break %u bits, %d Hz",
		DMX_BREAK_BITS, 1000 / DMX_PERIOD_MS);
	return 0;
}

void dmx_set_slot(uint16_t ch, uint8_t value)
{
	if (ch < 1 || ch > DMX_UNIVERSE_SIZE) {
		return;
	}
	k_mutex_lock(&universe_lock, K_FOREVER);
	universe[ch - 1] = value;
	k_mutex_unlock(&universe_lock);
}

void dmx_set_slots(uint16_t start, const uint8_t *buf, size_t n)
{
	if (start < 1 || start > DMX_UNIVERSE_SIZE || buf == NULL || n == 0) {
		return;
	}
	size_t max = DMX_UNIVERSE_SIZE - (start - 1);
	if (n > max) {
		n = max;
	}
	k_mutex_lock(&universe_lock, K_FOREVER);
	memcpy(&universe[start - 1], buf, n);
	k_mutex_unlock(&universe_lock);
}

void dmx_blackout(void)
{
	k_mutex_lock(&universe_lock, K_FOREVER);
	memset(universe, 0, sizeof(universe));
	k_mutex_unlock(&universe_lock);
}
