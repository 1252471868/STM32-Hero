#include "drv_canthread.h"

#define CAN1_DEV_NAME		"can1"				//can设备名称
#define CAN2_DEV_NAME		"can2"

static struct rt_semaphore can1_rx_sem;     	//用于接收消息的信号量
rt_device_t can1_dev;            		//CAN 设备句柄

static struct rt_semaphore can2_rx_sem;     	//用于接收消息的信号量
rt_device_t can2_dev;            		//CAN 设备句柄

//用户需要在task_create里重新定义这两个函数

//can1数据接收函数
extern void can1_rec(struct rt_can_msg *msg);
//can2数据接收函数
extern void can2_rec(struct rt_can_msg *msg);

/**
 * @brief  can1接收回调
 * @retval RT_EOK
 */
static rt_err_t can1_rx_call(rt_device_t dev, rt_size_t size)
{
    //CAN 接收到数据后产生中断，调用此回调函数，然后发送接收信号量
    rt_sem_release(&can1_rx_sem);
    return RT_EOK;
}
/**
 * @brief  can1读取线程
 */
static void can1_rx_thread(void *parameter)
{
    struct rt_can_msg rxmsg = {0};

    while (1)
    {
        //hdr 值为 - 1，表示直接从 uselist 链表读取数据
        rxmsg.hdr = -1;
        //阻塞等待接收信号量
        rt_sem_take(&can1_rx_sem, RT_WAITING_FOREVER);
        //从 CAN 读取一帧数据
        rt_device_read(can1_dev, 0, &rxmsg, sizeof(rxmsg));
		can1_rec(&rxmsg);
    }
}
/**
 * @brief  can2接收回调
 * @retval RT_EOK
 */
static rt_err_t can2_rx_call(rt_device_t dev, rt_size_t size)
{
    //CAN 接收到数据后产生中断，调用此回调函数，然后发送接收信号量
    rt_sem_release(&can2_rx_sem);
    return RT_EOK;
}
/**
 * @brief  can2读取线程
 */
static void can2_rx_thread(void *parameter)
{
    struct rt_can_msg rxmsg = {0};

    while (1)
    {
        //hdr 值为 - 1，表示直接从 uselist 链表读取数据
		rxmsg.hdr = -1;
        //阻塞等待接收信号量
		rt_sem_take(&can2_rx_sem, RT_WAITING_FOREVER);
		//从 CAN 读取一帧数据
		rt_device_read(can2_dev, 0, &rxmsg, sizeof(rxmsg));
		can2_rec(&rxmsg);
    }
}


/**
 * @brief  can1初始化，can1数据处理线程和中断设定
 * @param  None
 * @retval rt_err_t
 */
int can1_init(void)
{
	rt_err_t res=0;
	rt_thread_t thread;
	can1_dev = rt_device_find(CAN1_DEV_NAME);
	if (!can1_dev)
	{
		return RT_ERROR;
	}
	//配置can驱动
	res = rt_device_open(can1_dev, RT_DEVICE_FLAG_INT_TX | RT_DEVICE_FLAG_INT_RX);
	RT_ASSERT(res == RT_EOK);
	res = rt_device_control(can1_dev, RT_CAN_CMD_SET_MODE, (void *)RT_CAN_MODE_NORMAL);
	res = rt_device_control(can1_dev, RT_CAN_CMD_SET_BAUD, (void *)CAN1MBaud);
	//设置接收回调函数
	rt_device_set_rx_indicate(can1_dev, can1_rx_call);
	//can接收中断信号量
	rt_sem_init(&can1_rx_sem, "can1_sem", 0, RT_IPC_FLAG_FIFO);
	thread = rt_thread_create("can1_rx", can1_rx_thread, RT_NULL, 512, 2, 1);
	if (thread != RT_NULL)
	{
		rt_thread_startup(thread);
	}
	return res;
}
// INIT_APP_EXPORT(can1_init);
/**
 * @brief  can2初始化，can2数据处理线程和中断设定
 * @param  None
 * @retval rt_err_t
 */
int can2_init(void)
{
	rt_err_t res=0;
	rt_thread_t thread;
	can2_dev = rt_device_find(CAN2_DEV_NAME);
	if (!can2_dev)
	{
		return RT_ERROR;
	}
	//配置can驱动
	res = rt_device_open(can2_dev, RT_DEVICE_FLAG_INT_TX | RT_DEVICE_FLAG_INT_RX);
	RT_ASSERT(res == RT_EOK);
	res = rt_device_control(can2_dev, RT_CAN_CMD_SET_MODE, (void *)RT_CAN_MODE_NORMAL);
	res = rt_device_control(can2_dev, RT_CAN_CMD_SET_BAUD, (void *)CAN1MBaud);
	//设置接收回调函数
	rt_device_set_rx_indicate(can2_dev, can2_rx_call);
	//can接收中断信号量
	rt_sem_init(&can2_rx_sem, "can2_sem", 0, RT_IPC_FLAG_FIFO);
	thread = rt_thread_create("can2_rx", can2_rx_thread, RT_NULL, 512, 2, 1);
	if (thread != RT_NULL)
	{
		rt_thread_startup(thread);
	}
	return res;
}
// INIT_APP_EXPORT(can2_init);
