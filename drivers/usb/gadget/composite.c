/*
 * composite.c - infrastructure for Composite USB Gadgets
 *
 * Copyright (C) 2006-2008 David Brownell
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */


#ifdef pr_fmt
#undef pr_fmt
#endif
#define pr_fmt(fmt) "["KBUILD_MODNAME"]" fmt

#include <linux/kallsyms.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/utsname.h>

#include <linux/usb/composite.h>
#include <asm/unaligned.h>

#include <linux/printk.h>

#define REQUEST_RESET_DELAYED (HZ / 10) 
static int usb_autobot_mode(void);
static void mtp_update_mode(int _mac_mtp_mode);
static void fsg_update_mode(int _linux_fsg_mode);
void usb_composite_force_reset(struct usb_composite_dev *cdev)
{
	unsigned long           flags;

	spin_lock_irqsave(&cdev->lock, flags);
	
	if (cdev && cdev->gadget && cdev->gadget->speed != USB_SPEED_UNKNOWN) {
		usb_gadget_disconnect(cdev->gadget);
		spin_unlock_irqrestore(&cdev->lock, flags);

		msleep(500);

		spin_lock_irqsave(&cdev->lock, flags);
		usb_gadget_connect(cdev->gadget);
		spin_unlock_irqrestore(&cdev->lock, flags);
	} else {
		spin_unlock_irqrestore(&cdev->lock, flags);
	}
}

static void composite_request_reset(struct work_struct *w)
{
	struct usb_composite_dev *cdev = container_of(
			(struct delayed_work *)w,
			struct usb_composite_dev, request_reset);
	if (cdev) {
		if (usb_autobot_mode())
			return;
		if (os_type == OS_LINUX && is_mtp_enable) {
			fsg_update_mode(1);
			mtp_update_mode(0);
			fsg_mode = 1;
		} else {
			fsg_update_mode(0);
			fsg_mode = 0;
			if (cdev->do_serial_number_change)
				cdev->do_serial_number_change = false;
			else
				return;
		}
		
		usb_composite_force_reset(cdev);
	}
}


static struct usb_gadget_strings **get_containers_gs(
		struct usb_gadget_string_container *uc)
{
	return (struct usb_gadget_strings **)uc->stash;
}

static struct usb_descriptor_header**
next_ep_desc(struct usb_descriptor_header **t)
{
	for (; *t; t++) {
		if ((*t)->bDescriptorType == USB_DT_ENDPOINT)
			return t;
	}
	return NULL;
}

#define for_each_ep_desc(start, ep_desc) \
	for (ep_desc = next_ep_desc(start); \
	      ep_desc; ep_desc = next_ep_desc(ep_desc+1))

int config_ep_by_speed(struct usb_gadget *g,
			struct usb_function *f,
			struct usb_ep *_ep)
{
	struct usb_composite_dev	*cdev = get_gadget_data(g);
	struct usb_endpoint_descriptor *chosen_desc = NULL;
	struct usb_descriptor_header **speed_desc = NULL;

	struct usb_ss_ep_comp_descriptor *comp_desc = NULL;
	int want_comp_desc = 0;

	struct usb_descriptor_header **d_spd; 

	if (!g || !f || !_ep)
		return -EIO;

	
	switch (g->speed) {
	case USB_SPEED_SUPER:
		if (gadget_is_superspeed(g)) {
			speed_desc = f->ss_descriptors;
			want_comp_desc = 1;
			break;
		}
		
	case USB_SPEED_HIGH:
		if (gadget_is_dualspeed(g)) {
			speed_desc = f->hs_descriptors;
			break;
		}
		
	default:
		speed_desc = f->fs_descriptors;
	}
	
	for_each_ep_desc(speed_desc, d_spd) {
		chosen_desc = (struct usb_endpoint_descriptor *)*d_spd;
		if (chosen_desc->bEndpointAddress == _ep->address)
			goto ep_found;
	}
	return -EIO;

ep_found:
	
	_ep->maxpacket = usb_endpoint_maxp(chosen_desc);
	_ep->desc = chosen_desc;
	_ep->comp_desc = NULL;
	_ep->maxburst = 0;
	_ep->mult = 0;
	if (!want_comp_desc)
		return 0;

	comp_desc = (struct usb_ss_ep_comp_descriptor *)*(++d_spd);
	if (!comp_desc ||
	    (comp_desc->bDescriptorType != USB_DT_SS_ENDPOINT_COMP))
		return -EIO;
	_ep->comp_desc = comp_desc;
	if (g->speed == USB_SPEED_SUPER) {
		switch (usb_endpoint_type(_ep->desc)) {
		case USB_ENDPOINT_XFER_ISOC:
			
			_ep->mult = comp_desc->bmAttributes & 0x3;
		case USB_ENDPOINT_XFER_BULK:
		case USB_ENDPOINT_XFER_INT:
			_ep->maxburst = comp_desc->bMaxBurst + 1;
			break;
		default:
			if (comp_desc->bMaxBurst != 0)
				ERROR(cdev, "ep0 bMaxBurst must be 0\n");
			_ep->maxburst = 1;
			break;
		}
	}
	return 0;
}
EXPORT_SYMBOL_GPL(config_ep_by_speed);

int usb_add_function(struct usb_configuration *config,
		struct usb_function *function)
{
	int	value = -EINVAL;
	
	pr_debug("[XLOG_DEBUG][USB][COM]%s: \n", __func__);

	INFO(config->cdev, "adding '%s'/%p to config '%s'/%p\n",
			function->name, function,
			config->label, config);

	if (!function->set_alt || !function->disable)
		goto done;

	function->config = config;
	list_add_tail(&function->list, &config->functions);

	
	if (function->bind) {
		value = function->bind(config, function);
		if (value < 0) {
			list_del(&function->list);
			function->config = NULL;
		}
	} else
		value = 0;

	if (!config->fullspeed && function->fs_descriptors)
		config->fullspeed = true;
	if (!config->highspeed && function->hs_descriptors)
		config->highspeed = true;
	if (!config->superspeed && function->ss_descriptors)
		config->superspeed = true;

done:
	if (value)
		INFO(config->cdev, "adding '%s'/%p --> %d\n",
				function->name, function, value);
	return value;
}
EXPORT_SYMBOL_GPL(usb_add_function);

void usb_remove_function(struct usb_configuration *c, struct usb_function *f)
{
	if (f->disable) {
		INFO(c->cdev, "disable function '%s'/%p\n", f->name, f);
		f->disable(f);
	}

	bitmap_zero(f->endpoints, 32);
	list_del(&f->list);
	if (f->unbind) {
		INFO(c->cdev, "unbind function '%s'/%p\n", f->name, f);
		f->unbind(c, f);
	}
}
EXPORT_SYMBOL_GPL(usb_remove_function);

