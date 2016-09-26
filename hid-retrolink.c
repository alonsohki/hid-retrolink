/*
 *
 * HID driver for retro-bit/Retro Link dual controller port adapter.
 * Based on https://github.com/retuxx/hid-retrobit and the Linux kernel
 * XBOX controller driver.
 *
 * Copyright (c) 2013 RobMcMullen
 *
 * This file is part of hid-retrobit.
 *
 * hid-retrobit is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * hid-retrolink is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with hid-retrobit.  If not, see <http://www.gnu.org/licenses/>.
 *
 * ^ Same thing for hid-retrolink.
 *
 */

#include <linux/device.h>
#include <linux/hid.h>
#include <linux/input.h>
#include <linux/usb/input.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/usb.h>

#define RETROLINK_PKT_LEN 4

struct retrolink_data
{
    struct input_dev *dev;
    struct usb_device *udev;
    struct usb_interface *intf;

    struct urb *irq_in;
    unsigned char *idata;
    dma_addr_t idata_dma;

    int input_created;

    char phys[64];
};


/********************************************************
*                                                       *
* Setup supported devices                               *
*                                                       *
********************************************************/
#define USB_VENDOR_ID_INNEX                  0x1292
#define USB_DEVICE_ID_INNEX_SNES_ADAPTER     0x5366

static struct usb_device_id retrolink_table[] = {
    { USB_DEVICE(USB_VENDOR_ID_INNEX, USB_DEVICE_ID_INNEX_SNES_ADAPTER) },
    {}
};

MODULE_DEVICE_TABLE(usb, retrolink_table);

static const int abs_x[8] = { 0, 1, 1, 1, 0, -1, -1, -1 };
static const int abs_y[8] = { 1, 1, 0, -1, -1, -1, 0, 1 };

static void retrolink_process_packet(struct retrolink_data* data, unsigned char *rawdata)
{
    struct input_dev* dev = data->dev;

    input_report_key(dev, BTN_A, rawdata[0] & 0x1);
    input_report_key(dev, BTN_B, rawdata[0] & 0x2);
    input_report_key(dev, BTN_X, rawdata[0] & 0x8);
    input_report_key(dev, BTN_Y, rawdata[0] & 0x10);
    input_report_key(dev, BTN_TL, rawdata[0] & 0x40);
    input_report_key(dev, BTN_TR, rawdata[0] & 0x80);

    input_report_key(dev, BTN_SELECT, rawdata[1] & 0x4);
    input_report_key(dev, BTN_START, rawdata[1] & 0x8);

    if ((rawdata[2] & 0xF) != 0xF)
    {
        input_report_abs(dev, ABS_X, abs_x[rawdata[2] & 0xF]);
        input_report_abs(dev, ABS_Y, abs_y[rawdata[2] & 0xF]);
    }
    else
    {
        input_report_abs(dev, ABS_X, 128);
        input_report_abs(dev, ABS_Y, 128);
    }

    input_sync(dev);
}

static void retrolink_irq_in(struct urb *urb)
{
    struct retrolink_data* data = urb->context;
    int retval;
    int status = urb->status;

    switch (status)
    {
        case 0:
            break;

        case -ECONNRESET:
        case -ENOENT:
        case -ESHUTDOWN:
            //printk("%s - urb shutting down with status: %d\n", __func__, status);
            return;

        default:
            //printk("%s - nonzero urb status received: %d\n", __func__, status);
            goto exit;
    }

    retrolink_process_packet(data, data->idata);

exit:
    retval = usb_submit_urb(urb, GFP_ATOMIC);

    //if (retval)
    //    printk("%s - usb_submit_urb failed with result %d\n", __func__, retval);
}

static int retrolink_start_input(struct retrolink_data* data)
{
    if (usb_submit_urb(data->irq_in, GFP_KERNEL))
        return -EIO;
    return 0;
}

static void retrolink_stop_input(struct retrolink_data* data)
{
    usb_kill_urb(data->irq_in);
}

