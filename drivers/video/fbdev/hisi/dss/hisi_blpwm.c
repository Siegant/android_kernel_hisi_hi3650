/* Copyright (c) 2013-2014, Hisilicon Tech. Co., Ltd. All rights reserved.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 and
* only version 2 as published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
* GNU General Public License for more details.
*
*/

#include "hisi_fb.h"
#include "backlight_linear_to_exponential.h"
#include "backlight/lm36923.h"
#include "backlight/lm36274.h"
#include "backlight/lp8556.h"
#include <linux/timer.h>
#include <linux/delay.h>

/* default pwm clk */
#define DEFAULT_PWM_CLK_RATE	(120 * 1000000L)
static char __iomem *hisifd_blpwm_base;
static struct clk *g_dss_blpwm_clk;
static struct platform_device *g_blpwm_pdev;
static int g_blpwm_on;
static int g_blpwm_fpga_flag;
static bool g_blpwm_thread_stop;

static struct pinctrl_data blpwmpctrl;

static struct pinctrl_cmd_desc blpwm_pinctrl_init_cmds[] = {
	{DTYPE_PINCTRL_GET, &blpwmpctrl, 0},
	{DTYPE_PINCTRL_STATE_GET, &blpwmpctrl, DTYPE_PINCTRL_STATE_DEFAULT},
	{DTYPE_PINCTRL_STATE_GET, &blpwmpctrl, DTYPE_PINCTRL_STATE_IDLE},
};

static struct pinctrl_cmd_desc blpwm_pinctrl_normal_cmds[] = {
	{DTYPE_PINCTRL_SET, &blpwmpctrl, DTYPE_PINCTRL_STATE_DEFAULT},
};

static struct pinctrl_cmd_desc blpwm_pinctrl_lowpower_cmds[] = {
	{DTYPE_PINCTRL_SET, &blpwmpctrl, DTYPE_PINCTRL_STATE_IDLE},
};

static struct pinctrl_cmd_desc blpwm_pinctrl_finit_cmds[] = {
	{DTYPE_PINCTRL_PUT, &blpwmpctrl, 0},
};


#define BLPWM_OUT_CTRL	(0x100)
#define BLPWM_OUT_DIV	(0x104)
#define BLPWM_OUT_CFG	(0x108)

#define BLPWM_OUT_PRECISION_DEFAULT	(800)
#define BLPWM_OUT_PRECISION	(10000)
#define BLPWM_OUT_PRECISION_2048	(2047)
#define BLPWM_OUT_PRECISION_4096	(4095)
#define BLPWM_BL_LEVEL_MIN	(4)

//BLPWM input address
#define PWM_IN_CTRL_OFFSET	(0x000)
#define PWM_IN_DIV_OFFSET	(0x004)
#define PWM_IN_NUM_OFFSET	(0x008)
#define PWM_IN_MAX_COUNTER	(0x010)
#define PWM_IN_PRECISION_DEFAULT	(512)
#define PWM_IN_PRECISION_RGBW	(8192)
#define PWM_IN_SENSITY	(2)

#define CABC_DIMMING_STEP_TOTAL_NUM    32
#define RGBW_DIMMING_STEP_TOTAL_NUM    3
static struct task_struct *cabc_pwm_task;

static uint8_t rgbw_lcd_support = 0;
#define PWM_PERIOD_TOTAL_COUNTOR_JDI_SHARP   4000
#define PWM_PERIOD_TOTAL_COUNTOR_LGD   1000
#define PWM_MAX_COUNTER_FOR_RGBW_JDI_SHARP   4500
#define PWMDUTY_FILTER_NUMBER 20
#define HIGHLEVEL_PWMDUTY_FILTER_NUMBER 3
#define BRIGHTNESS_FILTER_FOR_PWMDUTY 100

static int pwmduty[PWMDUTY_FILTER_NUMBER] = {
	0xffff,0xffff,0xffff,0xffff,0xffff,
	0xffff,0xffff,0xffff,0xffff,0xffff,
	0xffff,0xffff,0xffff,0xffff,0xffff,
	0xffff,0xffff,0xffff,0xffff,0xffff};

#define BACKLIGHT_FILTER_NUMBER 20
static int backlight_buf[BACKLIGHT_FILTER_NUMBER] ={
	0xffff,0xffff,0xffff,0xffff,0xffff,
	0xffff,0xffff,0xffff,0xffff,0xffff,
	0xffff,0xffff,0xffff,0xffff,0xffff,
	0xffff,0xffff,0xffff,0xffff,0xffff};
static unsigned int f_count = 0;

#define MIN_PWMDUTY_FOR_RGBW 13
#define RGBW_BL_STOP_THRESHOLD 200
#define RGBW_LOW_LIGHT_THRESHOLD  35
#define RGBW_LOW_LIGHT_STOP 30

static uint32_t last_rgbw_bl_count = 0;
static uint32_t rgbw_bl_stop_num = 0;
static uint32_t rgbw_bl_stop_threshold_num = 0;
static uint32_t backlight_update_counter = 0;
static uint32_t rgbw_abnormal_flag = 0;
static int32_t ap_backlight_delta_sum = 0;
static int32_t low_light_count = 0;
static int32_t last_backlight = 0;
static int32_t last_pwm_duty = MIN_PWMDUTY_FOR_RGBW;
static int32_t last_delta_pwm_duty_abs = 0;



struct bl_info{
	int32_t bl_max;
	int32_t bl_min;
	int32_t ap_brightness;
	int32_t last_ap_brightness;
	int32_t index_cabc_dimming;
	int32_t cabc_pwm;
	int32_t prev_cabc_pwm;
	int32_t current_cabc_pwm;
	int32_t cabc_pwm_in;
	int32_t last_bl_level;
	int32_t bl_ic_ctrl_mode;
	int32_t blpwm_input_precision;
	int32_t blpwm_out_precision;
	struct semaphore bl_semaphore;
	int (*set_common_backlight)(int bl_level);
};