int usb_function_deactivate(struct usb_function *function)
{
	struct usb_composite_dev	*cdev = function->config->cdev;
	unsigned long			flags;
	int				status = 0;

	spin_lock_irqsave(&cdev->lock, flags);

	if (cdev->deactivations == 0)
		status = usb_gadget_disconnect(cdev->gadget);
	if (status == 0)
		cdev->deactivations++;

	spin_unlock_irqrestore(&cdev->lock, flags);
	return status;
}
EXPORT_SYMBOL_GPL(usb_function_deactivate);

int usb_function_activate(struct usb_function *function)
{
	struct usb_composite_dev	*cdev = function->config->cdev;
	unsigned long			flags;
	int				status = 0;

	spin_lock_irqsave(&cdev->lock, flags);

	if (WARN_ON(cdev->deactivations == 0))
		status = -EINVAL;
	else {
		cdev->deactivations--;
		if (cdev->deactivations == 0)
			status = usb_gadget_connect(cdev->gadget);
	}

	spin_unlock_irqrestore(&cdev->lock, flags);
	return status;
}
EXPORT_SYMBOL_GPL(usb_function_activate);

int usb_interface_id(struct usb_configuration *config,
		struct usb_function *function)
{
	unsigned id = config->next_interface_id;

	if (id < MAX_CONFIG_INTERFACES) {
		config->interface[id] = function;
		config->next_interface_id = id + 1;
		return id;
	}
	return -ENODEV;
}
EXPORT_SYMBOL_GPL(usb_interface_id);

static u8 encode_bMaxPower(enum usb_device_speed speed,
		struct usb_configuration *c)
{
	unsigned val;

	if (c->MaxPower)
		val = c->MaxPower;
	else
		val = CONFIG_USB_GADGET_VBUS_DRAW;
	if (!val)
		return 0;
	switch (speed) {
	case USB_SPEED_SUPER:
		return DIV_ROUND_UP(val, 8);
	default:
		return DIV_ROUND_UP(val, 2);
	};
}

static int config_buf(struct usb_configuration *config,
		enum usb_device_speed speed, void *buf, u8 type)
{
	struct usb_config_descriptor	*c = buf;
	void				*next = buf + USB_DT_CONFIG_SIZE;
	int				len;
	struct usb_function		*f;
	int				status;

	len = USB_COMP_EP0_BUFSIZ - USB_DT_CONFIG_SIZE;
	
	c = buf;
	c->bLength = USB_DT_CONFIG_SIZE;
	c->bDescriptorType = type;
	/* wTotalLength is written later */
	c->bNumInterfaces = config->next_interface_id;
	c->bConfigurationValue = config->bConfigurationValue;
	c->iConfiguration = config->iConfiguration;
	c->bmAttributes = USB_CONFIG_ATT_ONE | config->bmAttributes;
	c->bMaxPower = encode_bMaxPower(speed, config);

	
	if (config->descriptors) {
		status = usb_descriptor_fillbuf(next, len,
				config->descriptors);
		if (status < 0)
			return status;
		len -= status;
		next += status;
	}

	
	list_for_each_entry(f, &config->functions, list) {
		struct usb_descriptor_header **descriptors;

		switch (speed) {
		case USB_SPEED_SUPER:
			descriptors = f->ss_descriptors;
			break;
		case USB_SPEED_HIGH:
			descriptors = f->hs_descriptors;
			break;
		default:
			descriptors = f->fs_descriptors;
		}

		if (!descriptors)
			continue;
		status = usb_descriptor_fillbuf(next, len,
			(const struct usb_descriptor_header **) descriptors);
		if (status < 0)
			return status;
		len -= status;
		next += status;
	}

	len = next - buf;
	c->wTotalLength = cpu_to_le16(len);
	return len;
}

static int config_desc(struct usb_composite_dev *cdev, unsigned w_value)
{
	struct usb_gadget		*gadget = cdev->gadget;
	struct usb_configuration	*c;
	u8				type = w_value >> 8;
	enum usb_device_speed		speed = USB_SPEED_UNKNOWN;

	if (gadget->speed == USB_SPEED_SUPER)
		speed = gadget->speed;
	else if (gadget_is_dualspeed(gadget)) {
		int	hs = 0;
		if (gadget->speed == USB_SPEED_HIGH)
			hs = 1;
		if (type == USB_DT_OTHER_SPEED_CONFIG)
			hs = !hs;
		if (hs)
			speed = USB_SPEED_HIGH;

	}

	
	w_value &= 0xff;
	list_for_each_entry(c, &cdev->configs, list) {
		
		switch (speed) {
		case USB_SPEED_SUPER:
			if (!c->superspeed)
				continue;
			break;
		case USB_SPEED_HIGH:
			if (!c->highspeed)
				continue;
			break;
		default:
			if (!c->fullspeed)
				continue;
		}

		if (w_value == 0)
			return config_buf(c, speed, cdev->req->buf, type);
		w_value--;
	}
	return -EINVAL;
}

static int count_configs(struct usb_composite_dev *cdev, unsigned type)
{
	struct usb_gadget		*gadget = cdev->gadget;
	struct usb_configuration	*c;
	unsigned			count = 0;
	int				hs = 0;
	int				ss = 0;

	if (gadget_is_dualspeed(gadget)) {
		if (gadget->speed == USB_SPEED_HIGH)
			hs = 1;
		if (gadget->speed == USB_SPEED_SUPER)
			ss = 1;
		if (type == USB_DT_DEVICE_QUALIFIER)
			hs = !hs;
	}
	list_for_each_entry(c, &cdev->configs, list) {
		
		if (ss) {
			if (!c->superspeed)
				continue;
		} else if (hs) {
			if (!c->highspeed)
				continue;
		} else {
			if (!c->fullspeed)
				continue;
		}
		count++;
	}
	return count;
}

