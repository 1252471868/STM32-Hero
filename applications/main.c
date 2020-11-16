/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-11-06     SummerGift   first version
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>
#include "drv_remote.h"
//#include "can_motor.h"
// #include "my_uart.h"
#include "my_pin.h"
#include "my_timer.h"
#include "drv_gimbal.h"
#include "drv_canthread.h"
//#include "arm_math.h"
//#include "arm_const_structs.h"

int main(void)
{
    //SEGGER_SYSVIEW_Start();
    // timer_static_sample();
    //can_sample();
    gimbal_init();
    remote_uart_init();
	can1_init();







    // demo_init();
    //uart_dma_sample();


    return RT_EOK;
}
