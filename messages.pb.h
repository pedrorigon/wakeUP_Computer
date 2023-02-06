/* Automatically generated nanopb header */
/* Generated by nanopb-0.4.8-dev */

#ifndef PB_MESSAGES_PB_H_INCLUDED
#define PB_MESSAGES_PB_H_INCLUDED
#include <pb.h>

#if PB_PROTO_HEADER_VERSION != 40
#error Regenerate this file with the current version of nanopb generator.
#endif

/* Enum definitions */
typedef enum _MessageType {
    MessageType_DISCOVERY = 0,
    MessageType_CONFIRMATION = 1
} MessageType;

typedef enum _ConfirmationStatus {
    ConfirmationStatus_OK = 0,
    ConfirmationStatus_NOT_OK = 1
} ConfirmationStatus;

/* Struct definitions */
typedef struct _Discovery {
    char mac_address[19];
    char hostname[256];
} Discovery;

typedef struct _Confirmation {
    ConfirmationStatus status;
} Confirmation;

typedef struct _Packet {
    MessageType type;
    uint32_t seqn;
    uint32_t timestamp;
    pb_size_t which_payload;
    union {
        Discovery discovery;
        Confirmation confirmation;
    } payload;
} Packet;


#ifdef __cplusplus
extern "C" {
#endif

/* Helper constants for enums */
#define _MessageType_MIN MessageType_DISCOVERY
#define _MessageType_MAX MessageType_CONFIRMATION
#define _MessageType_ARRAYSIZE ((MessageType)(MessageType_CONFIRMATION+1))

#define _ConfirmationStatus_MIN ConfirmationStatus_OK
#define _ConfirmationStatus_MAX ConfirmationStatus_NOT_OK
#define _ConfirmationStatus_ARRAYSIZE ((ConfirmationStatus)(ConfirmationStatus_NOT_OK+1))

#define Packet_type_ENUMTYPE MessageType


#define Confirmation_status_ENUMTYPE ConfirmationStatus


/* Initializer values for message structs */
#define Packet_init_default                      {_MessageType_MIN, 0, 0, 0, {Discovery_init_default}}
#define Discovery_init_default                   {"", ""}
#define Confirmation_init_default                {_ConfirmationStatus_MIN}
#define Packet_init_zero                         {_MessageType_MIN, 0, 0, 0, {Discovery_init_zero}}
#define Discovery_init_zero                      {"", ""}
#define Confirmation_init_zero                   {_ConfirmationStatus_MIN}

/* Field tags (for use in manual encoding/decoding) */
#define Discovery_mac_address_tag                1
#define Discovery_hostname_tag                   2
#define Confirmation_status_tag                  1
#define Packet_type_tag                          1
#define Packet_seqn_tag                          2
#define Packet_timestamp_tag                     3
#define Packet_discovery_tag                     4
#define Packet_confirmation_tag                  5

/* Struct field encoding specification for nanopb */
#define Packet_FIELDLIST(X, a) \
X(a, STATIC,   SINGULAR, UENUM,    type,              1) \
X(a, STATIC,   SINGULAR, UINT32,   seqn,              2) \
X(a, STATIC,   SINGULAR, UINT32,   timestamp,         3) \
X(a, STATIC,   ONEOF,    MESSAGE,  (payload,discovery,payload.discovery),   4) \
X(a, STATIC,   ONEOF,    MESSAGE,  (payload,confirmation,payload.confirmation),   5)
#define Packet_CALLBACK NULL
#define Packet_DEFAULT NULL
#define Packet_payload_discovery_MSGTYPE Discovery
#define Packet_payload_confirmation_MSGTYPE Confirmation

#define Discovery_FIELDLIST(X, a) \
X(a, STATIC,   SINGULAR, STRING,   mac_address,       1) \
X(a, STATIC,   SINGULAR, STRING,   hostname,          2)
#define Discovery_CALLBACK NULL
#define Discovery_DEFAULT NULL

#define Confirmation_FIELDLIST(X, a) \
X(a, STATIC,   SINGULAR, UENUM,    status,            1)
#define Confirmation_CALLBACK NULL
#define Confirmation_DEFAULT NULL

extern const pb_msgdesc_t Packet_msg;
extern const pb_msgdesc_t Discovery_msg;
extern const pb_msgdesc_t Confirmation_msg;

/* Defines for backwards compatibility with code written before nanopb-0.4.0 */
#define Packet_fields &Packet_msg
#define Discovery_fields &Discovery_msg
#define Confirmation_fields &Confirmation_msg

/* Maximum encoded size of messages (where known) */
#define Confirmation_size                        2
#define Discovery_size                           278
#define Packet_size                              295

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif