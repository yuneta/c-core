/***********************************************************************
 *          C_ROUTER.C
 *          GClass of ROUTER gobj.
 *

┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃                         Router intra-events                                  ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛
                    ▲                               ▲                       ┃
                    ┃ static route                  ┃ dynamic routes        ┃
                    ┃                               ┃                       ┃
                    ▼                               ▼                       ┃
┌────────────┐  ┌───────────────────┐          ┌───────────────────┐        ┃
│ Persitence │◀▶│  ROUTE (s)        │ ▶━━━━━━◀ │  ROUTE (d)        │◀▶Persistence
└────────────┘  └───────────────────┘    ┃     └───────────────────┘        ┃
                    ▲  ▲  ▲        knowing queues   ▲  ▲  ▲                 ┃
        role        ┃  ┃  ┃         ◀━━━━┃━━━━▶     ┃ ┃ ┃ ┃                 ┃
        multi-route ┃  ┃  ┃              ┃          ┃ ┃ ┃ ┃                 ┃
        balancer    ┃  ┃  ┃              ┃          ┃ ┃ ┃ ┃                 ┃
        round-robin ┃  ┃  ┃              ┃          ┃ ┃ ┃ ┃                 ┃
                    ▼  ▼  ▼              ┃          ▼ ▼ ▼ ▼                 ┃
                ┌───────────────────┐    ┃     ┌───────────────────┐        ┃
                │ Websocket client  │    ┃     │ Websocket server  │        ┃
                └───────────────────┘    ┃     └───────────────────┘        ┃
                ┌───────────────────┐    ┃          ┃ ┃ ┃ ┃                 ┃
                │ GConnex           │    ┃          ┃ ┃ ┃ ┃                 ┃
                └───────────────────┘    ┃          ┃ ┃ ┃ ┃                 ┃
                    ▲  ▲  ▲              ┃          ┃ ┃ ┃ ┃                 ┃
    primary/2nd/.━▶ ┃  ┃  ┃              ┃          ┃ ┃ ┃ ┃                 ┃
    end point       ▼  ▼  ▼              ┃          ▼ ▼ ▼ ▼                 ▼
                ┌───────────────────┐    ┃      ┌───────────────────┐    ┌──────────┐
                │ Tcp0 client       │◀━queue━━━▶│ Tcp0 clisrv       │◀━━ │ Tcp0Srv  │
                └───────────────────┘    ┃      └───────────────────┘    └──────────┘
                                         ┃
                                         ▼
                                    send_event  EV_RELAUNCH EV_HELP to yuno!!
                                    (1)

   Static routes can scale                      Dynamic routes can scale
   launching yunos                            launching yunos of "origin" role
   of "destination" role                        (very rare, isn't it?).


Only static routes MUST send his identity card.
Static routes represent the client, asking for a service:
    - showing his identity card (self-name, self-role + back_way_roles).

The server answer with his identity-card ack:
    - showing his identity card (self-name, self-role subcontracted_roles).

    Yuno-Srv                          Yuno-Cli
    ----------                          ----------
    dynamic-route       ◀━━━━━━━━━━     static-route
                                        identity-card
                                        -my identity: (myrole + (^)back_way_roles)


    identity-card ACK   ━━━━━━━━━━▶
    (myrole + (*)subcontracted_roles)

The client must decide if the answer of server match his need,
updating his data or dropping the connection.


Keys of router system:

__persistent_event__    Public      bool, True if persistent event, saved in disk to ensure delivery

__event_priority__      Public      number, from 1 to 9, lower is highest priority, default is 5
                                    (No used by now)
__inter_event__         Public      original INTER_EVENT received, included in local sent event.
__persistence_reference__  Private
__persistence_saved__   Private

__inform_action_return__ Public     bool. When true, responds with an inter-event
                                    with the same event name and
                                    with the action return to the source gobj.
__action_return__       Public      number or json, with the action return
                                    C yunos will use __info_msg__ for return info,
                                    and this will be always an integer,
                                    but js yunos will return complex data here.
__info_msg__            Public      info msg, in the action return event

__original_event_name__ Public      puesto por gobj cuando se cambia el nombre del evento
__subscription_reference__ Public   puesto por gobj

This two events are managed by Persistence to interchange messages
Also if the message is marked with __persistent_event__ the message is save in disk
until the acknowledgment of receipt is received.

EV_PERSISTENT_IEVENT_ACK

When a message cannot be sent becouse all his routes are disconnected,
if the message is persistent then is saved by Persitence gobj,
else the message is enqueued in Router.
Each a route is connected, the router tries to send the enqueued messages.

WARNING: the order of sent outside events can be not in order
if you are mixing persistent and no-persistent events,
because they have different queues.
When a route is open, the persistent events are sent first.



(1) Como accede router a conocer el queue size de los tcp0?
Podría crear un atributo comun a todos los gobj, que indicará el "nivel de ocupación".
El padre puede preguntar al hijo, y este al hijo, etc, así devolvería el nivel de ocupación
en esa rama o hilo. En realidad en este caso, el que tiene el dato es el último gobj, Tcp0.
Es un medidor generico, algo así como el "uso de cpu del gobj",
que podria ser útil en otros casos de uso, que mediria el hilo completo.
En este caso, como manejamos mensajes en estructura GBUFFER, el resultado podría ser
el número total de gbuffs que hay en el hilo.
El que interroga, decidirá que peso tiene ese número de gbufs encolados.
Este dato tendra que ser publicado por snmp, y que el monitor decida.Uff, mejor por trap claro.

Los mensajes encolados debería medirse por level, o por mensajes por segundo!!
Bueno, hay que tener la estadistica de mensajes por segundo,
pero realmente mientras se procesen y no haya encolamientos,
los msg/sec no son utiles para decidir el relaunch.
Los smart useful es el queue size... como en Mercadona!..
cuando crece el tamaño de las colas de un nivel configurado, entonces se abren mas colas.



But the gobj with the queue level sensor must to known the destination role to relaunch more.
o.... the tcp0 for example, broadcast a event with the alert of queue level.
There can exist a gobj that their mission was relanch new instances,
automatically and locally or sending the event to a centralized system responsibled of
relanch new instances.



 *          Copyright (c) 2013 Niyamaka.
 *          All Rights Reserved.
 ***********************************************************************/
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <pcre2posix.h>
#include <time.h>
#include "c_router.h"

/***************************************************************************
 *              Constants
 ***************************************************************************/
typedef enum  {
    STATIC_ROUTE = 1,
    DYNAMIC_ROUTE,
} route_type;

enum {
    // for static route: if yuno_name mismatch, then drop de connection.
    // (We do know who we want)
    ROUTE_REFUSE_BY_YUNO_NAME_MISMATCH      = 0x0001,
    // for static route: if roles mismatch, then drop de connection.
    // (We do know who we want)
    ROUTE_REFUSE_BY_YUNO_ROLE_MISMATCH      = 0x0002,
    // for static route: yuno launched by yuneta_agent,
    // Inform of that in the __identity_card__
    ROUTE_CONTROLLED_BY_AGENT               = 0x0004,
    ROUTE_CONTROLLED_BY_DYNAGENT            = 0x0008,
    // for static route: exit on close,
    ROUTE_EXIT_ON_CLOSE                     = 0x1000,
};

#define PREFIX_WS_DYNAMIC "srvcli-"
#define PREFIX_WS_STATIC  "wscli"

/***************************************************************************
 *              Structures
 ***************************************************************************/
typedef struct _EXTERNAL_SUBSCRIPTION {  /* Container of external subscription */
    DL_ITEM_FIELDS

    hsdata inter_event;
    hsdata route;
    // TODO hsubs isubscription;    /* reference to internal subscription */
    int isubscription;
} EXTERNAL_SUBSCRIPTION;


/***************************************************************************
 *              Prototypes
 ***************************************************************************/
PRIVATE void fill_room_shelf(
    hgobj gobj,
    hsdata route,
    char *room,
    int room_size,
    char *shelf,
    int shelf_size
);
PRIVATE int persist_ievent(hgobj gobj, hsdata route, hsdata iev);
PRIVATE uint32_t live_time(void);

//TODO FUTURE
// a function to send json_t *jn_list_back_way_roles_list,
// previously was send in send_identity_card
// but better to do a new message for this

PRIVATE const char *route_repr(hsdata route);
PRIVATE void send_identity_card(hgobj gobj, hsdata static_route);
PRIVATE void route_destroy(hgobj gobj, hsdata route);
PRIVATE hsdata route_find(hgobj gobj, const char *name);

/***************************************************************************
 *          Data: config, public data, private data
 ***************************************************************************/

// yuno_names (original and/or updated) can be used for role balance.

PRIVATE sdata_desc_t routeTb_it[] = {
SDATA (ASN_UNSIGNED,    "index",            SDF_NOTACCESS, 0,  "table index"),
SDATA (ASN_INTEGER,     "type",             0, 0,  "route_type type"),
SDATA (ASN_OCTET_STR,   "orig_yuno_name",   0, 0,  "original yuno name from config"),
SDATA (ASN_OCTET_STR,   "yunoName",         SDF_RD, 0,  "yuno name updated by partner identity card ack"),
SDATA (ASN_OCTET_STR,   "orig_yuno_role",   0, 0,  "original yuno role from config"),
SDATA (ASN_OCTET_STR,   "yunoRole",         SDF_RD, 0,  "yuno role updated by partner identity card ack"),
SDATA (ASN_BOOLEAN,     "idGot",            SDF_RD, 0,  "dynamic route received identity"),
SDATA (ASN_BOOLEAN,     "idAck",            SDF_RD, 0,  "static route received identity ack"),
SDATA (ASN_BOOLEAN,     "connected",        SDF_RD, 0,  "reflects on_open/on_close"),
SDATA (ASN_OCTET_STR,   "repr",             SDF_RD, 0,  "route representation"),
SDATA (ASN_COUNTER64,   "mrkBlnce",         SDF_RD, 0,  "counter for balance traffic"),
SDATA (ASN_POINTER,     "user_data",        0, 0,  "user data"),
SDATA (ASN_POINTER,     "user_data2",       0, 0,  "more user data"),
SDATA (ASN_POINTER,     "ws",               0, 0,  "hgobj ws"),
SDATA (ASN_UNSIGNED,    "flag",             0, 0,  "route flags"),
SDATA (ASN_DL_LIST,     "dl_subscriptions", 0, 0,  "insert and delete income from outside"),
SDATA (ASN_UNSIGNED64,  "timeout_counter",  0, 0,  "retry timeout for pending idAck/idGot"),
SDATA_END()
};

/*---------------------------------------------*
 *      Attributes - order affect to oid's
 *---------------------------------------------*/
