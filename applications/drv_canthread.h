#ifndef __DRV_CANTHREAD_H__
#define __DRV_CANTHREAD_H__
#include <rtdevice.h>
#include <rtthread.h>

extern rt_device_t can1_dev;      //CAN 设备句柄
extern rt_device_t can2_dev;      //CAN 设备句柄

//can1初始化，can1数据处理线程和中断设定
extern int can1_init(void);
//can2初始化，can2数据处理线程和中断设定
extern int can2_init(void);

#endif



