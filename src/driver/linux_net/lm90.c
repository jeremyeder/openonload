/*
** Copyright 2005-2012  Solarflare Communications Inc.
**                      7505 Irvine Center Drive, Irvine, CA 92618, USA
** Copyright 2002-2005  Level 5 Networks Inc.
**
** This program is free software; you can redistribute it and/or modify it
** under the terms of version 2 of the GNU General Public License as
** published by the Free Software Foundation.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
*/

/*
 * lm90.c - Part of lm_sensors, Linux kernel modules for hardware
 *          monitoring
 * Copyright (C) 2003-2008  Jean Delvare <khali@linux-fr.org>
 *
 * Based on the lm83 driver. The LM90 is a sensor chip made by National
 * Semiconductor. It reports up to two temperatures (its own plus up to
 * one external one) with a 0.125 deg resolution (1 deg for local
 * temperature) and a 3-4 deg accuracy.
 *
 * This driver also supports the LM89 and LM99, two other sensor chips
 * made by National Semiconductor. Both have an increased remote
 * temperature measurement accuracy (1 degree), and the LM99
 * additionally shifts remote temperatures (measured and limits) by 16
 * degrees, which allows for higher temperatures measurement. The
 * driver doesn't handle it since it can be done easily in user-space.
 * Note that there is no way to differentiate between both chips.
 *
 * This driver also supports the LM86, another sensor chip made by
 * National Semiconductor. It is exactly similar to the LM90 except it
 * has a higher accuracy.
 *
 * This driver also supports the ADM1032, a sensor chip made by Analog
 * Devices. That chip is similar to the LM90, with a few differences
 * that are not handled by this driver. Among others, it has a higher
 * accuracy than the LM90, much like the LM86 does.
 *
 * This driver also supports the MAX6657, MAX6658 and MAX6659 sensor
 * chips made by Maxim. These chips are similar to the LM86.
 * Note that there is no easy way to differentiate between the three
 * variants. The extra address and features of the MAX6659 are not
 * supported by this driver. These chips lack the remote temperature
 * offset feature.
 *
 * This driver also supports the MAX6646, MAX6647 and MAX6649 chips
 * made by Maxim.  These are again similar to the LM86, but they use
 * unsigned temperature values and can report temperatures from 0 to
 * 145 degrees.
 *
 * This driver also supports the MAX6680 and MAX6681, two other sensor
 * chips made by Maxim. These are quite similar to the other Maxim
 * chips. The MAX6680 and MAX6681 only differ in the pinout so they can
 * be treated identically.
 *
 * This driver also supports the ADT7461 chip from Analog Devices.
 * It's supported in both compatibility and extended mode. It is mostly
 * compatible with LM90 except for a data format difference for the
 * temperature value registers.
 *
 * Since the LM90 was the first chipset supported by this driver, most
 * comments will refer to this chipset, but are actually general and
 * concern all supported chipsets, unless mentioned otherwise.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/jiffies.h>
#include <linux/i2c.h>
#include <linux/err.h>
#include <linux/sysfs.h>

#include "kernel_compat.h"
#ifndef EFX_HAVE_OLD_DEVICE_ATTRIBUTE
#include <linux/hwmon-sysfs.h>
#endif
#ifdef EFX_HAVE_HWMON_H
#include <linux/hwmon.h>
#endif
#ifdef EFX_HAVE_I2C_SENSOR_H
#include <linux/i2c-sensor.h>
#endif

#ifdef EFX_NEED_LM90_DRIVER

enum { lm90 = 1, adm1032, lm99, lm86, max6657, adt7461, max6680, max6646 };

/*
 * The LM90 registers
 */