PRIVATE sdata_desc_t tattr_desc[] = {
SDATA (ASN_BOOLEAN,     "server",           SDF_RD,  0,  "True if raise a server router. Any yuno with public services must raise one"),
SDATA (ASN_UNSIGNED,    "maxSizeDlTx",      SDF_RD,  1000, "size of tx queue"),
SDATA (ASN_COUNTER64,   "txBytes",          SDF_RD,  0, "Transmitted bytes"),
SDATA (ASN_COUNTER64,   "rxBytes",          SDF_RD,  0, "Received bytes"),
SDATA (ASN_COUNTER64,   "txMsgs",           SDF_RD,  0, "Transmitted messages"),
SDATA (ASN_COUNTER64,   "rxMsgs",           SDF_RD,  0, "Received messages"),
SDATA (ASN_SCHEMA,      "stTb",             SDF_RD,  routeTb_it, "Static route table"),
SDATA (ASN_SCHEMA,      "dyTb",             SDF_RD,  routeTb_it, "Dynamic route table"),
SDATA (ASN_UNSIGNED,    "timeout_polling",  SDF_WRITABLE, 5*1000, "poll timeout"),
SDATA (ASN_UNSIGNED,    "idAck_timeout",    SDF_WRITABLE, 5, "timeout in seconds for static routes retries"),
SDATA (ASN_UNSIGNED,    "idGot_timeout",    SDF_WRITABLE, 5, "timeout in seconds for dynamic routes retries"),
SDATA (ASN_UNSIGNED,    "maximum_events",   SDF_WRITABLE, 100, "maximum loading events for retries"),
SDATA (ASN_OCTET_STR,   "url",              0, "ws://127.0.0.1:%d",   "router server url"),
SDATA (ASN_JSON,        "static_routes",    0, "[]", // traffic balance
"List of dict with static routes: "
"[{"
"   name:str,"
"   role:str,"
"   urls:[str's],"  // urls: ip primary, secondary,etc
"   refuse_by_yuno_name_mismatch: bool,"
"   refuse_by_role_mismatch: bool,"
"   controlled_by_agent: bool,"
"   controlled_by_dynagent: bool,"
"   exit_on_close: bool"
"}]"
""
"The router will be traffic balance over the list of static routes"
"The router will be primary/secondary/etc connection over the `urls` of each route"),
SDATA_END()
};

/*---------------------------------------------*
 *      GClass trace levels
 *---------------------------------------------*/
enum {
    TRACE_ROUTING = 0x0001,
};
PRIVATE const trace_level_t s_user_trace_level[16] = {
{"routing",        "Trace routing"},
{0, 0},
};


/*---------------------------------------------*
 *              Private data
 *---------------------------------------------*/
typedef struct _PRIVATE_DATA {
    const char *my_yuno_name;
    const char *my_role;
    char server;
    const char *url;
    json_t *jn_list_static_routes;

    htable tb_static_routes;
    htable tb_dynamic_routes;

    hgobj hgobj_server;
    hgobj persist;
    hgobj timer;

    size_t counter_clisrv;
} PRIVATE_DATA;




            /***************************
             *      Framework Methods
             ***************************/




/***************************************************************************
 *      Framework Method create
 ***************************************************************************/
PRIVATE void mt_create(hgobj gobj)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    live_time();

    priv->my_yuno_name = gobj_yuno_name();
    priv->my_role = gobj_yuno_role();

    priv->server = gobj_read_bool_attr(gobj, "server");
    priv->url = gobj_read_str_attr(gobj, "url");

    priv->jn_list_static_routes = gobj_read_json_attr(gobj, "static_routes");

    /*
     *  Set the periodic timer to retry resend
     */
    priv->timer = gobj_create("", GCLASS_TIMER, 0, gobj);

    /*------------------------------------*
     *      Route tables
     *------------------------------------*/
    priv->tb_static_routes = gobj_read_table_attr(gobj, "stTb");
    priv->tb_dynamic_routes = gobj_read_table_attr(gobj, "dyTb");;

    /*--------------------------------------------*
     *      Server zone:
     *  Publish our role
     *  start a serversock with websocket filter
     *--------------------------------------------*/
    if(priv->server) {
        char url[256];
        snprintf(url, sizeof(url), priv->url, 0); // TODO conf_get_routerPort());
        gobj_write_str_attr(gobj, "url", url);
        json_t *kw_ws = json_pack("{s:b}", // kw for gwebsocket
            "iamServer", 1
        );

        json_t *kw_ss = json_pack("{s:s, s:s, s:I, s:o, s:s}", // kw for serversock
            "top_name", PREFIX_WS_DYNAMIC,
            "top_gclass_name", GCLASS_GWEBSOCKET_NAME,
            "top_parent", (json_int_t)(size_t)gobj,
            "top_kw", kw_ws,
            "url", url
        );
        /*
         *  ServerSock, we pass kw needed for create a filter on each new clisrv.
         */
        priv->hgobj_server = gobj_create(
            "router",
            GCLASS_TCP_S0,
            kw_ss,
            gobj
        );
    }

    /*------------------------------------*
     *      Static routes
     *------------------------------------*/
    {
        size_t index;
        json_t *ovalue;

        if(json_is_array(priv->jn_list_static_routes)) {
            json_array_foreach(priv->jn_list_static_routes, index, ovalue) {
                if(!json_is_object(ovalue)) {
                    log_error(0,
                        "gobj",         "%s", gobj_full_name(gobj),
                        "function",     "%s", __FUNCTION__,
                        "msgset",       "%s", MSGSET_PARAMETER_ERROR,
                        "msg",          "%s", "list item MUST BE a json object",
                        "path",         "%s", "static_routes[]",
                        NULL
                    );
                    continue;
                }
                json_incref(ovalue);
                gobj_send_event(gobj, "EV_ADD_STATIC_ROUTE", ovalue, gobj);
            }
        }
    }
}

/***************************************************************************
 *      Framework Method start
 ***************************************************************************/
PRIVATE int mt_start(hgobj gobj)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    /*------------------------------------*
     *      Get persistence service
     *------------------------------------*/
    priv->persist = gobj_find_service("persist");

    if(priv->hgobj_server) {
        gobj_start(priv->hgobj_server);
    }
    hsdata route = table_first_row(priv->tb_static_routes);
    while(route) {
        hgobj ws = sdata_read_pointer(route, "ws");
        gobj_start(ws);
        route = table_next_row(route);
    }

    gobj_start(priv->timer);
    set_timeout_periodic(priv->timer, gobj_read_uint32_attr(gobj, "timeout_polling"));

    return 0;
}

/***************************************************************************
 *      Framework Method stop
 ***************************************************************************/
PRIVATE int mt_stop(hgobj gobj)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    if(priv->hgobj_server)
        gobj_stop(priv->hgobj_server);

    clear_timeout(priv->timer);
    gobj_stop(priv->timer);

    hsdata route;
    route = table_first_row(priv->tb_static_routes);
    while(route) {
        hgobj ws = sdata_read_pointer(route, "ws");
        gobj_stop(ws);
        route = table_next_row(route);
    }
    route = table_first_row(priv->tb_dynamic_routes);
    while(route) {
        hgobj ws = sdata_read_pointer(route, "ws");
        gobj_stop(ws);
        route = table_next_row(route);
    }

    return 0;
}

/***************************************************************************
 *      Framework Method destroy
 ***************************************************************************/
PRIVATE void mt_destroy(hgobj gobj)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    table_flush(priv->tb_static_routes);
    priv->tb_static_routes = 0;

    table_flush(priv->tb_dynamic_routes);
    priv->tb_dynamic_routes = 0;
}




            /***************************
             *      Local Methods
             ***************************/




/***************************************************************************
 *  Live time
 ***************************************************************************/
PRIVATE uint32_t live_time(void)
{
    static time_t start_t=0;

    if(start_t==0)
        time(&start_t);
    time_t t;
    time(&t);
    return (uint32_t)difftime(t, start_t);
}

/***************************************************************************
 *  Centralized Log routing error
 ***************************************************************************/
PRIVATE void log_routing_error(
    hgobj gobj,
    const char *function,
    hsdata inter_event,
    const char *msg)
{
    log_error(0,
        "gobj",         "%s", gobj_full_name(gobj),
        "function",     "%s", function,
        "msgset",       "%s", MSGSET_ROUTER_PROTOCOL,
        "msg",          "%s", msg,
        NULL
    );
    trace_inter_event("ERROR", inter_event);
}

/***************************************************************************
 *  make a external subscription
 ***************************************************************************/
PRIVATE EXTERNAL_SUBSCRIPTION *make_external_subscription(
    hsdata inter_event,
    hsdata route)
{
    EXTERNAL_SUBSCRIPTION * ext_subs;

    ext_subs = gbmem_malloc(sizeof(EXTERNAL_SUBSCRIPTION));
    if(!ext_subs) {
        log_error(0,
            "function",         "%s", __FUNCTION__,
            "msgset",           "%s", MSGSET_MEMORY_ERROR,
            "msg",              "%s", "no memory for sizeof(EXTERNAL_SUBSCRIPTION)",
            "sizeof(EXTERNAL_SUBSCRIPTION)", "%d", sizeof(EXTERNAL_SUBSCRIPTION),
            NULL
        );
        iev_decref(inter_event);
        return 0;
    }
    ext_subs->inter_event = inter_event; // the inter_event with the __subscribe_event__ order

    ext_subs->route = route;

    dl_list_t *dl_subscriptions = sdata_read_dl_list(route, "dl_subscriptions");
    dl_add(dl_subscriptions, ext_subs);

    return ext_subs;
}

/***************************************************************************
 *  delete a external subscription
 ***************************************************************************/
PRIVATE void delete_external_subscription(EXTERNAL_SUBSCRIPTION *ext_subs)
{
    dl_delete(sdata_read_dl_list(ext_subs->route, "dl_subscriptions"), ext_subs, 0);

    if(ext_subs->isubscription) {
        //  TODO gobj_delete_subscription(ext_subs->isubscription);
        ext_subs->isubscription = 0;
    }
    if(ext_subs->inter_event) {
        iev_decref(ext_subs->inter_event);
        ext_subs->inter_event = 0;
    }
    gbmem_free(ext_subs);
}

/***************************************************************************
 *  find a external subscription
 ***************************************************************************/
