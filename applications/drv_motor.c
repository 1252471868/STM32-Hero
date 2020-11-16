#include "drv_motor.h"
/**
 * @brief  对反馈角度进行换算为0-8191
 * @param  angle：角度
 * @param  motor：电机数据结构体
 * @retval None
 */
static void motor_angle_adjsut(rt_uint16_t angle,DjiMotor_t* motor)
{
	float angletemp = 0;

	/*计算总的编码器转过角度*/
	if(motor->speed >= 0)
	{
		if(angle < motor->old_angle - 1000)														/*加1000是为了去除编码器波动*/
		{
			motor->loop += 1;
		}
	}
	else
	{
		if(angle > motor->old_angle + 1000)														/*加1000是为了去除编码器波动*/
		{
			motor->loop -= 1;
		}
	}
	/*计算0-8191范围内的角度*/
	angletemp = (motor->loop*8192 + angle) * motor->ratio / 8192;
	angletemp = angletemp - (int)angletemp;
	if(angletemp < 0)
	{
		motor->angle = (angletemp + 1)* 8192;
	}
	else
	{
		motor->angle = angletemp * 8192;
	}
	motor->old_angle = angle;
}
/**
 * @brief  读取can中的电机数据
 * @param  rxmsg：rtcan结构体
 * @param  moto：电机数据结构
 * @retval None
 */
void motor_readmsg(struct rt_can_msg* rxmsg,DjiMotor_t* motor)
{
	rt_uint16_t angle;

	motor->speed = (rt_int16_t)(rxmsg->data[2]<<8 | rxmsg->data[3]);
	motor->current = (rt_int16_t)(rxmsg->data[4]<<8 | rxmsg->data[5]);
	motor->temperature = rxmsg->data[6];

	angle = (rt_int16_t)(rxmsg->data[0]<<8 | rxmsg->data[1]);
	motor_angle_adjsut(angle, motor);
}

/**
 * @brief  电机电流发送
 * @param  dev：can设备
 * @param  setcurn:设定电流
 * @retval RT_EOK or RT_ERROR(成功，失败或其他报错)
 */
rt_size_t motor_current_send(rt_device_t dev,
														rt_int32_t   	ID,
														rt_int16_t 	setcur1,
														rt_int16_t 	setcur2,
														rt_int16_t 	setcur3,
														rt_int16_t 	setcur4)
{
	struct rt_can_msg txmsg;

	txmsg.id=ID;
	txmsg.ide = RT_CAN_STDID;txmsg.rtr=RT_CAN_DTR;
	txmsg.len=8;
	txmsg.data[0]=(rt_uint8_t)(setcur1>>8);txmsg.data[1]=(rt_uint8_t)(setcur1);
	txmsg.data[2]=(rt_uint8_t)(setcur2>>8);txmsg.data[3]=(rt_uint8_t)(setcur2);
	txmsg.data[4]=(rt_uint8_t)(setcur3>>8);txmsg.data[5]=(rt_uint8_t)(setcur3);
	txmsg.data[6]=(rt_uint8_t)(setcur4>>8);txmsg.data[7]=(rt_uint8_t)(setcur4);
	return rt_device_write(dev, 0, &txmsg, sizeof(txmsg));
}


void motor_angle_set(Motor_t *motor,float angle)
{
	motor->ang.set +=  (rt_int16_t)(angle/360.0f*8192);
}
void motor_init(Motor_t *motor,rt_uint32_t ID,float radio)
{
	motor->dji.motorID = ID;
	motor->dji.ratio = radio;
}
