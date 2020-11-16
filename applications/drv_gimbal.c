#include "drv_gimbal.h"

//云台电机PID线程句柄
static rt_thread_t 		gimbal_control 	= RT_NULL;


//m_yaw m_pitch电机数据结构体
 gimbalmotor_t	m_yaw;
 gimbalmotor_t	m_pitch;
/**
* @brief：该函数计算角度环并输出
* @param [in]	motor:云台电机指针
				gyrodata:对应轴的陀螺仪数据
* @return：		无
* @author：mqy
*/
static int angpid_cal(gimbalmotor_t* motor,int gyroangle)
{
	switch (motor->angdata_source)
	{
	case GYRO:
		motor->angpid_gyro.set = motor->setang;
		return pid_output_motor(&motor->angpid_gyro,motor->angpid_gyro.set,gyroangle);

	case DJI_MOTOR:
		motor->angpid_dji.set = motor->setang;
		return pid_output_motor(&motor->angpid_dji,motor->angpid_dji.set,motor->motordata.angle);

	default:
		return 0;
	}
}
/**
* @brief：该函数计算云台电机pid并保存在pid的out中
* @param [in]	motor:云台电机指针
				gyrodata:对应轴的陀螺仪数据
* @return：		无
* @author：mqy
*/
static int gimbalpid_cal(gimbalmotor_t* motor,int gyroangle,int gyropal,rt_uint8_t angle_time)
{
	switch (motor->control_mode)
	{
	case ANGLE:
		{
			motor->palpid.set = angpid_cal(motor,gyroangle);
		}
	case PALSTANCE:
		{
			//角速度环是一定会被计算的
			pid_output_calculate(&(motor->palpid),motor->palpid.set,gyropal);
		}
		break;

	default://不存在则电流归零
		motor->palpid.out = 0;
		break;
	}
	return 0;
}
/**
* @brief：根据云台电机的设定参数计算输出并发送数据
				在初始化之后该函数每经过x ms会执行一次
* @param [in]	parameter:该参数不会被使用
* @return：		无
* @author：mqy
*/
//1ms任务
static struct rt_timer task_1ms;
static struct rt_semaphore gimbal_1ms_sem;
static void task_1ms_IRQHandler(void *parameter)
{
	rt_sem_release(&gimbal_1ms_sem);
}
	int yawang,yawpal,pitchang,pitchpal;
