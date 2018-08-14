/*****************************************************************************
 *  Copyright 2015 - 2016 Qualcomm Technologies International, Ltd.
 *  Bluetooth Low Energy CSRmesh 2.1
 *  Application version 2.1.0
 *
 *  FILE
 *      app_hw.h
 *
 *  DESCRIPTION
 *      This file defines a interface to abstract hardware specifics of switch.
 *
 *****************************************************************************/

#ifndef __APP_HW_H__
#define __APP_HW_H__

#include "main_app.h"
/*============================================================================*
 *  Public Function Prototypes
 *============================================================================*/

/* This function initializes the switch hardware, like PIO, interface etc. */
extern void SwitchHardwareInit(void);

/* PIO changed events for handling button presses */
extern void HandlePIOChangedEvent(pio_changed_data *data);

/* This function reads the switch data from NVM. */
extern void ReadSwitchDataFromNVM(void);

/* This function writes switch data onto NVM. */
extern void WriteSwitchDataOntoNVM(void);

#endif /* __APP_HW_H__ */