static int bos_desc(struct usb_composite_dev *cdev)
{
	struct usb_ext_cap_descriptor	*usb_ext;
	struct usb_ss_cap_descriptor	*ss_cap;
	struct usb_dcd_config_params	dcd_config_params;
	struct usb_bos_descriptor	*bos = cdev->req->buf;

	bos->bLength = USB_DT_BOS_SIZE;
	bos->bDescriptorType = USB_DT_BOS;

	bos->wTotalLength = cpu_to_le16(USB_DT_BOS_SIZE);
	bos->bNumDeviceCaps = 0;

	usb_ext = cdev->req->buf + le16_to_cpu(bos->wTotalLength);
	bos->bNumDeviceCaps++;
	le16_add_cpu(&bos->wTotalLength, USB_DT_USB_EXT_CAP_SIZE);
	usb_ext->bLength = USB_DT_USB_EXT_CAP_SIZE;
	usb_ext->bDescriptorType = USB_DT_DEVICE_CAPABILITY;
	usb_ext->bDevCapabilityType = USB_CAP_TYPE_EXT;
	usb_ext->bmAttributes = cpu_to_le32(USB_LPM_SUPPORT | USB_BESL_SUPPORT);

	ss_cap = cdev->req->buf + le16_to_cpu(bos->wTotalLength);
	bos->bNumDeviceCaps++;
	le16_add_cpu(&bos->wTotalLength, USB_DT_USB_SS_CAP_SIZE);
	ss_cap->bLength = USB_DT_USB_SS_CAP_SIZE;
	ss_cap->bDescriptorType = USB_DT_DEVICE_CAPABILITY;
	ss_cap->bDevCapabilityType = USB_SS_CAP_TYPE;
	ss_cap->bmAttributes = 0; 
	ss_cap->wSpeedSupported = cpu_to_le16(USB_LOW_SPEED_OPERATION |
				USB_FULL_SPEED_OPERATION |
				USB_HIGH_SPEED_OPERATION |
				USB_5GBPS_OPERATION);
	ss_cap->bFunctionalitySupport = USB_LOW_SPEED_OPERATION;

	
	if (cdev->gadget->ops->get_config_params)
		cdev->gadget->ops->get_config_params(&dcd_config_params);
	else {
		dcd_config_params.bU1devExitLat = USB_DEFAULT_U1_DEV_EXIT_LAT;
		dcd_config_params.bU2DevExitLat =
			cpu_to_le16(USB_DEFAULT_U2_DEV_EXIT_LAT);
	}
	ss_cap->bU1devExitLat = dcd_config_params.bU1devExitLat;
	ss_cap->bU2DevExitLat = dcd_config_params.bU2DevExitLat;

	return le16_to_cpu(bos->wTotalLength);
}

static void device_qual(struct usb_composite_dev *cdev)
{
	struct usb_qualifier_descriptor	*qual = cdev->req->buf;

	qual->bLength = sizeof(*qual);
	qual->bDescriptorType = USB_DT_DEVICE_QUALIFIER;
	
	qual->bcdUSB = cdev->desc.bcdUSB;
	qual->bDeviceClass = cdev->desc.bDeviceClass;
	qual->bDeviceSubClass = cdev->desc.bDeviceSubClass;
	qual->bDeviceProtocol = cdev->desc.bDeviceProtocol;
	
	qual->bMaxPacketSize0 = cdev->gadget->ep0->maxpacket;
	qual->bNumConfigurations = count_configs(cdev, USB_DT_DEVICE_QUALIFIER);
	qual->bRESERVED = 0;
}


static void reset_config(struct usb_composite_dev *cdev)
{
	struct usb_function		*f;

	DBG(cdev, "reset config\n");

	list_for_each_entry(f, &cdev->config->functions, list) {
		INFO(cdev, "disable function '%s'/%p\n", f->name, f);
		if (f->disable)
			f->disable(f);

		bitmap_zero(f->endpoints, 32);
	}
	cdev->config = NULL;
	cdev->delayed_status = 0;
}

static int set_config(struct usb_composite_dev *cdev,
		const struct usb_ctrlrequest *ctrl, unsigned number)
{
	struct usb_gadget	*gadget = cdev->gadget;
	struct usb_configuration *c = NULL;
	int			result = -EINVAL;
	unsigned		power = gadget_is_otg(gadget) ? 8 : 100;
	int			tmp;

	if (number) {
		list_for_each_entry(c, &cdev->configs, list) {
			if (c->bConfigurationValue == number) {
				if (cdev->config)
					reset_config(cdev);
				result = 0;
				break;
			}
		}
		if (result < 0)
			goto done;
	} else { 
		if (cdev->config)
			reset_config(cdev);
		result = 0;
	}

	INFO(cdev, "%s config #%d: %s\n",
	     usb_speed_string(gadget->speed),
	     number, c ? c->label : "unconfigured");

	if (!c)
		goto done;

	cdev->config = c;

	
	for (tmp = 0; tmp < MAX_CONFIG_INTERFACES; tmp++) {
		struct usb_function	*f = c->interface[tmp];
		struct usb_descriptor_header **descriptors;

		if (!f)
			break;

		switch (gadget->speed) {
		case USB_SPEED_SUPER:
			descriptors = f->ss_descriptors;
			break;
		case USB_SPEED_HIGH:
			descriptors = f->hs_descriptors;
			break;
		default:
			descriptors = f->fs_descriptors;
		}

		for (; *descriptors; ++descriptors) {
			struct usb_endpoint_descriptor *ep;
			int addr;

			if ((*descriptors)->bDescriptorType != USB_DT_ENDPOINT)
				continue;

			ep = (struct usb_endpoint_descriptor *)*descriptors;
			addr = ((ep->bEndpointAddress & 0x80) >> 3)
			     |  (ep->bEndpointAddress & 0x0f);
			set_bit(addr, f->endpoints);
		}

		result = f->set_alt(f, tmp, 0);
		if (result < 0) {
			DBG(cdev, "interface %d (%s/%p) alt 0 --> %d\n",
					tmp, f->name, f, result);

			reset_config(cdev);
			goto done;
		}

		if (result == USB_GADGET_DELAYED_STATUS) {
			DBG(cdev,
			 "%s: interface %d (%s) requested delayed status\n",
					__func__, tmp, f->name);
			cdev->delayed_status++;
			DBG(cdev, "delayed_status count %d\n",
					cdev->delayed_status);
		}
	}

	
	power = c->MaxPower ? c->MaxPower : CONFIG_USB_GADGET_VBUS_DRAW;
done:
	usb_gadget_vbus_draw(gadget, power);
	if (result >= 0 && cdev->delayed_status)
		result = USB_GADGET_DELAYED_STATUS;
	return result;
}

int usb_add_config_only(struct usb_composite_dev *cdev,
		struct usb_configuration *config)
{
	struct usb_configuration *c;

	if (!config->bConfigurationValue)
		return -EINVAL;

	
	list_for_each_entry(c, &cdev->configs, list) {
		if (c->bConfigurationValue == config->bConfigurationValue)
			return -EBUSY;
	}

	config->cdev = cdev;
	list_add_tail(&config->list, &cdev->configs);

	INIT_LIST_HEAD(&config->functions);
	config->next_interface_id = 0;
	memset(config->interface, 0, sizeof(config->interface));

	return 0;
}
EXPORT_SYMBOL_GPL(usb_add_config_only);

