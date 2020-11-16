#ifndef __GIMBAL_CONTROL_H__
#define __GIMBAL_CONTROL_H__

#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>

#include "pid.h"
#include "robodata.h"
#include "can_receive.h"
#include "drv_canthread.h"
#include "drv_remote.h"
// #include "drv_IMU.h"

typedef enum
{
	ANGLE,			//设置位置
	PALSTANCE,		//角速度

} control_mode_t;
//控制模式枚举，可选的直接控制量

typedef enum
{
	DJI_MOTOR,		//数据源电机编码器
	GYRO			//数据源陀螺仪

} data_source_t;
//闭环数据源枚举

typedef struct
{
	rt_uint16_t 	angle;				//转子角度，大小范围0~8191
	rt_int16_t 		speed;				//转子转速，单位rpm
	rt_int16_t 		current;			//实际转矩电流
	rt_uint8_t 		temperature;		//电机的实际温度

} motordata_t;
//电机的当前转速等数据

typedef struct
{
	control_mode_t	control_mode;		//控制模式
	uint16_t 		motorID;			//枚举定义电机ID
	motordata_t		motordata;			//电机数据结构体
	int				setang;				//角度设定值

	pid_t		    palpid;				//加速度结构体
	data_source_t	angdata_source;		//角度闭环数据源
	pid_t			angpid_dji;			//数据源电机时pid
	pid_t			angpid_gyro;		//数据源陀螺仪时pid

} gimbalmotor_t;

//外部调用的控制函数
extern int gimbal_absangle_set(rt_uint16_t yawset,rt_uint16_t pitchset);
extern int gimbal_addangle_set(rt_int16_t yawset,rt_int16_t pitchset);
extern int gimbal_palstance_set(rt_uint16_t yawset,rt_uint16_t pitchset);
extern int gimbal_ctlmode_set(control_mode_t yawset,control_mode_t pitchset);
extern int angle_datasource_set(data_source_t yawset,data_source_t pitchset);

//必须调用的函数
//默认CAN1设备，默认ID YAW_ID = 0X205 PITCH_ID = 0X206
extern int refresh_gimbal_motor_data(struct rt_can_msg* message);
extern int gimbal_init(void);

//读取云台数据函数
extern int get_yawangle(void);
extern int get_pitchangle(void);

#endif