static struct bl_info g_bl_info;

#define BL_LVL_MAP_SIZE	(2047)
static int bl_lvl_map(int level)
{
	int ret = 0;
	int idx = 0;

	if (level < 0 || level > 10000) {
		HISI_FB_ERR("Need Valid Data! level = %d", level);
		return ret;
	}

	for (idx = 0; idx <= BL_LVL_MAP_SIZE; idx++) {
		if (level_map[idx] >= level) {
			if (level_map[idx] > level) {
				idx = idx - 1;
			}
			break;
		}
	}

	return idx;
}

void hisi_blpwm_bl_regisiter(int (*set_bl)(int bl_level))
{
	g_bl_info.set_common_backlight = set_bl;
}
int hisi_blpwm_bl_callback(int bl_level)
{
	if(g_bl_info.set_common_backlight)
	{
		return g_bl_info.set_common_backlight(bl_level);
	}
	return -1;
}

static void init_bl_info(struct hisi_panel_info *pinfo)
{
	g_bl_info.bl_max = pinfo->bl_max;
	g_bl_info.bl_min = pinfo->bl_min;
	g_bl_info.ap_brightness = 0;
	g_bl_info.last_ap_brightness = 0;

	if (pinfo->blpwm_input_precision == 0)
		pinfo->blpwm_input_precision = PWM_IN_PRECISION_DEFAULT;

	rgbw_lcd_support = pinfo->rgbw_support;
	if (rgbw_lcd_support) {
		pinfo->blpwm_input_precision = PWM_IN_PRECISION_RGBW;
	}
	pinfo->blpwm_in_num = 0xFFFF0000;
	g_bl_info.blpwm_input_precision = pinfo->blpwm_input_precision;
	g_bl_info.index_cabc_dimming = 0;

	g_bl_info.last_bl_level =0;
	if (pinfo->blpwm_precision_type == BLPWM_PRECISION_10000_TYPE) {
		g_bl_info.blpwm_out_precision = BLPWM_OUT_PRECISION;
		outp32(hisifd_blpwm_base + BLPWM_OUT_DIV, 0);
	} else if (pinfo->blpwm_precision_type == BLPWM_PRECISION_2048_TYPE) {
		g_bl_info.blpwm_out_precision = BLPWM_OUT_PRECISION_2048;
		outp32(hisifd_blpwm_base + BLPWM_OUT_DIV, 0x2);
	} else if (pinfo->blpwm_precision_type == BLPWM_PRECISION_4096_TYPE) {
		g_bl_info.blpwm_out_precision = BLPWM_OUT_PRECISION_4096;
		outp32(hisifd_blpwm_base + BLPWM_OUT_DIV, 0x1);
	} else {
		g_bl_info.blpwm_out_precision = BLPWM_OUT_PRECISION_DEFAULT;
		outp32(hisifd_blpwm_base + BLPWM_OUT_DIV, 0x2);
	}
	if (pinfo->blpwm_out_div_value) {
		outp32(hisifd_blpwm_base + BLPWM_OUT_DIV, pinfo->blpwm_out_div_value);
	}
	g_bl_info.cabc_pwm = g_bl_info.blpwm_input_precision;
	g_bl_info.prev_cabc_pwm = g_bl_info.blpwm_input_precision;
	g_bl_info.current_cabc_pwm = g_bl_info.blpwm_input_precision;
	g_bl_info.cabc_pwm_in = g_bl_info.blpwm_input_precision;
	sema_init(&g_bl_info.bl_semaphore, 1);
	g_bl_info.bl_ic_ctrl_mode = pinfo->bl_ic_ctrl_mode;
	return ;
}

static void update_backlight(uint32_t backlight)
{
	char __iomem *blpwm_base = NULL;
	uint32_t brightness = 0;
	uint32_t bl_level = (backlight * g_bl_info.blpwm_out_precision) / g_bl_info.bl_max;
	HISI_FB_DEBUG("cabc8:bl_level=%d, backlight=%d, blpwm_out_precision=%d, bl_max=%d\n",
			bl_level, backlight, g_bl_info.blpwm_out_precision, g_bl_info.bl_max);
	blpwm_base = hisifd_blpwm_base;
	if (!blpwm_base) {
		HISI_FB_ERR("blpwm_base is null!\n");
		return;
	}

	if ((g_bl_info.bl_ic_ctrl_mode >= REG_ONLY_MODE ) && (g_bl_info.bl_ic_ctrl_mode <= I2C_ONLY_MODE)) {
		bl_level = backlight;
		bl_level = bl_lvl_map(bl_level);
		HISI_FB_DEBUG("cabc9:bl_level=%d\n",bl_level);
		/* lm36923_ramp_brightness(bl_level); */
		if (REG_ONLY_MODE == g_bl_info.bl_ic_ctrl_mode) {
			lm36923_set_backlight_reg(bl_level);
		} else if (I2C_ONLY_MODE == g_bl_info.bl_ic_ctrl_mode) {
			lm36274_set_backlight_reg(bl_level);
		}
		return;
	}else if(g_bl_info.bl_ic_ctrl_mode == COMMON_IC_MODE) {
		int return_value = -1;
		bl_level = backlight;
		bl_level = bl_lvl_map(bl_level);
		return_value = hisi_blpwm_bl_callback(bl_level);
		if(0 == return_value)
			return;
	}

	brightness = (bl_level << 16) | (g_bl_info.blpwm_out_precision - bl_level);
	outp32(blpwm_base + BLPWM_OUT_CFG, brightness);
}