int usb_add_config(struct usb_composite_dev *cdev,
		struct usb_configuration *config,
		int (*bind)(struct usb_configuration *))
{
	int				status = -EINVAL;

	if (!bind)
		goto done;

	DBG(cdev, "adding config #%u '%s'/%p\n",
			config->bConfigurationValue,
			config->label, config);

	status = usb_add_config_only(cdev, config);
	if (status)
		goto done;

	status = bind(config);
	if (status < 0) {
		while (!list_empty(&config->functions)) {
			struct usb_function		*f;

			f = list_first_entry(&config->functions,
					struct usb_function, list);
			list_del(&f->list);
			if (f->unbind) {
				INFO(cdev, "unbind function '%s'/%p\n",
					f->name, f);
				f->unbind(config, f);
				
			}
		}
		list_del(&config->list);
		pr_debug("[XLOG_DEBUG][USB][COM]bind fialed and the list should be init because there is one entry only");
		config->cdev = NULL;
	} else {
		unsigned	i;

		INFO(cdev, "cfg %d/%p speeds:%s%s%s\n",
			config->bConfigurationValue, config,
			config->superspeed ? " super" : "",
			config->highspeed ? " high" : "",
			config->fullspeed
				? (gadget_is_dualspeed(cdev->gadget)
					? " full"
					: " full/low")
				: "");

		for (i = 0; i < MAX_CONFIG_INTERFACES; i++) {
			struct usb_function	*f = config->interface[i];

			if (!f)
				continue;
			DBG(cdev, "  interface %d = %s/%p\n",
				i, f->name, f);
		}
	}

	usb_ep_autoconfig_reset(cdev->gadget);

done:
	if (status)
		DBG(cdev, "added config '%s'/%u --> %d\n", config->label,
				config->bConfigurationValue, status);
	return status;
}
EXPORT_SYMBOL_GPL(usb_add_config);

static void unbind_config(struct usb_composite_dev *cdev,
			      struct usb_configuration *config)
{
	while (!list_empty(&config->functions)) {
		struct usb_function		*f;

		f = list_first_entry(&config->functions,
				struct usb_function, list);
		list_del(&f->list);
		if (f->unbind) {
			INFO(cdev, "unbind function '%s'/%p\n", f->name, f);
			f->unbind(config, f);
			
		}
	}
	if (config->unbind) {
		INFO(cdev, "unbind config '%s'/%p\n", config->label, config);
		config->unbind(config);
			
	}

	
	usb_ep_autoconfig_reset(cdev->gadget);
}

void usb_remove_config(struct usb_composite_dev *cdev,
		      struct usb_configuration *config)
{
	unsigned long flags;

	spin_lock_irqsave(&cdev->lock, flags);
	if (cdev->config == config)
		reset_config(cdev);


	if(config->cdev != NULL)
	{
		list_del(&config->list);
	}else
  	{
        DBG(cdev, "%s: config->list has been delete!! \n", __func__);
  	}
  	
	spin_unlock_irqrestore(&cdev->lock, flags);

	if (os_type != OS_LINUX){
		os_type = OS_NOT_YET;
		fsg_update_mode(0);
		fsg_mode = 0;
		mtp_update_mode(1);
	}
#ifdef CONFIG_HTC_USB_DEBUG_FLAG
	printk("[USB]%s unbind+\n",__func__);
#endif

	unbind_config(cdev, config);
}



static void collect_langs(struct usb_gadget_strings **sp, __le16 *buf)
{
	const struct usb_gadget_strings	*s;
	__le16				language;
	__le16				*tmp;

	while (*sp) {
		s = *sp;
		language = cpu_to_le16(s->language);
		for (tmp = buf; *tmp && tmp < &buf[126]; tmp++) {
			if (*tmp == language)
				goto repeat;
		}
		*tmp++ = language;
repeat:
		sp++;
	}
}

static int lookup_string(
	struct usb_gadget_strings	**sp,
	void				*buf,
	u16				language,
	int				id
)
{
	struct usb_gadget_strings	*s;
	int				value;

	while (*sp) {
		s = *sp++;
		if (s->language != language)
			continue;
		value = usb_gadget_get_string(s, id, buf);
		if (value > 0)
			return value;
	}
	return -EINVAL;
}

static int get_string(struct usb_composite_dev *cdev,
		void *buf, u16 language, int id)
{
	struct usb_composite_driver	*composite = cdev->driver;
	struct usb_gadget_string_container *uc;
	struct usb_configuration	*c;
	struct usb_function		*f;
	int				len;


	
	if (id == 0) {
		struct usb_string_descriptor	*s = buf;
		struct usb_gadget_strings	**sp;

		memset(s, 0, 256);
		s->bDescriptorType = USB_DT_STRING;

		sp = composite->strings;
		if (sp)
			collect_langs(sp, s->wData);

		list_for_each_entry(c, &cdev->configs, list) {
			sp = c->strings;
			if (sp)
				collect_langs(sp, s->wData);

			list_for_each_entry(f, &c->functions, list) {
				sp = f->strings;
				if (sp)
					collect_langs(sp, s->wData);
			}
		}
		list_for_each_entry(uc, &cdev->gstrings, list) {
			struct usb_gadget_strings **sp;

			sp = get_containers_gs(uc);
			collect_langs(sp, s->wData);
		}

		for (len = 0; len <= 126 && s->wData[len]; len++)
			continue;
		if (!len)
			return -EINVAL;

		s->bLength = 2 * (len + 1);
		return s->bLength;
	}

	list_for_each_entry(uc, &cdev->gstrings, list) {
		struct usb_gadget_strings **sp;

		sp = get_containers_gs(uc);
		len = lookup_string(sp, buf, language, id);
		if (len > 0)
			return len;
	}

	if (composite->strings) {
		len = lookup_string(composite->strings, buf, language, id);
		if (len > 0)
			return len;
	}
	list_for_each_entry(c, &cdev->configs, list) {
		if (c->strings) {
			len = lookup_string(c->strings, buf, language, id);
			if (len > 0)
				return len;
		}
		list_for_each_entry(f, &c->functions, list) {
			if (!f->strings)
				continue;
			len = lookup_string(f->strings, buf, language, id);
			if (len > 0)
				return len;
		}
	}
	return -EINVAL;
}

int usb_string_id(struct usb_composite_dev *cdev)
{
	if (cdev->next_string_id < 254) {
		
		cdev->next_string_id++;
		return cdev->next_string_id;
	}
	return -ENODEV;
}
EXPORT_SYMBOL_GPL(usb_string_id);

int usb_string_ids_tab(struct usb_composite_dev *cdev, struct usb_string *str)
{
	int next = cdev->next_string_id;

	for (; str->s; ++str) {
		if (unlikely(next >= 254))
			return -ENODEV;
		str->id = ++next;
	}

	cdev->next_string_id = next;

	return 0;
}
EXPORT_SYMBOL_GPL(usb_string_ids_tab);

