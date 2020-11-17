#include "drv_remote.h"

#define SAMPLE_UART_NAME       "uart1"      /* 串口设备名称 */

/* 串口接收消息结构*/
struct rx_msg
{
    rt_device_t dev;
    rt_size_t size;
};
/* 串口设备句柄 */
static rt_device_t serial;
/* 消息队列控制块 */
static struct rt_messagequeue rx_mq;

uint8_t rx_buffer[RT_SERIAL_RB_BUFSZ + 1];

/* 存储遥控器当前数据 */
RC_Ctrl_t RC_Ctrl_Data;
/*存储遥控器上次数据 */
static RC_Ctrl_t RC_Ctrl_Data_last;

void RCReadKeyBoard_Data(RC_Ctrl_t *RC_CtrlData)
{
		RC_CtrlData->KeyBoard_Data.Key_Data.W = (RC_CtrlData->KeyBoard_Data.v&0x0001)==0x0001;
		RC_CtrlData->KeyBoard_Data.Key_Data.S = (RC_CtrlData->KeyBoard_Data.v&0x0002)==0x0002;
		RC_CtrlData->KeyBoard_Data.Key_Data.A = (RC_CtrlData->KeyBoard_Data.v&0x0004)==0x0004;
		RC_CtrlData->KeyBoard_Data.Key_Data.D = (RC_CtrlData->KeyBoard_Data.v&0x0008)==0x0008;
		RC_CtrlData->KeyBoard_Data.Key_Data.shift = (RC_CtrlData->KeyBoard_Data.v&0x0010)==0x0010;
		RC_CtrlData->KeyBoard_Data.Key_Data.ctrl = (RC_CtrlData->KeyBoard_Data.v&0x0020)==0x0020;
		RC_CtrlData->KeyBoard_Data.Key_Data.Q = (RC_CtrlData->KeyBoard_Data.v&0x0040)==0x0040;
		RC_CtrlData->KeyBoard_Data.Key_Data.E = (RC_CtrlData->KeyBoard_Data.v&0x0080)==0x0080;
		RC_CtrlData->KeyBoard_Data.Key_Data.R = (RC_CtrlData->KeyBoard_Data.v&0x0100)==0x0100;
		RC_CtrlData->KeyBoard_Data.Key_Data.F = (RC_CtrlData->KeyBoard_Data.v&0x0200)==0x0200;
		RC_CtrlData->KeyBoard_Data.Key_Data.G = (RC_CtrlData->KeyBoard_Data.v&0x0400)==0x0400;
		RC_CtrlData->KeyBoard_Data.Key_Data.Z = (RC_CtrlData->KeyBoard_Data.v&0x0800)==0x0800;
		RC_CtrlData->KeyBoard_Data.Key_Data.X = (RC_CtrlData->KeyBoard_Data.v&0x1000)==0x1000;
		RC_CtrlData->KeyBoard_Data.Key_Data.C = (RC_CtrlData->KeyBoard_Data.v&0x2000)==0x2000;
		RC_CtrlData->KeyBoard_Data.Key_Data.V = (RC_CtrlData->KeyBoard_Data.v&0x4000)==0x4000;
		RC_CtrlData->KeyBoard_Data.Key_Data.B = (RC_CtrlData->KeyBoard_Data.v&0x8000)==0x8000;
}
void RemoteDataProcess(uint8_t *pData, RC_Ctrl_t *RC_CtrlData)
{
	int i = 0;
	uint8_t s1 = 0;
	uint8_t s2 = 0;
	uint16_t ch0 = 0;
	uint16_t ch1 = 0;
	uint16_t ch2 = 0;
	uint16_t ch3 = 0;
    if(pData==NULL)
	{
	   return;
	}

	s1=((pData[5]>>4)&0x000c)>>2;
	s2=((pData[5]>>4)&0x0003);
	ch0=((int16_t)pData[0]|((int16_t)pData[1]<<8))&0x07FF;
	ch1=(((int16_t)pData[1]>>3)|((int16_t)pData[2]<<5))&0x07FF;
	ch2=(((int16_t)pData[2]>>6)|((int16_t)pData[3]<<2)|((int16_t)pData[4]<<10))&0x07FF;
	ch3=(((int16_t)pData[4]>>1)|((int16_t)pData[5]<<7))&0x07FF;
	if(
		((s1==1) || (s1==2) || (s1==3)) &&
		((s2==1) || (s2==2) || (s2==3))&&
		((pData[i+12]==0)||(pData[i+12]==1))&&
		((pData[i+13]==0)||(pData[i+13]==1))&&
		(ch0<1684)&&(ch0>364)&&
		(ch1<1684)&&(ch1>364)&&
		(ch2<1684)&&(ch2>364)&&
		(ch3<1684)&&(ch3>364)
		)
	{
		RC_CtrlData->Remote_Data.ch0=ch0;
		RC_CtrlData->Remote_Data.ch1=ch1;
		RC_CtrlData->Remote_Data.ch2=ch2;
		RC_CtrlData->Remote_Data.ch3=ch3;
		RC_CtrlData->Remote_Data.s1=s1;
		RC_CtrlData->Remote_Data.s2=s2;
		RC_CtrlData->Mouse_Data.x_speed=((int16_t)pData[6])|((int16_t)pData[7]<<8);
		RC_CtrlData->Mouse_Data.y_speed=((int16_t)pData[8])|((int16_t)pData[9]<<8);
		RC_CtrlData->Mouse_Data.z_speed=((int16_t)pData[10])|((int16_t)pData[11]<<8);
		RC_CtrlData->Mouse_Data.press_l=pData[12];
		RC_CtrlData->Mouse_Data.press_r=pData[13];
		RC_CtrlData->KeyBoard_Data.v=((int16_t)pData[14])|((int16_t)pData[15]<<8);

		RCReadKeyBoard_Data(RC_CtrlData);
	}
}


