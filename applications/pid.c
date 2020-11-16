#include "pid.h"

#define ABS(a) (((a)>0)?(a):(-a))

/**
 * @brief  pid参数设定
 * @param  pid：pid结构体指针
 */
void pid_init(pid_t *pid,
			float kp, float ki, float kd,
			rt_uint16_t i_limit,
			rt_int16_t 	out_limit_up,
			rt_int16_t 	out_limit_down)
{
	//参数设置
	pid->kp = kp;
	pid->ki = ki;
	pid->kd = kd;
	pid->i_limit = i_limit;
	pid->out_limit_up = out_limit_up;
	pid->out_limit_down = out_limit_down;
	//状态归零
	pid->err = 0;
	pid->err_old = 0;
	pid->i_value = 0;
	pid->out = 0;
	pid->set = 0;
}

/**
* @brief：该函数根据四个电机的设定参数分别计算出其输出
* @param [in]	target:要计算的pid结构体
				error:期望值与实际值的误差（单位rpm
* @return：		pid的计算结果
* @author：mqy
*/
int pid_output_calculate(pid_t* target,int set,int now)
{
	target->set = set;
	int error = target->set - now;
	target->err_old = target->err;
	target->err = error;
	target->i_value += target->ki * error;

	//积分限幅
	if(target->i_value < 0)
	{
		if(target->i_value < -target->i_limit)
		{
			target->i_value = -target->i_limit;
		}
	}
	else
	{
		if(target->i_value > target->i_limit)
		{
			target->i_value = target->i_limit;
		}
	}
	target->out = target->kp*(float)error + target->i_value + target->kd*(float)(target->err - target->err_old);

	//输出限幅
	if(target->out > target->out_limit_up)
	{
		target->out = target->out_limit_up;
	}
	else if(target->out < target->out_limit_down)
	{
		target->out = target->out_limit_down;
	}
	//返回值
	return target->out;
}
/**
* @brief：电机pid输出的计算（0-8191格式
* @param [in]	target:要计算的pid结构体
				nowdata:当前值
* @return：		pid的计算结果
* @author：mqy
*/
int pid_output_motor(pid_t* target,int set,int now)
{
	target->set = set;
	rt_int16_t error = 0;
	if(target->set > 8191)
	{
		target->set -= 8191;
	}
	else if(target->set < 0)
	{
		target->set += 8191;
	}
	error = target->set - now;
	if(ABS(error) > (8192/2))
	{
		if(error>0){error -= 8192;}
		else {error += 8192;}
	}
//	if(ABS(error) > (8191 - ABS(error)))
//	{
//		if(error > 0)
//		{
//			error = 8191 - error;
//		}
//		else
//		{
//			error = 8191 + error;
//		}
//	}
	target->err_old = target->err;
	target->err = error;
	target->i_value += target->ki * error;

	//积分限幅
	if(target->i_value < 0)
	{
		if(target->i_value < -target->i_limit)
		{
			target->i_value = -target->i_limit;
		}
	}
	else
	{
		if(target->i_value > target->i_limit)
		{
			target->i_value = target->i_limit;
		}
	}
	target->out = target->kp*(float)error + target->i_value + target->kd*(float)(target->err - target->err_old);

	//输出限幅
	if(target->out > target->out_limit_up)
	{
		target->out = target->out_limit_up;
	}
	else if(target->out < target->out_limit_down)
	{
		target->out = target->out_limit_down;
	}
	//返回值
	return target->out;
}
/**
* @brief：清除PID的状态
* @param [in]	target:要清零的PID结构体指针
* @return：		无
* @author：mqy
*/
void pid_clear(pid_t* target)
{
	//状态归零
	target->err = 0;
	target->err_old = 0;
	target->i_value = 0;
	target->out = 0;
	target->set = 0;
}