static uint32_t get_pwm_duty(struct hisi_fb_data_type *hisifd){
	char __iomem *blpwm_base = NULL;

	uint32_t Continue_pwm_in_num = 0;
	uint32_t Continue_pwm_in_high_num = 0;
	uint32_t Continue_pwm_in_low_num  = 0;
	uint32_t Continue_pwm_in_duty = 0;
	uint32_t default_duty = MIN_PWMDUTY_FOR_RGBW;

	blpwm_base = hisifd_blpwm_base;

	if (pwmduty[PWMDUTY_FILTER_NUMBER-1] != 0xffff) {
		default_duty = (pwmduty[PWMDUTY_FILTER_NUMBER-1] >= MIN_PWMDUTY_FOR_RGBW) ? pwmduty[PWMDUTY_FILTER_NUMBER-1]:MIN_PWMDUTY_FOR_RGBW;
	}

	if (!blpwm_base) {
		HISI_FB_ERR("blpwm_base is null!\n");
		return default_duty;
	}

	Continue_pwm_in_num = inp32(blpwm_base + PWM_IN_NUM_OFFSET);
	if (Continue_pwm_in_num <= 0) {
		HISI_FB_DEBUG("pwm_in_num is null!\n");
		return default_duty;
	}

	Continue_pwm_in_high_num = Continue_pwm_in_num >> 16;
	Continue_pwm_in_low_num  = Continue_pwm_in_num & 0xFFFF;

	if (((Continue_pwm_in_high_num + Continue_pwm_in_low_num) < PWM_PERIOD_TOTAL_COUNTOR_JDI_SHARP) && (hisifd->de_info.ddic_panel_id == 1 ||hisifd->de_info.ddic_panel_id == 3) ) {
		HISI_FB_DEBUG("pwm_in_num is abnormal!\n");
		return default_duty;
	}
	if (((Continue_pwm_in_high_num + Continue_pwm_in_low_num) < PWM_PERIOD_TOTAL_COUNTOR_LGD) && (hisifd->de_info.ddic_panel_id == 2) ) {
		HISI_FB_DEBUG("pwm_in_num is abnormal!\n");
		return default_duty;
	}

	Continue_pwm_in_duty = g_bl_info.blpwm_input_precision * Continue_pwm_in_high_num / (Continue_pwm_in_high_num + Continue_pwm_in_low_num);

	if (Continue_pwm_in_duty < MIN_PWMDUTY_FOR_RGBW) {
		Continue_pwm_in_duty = MIN_PWMDUTY_FOR_RGBW;
	}

	return Continue_pwm_in_duty;
}

static void get_ap_dimming_to_update_backlight(void)
{
	int32_t delta_cabc_pwm = 0;
	int32_t pwm_duty = 0;
	int32_t backlight = 0;

	HISI_FB_DEBUG("cabc3:jump while\n");
    if (g_bl_info.index_cabc_dimming > CABC_DIMMING_STEP_TOTAL_NUM) {
            HISI_FB_DEBUG("cabc4:dimming 32 time\n");
            set_current_state(TASK_INTERRUPTIBLE);
            schedule();
            g_bl_info.index_cabc_dimming = 1;
    } else {
            down(&g_bl_info.bl_semaphore);
            HISI_FB_DEBUG("cabc5:dimming time=%d, g_bl_info.cabc_pwm_in=%d\n", g_bl_info.index_cabc_dimming, g_bl_info.cabc_pwm_in);
            if (g_bl_info.cabc_pwm_in != 0) {
                    g_bl_info.cabc_pwm = g_bl_info.cabc_pwm_in;
                    g_bl_info.cabc_pwm_in = 0;
                    g_bl_info.index_cabc_dimming = 1;
                    g_bl_info.prev_cabc_pwm = g_bl_info.current_cabc_pwm;
                    HISI_FB_DEBUG("cabc6:cabc_pwm=%d, cabc_pwm_in=%d, prev_cabc_pwm=%d\n",
                            g_bl_info.cabc_pwm, g_bl_info.cabc_pwm_in, g_bl_info.prev_cabc_pwm);
            }
            delta_cabc_pwm = g_bl_info.cabc_pwm - g_bl_info.prev_cabc_pwm;
            pwm_duty = delta_cabc_pwm*g_bl_info.index_cabc_dimming/32 + delta_cabc_pwm *g_bl_info.index_cabc_dimming % 32 /16;

            g_bl_info.current_cabc_pwm = g_bl_info.prev_cabc_pwm + pwm_duty;
            backlight = g_bl_info.current_cabc_pwm * g_bl_info.ap_brightness / g_bl_info.blpwm_input_precision;
            if (backlight > 0 && backlight < g_bl_info.bl_min) {
                    backlight = g_bl_info.bl_min;
            }
            HISI_FB_DEBUG("cabc7:g_bl_info.ap_brightness=%d, g_bl_info.last_bl_level=%d, backlight=%d,\n\
                            g_bl_info.current_cabc_pwm=%d, pwm_duty=%d, g_bl_info.cabc_pwm=%d, g_bl_info.prev_cabc_pwm=%d, delta_cabc_pwm=%d,\n",
                            g_bl_info.ap_brightness, g_bl_info.last_bl_level, backlight, g_bl_info.current_cabc_pwm, pwm_duty, g_bl_info.cabc_pwm,
                            g_bl_info.prev_cabc_pwm, delta_cabc_pwm);
            if (g_bl_info.ap_brightness != 0 && backlight != g_bl_info.last_bl_level) {
                    update_backlight(backlight);
                    g_bl_info.last_bl_level = backlight;
            }

            g_bl_info.index_cabc_dimming++;
            up(&g_bl_info.bl_semaphore);
            msleep(16);
    }
}

