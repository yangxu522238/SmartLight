/*! \file gaia_otau_client_api.h
 * \brief GAIA OTAu Client Public API
 *
 * %%fullcopyright(2016)
 * %%version
 * %%appversion
 *
 */

#ifndef GAIA_OTAU_CLIENT_API_H_
#define GAIA_OTAU_CLIENT_API_H_

/*=============================================================================*
 *  SDK Header Files
 *============================================================================*/

/*=============================================================================*
 *  Public Data Types
 *============================================================================*/
/* This structure defines the states used for GAIA Client OTAu */
typedef enum
{
    STATE_VM_UPGRADE_IDLE,      /*!< Device is in idle state */
    STATE_VM_UPGRADE_CONNECT,   /*!< Device sends upgrade connect request */
    STATE_VM_UPGRADE_SYNC_REQ,  /*!< Device is connected and sends sync request */
    STATE_VM_UPGRADE_START_REQ, /*!< Device received sync confirm and sends start request */
    STATE_VM_UPGRADE_DATA_REQ,  /*!< Device received start confirm and sends sdata request  */
    STATE_VM_UPGRADE_TRANSFER,  /*!< Device started the transfer */
    STATE_VM_UPGRADE_WAIT_VALIDATION,     /*!< Device is waiting for the other side to complete validation */
    STATE_VM_UPGRADE_WAIT_VALIDATION_RSP, /*!< Device completed transfer and sent validation done request */
    STATE_VM_UPGRADE_WAIT_POST_TRANSFER_DISCONNECT,         /*!< Device waiting for post transfer disconnect */
    STATE_VM_UPGRADE_WAIT_POST_TRANSFER_WAIT_DISCONNECT,    /*!< Device waiting for post transfer disconnect */
    STATE_VM_UPGRADE_WAIT_POST_TRANSFER_RECONNECTION_DELAY, /*!< Device waiting for some time to send a connect request after reboot */
    STATE_VM_UPGRADE_WAIT_POST_TRANSFER_RECONNECTION,       /*!< Device waiting for other device to reconnect */
    STATE_VM_UPGRADE_START_REQ_AFTER_REBOOT,                /*!< Device sends start request after reboot */
    STATE_VM_UPGRADE_IN_PROGRESS_REQ,                       /*!< Device received start confirm and sends in progress request */
    STATE_VM_UPGRADE_COMMIT_REQ,                            /*!< Device received commit request and sends commit confirm */
    STATE_VM_UPGRADE_DISCONNECT,                            /*!< Device sends upgrade disconnect request*/
    STATE_VM_UPGRADE_COMPLETED,                             /*!< Device completed upgrade */
    STATE_VM_UPGRADE_ABORTING_DISCONNECT,                   /*!< Device received abort confirm and sends upgrade disconnect request */
} GAIA_VMUPGRADE_STATE;

/*!
 * \brief GAIA OTAu Client event notifications that can be sent to the application.
 *
 * An event handler callback function must be registered before events will
 * be received. See \link GaiaOtauClientRegisterCallback \endlink
 */
typedef enum
{
    gaia_otau_client_config_status_event,      /*!< event to indicate completion of gaia client config procedure */
    gaia_otau_client_upgrade_start_event,       /*!< event to indicate start of the gaia otau upgrade procedure. */
    gaia_otau_client_conn_event,       /*!< event to indicate conn status of the gaia otau client. */
    gaia_otau_client_data_req_event,       /*!< event to indicate to transfer data over the gaia otau client. */
    gaia_otau_client_upgrade_end_event,       /*!< event to indicate end of the gaia otau upgrade procedure. */
    gaia_otau_client_reset_event       /*!< event to indicate the gaia otau client is reset. */
} gaia_otau_client_event;

/*!
 * \brief Event data sent with \link gaia_otau_client_config_status_event \endlink
 */
typedef struct
{
    uint8 status;   /* Configuration status */
    device_handle_id    device_id; /*!< Device id */
} GAIA_OTAU_CLIENT_CONFIG_STATUS_EVENT_T;


/*!
 * \brief Event data sent with \link gaia_otau_client_upgrade_start_event \endlink
 */
typedef struct
{
    device_handle_id    device_id;  /*!< Device id */
} GAIA_OTAU_CLIENT_UPGRADE_START_EVENT_T;

/*!
 * \brief Event data sent with \link gaia_otau_client_upgrade_end_event \endlink
 */
typedef struct
{
    device_handle_id    device_id;  /*!< Device id */
} GAIA_OTAU_CLIENT_UPGRADE_END_EVENT_T;

/*!
 * \brief Event data sent with \link gaia_otau_client_conn_event \endlink
 */
typedef struct
{
    device_handle_id                            device_id;  /*!< Device id */
    cm_conn_result                              result;     /*!< Connection result */
    uint8                                       reason;     /*!< HCI error code */
} GAIA_OTAU_CLIENT_CONN_EVENT_T;

/*!
 * \brief Event data sent with \link gaia_otau_client_data_req_event \endlink
 */
typedef struct
{
    uint32              no_bytes_requested; /*!< Bytes Requested */
    device_handle_id    device_id;     /*!< Device id */
} GAIA_OTAU_CLIENT_DATA_REQ_EVENT_T;

/*!
 * \brief Event data sent with \link gaia_otau_client_reset_event \endlink
 */
typedef struct
{
} GAIA_OTAU_CLIENT_RESET_EVENT_T;

/*!
 * \brief Union of all possible event data types.
 *
 * Event specific data may be accessed via the corresponding member according
 * to the event type (see \link gaia_otau_client_event \endlink ).
 */
