#ifndef MOTOR_H
#define MOTOR_H

#include <rtthread.h>
#include "rtdevice.h"
#include "rthw.h"
#include "drv_remote.h"
#include "drv_motor.h"

typedef struct motor_id
{
    rt_int32_t id_tx;
    rt_int32_t id_rx;
}motor_id_t;


int can_sample(void);

#endif