static int is_pwm_dimming_stop(int pwm_duty)
{
	int i;
	int delta_pwmduty = 0;
	int dimming_stop_counter = 0;
	static int dimming_stop_flag = 0;
	if (rgbw_bl_stop_threshold_num < RGBW_BL_STOP_THRESHOLD){
		return 0;
	}

	for (i = 0; i < PWMDUTY_FILTER_NUMBER; i++) {
		if (i < (PWMDUTY_FILTER_NUMBER-1)) {
			pwmduty[i] = pwmduty[i+1];//lint !e679
		}
		if (i == (PWMDUTY_FILTER_NUMBER-1)) {
			pwmduty[i] = pwm_duty;
		}
	}

	for (i = 0; i < (PWMDUTY_FILTER_NUMBER-1); i++) {
		delta_pwmduty = pwmduty[i+1] - pwmduty[i];//lint !e679
		if (delta_pwmduty <= PWM_IN_SENSITY && delta_pwmduty >= (-PWM_IN_SENSITY)) {
			dimming_stop_counter ++;
		}
	}

	dimming_stop_flag = (dimming_stop_counter == (PWMDUTY_FILTER_NUMBER-1)) ? 1:0;
	 if (1==dimming_stop_flag){
		rgbw_bl_stop_threshold_num = 0;
	}
	return dimming_stop_flag;
}

static void reset_pwm_buf(uint32_t value) {
	int i;
	for (i = 0; i < PWMDUTY_FILTER_NUMBER; i++) {
		pwmduty[i] = value;
	}
	for (i = 0; i < BACKLIGHT_FILTER_NUMBER; i++) {
		backlight_buf[i] = value;
	}
}

static int calc_backlight(struct hisi_fb_data_type *hisifd, int32_t pwm_duty, int32_t is_rgbw_abnormal) {
	int32_t backlight = 0;
	int32_t delta_pwm_duty = 0;
	int32_t delta_pwm_duty_abs = 0;
	delta_pwm_duty = pwm_duty - last_pwm_duty;
	delta_pwm_duty_abs = abs(delta_pwm_duty);

	if ((delta_pwm_duty_abs <= 1) || ((delta_pwm_duty_abs == 3) && (last_delta_pwm_duty_abs==1))) {
		g_bl_info.cabc_pwm_in = last_pwm_duty;
	} else {
		if (pwm_duty > last_pwm_duty) {
			last_pwm_duty =  pwm_duty -1;
		} else  {
			last_pwm_duty =  pwm_duty +1;
		}
		g_bl_info.cabc_pwm_in = last_pwm_duty;
	}
	last_delta_pwm_duty_abs = delta_pwm_duty_abs;

	if (g_bl_info.cabc_pwm_in > g_bl_info.blpwm_input_precision) {
		g_bl_info.cabc_pwm_in = g_bl_info.blpwm_input_precision;
	} else if(g_bl_info.cabc_pwm_in < 0) {
		g_bl_info.cabc_pwm_in = 0;
	}

	if ((g_bl_info.last_ap_brightness == g_bl_info.ap_brightness) && (g_bl_info.ap_brightness < RGBW_LOW_LIGHT_THRESHOLD)) {
       	low_light_count ++;
	} else {
		low_light_count = 0;    //clear low light count
	}

	if (low_light_count > RGBW_LOW_LIGHT_STOP) {
		backlight = last_backlight;
		HISI_FB_DEBUG("cabc_rgbw lock_backlight: backlight= %d\n", backlight);
	} else {
		if (hisifd->de_info.ddic_panel_id == 1 && !is_rgbw_abnormal) {
			backlight = ((int32_t)hisifd->panel_info.bl_max) * g_bl_info.cabc_pwm_in / g_bl_info.blpwm_input_precision; // for JDI mode 1,2,3,4
		} else if(hisifd->de_info.ddic_panel_id == 2 && !is_rgbw_abnormal) {
			backlight = g_bl_info.ap_brightness * g_bl_info.cabc_pwm_in / g_bl_info.blpwm_input_precision; //for LG
		} else if(hisifd->de_info.ddic_panel_id == 3 && !is_rgbw_abnormal) {
			backlight = ((int32_t)hisifd->panel_info.bl_max) * g_bl_info.cabc_pwm_in / g_bl_info.blpwm_input_precision; // for SHARP mode 1,2,3,4
		} else {
			backlight = g_bl_info.ap_brightness;
		}
		last_backlight = backlight;
	}
	return backlight;
}

static int get_smooth_backlight(int32_t backlight) {
	int i = 0,  j = 0, sum_backlight = 0;

	backlight_buf[f_count % BACKLIGHT_FILTER_NUMBER] = backlight;
	f_count ++;

	for (i = 0; i < BACKLIGHT_FILTER_NUMBER; i++) {
		if (backlight_buf[i] == 0xffff)
			continue;
		sum_backlight += backlight_buf[i];
		j++;
	}

	if (j != 0) {
		backlight = (sum_backlight + j / 2)  / j;
	}

	return backlight;
}

