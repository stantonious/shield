/* main.c - Application main entry point */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/kernel.h>

#include <zephyr/settings/settings.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/drivers/i2c.h>

#include <zephyr/drivers/gpio.h>
#include "mlx90614esf.h"
#include "tf-luna.h"

#define I2C0_NODE DT_NODELABEL(tempsensor)
#define LIDAR_I2C1_NODE DT_NODELABEL(lidarsensor)
#define BUZZER_NODE DT_NODELABEL(buzzer)
#define ICE_THRESH_F 40.
#define SURFACE_MIN_THRESH 100
#define SURFACE_MAX_THRESH 120

/* Custom Service Variables */
#define BT_UUID_CUSTOM_SERVICE_VAL \
	BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef0)

static struct bt_uuid_128 vnd_uuid = BT_UUID_INIT_128(
	BT_UUID_CUSTOM_SERVICE_VAL);

#define BT_UUID_DW_BUZZ_VAL 0xaa00
#define BT_UUID_DW_BUZZ_CNTRL \
	BT_UUID_DECLARE_16(BT_UUID_DW_BUZZ_VAL)

static uint16_t dw_bt_dist = 0;
static float dw_bt_temp;

//Acquire the distance measurement from lidar sensor
static ssize_t read_dist(struct bt_conn *conn,
						 const struct bt_gatt_attr *attr, void *buf,
						 uint16_t len, uint16_t offset)
{
	dw_bt_dist = get_dist();
	return bt_gatt_attr_read(conn, attr, buf, len, offset, &dw_bt_dist,
							 sizeof(dw_bt_dist));
}

//Acquire the temp measurement from the temperature sensor
static void take_temp()
{
	dw_bt_temp = read_sample();
}

//Acquire the temp measurement from the temperature sensor
static ssize_t read_temp(struct bt_conn *conn,
						 const struct bt_gatt_attr *attr, void *buf,
						 uint16_t len, uint16_t offset)
{
	take_temp();
	return bt_gatt_attr_read(conn, attr, buf, len, offset, &dw_bt_temp,
							 sizeof(dw_bt_temp));
}

uint32_t dw_bt_buzz_ctrl = 0;
static ssize_t read_buzz_ctrl(struct bt_conn *conn, const struct bt_gatt_attr *attr,
							  void *buf, uint16_t len, uint16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset, &dw_bt_buzz_ctrl,
							 sizeof(dw_bt_buzz_ctrl));
}

static ssize_t write_buzz_ctrl(struct bt_conn *conn, const struct bt_gatt_attr *attr,
							   const void *buf, uint16_t len, uint16_t offset,
							   uint8_t flags)
{
	memcpy(&dw_bt_buzz_ctrl, buf, 4);
	return len;
}

static void dw_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
}

/* Vendor Primary Service Declaration */
BT_GATT_SERVICE_DEFINE(vnd_svc,
					   BT_GATT_PRIMARY_SERVICE(&vnd_uuid),
					   BT_GATT_CHARACTERISTIC(BT_UUID_GATT_DST,
											  BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
											  BT_GATT_PERM_READ, read_dist, NULL,
											  &dw_bt_dist),
					   BT_GATT_CCC(dw_ccc_cfg_changed,
								   BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
					   BT_GATT_CHARACTERISTIC(BT_UUID_HTS_TEMP_F,
											  BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
											  BT_GATT_PERM_READ, read_temp, NULL,
											  &dw_bt_temp),
					   BT_GATT_CCC(dw_ccc_cfg_changed,
								   BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
					   BT_GATT_CHARACTERISTIC(BT_UUID_DW_BUZZ_CNTRL,
											  BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE_WITHOUT_RESP | BT_GATT_CHRC_WRITE,
											  BT_GATT_PERM_READ | BT_GATT_PERM_WRITE, read_buzz_ctrl, write_buzz_ctrl,
											  &dw_bt_buzz_ctrl),
					   BT_GATT_CCC(dw_ccc_cfg_changed,
								   BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),

);

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_CUSTOM_SERVICE_VAL),
};

static void connected(struct bt_conn *conn, uint8_t err)
{
	if (err)
	{
		printk("Connection failed (err 0x%02x)\n", err);
	}
	else
	{
		printk("Connected\n");
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	printk("Disconnected (reason 0x%02x)\n", reason);
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
};


static void bt_ready(void)
{
	int err;

	printk("Bluetooth initialized\n");

	if (IS_ENABLED(CONFIG_SETTINGS))
	{
		settings_load();
	}

	err = bt_le_adv_start(BT_LE_ADV_CONN_NAME, ad, ARRAY_SIZE(ad), NULL, 0);
	if (err)
	{
		printk("Advertising failed to start (err %d)\n", err);
		return;
	}

	printk("Advertising successfully started\n");
}

int main(void)
{
	static const struct i2c_dt_spec lidar_dev_i2c = I2C_DT_SPEC_GET(LIDAR_I2C1_NODE);

	if (!device_is_ready(lidar_dev_i2c.bus))
	{
		printk("I2C bus %s is not ready!\n\r", lidar_dev_i2c.bus->name);
		return 1;
	}
	else
	{
		printk("I2C bus %s is ready!\n\r", lidar_dev_i2c.bus->name);
	}

	static const struct i2c_dt_spec dev_i2c = I2C_DT_SPEC_GET(I2C0_NODE);

	if (!device_is_ready(dev_i2c.bus))
	{
		printk("I2C bus %s is not ready!\n\r", dev_i2c.bus->name);
		return 1;
	}

	init_mlx90614esf(&dev_i2c);

	printk("Initing tfluna\n");
	init_tfluna(&lidar_dev_i2c);
	printk("Done Initing tfluna\n");

	int err;

	err = bt_enable(NULL);
	if (err)
	{
		printk("Bluetooth init failed (err %d)\n", err);
		return 0;
	}

	bt_ready();

	uint16_t step = 0;

	// Configure buzz gpio
	static const struct gpio_dt_spec buz_gpio = GPIO_DT_SPEC_GET(BUZZER_NODE, gpios);
	int ret = gpio_pin_configure_dt(&buz_gpio, GPIO_OUTPUT | GPIO_ACTIVE_LOW);
	if (ret != 0)
	{
		printk("Failed to configure gpio %d", ret);
	}

	while (1)
	{
		step++;
		dw_bt_dist = get_dist();
		take_temp();
		bool ice_detect = false;

		gpio_pin_set_dt(&buz_gpio, 0);

		if (dw_bt_buzz_ctrl == 0)
		{
			gpio_pin_set_dt(&buz_gpio, 0);
		}
		else
		{
			if (dw_bt_temp <= ICE_THRESH_F)
			{
				gpio_pin_set_dt(&buz_gpio, 1);
				ice_detect = true;
			}
			else
			{
				gpio_pin_set_dt(&buz_gpio, 0);
				ice_detect = false;
			}

			if (!ice_detect)
			{
				if (dw_bt_dist <= SURFACE_MIN_THRESH || dw_bt_dist >= SURFACE_MAX_THRESH)
				{
					gpio_pin_set_dt(&buz_gpio, 1);
				}
				else
				{
					gpio_pin_set_dt(&buz_gpio, 0);
				}
			}
		}

		k_sleep(K_MSEC(10));

		if (step % 100 == 0)
		{
			bt_gatt_notify(NULL, &vnd_svc.attrs[2], &dw_bt_dist, sizeof(dw_bt_dist));
			int tmp_tmp = dw_bt_temp * 10;
			bt_gatt_notify(NULL, &vnd_svc.attrs[4], &tmp_tmp, 4);
		}
	}
	return 0;
}
