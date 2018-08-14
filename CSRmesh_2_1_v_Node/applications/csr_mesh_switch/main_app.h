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

/* Configuration bits on the User Key */
#define CSKEY_RELAY_ENABLE_BIT         (1)
#define CSKEY_BRIDGE_ENABLE_BIT        (2)
#define CSKEY_RANDOM_UUID_ENABLE_BIT   (4)
#define UUID_LENGTH_WORDS              (8)
#define AUTH_CODE_LENGTH_WORDS         (4)

/* Maximum number of timers */
#define MAX_APP_TIMERS                 (20)

/* Magic value to check the sanity of NVM region used by the application */
#define NVM_SANITY_MAGIC               (0xAB91)

/* NVM Offset for switch data */
#define NVM_OFFSET_SWITCH_STATE        (CSR_MESH_NVM_SIZE)
/* Size of switch state in Words */
#define NVM_SWITCH_STATE_SIZE          (1)

/* NVM Offset for Wdog data */
#define NVM_WDOG_DATA_OFFSET           (NVM_OFFSET_SWITCH_STATE + \
                                        NVM_SWITCH_STATE_SIZE)

#ifdef ENABLE_WATCHDOG_MODEL
/* Size of Watchdog Model Data in Words */
#define WDOG_MODEL_NVM_SIZE             (2)
#else
#define WDOG_MODEL_NVM_SIZE             (0)
#endif

#define NVM_OFFSET_TIME_INTERVAL       (NVM_WDOG_DATA_OFFSET + \
                                        WDOG_MODEL_NVM_SIZE)

#ifdef ENABLE_TIME_MODEL
#define NVM_TIME_INTERVAL_SIZE         (1)
#else
#define NVM_TIME_INTERVAL_SIZE         (0)
#endif

#define NVM_OFFSET_ACTION_MODEL_DATA   ((NVM_OFFSET_TIME_INTERVAL + \
                                         NVM_TIME_INTERVAL_SIZE))

#ifdef ENABLE_ACTION_MODEL
#define ACTION_SIZE                    (16)
#define NVM_ACTIONS_SIZE               (MAX_ACTIONS_SUPPORTED * ACTION_SIZE)

/* Get NVM Offset of a specific action from its index. */
#define GET_ACTION_NVM_OFFSET(idx)     (NVM_OFFSET_ACTION_MODEL_DATA + \
                                       ((idx) * (ACTION_SIZE)))
#else
#define NVM_ACTIONS_SIZE               (0)
#endif

#define NVM_OFFSET_EXTENSION_MODEL_DATA (NVM_OFFSET_ACTION_MODEL_DATA + \
                                         NVM_ACTIONS_SIZE)

#ifdef ENABLE_EXTENSION_MODEL
#define EXTENSION_SIZE                 (2)
#define NVM_EXTENSION_SIZE             (MAX_EXT_OPCODE_RANGE_SUPPORTED * \
                                        EXTENSION_SIZE)

/* Get NVM Offset of a specific action from its index. */
#define GET_EXTENSION_NVM_OFFSET(idx)  (NVM_OFFSET_EXTENSION_MODEL_DATA + \
                                       ((idx) * (EXTENSION_SIZE)))
#else
#define NVM_EXTENSION_SIZE             (0)
#endif

#define NVM_OFFSET_SWITCH_MODEL_GROUPS (NVM_OFFSET_EXTENSION_MODEL_DATA + \
                                        NVM_EXTENSION_SIZE)

#ifdef ENABLE_SWITCH_MODEL
#define SIZEOF_SWITCH_MODEL_GROUPS     (sizeof(uint16)*MAX_MODEL_GROUPS)
#else
#define SIZEOF_SWITCH_MODEL_GROUPS     (0)
#endif /* ENABLE_SWITCH_MODEL */

#define NVM_OFFSET_WDOG_MODEL_GROUPS  (NVM_OFFSET_SWITCH_MODEL_GROUPS + \
                                        SIZEOF_SWITCH_MODEL_GROUPS)

#ifdef ENABLE_WATCHDOG_MODEL
#define SIZEOF_WDOG_MODEL_GROUPS       (sizeof(uint16)*MAX_MODEL_GROUPS)
#else
#define SIZEOF_WDOG_MODEL_GROUPS       (0)
#endif /* ENABLE_WDOG_MODEL */

#define NVM_OFFSET_ATT_MODEL_GROUPS    (NVM_OFFSET_WDOG_MODEL_GROUPS + \
                                        SIZEOF_WDOG_MODEL_GROUPS)

#ifdef ENABLE_ATTENTION_MODEL
#define SIZEOF_ATT_MODEL_GROUPS       (sizeof(uint16)*MAX_MODEL_GROUPS)
#else
#define SIZEOF_ATT_MODEL_GROUPS       (0)
#endif /* ENABLE_ATTENTION_MODEL */

#define NVM_OFFSET_DATA_MODEL_GROUPS   (NVM_OFFSET_ATT_MODEL_GROUPS + \
                                        SIZEOF_ATT_MODEL_GROUPS)

#ifdef ENABLE_DATA_MODEL
#define SIZEOF_DATA_MODEL_GROUPS       (sizeof(uint16)*MAX_MODEL_GROUPS)
#else
#define SIZEOF_DATA_MODEL_GROUPS       (0)
#endif /* ENABLE_DATA_MODEL */

#define NVM_OFFSET_LOT_MODEL_GROUPS    (NVM_OFFSET_DATA_MODEL_GROUPS + \
                                        SIZEOF_DATA_MODEL_GROUPS)

#ifdef ENABLE_LOT_MODEL
#define SIZEOF_LOT_MODEL_GROUPS        (sizeof(uint16)*MAX_MODEL_GROUPS)
#else
#define SIZEOF_LOT_MODEL_GROUPS        (0)
#endif /* ENABLE_LOT_MODEL */

/* NVM Offset for Application data */
#define NVM_MAX_APP_MEMORY_WORDS       (NVM_OFFSET_LOT_MODEL_GROUPS + \
                                        SIZEOF_LOT_MODEL_GROUPS)

uint16                                  g_app_nvm_offset;
bool                                    g_app_nvm_fresh;
uint16                                  g_cskey_flags;

#ifndef CSR101x
/* Cached Value of UUID. */
extern uint16 cached_uuid[UUID_LENGTH_WORDS];

/* Cached Value of Authorization Code. */
#ifdef USE_AUTHORISATION_CODE
extern uint16 cached_auth_code[AUTH_CODE_LENGTH_WORDS];
#endif /* USE_AUTHORISATION_CODE */

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