/* 接收数据回调函数 */
static rt_err_t uart_input(rt_device_t dev, rt_size_t size)
{
    struct rx_msg msg;
    rt_err_t result;
    msg.dev = dev;
    msg.size = size;

    result = rt_mq_send(&rx_mq, &msg, sizeof(msg));
    // if ( result == -RT_EFULL)
    // {
    //     /* 消息队列满 */
    //     rt_kprintf("message queue full！\n");
    // }
    return result;
}


static void serial_thread_entry(void *parameter)
{
    struct rx_msg msg;
    rt_err_t result;
    rt_uint32_t rx_length;
	rt_int16_t yawset, pitchset;
	//初始化
	RC_Ctrl_Data.Remote_Data.ch0 = 1024;
	RC_Ctrl_Data.Remote_Data.ch1 = 1024;
	RC_Ctrl_Data.Remote_Data.ch2 = 1024;
	RC_Ctrl_Data.Remote_Data.ch3 = 1024;
	RC_Ctrl_Data.Remote_Data.s1 = 3;
	RC_Ctrl_Data.Remote_Data.s2 = 3;
	RC_Ctrl_Data_last.Remote_Data.ch0 = 1024;
	RC_Ctrl_Data_last.Remote_Data.ch1 = 1024;
	RC_Ctrl_Data_last.Remote_Data.ch2 = 1024;
	RC_Ctrl_Data_last.Remote_Data.ch3 = 1024;
	RC_Ctrl_Data_last.Remote_Data.s1 = 3;
	RC_Ctrl_Data_last.Remote_Data.s2 = 3;

    while (1)
    {
        rt_memset(&msg, 0, sizeof(msg));
        /* 从消息队列中读取消息*/
        result = rt_mq_recv(&rx_mq, &msg, sizeof(msg), RT_WAITING_FOREVER);
        if (result == RT_EOK)
        {
			/*记录上次数据*/
			rt_memcpy(&RC_Ctrl_Data_last,&RC_Ctrl_Data,sizeof(RC_Ctrl_Data));

            /* 从串口读取数据*/
            rx_length = rt_device_read(msg.dev, 0, rx_buffer, msg.size);

			RemoteDataProcess(rx_buffer,&RC_Ctrl_Data);
			if(RC_Ctrl_Data.Remote_Data.s2!=1)
			{
				yawset = (RC_Ctrl_Data.Remote_Data.ch0 - 364) * 8192 / 660;
				pitchset = (RC_Ctrl_Data.Remote_Data.ch1 - 1024) * (PITCH_ZERO_ANGLE-PITCH_MIN_ANGLE) / 660 +PITCH_ZERO_ANGLE;
				if(pitchset>PITCH_MAX_ANGLE)
					pitchset = PITCH_MAX_ANGLE;
				//rt_kprintf("s1: %d x: %d y: %d z:%d", RC_Ctrl_Data.Remote_Data.s1,RC_Ctrl_Data.Mouse_Data.x_speed, RC_Ctrl_Data.Mouse_Data.y_speed, RC_Ctrl_Data.Mouse_Data.z_speed);
				gimbal_absangle_set(yawset, pitchset);
			}
			if(RC_Ctrl_Data.Remote_Data.s2==1)
			{
				yawset = (RC_Ctrl_Data.Mouse_Data.x_speed) * 8192 / 32768;
				pitchset = (RC_Ctrl_Data.Mouse_Data.y_speed) * 8192 / 32768;
				// rt_kprintf("x: %d y: %d z:%d\n", RC_Ctrl_Data.Mouse_Data.x_speed, RC_Ctrl_Data.Mouse_Data.y_speed, RC_Ctrl_Data.Mouse_Data.z_speed);
				gimbal_addangle_set(yawset, pitchset);
			}

		}
    }
}