#define LM90_REG_R_MAN_ID		0xFE
#define LM90_REG_R_CHIP_ID		0xFF
#define LM90_REG_R_CONFIG1		0x03
#define LM90_REG_W_CONFIG1		0x09
#define LM90_REG_R_CONFIG2		0xBF
#define LM90_REG_W_CONFIG2		0xBF
#define LM90_REG_R_CONVRATE		0x04
#define LM90_REG_W_CONVRATE		0x0A
#define LM90_REG_R_STATUS		0x02
#define LM90_REG_R_LOCAL_TEMP		0x00
#define LM90_REG_R_LOCAL_HIGH		0x05
#define LM90_REG_W_LOCAL_HIGH		0x0B
#define LM90_REG_R_LOCAL_LOW		0x06
#define LM90_REG_W_LOCAL_LOW		0x0C
#define LM90_REG_R_LOCAL_CRIT		0x20
#define LM90_REG_W_LOCAL_CRIT		0x20
#define LM90_REG_R_REMOTE_TEMPH		0x01
#define LM90_REG_R_REMOTE_TEMPL		0x10
#define LM90_REG_R_REMOTE_OFFSH		0x11
#define LM90_REG_W_REMOTE_OFFSH		0x11
#define LM90_REG_R_REMOTE_OFFSL		0x12
#define LM90_REG_W_REMOTE_OFFSL		0x12
#define LM90_REG_R_REMOTE_HIGHH		0x07
#define LM90_REG_W_REMOTE_HIGHH		0x0D
#define LM90_REG_R_REMOTE_HIGHL		0x13
#define LM90_REG_W_REMOTE_HIGHL		0x13
#define LM90_REG_R_REMOTE_LOWH		0x08
#define LM90_REG_W_REMOTE_LOWH		0x0E
#define LM90_REG_R_REMOTE_LOWL		0x14
#define LM90_REG_W_REMOTE_LOWL		0x14
#define LM90_REG_R_REMOTE_CRIT		0x19
#define LM90_REG_W_REMOTE_CRIT		0x19
#define LM90_REG_R_TCRIT_HYST		0x21
#define LM90_REG_W_TCRIT_HYST		0x21

/* MAX6646/6647/6649/6657/6658/6659 registers */

#define MAX6657_REG_R_LOCAL_TEMPL	0x11

/*
 * Device flags
 */
#define LM90_FLAG_ADT7461_EXT		0x01	/* ADT7461 extended mode */

/*
 * Functions declaration
 */

static void lm90_init_client(struct i2c_client *client);
static int lm90_remove(struct i2c_client *client);
#ifdef EFX_USE_I2C_LEGACY
static int lm90_detach_client(struct i2c_client *client);
#endif
static struct lm90_data *lm90_update_device(struct device *dev);

/*
 * Driver data (common to all clients)
 */

#if !defined(EFX_USE_I2C_LEGACY) && !defined(EFX_HAVE_OLD_I2C_DRIVER_PROBE)
static const struct i2c_device_id lm90_id[] = {
        { "sfc_max6647" },
        { }
};
#endif

struct i2c_driver efx_lm90_driver = {
#ifdef EFX_USE_I2C_DRIVER_NAME
	.name		= "sfc_lm90",
#else
	.driver.name	= "sfc_lm90",
#endif
#ifdef EFX_USE_I2C_LEGACY
        .detach_client  = lm90_detach_client,
#else
	.probe		= efx_lm90_probe,
	.remove		= lm90_remove,
#ifndef EFX_HAVE_OLD_I2C_DRIVER_PROBE
	.id_table	= lm90_id,
#endif
#endif
};

/*
 * Client data (each client gets its own)
 */

struct lm90_data {
#ifdef EFX_HAVE_HWMON_CLASS_DEVICE
        struct class_device *hwmon_dev;
#else
	struct device *hwmon_dev;
#endif
	struct mutex update_lock;
	char valid; /* zero until following fields are valid */
	unsigned long last_updated; /* in jiffies */
	int kind;
	int flags;

	/* registers values */
	s8 temp8[4];	/* 0: local low limit
			   1: local high limit
			   2: local critical limit
			   3: remote critical limit */
	s16 temp11[5];	/* 0: remote input
			   1: remote low limit
			   2: remote high limit
			   3: remote offset (except max6646 and max6657)
			   4: local input */
	u8 temp_hyst;
	u8 alarms; /* bitvector */
};

/*
 * Conversions
 * For local temperatures and limits, critical limits and the hysteresis
 * value, the LM90 uses signed 8-bit values with LSB = 1 degree Celsius.
 * For remote temperatures and limits, it uses signed 11-bit values with
 * LSB = 0.125 degree Celsius, left-justified in 16-bit registers.  Some
 * Maxim chips use unsigned values.
 */

static inline int temp_from_s8(s8 val)
{
	return val * 1000;
}

static inline int temp_from_u8(u8 val)
{
	return val * 1000;
}

static inline int temp_from_s16(s16 val)
{
	return val / 32 * 125;
}

static inline int temp_from_u16(u16 val)
{
	return val / 32 * 125;
}

