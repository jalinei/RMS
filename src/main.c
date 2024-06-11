#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include "zephyr/timing/timing.h"
#include "stm32g4xx_ll_tim.h"
#include "stm32g4xx_ll_bus.h"


#define PER_1 10
// #define PER_2 20
#define DELAY 100
#define COUNT_LIMIT0 199

#define COUNT_LIMIT1 100

void RMS_0();
void RMS_1();
// void RMS_2();
int flag_rms1 = 0;


timing_t start_time_0[200];
timing_t end_time_0[200];
timing_t start_time_1[200];
timing_t end_time_1[200];
timing_t temp_st;
timing_t temp_ed;
static int count_0 = 0;
static int count = 0;
static int count_1 = 0;
static int sched_1 = 0;

void RMS_0()
{
    if(sched_1 < PER_1)
    {
        sched_1++;
    }
    else {
        NVIC_SetPendingIRQ(55);
        sched_1 = 0;
    }

    if (LL_TIM_IsActiveFlag_UPDATE(TIM6))
    {
        LL_TIM_ClearFlag_UPDATE(TIM6);
    }
    start_time_0[count_0] = timing_counter_get();
    
    uint32_t cycles = (SystemCoreClock / 1000000) * 3; 
    while (cycles > 0)
        {
            //flag_rms1 is rised because rms_1 has been preempted
            if (flag_rms1 == 1)
            {
                // get timestamp at which we detected that rms_1 has ended
                end_time_1[count_1] = timing_counter_get();
                // reset rms_1 flag
                flag_rms1 = 0;
                // increment record index to capture next run of rms_1
                if (count_1 < COUNT_LIMIT1) 
                {
                    count_1++;
                }
            }

            cycles--;
            __asm volatile ("nop"); // No operation, consumes 1 cycle
            
        }

    end_time_0[count_0] = timing_counter_get();

    if (count_0 < COUNT_LIMIT0) {
        count_0++;
    }
}

void RMS_1()
{
    // RMS_1 start
    start_time_1[count_1] = timing_counter_get();
    // set flag
    flag_rms1 = 1;
    // Modified busy wait to detect preemption by RMS_0
    uint32_t cycles = (SystemCoreClock / 1000000) * 33;
    while (cycles > 0)
    {
        //In case rms_1 has been preempted, flag will be at 0
        if (flag_rms1 == 0)
        {
            //record the resume timestamp
            start_time_1[count_1] = timing_counter_get();
            //set flag to 1
            flag_rms1 = 1;
        }
        cycles--;
        __asm volatile ("nop"); // No operation, consumes 1 cycle
    
    }

    //busy wait is finished and exception will yield without preemption.
    flag_rms1 = 0;
    //retrieve yielding timestamp
    end_time_1[count_1] = timing_counter_get();
    //increment count
    if (count_1 < COUNT_LIMIT1) 
    {
        count_1++;
    }
    NVIC_ClearPendingIRQ(55);
}

int setup()
{
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_TIM6);
    // Configure TIM6 prescaler to have a 1 MHz timer clock
    LL_TIM_SetPrescaler(TIM6, __LL_TIM_CALC_PSC(SystemCoreClock, 1000000));

    // Configure TIM6 to count for 200us
    LL_TIM_SetAutoReload(TIM6, 200 - 1);  // 200 counts for 200us

    // Enable update interrupt
    LL_TIM_EnableIT_UPDATE(TIM6);
    NVIC_EnableIRQ(TIM6_DAC_IRQn);
    NVIC_EnableIRQ(55);
    IRQ_DIRECT_CONNECT(TIM6_DAC_IRQn, 0, RMS_0, IRQ_ZERO_LATENCY);
    IRQ_DIRECT_CONNECT(55, 1, RMS_1, IRQ_ZERO_LATENCY);

    LL_TIM_EnableCounter(TIM6);
    timing_init();
    timing_start();

    return 0;
}

void main(void)
{
    k_msleep(100);
    setup();
    printk("time,level,time1,level1\n");
    while(1)
    {
        if (count_0 == COUNT_LIMIT0){
            if(count < COUNT_LIMIT0)
            {
                printk("%llu, 1, %llu, 1\n", start_time_0[count], start_time_1[count]);
                printk("%llu, 0, %llu, 0\n", end_time_0[count], end_time_1[count]);

                count++;
            }
        }
        k_msleep(100);
    }
}