static int is_rgbw_bl_abnormal(uint32_t count) {

	if (count == last_rgbw_bl_count) {
		rgbw_bl_stop_num++;
		ap_backlight_delta_sum += (g_bl_info.ap_brightness - g_bl_info.last_ap_brightness);
	} else {
		rgbw_bl_stop_num = 0;
		ap_backlight_delta_sum = 0;
	}

	if (g_bl_info.last_ap_brightness != g_bl_info.ap_brightness) {
		backlight_update_counter++;
	} else {
		backlight_update_counter = 0;
	}

	if (rgbw_abnormal_flag == 0) {
		rgbw_abnormal_flag = (uint32_t)((rgbw_bl_stop_num > 120) && (backlight_update_counter > 1)
			&& ((ap_backlight_delta_sum > 30) || (ap_backlight_delta_sum < -30)));
	} else {
		rgbw_abnormal_flag = (uint32_t)(rgbw_bl_stop_num != 0);
	}

	last_rgbw_bl_count = count;

	HISI_FB_DEBUG("cabc_rgbw rgbw_bl_stop_num = %d, backlight_update_counter = %d\n", rgbw_bl_stop_num, backlight_update_counter);
	return rgbw_abnormal_flag;
}

static void get_rgbw_pwmduty_to_update_backlight(struct hisi_fb_data_type *hisifd)
{
	int32_t backlight = 0;
	int dimming_stop = 0;
	int temp_current_pwm_duty = 0;
	int is_rgbw_abnormal = 0;

	if ((g_bl_info.last_ap_brightness == 0 && g_bl_info.ap_brightness != 0)) {
		reset_pwm_buf(0xffff);
		last_pwm_duty = MIN_PWMDUTY_FOR_RGBW;
		last_delta_pwm_duty_abs = 0;
	}

	if (g_bl_info.ap_brightness != 0) {
		temp_current_pwm_duty = get_pwm_duty(hisifd);
	}
	rgbw_bl_stop_threshold_num++;
	dimming_stop = is_pwm_dimming_stop(temp_current_pwm_duty);

	HISI_FB_DEBUG("cabc_rgbw temp_current_pwm_duty = %d ", temp_current_pwm_duty);

	if((g_bl_info.last_ap_brightness == g_bl_info.ap_brightness) && (dimming_stop == 1)) {
		down(&g_bl_info.bl_semaphore);
		HISI_FB_DEBUG("cabc_rgbw :(stop!!!) last_ap_brightness =%d  ap_brightness = %d current_duty =%d  prev_duty = %d g_blpwm_thread_stop = %d\n",g_bl_info.last_ap_brightness,g_bl_info.ap_brightness,temp_current_pwm_duty,g_bl_info.cabc_pwm_in,g_blpwm_thread_stop);
		if (g_blpwm_thread_stop) {
			msleep(5);
			up(&g_bl_info.bl_semaphore);
			return ;
		}
		set_current_state(TASK_INTERRUPTIBLE);//lint !e446  !e666
		up(&g_bl_info.bl_semaphore);
		schedule();
	} else {
		down(&g_bl_info.bl_semaphore);

		is_rgbw_abnormal = is_rgbw_bl_abnormal(hisifd->rgbw_bl_count);

		backlight = calc_backlight(hisifd, temp_current_pwm_duty, is_rgbw_abnormal);
		HISI_FB_DEBUG("cabc_rgbw backlight = %d is_rgbw_abnormal = %d", backlight, is_rgbw_abnormal);

		//smooth filter for backlight
		backlight = get_smooth_backlight(backlight);

		HISI_FB_DEBUG("cabc_rgbw  panel_id =%d last_ap_brightness =%d ap_brightness =%d current_duty =%d temp_duty =%d backlight =%d \n",
				hisifd->de_info.ddic_panel_id,g_bl_info.last_ap_brightness,g_bl_info.ap_brightness,g_bl_info.cabc_pwm_in,temp_current_pwm_duty,backlight);
		if (backlight > 0 && backlight < g_bl_info.bl_min) {
			backlight = g_bl_info.bl_min;
		}

		g_bl_info.last_ap_brightness = g_bl_info.ap_brightness;

		if (g_bl_info.ap_brightness != 0 && backlight != g_bl_info.last_bl_level) {
			update_backlight(backlight);
			g_bl_info.last_bl_level = backlight;
		}
		msleep(12);
		up(&g_bl_info.bl_semaphore);
	}

	if (g_bl_info.ap_brightness == 0) {
		reset_pwm_buf(0xffff);
	}
}

static int cabc_pwm_thread(void *p)
{

	struct hisi_fb_data_type *hisifd = NULL;
	hisifd = (struct hisi_fb_data_type *)p;

	while(!kthread_should_stop()) {
		if (rgbw_lcd_support)
		{
			get_rgbw_pwmduty_to_update_backlight(hisifd);
		}
	    else
		{
			get_ap_dimming_to_update_backlight();
		}
	}
	return 0;
}

int hisi_cabc_set_backlight(uint32_t cabc_pwm_in)
{
	HISI_FB_DEBUG("cabc2:cabc_pwm_in=%d,g_bl_info.ap_brightness=%d,if null=%d\n",
			cabc_pwm_in, g_bl_info.ap_brightness, cabc_pwm_task == NULL);
	if (cabc_pwm_task == NULL) {
		return 0;
	}

	if (g_bl_info.ap_brightness == 0) {
	     g_bl_info.current_cabc_pwm = cabc_pwm_in;
	     return 0;
	}
	if (!rgbw_lcd_support) {
		g_bl_info.cabc_pwm_in = cabc_pwm_in;
	}

	wake_up_process(cabc_pwm_task);
	return 0;
}