static s8 temp_to_s8(long val)
{
	if (val <= -128000)
		return -128;
	if (val >= 127000)
		return 127;
	if (val < 0)
		return (val - 500) / 1000;
	return (val + 500) / 1000;
}

static u8 temp_to_u8(long val)
{
	if (val <= 0)
		return 0;
	if (val >= 255000)
		return 255;
	return (val + 500) / 1000;
}

static s16 temp_to_s16(long val)
{
	if (val <= -128000)
		return 0x8000;
	if (val >= 127875)
		return 0x7FE0;
	if (val < 0)
		return (val - 62) / 125 * 32;
	return (val + 62) / 125 * 32;
}

static u8 hyst_to_reg(long val)
{
	if (val <= 0)
		return 0;
	if (val >= 30500)
		return 31;
	return (val + 500) / 1000;
}

/*
 * ADT7461 in compatibility mode is almost identical to LM90 except that
 * attempts to write values that are outside the range 0 < temp < 127 are
 * treated as the boundary value.
 *
 * ADT7461 in "extended mode" operation uses unsigned integers offset by
 * 64 (e.g., 0 -> -64 degC).  The range is restricted to -64..191 degC.
 */
static inline int temp_from_u8_adt7461(struct lm90_data *data, u8 val)
{
	if (data->flags & LM90_FLAG_ADT7461_EXT)
		return (val - 64) * 1000;
	else
		return temp_from_s8(val);
}

static inline int temp_from_u16_adt7461(struct lm90_data *data, u16 val)
{
	if (data->flags & LM90_FLAG_ADT7461_EXT)
		return (val - 0x4000) / 64 * 250;
	else
		return temp_from_s16(val);
}

static u8 temp_to_u8_adt7461(struct lm90_data *data, long val)
{
	if (data->flags & LM90_FLAG_ADT7461_EXT) {
		if (val <= -64000)
			return 0;
		if (val >= 191000)
			return 0xFF;
		return (val + 500 + 64000) / 1000;
	} else {
		if (val <= 0)
			return 0;
		if (val >= 127000)
			return 127;
		return (val + 500) / 1000;
	}
}

static u16 temp_to_u16_adt7461(struct lm90_data *data, long val)
{
	if (data->flags & LM90_FLAG_ADT7461_EXT) {
		if (val <= -64000)
			return 0;
		if (val >= 191750)
			return 0xFFC0;
		return (val + 64000 + 125) / 250 * 64;
	} else {
		if (val <= 0)
			return 0;
		if (val >= 127750)
			return 0x7FC0;
		return (val + 125) / 250 * 64;
	}
}

/*
 * Sysfs stuff
 */

static ssize_t show_temp8(struct device *dev, struct device_attribute *devattr,
			  char *buf)
{
	struct sensor_device_attribute *attr = to_sensor_dev_attr(devattr);
	struct lm90_data *data = lm90_update_device(dev);
	int temp;

	if (data->kind == adt7461)
		temp = temp_from_u8_adt7461(data, data->temp8[attr->index]);
	else if (data->kind == max6646)
		temp = temp_from_u8(data->temp8[attr->index]);
	else
		temp = temp_from_s8(data->temp8[attr->index]);

	return sprintf(buf, "%d\n", temp);
}

static ssize_t set_temp8(struct device *dev, struct device_attribute *devattr,
			 const char *buf, size_t count)
{
	static const u8 reg[4] = {
		LM90_REG_W_LOCAL_LOW,
		LM90_REG_W_LOCAL_HIGH,
		LM90_REG_W_LOCAL_CRIT,
		LM90_REG_W_REMOTE_CRIT,
	};

	struct sensor_device_attribute *attr = to_sensor_dev_attr(devattr);
	struct i2c_client *client = to_i2c_client(dev);
	struct lm90_data *data = i2c_get_clientdata(client);
	long val = simple_strtol(buf, NULL, 10);
	int nr = attr->index;

	mutex_lock(&data->update_lock);
	if (data->kind == adt7461)
		data->temp8[nr] = temp_to_u8_adt7461(data, val);
	else if (data->kind == max6646)
		data->temp8[nr] = temp_to_u8(val);
	else
		data->temp8[nr] = temp_to_s8(val);
	i2c_smbus_write_byte_data(client, reg[nr], data->temp8[nr]);
	mutex_unlock(&data->update_lock);
	return count;
}