static struct usb_gadget_string_container *copy_gadget_strings(
		struct usb_gadget_strings **sp, unsigned n_gstrings,
		unsigned n_strings)
{
	struct usb_gadget_string_container *uc;
	struct usb_gadget_strings **gs_array;
	struct usb_gadget_strings *gs;
	struct usb_string *s;
	unsigned mem;
	unsigned n_gs;
	unsigned n_s;
	void *stash;

	mem = sizeof(*uc);
	mem += sizeof(void *) * (n_gstrings + 1);
	mem += sizeof(struct usb_gadget_strings) * n_gstrings;
	mem += sizeof(struct usb_string) * (n_strings + 1) * (n_gstrings);
	uc = kmalloc(mem, GFP_KERNEL);
	if (!uc)
		return ERR_PTR(-ENOMEM);
	gs_array = get_containers_gs(uc);
	stash = uc->stash;
	stash += sizeof(void *) * (n_gstrings + 1);
	for (n_gs = 0; n_gs < n_gstrings; n_gs++) {
		struct usb_string *org_s;

		gs_array[n_gs] = stash;
		gs = gs_array[n_gs];
		stash += sizeof(struct usb_gadget_strings);
		gs->language = sp[n_gs]->language;
		gs->strings = stash;
		org_s = sp[n_gs]->strings;

		for (n_s = 0; n_s < n_strings; n_s++) {
			s = stash;
			stash += sizeof(struct usb_string);
			if (org_s->s)
				s->s = org_s->s;
			else
				s->s = "";
			org_s++;
		}
		s = stash;
		s->s = NULL;
		stash += sizeof(struct usb_string);

	}
	gs_array[n_gs] = NULL;
	return uc;
}

struct usb_string *usb_gstrings_attach(struct usb_composite_dev *cdev,
		struct usb_gadget_strings **sp, unsigned n_strings)
{
	struct usb_gadget_string_container *uc;
	struct usb_gadget_strings **n_gs;
	unsigned n_gstrings = 0;
	unsigned i;
	int ret;

	for (i = 0; sp[i]; i++)
		n_gstrings++;

	if (!n_gstrings)
		return ERR_PTR(-EINVAL);

	uc = copy_gadget_strings(sp, n_gstrings, n_strings);
	if (IS_ERR(uc))
		return ERR_PTR(PTR_ERR(uc));

	n_gs = get_containers_gs(uc);
	ret = usb_string_ids_tab(cdev, n_gs[0]->strings);
	if (ret)
		goto err;

	for (i = 1; i < n_gstrings; i++) {
		struct usb_string *m_s;
		struct usb_string *s;
		unsigned n;

		m_s = n_gs[0]->strings;
		s = n_gs[i]->strings;
		for (n = 0; n < n_strings; n++) {
			s->id = m_s->id;
			s++;
			m_s++;
		}
	}
	list_add_tail(&uc->list, &cdev->gstrings);
	return n_gs[0]->strings;
err:
	kfree(uc);
	return ERR_PTR(ret);
}
EXPORT_SYMBOL_GPL(usb_gstrings_attach);

int usb_string_ids_n(struct usb_composite_dev *c, unsigned n)
{
	unsigned next = c->next_string_id;
	if (unlikely(n > 254 || (unsigned)next + n > 254))
		return -ENODEV;
	c->next_string_id += n;
	return next + 1;
}
EXPORT_SYMBOL_GPL(usb_string_ids_n);


void composite_setup_complete(struct usb_ep *ep, struct usb_request *req)
{
	if (req->status || req->actual != req->length)
		DBG((struct usb_composite_dev *) ep->driver_data,
				"setup complete --> %d, %d/%d\n",
				req->status, req->actual, req->length);
}

EXPORT_SYMBOL_GPL(composite_setup_complete);

