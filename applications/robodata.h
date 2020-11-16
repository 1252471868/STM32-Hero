#ifndef __ROBODATA_H__
#define __ROBODATA_H__

#include <rtthread.h>
#include <rtdevice.h>

#define YAW_ZERO_ANGLE          1360
#define PITCH_ZERO_ANGLE        1330

#define PITCH_MIN_ANGLE			1000
#define PITCH_MAX_ANGLE			1500

#define SNALL   0
#define FIRE_ANGLE 60
#define HERO

typedef enum
{
    //发送
    GIMBAL_CTL = 0x1FF,
	CHASSIS_CTL = 0x100,

    //接收
    GYRO_ANGLE_ID = 0x101,  //陀螺仪CAN数据ID
    GYRO_SPEED_ID = 0x102,

    YAW_ID      = 0x205,	//云台两电机数据更新ID
    PITCH_ID    = 0x206

}drv_can1ID_e;
//CAN1设备ID

typedef enum
{
    //发送
    VISUAL_CTLID = 0X301,	//视觉通信发送

    //接收
	VISUAL_REVID = 0X302	//视觉通信接收

}drv_can2ID_e;
//云台CAN2设备ID

#endif