static ssize_t show_temp11(struct device *dev, struct device_attribute *devattr,
			   char *buf)
{
	struct sensor_device_attribute *attr = to_sensor_dev_attr(devattr);
	struct lm90_data *data = lm90_update_device(dev);
	int temp;

	if (data->kind == adt7461)
		temp = temp_from_u16_adt7461(data, data->temp11[attr->index]);
	else if (data->kind == max6646)
		temp = temp_from_u16(data->temp11[attr->index]);
	else
		temp = temp_from_s16(data->temp11[attr->index]);

	return sprintf(buf, "%d\n", temp);
}

static ssize_t set_temp11(struct device *dev, struct device_attribute *devattr,
			  const char *buf, size_t count)
{
	static const u8 reg[6] = {
		LM90_REG_W_REMOTE_LOWH,
		LM90_REG_W_REMOTE_LOWL,
		LM90_REG_W_REMOTE_HIGHH,
		LM90_REG_W_REMOTE_HIGHL,
		LM90_REG_W_REMOTE_OFFSH,
		LM90_REG_W_REMOTE_OFFSL,
	};

	struct sensor_device_attribute *attr = to_sensor_dev_attr(devattr);
	struct i2c_client *client = to_i2c_client(dev);
	struct lm90_data *data = i2c_get_clientdata(client);
	long val = simple_strtol(buf, NULL, 10);
	int nr = attr->index;

	mutex_lock(&data->update_lock);
	if (data->kind == adt7461)
		data->temp11[nr] = temp_to_u16_adt7461(data, val);
	else if (data->kind == max6657 || data->kind == max6680)
		data->temp11[nr] = temp_to_s8(val) << 8;
	else if (data->kind == max6646)
		data->temp11[nr] = temp_to_u8(val) << 8;
	else
		data->temp11[nr] = temp_to_s16(val);

	i2c_smbus_write_byte_data(client, reg[(nr - 1) * 2],
				  data->temp11[nr] >> 8);
	if (data->kind != max6657 && data->kind != max6680
	    && data->kind != max6646)
		i2c_smbus_write_byte_data(client, reg[(nr - 1) * 2 + 1],
					  data->temp11[nr] & 0xff);
	mutex_unlock(&data->update_lock);
	return count;
}

static ssize_t show_temphyst(struct device *dev, struct device_attribute *devattr,
			     char *buf)
{
	struct sensor_device_attribute *attr = to_sensor_dev_attr(devattr);
	struct lm90_data *data = lm90_update_device(dev);
	int temp;

	if (data->kind == adt7461)
		temp = temp_from_u8_adt7461(data, data->temp8[attr->index]);
	else
		temp = temp_from_s8(data->temp8[attr->index]);

	return sprintf(buf, "%d\n", temp - temp_from_s8(data->temp_hyst));
}

static ssize_t set_temphyst(struct device *dev, struct device_attribute *dummy,
			    const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct lm90_data *data = i2c_get_clientdata(client);
	long val = simple_strtol(buf, NULL, 10);
	long hyst;

	mutex_lock(&data->update_lock);
	hyst = temp_from_s8(data->temp8[2]) - val;
	i2c_smbus_write_byte_data(client, LM90_REG_W_TCRIT_HYST,
				  hyst_to_reg(hyst));
	mutex_unlock(&data->update_lock);
	return count;
}

static ssize_t show_alarms(struct device *dev, struct device_attribute *dummy,
			   char *buf)
{
	struct lm90_data *data = lm90_update_device(dev);
	return sprintf(buf, "%d\n", data->alarms);
}

static ssize_t show_alarm(struct device *dev, struct device_attribute
			  *devattr, char *buf)
{
	struct sensor_device_attribute *attr = to_sensor_dev_attr(devattr);
	struct lm90_data *data = lm90_update_device(dev);
	int bitnr = attr->index;

	return sprintf(buf, "%d\n", (data->alarms >> bitnr) & 1);
}

static SENSOR_DEVICE_ATTR(temp1_input, S_IRUGO, show_temp11, NULL, 4);
static SENSOR_DEVICE_ATTR(temp2_input, S_IRUGO, show_temp11, NULL, 0);
static SENSOR_DEVICE_ATTR(temp1_min, S_IWUSR | S_IRUGO, show_temp8,
	set_temp8, 0);
static SENSOR_DEVICE_ATTR(temp2_min, S_IWUSR | S_IRUGO, show_temp11,
	set_temp11, 1);
