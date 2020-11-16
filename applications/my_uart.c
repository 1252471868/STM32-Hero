#include "my_uart.h"

RC_Ctrl_t remote_rec;
/* 用于接收消息的信号量 */
struct rx_msg
{
    rt_device_t dev;
    rt_size_t size;
};
static struct rt_semaphore rx_sem;
static rt_device_t serial,console;
static struct rt_messagequeue rx_mq;



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
    static char rx_buffer[RT_SERIAL_RB_BUFSZ + 1];

    while (1)
    {
        rt_memset(&msg, 0, sizeof(msg));
        /* 从消息队列中读取消息*/
        result = rt_mq_recv(&rx_mq, &msg, sizeof(msg), RT_WAITING_FOREVER);
        if (result == RT_EOK)
        {
            /* 从串口读取数据*/
            rx_length = rt_device_read(msg.dev, 0, rx_buffer, msg.size);
            rx_buffer[rx_length] = '\0';
            RemoteDataProcess(rx_buffer, &remote_rec);

            /* 通过串口设备 serial 输出读取到的消息 */
            // rt_device_write(console, 0, rx_buffer, rx_length);
            /* 打印数据 */
            // rt_kprintf("ch0:%d ch1:%d ch2:%d ch3:%d s1:%d s2:%d \n",remote_rec.Remote_Data.ch0,remote_rec.Remote_Data.ch1,remote_rec.Remote_Data.ch2,remote_rec.Remote_Data.ch3,remote_rec.Remote_Data.s1,\
            //                 remote_rec.Remote_Data.s2);
        }
    }
}


int  uart_dma_sample(void)
{
    rt_err_t res;
    struct serial_configure config = RT_SERIAL_CONFIG_DEFAULT;
    serial = rt_device_find("uart1");
    console = rt_device_find("uart8");
    static char msg_pool[256];
    config.baud_rate = 100000;
    config.bufsz = 64;
    config.parity = PARITY_EVEN;

    rt_device_control(serial, RT_DEVICE_CTRL_CONFIG, &config);
    /* 初始化消息队列 */
    rt_mq_init(&rx_mq, "rx_mq",
               msg_pool,                 /* 存放消息的缓冲区 */
               sizeof(struct rx_msg),    /* 一条消息的最大长度 */
               sizeof(msg_pool),         /* 存放消息的缓冲区大小 */
               RT_IPC_FLAG_FIFO);        /* 如果有多个线程等待，按照先来先得到的方法分配消息 */

    /* 以 DMA 接收及轮询发送方式打开串口设备 */
    res = rt_device_open(serial, RT_DEVICE_FLAG_DMA_RX);
    /* 设置接收回调函数 */
    res = rt_device_set_rx_indicate(serial, uart_input);

    /* 创建 serial 线程 */
    rt_thread_t thread = rt_thread_create("serial", serial_thread_entry, RT_NULL, 1024, 5, 10);
    /* 创建成功则启动线程 */
    if (thread != RT_NULL)
    {
        rt_thread_startup(thread);
    }
    return res;
}
/* 导出到 msh 命令列表中 */
MSH_CMD_EXPORT(uart_dma_sample, uart device dma sample);

