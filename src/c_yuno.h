/****************************************************************************
 *          C_YUNO.H
 *          Container of yunos GClass.
 *          Supplies the main event loop.
 *
 * c_yuno, el padre, la raíz del arbol jerárquico.
 * Controla el event loop, o sea, la vida del yuno.
 * Es la puerta de entrada al control del yuno:
 * traces, cambios de configuración, activación de servicios, etc
 *

    Client
    ------

    external service
                Ex: yunos and yuneta utilities (yuneta,ybatch,...) being clients of agent service,
                using agent_client connector.
        │
        └━━━▷ IEvent_cli                    IEV_MESSAGE/IEvent      Acting as gate to yuneta world.
                │
                └━━━▷ IOGate                ON_MESSAGE/GBuffer
                    │
                    └━━━▷ Channel           ON_MESSAGE/GBuffer
                        │
                        └━━━▷ GWebsocket    RX_DATA
                            ...

    Server
    ------

    service (Your offer)

        │
        └━━━▷ IOGate                        IEV_MESSAGE/IEvent
                │
                └━━━▷ IEvent_srv            ON_MESSAGE/GBuffer
                    │
                    └━━━▷ Channel           ON_MESSAGE/GBuffer
                        │
                        └━━━▷ GWebsocket    RX_DATA
                        ...


 *  Copyright (c) 2013-2018 Niyamaka.
 *          All Rights Reserved.
 ****************************************************************************/

#ifndef _C_YUNO_H
#define _C_YUNO_H 1

#include <ginsfsm.h>
#include "yuneta_environment.h"
#include "c_timer.h"
#include "yuneta_version.h"

#ifdef __cplusplus
extern "C"{
#endif


/*********************************************************************
 *      GClass
 *********************************************************************/
PUBLIC GCLASS *gclass_default_yuno(void);

#define GCLASS_DEFAULT_YUNO_NAME "DefaultYuno"
#define GCLASS_DEFAULT_YUNO gclass_default_yuno()

/*
 *  Run event loop.
 *  The function blocks while some uv handler is active.
 *  Return when all uv handler has been closed.
 */
PUBLIC void mt_run(hgobj gobj);
PUBLIC void mt_clean(hgobj gobj);

/*
 *  uv loop
 */
PUBLIC void *yuno_uv_event_loop(void);

/*
 *  Used for __inform_action_return__
 */
PUBLIC int yuno_set_info_msg(const char *msg);
PUBLIC int yuno_printf_info_msg(const char *msg, ...);
PUBLIC const char *yuno_get_info_msg(void);

PUBLIC const char *yuno_app_doc(void);

/*--------------------------------------------------*
 *  Queue Stats functions
 *  Son estadísticas (atributos con la marca STAT)
 *  que se pueden considerar globales al yuno.
 *  Ofertadas por el yuno, si quieres más u otras,
 *  te las montas
 *--------------------------------------------------*/
typedef enum {
    QS_TXMSGS = 1,                  // msgs salida
    QS_RXMSGS,                      // msgs entrada
    QS_TXBYTES,                     // bytes salida
    QS_RXBYTES,                     // bytes entrada
    QS_OUPUT_QUEUE,                 // Encolamiento de salida.
    QS_INPUT_QUEUE,                 // Encolamiento de entrada.
    QS_INTERNAL_QUEUE,              // Encolamiento interno.
    QS_DROP_BY_OVERFLOW,            // Tira por overflow en la cola, sin especificar quién.
    QS_DROP_BY_NO_EXIT,             // Tira por no-salida.
    QS_DROP_BY_DOWN,                // Tira por shutdown, stop, exit.
    QS_LOWER_RESPONSE_TIME,
    QS_MEDIUM_RESPONSE_TIME,
    QS_HIGHER_RESPONSE_TIME,
    QS_REPEATED,
    QS__LAST_ITEM__
} qs_type_t;

PUBLIC int gobj_set_qs(qs_type_t qs_type, uint64_t value);
PUBLIC int gobj_incr_qs(qs_type_t qs_type, uint64_t value);
PUBLIC int gobj_decr_qs(qs_type_t qs_type, uint64_t value);
PUBLIC uint64_t gobj_get_qs(qs_type_t qs_type);

/*--------------------------------------------------*
 *  New Stats functions, using json
 *--------------------------------------------------*/
PUBLIC json_int_t yuno_set_stat(const char *path, json_int_t value); // return old value
PUBLIC json_int_t yuno_incr_stat(const char *path, json_int_t value); // return new value
PUBLIC json_int_t yuno_decr_stat(const char *path, json_int_t value); // return new value
PUBLIC json_int_t yuno_get_stat(const char *path);

PUBLIC json_int_t default_service_set_stat(const char *path, json_int_t value); // return old value
PUBLIC json_int_t default_service_incr_stat(const char *path, json_int_t value); // return new value
PUBLIC json_int_t default_service_decr_stat(const char *path, json_int_t value); // return new value
PUBLIC json_int_t default_service_get_stat(const char *path);

/*--------------------------------------------------*
 *  Allowed ips
 *--------------------------------------------------*/
PUBLIC BOOL is_ip_allowed(const char *peername);
PUBLIC int add_allowed_ip(const char *ip, BOOL allowed); // allowed: TRUE to allow, FALSE to not allow
PUBLIC int remove_allowed_ip(const char *ip); // Remove from interna list

PUBLIC BOOL is_ip_denied(const char *peername);
PUBLIC int add_denied_ip(const char *ip, BOOL denied); // denied: TRUE to deny, FALSE to not deny
PUBLIC int remove_denied_ip(const char *ip); // Remove from interna list

#ifdef __cplusplus
}
#endif


#endif

