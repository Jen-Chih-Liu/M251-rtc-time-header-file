/****************************************************************************
 * @file     main.c
 * @version  V1.00
 * @brief    Demonstrate the RTC function and displays current time to the
 *           UART console
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2019 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/
#include <stdio.h>
#include "NuMicro.h"
#include <time.h>

#define CLK_HIRC    0
#define CLK_HXT     1
#define CLK_SOURCE  CLK_HIRC
#define PLL_CLOCK   FREQ_48MHZ

/*---------------------------------------------------------------------------------------------------------*/
/* Global variables                                                                                        */
/*---------------------------------------------------------------------------------------------------------*/

volatile uint32_t  g_u32TICK = 0;

/*---------------------------------------------------------------------------------------------------------*/
/*                            Define functions prototype                                                   */
/*---------------------------------------------------------------------------------------------------------*/
void RTC_TickHandle(void);
void RTC_IRQHandler(void);
void Delay(uint32_t ucnt);
void LXT_Enable(void);
void SYS_Init(void);
void UART0_Init(void);

int32_t main(void);

#if defined (__GNUC__) && !defined(__ARMCC_VERSION) && defined(OS_USE_SEMIHOSTING)
    extern void initialise_monitor_handles(void);
#endif
volatile uint32_t time_dat = 0;
time_t time(time_t *_timer)
{
    if (_timer != NULL)
    {
        time_dat = *_timer;
    }

    return time_dat;
}
/*---------------------------------------------------------------------------------------------------------*/
/*                                        RTC Tick Handle                                                  */
/*---------------------------------------------------------------------------------------------------------*/
unsigned char sec_flag = 0;
void RTC_TickHandle(void)
{
    S_RTC_TIME_DATA_T sCurTime;

    /* Get the current time */
    RTC_GetDateAndTime(&sCurTime);
    time_dat++;
    sec_flag = 1;
}

/**
  * @brief  RTC ISR to handle interrupt event
  * @param  None
  * @retval None
  */
void RTC_IRQHandler(void)
{

    if ((RTC->INTEN & RTC_INTEN_TICKIEN_Msk) && (RTC->INTSTS & RTC_INTSTS_TICKIF_Msk))          /* tick interrupt occurred */
    {
        RTC->INTSTS = 0x2;

        RTC_TickHandle();
    }

}

void Delay(uint32_t u32Cnt)
{
    volatile uint32_t u32DlyCnt = u32Cnt;

    while (u32DlyCnt--);
}

/*---------------------------------------------------------------------------------------------------------*/
/*                           Enable the external 32768Hz XTAL Clock                                        */
/*---------------------------------------------------------------------------------------------------------*/
void LXT_Enable(void)
{
    /* Set X32_OUT(PF.4) and X32_IN(PF.5) to input mode */
    PF->MODE &= ~(GPIO_MODE_MODE4_Msk | GPIO_MODE_MODE5_Msk);

    /* Enable external 32768Hz XTAL */
    CLK_EnableXtalRC(CLK_PWRCTL_LXTEN_Msk);

    /* Waiting for clock ready */
    CLK_WaitClockReady(CLK_STATUS_LXTSTB_Msk);

    /* Disable digital input path of analog pin X32_OUT to prevent leakage */
    GPIO_DISABLE_DIGITAL_PATH(PF, (1ul << 4));

    /* Disable digital input path of analog pin XT32_IN to prevent leakage */
    GPIO_DISABLE_DIGITAL_PATH(PF, (1ul << 5));
}

