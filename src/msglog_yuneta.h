/****************************************************************************
 *              MSGLOG_YUNETA.H
 *              Copyright (c) 2015 Niyamaka.
 *              All Rights Reserved.
 ****************************************************************************/
#ifndef _MSGLOG_YUNETA_H
#define _MSGLOG_YUNETA_H 1


#ifdef __cplusplus
extern "C"{
#endif

/*
 *   Yuneta Error's MSGLOGs
 */
#define MSGSET_RUNTIME_ERROR            "Runtime error"
#define MSGSET_CONFIGURATION_ERROR      "Configuration error"
#define MSGSET_PROTOCOL_ERROR           "Protocol error"
#define MSGSET_HTTP_PARSER              "HTTP parser error"
#define MSGSET_APP_ERROR                "App error"
#define MSGSET_LIBCURL_ERROR            "Curl error"
#define MSGSET_SERVICE_ERROR            "Service error"
#define MSGSET_DATABASE_ERROR           "Database error"

/*
 *   Yuneta Info's MSGLOGs
 */
#define MSGSET_START_STOP               "Start Stop"
#define MSGSET_PERSISTENT_IEVENTS       "Persistent IEvents"
#define MSGSET_CONNECT_DISCONNECT       "Connect Disconnect"
#define MSGSET_OPEN_CLOSE               "Open Close"
#define MSGSET_DATABASE                 "Database"
#define MSGSET_CREATION_DELETION_GOBJS  "Creation Deletion GObjs"
#define MSGSET_SUBSCRIPTIONS            "Creation Deletion Subscriptions"
#define MSGSET_BAD_BEHAVIOUR            "Bad Behaviour"
#define MSGSET_INFO                     "Info"
#define MSGSET_PROTOCOL_INFO            "Protocol info"


#ifdef __cplusplus
}
#endif

#endif /* _MSGLOG_YUNETA_H */
