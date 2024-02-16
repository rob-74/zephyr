/*
 * Copyright (c) 2024 BoRob Engineering
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#define DT_DRV_COMPAT anamicro_ams5915

#include <zephyr/types.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/init.h>
#include <zephyr/sys/__assert.h>

#include <zephyr/logging/log.h>

#include <math.h>

LOG_MODULE_REGISTER(AMS5915, CONFIG_SENSOR_LOG_LEVEL);

#define AMS5915_DIGPOUT_P_MAX		14745
#define AMS5915_DIGPOUT_P_MIN		1638
#define AMS5915_AMBIENT_TEMP_MAX	85.0f

struct ams5915_data {
	float temp_c;
	float press_mbar;
};

struct ams5915_config {
	const struct i2c_dt_spec bus;
	const uint8_t myVariant;
};

/* define range of different sensor variants (see datasheet) */

struct ams5915_range {
	float pmin;
	float pmax;	
};

const struct ams5915_range AMS5915_rangeMilliBar[26] =
{
	/* Ultra low pressure ranges */
	{ 0.0	, 	5.0		}, /* 0: AMS5915_0005_D_N     */
	{ 0.0	, 	10.0	}, /* 1: AMS5915_0010_D_N     */
	{ -2.5	, 	2.5		}, /* 2: AMS5915_0002_D_B_N   */
	{ -5.0	, 	5.0		}, /* 3: AMS5915_0005_D_B_N   */
	{ -10.0	,	10.0	}, /* 4: AMS5915_0010_D_B_N   */
	/* Low pressure ranges */
	{ 0.0	, 	20.0	}, /* 5: AMS5915_0020_D_N     */
	{ 0.0	, 	50.0	}, /* 6: AMS5915_0050_D_N     */
	{ 0.0	, 	100.0	}, /* 7: AMS5915_0100_D_N     */
	{ -20.0	, 	20.0	}, /* 8: AMS5915_0020_D_B_N   */
	{ -50.0	,	50.0	}, /* 9: AMS5915_0050_D_B_N   */
	{ -100.0,	100.0	}, /* 10: AMS5915_0100_D_B_N  */
	/* Standard pressure ranges */ 
	{ 0.0	, 	200.0	}, /* 11: AMS5915_0200_D_N    */
	{ 0.0	, 	350.0	}, /* 12: AMS5915_0350_D_N    */
	{ 0.0	, 	1000.0	}, /* 13: AMS5915_1000_D_N    */
	{ 0.0	, 	2000.0	}, /* 14: AMS5915_2000_D_N    */
	{ 0.0	,	4000.0	}, /* 15: AMS5915_4000_D_N    */
	{ 0.0	,	7000.0	}, /* 16: AMS5915_7000_D_N    */
	{ 0.0	,	10000.0	}, /* 17: AMS5915_10000_D_N   */
	{ -200.0,	200.0	}, /* 18: AMS5915_0200_D_B_N  */
	{ -350.0,	350.0	}, /* 19: AMS5915_0350_D_B_N  */
	{ -1000.0,	1000.0	}, /* 20: AMS5915_1000_D_B_N  */
 	/* Pneumatic ranges */ 
	{ 0.0	,	4000.0	}, /* 21: AMS5915_4000_D_I_N  */
	{ 0.0	,	7000.0	}, /* 22: AMS5915_7000_D_I_N  */
	{ 0.0	,	10000.0	}, /* 23: AMS5915_10000_D_I_N */
	{ 0.0	, 	12000.0	}, /* 24: AMS5915_12000_D_I_N */
	{ 0.0	, 	16000.0	}, /* 25: AMS5915_16000_D_I_N */
};

static void ams5915_adc2temp(const struct device *dev, int32_t adc_temp)
{
	struct ams5915_data *data = dev->data;

	float temp = (float)(adc_temp * 200) / 2048.0f - 50.0f;

	if (temp > AMS5915_AMBIENT_TEMP_MAX) {
		/* no valid temperature */
		temp = NAN;
	}

	data->temp_c = temp;
}

static void ams5915_adc2press(const struct device *dev, int32_t adc_press)
{
	struct ams5915_data *data = dev->data;
 	const struct ams5915_config *config = dev->config;

	/* get range counts per milli Bar */
	float Sensp = (AMS5915_DIGPOUT_P_MAX - AMS5915_DIGPOUT_P_MIN) / 
				(AMS5915_rangeMilliBar[config->myVariant].pmax - 
				 AMS5915_rangeMilliBar[config->myVariant].pmin);

	/* get milli bar */
	data->press_mbar = (float)(adc_press - AMS5915_DIGPOUT_P_MIN) / Sensp + 
							AMS5915_rangeMilliBar[config->myVariant].pmin;
}

static int ams5915_sample_fetch(const struct device *dev,
			       enum sensor_channel chan)
{
	const struct ams5915_config *cfg = dev->config;

	uint8_t buf[4] = {0};
	int32_t adc_press;
	int32_t adc_temp;
	int ret = 0;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL);

	ret = i2c_read_dt(&cfg->bus, buf, sizeof(buf));

	if (ret < 0) {
		return ret;
	}

	/* extract raw adc values from receive buffer */
	adc_press = (uint16_t)(buf[0] & 0x3F) << 8 | buf[1];
	adc_temp = (uint16_t)(buf[2]) << 3 | (buf[3] & 0xE0) >> 5;

	ams5915_adc2temp(dev, adc_temp);
	ams5915_adc2press(dev, adc_press); 

	return 0;
}

static int ams5915_channel_get(const struct device *dev,
			      enum sensor_channel chan,
			      struct sensor_value *val)
{
	struct ams5915_data *data = dev->data;

	switch (chan) {
	case SENSOR_CHAN_AMBIENT_TEMP:
		if(data->temp_c == NAN) {
			return -EINVAL;
		}
		sensor_value_from_float(val, data->temp_c);
		break;
	case SENSOR_CHAN_PRESS:
		sensor_value_from_float(val, data->press_mbar);
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static const struct sensor_driver_api ams5915_driver_api = {
	.sample_fetch	= ams5915_sample_fetch,
	.channel_get	= ams5915_channel_get,
};

int ams5915_init(const struct device *dev)
{
	const struct ams5915_config *cfg = dev->config;

	if (!device_is_ready(cfg->bus.bus)) {
		LOG_ERR("I2C dev %s not ready", cfg->bus.bus->name);
		return -EINVAL;
	}
	
	return 0;
}


#define AMS5915_INST(inst)                                          \
	static struct ams5915_data ams5915_data_##inst;                 \
	static const struct ams5915_config ams5915_config_##inst =	{   \
		.bus = I2C_DT_SPEC_INST_GET(inst),                          \
		.myVariant = DT_INST_ENUM_IDX(inst, variant),               \
	};	                                                            \
	                                                                \
	SENSOR_DEVICE_DT_INST_DEFINE(inst,                              \
			 ams5915_init,                                          \
			 NULL,                                                  \
			 &ams5915_data_##inst,                                  \
			 &ams5915_config_##inst,                                \
			 POST_KERNEL,                                           \
			 CONFIG_SENSOR_INIT_PRIORITY,                           \
			 &ams5915_driver_api);

DT_INST_FOREACH_STATUS_OKAY(AMS5915_INST)
