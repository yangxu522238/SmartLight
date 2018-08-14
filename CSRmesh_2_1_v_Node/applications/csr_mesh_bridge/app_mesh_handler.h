/******************************************************************************
 *  Copyright 2015 - 2016 Qualcomm Technologies International, Ltd.
 *  Bluetooth Low Energy CSRmesh 2.1
 *  Application version 2.1.0
 *
 *  FILE
 *      app_mesh_handler.h
 *
 *
 ******************************************************************************/
#ifndef __APP_MESH_HANDLER_H__
#define __APP_MESH_HANDLER_H__

#include "core_mesh_handler.h"
#include "user_config.h"

/*============================================================================*
 *  Public Data
 *============================================================================*/
extern uint8 mesh_ad_data[];

/*============================================================================*
 *  Public Function Prototypes
 *============================================================================*/
/* The function Initializes core mesh stack and the supported models. */
extern void AppMeshInit(void);

#ifdef NVM_TYPE_FLASH
/* This function writes the application data to NVM. This function should 
 * be called on getting nvm_status_needs_erase
 */
extern void WriteApplicationAndServiceDataToNVM(void);
#endif /* NVM_TYPE_FLASH */

#endif /* __APP_MESH_HANDLER_H__ */
