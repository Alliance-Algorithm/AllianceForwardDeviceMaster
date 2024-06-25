/*
 * libusb example program to list devices on the bus
 * Copyright Ã‚Â© 2007 Daniel Drake <dsd@gentoo.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <cstddef>
#include <cstdint>
#include <stdio.h>
#include <unistd.h>

#include "libusb.h"
#include <string>

#include <thread>

#define VID 0x0483
#define PID 0x5740

#define IFACE_NUM 0x00

#define EP_ISO_IN 0x81U
#define EP_ISO_OUT 0x01U

#define NUM_TRANSFERS 2   //这个可以改，if you like
#define PACKET_SIZE 0x190 // lsusb 列出来的这个传输最大支持 64
#define NUM_PACKETS 1U    // lsusb 列出来的这个传输最大支持 64

//-------------------开始启动 transfer
uint8_t buf[PACKET_SIZE * NUM_PACKETS] = "0";
uint8_t buf2[PACKET_SIZE * NUM_PACKETS] = "0";
int iface[NUM_TRANSFERS] = {0x01, 0x02};
int ep[NUM_TRANSFERS] = {EP_ISO_IN, EP_ISO_OUT};
int data = 1;

uint8_t *bufs[2] = {buf2, buf};

static void cb_xfr(struct libusb_transfer *xfr) {
  // sleep(1);
  // using namespace std::chrono_literals;
  // std::this_thread::sleep_for(1ms);
  if (libusb_submit_transfer(xfr) < 0) {
    printf("error re-submitting !!!!!!!exit ----------[%d%s]\n", __LINE__,
           __FUNCTION__);
    _exit(1);
  } else {
    // printf("re-submint ok !\n");
  }
}
static struct libusb_device_handle *devh = NULL;

int main() {

  int ret;
  //-----------------库的初始化
  ret = libusb_init(NULL);
  if (ret < 0) {
    printf("can:: init erro:%d [%d%s]\n", ret, __LINE__, __FUNCTION__);
    return -1;
  }

  //------------------打开设备
  //每一个设备都有自己独有的 VID vendor 厂家ID, PID product 产品ID,
  //相当于usb设备的生份证 先通过lsusb命令 可以查看确定自己设备的VID PID devh =
  // libusb_open_device_with_vid_pid(NULL, 0x05ba, 0x000a);
  devh = libusb_open_device_with_vid_pid(NULL, VID, PID);
  if (devh == NULL) {
    printf("can:: open erro [%d%s] \n", __LINE__, __FUNCTION__);
    return -1;
  }

  struct libusb_transfer *xfr[NUM_TRANSFERS];

  int i = 0;
  for (i = 0; i < 2; i++) {

    //先做一个check，确保设备没有占用，在最小demo情况下，可以先不考虑这种复杂情况
    ret = libusb_kernel_driver_active(devh, iface[i]);
    if (ret == 1) {
      printf("acticve ,to deteach .");
      ret = libusb_detach_kernel_driver(devh, iface[i]);
      if (ret < 0) {
        printf("canok:: erro to detach kernel!!![%d%s]\n", __LINE__,
               __FUNCTION__);
        return -1;
      }
    }
    //------------------请求使用一个 interface 第二个参数是 interface 编号
    // ret = libusb_claim_interface(devh, IFACE_NUM);
    ret = libusb_claim_interface(devh, iface[i]);
    if (ret < 0) {
      printf("can:: erro claming interface %s %d [%d%s]\n",
             libusb_error_name(ret), ret, __LINE__, __FUNCTION__);
      return -1;
    }
    //-------------------同一个接口可以有多个接口描述符，用bAlternateSetting来识别.
    //在Interface Descriptor 中的bAlternateSetting 值
    // libusb_set_interface_alt_setting(devh, IFACE_NUM, 1);
    libusb_set_interface_alt_setting(devh, iface[i], 0);
    xfr[i] = libusb_alloc_transfer(NUM_PACKETS);
    if (!xfr[i]) {
      printf("can:: alloc transfer err [%d%s]o\n", __LINE__, __FUNCTION__);
      return -1;
    }

    libusb_fill_iso_transfer(xfr[i], devh, ep[i], bufs[i],
                             PACKET_SIZE * NUM_PACKETS, NUM_PACKETS, cb_xfr,
                             NULL, 1000);
    libusb_set_iso_packet_lengths(xfr[i], PACKET_SIZE);

    //正式提交任务
    ret = libusb_submit_transfer(xfr[i]);
    if (ret == 0) {
      printf("canok:: transfer submint ok ! start capture[%d%s]\n", __LINE__,
             __FUNCTION__);
    } else {
      printf("canok:: transfer submint erro %d %s [%d%s]\n", ret,
             libusb_error_name(ret), __LINE__, __FUNCTION__);
    }
  }

  //------------------主循环 ，驱动事件
  while (1) {
    ret = libusb_handle_events(NULL);
    if (ret != LIBUSB_SUCCESS) {
      printf("can:: handle event erro ,exit! [%d%s]\n", __LINE__, __FUNCTION__);
      break;
    }

    data++;
    std::to_string(data).copy((char *)buf, PACKET_SIZE * NUM_PACKETS);
    printf("%s\n", buf2);
  }

  //逆初始化
  libusb_release_interface(devh, 0);

  if (devh) {
    libusb_close(devh);
  }

  libusb_exit(NULL);

  //好像没有释放 transfer？？？？？
  return 0;
}