int
composite_setup(struct usb_gadget *gadget, const struct usb_ctrlrequest *ctrl)
{
	struct usb_composite_dev	*cdev = get_gadget_data(gadget);
	struct usb_request		*req = cdev->req;
	int				value = -EOPNOTSUPP;
	int				status = 0;
	u16				w_index = le16_to_cpu(ctrl->wIndex);
	u8				intf = w_index & 0xFF;
	u16				w_value = le16_to_cpu(ctrl->wValue);
	u16				w_length = le16_to_cpu(ctrl->wLength);
	struct usb_function		*f = NULL;
	u8				endp;

	INFO(cdev, "%s bRequest=0x%X\n", __func__, ctrl->bRequest);

	req->zero = 0;
	req->complete = composite_setup_complete;
	req->length = 0;
	gadget->ep0->driver_data = cdev;

	switch (ctrl->bRequest) {

	
	case USB_REQ_GET_DESCRIPTOR:
		if (ctrl->bRequestType != USB_DIR_IN)
			goto unknown;
		switch (w_value >> 8) {
#ifdef CONFIG_USBIF_COMPLIANCE
		case USB_DT_OTG:
            {
            struct usb_otg_descriptor *otg_desc = req->buf;
            otg_desc->bLength = sizeof(*otg_desc);
            otg_desc->bDescriptorType = USB_DT_OTG;
            otg_desc->bmAttributes = USB_OTG_SRP | USB_OTG_HNP;
            otg_desc->bcdOTG = cpu_to_le16(0x0200);
			value = min_t(int, w_length,sizeof(struct usb_otg_descriptor));
            }
			break;
#endif
		case USB_DT_DEVICE:
			cdev->desc.bNumConfigurations =
				count_configs(cdev, USB_DT_DEVICE);
			cdev->desc.bMaxPacketSize0 =
				cdev->gadget->ep0->maxpacket;
			if (gadget_is_superspeed(gadget)) {
				if (gadget->speed >= USB_SPEED_SUPER) {
					cdev->desc.bcdUSB = cpu_to_le16(0x0300);
					cdev->desc.bMaxPacketSize0 = 9;
				} else {
					cdev->desc.bcdUSB = cpu_to_le16(0x0210);
				}
			}

			value = min(w_length, (u16) sizeof cdev->desc);
			memcpy(req->buf, &cdev->desc, value);
			INFO(cdev, "[COM]USB_REQ_GET_DESCRIPTOR: "
							"USB_DT_DEVICE, value=%d\n",value);
			break;
		case USB_DT_DEVICE_QUALIFIER:
			if (!gadget_is_dualspeed(gadget) ||
			    gadget->speed >= USB_SPEED_SUPER)
				break;
			device_qual(cdev);
			value = min_t(int, w_length,
				sizeof(struct usb_qualifier_descriptor));
			INFO(cdev, "[COM]USB_REQ_GET_DESCRIPTOR: "
							"USB_DT_DEVICE_QUALIFIER, value=%d\n",value);
			break;
		case USB_DT_OTHER_SPEED_CONFIG:
			INFO(cdev, "[COM]USB_REQ_GET_DESCRIPTOR: "
							"USB_DT_OTHER_SPEED_CONFIG\n");
			if (!gadget_is_dualspeed(gadget) ||
			    gadget->speed >= USB_SPEED_SUPER)
				break;
			
		case USB_DT_CONFIG:
			if (w_length == 4) {
				pr_info("%s: OS_MAC\n", __func__);
				os_type = OS_MAC;
				mtp_update_mode(1);
			} else if (w_length == 255) {
				pr_info("%s: OS_WINDOWS\n", __func__);
				os_type = OS_WINDOWS;
			} else if (w_length == 9 && os_type != OS_WINDOWS) {
				pr_info("%s: OS_LINUX\n", __func__);
				os_type = OS_LINUX;
				if (!fsg_mode) {
					schedule_delayed_work(&cdev->request_reset, REQUEST_RESET_DELAYED);
				}
			}

			value = config_desc(cdev, w_value);
			if (value >= 0)
				value = min(w_length, (u16) value);
			INFO(cdev, "[COM]USB_REQ_GET_DESCRIPTOR: "
							"USB_DT_CONFIG, value=%d\n",value);
			break;
		case USB_DT_STRING:
			value = get_string(cdev, req->buf,
					w_index, w_value & 0xff);
			if (value >= 0) {
				value = min(w_length, (u16) value);
				INFO(cdev, "[COM]USB_REQ_GET_DESCRIPTOR: "
						"USB_DT_STRING, value=%d\n" ,value);
			}
			break;
		case USB_DT_BOS:
			if (gadget_is_superspeed(gadget)) {
				value = bos_desc(cdev);
				value = min(w_length, (u16) value);
			}
			INFO(cdev, "[COM]USB_REQ_GET_DESCRIPTOR: "
						"USB_DT_BOS, value=%d\n",value);
			break;
		default:
			INFO(cdev, "[COM]USB_REQ_GET_DESCRIPTOR w_value=0x%X\n", w_value);
			break;
		}
		break;

	
	case USB_REQ_SET_CONFIGURATION:
		if (ctrl->bRequestType != 0)
			goto unknown;
		if (gadget_is_otg(gadget)) {
			if (gadget->a_hnp_support)
				DBG(cdev, "HNP available\n");
			else if (gadget->a_alt_hnp_support)
				DBG(cdev, "HNP on another port\n");
			else
				VDBG(cdev, "HNP inactive\n");
		}
		spin_lock(&cdev->lock);
		value = set_config(cdev, ctrl, w_value);
		spin_unlock(&cdev->lock);

		INFO(cdev, "[COM]USB_REQ_SET_CONFIGURATION: "
						"value=%d\n",value);
		break;
	case USB_REQ_GET_CONFIGURATION:
		if (ctrl->bRequestType != USB_DIR_IN)
			goto unknown;
		if (cdev->config)
			*(u8 *)req->buf = cdev->config->bConfigurationValue;
		else
			*(u8 *)req->buf = 0;
		value = min(w_length, (u16) 1);

		INFO(cdev, "[COM]USB_REQ_GET_CONFIGURATION: "
						"value=%d\n", value);

		break;

	/* function drivers must handle get/set altsetting */
	case USB_REQ_SET_INTERFACE:
		if (ctrl->bRequestType != USB_RECIP_INTERFACE)
			goto unknown;
		if (!cdev->config || intf >= MAX_CONFIG_INTERFACES)
			break;

		if (cdev->config)
		    f = cdev->config->interface[intf];
		else
			INFO(cdev, "%s: cdev->config = NULL \n", __func__);

		if (!f)
			break;

		/*
		 * If there's no get_alt() method, we know only altsetting zero
		 * works. There is no need to check if set_alt() is not NULL
		 * as we check this in usb_add_function().
		 */
		if (w_value && !f->get_alt)
			break;
		value = f->set_alt(f, w_index, w_value);
		if (value == USB_GADGET_DELAYED_STATUS) {
			DBG(cdev,
			 "%s: interface %d (%s) requested delayed status\n",
					__func__, intf, f->name);
			cdev->delayed_status++;
			DBG(cdev, "delayed_status count %d\n",
					cdev->delayed_status);
		}

		INFO(cdev, "[COM]USB_REQ_SET_INTERFACE: "
						"value=%d\n", value);
		break;
	case USB_REQ_GET_INTERFACE:
		if (ctrl->bRequestType != (USB_DIR_IN|USB_RECIP_INTERFACE))
			goto unknown;
		if (!cdev->config || intf >= MAX_CONFIG_INTERFACES)
			break;
		f = cdev->config->interface[intf];
		if (!f)
			break;
		
		value = f->get_alt ? f->get_alt(f, w_index) : 0;
		if (value < 0)
			break;
		*((u8 *)req->buf) = value;
		value = min(w_length, (u16) 1);

		INFO(cdev, "[COM]USB_REQ_GET_INTERFACE: "
						"value=%d\n", value);
		break;

	case USB_REQ_GET_STATUS:
		INFO(cdev, "[COM]USB_REQ_GET_STATUS\n");
		if (!gadget_is_superspeed(gadget))
			goto unknown;
		if (ctrl->bRequestType != (USB_DIR_IN | USB_RECIP_INTERFACE))
			goto unknown;
		value = 2;	
		put_unaligned_le16(0, req->buf);
		if (!cdev->config || intf >= MAX_CONFIG_INTERFACES)
			break;
		f = cdev->config->interface[intf];
		if (!f)
			break;
		status = f->get_status ? f->get_status(f) : 0;
		if (status < 0)
			break;
		put_unaligned_le16(status & 0x0000ffff, req->buf);
		break;
	case USB_REQ_CLEAR_FEATURE:
	case USB_REQ_SET_FEATURE:
		INFO(cdev, "[COM]%s w_value=%d\n",
			((ctrl->bRequest==USB_REQ_SET_FEATURE)? "USB_REQ_SET_FEATURE" : "USB_REQ_CLEAR_FEATURE"), w_value);
		if (!gadget_is_superspeed(gadget))
			goto unknown;
		if (ctrl->bRequestType != (USB_DIR_OUT | USB_RECIP_INTERFACE))
			goto unknown;
		switch (w_value) {
		case USB_INTRF_FUNC_SUSPEND:
			INFO(cdev, "[COM]USB_INTRF_FUNC_SUSPEND\n");
			if (!cdev->config || intf >= MAX_CONFIG_INTERFACES)
				break;
			f = cdev->config->interface[intf];
			if (!f)
				break;
			value = 0;
			if (f->func_suspend)
				value = f->func_suspend(f, w_index >> 8);
			if (value < 0) {
				ERROR(cdev,
				      "func_suspend() returned error %d\n",
				      value);
				value = 0;
			}
			break;
		}
		break;
	case USB_REQ_SET_SEL:
		INFO(cdev, "[COM]USB_REQ_SET_SEL Pretend success\n");
		value = 0;
		break;
	default:
unknown:
		INFO(cdev,
			"non-core control req%02x.%02x v%04x i%04x l%d\n",
			ctrl->bRequestType, ctrl->bRequest,
			w_value, w_index, w_length);

		switch (ctrl->bRequestType & USB_RECIP_MASK) {
		case USB_RECIP_INTERFACE:
			if (!cdev->config || intf >= MAX_CONFIG_INTERFACES)
				break;
			f = cdev->config->interface[intf];
			break;

		case USB_RECIP_ENDPOINT:
			endp = ((w_index & 0x80) >> 3) | (w_index & 0x0f);
			list_for_each_entry(f, &cdev->config->functions, list) {
				if (test_bit(endp, f->endpoints))
					break;
			}
			if (&f->list == &cdev->config->functions)
				f = NULL;
			break;
		}

		if (f && f->setup)
			value = f->setup(f, ctrl);
		else {
			struct usb_configuration	*c;

			c = cdev->config;
			if (c && c->setup)
				value = c->setup(c, ctrl);
		}

		goto done;
	}

	
	if (value >= 0 && value != USB_GADGET_DELAYED_STATUS) {
		req->length = value;
		req->zero = value < w_length;
		value = usb_ep_queue(gadget->ep0, req, GFP_ATOMIC);
		if (value < 0) {
			DBG(cdev, "ep_queue --> %d\n", value);
			req->status = 0;
			composite_setup_complete(gadget->ep0, req);
		}
	} else if (value == USB_GADGET_DELAYED_STATUS && w_length != 0) {
		WARN(cdev,
			"%s: Delayed status not supported for w_length != 0",
			__func__);
	}

done:
	if(value < 0) {
		INFO(cdev, "[COM]composite_setup: value=%d,"
				"bRequestType=0x%x, bRequest=0x%x, w_value=0x%x, w_length=0x%x \n", value,
				ctrl->bRequestType,	ctrl->bRequest, w_value, w_length);
	}
	
	return value;
}

