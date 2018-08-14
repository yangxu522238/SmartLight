/******************************************************************************
 *  Copyright 2016 Qualcomm Technologies International, Ltd.
 *  Bluetooth Low Energy CSRmesh 2.1
 *  CSRmesh is a product of Qualcomm Technologies International Ltd.
 */
/*! \file gaia_otau_client_api.h
 * \brief GAIA OTAu Public API
 *
 */
#ifndef APP_OTAU_CLIENT_HANDLER_H_
#define APP_OTAU_CLIENT_HANDLER_H_

#ifdef GAIA_OTAU_RELAY_SUPPORT

/*=============================================================================*
 *  SDK Header Files
 *============================================================================*/
#include <store_update_msg.h>
#include "gaia_otau_api.h"

/*=============================================================================*
 *  Public Data Types
 *============================================================================*/
/* Bin File Header Offsets */
#define PARTITION_HEADER_BODY_OFFSET                           (12)
#define PARTITION_HEADER_COMPANY_CODE_OFFSET                   (16)
#define PARTITION_HEADER_PLATFORMTYPE_OFFSET                   (18)
#define PARTITION_HEADER_TYPEENCODING_OFFSET                   (19)
#define PARTITION_HEADER_IMAGETYPE_OFFSET                      (20)
#define PARTITION_HEADER_APP_VERSION_OFFSET                    (21) 
#define PARTITION_ID_OFFSET                                    (26)
#define PARTITION_LENGTH_OFFSET                                (34)

/* Bin File Footer Offsets */
#define PARTITION_FOOTER_LENGTH_OFFSET                         (8)
#define PARTITION_FOOTER_SIGNATURE_OFFSET                      (12)

/* Bin File Header Values */
#define HEADER_SIZE                                            (42)
#define MESH_HEADER_BODY_SIZE                                  (14)
#define PARTITION_INFO_SIZE                                     (8)

/* Bin File Footer Values */
#define FOOTER_SIZE                                            (44)
#define PARTITION_FOOTER_ID_SIZE                               (8)
#define PARTITION_FOOTER_LENGTH_SIZE                           (4)
#define PARTITION_FOOTER_SIGNATURE_LENGTH                      (32)

/*============================================================================*
 *  Public Data
 *============================================================================*/

bool scanning_ongoing;

/*=============================================================================*
 *  Public Function Prototypes
 *============================================================================*/

extern void AppOtauClientInit(void);
extern void GaiaAppClientInit(bool nvm_start_fresh, uint16 *nvm_offset);
extern void StorePartitionSignatureToNVM(uint8* p_sig_data);
extern void StorePartitionDataToNVM(GAIA_OTAU_EVENT_PARTITION_INFO_T* p_part_data);
extern void StoreHeaderDataToNVM(uint8* p_hdr_data);
extern void StoreRelayStoreInfo(GAIA_OTAU_EVENT_PARTITION_INFO_T *p_store_info);
extern void SendLOTAnnouncePacket(void);
extern void GaiaOtauClientConfigStoreMsg(msg_t *msg);
extern void GaiaOtauClientHandleStoreUpdateMsg(device_handle_id device_id, store_update_msg_t *msg);
extern void GaiaOtauSetCommitStatus(bool status);
extern void GaiaOtauSetRelayStore(bool commit_successful);

#endif
#endif /* APP_OTAU_CLIENT_HANDLER_H_ */