static int hisi_blpwm_input_enable(struct hisi_fb_data_type *hisifd)
{
	char __iomem *blpwm_base = NULL;
	struct hisi_panel_info *pinfo = NULL;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return -EINVAL;
	}
	pinfo = &(hisifd->panel_info);

	blpwm_base = hisifd_blpwm_base;
	if (!blpwm_base) {
		HISI_FB_ERR("blpwm_base is null!\n");
		return -EINVAL;
	}

	outp32(blpwm_base + PWM_IN_CTRL_OFFSET, 1);
	if (rgbw_lcd_support) {
		outp32(blpwm_base + PWM_IN_DIV_OFFSET, 0x01);
		outp32(blpwm_base + PWM_IN_MAX_COUNTER, PWM_MAX_COUNTER_FOR_RGBW_JDI_SHARP);
	} else {
		outp32(blpwm_base + PWM_IN_DIV_OFFSET, 0x02);
	}
	cabc_pwm_task = kthread_create(cabc_pwm_thread, hisifd, "cabc_pwm_task");
	if(IS_ERR(cabc_pwm_task)) {
		HISI_FB_ERR("Unable to start kernel cabc_pwm_task./n");
		cabc_pwm_task = NULL;
		return -EINVAL;
	}
	if (rgbw_lcd_support) {
		g_blpwm_thread_stop = false;
	}
	return 0;
}

static int hisi_blpwm_input_disable(struct hisi_fb_data_type *hisifd)
{
	char __iomem *blpwm_base = NULL;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return -EINVAL;
	}

	if (cabc_pwm_task) {
		if (rgbw_lcd_support) {
			down(&g_bl_info.bl_semaphore);
			g_blpwm_thread_stop = true;
			up(&g_bl_info.bl_semaphore);
		}
		kthread_stop(cabc_pwm_task);
		cabc_pwm_task = NULL;
	}

	blpwm_base = hisifd_blpwm_base;
	if (!blpwm_base) {
		HISI_FB_ERR("blpwm_base is null!\n");
		return -EINVAL;
	}

	outp32(blpwm_base + PWM_IN_CTRL_OFFSET, 0);

	return 0;
}

int updateCabcPwm(struct hisi_fb_data_type *hisifd)
{
	char __iomem *blpwm_base = NULL;
	struct hisi_panel_info *pinfo = NULL;
	uint32_t pwm_in_num	= 0;
	uint32_t pwm_in_high_num = 0;
	uint32_t pwm_in_low_num  = 0;
	uint32_t pwm_in_duty = 0;
	uint32_t pre_pwm_in_num	= 0;
	uint32_t pre_pwm_in_high_num = 0;
	uint32_t pre_pwm_in_low_num  = 0;
	uint32_t pre_pwm_in_duty = 0;
	int delta_duty = 0;
	bool hiace_refresh = false;
	uint32_t cabc_is_open = 0;
	int ret = 0;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return -EINVAL;
	}
	pinfo = &(hisifd->panel_info);

	blpwm_base = hisifd_blpwm_base;
	if (!blpwm_base) {
		HISI_FB_ERR("blpwm_base is null!\n");
		return -EINVAL;
	}

	if (!g_blpwm_on || !hisifd->bl_level || !hisifd->backlight.bl_updated)
		return 0;


	hiace_refresh = hisifb_display_effect_fine_tune_backlight(hisifd, (int)hisifd->bl_level, &g_bl_info.ap_brightness); //lint !e838

	if (pinfo->blpwm_input_ena) {
		pwm_in_num = inp32(blpwm_base + PWM_IN_NUM_OFFSET);
		if (pwm_in_num <= 0) {
			HISI_FB_DEBUG("pwm_in_num is null!\n");
			ret = -EINVAL;
			goto err_out;

		}
		pwm_in_high_num = pwm_in_num >> 16;
		pwm_in_low_num  = pwm_in_num & 0xFFFF;
		if ((!rgbw_lcd_support) && (pwm_in_high_num < pwm_in_low_num)) {//high duty must larger than low duty
			HISI_FB_DEBUG("high duty should not be smaller than low duty !\n");
			ret = -EINVAL;
			goto err_out;
		}

		pwm_in_duty = pinfo->blpwm_input_precision * pwm_in_high_num / (pwm_in_high_num + pwm_in_low_num);

		HISI_FB_DEBUG("cabc0:pwm_in_numall=%d, pwm_in_high_num=%d, pwm_in_low_num=%d\n",
					pwm_in_high_num + pwm_in_low_num, pwm_in_high_num, pwm_in_low_num);
		pre_pwm_in_num = pinfo->blpwm_in_num;
		pre_pwm_in_high_num = pre_pwm_in_num >> 16;
		pre_pwm_in_low_num  = pre_pwm_in_num & 0xFFFF;
		pre_pwm_in_duty = pinfo->blpwm_input_precision * pre_pwm_in_high_num / (pre_pwm_in_high_num + pre_pwm_in_low_num);

		cabc_is_open = 1;

		delta_duty = pwm_in_duty - pre_pwm_in_duty;
		HISI_FB_DEBUG("cabc1:Previous pwm in duty:%d, Current pwm in duty:%d, delta_duty:%d,pwm_in_num=%d\n",
				pre_pwm_in_duty, pwm_in_duty, delta_duty, pwm_in_num);

		if (rgbw_lcd_support || hiace_refresh || (delta_duty > PWM_IN_SENSITY || delta_duty < (-PWM_IN_SENSITY))) {
			pinfo->blpwm_in_num = pwm_in_num;
			hisi_cabc_set_backlight(pwm_in_duty);
		}
	}

err_out:
	if (cabc_is_open == 0 && hiace_refresh) {
		update_backlight(g_bl_info.ap_brightness);
	}

	return ret;

}

