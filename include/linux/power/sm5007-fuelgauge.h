/*
 * include/linux/power/sm5007-fuelgauge.h
 *
 * SILICONMITUS SM5007 Fuelgauge Driver
 *
 * Copyright (C) 2012-2014 SILICONMITUS COMPANY,LTD
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef SM5007_FUELGAUGE_H
#define SM5007_FUELGAUGE_H

#define FG_DRIVER_VER "0.0.0.1"
#define SM5007_DELAY 1000
#define MINVAL(a, b) ((a <= b) ? a : b)


struct battery_data_t {
	const int battery_type; /* 4200 or 4350 or 4400*/
    const int battery_table[3][16];
    const int rce_value[3];
    const int dtcd_value;
};

struct sm5007_fg_info {
	/* State Of Connect */
	int online;
	/* battery SOC (capacity) */
	int batt_soc;
	/* battery voltage */
	int batt_voltage;
	/* battery AvgVoltage */
	int batt_avgvoltage;
	/* battery OCV */
	int batt_ocv;
	/* Current */
	int batt_current;
	/* battery Avg Current */
	int batt_avgcurrent;

	struct battery_data_t *comp_pdata;

	struct mutex param_lock;
	/* copy from platform data /
	 * DTS or update by shell script */

	struct mutex io_lock;
	struct device *dev;
	int32_t temperature;; /* 0.1 deg C*/
	/* register programming */
	int reg_addr;
	u8 reg_data[2];

    int battery_table[3][16];
    int rce_value[3];
    int dtcd_value;
    int rs_value[4]; /*rs mix_factor max min*/
    int vit_period;
    int mix_value[2]; /*mix_rate init_blank*/

    int enable_topoff_soc;
    int topoff_soc;

    int volt_cal;
    int curr_cal;

    int temp_std;
    int temp_offset;
    int temp_offset_cal;
    int charge_offset_cal;

    int battery_type; /* 4200 or 4350 or 4400*/
	uint32_t soc_alert_flag : 1;  /* 0 : nu-occur, 1: occur */
	uint32_t volt_alert_flag : 1; /* 0 : nu-occur, 1: occur */
	uint32_t flag_full_charge : 1; /* 0 : no , 1 : yes*/
	uint32_t flag_chg_status : 1; /* 0 : discharging, 1: charging*/

	int iocv_error_count;
	/* previous battery voltage */
	int p_batt_voltage;
	int p_batt_current;

	/*low voltage alarm mV level*/
	int alarm_vol_mv;

	int32_t irq_ctrl;

};

struct sm5007_fuelgauge_info {
    struct device      *dev;
	struct i2c_client		*client;
	/*sm5702_fg_platform_data_t *pdata;*/
	struct power_supply		psy_fg;
	struct delayed_work isr_work;
    struct delayed_work monitor_work;

	int cable_type;
	bool is_charging;

	struct sm5007_fg_info	info;

	bool is_fuel_alerted;
	/*struct wake_lock fuel_alert_wake_lock;*/

	unsigned int capacity_old;	/* only for atomic calculation */
	unsigned int capacity_max;	/* only for dynamic calculation */

	bool initial_update_of_soc;
	struct mutex fg_lock;

	/* register programming */
	int reg_addr;
	u8 reg_data[2];

	int fg_irq;
};

struct sm5007_fg_subdev_info {
	int			id;
	const char	*name;
	void		*platform_data;
};

struct sm5007_fg_platform_data {
	int		num_subdevs;
	struct	sm5007_fg_subdev_info *subdevs;
	int		irq_base;
};

struct sm5007_fuelgauge_type_data {
    int id;
    int battery_type;
    int *battery_table;
    int temp_std;
    int temp_offset;
    int temp_offset_cal;
    int charge_offset_cal;
    int rce_value[3];
    int dtcd_value;
    int rs_value[4];
    int vit_period;
    int mix_value[2];
    int topoff_soc[2];
    int volt_cal;
    int curr_cal;
};

#define BATTERY_TYPE_NUM 2
struct sm5007_fuelgauge_platform_data {
	int	irq;
	int	alarm_vol_mv;
	//int	multiple;
	//unsigned long	monitor_time;
	struct sm5007_fuelgauge_type_data type[BATTERY_TYPE_NUM];
};

#endif // SM5007_FUELGAUGE_H
