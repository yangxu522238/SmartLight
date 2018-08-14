/******************************************************************************
 *  Copyright 2015 - 2016 Qualcomm Technologies International, Ltd.
 *  Bluetooth Low Energy CSRmesh 2.1
 *  CSRmesh is a product of Qualcomm Technologies International Ltd.
 *****************************************************************************/
/*! \file csr_mesh_nvm.h
 *  \brief defines values and offsets for CSRmesh Stack NVM values
 *
 *   This file contains the function and definations for csr_mesh_ps_ifce.c
 */
 /*****************************************************************************/
#include "nvm_access.h"
#include "csr_mesh_ps_ifce.h"
#include "csr_mesh.h"

/*============================================================================*
 *  Public Definitions 
 *============================================================================*/

#define GAIA_OTA_SERVICE_SIZE          (150)
#define MESH_APP_SERVICES_SIZE         (50)
#define CM_SIZE                        (60)

/* NVM offset for the GAIA OTA service */
#define NVM_OFFSET_GAIA_OTA_SERVICE    (0)

/* NVM offset for the Application services */
#define NVM_OFFSET_MESH_APP_SERVICES   (NVM_OFFSET_GAIA_OTA_SERVICE + GAIA_OTA_SERVICE_SIZE)

/* NVM offset for the CM Initialisation */
#define NVM_OFFSET_CM_INITIALISATION   (NVM_OFFSET_MESH_APP_SERVICES + MESH_APP_SERVICES_SIZE)

#define NVM_OFFSET_MESH_STACK          (NVM_OFFSET_CM_INITIALISATION + CM_SIZE)

/* NVM offset for the application NVM version
 */
#ifdef CSR101x_A05
#define NVM_OFFSET_SANITY_WORD         (0)
#else
#define NVM_OFFSET_SANITY_WORD         (NVM_OFFSET_MESH_STACK)
#endif

/* NVM offset for NVM sanity word */
#define NVM_OFFSET_APP_NVM_VERSION     (NVM_OFFSET_SANITY_WORD + 1)

/* Number of words of NVM used by application. Memory used by supported
 * services is not taken into consideration here. */
#define NVM_OFFSET_ASSOCIATION_STATE   (NVM_OFFSET_APP_NVM_VERSION + 1)

/* Number of words for TTL */
#define NVM_OFFSET_TTL_VALUE           (NVM_OFFSET_ASSOCIATION_STATE + 1)

/* Number of words for Bearer State */
#define NVM_OFFSET_BEARER_STATE        (NVM_OFFSET_TTL_VALUE + 1)

/* Set the NVM offsets for the CSRmesh stack PS values */
#define CSR_MESH_NVM_SANITY_WORD_OFFSET      (NVM_OFFSET_BEARER_STATE + \
                                              sizeof(CSR_MESH_BEARER_STATE_DATA_T))

#define CSR_MESH_NVM_DEVICE_UUID_OFFSET      (CSR_MESH_NVM_SANITY_WORD_OFFSET + \
                                              CSR_MESH_NVM_SANITY_PSKEY_SIZE)

#define CSR_MESH_NVM_DEVICE_AUTH_CODE_OFFSET  (CSR_MESH_NVM_DEVICE_UUID_OFFSET + \
                                              CSR_MESH_DEVICE_UUID_PSKEY_SIZE)
                                            
#define CSR_MESH_MTL_NW_KEY_AVL_NVM_OFFSET   (CSR_MESH_NVM_DEVICE_AUTH_CODE_OFFSET + \
                                              CSR_MESH_DEVICE_AUTHCODE_PSKEY_SIZE)

#define CSR_MESH_MTL_MCP_NW_KEY_NVM_OFFSET   (CSR_MESH_MTL_NW_KEY_AVL_NVM_OFFSET + \
                                              CSR_MESH_MTL_NW_KEY_AVL_PSKEY_SIZE)

#define CSR_MESH_MASP_RELAY_STATE_NVM_OFFSET (CSR_MESH_MTL_MCP_NW_KEY_NVM_OFFSET + \
                                              CSR_MESH_MTL_MCP_NWKEY_PSKEY_SIZE)

#define CSR_MESH_MCP_SEQ_NUM_NVM_OFFSET      (CSR_MESH_MASP_RELAY_STATE_NVM_OFFSET + \
                                              CSR_MESH_MASP_RELAY_STATE_PSKEY_SIZE)

#define CSR_MESH_MCP_GEN_SEQ_NUM_NVM_OFFSET  (CSR_MESH_MCP_SEQ_NUM_NVM_OFFSET + \
                                              CSR_MESH_MCP_SEQ_NUM_KEY_PSKEY_SIZE)

#define CSR_MESH_CONFIG_LAST_ETAG_NVM_OFFSET (CSR_MESH_MCP_GEN_SEQ_NUM_NVM_OFFSET + \
                                              CSR_MESH_MCP_GEN_SEQ_NUM_KEY_PSKEY_SIZE)

#define CSR_MESH_DEVICE_ID_OFFSET            (CSR_MESH_CONFIG_LAST_ETAG_NVM_OFFSET + \
                                              CSR_MESH_CONFIG_LAST_ETAG_PSKEY_SIZE)

#define CSR_MESH_MTL_RELAY_STATUS_NVM_OFFSET (CSR_MESH_DEVICE_ID_OFFSET + \
                                              CSR_MESH_DEVICE_ID_KEY_PSKEY_SIZE)

#define CSR_MESH_DHM_KEY_NVM_OFFSET          (CSR_MESH_MTL_RELAY_STATUS_NVM_OFFSET + \
                                              CSR_MESH_MTL_RELAY_STATUS_PSKEY_SIZE)

#define CSR_MESH_IV_NVM_OFFSET               (CSR_MESH_DHM_KEY_NVM_OFFSET + \
                                              CSR_MESH_DEVICE_DHM_PSKEY_SIZE)

#ifdef CSR101x_A05
#define CSR_MESH_NVM_SIZE                    (CSR_MESH_IV_NVM_OFFSET + \
                                              CSR_MESH_NW_IV_PSKEY_SIZE)
#else
#define CSR_MESH_MCP_SEQ_DEV_COUNT_OFFSET    (CSR_MESH_IV_NVM_OFFSET + \
                                              CSR_MESH_NW_IV_PSKEY_SIZE)

#define CSR_MESH_MCP_SEQ_CACHE_TABLE_OFFSET  (CSR_MESH_MCP_SEQ_DEV_COUNT_OFFSET + \
                                              CSR_MESH_MCP_SEQ_DEV_COUNT_SIZE)

#define CSR_MESH_NVM_SIZE                    (CSR_MESH_MCP_SEQ_CACHE_TABLE_OFFSET + \
                                              (CSR_MESH_SEQ_TABLE_SIZE * SRC_SEQ_CACHE_SLOT_SIZE))
#endif