typedef union
{
    GAIA_OTAU_CLIENT_CONFIG_STATUS_EVENT_T      config_event;     /*!< Data for gaia_otau_client_config_status_event */
    GAIA_OTAU_CLIENT_CONN_EVENT_T               conn_event;       /*!< Data for gaia_otau_client_conn_event */
    GAIA_OTAU_CLIENT_DATA_REQ_EVENT_T           data_req_event; /*!< Data for gaia_otau_client_data_req_event */
    GAIA_OTAU_CLIENT_RESET_EVENT_T              reset_event;    /*!< Data for gaia_otau_client_reset_event */
    GAIA_OTAU_CLIENT_UPGRADE_START_EVENT_T      upgrade_start_event; /*!< Data for gaia_otau_client_upgrade_start_event */
    GAIA_OTAU_CLIENT_UPGRADE_END_EVENT_T        upgrade_end_event; /*!< Data for gaia_otau_client_upgrade_end_event */
} GAIA_OTAU_CLIENT_EVENT_T;

/*!
 * \brief Prototype for a GAIA OTAu Client event handler callback function.
 *
 * Called by the OTAu library to notify the application of various events, see
 * \link gaia_otau_client_event \endlink for a list of all possible events.
 *
 * \param[in] event The type of event being notified.
 * \param[in,out] data Pointer to the event data associated with the event.
 *
 * \return sys_status - The application should return sys_status_success if the
                        event was handled, and sys_status_failure otherwise, in
                        which case any expected return data will be ignored.
 */
typedef sys_status (*gaia_otau_client_event_handler)(gaia_otau_client_event event, GAIA_OTAU_CLIENT_EVENT_T *data);

/*=============================================================================*
 *  Public Function Prototypes
 *============================================================================*/
/*----------------------------------------------------------------------------
 *  GaiaOtauClientInitEvent
 *----------------------------------------------------------------------------*/
/*! \brief This function is used to initialise upgrade client data structure.
 *
 * \param[in] none
 * \returns Nothing
 *
 */
extern void GaiaOtauClientInitEvent(void);

/*----------------------------------------------------------------------------
 *  GaiaOtauClientConnNotifyEvent
 *----------------------------------------------------------------------------*/
/*! \brief The function is called to notify Gaia OTAu Client of connection status
 *
 * \param[in] cm_event_data The connection information received from the CM.
 * \returns Nothing
 *
 */
extern void GaiaOtauClientConnNotifyEvent(CM_CONNECTION_NOTIFY_T *cm_event_data);

/*----------------------------------------------------------------------------
 *  GaiaOtauClientRegisterCallback
 *----------------------------------------------------------------------------*/
/*! \brief Implements the callback registration
 *
 * \param[in] gaia_otau_client_event_handler App callback handler
 * \returns Nothing
 *
 */
extern void GaiaOtauClientRegisterCallback(gaia_otau_client_event_handler callback);

/*----------------------------------------------------------------------------
 *  GaiaOtauClientStartUpgrade
 *----------------------------------------------------------------------------*/
/*! \brief Starts the GAIA OTAU Client Upgrade procedure with the device id passed
 *
 * \param[in] device_id Device handle of the target device
 * \returns Nothing
 *
 */
extern void GaiaOtauClientStartUpgrade(device_handle_id device_id);

/*----------------------------------------------------------------------------
 *  GaiaOtauClientSendData
 *----------------------------------------------------------------------------*/
/*! \brief Sends the data passed through the API onto the device through GAIA OTAU Client Upgrade procedure
 *
 * \param[in] device_id Device handle of the target device
 * \param[in] payload payload received through notifications
 * \param[in] payload_length payload length received 
 * \param[in] reached_eof flag indicating the end of file is reached.
 * \returns Nothing
 *
 */
extern void GaiaOtauClientSendData(device_handle_id device_id, uint8* payload,
                                   uint32 payload_length, bool reached_eof);

/*----------------------------------------------------------------------------
 *  GaiaOtauClientGetState
 *----------------------------------------------------------------------------*/
/*! \brief Returns the current state of the GAIA OTAU Client
 *
 * \param[in] None
 * \returns The GAIA OTAU Client state
 *
 */
extern uint16 GaiaOtauClientGetState(void);

/*----------------------------------------------------------------------------
 *  GaiaOtauClientResetTransferState
 *----------------------------------------------------------------------------*/
/*! \brief Resets the GAIA Client Transfer state
 *
 * \param[in] None
 * \returns None
 *
 */
extern void GaiaOtauClientResetTransferState(void);

/*----------------------------------------------------------------------------
 *  GaiaOtauClientUpdateConnStatus
 *----------------------------------------------------------------------------*/
/*! \brief The function handles the connection/disconnection with the GAIA device being upgraded.
 *
 * \param[in] device_id Device handle of the target device
 * \param[in] result status of the connection with the target device
 * \returns Nothing
 *
 */
extern void GaiaOtauClientUpdateConnStatus(device_handle_id device_id,
                                           cm_conn_result result);

/*----------------------------------------------------------------------------
 *  GaiaOtauClientConfigurationComplete
 *----------------------------------------------------------------------------*/
/*! \brief This function is called when the Gaia client service configuration is complete.
 *
 * \param[in] device_id Device handle of the configured device id
 * \param[in] config_state GAIA Client configuration status
 * \returns Nothing
 *
 */
extern void GaiaOtauClientConfigurationComplete(device_handle_id device_id ,
                                                uint16 config_state);

#endif /* GAIA_OTAU_CLIENT_API_H_ */

