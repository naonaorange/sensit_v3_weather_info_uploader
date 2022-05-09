/*!******************************************************************
 * \file main_TEMPERATURE.c
 * \brief Sens'it Discovery mode Temperature demonstration code
 * \author Sens'it Team
 * \copyright Copyright (c) 2018 Sigfox, All Rights Reserved.
 *
 * For more information on this firmware, see temperature.md.
 *******************************************************************/
/******* INCLUDES **************************************************/
#include "sensit_types.h"
#include "sensit_api.h"
#include "error.h"
#include "button.h"
#include "battery.h"
#include "radio_api.h"
#include "hts221.h"
#include "discovery.h"


/******** DEFINES **************************************************/
#define MEASUREMENT_PERIOD                 (60 * 30) /* Measurement & Message sending period, in second */
#define PAYLOAD_SIZE                        5

/******* GLOBAL VARIABLES ******************************************/
u8 firmware_version[] = "TEMP_v2.0.0";


/*******************************************************************/

int main()
{
    error_t err;
    button_e btn;
    bool send = FALSE;
    u8 payload[PAYLOAD_SIZE] = {0};
    u8 count = 0;
    s8 temperature = 0;
    u8 humidity = 0;
    u8 battery = 0;

    /* Start of initialization */

    /* Configure button */
    SENSIT_API_configure_button(INTERRUPT_BOTH_EGDE);

    /* Initialize Sens'it radio */
    err = RADIO_API_init();
    ERROR_parser(err);

    /* Initialize temperature & humidity sensor */
    err = HTS221_init();
    ERROR_parser(err);

    /* Initialize RTC alarm timer */
    SENSIT_API_set_rtc_alarm(MEASUREMENT_PERIOD);

    /* Clear pending interrupt */
    pending_interrupt = 0;

    /* End of initialization */

    while (TRUE)
    {
        /* Execution loop */

        /* RTC alarm interrupt handler */
        if ((pending_interrupt & INTERRUPT_MASK_RTC) == INTERRUPT_MASK_RTC)
        {
            s16 t = 0;
            u16 h = 0;
            u16 b = 0;

            /* Check of battery level */
            BATTERY_handler(&b);

            /* Do a temperatue & relative humidity measurement */
            err = HTS221_measure(&t, &h);
            if (err != HTS221_ERR_NONE)
            {
                ERROR_parser(err);
                temperature = 0;
                humidity = 0;
                battery = 0;
            }
            else
            {
                temperature = (s8)((float)t / 8.0F + 0.5F);
                humidity = (u8)(h / 2);
                battery = (u8)(b / 100);
                /* Set send flag */
                send = TRUE;
            }

            /* Clear interrupt */
            pending_interrupt &= ~INTERRUPT_MASK_RTC;
        }

        /* Button interrupt handler */
        if ((pending_interrupt & INTERRUPT_MASK_BUTTON) == INTERRUPT_MASK_BUTTON)
        {
            /* RGB Led ON during count of button presses */
            SENSIT_API_set_rgb_led(RGB_GREEN);

            /* Count number of presses */
            btn = BUTTON_handler();

            /* RGB Led OFF */
            SENSIT_API_set_rgb_led(RGB_OFF);

            if (btn == BUTTON_TWO_PRESSES)
            {
                /* Force a RTC alarm interrupt to do a new measurement */
                pending_interrupt |= INTERRUPT_MASK_RTC;
            }
            else if (btn == BUTTON_FOUR_PRESSES)
            {
                /* Reset the device */
                SENSIT_API_reset();
            }

            /* Clear interrupt */
            pending_interrupt &= ~INTERRUPT_MASK_BUTTON;
        }

        /* Reed switch interrupt handler */
        if ((pending_interrupt & INTERRUPT_MASK_REED_SWITCH) == INTERRUPT_MASK_REED_SWITCH)
        {
            /* Clear interrupt */
            pending_interrupt &= ~INTERRUPT_MASK_REED_SWITCH;
        }

        /* Accelerometer interrupt handler */
        if ((pending_interrupt & INTERRUPT_MASK_FXOS8700) == INTERRUPT_MASK_FXOS8700)
        {
            /* Clear interrupt */
            pending_interrupt &= ~INTERRUPT_MASK_FXOS8700;
        }

        /* Check if we need to send a message */
        if (send == TRUE)
        {
            /* Build the payload */
            payload[0] = 0xAA;
            payload[1] = count;
            payload[2] = temperature;
            payload[3] = humidity;
            payload[4] = battery;

            /* Send the message */
            err = RADIO_API_send_message(RGB_GREEN, payload, PAYLOAD_SIZE, FALSE, NULL);
            if(err == RADIO_ERR_NONE)
            {
                count++;
            }
            /* Parse the error code */
            ERROR_parser(err);

            /* Clear send flag */
            send = FALSE;
        }

        /* Check if all interrupt have been clear */
        if (pending_interrupt == 0)
        {
            /* Wait for Interrupt */
            SENSIT_API_sleep(FALSE);
        }
    } /* End of while */
}

/*******************************************************************/