void composite_disconnect(struct usb_gadget *gadget)
{
	struct usb_composite_dev	*cdev = get_gadget_data(gadget);
	unsigned long			flags;

	spin_lock_irqsave(&cdev->lock, flags);
	if (cdev->config)
		reset_config(cdev);
	if (cdev->driver->disconnect)
		cdev->driver->disconnect(cdev);

	
	
	if(cdev->req->complete)	{
		INFO(cdev, "[COM]%s: reassign the complete function!!\n", __func__);
		cdev->req->complete = composite_setup_complete;
	}

	spin_unlock_irqrestore(&cdev->lock, flags);
}


static ssize_t composite_show_suspended(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	struct usb_gadget *gadget = dev_to_usb_gadget(dev);
	struct usb_composite_dev *cdev = get_gadget_data(gadget);

	return sprintf(buf, "%d\n", cdev->suspended);
}

static DEVICE_ATTR(suspended, 0444, composite_show_suspended, NULL);

static void __composite_unbind(struct usb_gadget *gadget, bool unbind_driver)
{
	struct usb_composite_dev	*cdev = get_gadget_data(gadget);

	WARN_ON(cdev->config);

	while (!list_empty(&cdev->configs)) {
		struct usb_configuration	*c;
		c = list_first_entry(&cdev->configs,
				struct usb_configuration, list);
		list_del(&c->list);
		unbind_config(cdev, c);
	}
	if (cdev->driver->unbind && unbind_driver)
		cdev->driver->unbind(cdev);

	composite_dev_cleanup(cdev);

	kfree(cdev->def_manufacturer);
	kfree(cdev);
	set_gadget_data(gadget, NULL);
}

static void composite_unbind(struct usb_gadget *gadget)
{
	__composite_unbind(gadget, true);
}

static void update_unchanged_dev_desc(struct usb_device_descriptor *new,
		const struct usb_device_descriptor *old)
{
	__le16 idVendor;
	__le16 idProduct;
	__le16 bcdDevice;
	u8 iSerialNumber;
	u8 iManufacturer;
	u8 iProduct;

	idVendor = new->idVendor;
	idProduct = new->idProduct;
	bcdDevice = new->bcdDevice;
	iSerialNumber = new->iSerialNumber;
	iManufacturer = new->iManufacturer;
	iProduct = new->iProduct;

	*new = *old;
	if (idVendor)
		new->idVendor = idVendor;
	if (idProduct)
		new->idProduct = idProduct;
	if (bcdDevice)
		new->bcdDevice = bcdDevice;
	else
		new->bcdDevice = cpu_to_le16(get_default_bcdDevice());
	if (iSerialNumber)
		new->iSerialNumber = iSerialNumber;
	if (iManufacturer)
		new->iManufacturer = iManufacturer;
	if (iProduct)
		new->iProduct = iProduct;
}

int composite_dev_prepare(struct usb_composite_driver *composite,
		struct usb_composite_dev *cdev)
{
	struct usb_gadget *gadget = cdev->gadget;
	int ret = -ENOMEM;

	
	cdev->req = usb_ep_alloc_request(gadget->ep0, GFP_KERNEL);
	if (!cdev->req)
		return -ENOMEM;

#if defined(CONFIG_64BIT) && defined(CONFIG_MTK_LM_MODE)
	cdev->req->buf = kmalloc(USB_COMP_EP0_BUFSIZ, GFP_KERNEL | GFP_DMA);
#else
	cdev->req->buf = kmalloc(USB_COMP_EP0_BUFSIZ, GFP_KERNEL);
#endif
	if (!cdev->req->buf)
		goto fail;

	ret = device_create_file(&gadget->dev, &dev_attr_suspended);
	if (ret)
		goto fail_dev;

	cdev->req->complete = composite_setup_complete;
	gadget->ep0->driver_data = cdev;

	cdev->driver = composite;

	if (CONFIG_USB_GADGET_VBUS_DRAW <= USB_SELF_POWER_VBUS_MAX_DRAW)
		usb_gadget_set_selfpowered(gadget);

	usb_ep_autoconfig_reset(gadget);
	return 0;
fail_dev:
	kfree(cdev->req->buf);
fail:
	usb_ep_free_request(gadget->ep0, cdev->req);
	cdev->req = NULL;
	return ret;
}

void composite_dev_cleanup(struct usb_composite_dev *cdev)
{
	struct usb_gadget_string_container *uc, *tmp;

	list_for_each_entry_safe(uc, tmp, &cdev->gstrings, list) {
		list_del(&uc->list);
		kfree(uc);
	}
	if (cdev->req) {
		kfree(cdev->req->buf);
		usb_ep_free_request(cdev->gadget->ep0, cdev->req);
	}
	cdev->next_string_id = 0;
	device_remove_file(&cdev->gadget->dev, &dev_attr_suspended);
}

static int composite_bind(struct usb_gadget *gadget,
		struct usb_gadget_driver *gdriver)
{
	struct usb_composite_dev	*cdev;
	struct usb_composite_driver	*composite = to_cdriver(gdriver);
	int				status = -ENOMEM;

