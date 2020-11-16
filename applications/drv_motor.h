#ifndef __DRV_MOTOR_H
#define __DRV_MOTOR_H

#include <rtthread.h>
#include <rtdevice.h>
#include "pid.h"
// #include "robodata.h"
#define ABS(x)						((x)<0?-(x):(x))
/*电机消息发送ID（STDID）*/
typedef enum
{
	STDID_Pantilt = 0x1FF,
	STDID_launch = 0x200
}SendID_e;
typedef struct
{
	rt_uint16_t angle;																													/*角度 0~8191*/
	rt_int16_t speed;
	rt_int16_t current;
	rt_uint8_t temperature;

	rt_uint16_t old_angle;																										/*上一次编码器数值*/
	rt_int32_t loop;																											/*一共所转度数*/
	float ratio;																															/*减速箱的减速比<=1*/

	rt_uint32_t motorID;																													/*电机反馈报文ID*/
}DjiMotor_t;
typedef struct __Motor_t{
	DjiMotor_t dji;
	pid_t ang;
	pid_t spe;
}Motor_t;
void 	motor_readmsg(struct rt_can_msg* rxmsg,DjiMotor_t* motor);							/*读取can中的电机数据*/
rt_size_t motor_current_send(rt_device_t dev, 																/*电机电流发送*/
														rt_int32_t   ID,
														rt_int16_t setcur1,
														rt_int16_t setcur2,
														rt_int16_t setcur3,
														rt_int16_t setcur4);
void motor_angle_set(Motor_t *motor,float angle);
void motor_init(Motor_t *motor,rt_uint32_t ID,float radio);

#endif
