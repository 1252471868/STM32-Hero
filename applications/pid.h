#ifndef __PID_H__
#define __PID_H__

#include <rtthread.h>


typedef struct
{
	float 	kp;					//pid三个参数
	float 	ki;
	float 	kd;

	int 	set;				//pid的设定值
	int 	err;				//偏差值
	int 	err_old;			//上次偏差值

	float 	i_value;			//现在的积分值（该积分已经乘了ki参数了
	float  	i_limit;			//积分限幅

	int		out;				//pid的输出
	int		out_limit_up;		//输出上限幅
	int		out_limit_down;		//输出下限幅

}pid_t;
//电机PID闭环结构体

extern void pid_init(pid_t *pid,
	float kp, float ki, float kd, 	//pid参数
	rt_uint16_t i_limite, 			//积分限幅
	rt_int16_t out_limite_up, 		//输出上限幅
	rt_int16_t out_limite_down);	//输出下限幅

extern int pid_output_calculate(pid_t* target,int set,int now);
extern int pid_output_motor(pid_t* target,int set,int now);
extern void pid_clear(pid_t* target);

#endif