int hisi_blpwm_set_backlight(struct hisi_fb_data_type *hisifd, uint32_t bl_level)
{
	struct hisi_panel_info *pinfo = NULL;
	char __iomem *blpwm_base = NULL;
	uint32_t brightness = 0;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return -EINVAL;
	}
	pinfo = &(hisifd->panel_info);

	blpwm_base = hisifd_blpwm_base;
	if (!blpwm_base) {
		HISI_FB_ERR("blpwm_base is null!\n");
		return -EINVAL;
	}

	if (g_blpwm_on == 0) {
		HISI_FB_ERR("blpwm is not on, return!\n");
		return 0;
	}

	HISI_FB_DEBUG("fb%d, bl_level=%d.\n", hisifd->index, bl_level);

	if (pinfo->bl_max < 1) {
		HISI_FB_ERR("bl_max(%d) is out of range!!", pinfo->bl_max);
		return -EINVAL;
	}

	if (bl_level > pinfo->bl_max) {
		bl_level = pinfo->bl_max;
	}

	if (rgbw_lcd_support) {
		g_bl_info.last_ap_brightness = g_bl_info.ap_brightness;
	}
	//allow backlight zero
	if (bl_level < pinfo->bl_min && bl_level) {
		bl_level = pinfo->bl_min;
	}

	HISI_FB_DEBUG("cabc:fb%d, bl_level=%d, blpwm_input_ena=%d, blpwm_in_num=%d\n",
					hisifd->index, bl_level, pinfo->blpwm_input_ena, pinfo->blpwm_in_num);
	down(&g_bl_info.bl_semaphore);

	hisifb_display_effect_fine_tune_backlight(hisifd, (int)bl_level, (int *)&bl_level);

	g_bl_info.ap_brightness = bl_level;

	if (bl_level && rgbw_lcd_support && pinfo->blpwm_input_ena) {

		if (g_bl_info.last_ap_brightness  != g_bl_info.ap_brightness) {
			hisi_cabc_set_backlight(g_bl_info.current_cabc_pwm);
		}

		up(&g_bl_info.bl_semaphore);
			return 0;
	}

	if (pinfo->blpwm_input_ena && pinfo->blpwm_in_num) {

		if(bl_level > 0){
			bl_level= bl_level * g_bl_info.current_cabc_pwm / pinfo->blpwm_input_precision;
			bl_level =  bl_level < g_bl_info.bl_min ? g_bl_info.bl_min : bl_level ;
		}
		g_bl_info.last_bl_level = bl_level;
		HISI_FB_DEBUG("cabc:ap_brightness=%d, current_cabc_pwm=%d, blpwm_input_precision=%d, bl_level=%d\n",
				g_bl_info.ap_brightness, g_bl_info.current_cabc_pwm,
				pinfo->blpwm_input_precision, bl_level);
	}

	if ((g_bl_info.bl_ic_ctrl_mode >= REG_ONLY_MODE ) && (g_bl_info.bl_ic_ctrl_mode <= I2C_ONLY_MODE)) {
		bl_level = bl_lvl_map(bl_level);
		HISI_FB_DEBUG("cabc:bl_level=%d\n",bl_level);
		/* lm36923_ramp_brightness(bl_level); */
		if (REG_ONLY_MODE == pinfo->bl_ic_ctrl_mode) {
			lm36923_set_backlight_reg(bl_level);
		} else if (I2C_ONLY_MODE == pinfo->bl_ic_ctrl_mode) {
			lm36274_set_backlight_reg(bl_level);
		}
		up(&g_bl_info.bl_semaphore);
		return 0;
	} else if (BLPWM_AND_CABC_MODE == g_bl_info.bl_ic_ctrl_mode) {
		lp8556_set_backlight_init(bl_level);
	}else if (COMMON_IC_MODE == g_bl_info.bl_ic_ctrl_mode) {
		int return_value = -1;

		bl_level = bl_lvl_map(bl_level);
		return_value = hisi_blpwm_bl_callback(bl_level);
		if(0 == return_value)
		{
			up(&g_bl_info.bl_semaphore);
			return 0;
		}
	}

	bl_level = (bl_level * g_bl_info.blpwm_out_precision) / pinfo->bl_max;

	brightness = (bl_level << 16) | (g_bl_info.blpwm_out_precision - bl_level);
	outp32(blpwm_base + BLPWM_OUT_CFG, brightness);
	HISI_FB_DEBUG("cabc:ap_brightness=%d, current_cabc_pwm=%d, blpwm_input_precision=%d, \
				blpwm_out_precision=%d, bl_level=%d,\
				brightness=%d\n", g_bl_info.ap_brightness, g_bl_info.current_cabc_pwm,
				pinfo->blpwm_input_precision, g_bl_info.blpwm_out_precision, bl_level, brightness);
	up(&g_bl_info.bl_semaphore);
	return 0;
}

int hisi_blpwm_on(struct platform_device *pdev)
{
	int ret = 0;
	struct clk *clk_tmp = NULL;
	char __iomem *blpwm_base = NULL;
	struct hisi_fb_data_type *hisifd = NULL;
	struct hisi_panel_info *pinfo = NULL;

	if (NULL == pdev) {
		HISI_FB_ERR("pdev is NULL");
		return -EINVAL;
	}
	hisifd = platform_get_drvdata(pdev);
	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return -EINVAL;
	}

	pinfo = &(hisifd->panel_info);

	blpwm_base = hisifd_blpwm_base;
	if (!blpwm_base) {
		HISI_FB_ERR("blpwm_base is null!\n");
		return -EINVAL;
	}

	if (g_blpwm_on == 1)
		return 0;

	clk_tmp = g_dss_blpwm_clk;
	if (clk_tmp) {
		ret = clk_prepare(clk_tmp);
		if (ret) {
			HISI_FB_ERR("dss_blpwm_clk clk_prepare failed, error=%d!\n", ret);
			return -EINVAL;
		}

		ret = clk_enable(clk_tmp);
		if (ret) {
			HISI_FB_ERR("dss_blpwm_clk clk_enable failed, error=%d!\n", ret);
			return -EINVAL;
		}
	}

	ret = pinctrl_cmds_tx(g_blpwm_pdev, blpwm_pinctrl_normal_cmds,
		ARRAY_SIZE(blpwm_pinctrl_normal_cmds));

	//if enable BLPWM, please set IOMG_003, IOMG_004 in IOC_AO module
	// set IOMG_003: select BLPWM_CABC
	// set IOMG_004: select BLPWM_BL

	outp32(blpwm_base + BLPWM_OUT_CTRL, 0x1);
	init_bl_info(pinfo);
	if (pinfo->blpwm_input_ena) {
		hisi_blpwm_input_enable(hisifd);
	}

	g_blpwm_on = 1;

	return ret;
}