static SENSOR_DEVICE_ATTR(temp1_max, S_IWUSR | S_IRUGO, show_temp8,
	set_temp8, 1);
static SENSOR_DEVICE_ATTR(temp2_max, S_IWUSR | S_IRUGO, show_temp11,
	set_temp11, 2);
static SENSOR_DEVICE_ATTR(temp1_crit, S_IWUSR | S_IRUGO, show_temp8,
	set_temp8, 2);
static SENSOR_DEVICE_ATTR(temp2_crit, S_IWUSR | S_IRUGO, show_temp8,
	set_temp8, 3);
static SENSOR_DEVICE_ATTR(temp1_crit_hyst, S_IWUSR | S_IRUGO, show_temphyst,
	set_temphyst, 2);
static SENSOR_DEVICE_ATTR(temp2_crit_hyst, S_IRUGO, show_temphyst, NULL, 3);
static SENSOR_DEVICE_ATTR(temp2_offset, S_IWUSR | S_IRUGO, show_temp11,
	set_temp11, 3);

/* Individual alarm files */
static SENSOR_DEVICE_ATTR(temp1_crit_alarm, S_IRUGO, show_alarm, NULL, 0);
static SENSOR_DEVICE_ATTR(temp2_crit_alarm, S_IRUGO, show_alarm, NULL, 1);
static SENSOR_DEVICE_ATTR(temp2_fault, S_IRUGO, show_alarm, NULL, 2);
static SENSOR_DEVICE_ATTR(temp2_min_alarm, S_IRUGO, show_alarm, NULL, 3);
static SENSOR_DEVICE_ATTR(temp2_max_alarm, S_IRUGO, show_alarm, NULL, 4);
static SENSOR_DEVICE_ATTR(temp1_min_alarm, S_IRUGO, show_alarm, NULL, 5);
static SENSOR_DEVICE_ATTR(temp1_max_alarm, S_IRUGO, show_alarm, NULL, 6);
/* Raw alarm file for compatibility */
static DEVICE_ATTR(alarms, S_IRUGO, show_alarms, NULL);

static struct attribute *lm90_attributes[] = {
	&sensor_dev_attr_temp1_input.dev_attr.attr,
	&sensor_dev_attr_temp2_input.dev_attr.attr,
	&sensor_dev_attr_temp1_min.dev_attr.attr,
	&sensor_dev_attr_temp2_min.dev_attr.attr,
	&sensor_dev_attr_temp1_max.dev_attr.attr,
	&sensor_dev_attr_temp2_max.dev_attr.attr,
	&sensor_dev_attr_temp1_crit.dev_attr.attr,
	&sensor_dev_attr_temp2_crit.dev_attr.attr,
	&sensor_dev_attr_temp1_crit_hyst.dev_attr.attr,
	&sensor_dev_attr_temp2_crit_hyst.dev_attr.attr,

	&sensor_dev_attr_temp1_crit_alarm.dev_attr.attr,
	&sensor_dev_attr_temp2_crit_alarm.dev_attr.attr,
	&sensor_dev_attr_temp2_fault.dev_attr.attr,
	&sensor_dev_attr_temp2_min_alarm.dev_attr.attr,
	&sensor_dev_attr_temp2_max_alarm.dev_attr.attr,
	&sensor_dev_attr_temp1_min_alarm.dev_attr.attr,
	&sensor_dev_attr_temp1_max_alarm.dev_attr.attr,
	&dev_attr_alarms.attr,
	NULL
};

static const struct attribute_group lm90_group = {
	.attrs = lm90_attributes,
};

/* pec used for ADM1032 only */
static ssize_t show_pec(struct device *dev, struct device_attribute *dummy,
			char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	return sprintf(buf, "%d\n", !!(client->flags & I2C_CLIENT_PEC));
}

static ssize_t set_pec(struct device *dev, struct device_attribute *dummy,
		       const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	long val = simple_strtol(buf, NULL, 10);

	switch (val) {
	case 0:
		client->flags &= ~I2C_CLIENT_PEC;
		break;
	case 1:
		client->flags |= I2C_CLIENT_PEC;
		break;
	default:
		return -EINVAL;
	}

	return count;
}

static DEVICE_ATTR(pec, S_IWUSR | S_IRUGO, show_pec, set_pec);

/*
 * Real code
 */

/* The ADM1032 supports PEC but not on write byte transactions, so we need
   to explicitly ask for a transaction without PEC. */
