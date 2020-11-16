#include "can_motor.h"


#define CAN_DEV_NAME       "can1"      /* CAN 设备名称 */
#define GM6020_ID                1
#define C620_ID                1
static struct rt_semaphore rx_sem[2];     /* 用于接收消息的信号量 */
static rt_device_t can_dev[2];            /* CAN 设备句柄 */
extern RC_Ctrl_t remote_rec;
motor_id_t motor_id[2];
Motor_t motor_rec[2];
pid_t pid[2]={0};
float pid_temp[5] = {2, 1, 0,1000,-1000};

/* 接收数据回调函数 */
static rt_err_t can_rx_call(rt_device_t dev, rt_size_t size)
{
    /* CAN 接收到数据后产生中断，调用此回调函数，然后发送接收信号量 */
    if(rt_strncmp(dev->parent.name, "can1", RT_NAME_MAX) == 0)
    rt_sem_release(&rx_sem[0]);
    if(rt_strncmp(dev->parent.name, "can2", RT_NAME_MAX) == 0)
    rt_sem_release(&rx_sem[1]);
    return RT_EOK;
}

static void can_rx_thread(void *parameter)
{
    rt_err_t res;
    //rt_int32_t level;
    struct rt_can_msg rxmsg[2] = {0};
    rt_int32_t canx = (rt_int32_t)parameter;
    motor_init(&motor_rec[canx], motor_id[canx].id_rx, 1000);
    // if(canx==0)
    //     PID_Init(&pid[canx], 5, 5, 1, 30000, -30000);
    // if (canx==1)
    //     PID_Init(&pid[canx], pid_temp[0], pid_temp[1], pid_temp[2], pid_temp[3],pid_temp[4]);

    /* 设置接收回调函数 */
    rt_device_set_rx_indicate(can_dev[canx], can_rx_call);
#ifdef RT_CAN_USING_HDR
    struct rt_can_filter_item items[1] =
        {
            RT_CAN_FILTER_ITEM_INIT(motor_id[canx].id_rx, 0, 0, 1, 0x7FF, RT_NULL, RT_NULL)};
    struct rt_can_filter_config config = {1, 1, items};
    /* 设置硬件过滤表 */
    res = rt_device_control(can_dev[canx], RT_CAN_CMD_SET_FILTER, &config);
    RT_ASSERT(res == RT_EOK);
#endif
    while (1)
    {
        /* hdr 值为 - 1，表示直接从 uselist 链表读取数据 */
        rxmsg[canx].hdr = -1;
        /* 阻塞等待接收信号量 */
        rt_sem_take(&rx_sem[canx], RT_WAITING_FOREVER);
        /* 从 CAN 读取一帧数据 */

        rt_device_read(can_dev[canx], 0, &rxmsg[canx], sizeof(rxmsg[canx]));
        motor_readmsg(&rxmsg[canx], &motor_rec[canx].dji);

        /* 打印数据 ID 及内容 */
        // if(canx==1)
        // {
        // rt_kprintf("ID:%x Speed: %d Angle: %d Cur: %d Temp: %d out: %d set: %d ",
        //            rxmsg[canx].id, motor_rec[canx].dji.speed, motor_rec[canx].dji.angle, motor_rec[canx].dji.current, motor_rec[canx].dji.temperature, pid[canx].output, pid[canx].set);
        // rt_kprintf("\n");
        // }

    }
}

static void can_tx_thread(void *parameter)
{
    rt_size_t size;
    rt_int16_t data[2];
    rt_int32_t canx = (rt_int32_t)parameter;
    while (1)
    {
        if(canx==0)
        data[0] = (remote_rec.Remote_Data.ch1-1024)*320/660;
        if(canx==1)
        data[1] = (remote_rec.Remote_Data.ch3-1024)*16384/660;
        pid_output_calculate(&pid[canx], motor_rec[canx].dji.speed, data[canx]);
        size = motor_current_send(can_dev[canx], motor_id[canx].id_tx, pid[canx].out, 0, 0, 0);
        // size = motor_current_send(can_dev[canx], motor_id[canx].id_tx, data[canx], 0, 0, 0);
        if (size == 0)
        {
            rt_kprintf("can dev write data failed!\n");
        }
        rt_thread_mdelay(1);
    }
}

int can_sample(void)
{
    rt_err_t res;
    rt_thread_t thread1, thread2, thread3, thread4;
    motor_id[0].id_tx = 0x1ff;
    motor_id[0].id_rx = 0x204+GM6020_ID;
    motor_id[1].id_tx = 0x200;
    motor_id[1].id_rx = 0x200+C620_ID;
    /* 查找 CAN 设备 */
    can_dev[0] = rt_device_find("can1");
    can_dev[1] = rt_device_find("can2");
    if (!can_dev[0])
    {
        rt_kprintf("find %s failed!\n", "can1");
        return RT_ERROR;
    }
    if (!can_dev[1])
    {
        rt_kprintf("find %s failed!\n", "can2");
        return RT_ERROR;
    }
    /* 初始化 CAN 接收信号量 */
    rt_sem_init(&rx_sem[0], "rx_sem0", 0, RT_IPC_FLAG_FIFO);
    rt_sem_init(&rx_sem[1], "rx_sem1", 0, RT_IPC_FLAG_FIFO);
    /* 以中断接收及发送方式打开 CAN 设备 */
    res = rt_device_open(can_dev[0], RT_DEVICE_FLAG_INT_TX | RT_DEVICE_FLAG_INT_RX);
    res = rt_device_control(can_dev[0], RT_CAN_CMD_SET_BAUD, (void *)CAN1MBaud);
    RT_ASSERT(res == RT_EOK);
    /* 以中断接收及发送方式打开 CAN 设备 */
    res = rt_device_open(can_dev[1], RT_DEVICE_FLAG_INT_TX | RT_DEVICE_FLAG_INT_RX);
    res = rt_device_control(can_dev[1], RT_CAN_CMD_SET_BAUD, (void *)CAN1MBaud);
    RT_ASSERT(res == RT_EOK);

    /* 创建数据接收线程 */
    thread1 = rt_thread_create("can_rx", can_rx_thread, (void *)0, 1024, 10, 10);
    if (thread1 != RT_NULL)
    {
        rt_thread_startup(thread1);
    }
    thread2 = rt_thread_create("can_tx", can_tx_thread, (void *)0, 1024, 7, 10);
    if (thread2 != RT_NULL)
    {
        rt_thread_startup(thread2);
    }
        /* 创建数据接收线程 */
    thread3 = rt_thread_create("can_rx", can_rx_thread, (void *)1, 1024, 11, 10);
    if (thread1 != RT_NULL)
    {
        rt_thread_startup(thread3);
    }
    thread4 = rt_thread_create("can_tx", can_tx_thread, (void *)1, 1024, 8, 10);
    if (thread2 != RT_NULL)
    {
        rt_thread_startup(thread4);
    }
    return res;
}
/* 导出到 msh 命令列表中 */
MSH_CMD_EXPORT(can_sample, can device sample);