	cdev = kzalloc(sizeof *cdev, GFP_KERNEL);
	if (!cdev)
		return status;

	spin_lock_init(&cdev->lock);
	cdev->gadget = gadget;
	set_gadget_data(gadget, cdev);
	INIT_LIST_HEAD(&cdev->configs);
	INIT_LIST_HEAD(&cdev->gstrings);

	status = composite_dev_prepare(composite, cdev);
	if (status)
		goto fail;

	INIT_DELAYED_WORK(&cdev->request_reset, composite_request_reset);

	status = composite->bind(cdev);
	if (status < 0)
		goto fail;

	update_unchanged_dev_desc(&cdev->desc, composite->dev);

	
	if (composite->needs_serial && !cdev->desc.iSerialNumber)
		WARNING(cdev, "userspace failed to provide iSerialNumber\n");

	INFO(cdev, "%s ready\n", composite->name);
	return 0;

fail:
	__composite_unbind(gadget, false);
	return status;
}


static void
composite_suspend(struct usb_gadget *gadget)
{
	struct usb_composite_dev	*cdev = get_gadget_data(gadget);
	struct usb_function		*f;

	DBG(cdev, "suspend\n");
	if (cdev->config) {
		list_for_each_entry(f, &cdev->config->functions, list) {
			if (f->suspend)
				f->suspend(f);
		}
	}
	if (cdev->driver->suspend)
		cdev->driver->suspend(cdev);

	cdev->suspended = 1;

	usb_gadget_vbus_draw(gadget, 2);
}

static void
composite_resume(struct usb_gadget *gadget)
{
	struct usb_composite_dev	*cdev = get_gadget_data(gadget);
	struct usb_function		*f;
	u8				maxpower;

	DBG(cdev, "resume\n");
	if (cdev->driver->resume)
		cdev->driver->resume(cdev);
	if (cdev->config) {
		list_for_each_entry(f, &cdev->config->functions, list) {
			if (f->resume)
				f->resume(f);
		}

		maxpower = cdev->config->MaxPower;

		usb_gadget_vbus_draw(gadget, maxpower ?
			maxpower : CONFIG_USB_GADGET_VBUS_DRAW);
	}

	cdev->suspended = 0;
}


static const struct usb_gadget_driver composite_driver_template = {
	.bind		= composite_bind,
	.unbind		= composite_unbind,

	.setup		= composite_setup,
	.disconnect	= composite_disconnect,

	.suspend	= composite_suspend,
	.resume		= composite_resume,

	.driver	= {
		.owner		= THIS_MODULE,
	},
};

int usb_composite_probe(struct usb_composite_driver *driver)
{
	struct usb_gadget_driver *gadget_driver;

	if (!driver || !driver->dev || !driver->bind)
		return -EINVAL;

	pr_debug("[XLOG_DEBUG][USB][COM]%s: driver->name = %s", __func__, driver->name);

	if (!driver->name)
		driver->name = "composite";

	driver->gadget_driver = composite_driver_template;
	gadget_driver = &driver->gadget_driver;

	gadget_driver->function =  (char *) driver->name;
	gadget_driver->driver.name = driver->name;
	gadget_driver->max_speed = driver->max_speed;

	return usb_gadget_probe_driver(gadget_driver);
}
EXPORT_SYMBOL_GPL(usb_composite_probe);

void usb_composite_unregister(struct usb_composite_driver *driver)
{
	usb_gadget_unregister_driver(&driver->gadget_driver);
}
EXPORT_SYMBOL_GPL(usb_composite_unregister);

void usb_composite_setup_continue(struct usb_composite_dev *cdev)
{
	int			value;
	struct usb_request	*req = cdev->req;
	unsigned long		flags;

	DBG(cdev, "%s\n", __func__);
	spin_lock_irqsave(&cdev->lock, flags);

	if (cdev->delayed_status == 0) {
		WARN(cdev, "%s: Unexpected call\n", __func__);

	} else if (--cdev->delayed_status == 0) {
		DBG(cdev, "%s: Completing delayed status\n", __func__);
		req->length = 0;
		value = usb_ep_queue(cdev->gadget->ep0, req, GFP_ATOMIC);
		if (value < 0) {
			DBG(cdev, "ep_queue --> %d\n", value);
			req->status = 0;
			composite_setup_complete(cdev->gadget->ep0, req);
		}
	}

	spin_unlock_irqrestore(&cdev->lock, flags);
}
EXPORT_SYMBOL_GPL(usb_composite_setup_continue);

static char *composite_default_mfr(struct usb_gadget *gadget)
{
	char *mfr;
	int len;

	len = snprintf(NULL, 0, "%s %s with %s", init_utsname()->sysname,
			init_utsname()->release, gadget->name);
	len++;
	mfr = kmalloc(len, GFP_KERNEL);
	if (!mfr)
		return NULL;
	snprintf(mfr, len, "%s %s with %s", init_utsname()->sysname,
			init_utsname()->release, gadget->name);
	return mfr;
}

void usb_composite_overwrite_options(struct usb_composite_dev *cdev,
		struct usb_composite_overwrite *covr)
{
	struct usb_device_descriptor	*desc = &cdev->desc;
	struct usb_gadget_strings	*gstr = cdev->driver->strings[0];
	struct usb_string		*dev_str = gstr->strings;

	if (covr->idVendor)
		desc->idVendor = cpu_to_le16(covr->idVendor);

	if (covr->idProduct)
		desc->idProduct = cpu_to_le16(covr->idProduct);

	if (covr->bcdDevice)
		desc->bcdDevice = cpu_to_le16(covr->bcdDevice);

	if (covr->serial_number) {
		desc->iSerialNumber = dev_str[USB_GADGET_SERIAL_IDX].id;
		dev_str[USB_GADGET_SERIAL_IDX].s = covr->serial_number;
	}
	if (covr->manufacturer) {
		desc->iManufacturer = dev_str[USB_GADGET_MANUFACTURER_IDX].id;
		dev_str[USB_GADGET_MANUFACTURER_IDX].s = covr->manufacturer;

	} else if (!strlen(dev_str[USB_GADGET_MANUFACTURER_IDX].s)) {
		desc->iManufacturer = dev_str[USB_GADGET_MANUFACTURER_IDX].id;
		cdev->def_manufacturer = composite_default_mfr(cdev->gadget);
		dev_str[USB_GADGET_MANUFACTURER_IDX].s = cdev->def_manufacturer;
	}

	if (covr->product) {
		desc->iProduct = dev_str[USB_GADGET_PRODUCT_IDX].id;
		dev_str[USB_GADGET_PRODUCT_IDX].s = covr->product;
	}
}
EXPORT_SYMBOL_GPL(usb_composite_overwrite_options);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("David Brownell");