static inline s32 adm1032_write_byte(struct i2c_client *client, u8 value)
{
	return i2c_smbus_xfer(client->adapter, client->addr,
			      client->flags & ~I2C_CLIENT_PEC,
			      I2C_SMBUS_WRITE, value, I2C_SMBUS_BYTE, NULL);
}

/* It is assumed that client->update_lock is held (unless we are in
   detection or initialization steps). This matters when PEC is enabled,
   because we don't want the address pointer to change between the write
   byte and the read byte transactions. */
static int lm90_read_reg(struct i2c_client* client, u8 reg, u8 *value)
{
	int err;

 	if (client->flags & I2C_CLIENT_PEC) {
 		err = adm1032_write_byte(client, reg);
 		if (err >= 0)
 			err = i2c_smbus_read_byte(client);
 	} else
 		err = i2c_smbus_read_byte_data(client, reg);

	if (err < 0) {
		dev_warn(&client->dev, "Register %#02x read failed (%d)\n",
			 reg, err);
		return err;
	}
	*value = err;

	return 0;
}

#ifdef EFX_HAVE_OLD_I2C_DRIVER_PROBE
int efx_lm90_probe(struct i2c_client *new_client)
#else
int efx_lm90_probe(struct i2c_client *new_client,
		   const struct i2c_device_id *id)
#endif
{
	struct lm90_data *data;
	int err;

	data = kzalloc(sizeof(struct lm90_data), GFP_KERNEL);
	if (!data) {
		err = -ENOMEM;
		goto exit;
	}
	i2c_set_clientdata(new_client, data);
	mutex_init(&data->update_lock);

	/* Set the device type */
	data->kind = max6646;

	/* Initialize the LM90 chip */
	lm90_init_client(new_client);

	/* Register sysfs hooks */
	if ((err = sysfs_create_group(&new_client->dev.kobj, &lm90_group)))
		goto exit_free;
	if (new_client->flags & I2C_CLIENT_PEC) {
		if ((err = device_create_file(&new_client->dev,
					      &dev_attr_pec)))
			goto exit_remove_files;
	}
	if (data->kind != max6657 && data->kind != max6646) {
		if ((err = device_create_file(&new_client->dev,
				&sensor_dev_attr_temp2_offset.dev_attr)))
			goto exit_remove_files;
	}

	data->hwmon_dev = hwmon_device_register(&new_client->dev);
	if (IS_ERR(data->hwmon_dev)) {
		err = PTR_ERR(data->hwmon_dev);
		goto exit_remove_files;
	}

	return 0;

exit_remove_files:
	sysfs_remove_group(&new_client->dev.kobj, &lm90_group);
	device_remove_file(&new_client->dev, &dev_attr_pec);
exit_free:
	kfree(data);
exit:
	return err;
}

static void lm90_init_client(struct i2c_client *client)
{
	u8 config, config_orig;
	struct lm90_data *data = i2c_get_clientdata(client);

	/*
	 * Start the conversions.
	 */
	i2c_smbus_write_byte_data(client, LM90_REG_W_CONVRATE,
				  5); /* 2 Hz */
	if (lm90_read_reg(client, LM90_REG_R_CONFIG1, &config) < 0) {
		dev_warn(&client->dev, "Initialization failed!\n");
		return;
	}
	config_orig = config;

	/* Check Temperature Range Select */
	if (data->kind == adt7461) {
		if (config & 0x04)
			data->flags |= LM90_FLAG_ADT7461_EXT;
	}

	/*
	 * Put MAX6680/MAX8881 into extended resolution (bit 0x10,
	 * 0.125 degree resolution) and range (0x08, extend range
	 * to -64 degree) mode for the remote temperature sensor.
	 */
	if (data->kind == max6680) {
		config |= 0x18;
	}

	config &= 0xBF;	/* run */
	if (config != config_orig) /* Only write if changed */
		i2c_smbus_write_byte_data(client, LM90_REG_W_CONFIG1, config);
}

static int lm90_remove(struct i2c_client *client)
{
	struct lm90_data *data = i2c_get_clientdata(client);

	hwmon_device_unregister(data->hwmon_dev);
	sysfs_remove_group(&client->dev.kobj, &lm90_group);
	device_remove_file(&client->dev, &dev_attr_pec);
	if (data->kind != max6657 && data->kind != max6646)
		device_remove_file(&client->dev,
				   &sensor_dev_attr_temp2_offset.dev_attr);

	kfree(data);
	return 0;
}

