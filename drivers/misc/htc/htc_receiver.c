/* Copyright (c) 2014, HTC Corporation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#include <linux/of.h>
#include <mach/mt_gpio.h>

#define AUD_RECEIVER_NAME		"/htc_receiver"
#define AUD_BOARD_INFO_PATH    "/htc_aud_board_info"

typedef enum
{
    AUDIO_RECEIVER_MODE = 1,
    AUDIO_SPEAKER_MODE = 2,
} Select_Mode;

static int select_gpio = -1;
static int enable_gpio = -1;
static int headset_speaker_gpio = -1;
static int invert_gpio = -1;
static int rec_amp_gpio = -1;

static void setAmpPower(bool enable, int gpio) {
	unsigned long local_enable_gpio = 0;
	if (gpio < 0) {
		pr_err("setAmpPower Error, cannot find the gpio: %d, enable: %d \n ", gpio, enable);
		return;
	}
	local_enable_gpio = (gpio | 0x80000000);

	printk("eleven: setAmpPower %d, gpio %d \n ", enable, gpio);

	mt_set_gpio_mode(local_enable_gpio, GPIO_MODE_00);
	mt_set_gpio_dir(local_enable_gpio, GPIO_DIR_OUT);
	if (enable)
		mt_set_gpio_out(local_enable_gpio, GPIO_OUT_ONE);
	else
		mt_set_gpio_out(local_enable_gpio, GPIO_OUT_ZERO);
}

static void select(Select_Mode mode) {
	unsigned long local_select_gpio = 0;

	if (select_gpio < 0) {
		pr_err("select Error, cannot find the select gpio: %d, mode: %d \n ", select_gpio, mode);
		return;
	}

	printk("eleven: select  %d, gpio: %d\n ", mode, select_gpio);

	local_select_gpio = (select_gpio | 0x80000000);
	mt_set_gpio_mode(local_select_gpio, GPIO_MODE_00);
	mt_set_gpio_dir(local_select_gpio, GPIO_DIR_OUT);
	if (mode == AUDIO_SPEAKER_MODE)
		mt_set_gpio_out(local_select_gpio, GPIO_OUT_ZERO);
	else if (mode == AUDIO_RECEIVER_MODE && invert_gpio == 1)
		mt_set_gpio_out(local_select_gpio, GPIO_OUT_ZERO);
	else
		mt_set_gpio_out(local_select_gpio, GPIO_OUT_ONE);

}

void setReceiverSelect(bool enable) {
	setAmpPower(enable, rec_amp_gpio);

	if(enable)
		select(AUDIO_RECEIVER_MODE);
	else
		select(AUDIO_SPEAKER_MODE);
}

void setSpeakerSelect(bool enable) {
	setAmpPower(enable, enable_gpio);

	if(enable)
		select(AUDIO_SPEAKER_MODE);
}

void setHeadsetSpeaker(bool enable) {
	unsigned long local_headset_speaker_gpio = 0;
	if (headset_speaker_gpio < 0) {
		pr_err("setHeadsetSpeaker Error, cannot find the gpio, enable %d \n ", enable);
		return;
	}

	local_headset_speaker_gpio = (headset_speaker_gpio | 0x80000000);
	printk("eleven: setHeadsetSpeaker() enabel: %d, gpio: %d \n ", enable, headset_speaker_gpio);
	mt_set_gpio_mode(local_headset_speaker_gpio, GPIO_MODE_00);
	mt_set_gpio_dir(local_headset_speaker_gpio, GPIO_DIR_OUT);

	if (enable)
		mt_set_gpio_out(local_headset_speaker_gpio, GPIO_OUT_ONE);
	else
		mt_set_gpio_out(local_headset_speaker_gpio, GPIO_OUT_ZERO);

}

int htc_receiver_init(void) {
	struct device_node *receiver_node;
	struct property *prop;

	receiver_node = of_find_node_by_path(AUD_RECEIVER_NAME);
	prop = of_find_property(receiver_node, "htc_aud,select-gpio", NULL);
	if (prop) {
		of_property_read_u32(receiver_node, "htc_aud,select-gpio", (u32*)&select_gpio);
		printk("%s: reciver_select = %d\n", __func__, select_gpio);
	} else {
		printk("%s: reciver_select not found", __func__);
		select_gpio = -1;
	}

	prop = of_find_property(receiver_node, "htc_aud,enable-gpio", NULL);
	if (prop) {
		of_property_read_u32(receiver_node, "htc_aud,enable-gpio", (u32*)&enable_gpio);
		printk("%s: reciver_enable = %d\n", __func__, enable_gpio);
	} else {
		printk("%s: reciver_enable not found", __func__);
		enable_gpio = -1;
	}

	prop = of_find_property(receiver_node, "htc_aud,headset-speaker-gpio", NULL);
	if (prop) {
		of_property_read_u32(receiver_node, "htc_aud,headset-speaker-gpio", (u32*)&headset_speaker_gpio);
		printk("%s: headset_speaker_gpio = %d\n", __func__, headset_speaker_gpio);
	} else {
		printk("%s: headset_speaker_gpio not found", __func__);
		headset_speaker_gpio = -1;
	}

	prop = of_find_property(receiver_node, "htc_aud,invert-gpio", NULL);
	if (prop) {
		of_property_read_u32(receiver_node, "htc_aud,invert-gpio", (u32*)&invert_gpio);
		printk("%s: invert_gpio = %d\n", __func__, invert_gpio);
	} else {
		printk("%s: invert_gpio not found", __func__);
		invert_gpio = -1;
	}

	prop = of_find_property(receiver_node, "htc_aud,rec-amp-gpio", NULL);
	if (prop) {
		of_property_read_u32(receiver_node, "htc_aud,rec-amp-gpio", (u32*)&rec_amp_gpio);
		printk("%s: rec_amp_gpio = %d\n", __func__, rec_amp_gpio);
	} else {
		printk("%s: rec_amp_gpio not found", __func__);
		rec_amp_gpio = -1;
	}

	return 0;
}

void htc_receiver_release(void) {
	select_gpio = -1;
	enable_gpio = -1;
	headset_speaker_gpio = -1;
	invert_gpio = -1;
}