PRIVATE EXTERNAL_SUBSCRIPTION *find_external_subscription(
    hsdata inter_event,
    hsdata route)
{
    EXTERNAL_SUBSCRIPTION *ext_subs;
    hsdata it;

    ext_subs = dl_first(sdata_read_dl_list(route, "dl_subscriptions"));
    while(ext_subs) {
        it = ext_subs->inter_event;
        if(strcmp(
                sdata_read_str(it, "iev_dst_yuno"),
                sdata_read_str(inter_event, "iev_dst_yuno")
                )==0 &&
            strcmp(
                sdata_read_str(it, "iev_dst_role"),
                sdata_read_str(inter_event, "iev_dst_role")
                )==0 &&
            strcmp(
                sdata_read_str(it, "iev_dst_srv"),
                sdata_read_str(inter_event, "iev_dst_srv")
                )==0 &&
            strcmp(
                sdata_read_str(it, "iev_event"),
                sdata_read_str(inter_event, "iev_event")
                )==0 &&
            strcmp(
                sdata_read_str(it, "iev_src_yuno"),
                sdata_read_str(inter_event, "iev_src_yuno")
                )==0 &&
            strcmp(
                sdata_read_str(it, "iev_src_role"),
                sdata_read_str(inter_event, "iev_src_role")
                )==0 &&
            strcmp(
                sdata_read_str(it, "iev_src_srv"),
                sdata_read_str(inter_event, "iev_src_srv")
                )==0
            ) {
            return ext_subs;
        }
        ext_subs = dl_next(ext_subs);
    }
    return 0;
}

/***************************************************************************
 *  Create route
 ***************************************************************************/
PRIVATE hsdata route_create(
    hgobj gobj,
    route_type type,
    const char *yuno_name,
    const char *yuno_role,
    hgobj ws,
    uint32_t flag)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    hsdata route;

    if(type == STATIC_ROUTE) {
        route = table_create_row(priv->tb_static_routes);
    } else {
        route = table_create_row(priv->tb_dynamic_routes);
    }
    table_append_row(route);

    sdata_write_int32(route, "type", type);
    sdata_write_str(route, "orig_yuno_name", yuno_name);
    sdata_write_str(route, "yunoName", yuno_name);
    sdata_write_str(route, "orig_yuno_role", yuno_role);
    sdata_write_str(route, "yunoRole", yuno_role);

    sdata_write_pointer(route, "ws", ws);
    sdata_write_uint32(route, "flag", flag);
    sdata_write_bool(route, "idGot", 0);
    sdata_write_bool(route, "idAck", 0);
    sdata_write_bool(route, "connected", 0);
    sdata_write_str(route, "repr", 0);

    return route;
}

/***************************************************************************
 *  Destroy route
 ***************************************************************************/
PRIVATE void route_destroy(hgobj gobj, hsdata route)
{
    EXTERNAL_SUBSCRIPTION *ext_subs;

    dl_list_t *ndl;
    ndl = sdata_read_dl_list(route, "dl_subscriptions");
    while((ext_subs = dl_first(ndl))!=0) {
        delete_external_subscription(ext_subs);
    }
    table_delete_row(route);
}

/***************************************************************************
 *  Find route
 ***************************************************************************/
PRIVATE hsdata route_find(hgobj gobj, const char *name)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    hsdata route = table_first_row(priv->tb_static_routes);
    while(route) {
        const char *yunoName = sdata_read_str(route, "yunoName");
        if(strcmp(yunoName, name)==0) {
            return route;
        }
        route = table_next_row(route);
    }
    return 0;
}

/***************************************************************************
 *  Reload and send persistent inter-events of route
 *  Return # of iev's loaded
 ***************************************************************************/
PRIVATE int reload_route_interevents(hgobj gobj, hsdata route)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    if(!priv->persist || !gobj_is_running(priv->persist)) {
        return 0;
    }

    json_t *extra_kw = json_pack("{s:I}",
        "__route__", (json_int_t)route
    );
    char room[80];
    char shelf[80];
    fill_room_shelf(gobj, route, room, sizeof(room), shelf, sizeof(shelf));

    json_t *kw_pers = json_pack("{s:s, s:s, s:s, s:s, s:s, s:i, s:i, s:o}",
        "event2send", "EV_SEND_INTER_EVENT",
        "store", gobj_name(gobj),
        "room", room,
        "shelf", shelf,
        "reference", "*",
        "maximum_events", gobj_read_uint32_attr(gobj, "maximum_events"),
        "owner", 0,
        "extra_kw", extra_kw
    );
    return gobj_send_event(
        priv->persist,
        "EV_LOAD_IEVENT",
        kw_pers,
        gobj
    );
}

/***************************************************************************
 *  Route opened (static or dynamic).
 ***************************************************************************/
PRIVATE void publish_on_open_route(hgobj gobj, json_t *kw)
{
    /*-------------------------------------------------------*
     *  Inform of "role" open
     *  Useful for subscriptor! for launching subscriptions
     *-------------------------------------------------------*/
    json_incref(kw);
    gobj_publish_event(gobj, "EV_ON_OPEN_ROUTE", kw); // own kw

    /*-------------------------------------------*
     *  re-send pending persistent events
     *-------------------------------------------*/
    hsdata route = GET_ROUTER_ROUTE(kw);
    reload_route_interevents(gobj, route);
    JSON_DECREF(kw);
}

/***************************************************************************
 *  update route yunoName
 ***************************************************************************/
PRIVATE void update_route_yuno_name(hsdata route, const char *yuno_name)
{
    sdata_write_str(route, "yunoName", yuno_name);
}

/***************************************************************************
 *  update reoute yunoRole
 ***************************************************************************/