int hisi_blpwm_off(struct platform_device *pdev)
{
	int ret = 0;
	struct clk *clk_tmp = NULL;
	char __iomem *blpwm_base = NULL;
	struct hisi_fb_data_type *hisifd = NULL;
	struct hisi_panel_info *pinfo = NULL;

	if (NULL == pdev) {
		HISI_FB_ERR("pdev is NULL");
		return -EINVAL;
	}
	hisifd = platform_get_drvdata(pdev);
	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return -EINVAL;
	}

	pinfo = &(hisifd->panel_info);

	blpwm_base = hisifd_blpwm_base;
	if (!blpwm_base) {
		HISI_FB_ERR("blpwm_base is null!\n");
		return -EINVAL;
	}

	if (g_blpwm_on == 0)
		return 0;

	outp32(blpwm_base + BLPWM_OUT_CTRL, 0x0);

	ret = pinctrl_cmds_tx(g_blpwm_pdev, blpwm_pinctrl_lowpower_cmds,
		ARRAY_SIZE(blpwm_pinctrl_lowpower_cmds));

	clk_tmp = g_dss_blpwm_clk;
	if (clk_tmp) {
		clk_disable(clk_tmp);
		clk_unprepare(clk_tmp);
	}

	if (pinfo->blpwm_input_ena) {
		hisi_blpwm_input_disable(hisifd);
	}

	g_blpwm_on = 0;

	return ret;
}

static int hisi_blpwm_probe(struct platform_device *pdev)
{
	struct device_node *np = NULL;
	int ret = 0;

	HISI_FB_DEBUG("+.\n");

	if (NULL == pdev) {
		HISI_FB_ERR("pdev is NULL");
		return -EINVAL;
	}

	g_blpwm_pdev = pdev;

	np = of_find_compatible_node(NULL, NULL, DTS_COMP_BLPWM_NAME);
	if (!np) {
		HISI_FB_ERR("NOT FOUND device node %s!\n", DTS_COMP_BLPWM_NAME);
		ret = -ENXIO;
		goto err_return;
	}

	/* get blpwm reg base */
	hisifd_blpwm_base = of_iomap(np, 0);
	if (!hisifd_blpwm_base) {
		HISI_FB_ERR("failed to get blpwm_base resource.\n");
		ret = -ENXIO;
		goto err_return;
	}

	ret = of_property_read_u32(np, "fpga_flag", &g_blpwm_fpga_flag);
	if (ret) {
		HISI_FB_ERR("failed to get fpga_flag resource.\n");
		ret = -ENXIO;
		goto err_return;
	}

	if (g_blpwm_fpga_flag == 0) {
		/* blpwm pinctrl init */
		ret = pinctrl_cmds_tx(pdev, blpwm_pinctrl_init_cmds,
				ARRAY_SIZE(blpwm_pinctrl_init_cmds));
		if (ret != 0) {
			HISI_FB_ERR("Init blpwm pinctrl failed! ret=%d.\n", ret);
			goto err_return;
		}

		/* get blpwm clk resource */
		g_dss_blpwm_clk = of_clk_get(np, 0);
		if (IS_ERR(g_dss_blpwm_clk)) {
			HISI_FB_ERR("%s clock not found: %d!\n",
					np->name, (int)PTR_ERR(g_dss_blpwm_clk));
			ret = -ENXIO;
			goto err_return;
		}


		HISI_FB_INFO("dss_blpwm_clk:[%lu]->[%lu].\n",
				DEFAULT_PWM_CLK_RATE, clk_get_rate(g_dss_blpwm_clk));
	}

	hisi_fb_device_set_status0(DTS_PWM_READY);

	HISI_FB_DEBUG("-.\n");

	return 0;

err_return:
	return ret;
}

static int hisi_blpwm_remove(struct platform_device *pdev)
{
	struct clk *clk_tmp = NULL;
	int ret = 0;

	ret = pinctrl_cmds_tx(pdev, blpwm_pinctrl_finit_cmds,
		ARRAY_SIZE(blpwm_pinctrl_finit_cmds));

	clk_tmp = g_dss_blpwm_clk;
	if (clk_tmp) {
		clk_put(clk_tmp);
		clk_tmp = NULL;
	}

	return ret;
}

static const struct of_device_id hisi_blpwm_match_table[] = {
	{
		.compatible = DTS_COMP_BLPWM_NAME,
		.data = NULL,
	},
	{},
};
MODULE_DEVICE_TABLE(of, hisi_blpwm_match_table);

static struct platform_driver this_driver = {
	.probe = hisi_blpwm_probe,
	.remove = hisi_blpwm_remove,
	.suspend = NULL,
	.resume = NULL,
	.shutdown = NULL,
	.driver = {
		.name = DEV_NAME_BLPWM,
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(hisi_blpwm_match_table),
	},
};

static int __init hisi_blpwm_init(void)
{
	int ret = 0;

	ret = platform_driver_register(&this_driver);
	if (ret) {
		HISI_FB_ERR("platform_driver_register failed, error=%d!\n", ret);
		return ret;
	}

	return ret;
}

module_init(hisi_blpwm_init);
