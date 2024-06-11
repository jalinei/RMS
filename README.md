# Real-Time Monitoring System (RMS) on Zephyr with STM32G4

## RMS example using Zero Latency Interrupts. 

This example implements 2 real time interrupts with a preemption mechanism. 
It stays compatible with Zephyr RTOS. 

- RMS_0 is based on a hardware interrupt from TIM6. Any phyisical clock source could be used. 
- RMS_1 is trigged as a software interruption by RMS_0 using an unused IRQ channel from NVIC.

```
    IRQ_DIRECT_CONNECT(TIM6_DAC_IRQn, 0, RMS_0, IRQ_ZERO_LATENCY);
    IRQ_DIRECT_CONNECT(55, 1, RMS_1, IRQ_ZERO_LATENCY);
```

Wide view of the scheduling plot with a CPU load of approx 70% on RMS_0
![image](https://github.com/jalinei/RMS/assets/22010135/e4322cfe-b678-4c91-a056-e788ef283eb7)
Detail view of the scheduling plot with a CPU load of approx 70% on RMS_0
![image](https://github.com/jalinei/RMS/assets/22010135/99765ab1-70c9-4028-8dd3-8a68888e0a0c)
Detail view of the scheduling plot with a CPU load of approx 30% on RMS_0
![image](https://github.com/jalinei/RMS/assets/22010135/af580f50-be78-4847-86cf-bf7082fea0b0)
 
## Files

- **main.c**: Contains the main code implementation.

## Dependencies

- Zephyr RTOS
- STM32G4 HAL/LL libraries

## Macros and Constants

- `PER_1`: Periodicity for `RMS_1` in terms of TIM6 interrupts.
- `DELAY`: Unused in this code.
- `COUNT_LIMIT0`: Maximum count limit for `RMS_0` task executions.
- `COUNT_LIMIT1`: Maximum count limit for `RMS_1` task executions.

## Global Variables

- `flag_rms1`: Flag to indicate if `RMS_1` has been preempted by `RMS_0`.
- `start_time_0`, `end_time_0`, `start_time_1`, `end_time_1`: Arrays to store start and end times for `RMS_0` and `RMS_1`.
- `count_0`, `count`, `count_1`: Counters for the number of executions of `RMS_0`, overall execution count, and executions of `RMS_1`.
- `sched_1`: Scheduler counter for `RMS_1` to manage its periodicity.

## Functions

### `void RMS_0()`

The primary task that runs periodically with the timer interrupt. It performs the following:

1. Increments the scheduler counter for `RMS_1` and sets a pending interrupt for `RMS_1` if the counter reaches `PER_1`.
2. Clears the update flag for TIM6.
3. Records the start time.
4. Busy-waits for a short period.
5. Checks and handles preemption of `RMS_1`.
6. Records the end time.
7. Increments the count of `RMS_0` executions.

### `void RMS_1()`

A secondary task that can be preempted by `RMS_0`. It performs the following:

1. Records the start time and sets a flag indicating its execution.
2. Busy-waits for a longer period.
3. Handles preemption by resetting the flag and recording resume times.
4. Records the end time.
5. Clears the pending interrupt and increments the count of `RMS_1` executions.

### `int setup()`

Configures the hardware and peripherals:

1. Enables the clock for TIM6.
2. Sets the prescaler for a 1 MHz timer clock.
3. Configures TIM6 to count for 200 µs.
4. Enables update interrupts for TIM6.
5. Connects interrupts for TIM6 and custom interrupt (IRQ 55) to `RMS_0` and `RMS_1`.
6. Enables the TIM6 counter and initializes the timing system.

### `void main(void)`

The main function that:

1. Initializes the system and calls the setup function.
2. Enters an infinite loop to print the recorded execution times of `RMS_0` and `RMS_1` tasks after reaching the count limit.

## Usage

1. **Compilation and Flashing**: Compile the code with the appropriate toolchain and flash it to the STM32G4 microcontroller.
2. **Execution**: The system starts with an initial delay, sets up the timer and interrupts, and enters a loop to print execution times.
3. **Output**: Execution times of `RMS_0` and `RMS_1` are printed to the console in CSV format.
4. **Copy and Paste**: Copy the output and place it in a file called timing_test at project root.
5. **Plot scheduling chronograph**: Launch ploting_sched.py.

```bash
time,level,time1,level1
```


## Interrupt levels and hardly obtained wisdom.
### Number of priorities and avoiding confusions because of poorly written datasheet...  
ARM Cortex M4 has 16 priority level ranging from 0 to 15. 
This are defined in SCB register in NVIC_IPRx. 
On the STM32G474, priority is encoded on bits [7:4] in IPR registers. 
The reference manual is super missleading as NVIC vector table is presented with priorities from 0 to 
108. 
![image](https://github.com/jalinei/RMS/assets/22010135/7470968d-6614-4bc8-bb17-0d234ad41396)
In reality only the 4 MSB of this value have to be considered. 

### Priority grouping.
There are also Priority groups and Subpriority groups. https://www.st.com/resource/en/product_training/STM32G4-System-Nested_Vectored_Interrupt_Control_NVIC.pdf
Priority groups are for premption interrupts. As per schematic below
![image](https://github.com/jalinei/RMS/assets/22010135/f2063585-9ecf-4ec5-ad6e-d4d126c83a6c)
Subpriority are for cooperative interrupts order. As per schematic below
![image](https://github.com/jalinei/RMS/assets/22010135/01a19266-98c5-4a00-a302-e54363b037f4)
![image](https://github.com/jalinei/RMS/assets/22010135/9f05172d-8291-4efd-8122-4aeb6e0a5aa0)

In our RMS case we want preemptive interrupts. By default in stm32 there are 2 bits defining the priority, and two bits defining the subpriority. 
It does not seem to be the case when using Zephyr. All the 4 bits are used to define Priority and not subpriorities. 

#### Zephyr context
In a Zephyr application, by default, all priorities are rerouted to Zephyr ISR handlers with a priority level of 
SVcall + 1. 
In our case we can not accept jitter from zephyr on our Hard Real Time application. 
As such we use Zero Latency Interrupts which can map interrupt beneath the priority level of the RTOS. 

### Findings 

- `IRQ_CONNECT` cannot set a Zero Latency Interrupt(ZLI). Only `IRQ_DIRECT_CONNECT` can declare a ZLI 
- `CONFIG_ZERO_LATENCY_LEVELS=8` set the number of priority level "under" Zephyr RTOS.

This example has the following IRQ priorities : 
```
Reset -3
NMI -2
HardFault -1
TIM6_IRQ (RMS_0) 0
55_IRQ (RMS_1) 1
unused ZLI 2
unused ZLI 3
...
unused ZLI 7
SVCall (Zephyr Scheduler) 8
Systick 9
Zephyr_ISRs (all ISR handler controlled by zephyr) 9
PendSV 15 (Zephyr threads)
```