PRIVATE void update_route_yuno_role(hsdata route, const char *yuno_role)
{
    sdata_write_str(route, "yunoRole", yuno_role);
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE BOOL route_is_on_open(hsdata route)
{
    int type = sdata_read_int32(route, "type");
    int connected = sdata_read_bool(route, "connected");
    int identity_ack = sdata_read_int32(route, "idAck");
    int identity_got = sdata_read_int32(route, "idGot");

    if(type == STATIC_ROUTE) {
        if(connected && identity_ack) {
            return TRUE;
        }
    } else {
        if(connected && identity_got) {
            return TRUE;
        }
    }
    return FALSE;
}

/***************************************************************************
 *  Check static routes without identity_ack
 ***************************************************************************/
PRIVATE int check_identity_ack(hgobj gobj, hsdata route)
{
    time_t timeout_counter = sdata_read_uint64(route, "timeout_counter");
    if(test_sectimer(timeout_counter)) {
        // re-send identity cartd
        // By now, drop connection, as dynamic routes
        log_error(0,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_PROTOCOL_ERROR,
            "msg",          "%s", "idAck timeout",
            "repr",         "%s", route_repr(route),
            NULL
        );

        gobj_send_event(sdata_read_pointer(route, "ws"), "EV_DROP", 0, gobj);
    }

    return 0;
}

/***************************************************************************
 *  Check dynamic routes without identity_got
 ***************************************************************************/
PRIVATE int check_identity_got(hgobj gobj, hsdata route)
{
    time_t timeout_counter = sdata_read_uint64(route, "timeout_counter");
    if(test_sectimer(timeout_counter)) {
        // drop connection
        log_error(0,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_PROTOCOL_ERROR,
            "msg",          "%s", "idGot_timeout",
            "repr",         "%s", route_repr(route),
            NULL
        );
        gobj_send_event(sdata_read_pointer(route, "ws"), "EV_DROP", 0, gobj);
    }

    return 0;
}

/***************************************************************************
 *  Roles of static routes are our subcontracted roles
 *  Return string's array with roles of static routes (*)
 ***************************************************************************/
// PRIVATE json_t * get_subcontracted_roles(hgobj gobj)
// {
//     PRIVATE_DATA *priv = gobj_priv_data(gobj);
//     json_t *found_roles = json_array();
//
//     /*
//      *  search in static routes
//      */
//     hsdata route = table_first_row(priv->tb_static_routes);
//     while(route) {
//         if(!route_is_on_open(route)) {
//             route = table_next_row(route);
//             continue;
//         }
//
//         size_t index;
//         json_t *jn_str;
//         json_t *roles_list = sdata_read_json(route, "roleList");
//         json_array_foreach(roles_list, index, jn_str) {
//             if(!json_is_string(jn_str)) {
//                 continue;
//             }
//             const char *str = json_string_value(jn_str);
//             if(strcmp(str, priv->my_role)==0) {
//                 continue;
//             }
//             char *s = gbmem_malloc(strlen(str)+2);
//             strcpy(s, "*");
//             strcat(s, str);
//             if(json_str_in_list(found_roles, s, TRUE)) {
//                 gbmem_free(s);
//                 continue;
//             }
//             json_array_append_new(found_roles, json_string(s));
//             gbmem_free(s);
//         }
//         route = table_next_row(route);
//     }
//     return found_roles;
// }

/***************************************************************************
 *  Return String's array with roles of dynamic routes (^)
 ***************************************************************************/
// PRIVATE json_t *get_back_way_roles(hgobj gobj)
// {
//     PRIVATE_DATA *priv = gobj_priv_data(gobj);
//     json_t *found_roles = json_array();
//
//     /*
//      *  search in dynamic routes
//      */
//     hsdata route;
//
//     route = table_first_row(priv->tb_dynamic_routes);
//     while(route) {
//         if(!route_is_on_open(route)) {
//             route = table_next_row(route);
//             continue;
//         }
//
//         size_t index;
//         json_t *jn_str;
//
//         json_t *roles_list = sdata_read_json(route, "roleList");
//         json_array_foreach(roles_list, index, jn_str) {
//             if(!json_is_string(jn_str)) {
//                 continue;
//             }
//             const char *str = json_string_value(jn_str);
//             if(strcmp(str, priv->my_role)==0) {
//                 continue;
//             }
//             char *s = gbmem_malloc(strlen(str)+2);
//             strcpy(s, "^");
//             strcat(s, str);
//             if(json_str_in_list(found_roles, s, TRUE)) {
//                 gbmem_free(s);
//                 continue;
//             }
//             json_array_append_new(found_roles, json_string(s));
//             gbmem_free(s);
//         }
//         route = table_next_row(route);
//     }
//     return found_roles;
// }

/***************************************************************************
 *  Re-send our identity card to static routes:
 *  there is some new back_way_role
 ***************************************************************************/
// PRIVATE void broadcast_role_back_way(hgobj gobj, json_t *back_way_roles)
// {
//     PRIVATE_DATA *priv = gobj_priv_data(gobj);
//
//     if(__trace_router__) {
//         char *s = json_dumps(back_way_roles, JSON_PRESERVE_ORDER|JSON_INDENT(4));
//         if(s) {
//             log_debug_printf("BROADCAST BACK WAY ====>", s);
//             gbmem_free(s);
//         }
//     }
//     /*
//      * search in static routes
//      */
//     hsdata route = table_first_row(priv->tb_static_routes);
//     while(route) {
//         if(!route_is_on_open(route)) {
//             route = table_next_row(route);
//             continue;
//         }
//         if(json_str_in_list(back_way_roles, priv->my_role, FALSE)) {
//             // avoid inter-yuno loops
//             route = table_next_row(route);
//             continue;
//         }
//
//         json_incref(back_way_roles);
//         send_identity_card(gobj, route, back_way_roles, 0);
//         route = table_next_row(route);
//     }
//     JSON_DECREF(back_way_roles);
// }

/***************************************************************************
 *  Broadcast forward the new role's back ways.
 ***************************************************************************/
// PRIVATE void check_broadcast_roles_back_way(hgobj gobj, json_t *roles_list)
// {
//     PRIVATE_DATA *priv = gobj_priv_data(gobj);
//     json_t * broadcasted_roles = json_array();
//     json_t * to_broadcast = json_array();
//
//     // remove **all** leading ^
//     json_t * pure_roles = json_list_lstrip(roles_list, '^');
//
//     size_t index;
//     json_t *jn_str;
//
//     json_array_foreach(pure_roles, index, jn_str) {
//         if(!json_is_string(jn_str)) {
//             continue;
//         }
//         const char *ro = json_string_value(jn_str);
//         if(strcmp(ro, priv->my_role)==0) {
//             // avoid inter-yuno loops
//             JSON_DECREF(to_broadcast);
//             JSON_DECREF(pure_roles);
//             JSON_DECREF(broadcasted_roles);
//             return;
//         }
//         if(!json_str_in_list(broadcasted_roles, ro, TRUE)) {
//             json_array_append_new(broadcasted_roles, json_string(ro));
//             const char *str = helper_json_list_str(roles_list, index);
//             char *s = gbmem_malloc(strlen(str)+2);
//             if(s) {
//                 strcpy(s, "^");
//                 strcat(s, str);
//                 json_array_append_new(to_broadcast, json_string(s));
//                 gbmem_free(s);
//             }
//         }
//     }
//     if(json_array_size(to_broadcast)) {
//         broadcast_role_back_way(gobj, to_broadcast);
//     } else {
//         JSON_DECREF(to_broadcast);
//     }
//     JSON_DECREF(pure_roles);
//     JSON_DECREF(broadcasted_roles);
// }

/***************************************************************************
 *  __str__
 ***************************************************************************/
PRIVATE void update_repr(hsdata route)
{
    char bftemp[120];
    char *yunoName = sdata_read_str(route, "yunoName");
    char *yunoRole = sdata_read_str(route, "yunoRole");
    int type = sdata_read_int32(route, "type");
    snprintf(bftemp, sizeof(bftemp), "%c-%s-%s",
        type==STATIC_ROUTE?'s':'d',
        yunoRole,
        yunoName
    );
    sdata_write_str(route, "repr", bftemp);
}

/***************************************************************************
 *  __repr__
 ***************************************************************************/
PRIVATE const char *route_repr(hsdata route)
{
    char *s = sdata_read_str(route, "repr");
    if(!s) {
        update_repr(route);
        s = sdata_read_str(route, "repr");
    }
    return s;
}

/***************************************************************************
 *  send back way roles
 ***************************************************************************/
//     if(jn_list_back_way_roles_list) {
//         snprintf(prefix, sizeof(prefix),
//             "INTRA-EVENT %s:%s ^==> %s",
//             priv->my_role,
//             priv->my_yuno_name,
//             route_repr(static_route)
//         );
//     } else {
//         jn_list_back_way_roles_list = get_back_way_roles(gobj);
//         snprintf(prefix, sizeof(prefix),
//             "INTRA-EVENT %s:%s ==> %s",
//             priv->my_role,
//             priv->my_yuno_name,
//             route_repr(static_route)
//         );
//     }
//
//     json_t *my_roles_list = json_array();
//     json_array_append_new(my_roles_list, json_string(priv->my_role));
//     if(jn_list_back_way_roles_list) {
//         json_array_extend(my_roles_list, jn_list_back_way_roles_list);
//         JSON_DECREF(jn_list_back_way_roles_list);
//     }
//    JSON_DECREF(my_roles_list);


/***************************************************************************
 *  Route must be connected
 *  WARNING: inter_event is decref
 ***************************************************************************/
PRIVATE int _direct_inter_event_by_route(
    hgobj gobj,
    hsdata route_dst,
    hsdata inter_event,
    BOOL check_persistence)
{
    /*---------------------------------------*
     *  Is a persistent event?
     *---------------------------------------*/
    json_t *iev_kw = sdata_read_kw(inter_event, "iev_kw");
    if(check_persistence) {
        BOOL persistent_event = IS_PERSISTENT_IEV(iev_kw);
        if(persistent_event) {
            iev_incref(inter_event);
            persist_ievent(gobj, route_dst, inter_event); // inter_event decref
        }
    }
    if(!sdata_read_bool(route_dst, "connected")) {
        log_error(0,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_PROTOCOL_ERROR,
            "msg",          "%s", "route DISCONNECTED",
            "repr",         "%s", route_repr(route_dst),
            NULL
        );
        trace_inter_event("ERROR", inter_event);
        iev_decref(inter_event);
        return -1;
    }

    /*
     *  Borra keys
     *  Se ponen en los iev entrantes al injectarlos en el gobj destino,
     *  y vendrán en iev_kw si se ha reusado el iev_kw para la respuesta
     */
    if(kw_has_key(iev_kw, "__inter_event__")) {
        json_object_del(iev_kw, "__inter_event__");
    }
    if(kw_has_key(iev_kw, "__route__")) {
        json_object_del(iev_kw, "__route__");
    }


    if(gobj_trace_level(gobj) & TRACE_ROUTING) {
        char prefix[120];
        snprintf(prefix, sizeof(prefix),
            "INTRA-EVENT %s:%s ==> %s",
            gobj_yuno_role(),
            gobj_yuno_name(),
            route_repr(route_dst)
        );
        trace_inter_event(prefix, inter_event);
    }

    hsdata twin_inter_event = iev_twin(inter_event); // iev NOT own
    iev_decref(inter_event);
    GBUFFER *gbuf = iev2gbuffer(twin_inter_event, FALSE); // inter_event decref
    json_t *kw_tx = json_pack("{s:I}",
        "gbuffer", (json_int_t)(size_t)gbuf
    );
    return gobj_send_event(
        sdata_read_pointer(route_dst, "ws"),
        "EV_SEND_MESSAGE",
        kw_tx,
        gobj
    );
}

/***************************************************************************
 *  send our identity card if iam client
 *  we only send the identity card in static routes!
 ***************************************************************************/
PRIVATE void send_identity_card(hgobj gobj, hsdata static_route)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    uint32_t flag = sdata_read_uint32(static_route, "flag");
    BOOL controlled_by_agent = (flag & ROUTE_CONTROLLED_BY_AGENT)?1:0;
    BOOL controlled_by_dynagent = (flag & ROUTE_CONTROLLED_BY_DYNAGENT)?1:0;
    BOOL playing = gobj_is_playing(gobj_yuno());
    char *yuno_version = gobj_read_str_attr(gobj_yuno(), "yuno_version");
    char *routerPort = priv->hgobj_server?
        gobj_read_str_attr(priv->hgobj_server, "lPort"):
        "";
    hgobj hgobj_snmp = gobj_find_unique_gobj("snmp");
    char *snmpPort = hgobj_snmp?gobj_read_str_attr(hgobj_snmp, "lPort"):"";
    json_t *kw_identity_card = json_pack(
        "{s:s, s:s, s:b, s:b, s:s, s:s, s:b, s:s, s:s, s:i}",
        "yunoName", priv->my_yuno_name,
        "yunoRole", priv->my_role,
        "controlled_by_agent", controlled_by_agent,
        "controlled_by_dynagent", controlled_by_dynagent,
        "yuno_version", yuno_version,
        "yunetaVersion", __yuneta_version__,
        "playing", playing,
        "routerPort", routerPort,
        "snmpPort", snmpPort?snmpPort:"",
        "pid", getpid()
    );

    hsdata idt = iev_create(
        ".*",
        ".*",
        "router",
        "EV_IDENTITY_CARD",
        kw_identity_card,
        gobj_name(gobj)
    );

    if(gobj_trace_level(gobj) & TRACE_ROUTING) {
        char prefix[120];
        snprintf(prefix, sizeof(prefix),
            "INTRA-EVENT %s:%s ==> %s",
            priv->my_role,
            priv->my_yuno_name,
            route_repr(static_route)
        );
        trace_inter_event(prefix, idt);
    }

    GBUFFER *gbuf = iev2gbuffer(idt, FALSE);
    json_t *kw = json_pack("{s:I}",
        "gbuffer", (json_int_t)(size_t)gbuf
    );
    gobj_send_event(
        sdata_read_pointer(static_route, "ws"),
        "EV_SEND_MESSAGE",
        kw,
        gobj
    );
}

/***************************************************************************
 *  Response the ack to a persistence intra event
 ***************************************************************************/
PRIVATE int send_persistence_ack(
    hgobj gobj,
    hsdata route_dst,
    hsdata inter_event,
    const char *persistence_reference)
{
    if(empty_string(persistence_reference)) {
        log_routing_error(
            gobj,
            __FUNCTION__,
            inter_event,
            "Persistent event WITHOUT reference"
        );
        return -1;
    }

    /*---------------------------------------*
     *  Create ack event
     *---------------------------------------*/
    json_t *kw_ack = json_pack("{s:s, s:s, s:s, s:s, s:i}",
        "store", "*",
        "room", "*",
        "shelf", "*",
        "reference", persistence_reference,
        "owner", 0
    );

    hsdata iev_ack = iev_create(
        GET_IEV_SOURCE_YUNO(inter_event),
        GET_IEV_SOURCE_ROLE(inter_event),
        "persist",
        "EV_REMOVE_IEVENT",         // iev_event
        kw_ack,                     // iev_kw
        gobj_name(gobj)
    );

    return _direct_inter_event_by_route(gobj, route_dst, iev_ack, FALSE);
}

/***************************************************************************
 *  Function to be used by externals to router.
 *  Route must be connected
 *  WARNING: inter_event is decref
 ***************************************************************************/
PUBLIC int send_inter_event_by_route(
    hsdata route_dst,
    hsdata inter_event)
{
    hgobj gobj = gobj_find_service("router");
    if(!gobj) {
        log_error(0,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_CONFIGURATION_ERROR,
            "msg",          "%s", "router service NOT FOUND",
            NULL
        );
        return -1;
    }
    return _direct_inter_event_by_route(gobj, route_dst, inter_event, TRUE);
}

/***************************************************************************
 *  Fill room/shelf for persitence
 ***************************************************************************/
