#include "my_pin.h"

/* defined the LED0 pin: PB1 */
#define RGB1_PIN    GET_PIN(A, 2)
#define RGB2_PIN    GET_PIN(E, 5)
#define RGB3_PIN    GET_PIN(E, 6)
#define KEY_PIN    GET_PIN(C, 5)

#define THREAD_PRIORITY         25  //线程优先级（rtconfig.h 中的 RT_THREAD_PRIORITY_MAX 宏定义优先级范围）
#define THREAD_STACK_SIZE       512 //线程栈大小，单位是字节
#define THREAD_TIMESLICE        5   //同优先级线程时间片大小

rt_bool_t RGB3_flag=PIN_LOW;

rt_thread_t tid1 = RT_NULL;
rt_thread_t tid2 = RT_NULL;



static void Thread_RGB1_Entry(void *para)
{
    while(1)
    {
        rt_pin_write(RGB1_PIN, PIN_HIGH);
        rt_thread_mdelay(500);
        rt_pin_write(RGB1_PIN, PIN_LOW);
        rt_thread_mdelay(500);
    }

}
static void Thread_RGB2_Entry(void *para)
{
    while(1)
    {
        rt_pin_write(RGB2_PIN, PIN_HIGH);
        rt_thread_mdelay(250);
        // rt_pin_write(RGB2_PIN, PIN_LOW);
        // rt_thread_mdelay(250);
    }

}
void RGB3_on_off(void *args)
{
    rt_kprintf("Change LED!\n");
    RGB3_flag = ~RGB3_flag;
    rt_pin_write(RGB3_PIN, RGB3_flag);
}
rt_uint32_t PWM2_freq = 2000;
rt_uint32_t PWM2_duty = 20;

int my_Pin_Init(void)
{
    /* set LED0 pin mode to output */
    rt_pin_mode(RGB1_PIN, PIN_MODE_OUTPUT);
    rt_pin_mode(RGB2_PIN, PIN_MODE_OUTPUT);
    rt_pin_mode(RGB3_PIN, PIN_MODE_OUTPUT);
    rt_pin_mode(KEY_PIN, PIN_MODE_INPUT);
    rt_pin_attach_irq(KEY_PIN, PIN_IRQ_MODE_RISING, RGB3_on_off, RT_NULL);
    rt_pin_irq_enable(KEY_PIN, PIN_IRQ_ENABLE);

    tid1 = rt_thread_create("RGB1", Thread_RGB1_Entry, RT_NULL, THREAD_STACK_SIZE, 30, THREAD_TIMESLICE);
    tid2=rt_thread_create("RGB2", Thread_RGB2_Entry, RT_NULL, THREAD_STACK_SIZE, 2, THREAD_TIMESLICE);
       if (tid1 != RT_NULL)
        rt_thread_startup(tid1);
        else
            return -1;
        if (tid2 != RT_NULL)
            rt_thread_startup(tid2);
            else
                return -1;
		return 0;
}
MSH_CMD_EXPORT(my_Pin_Init,my pin init);