#ifdef EFX_USE_I2C_LEGACY
static int lm90_detach_client(struct i2c_client *client)
{
        int err;

        lm90_remove(client);
        err = i2c_detach_client(client);
        if (!err)
                kfree(client);
        return err;
}
#endif

static int lm90_read16(struct i2c_client *client, u8 regh, u8 regl, u16 *value)
{
	int err;
	u8 oldh, newh, l;

	/*
	 * There is a trick here. We have to read two registers to have the
	 * sensor temperature, but we have to beware a conversion could occur
	 * inbetween the readings. The datasheet says we should either use
	 * the one-shot conversion register, which we don't want to do
	 * (disables hardware monitoring) or monitor the busy bit, which is
	 * impossible (we can't read the values and monitor that bit at the
	 * exact same time). So the solution used here is to read the high
	 * byte once, then the low byte, then the high byte again. If the new
	 * high byte matches the old one, then we have a valid reading. Else
	 * we have to read the low byte again, and now we believe we have a
	 * correct reading.
	 */
	if ((err = lm90_read_reg(client, regh, &oldh))
	 || (err = lm90_read_reg(client, regl, &l))
	 || (err = lm90_read_reg(client, regh, &newh)))
		return err;
	if (oldh != newh) {
		err = lm90_read_reg(client, regl, &l);
		if (err)
			return err;
	}
	*value = (newh << 8) | l;

	return 0;
}

static struct lm90_data *lm90_update_device(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct lm90_data *data = i2c_get_clientdata(client);

	mutex_lock(&data->update_lock);

	if (time_after(jiffies, data->last_updated + HZ * 2) || !data->valid) {
		u8 h, l;

		dev_dbg(&client->dev, "Updating lm90 data.\n");
		lm90_read_reg(client, LM90_REG_R_LOCAL_LOW, &data->temp8[0]);
		lm90_read_reg(client, LM90_REG_R_LOCAL_HIGH, &data->temp8[1]);
		lm90_read_reg(client, LM90_REG_R_LOCAL_CRIT, &data->temp8[2]);
		lm90_read_reg(client, LM90_REG_R_REMOTE_CRIT, &data->temp8[3]);
		lm90_read_reg(client, LM90_REG_R_TCRIT_HYST, &data->temp_hyst);

		if (data->kind == max6657 || data->kind == max6646) {
			lm90_read16(client, LM90_REG_R_LOCAL_TEMP,
				    MAX6657_REG_R_LOCAL_TEMPL,
				    &data->temp11[4]);
		} else {
			if (lm90_read_reg(client, LM90_REG_R_LOCAL_TEMP,
					  &h) == 0)
				data->temp11[4] = h << 8;
		}
		lm90_read16(client, LM90_REG_R_REMOTE_TEMPH,
			    LM90_REG_R_REMOTE_TEMPL, &data->temp11[0]);

		if (lm90_read_reg(client, LM90_REG_R_REMOTE_LOWH, &h) == 0) {
			data->temp11[1] = h << 8;
			if (data->kind != max6657 && data->kind != max6680
			 && data->kind != max6646
			 && lm90_read_reg(client, LM90_REG_R_REMOTE_LOWL,
					  &l) == 0)
				data->temp11[1] |= l;
		}
		if (lm90_read_reg(client, LM90_REG_R_REMOTE_HIGHH, &h) == 0) {
			data->temp11[2] = h << 8;
			if (data->kind != max6657 && data->kind != max6680
			 && data->kind != max6646
			 && lm90_read_reg(client, LM90_REG_R_REMOTE_HIGHL,
					  &l) == 0)
				data->temp11[2] |= l;
		}

		if (data->kind != max6657 && data->kind != max6646) {
			if (lm90_read_reg(client, LM90_REG_R_REMOTE_OFFSH,
					  &h) == 0
			 && lm90_read_reg(client, LM90_REG_R_REMOTE_OFFSL,
					  &l) == 0)
				data->temp11[3] = (h << 8) | l;
		}
		lm90_read_reg(client, LM90_REG_R_STATUS, &data->alarms);

		data->last_updated = jiffies;
		data->valid = 1;
	}

	mutex_unlock(&data->update_lock);

	return data;
}

#endif /* EFX_NEED_LM90_DRIVER */