PRIVATE void fill_room_shelf(
    hgobj gobj,
    hsdata route,
    char *room,
    int room_size,
    char *shelf,
    int shelf_size)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    snprintf(
        room,
        room_size,
        "%s+%s",
        priv->my_role,
        priv->my_yuno_name
    );
    snprintf(
        shelf,
        shelf_size,
        "%s+%s",
        sdata_read_str(route, "yunoRole"),
        sdata_read_str(route, "yunoName")
    );
}

/***************************************************************************
 *  Persist ievent
 ***************************************************************************/
PRIVATE int persist_ievent(hgobj gobj, hsdata route, hsdata iev)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    if(!priv->persist) {
        iev_decref(iev);
        return -1;
    }

    char room[80];
    char shelf[80];
    fill_room_shelf(gobj, route, room, sizeof(room), shelf, sizeof(shelf));

    json_t *kw_pers = json_pack("{s:s, s:s, s:s, s:i, s:I}",
        "store", gobj_name(gobj),
        "room", room,
        "shelf", shelf,
        "owner", 0,
        "__inter_event__", (json_int_t)iev
    );
    return gobj_send_event(
        priv->persist,
        "EV_SAVE_IEVENT",
        kw_pers,
        gobj
    );
    return 0;
}

/***************************************************************************
 *  Clean persistence data, when a iev is repeated through several routes
 ***************************************************************************/
PRIVATE int clean_persistence(hgobj gobj, hsdata inter_event)
{
    json_t *iev_kw = sdata_read_kw(inter_event, "iev_kw");
    BOOL persistent_event = IS_PERSISTENT_IEV(iev_kw);
    if(persistent_event) {
        json_object_set_new(
            iev_kw,
            "__persistence_reference__",
            json_string("")
        );
    }
    return 0;
}

/***************************************************************************
 *  Send an inter_event to the matched routes.
 *  The routes are collecting from yuno_name/role_name regular expression match.
 *  WARNING `inter_event` is owned (decref) by this function.
 *  Return total of iev sent, -1 if error or not sent
 ***************************************************************************/
PRIVATE int route_inter_event(hgobj gobj, hsdata inter_event, BOOL check_persistence)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);
    hsdata route;
    int sent = 0;

    const char *re_yunoName = sdata_read_str(inter_event, "iev_dst_yuno");
    if(empty_string(re_yunoName))
        re_yunoName = ".*";
    const char *re_yunoRole = sdata_read_str(inter_event, "iev_dst_role");
    regex_t regex_yunoName;
    regex_t regex_yunoRole;
    int reti;

    /* Compile regular expression */
    reti = regcomp(&regex_yunoRole, re_yunoRole, 0);
    if(reti!=0) {
        log_error(0,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_PARAMETER_ERROR,
            "msg",          "%s", "regcomp() 1 FAILED",
            "re",           "%s", re_yunoRole,
            NULL
        );
        iev_decref(inter_event);
        return -1;
    }
    reti = regcomp(&regex_yunoName, re_yunoName, 0);
    if(reti!=0) {
        log_error(0,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_PARAMETER_ERROR,
            "msg",          "%s", "regcomp() 2 FAILED",
            "re",           "%s", re_yunoName,
            NULL
        );
        regfree(&regex_yunoRole);
        iev_decref(inter_event);
        return -1;
    }

    /*------------------------------------*
     *  Try to match with static routes
     *------------------------------------*/
    route = table_first_row(priv->tb_static_routes);
    while(route) {
        const char *yunoRole = sdata_read_str(route, "yunoRole");
        reti = regexec(&regex_yunoRole, yunoRole, 0, NULL, 0);
        if(reti!=0) {
            // No Match
            route = table_next_row(route);
            continue;
        }
        const char *yunoName = sdata_read_str(route, "yunoName");
        reti = regexec(&regex_yunoName, yunoName, 0, NULL, 0);
        if(reti!=0) {
            // No Match
            route = table_next_row(route);
            continue;
        }

        /*------------------------------*
         *      static route MATCHED
         *------------------------------*/
        iev_incref(inter_event);
        if(_direct_inter_event_by_route(
            gobj,
            route,
            inter_event,
            check_persistence)>=0) {
            sent++;
        }
        clean_persistence(gobj, inter_event);
        route = table_next_row(route);
    }

    /*------------------------------------*
     *  Try to match with dynamic routes
     *------------------------------------*/
    route = table_first_row(priv->tb_dynamic_routes);
    while(route) {
        const char *yunoRole = sdata_read_str(route, "yunoRole");
        reti = regexec(&regex_yunoRole, yunoRole, 0, NULL, 0);
        if(reti!=0) {
            // No Match
            route = table_next_row(route);
            continue;
        }
        const char *yunoName = sdata_read_str(route, "yunoName");
        reti = regexec(&regex_yunoName, yunoName, 0, NULL, 0);
        if(reti!=0) {
            // No Match
            route = table_next_row(route);
            continue;
        }

        /*------------------------------*
         *      dynamic route MATCHED
         *------------------------------*/
        iev_incref(inter_event);
        if(_direct_inter_event_by_route(
            gobj,
            route,
            inter_event,
            check_persistence)>=0) {
            sent++;
        }
        clean_persistence(gobj, inter_event);
        route = table_next_row(route);
    }
    regfree(&regex_yunoName);
    regfree(&regex_yunoRole);

    if(!sent) {
        // TODO si viene por aquí el inter-evento aunque sea persistente
        // se perderá.
        log_routing_error(
            gobj,
            __FUNCTION__,
            inter_event,
            "route NOT FOUND"
        );
        sent = -1;
    }
    iev_decref(inter_event);
    return sent;
}




            /***************************
             *      Actions
             ***************************/




/***************************************************************************
 *  Add static route
 ***************************************************************************/
PRIVATE int ac_add_static_route(hgobj gobj, const char *event, json_t *kw, hgobj src)
{
    const char *name = kw_get_str(kw, "name", "", 0);
    const char *role = kw_get_str(kw, "role", 0, 0);
    if(empty_string(role)) {
        log_error(0,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_PARAMETER_ERROR,
            "msg",          "%s", "role CANNOT be EMPTY",
            NULL
        );
        JSON_DECREF(kw);
        return -1;
    }

    BOOL refuse_by_yuno_name_mismatch = kw_get_bool(kw, "refuse_by_yuno_name_mismatch", FALSE, FALSE);
    BOOL refuse_by_role_mismatch = kw_get_bool(kw, "refuse_by_role_mismatch", FALSE, FALSE);
    BOOL controlled_by_agent = kw_get_bool(kw, "controlled_by_agent", FALSE, FALSE);
    BOOL controlled_by_dynagent = kw_get_bool(kw, "controlled_by_dynagent", FALSE, FALSE);
    BOOL exit_on_close = kw_get_bool(kw, "exit_on_close", FALSE, FALSE);
    json_t *jn_urls = json_object_get(kw, "urls");
    json_t *kw_connex = json_pack("{s:O, s:i, s:i}",
        "urls", jn_urls,
        "timeout_inactivity", -1,
        "timeout_between_connections", 2*1000
    );
    json_t *kw_ws = json_pack("{s:s, s:b, s:O}",
        "resource", "/",
        "iamServer", 0,         // with `iamServer` to 0, and `kw_connex`,
        "kw_connex", kw_connex  // gwebsocket has to create a connex, ant must to know how do it
    );

    /*
     *  Websocket client. We pass kw needed for create a gconnex by websocket.
     */
    char bftemp[80];
    snprintf(bftemp, sizeof(bftemp), "%s%s", PREFIX_WS_STATIC, name);
    hgobj ws = gobj_create(
        bftemp,
        GCLASS_GWEBSOCKET,
        kw_ws,
        gobj
    );
    if(gobj_is_running(gobj)) {
        gobj_start(ws);
    }

    uint32_t flag = 0;
    if(refuse_by_yuno_name_mismatch) {
        flag |= ROUTE_REFUSE_BY_YUNO_NAME_MISMATCH;
    }
    if(refuse_by_role_mismatch) {
        flag |= ROUTE_REFUSE_BY_YUNO_ROLE_MISMATCH;
    }
    if(controlled_by_agent) {
        flag |= ROUTE_CONTROLLED_BY_AGENT;
    }
    if(controlled_by_dynagent) {
        flag |= ROUTE_CONTROLLED_BY_DYNAGENT;
    }
    if(exit_on_close) {
        flag |= ROUTE_EXIT_ON_CLOSE;
    }
    hsdata static_route = route_create(gobj, STATIC_ROUTE, name, role, ws, flag);
    gobj_write_pointer_attr(ws, "user_data", static_route);

    JSON_DECREF(kw);
    return 0;
}

/***************************************************************************
 *  Delete static route
 ***************************************************************************/
