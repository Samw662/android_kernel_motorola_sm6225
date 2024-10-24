
/* SPDX-License-Identifier: GPL-2.0+ OR BSD-3-Clause */
/******************************************************************************
 * Copyright (c) 2020, STMicroelectronics - All Rights Reserved

 This file is part of VL53L1 and is dual licensed,
 either GPL-2.0+
 or 'BSD 3-clause "New" or "Revised" License' , at your option.
 ******************************************************************************
 */

/**
 * @file stmvl53l1.h header for vl53l1 sensor driver
 */
#ifndef STMVL53L1_H
#define STMVL53L1_H

#include <linux/types.h>
#include <linux/mutex.h>
#include <linux/workqueue.h>
#include <linux/miscdevice.h>
#include <linux/wait.h>

#include "vl53l1_api.h"

#ifdef DEBUG
#	define IPP_PRINT(...) printk(__VA_ARGS__)
#else
#	define IPP_PRINT(...) (void)0
#endif

struct st_timeval {
	time64_t tv_sec; /* seconds */
	long tv_usec; /* microseconds */
};

void st_gettimeofday(struct st_timeval *tv);

#include "stmvl53l1_if.h"

#define STMVL53L1_MAX_CCI_XFER_SZ	256
#define STMVL53L1_DRV_NAME	"stmvl53l1"

/**
 * configure usage of regulator device from device tree info
 * to enable/disable sensor power
 * see module-i2c or module-cci file
 */
/* define CFG_STMVL53L1_HAVE_REGULATOR */

#define DRIVER_VERSION		"14.1.2"

/** @ingroup vl53l1_config
 * @{
 */
/**
 * Configure max number of device the driver can support
 */
#define STMVL53L1_CFG_MAX_DEV	2
/** @} */ /* ingroup vl53l1_config */

/** @ingroup vl53l1_mod_dbg
 * @{
 */
#if 0
#define DEBUG	1
#endif
#if 0
#define FORCE_CONSOLE_DEBUG 1
#endif

extern int stmvl53l1_enable_debug;

#ifdef DEBUG
#	ifdef FORCE_CONSOLE_DEBUG
#define vl53l1_dbgmsg(str, ...) do { \
	if (stmvl53l1_enable_debug) \
		pr_err("%s: " str, __func__, ##__VA_ARGS__); \
} while (0)
#	else
#define vl53l1_dbgmsg(str, ...) do { \
	if (stmvl53l1_enable_debug) \
		pr_err("%s: " str, __func__, ##__VA_ARGS__); \
} while (0)
#	endif
#else
#	define vl53l1_dbgmsg(...) (void)0
#endif

/**
 * set to 0 1 activate or not debug from work (data interrupt/polling)
 */