/*---------------------------------------------------------------------------------------------------------*/
/*                                      Init System Clock                                                  */
/*---------------------------------------------------------------------------------------------------------*/
void SYS_Init(void)
{

    /* Unlock protected registers */
    SYS_UnlockReg();


#if (CLK_SOURCE == CLK_HIRC )
    /* Enable HIRC clock (Internal RC 48MHz) */
    CLK_EnableXtalRC(CLK_PWRCTL_HIRCEN_Msk);

    /* Wait for HIRC clock ready */
    CLK_WaitClockReady(CLK_STATUS_HIRCSTB_Msk);

    /* Select HCLK clock source as HIRC and and HCLK source divider as 1 */
    CLK_SetHCLK(CLK_CLKSEL0_HCLKSEL_HIRC, CLK_CLKDIV0_HCLK(1));

    /* Set both PCLK0 and PCLK1 as HCLK/2 */
    CLK->PCLKDIV = CLK_PCLKDIV_APB0DIV_DIV2 | CLK_PCLKDIV_APB1DIV_DIV2;

    /* Select HIRC as the clock source of UART */
    CLK_SetModuleClock(UART0_MODULE, CLK_CLKSEL1_UART0SEL_HIRC, CLK_CLKDIV0_UART0(1));
#else

    /* Set XT1_OUT(PF.2) and XT1_IN(PF.3) to input mode */
    PF->MODE &= ~(GPIO_MODE_MODE2_Msk | GPIO_MODE_MODE3_Msk);

    /* Enable external 12MHz XTAL */
    CLK_EnableXtalRC(CLK_PWRCTL_HXTEN_Msk);

    /* Waiting for clock ready */
    CLK_WaitClockReady(CLK_STATUS_HXTSTB_Msk);

    /* Set core clock as PLL_CLOCK from PLL */
    CLK_SetCoreClock(PLL_CLOCK);

    /* Set both PCLK0 and PCLK1 as HCLK */
    CLK->PCLKDIV = CLK_PCLKDIV_APB0DIV_DIV1 | CLK_PCLKDIV_APB1DIV_DIV1;

    /* Select HXT as the clock source of UART */
    CLK_SetModuleClock(UART0_MODULE, CLK_CLKSEL1_UART0SEL_HXT, CLK_CLKDIV0_UART0(1));

    /* Disable digital input path of analog pin XT1_OUT to prevent leakage */
    GPIO_DISABLE_DIGITAL_PATH(PF, (1ul << 2));

    /* Disable digital input path of analog pin XT1_IN to prevent leakage */
    GPIO_DISABLE_DIGITAL_PATH(PF, (1ul << 3));
#endif

    /* Enable external 32768Hz XTAL */
    LXT_Enable();

    /* Enable UART peripheral clock */
    CLK_EnableModuleClock(UART0_MODULE);
    /* Enable RTC peripheral clock */
    CLK_EnableModuleClock(RTC_MODULE);

    /* Update System Core Clock */
    /* User can use SystemCoreClockUpdate() to calculate PllClock, SystemCoreClock and CycylesPerUs automatically. */
    SystemCoreClockUpdate();

    /*---------------------------------------------------------------------------------------------------------*/
    /*                                          Init I/O Multi-function                                        */
    /*---------------------------------------------------------------------------------------------------------*/
    /* Set PB multi-function pins for UART0 RXD and TXD */
    Uart0DefaultMPF();

    /* Lock protected registers */
    SYS_LockReg();

}
/*---------------------------------------------------------------------------------------------------------*/
/*                                            Init UART                                                    */
/*---------------------------------------------------------------------------------------------------------*/
void UART0_Init(void)
{
    UART_Open(UART0, 115200);
}



/*---------------------------------------------------------------------------------------------------------*/
/*                                          MAIN function                                                  */
/*---------------------------------------------------------------------------------------------------------*/

int32_t main(void)
{
    time_t tim;
    struct tm *ts;
    struct tm ctime;
    S_RTC_TIME_DATA_T sInitTime;

    SYS_Init();



    UART0_Init();
#if defined (__GNUC__) && !defined(__ARMCC_VERSION) && defined(OS_USE_SEMIHOSTING)
    initialise_monitor_handles();
#endif


    printf("\n\nCPU @ %d Hz\n", SystemCoreClock);

    /* Time Setting */
    sInitTime.u32Year       = 2023;
    sInitTime.u32Month      = 9;
    sInitTime.u32Day        = 6;
    sInitTime.u32Hour       = 10;
    sInitTime.u32Minute     = 0;
    sInitTime.u32Second     = 0;
    sInitTime.u32DayOfWeek  = RTC_TUESDAY;
    sInitTime.u32TimeScale  = RTC_CLOCK_24;

    RTC_Open(&sInitTime);
    ctime.tm_year = sInitTime.u32Year  - 1900;
    ctime.tm_mon  = sInitTime.u32Month - 1;
    ctime.tm_mday = sInitTime.u32Day ;
    ctime.tm_hour = sInitTime.u32Hour ;
    ctime.tm_min  = sInitTime.u32Minute;
    ctime.tm_sec  = sInitTime.u32Second;
    tim = mktime(&ctime);
    time(&tim);
    tim = time(NULL);
    ts = localtime(&tim);
    printf(" %04d-%02d-%02d %02d:%02d:%02d \r\n",

           ts->tm_year + 1900,
           ts->tm_mon  + 1,
           ts->tm_mday,
           ts->tm_hour,
           ts->tm_min,
           ts->tm_sec
          );

    RTC_SetTickPeriod(RTC_TICK_1_SEC);

    /* Enable RTC Tick Interrupt */
    RTC_EnableInt(RTC_INTEN_TICKIEN_Msk);
    NVIC_EnableIRQ(RTC_IRQn);

    while (1)
    {
        if (sec_flag == 1)
        {
					sec_flag=0;
            tim = time(NULL);
            ts = localtime(&tim);
            printf(" %04d-%02d-%02d %02d:%02d:%02d \r\n",
                   ts->tm_year + 1900,
                   ts->tm_mon  + 1,
                   ts->tm_mday,
                   ts->tm_hour,
                   ts->tm_min,
                   ts->tm_sec
                  );
        }
    }

}


/*** (C) COPYRIGHT 2019 Nuvoton Technology Corp. ***/



