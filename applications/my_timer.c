#include "my_timer.h"

/* 定时器的控制块 */
static struct rt_timer timer1;
static struct rt_timer timer2;
static int cnt = 0;

/* 定时器 1 超时函数 */
void timeout1(void* parameter)
{
    // rt_kprintf("periodic timer is timeout\n");
    cnt++;
    /* 运行 10 次 */

        // rt_timer_stop(&timer1);

}

/* 定时器 2 超时函数 */
void timeout2(void* parameter)
{
    cnt++;
    cnt++;
    // rt_kprintf("one shot timer is timeout\n");
}

int timer_static_sample(void)
{
    /* 初始化定时器 */
    rt_timer_init(&timer1, "timer1",  /* 定时器名字是 timer1 */
                    timeout1, /* 超时时回调的处理函数 */
                    RT_NULL, /* 超时函数的入口参数 */
                    3, /* 定时长度，以 OS Tick 为单位，即 10 个 OS Tick */
                    RT_TIMER_FLAG_PERIODIC|RT_TIMER_FLAG_SOFT_TIMER); /* 周期性定时器 */
    rt_timer_init(&timer2, "timer2",   /* 定时器名字是 timer2 */
                    timeout2, /* 超时时回调的处理函数 */
                      RT_NULL, /* 超时函数的入口参数 */
                      30, /* 定时长度为 30 个 OS Tick */
                    RT_TIMER_FLAG_PERIODIC|RT_TIMER_FLAG_SOFT_TIMER); /* 单次定时器 */

    /* 启动定时器 */
    rt_timer_start(&timer1);
    // rt_timer_start(&timer2);
    return 0;
}
/* 导出到 msh 命令列表中 */
MSH_CMD_EXPORT(timer_static_sample, timer_static sample);



#define THREAD_PRIORITY         25
#define THREAD_STACK_SIZE       512
#define THREAD_TIMESLICE        5
/* 指向信号量的指针 */
rt_sem_t sem_food;
/* 线程1入口 */
void thread1_entry(void* parameter)
{
    while (1)
    {
        /* 线程1第一次运行 */
        rt_kprintf("thread1 is run!\n");
        /* 释放一次信号量 */
        rt_sem_release(sem_food);
        /* 线程1第二次运行 */
        rt_kprintf("thread1 run again!\n");
        /* 线程1延时1秒 */
        rt_thread_delay(RT_TICK_PER_SECOND);
    }
}
/* 线程2入口 */
void thread2_entry(void* parameter)
{
    while (1)
    {
        /* 试图持有信号量，并永远等待直到持有到信号量 */
        rt_sem_take(sem_food, RT_WAITING_FOREVER);
        /* 线程2正在运行 */
        rt_kprintf("thread2 is run!\n");
    }
}
/* DEMO初始化函数 */
void demo_init(void)
{
    /* 指向线程控制块的指针 */
    rt_thread_t thread1_id, thread2_id;
    /* 创建一个信号量，初始值是0 */
    sem_food = rt_sem_create("sem_food", 0, RT_IPC_FLAG_FIFO);
    if (sem_food == RT_NULL)
    {
        rt_kprintf("sem created fail!\n");
        return ;
    }
    /* 创建线程1 */
    thread1_id = rt_thread_create("thread1",
                    thread1_entry, RT_NULL,/* 线程入口是thread1_entry, 参数RT_NULL */
                    THREAD_STACK_SIZE, THREAD_PRIORITY, THREAD_TIMESLICE);
    if (thread1_id != RT_NULL)
        rt_thread_startup(thread1_id);
    /* 创建线程2 */
    thread2_id = rt_thread_create("thread2",
                    thread2_entry, RT_NULL,/* 线程入口是thread2_entry, 参数RT_NULL */
                    THREAD_STACK_SIZE, THREAD_PRIORITY - 1, THREAD_TIMESLICE);
    if (thread2_id != RT_NULL)
        rt_thread_startup(thread2_id);
}