#define WORK_DEBUG	0
#if WORK_DEBUG
#	define work_dbg(msg, ...)\
	printk("[D WK53L1] :" msg "\n", ##__VA_ARGS__)
#else
#	define work_dbg(...) (void)0
#endif

#define vl53l1_info(str, args...) \
	pr_info("%s: " str, __func__, ##args)

#define vl53l1_errmsg(str, args...) \
	pr_err("%s: " str, __func__, ##args)

#define vl53l1_wanrmsg(str, args...) \
	pr_warn("%s: " str, __func__, ##args)

/* turn off poll log if not defined */
#ifndef STMVL53L1_LOG_POLL_TIMING
#	define STMVL53L1_LOG_POLL_TIMING	0
#endif
/* turn off cci log timing if not defined */
#ifndef STMVL53L1_LOG_CCI_TIMING
#	define STMVL53L1_LOG_CCI_TIMING	0
#endif

/**@} */ /* ingroup mod_dbg*/

#include <net/sock.h>
#include <linux/netlink.h>
#include <linux/wait.h>

struct stmvl53l1_waiters {
	struct list_head list;
	pid_t pid;
};

/*
 *  driver data structs
 */
struct stmvl53l1_data {
	int id;			/*!< multiple device id 0 based*/
	char name[64];		/*!< misc device name */
	uint8_t product_type; /*!< L1:0xCC L3:0xAA i.e. module_type in datasheet */

	VL53L1_DevData_t stdev;	/*!<embed ST VL53L1 Dev data as "stdev" */

	void *client_object;	/*!< cci or i2c model i/f specific ptr  */
	bool is_device_remove;	/*!< true when device has been remove */

	struct mutex work_mutex; /*!< main dev mutex/lock */;
	struct delayed_work	dwork;
	/*!< work for pseudo irq polling check  */

	struct input_dev *input_dev_ps;
	/*!< input device used for sending event */

	/* misc device */
	struct miscdevice miscdev;
	/* first irq has no valid data, so avoid to update data on first one */
	int is_first_irq;
	/* set when first start has be done */
	int is_first_start_done;

#ifdef PHIO_TIMEOUT_FIX
	/* set when first frame happen */
	bool not_first_frame;
#endif

	/* control data */
	int poll_mode;	/*!< use poll even if interrupt line present*/
	int poll_delay_ms;	/*!< rescheduled time use in poll mode  */
	int enable_sensor;	/*!< actual device enabled state  */
	struct st_timeval start_tv;/*!< stream start time */
	int enable_debug;
	bool allow_hidden_start_stop; /*!< allow stop/start sequence in bare */

	/* Custom values set by app */

	int preset_mode;	/*!< preset working mode of the device */
	uint32_t timing_budget;	/*!< Timing Budget */
	int distance_mode;	/*!< distance mode of the device */
	int crosstalk_enable;	/*!< is crosstalk compensation is enable */
	int output_mode;	/*!< output mode of the device */
	bool force_device_on_en;/*!< keep device active when stopped */
	bool force_device_on_en_default;
	int sar_mode; /*!< is sar mode enable/disabled */
	int cam_mode; /*!< is camera mode enable/disabled */
	VL53L1_Error last_error;/*!< last device internal error */
	int offset_correction_mode;/*!< offset correction mode to apply */
	FixPoint1616_t dmax_reflectance;/*!< reflectance use for dmax calc */
	int dmax_mode;		/*!< Dmax mode of the device */
	int smudge_correction_mode; /*!< smudge mode */

	/* Read only values */
	FixPoint1616_t optical_offset_x;
	FixPoint1616_t optical_offset_y;
	bool is_xtalk_value_changed; /*!< xtalk values has been updated */

	/* PS parameters */

	/* Calibration parameters */
	bool is_calibrating;	/*!< active during calibration phases */
	uint16_t xtalk_offset; /*!< crosstalk compensation plane offset kcps */
	int16_t xtalk_x; /*!< crosstalk compensation x plane gradient kcps */
	int16_t xtalk_y; /*!< crosstalk compensation y plane gradient kcps */
	int16_t inner_offset; /*!< inner offset mm */
	int16_t outer_offset; /*!< outer offset mm */
	VL53L1_CalibrationData_t *cali_data; /*!< full calibration data */

	/* Range Data and stat */
	struct range_t {
		uint32_t	cnt;
		uint32_t	intr;
		int	poll_cnt;
		uint32_t	err_cnt; /* on actual measurement */
		uint32_t	err_tot; /* from start */
		struct st_timeval start_tv;
		struct st_timeval comp_tv;
		VL53L1_RangingMeasurementData_t single_range_data;
		VL53L1_MultiRangingData_t multi_range_data;
		VL53L1_MultiRangingData_t tmp_range_data;
		VL53L1_AdditionalData_t additional_data;
		/* non mode 1 for data agregation */
	} meas;

	/* workqueue use to fire flush event */
	uint32_t flushCount;
	int flush_todo_counter;

	/* Device parameters */
	/* Polling thread */
	/* Wait Queue on which the poll thread blocks */

	/* Manage blocking ioctls */
	struct list_head simple_data_reader_list;
	struct list_head mz_data_reader_list;
	wait_queue_head_t waiter_for_data;
	bool is_data_valid;

	/* control when using delay is acceptable */
	bool is_delay_allowed;

	/* maintain reset state */
	int reset_state;

	/* Recent interrupt status */
	/* roi */
	VL53L1_RoiConfig_t roi_cfg;

	/* use for zone calibration / roi mismatch detection */
	uint32_t current_roi_id;

	/* Embed here since it's too huge for kernek stack */
	struct stmvl53l1_ioctl_zone_calibration_data_t calib;

	/* autonomous config */
	uint32_t auto_pollingTimeInMs;
	VL53L1_DetectionConfig_t auto_config;

	/* autonomous config for camera */
	uint32_t auto_pollingTimeInMs_cam;
	VL53L1_DetectionConfig_t auto_config_cam;

	int sysfs_base;
};


/**
 * st_timeval diff in us
 *
 * @param pstart_tv
 * @param pstop_tv
 */
long stmvl53l1_tv_dif(struct st_timeval *pstart_tv, struct st_timeval *pstop_tv);


/**
 * The device table list table is update as device get added
 * we do not support adding removing device mutiple time !
 * use for clean "unload" purpose
 */
extern struct stmvl53l1_data *stmvl53l1_dev_table[];

int stmvl53l1_setup(struct stmvl53l1_data *data);
void stmvl53l1_cleanup(struct stmvl53l1_data *data);
#ifdef CONFIG_PM_SLEEP
void stmvl53l1_pm_suspend_stop(struct stmvl53l1_data *data);
#endif
int stmvl53l1_intr_handler(struct stmvl53l1_data *data);

int stmvl53l1_sysfs_laser(struct stmvl53l1_data *data, bool create);

/*
 *  function pointer structs
 */



#endif /* STMVL53L1_H */