int remote_uart_init(void)
{
    rt_err_t ret = RT_EOK;
    char uart_name[RT_NAME_MAX];
    static char msg_pool[256];
    char str[] = "hello RT-Thread!\r\n";

    rt_strncpy(uart_name, SAMPLE_UART_NAME, RT_NAME_MAX);

    /* 查找串口设备 */
    struct serial_configure config = RT_SERIAL_CONFIG_DEFAULT;  /* 初始化配置参数 */

	/* step1：查找串口设备 */
	serial = rt_device_find(SAMPLE_UART_NAME);

	/* step2：修改串口配置参数 */
	config.baud_rate = 100000;        		//修改波特率为 9600
	config.data_bits = DATA_BITS_8;			//数据位 8
	config.stop_bits = STOP_BITS_1;			//停止位 1
	config.bufsz     = 128;         		//修改缓冲区 buff size 为 128
	config.parity    = PARITY_EVEN;     	//无奇偶校验位

	/* step3：控制串口设备。通过控制接口传入命令控制字，与控制参数 */
	rt_device_control(serial, RT_DEVICE_CTRL_CONFIG, &config);

    /* 初始化消息队列 */
    rt_mq_init(&rx_mq, "rx_mq",
               msg_pool,                 /* 存放消息的缓冲区 */
               sizeof(struct rx_msg),    /* 一条消息的最大长度 */
               sizeof(msg_pool),         /* 存放消息的缓冲区大小 */
               RT_IPC_FLAG_FIFO);        /* 如果有多个线程等待，按照先来先得到的方法分配消息 */

    /* 以 DMA 接收及轮询发送方式打开串口设备 */
    rt_device_open(serial, RT_DEVICE_FLAG_DMA_RX);
    /* 设置接收回调函数 */
    rt_device_set_rx_indicate(serial, uart_input);
    /* 发送字符串 */
    // rt_device_write(serial, 0, str, (sizeof(str) - 1));

    /* 创建 serial 线程 */
    rt_thread_t thread = rt_thread_create("serial", serial_thread_entry, RT_NULL, 1024, 2, 1);
    /* 创建成功则启动线程 */
    if (thread != RT_NULL)
    {
        rt_thread_startup(thread);
    }
    else
    {
        ret = RT_ERROR;
    }

    return ret;
}