static int init_input(struct retrolink_data* data)
{
    struct input_dev *input_dev;
    int err;

    input_dev = input_allocate_device();
    if (!input_dev)
        return -ENOMEM;

    data->dev = input_dev;
    input_dev->name = "Retrolink dual controller port adapter";
    input_dev->phys = data->phys;
    usb_to_input_id(data->udev, &input_dev->id);
    input_dev->dev.parent = &data->intf->dev;
    input_dev->id.bustype = BUS_HOST;

    input_set_drvdata(input_dev, data);

    __set_bit(EV_KEY, input_dev->evbit);
    __set_bit(EV_ABS, input_dev->evbit);
    __set_bit(BTN_START, input_dev->keybit);
    __set_bit(BTN_SELECT, input_dev->keybit);
    __set_bit(BTN_A, input_dev->keybit);
    __set_bit(BTN_B, input_dev->keybit);
    __set_bit(BTN_X, input_dev->keybit);
    __set_bit(BTN_Y, input_dev->keybit);
    __set_bit(BTN_TL, input_dev->keybit);
    __set_bit(BTN_TR, input_dev->keybit);

    __set_bit(ABS_X, input_dev->absbit);
    __set_bit(ABS_Y, input_dev->absbit);
    input_set_abs_params(input_dev, ABS_X, -1, 1, 0, 0);
    input_set_abs_params(input_dev, ABS_Y, -1, 1, 0, 0);


    err = input_register_device(data->dev);
    if (err)
    {
        input_free_device(input_dev);
        return err;
    }

    data->input_created = 1;
    return 0;
}


static int retrolink_probe(struct usb_interface *intf, const struct usb_device_id *id)
{
    struct usb_device *udev = interface_to_usbdev(intf);
    struct retrolink_data* data;
    struct usb_endpoint_descriptor *ep_irq_in;
    int err;

    data = kzalloc(sizeof(struct retrolink_data), GFP_KERNEL);
    if (!data)
        return -ENOMEM;

    usb_make_path(udev, data->phys, sizeof(data->phys));
    strlcat(data->phys, "/input0", sizeof(data->phys));

    data->idata = usb_alloc_coherent(udev, RETROLINK_PKT_LEN,
                                     GFP_KERNEL, &data->idata_dma);
    if (!data->idata)
    {
        err = -ENOMEM;
        goto err_free_mem;
    }

    data->udev = udev;
    data->intf = intf;

    usb_set_intfdata(intf, data);

    data->irq_in = usb_alloc_urb(0, GFP_KERNEL);
    if (!data->irq_in)
    {
        err = -ENOMEM;
        goto err_free_idata;
    }

    ep_irq_in = &intf->cur_altsetting->endpoint[0].desc;
    usb_fill_int_urb(data->irq_in, udev,
                     usb_rcvintpipe(udev, ep_irq_in->bEndpointAddress),
                     data->idata, RETROLINK_PKT_LEN, retrolink_irq_in,
                     data, ep_irq_in->bInterval);
    data->irq_in->transfer_dma = data->idata_dma;
    data->irq_in->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;

    err = init_input(data);
    if (err < 0)
        goto err_free_in_urb;

    retrolink_start_input(data);

    return 0;


err_free_in_urb:
    usb_free_urb(data->irq_in);

err_free_idata:
    usb_free_coherent(udev, RETROLINK_PKT_LEN, data->idata, data->idata_dma);

err_free_mem:
    usb_set_intfdata(intf, NULL);
    kfree(data);

    return err;
}

static void retrolink_disconnect(struct usb_interface *intf)
{
    struct retrolink_data* data = usb_get_intfdata(intf);
    if (data)
    {
        if (data->input_created)
        {
            retrolink_stop_input(data);
            input_unregister_device(data->dev);
        }
        if (data->irq_in)
            usb_free_urb(data->irq_in);

        usb_free_coherent(data->udev, RETROLINK_PKT_LEN,
                          data->idata, data->idata_dma);

        kfree(data);
        usb_set_intfdata(intf, NULL);
    }
}

static int retrolink_suspend(struct usb_interface *intf, pm_message_t message)
{
    return 0;
}

static int retrolink_resume(struct usb_interface *intf)
{
    return 0;
}

/********************************************************
*                                                       *
* Setup module                                          *
*                                                       *
********************************************************/
static struct usb_driver retrolink_driver = {
    .name           = "retrolink",
    .probe          = retrolink_probe,
    .disconnect     = retrolink_disconnect,
    .suspend        = retrolink_suspend,
    .resume         = retrolink_resume,
    .reset_resume   = retrolink_resume,
    .id_table       = retrolink_table,
};


module_usb_driver(retrolink_driver);
MODULE_LICENSE("GPL");