PRIVATE int ac_del_static_route(hgobj gobj, const char *event, json_t *kw, hgobj src)
{
    JSON_DECREF(kw);
    return -1;
//     PRIVATE_DATA *priv = gobj_priv_data(gobj);
// TODO hay que rehacer esta clase
/*
    const char *name = kw_get_str(kw, "name", "", 0);
    if(empty_string(name)) {
        log_error(0,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_PARAMETER_ERROR,
            "msg",          "%s", "name CANNOT be EMPTY",
            NULL
        );
        JSON_DECREF(kw);
        return -1;
    }

    hsdata route = route_find(gobj, name);
    if(route) {
        route_destroy(gobj, route);
    } else {
        log_warning(0,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_PARAMETER_ERROR,
            "msg",          "%s", "route not found",
            "route",        "%s", name,
            NULL
        );
        JSON_DECREF(kw);
        return -1;
    }
*/
    JSON_DECREF(kw);
    return 0;
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE int ac_send_inter_event(hgobj gobj, const char *event, json_t *kw, hgobj src)
{
    hsdata inter_event = (hsdata)kw_get_int(kw, "__inter_event__", 0, FALSE);
    hsdata route = (hsdata)kw_get_int(kw, "__route__", 0, FALSE);

    if(!inter_event) {
        log_error(0,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_INTERNAL_ERROR,
            "msg",          "%s", "inter_event NULL",
            NULL
        );
        KW_DECREF(kw);
        return -1;
    }
    int ret;
    if(route) {
        ret = _direct_inter_event_by_route(gobj, route, inter_event, TRUE);
    } else {
        ret = route_inter_event(gobj, inter_event, TRUE);
    }

    KW_DECREF(kw);
    return ret;
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE int ac_on_open(hgobj gobj, const char *event, json_t *kw, hgobj src)
{
    hsdata route = (hsdata)gobj_read_pointer_attr(src, "user_data");
    if(route) {
        /*
         *  Static route
         */
        /*
         *  send our identity card if iam client
         *  we only send the identity card in static routes!
         */
        sdata_write_bool(route, "connected", TRUE);
        send_identity_card(gobj, route);

        uint32_t idAck_timeout = gobj_read_uint32_attr(gobj, "idAck_timeout");
        sdata_write_uint64(
            route,
            "timeout_counter",
            start_sectimer(idAck_timeout)
        );

    } else {
        /*
         *  Dynamic route
         */
        hsdata dynamic_route = route_create(gobj, DYNAMIC_ROUTE, NULL, NULL, src, 0);
        sdata_write_bool(dynamic_route, "connected", TRUE);
        gobj_write_pointer_attr(src, "user_data", dynamic_route);

        uint32_t idGot_timeout = gobj_read_uint32_attr(gobj, "idGot_timeout");
        sdata_write_uint64(
            dynamic_route,
            "timeout_counter",
            start_sectimer(idGot_timeout)
        );
    }

    JSON_DECREF(kw);
    return 0;
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE int ac_on_close(hgobj gobj, const char *event, json_t *kw, hgobj src)
{
    hsdata route = (hsdata )gobj_read_pointer_attr(src, "user_data");

    if(route) {
        sdata_write_bool(route, "connected", FALSE);

        /*-------------------------------------------------------*
         *  Inform of "role" close
         *  Useful for subscriptor!
         *-------------------------------------------------------*/
        json_t *kw_on_route = json_pack("{s:I}",
            "__route__", (json_int_t)route
        );
        gobj_publish_event(gobj, "EV_ON_CLOSE_ROUTE", kw_on_route);

        int type = sdata_read_int32(route, "type");
        if(type == STATIC_ROUTE) {
            sdata_write_bool(route, "idAck", FALSE);
            if(sdata_read_uint32(route, "flag") & ROUTE_EXIT_ON_CLOSE) {
                gobj_shutdown();
            }
        } else {
            sdata_write_bool(route, "idGot", FALSE);
            route_destroy(gobj, route);
        }
    }

    JSON_DECREF(kw);
    return 0;
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE int ac_on_message(hgobj gobj, const char *event, json_t *kw, hgobj src)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);
    GBUFFER *gbuf = (GBUFFER *)(size_t)kw_get_int(kw, "gbuffer", 0, FALSE);

    hsdata this_route = (hsdata)gobj_read_pointer_attr(src, "user_data");
    /*---------------------------------------*
     *          Get route
     *---------------------------------------*/
    if(!this_route) {
        log_error(0,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_INTERNAL_ERROR,
            "msg",          "%s", "route is NULL",
            NULL
        );
        gobj_send_event(sdata_read_pointer(this_route, "ws"), "EV_DROP", 0, gobj);
        KW_DECREF(kw);
        return -1;
    }

    // TEST ECHO to test with autobahn
    //     gbuf_reset_rd(gbuf);
    //     gbuf_incref(gbuf);
    //     json_incref(kw);
    //     gobj_send_event(src, "EV_SEND_MESSAGE", kw, gobj);
    // TEST

    /*---------------------------------------*
     *  Create inter_event from gbuf
     *---------------------------------------*/
    gbuf_incref(gbuf);
    hsdata inter_event = iev_create_from_gbuffer(gbuf);  // gbuf decref
    if(!inter_event) {
        log_error(0,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_INTERNAL_ERROR,
            "msg",          "%s", "iev_create_from_json() FAILED",
            NULL
        );
        gobj_send_event(sdata_read_pointer(this_route, "ws"), "EV_DROP", 0, gobj);
        KW_DECREF(kw);
        return -1;
    }

    // avoid reuse
    KW_DECREF(kw);
    gbuf = 0;

    /*---------------------------------------*
     *          trace inter_event
     *---------------------------------------*/
    if(gobj_trace_level(gobj) & TRACE_ROUTING) {
        char prefix[120];
        snprintf(prefix, sizeof(prefix),
            "INTRA-EVENT %s:%s <== %s",
            priv->my_role,
            priv->my_yuno_name,
            route_repr(this_route)
        );
        trace_inter_event(prefix, inter_event);
    }

    /*---------------------------------------*
     *  Do some check
     *---------------------------------------*/
    char *iev_event = sdata_read_str(inter_event, "iev_event");
    if(!(strcmp(iev_event, "EV_IDENTITY_CARD")==0 || strcmp(iev_event, "EV_IDENTITY_CARD_ACK")==0)) {
        switch(sdata_read_int32(this_route, "type")) {
        case STATIC_ROUTE:
            if(!sdata_read_bool(this_route, "idAck")) {
                log_routing_error(
                    gobj,
                    __FUNCTION__,
                    inter_event,
                    "s-route GOT __event__ WITHOUT identity_ack"
                );
                gobj_send_event(sdata_read_pointer(this_route, "ws"), "EV_DROP", 0, gobj);
                iev_decref(inter_event);
                return -1;
            }
            break;
        case DYNAMIC_ROUTE:
            if(!sdata_read_bool(this_route, "idGot")) {
                log_routing_error(
                    gobj,
                    __FUNCTION__,
                    inter_event,
                    "d-route GOT __event__ WITHOUT identity_got"
                );
                gobj_send_event(sdata_read_pointer(this_route, "ws"), "EV_DROP", 0, gobj);
                iev_decref(inter_event);
                return -1;
            }
            if(empty_string(sdata_read_str(inter_event, "iev_src_yuno"))) {
                // Set our extended yuno name
                // why I do this? (think in browsers)
                char *extended_name = sdata_read_str(this_route, "yunoName");
                sdata_write_str(inter_event, "iev_src_yuno", extended_name);
            }
            break;
        }
    }

    /*---------------------------------------*
     *  Search if it is for us or to resend
     *---------------------------------------*/
    char *re_yunoName = sdata_read_str(inter_event, "iev_dst_yuno");
    char *dyn_part = strchr(re_yunoName, '^');
    if(dyn_part)
        *dyn_part = 0;
    if(empty_string(re_yunoName))
        re_yunoName = ".*";
    const char *re_yunoRole = sdata_read_str(inter_event, "iev_dst_role");
    regex_t regex_yunoName;
    regex_t regex_yunoRole;
    int reti;

    /* Compile regular expression */
    reti = regcomp(&regex_yunoRole, re_yunoRole, 0);
    if(reti!=0) {
        log_error(0,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_PARAMETER_ERROR,
            "msg",          "%s", "regcomp() 3 FAILED",
            "re",           "%s", re_yunoRole,
            NULL
        );
        gobj_send_event(sdata_read_pointer(this_route, "ws"), "EV_DROP", 0, gobj);
        iev_decref(inter_event);
        return -1;
    }
    reti = regcomp(&regex_yunoName, re_yunoName, 0);
    if(reti!=0) {
        log_error(0,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_PARAMETER_ERROR,
            "msg",          "%s", "regcomp() 4 FAILED",
            "re",           "%s", re_yunoName,
            NULL
        );
        gobj_send_event(sdata_read_pointer(this_route, "ws"), "EV_DROP", 0, gobj);
        regfree(&regex_yunoRole);
        iev_decref(inter_event);
        return -1;
    }

    BOOL role_matched = regexec(&regex_yunoRole, priv->my_role, 0, NULL, 0);
    BOOL name_matched = regexec(&regex_yunoName, priv->my_yuno_name, 0, NULL, 0);
    regfree(&regex_yunoName);
    regfree(&regex_yunoRole);
    BOOL own = role_matched==0 && name_matched==0;
    if(!own) {
        /*-------------------------------------------*
         *  Search route for transit event.
         *-------------------------------------------*/
        if(gobj_trace_level(gobj) & TRACE_ROUTING) {
            trace_inter_event("   InterEvent in transit", inter_event);
        }
        //route_inter_event(gobj, inter_event, FALSE);
        //TODO FUTURE ievent propagation not implemented
        log_routing_error(
            gobj,
            __FUNCTION__,
            inter_event,
            "inter_event propagation NOT IMPLEMENTED"
        );
        gobj_send_event(sdata_read_pointer(this_route, "ws"), "EV_DROP", 0, gobj);
        iev_decref(inter_event);
        return -1;
    }

    /*------------------------------------*
     *  The event is for our yuno.
     *  Find the destination gobj
     *------------------------------------*/
    hgobj named_gobj = gobj_find_unique_gobj(
        sdata_read_str(inter_event, "iev_dst_srv")
    );
    if (!named_gobj) {
        log_routing_error(
            gobj,
            __FUNCTION__,
            inter_event,
            "iev_dst_srv NOT FOUND in this yuno"
        );
        gobj_send_event(sdata_read_pointer(this_route, "ws"), "EV_DROP", 0, gobj);
        iev_decref(inter_event);
        return -1;
    }

//     if(gobj_get_user_trace_level(gobj, ROUTER_TRACE_ROUTING)) {
//         //trace_inter_event(gobj, "   !!! InterEvent MINE !!!", inter_event);
//     }

    /*------------------------------------*
     *   Analyze inter_event
     *------------------------------------*/
    json_t *iev_kw = sdata_read_kw(inter_event, "iev_kw");

    /*------------------------------------*
     *   Is the event a subscription?
     *------------------------------------*/
    if(kw_get_bool(iev_kw, "__subscribe_event__", FALSE, FALSE)) {
        /*
         *  it's a external subscription
         */
        EXTERNAL_SUBSCRIPTION *ext_subs = find_external_subscription(
            inter_event,
            this_route
        );
        if(!ext_subs) {
            iev_incref(inter_event);
            ext_subs = make_external_subscription(
                inter_event,
                this_route
            );
        }
        if(!ext_subs) {
            log_routing_error(
                gobj,
                __FUNCTION__,
                inter_event,
                "CANNOT create EXTERNAL_SUBSCRIPTION"
            );
            iev_decref(inter_event);
            return -1;
        }

        json_t *subs_kw = json_pack("{s:s, s:I}",
            "__rename_event_name__", "EV_SUBSCRIPTION",
            "__subscription_reference__", (json_int_t)ext_subs
        );
//         hsubs isubscription = gobj_subscribe_event( TODO
//             named_gobj,
//             iev_event,
//             subs_kw,
//             gobj
//         );
//         if(!isubscription) {
//             log_routing_error(
//                 gobj,
//                 __FUNCTION__,
//                 inter_event,
//                 "CANNOT subscribe EVENT"
//             );
//             delete_external_subscription(ext_subs);
//             iev_decref(inter_event);
//             return -1;
//         }
//         ext_subs->isubscription = isubscription;
//         iev_decref(inter_event);
        return 0;
    }

    /*---------------------------------------*
     *   Is the event is a unsubscription?
     *---------------------------------------*/
    if(kw_get_bool(iev_kw, "__unsubscribe_event__", FALSE, FALSE)) {
        /*
         *  it's a external unsubscription
         */
        EXTERNAL_SUBSCRIPTION *ext_subs = find_external_subscription(
            inter_event,
            this_route
        );
        if(!ext_subs) {
            log_routing_error(
                gobj,
                __FUNCTION__,
                inter_event,
                "unsubscription EXTERNAL_SUBSCRIPTION NOT FOUND"
            );
        } else {
            delete_external_subscription(ext_subs);
        }
        iev_decref(inter_event);
        return 0;
    }

    /*------------------------------------*
     *   The event is a normal event
     *------------------------------------*/
    /*
     *  Prepare kw
     */
    json_object_set_new(
        iev_kw,
        "__route__",
        json_integer((json_int_t)(size_t)this_route)
    );
    json_object_set_new(    // __inter_event__=inter_event,
        iev_kw,
        "__inter_event__",
        json_integer((json_int_t)(size_t)inter_event)
    );
    kw_incref(iev_kw);

    /*
     *  Pop persistence kw
     */
    const char *persistence_reference = 0;
    BOOL is_persistent_iev = IS_PERSISTENT_IEV(iev_kw);
    if(is_persistent_iev) {
        GBMEM_STR_DUP(
            persistence_reference,
            GET_PERSISTENT_REFERENCE(iev_kw)
        );
        if(empty_string(persistence_reference)) {
            is_persistent_iev = FALSE;
        }
        json_object_del(iev_kw, "__persistent_event__");
        json_object_del(iev_kw, "__persistence_reference__");
    }

    /*
     *  inject the event in the yuno
     */
    int ret_action = gobj_send_event(
        named_gobj,
        iev_event,
        iev_kw,
        gobj
    );

    /*
     *  Send ack if persistent event
     */
    if(ret_action >= 0) {
        if(is_persistent_iev) {
            send_persistence_ack(gobj, this_route, inter_event, persistence_reference);
        }
    }
    GBMEM_FREE(persistence_reference);

    /*
     *  send action resp as event if they subscribe it.
     */
    if(kw_get_bool(iev_kw, "__inform_action_return__", 0, FALSE)) {
        json_t *kw_resp = json_pack("{s:i, s:s}",
            "__action_return__", ret_action,
            "__info_msg__", yuno_get_info_msg()
        );
        hsdata resp = iev_create_response(
            inter_event, // not own
            iev_event,
            kw_resp,
            named_gobj
        );
        _direct_inter_event_by_route(
            gobj,
            this_route,
            resp,
            FALSE
        );
        yuno_set_info_msg(0);
    }

    /*
     *  Remove inter_event
     */
    iev_decref(inter_event);
    return 0;
}

/***************************************************************************
 *  Internal subscription event has arrived.
 *  Redirect to his external subscriptor.
 ***************************************************************************/
PRIVATE int ac_subscription(hgobj gobj, const char *event, json_t *kw, hgobj src)
{
    EXTERNAL_SUBSCRIPTION *ext_subs;

    /*---------------------------------------*
     *  Recover the reference
     *---------------------------------------*/
    ext_subs = (EXTERNAL_SUBSCRIPTION *)kw_get_int(
        kw,
        "__subscription_reference__",
        0,
        TRUE
    );
    if(!ext_subs) {
        log_error(0,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_INTERNAL_ERROR,
            "msg",          "%s", "__subscription_reference__ is NULL",
            NULL
        );
        kw_decref(kw);
        return -1;
    }

    const char *original_event_name = kw_get_str(
        kw,
        "__original_event_name__",
        0,
        0
    );
    hsdata route = ext_subs->route;
    if(route && !sdata_read_bool(route, "connected")) {
        char *yunoName = sdata_read_str(route, "yunoName");
        char *yunoRole = sdata_read_str(route, "yunoRole");
        log_error(0,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_INTERNAL_ERROR,
            "msg",          "%s", "subscription route DISCONNECTED",
            "event",        "%s", event,
            "yunoRole dst", "%s", yunoRole,
            "yunoName dst", "%s", yunoName,
            NULL
        );
        kw_decref(kw);
        return -1;
    }

    /*---------------------------------------*
     *  Create inter_event
     *---------------------------------------*/
    kw_incref(kw);
    hsdata inter_event = iev_create_response(
        ext_subs->inter_event,
        original_event_name,
        kw,
        gobj_name(src)
    );
    json_object_del(kw, "__original_event_name__");
    if(!inter_event) {
        log_error(0,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_INTERNAL_ERROR,
            "msg",          "%s", "inter_event_create() FAILED",
            NULL
        );
        kw_decref(kw);
        return -1;
    }

    /*-------------------------------------------------*
     *  Route the intra event subscription from here
     *--------------------------------------------------*/
    int ret;
    if(route) {
        ret = _direct_inter_event_by_route(gobj, route, inter_event, TRUE);
    } else {
        ret = route_inter_event(gobj, inter_event, TRUE);
    }

    kw_decref(kw);
    return ret;
}

/***************************************************************************
 *  It's a dynamic route
 ***************************************************************************/
PRIVATE int ac_identity_card(hgobj gobj, const char *event, json_t *kw, hgobj src)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    hsdata this_route = GET_ROUTER_ROUTE(kw);
    hsdata inter_event = GET_INTER_EVENT(kw);

    if(sdata_read_int32(this_route, "type") != DYNAMIC_ROUTE) {
        log_routing_error(
            gobj,
            __FUNCTION__,
            inter_event,
            "GOT identity card in a NOT DYNAMIC route"
        );
        gobj_send_event(sdata_read_pointer(this_route, "ws"), "EV_DROP", 0, gobj);
        KW_DECREF(kw);
        return -1;
    }

    /*
     *  dynamic route: update their yuno name and roles
     */
    if(sdata_read_bool(this_route, "idGot")) {
        /*  Accept re-send of identity card?
         *  YES, the client re-send his identity-card
         *  when he got new back_way_roles
         *  Now NO!! back_way_roles will come in another event
         */
        log_routing_error(
            gobj,
            __FUNCTION__,
            inter_event,
            "GOT REPEATED identity card"
        );
        gobj_send_event(sdata_read_pointer(this_route, "ws"), "EV_DROP", 0, gobj);
        KW_DECREF(kw);
        return -1;
    } else {
        // First time
        const char *his_yunoName = kw_get_str(kw, "yunoName", 0, 0);
        const char *his_yunoRole = kw_get_str(kw, "yunoRole", 0, 0);

        if(empty_string(his_yunoName)) {
            void *ptr = sdata_read_pointer(this_route, "ws");
            const char *peername = gobj_read_str_attr(ptr, "peername");
            const char *sockname = gobj_read_str_attr(ptr, "sockname");
            // anonymous yuno name
            char temp[80];
            snprintf(temp, sizeof(temp), "^%s-%s", peername, sockname);
            update_route_yuno_name(this_route, temp);
        } else {
            update_route_yuno_name(this_route, his_yunoName);
        }

        if(empty_string(his_yunoRole)) {
            log_routing_error(
                gobj,
                __FUNCTION__,
                inter_event,
                "GOT identity card without yunoRole"
            );
            gobj_send_event(sdata_read_pointer(this_route, "ws"), "EV_DROP", 0, gobj);
            KW_DECREF(kw);
            return -1;
        }
        update_route_yuno_role(this_route, his_yunoRole);
        update_repr(this_route);
    }

    json_t *kw_ack = json_pack(
        "{s:s, s:s}",
        "yunoName", priv->my_yuno_name,
        "yunoRole", priv->my_role
    );

    hsdata ack = iev_create_response(
        inter_event,
        "EV_IDENTITY_CARD_ACK",
        kw_ack,
        gobj_name(gobj)
    );

    _direct_inter_event_by_route(gobj, this_route, ack, FALSE);
    sdata_write_int32(this_route, "idGot", TRUE);
    sdata_write_uint64(
        this_route,
        "timeout_counter",
        0
    );

    /*
     *  Somebody can be interested in know new clients
     *  Dynamic route
     */
    publish_on_open_route(gobj, kw); // own kw
    return 0;
}

/***************************************************************************
 *  It's a static route,
 *  getting the response from his identity card presentation.
 ***************************************************************************/
PRIVATE int ac_identity_card_ack(hgobj gobj, const char *event, json_t *kw, hgobj src)
{
    hsdata this_route = GET_ROUTER_ROUTE(kw);
    hsdata inter_event = GET_INTER_EVENT(kw);

    if(sdata_read_int32(this_route, "type") != STATIC_ROUTE) {
        log_routing_error(
            gobj,
            __FUNCTION__,
            inter_event,
            "GOT identity card ACK in a NOT STATIC route"
        );
        gobj_send_event(sdata_read_pointer(this_route, "ws"), "EV_DROP", 0, gobj);
        JSON_DECREF(kw);
        return -1;
    }

    /*
     * check if the info we have is correct
     */
    const char *his_yunoName = kw_get_str(kw, "yunoName", 0, 0);
    const char *his_yunoRole = kw_get_str(kw, "yunoRole", 0, 0);
    if(empty_string(his_yunoRole)) {
        log_routing_error(
            gobj,
            __FUNCTION__,
            inter_event,
            "GOT identity card ACK without yunoRole"
        );
        gobj_send_event(sdata_read_pointer(this_route, "ws"), "EV_DROP", 0, gobj);
        JSON_DECREF(kw);
        return -1;
    }

    const char *this_route_yunoName = sdata_read_str(this_route, "yunoName");
    if(!empty_string(this_route_yunoName)) {
        if (strcmp(this_route_yunoName, his_yunoName)!=0) {
            log_warning(0,
                "gobj",         "%s", gobj_full_name(gobj),
                "function",     "%s", __FUNCTION__,
                "msgset",       "%s", MSGSET_PARAMETER_ERROR,
                "msg",          "%s", "yuno_name MISMATCH",
                "r:yuno_name",  "%s", this_route_yunoName,
                "i:yuno_name",  "%s", his_yunoName,
                NULL
            );
            if(sdata_read_uint32(this_route, "flag") & ROUTE_REFUSE_BY_YUNO_NAME_MISMATCH) {
                gobj_send_event(sdata_read_pointer(this_route, "ws"), "EV_DROP", 0, gobj);
                JSON_DECREF(kw);
                return -1;
            }
        }
    }

    /*
     *  static route: update their yuno name
     */
    update_route_yuno_name(this_route, his_yunoName);

    /*
     * check if the info we have is correct
     */
    const char *this_route_yunoRole = sdata_read_str(this_route, "yunoRole");
    if(empty_string(this_route_yunoRole)) {
        gobj_send_event(sdata_read_pointer(this_route, "ws"), "EV_DROP", 0, gobj);
        JSON_DECREF(kw);
        return -1;
    }
    if (strcmp(this_route_yunoRole, his_yunoRole)!=0) {
        log_warning(0,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_PARAMETER_ERROR,
            "msg",          "%s", "yuno_name MISMATCH",
            "r:yuno_name",  "%s", this_route_yunoRole,
            "i:yuno_name",  "%s", his_yunoRole,
            NULL
        );
        if(sdata_read_uint32(this_route, "flag") & ROUTE_REFUSE_BY_YUNO_ROLE_MISMATCH) {
            gobj_send_event(sdata_read_pointer(this_route, "ws"), "EV_DROP", 0, gobj);
            JSON_DECREF(kw);
            return -1;
        }
    }

    /*
     *  Update role
     */
    update_route_yuno_role(this_route, his_yunoRole);

    sdata_write_bool(this_route, "idAck", TRUE);
    sdata_write_uint64(
        this_route,
        "timeout_counter",
        0
    );

    /*
     *  Somebody can be interested in knowing new connection
     *  Static route
     */
    publish_on_open_route(gobj, kw); // own kw
    return 0;
}

/***************************************************************************
 *  Play yuno
 ***************************************************************************/
PRIVATE int ac_play_yuno(hgobj gobj, const char *event, json_t *kw, hgobj src)
{
    hsdata this_route = GET_ROUTER_ROUTE(kw);
    hsdata inter_event = GET_INTER_EVENT(kw);

    int ret = gobj_play(gobj_yuno());
    json_object_set_new(
        kw,
        "__action_return__",
        json_integer(ret)
    );
    hsdata ack = iev_create_response(
        inter_event,
        "EV_PLAY_YUNO_ACK",
        kw, // use the same kw
        gobj_name(gobj)
    );
    _direct_inter_event_by_route(gobj, this_route, ack, FALSE);
    return ret;
}

/***************************************************************************
 *  Pause yuno
 ***************************************************************************/
PRIVATE int ac_pause_yuno(hgobj gobj, const char *event, json_t *kw, hgobj src)
{
    hsdata this_route = GET_ROUTER_ROUTE(kw);
    hsdata inter_event = GET_INTER_EVENT(kw);

    int ret = gobj_pause(gobj_yuno());
    json_object_set_new(
        kw,
        "__action_return__",
        json_integer(ret)
    );
    hsdata ack = iev_create_response(
        inter_event,
        "EV_PAUSE_YUNO_ACK",
        kw, // use the same kw
        gobj_name(gobj)
    );
    _direct_inter_event_by_route(gobj, this_route, ack, FALSE);
    return ret;
}

/***************************************************************************
 *  Stats yuno
 ***************************************************************************/
PRIVATE int ac_stats_yuno(hgobj gobj, const char *event, json_t *kw, hgobj src)
{
    hsdata this_route = GET_ROUTER_ROUTE(kw);
    hsdata inter_event = GET_INTER_EVENT(kw);
    //TODO const char *service = kw_get_str(kw, "service", "", 0);

    // TODO fill_service_stats(service, kw); // use the same kw

    hsdata ack = iev_create_response(
        inter_event,
        "EV_STATS_YUNO_ACK",
        kw, // use the same kw
        gobj_name(gobj)
    );
    _direct_inter_event_by_route(gobj, this_route, ack, FALSE);
    return 0;
}

/***************************************************************************
 *  Command yuno
 ***************************************************************************/
PRIVATE int ac_command_yuno(hgobj gobj, const char *event, json_t *kw, hgobj src)
{
    hsdata this_route = GET_ROUTER_ROUTE(kw);
    hsdata inter_event = GET_INTER_EVENT(kw);
    //TODO const char *command = kw_get_str(kw, "command", "", 0);
    //TODO const char *service = kw_get_str(kw, "service", "", 0);

    // TODO execute_service_command(service, command, kw); // use the same kw

    hsdata ack = iev_create_response(
        inter_event,
        "EV_COMMAND_YUNO_ACK",
        kw, // use the same kw
        gobj_name(gobj)
    );
    _direct_inter_event_by_route(gobj, this_route, ack, FALSE);
    return 0;
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE int ac_stopped(hgobj gobj, const char *event, json_t *kw, hgobj src)
{
    /*
     *  priv->timer or priv->hgobj_server or ... has stopped
     */
    JSON_DECREF(kw);
    return 0;
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE int ac_timeout_polling(hgobj gobj, const char *event, json_t *kw, hgobj src)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    /*------------------------------------*
     *  static routes
     *------------------------------------*/
    hsdata route = table_first_row(priv->tb_static_routes);
    while(route) {
        if(route_is_on_open(route)) {
            reload_route_interevents(gobj, route);
        } else {
            check_identity_ack(gobj, route);
        }
        route = table_next_row(route);
    }

    /*------------------------------------*
     *  dynamic routes
     *------------------------------------*/
    route = table_first_row(priv->tb_dynamic_routes);
    while(route) {
        if(route_is_on_open(route)) {
            reload_route_interevents(gobj, route);
        } else {
            check_identity_got(gobj, route);
        }
        route = table_next_row(route);
    }

    JSON_DECREF(kw);
    return 0;
}


/***************************************************************************
 *                          FSM
 ***************************************************************************/
PRIVATE const EVENT input_events[] = {
    // top input
    {"EV_ADD_STATIC_ROUTE",     EVF_PUBLIC_EVENT,   0,      "Add a static route"},
    {"EV_DEL_STATIC_ROUTE",     EVF_PUBLIC_EVENT,   0,      "Delete a static route"},
    {"EV_SEND_INTER_EVENT",     EVF_PUBLIC_EVENT,   0,      "Send an inter-event"},
    // bottom input
    {"EV_SUBSCRIPTION",         0},
    {"EV_ON_OPEN",              0},
    {"EV_ON_CLOSE",             0},
    {"EV_ON_MESSAGE",           0},
    {"EV_PLAY_YUNO",            0},
    {"EV_PAUSE_YUNO",           0},
    {"EV_STATS_YUNO",           0},
    {"EV_COMMAND_YUNO",         0},
    // internal
    {"EV_IDENTITY_CARD",        0},
    {"EV_IDENTITY_CARD_ACK",    0},

    {"EV_STOPPED",              0},
    {"EV_TIMEOUT",              0},
    {NULL, 0}
};
PRIVATE const EVENT output_events[] = {
    {"EV_ON_OPEN_ROUTE",        0},
    {"EV_ON_CLOSE_ROUTE",       0},
    {NULL, 0}
};
PRIVATE const char *state_names[] = {
    "ST_IDLE",
    NULL
};

PRIVATE EV_ACTION ST_IDLE[] = {
    {"EV_ON_MESSAGE",           ac_on_message,          0},
    {"EV_SEND_INTER_EVENT",     ac_send_inter_event,    0},
    {"EV_SUBSCRIPTION",         ac_subscription,        0},
    {"EV_ON_OPEN",              ac_on_open,             0},
    {"EV_ON_CLOSE",             ac_on_close,            0},

    {"EV_IDENTITY_CARD",        ac_identity_card,       0},
    {"EV_IDENTITY_CARD_ACK",    ac_identity_card_ack,   0},

    {"EV_PLAY_YUNO",            ac_play_yuno,           0},
    {"EV_PAUSE_YUNO",           ac_pause_yuno,          0},
    {"EV_STATS_YUNO",           ac_stats_yuno,          0},
    {"EV_COMMAND_YUNO",         ac_command_yuno,        0},

    {"EV_ADD_STATIC_ROUTE",     ac_add_static_route,    0},
    {"EV_DEL_STATIC_ROUTE",     ac_del_static_route,    0},

    {"EV_TIMEOUT",              ac_timeout_polling,     0},
    {"EV_STOPPED",              ac_stopped,             0},

    {0,0,0}
};

PRIVATE EV_ACTION *states[] = {
    ST_IDLE,
    NULL
};

PRIVATE FSM fsm = {
    input_events,
    output_events,
    state_names,
    states,
};

/***************************************************************************
 *              GClass
 ***************************************************************************/
/*---------------------------------------------*
 *              Local methods table
 *---------------------------------------------*/
PRIVATE LMETHOD lmt[] = {
    {0, 0, 0}
};

/*---------------------------------------------*
 *              GClass
 *---------------------------------------------*/
PRIVATE GCLASS _gclass = {
    0,  // base
    GCLASS_ROUTER_NAME,
    &fsm,
    {
        mt_create,
        mt_destroy,
        mt_start,
        mt_stop,
        0, //mt_writing,
        0, //mt_reading,
        0, //mt_subscription_added,
        0, //mt_subscription_deleted,
        0, //mt_resource_bin,
        0, //mt_resource_desc,
        0, //mt_play,
        0, //mt_pause,
        0, //mt_resource_data,
        0, //mt_child_added,
        0, //mt_child_removed,
        0, //mt_stats,
        0, //mt_command,
        0, //mt_create2,
        0, //mt_inject_event,
        0, //mt_create_resource,
        0, //mt_list_resource,
        0, //mt_save_resource,
        0, //mt_delete_resource,
        0, //mt_future21
        0, //mt_future22
        0, //mt_get_resource
        0, //mt_state_changed,
        0, //mt_authenticate,
        0, //mt_list_childs,
        0, //mt_stats_updated,
        0, //mt_disable,
        0, //mt_enable,
        0, //mt_trace_on,
        0, //mt_trace_off,
        0, //mt_gobj_created,
        0, //mt_future33,
        0, //mt_future34,
        0, //mt_publish_event,
        0, //mt_publication_pre_filter,
        0, //mt_publication_filter,
        0, //mt_authz_checker,
        0, //mt_future39,
        0, //mt_create_node,
        0, //mt_update_node,
        0, //mt_delete_node,
        0, //mt_link_nodes,
        0, //mt_future44,
        0, //mt_unlink_nodes,
        0, //mt_topic_jtree,
        0, //mt_get_node,
        0, //mt_list_nodes,
        0, //mt_shoot_snap,
        0, //mt_activate_snap,
        0, //mt_list_snaps,
        0, //mt_treedbs,
        0, //mt_treedb_topics,
        0, //mt_topic_desc,
        0, //mt_topic_links,
        0, //mt_topic_hooks,
        0, //mt_node_parents,
        0, //mt_node_childs,
        0, //mt_list_instances,
        0, //mt_node_tree,
        0, //mt_topic_size,
        0, //mt_future62,
        0, //mt_future63,
        0, //mt_future64
    },
    lmt,
    tattr_desc,
    sizeof(PRIVATE_DATA),
    0,  // acl
    s_user_trace_level,
    0,  // cmds,
    0,  // gcflag
};

/***************************************************************************
 *              Public access
 ***************************************************************************/
PUBLIC GCLASS *gclass_router(void)
{
    return &_gclass;
}
