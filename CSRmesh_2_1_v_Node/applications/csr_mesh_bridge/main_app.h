/******************************************************************************
 *  Copyright 2015 - 2016 Qualcomm Technologies International, Ltd.
 *  Bluetooth Low Energy CSRmesh 2.1
 *  Application version 2.1.0
 *
 *  FILE
 *      main_app.h
 *
 *  DESCRIPTION
 *      Header definitions for CSR Mesh application file
 *
 ******************************************************************************/

#ifndef __MAIN_APP_H__
#define __MAIN_APP_H__

 /*============================================================================*
 *  SDK Header Files
 *============================================================================*/

/*============================================================================*
 *  Local Header Files
 *============================================================================*/
#include "user_config.h"

/*============================================================================*
 *  CSR Mesh Header Files
 *============================================================================*/
#include "csr_mesh.h"
#include "csr_sched.h"
#include "cm_types.h"
#include "csr_mesh_nvm.h"
/*============================================================================*
 *  Public Definitions
 *============================================================================*/
/* Maximum number of timers */
#define MAX_APP_TIMERS                 (20)

/* Magic value to check the sanity of NVM region used by the application */
#define NVM_SANITY_MAGIC               (0xAB94)

#define NVM_MAX_APP_MEMORY_WORDS       (CSR_MESH_NVM_SIZE)

uint16                                  g_app_nvm_offset;
bool                                    g_app_nvm_fresh;

#ifndef CSR101x
#ifdef RESET_NVM
bool                                    g_gaia_nvm_fresh;
#endif /* RESET_NVM */
#endif /* !CSR101x */

/*============================================================================*
 *  Public Function Prototypes
 *============================================================================*/

/* Initialise application data */
extern void AppDataInit(void);

/* Initialise the Application supported services */
extern void InitAppSupportedServices(void);
#endif /* __MAIN_APP_H__ */