static void gimbal_contral_thread(void* parameter)
{
	//初始化CAN控制帧
	struct rt_can_msg gimctl_msg;
	gimctl_msg.id	= GIMBAL_CTL;	//设置ID
	gimctl_msg.ide	= RT_CAN_STDID;	//标准帧
	gimctl_msg.rtr	= RT_CAN_DTR;	//数据帧
	gimctl_msg.priv = 0;			//报文优先级最高
	gimctl_msg.len = 8;				//长度8

	//控制数据清零
	for(int a = 0;a<8;a++)
	{
		gimctl_msg.data[a] = 0;
	}

	rt_uint8_t angle_time = 0;		//记录速度环次数以执行角度环

	while(1)
	{
		rt_sem_take(&gimbal_1ms_sem, RT_WAITING_FOREVER);

		//计数执行角度环，1-10共10个状态
		if(angle_time > 9)
		{
			angle_time = 0;
		}
		angle_time++;


		//IMU的数据与编码器的数据应该
		//统一零位
		//统一正方向
		//统一单位

		//进行数据转换
		// yawang = (int)((gimbal_atti.yaw + 180.0f)*8192.0f/360.0f);
		// yawpal = (int)((gimbal_atti.yaw_speed)*8192.0f/360.0f);
		// pitchang = (int)((gimbal_atti.pitch + 180.0f)*8192.0f/360.0f);
		// pitchpal = (int)((gimbal_atti.pitch_speed)*8192.0f/360.0f);
        yawang = (int)m_yaw.motordata.angle;
        yawpal = (int)m_yaw.motordata.speed;
        pitchang = (int)m_pitch.motordata.angle;
        pitchpal = (int)m_pitch.motordata.speed;
        //计算云台电机等
		gimbalpid_cal(&m_yaw,yawang,yawpal,angle_time);
		gimbalpid_cal(&m_pitch,pitchang,pitchpal,angle_time);
		//TODO:m_pitch轴的限位

		//在对应位置写电流并发送
		gimctl_msg.data[(rt_uint16_t)(PITCH_ID - 0x205)*2] = m_pitch.palpid.out>>8;
		gimctl_msg.data[(rt_uint16_t)(PITCH_ID - 0x205)*2 + 1] = m_pitch.palpid.out;
		gimctl_msg.data[(rt_uint16_t)(YAW_ID - 0x205)*2] = m_yaw.palpid.out>>8;
		gimctl_msg.data[(rt_uint16_t)(YAW_ID - 0x205)*2 + 1] = m_yaw.palpid.out;

		//如果存在第二个云台电机
		#ifdef DUAL_PITCH_MOTOR
			gimctl_msg.DATA[(rt_uint16_t)(DUAL_PITCH_ID - 0x205)*2] = (-m_yaw.palpid.out)>>8;
			gimctl_msg.DATA[(rt_uint16_t)(DUAL_PITCH_ID - 0x205)*2 + 1] = (-m_yaw.palpid.out);
		#endif

		rt_device_write(can1_dev,0,&gimctl_msg,sizeof(gimctl_msg));
	}
}
/**
* @brief：初始化云台线程
* @param [in]	无
* @return：		1:初始化成功
				0:初始化失败
* @author：mqy
*/
int gimbal_init(void)
{
	//初始化结构体数据
	m_yaw.control_mode = ANGLE;//默认设置位置控制
	m_pitch.control_mode = ANGLE;

	m_yaw.motorID = YAW_ID;
	m_yaw.angdata_source = DJI_MOTOR;//默认数据源陀螺仪
	m_yaw.setang = 0;//初始化默认角度

	m_pitch.motorID = PITCH_ID;
	m_pitch.angdata_source = DJI_MOTOR;//默认数据源陀螺仪
	m_pitch.setang = 0;//初始化默认角度

	//初始化PID
	pid_init(&m_yaw.palpid,10,0.1,0,1000,30000,-30000);
	pid_init(&m_pitch.palpid,10,0.1,0,1000,30000,-30000);

	// pid_init(&m_yaw.angpid_gyro,0.5,0,0,3,20000,-20000);
	pid_init(&m_yaw.angpid_dji,3,0.5,0,10,320,-320);
	// pid_init(&m_pitch.angpid_gyro,2,0,0,3,20000,-20000);
	pid_init(&m_pitch.angpid_dji,3,0.5,0,10,320,-320);

	//初始化中断释放的信号量
	rt_sem_init(&gimbal_1ms_sem, "1ms_sem", 0, RT_IPC_FLAG_FIFO);

	//初始化云台线程
	gimbal_control = rt_thread_create(
	"gimbal_control",		//线程名
	gimbal_contral_thread,	//线程入口
	RT_NULL,				//入口参数无
	2048,					//线程栈
	1,	                    //线程优先级
	1);						//线程时间片大小


	//线程创建失败返回false
	if(gimbal_control == RT_NULL)
	{
		return 0;
	}

    //创建线程定时器
	rt_timer_init(&task_1ms,
                "1ms_task",
                task_1ms_IRQHandler,
				RT_NULL,
                1,
                RT_TIMER_FLAG_PERIODIC | RT_TIMER_FLAG_SOFT_TIMER);

	//线程启动失败返回false
	if(rt_thread_startup(gimbal_control) != RT_EOK)
	{
		return 0;
	}

    //启动定时器
	if(rt_timer_start(&task_1ms) != RT_EOK)
	{
		return 0;
	}

	return 1;
}
/**
* @brief：refresh_motor_data函数会使用该函数来更新数据
* @param [in]	message:接收到的数据帧指针
				motordata:要更新的电机数据结构体指针
* @return：		无
* @author：mqy
*/
static void assign_motor_data(motordata_t* motordata,struct rt_can_msg* message)
{
	motordata->angle = (message->data[0]<<8) + message->data[1];	//转子角度
	motordata->speed = (message->data[2]<<8) + message->data[3];	//转子转速
	motordata->current = (message->data[4]<<8) + message->data[5];  //实际电流
	motordata->temperature = message->data[6];						//温度
}
/**
* @brief：利用该函数更新云台电机相关数据
* @param [in]	message:接收到的数据帧指针
* @return：		true:更新成功
				false:id不匹配更新失败
* @author：mqy
*/
int refresh_gimbal_motor_data(struct rt_can_msg* message)
{
	//其他数据
	switch(message->id)
	{
		case YAW_ID:
			assign_motor_data(&m_yaw.motordata,message);
			//转换角度数值的坐标系
			if(m_yaw.motordata.angle < YAW_ZERO_ANGLE)
			{
				m_yaw.motordata.angle = 8191 - YAW_ZERO_ANGLE + m_yaw.motordata.angle;
			}
			else
			{
				m_yaw.motordata.angle = m_yaw.motordata.angle - YAW_ZERO_ANGLE;
			}
			rt_kprintf("ID:%x Speed: %d Angle: %d  angset: %d angout: %d palset: %d palout: %d ", m_yaw.motorID, m_yaw.motordata.speed,
					   m_yaw.motordata.angle, m_yaw.angpid_dji.set, m_yaw.angpid_dji.out,m_yaw.palpid.set, m_yaw.palpid.out);
			rt_kprintf("\n");
			return 1;

		case PITCH_ID:
			assign_motor_data(&m_pitch.motordata,message);
			return 1;

		default:

			break;
	}
	return 0;
}

