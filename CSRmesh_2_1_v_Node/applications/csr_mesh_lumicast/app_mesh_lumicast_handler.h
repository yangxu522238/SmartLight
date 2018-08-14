/*******************************************************************************
 *  Copyright 2016 Qualcomm Technologies International, Ltd.
 *  Bluetooth Low Energy CSRmesh 2.1
 *  Application version 2.1.0
 *
 *  FILE
 *      app_mesh_lumicast_handler.h
 *
 *  DESCRIPTION
 *      This file defines the lumicast related structures and definitions.
 *
 ******************************************************************************/
#ifndef __APP_MESH_LUMICAST_HANDLER_H__
#define __APP_MESH_LUMICAST_HANDLER_H__

/*=============================================================================*
 *  Application Header Files
 *============================================================================*/

#include "app_gatt.h"

/*=============================================================================*
 *  Public Function Prototypes
 *============================================================================*/

extern void SetLumicastPayload(uint8 payload[], uint16 payload_size);

#endif /* __APP_MESH_LUMICAST_HANDLER_H__ */