/**
* @brief：直接设置云台绝对角度
* @param [in]	yawset：m_yaw轴设置角度，（值有效范围0-8191，若超出范围会被限制至范围内
				pitchset：m_pitch设置角度，（值有效范围 PITCH_MIN_ANGLE - PITCH_MAX_ANGLE
* @return：		m_yaw轴角度，格式0-8191
* @author：mqy
*/
int gimbal_absangle_set(rt_uint16_t yawset,rt_uint16_t pitchset)
{
	m_yaw.setang = yawset;
	m_pitch.setang = pitchset;

	return 1;
}
/**
* @brief：增量式设置云台角度
* @param [in]	yawset：m_yaw轴设置角度，（值有效范围0-8191，若超出范围会被限制至范围内
				pitchset：m_pitch设置角度，（值有效范围 PITCH_MIN_ANGLE - PITCH_MAX_ANGLE
* @return：		m_yaw轴角度，格式0-8191
* @author：mqy
*/
int gimbal_addangle_set(rt_int16_t yawset,rt_int16_t pitchset)
{
	m_yaw.setang += yawset;
	m_pitch.setang += pitchset;

	m_yaw.setang %= 8192;
	m_pitch.setang %= 8192;

	return 1;
}
/**
* @brief：云台控制模式设置
* @param [in]	yawset：m_yaw轴控制模式
				pitchset：m_pitch轴控制模式
* @return：		m_yaw轴角度，格式0-8191
* @author：mqy
*/
int gimbal_ctlmode_set(control_mode_t yawset,control_mode_t pitchset)
{
	m_yaw.control_mode = yawset;
	m_pitch.control_mode = pitchset;
	//TODO:保证在从一种模式切换为另一种模式时，云台不会突然转动一个角度
	return 1;
}
/**
* @brief：设置加速度
* @param [in]	yawset：m_yaw轴设置角度，（值有效范围0-8191，若超出范围会被限制至范围内
				pitchset：m_pitch设置角度，（值有效范围 PITCH_MIN_ANGLE - PITCH_MAX_ANGLE
* @return：		m_yaw轴角度，格式0-8191
* @author：mqy
*/
int gimbal_palstance_set(rt_uint16_t yawset,rt_uint16_t pitchset)
{
	if(m_yaw.control_mode == PALSTANCE)
	{
		m_yaw.palpid.set = yawset;
	}
	if(m_pitch.control_mode == PALSTANCE)
	{
		m_pitch.palpid.set = pitchset;
	}
	return 1;
}
/**
* @brief：设置云台数据元
* @param [in]	yawset：m_yaw轴数据源，可取值见枚举
				pitchset：m_pitch轴数据源，可取值见枚举
* @return：		m_yaw轴角度，格式0-8191
* @author：mqy
*/
int angle_datasource_set(data_source_t yawset,data_source_t pitchset)
{
	m_yaw.angdata_source = yawset;
	m_pitch.angdata_source = pitchset;

	return 1;
}
/**
* @brief：获取m_yaw轴角度
* @param [in]	data_source:希望的数据源
* @return：		m_yaw轴角度，格式0-8191
* @author：mqy
*/
int get_yawangle(void)
{
	return m_yaw.motordata.angle;
}
/**
* @brief：获取m_pitch轴角度
* @param [in]	data_source:希望的数据源
* @return：		m_pitch轴角度，格式0-8191
* @author：mqy
*/
int get_pitchangle(void)
{
	return m_pitch.motordata.angle;
}
