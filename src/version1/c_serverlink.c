/***********************************************************************
 *          C_SERVERLINK.C
 *          A end point link acting as server.
 *
 *          Copyright (c) 2014 Niyamaka.
 *          All Rights Reserved.
 ***********************************************************************/
#include "c_serverlink.h"

/***************************************************************************
 *              Constants
 ***************************************************************************/

/***************************************************************************
 *              Structures
 ***************************************************************************/

/***************************************************************************
 *              Prototypes
 ***************************************************************************/


/***************************************************************************
 *          Data: config, public data, private data
 ***************************************************************************/

/*---------------------------------------------*
 *      Attributes - order affect to oid's
 *---------------------------------------------*/
PRIVATE sdata_desc_t tattr_desc[] = {
SDATA (ASN_OCTET_STR,   "url",              SDF_RD,  0, "url listening"),
SDATA (ASN_POINTER,     "user_data",        0,  0, "user data"),
SDATA (ASN_POINTER,     "user_data2",       0,  0, "more user data"),
SDATA (ASN_POINTER,     "subscriber",       0,  0, "subscriber of output-events. Default if null is parent."),
SDATA_END()
};

/*---------------------------------------------*
 *      GClass trace levels
 *---------------------------------------------*/
enum {
    TRACE_DEBUG = 0x0001,
};
PRIVATE const trace_level_t s_user_trace_level[16] = {
{"debug",        "Trace to debug"},
{0, 0},
};

/*---------------------------------------------*
 *              Private data
 *---------------------------------------------*/
typedef struct _PRIVATE_DATA {
    hgobj gobj_srv;
    hgobj gobj_clisrv;
} PRIVATE_DATA;




            /******************************
             *      Framework Methods
             ******************************/




/***************************************************************************
 *      Framework Method create
 ***************************************************************************/
PRIVATE void mt_create(hgobj gobj)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    json_t *kw_srvsock = json_pack("{s:s, s:I}",
        "url", gobj_read_str_attr(gobj, "url"),
        "subscriber", (json_int_t)(size_t)gobj
    );
    priv->gobj_srv = gobj_create("", GCLASS_TCP_S0, kw_srvsock, gobj);
    if(!priv->gobj_srv) {
        log_error(0,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_INTERNAL_ERROR,
            "msg",          "%s", "gobj_create() FAILED",
            NULL
        );
    }

    hgobj subscriber = (hgobj)gobj_read_pointer_attr(gobj, "subscriber");
    if(!subscriber)
        subscriber = gobj_parent(gobj);
    gobj_subscribe_event(gobj, NULL, NULL, subscriber);
}

/***************************************************************************
 *      Framework Method destroy
 ***************************************************************************/
PRIVATE void mt_destroy(hgobj gobj)
{
}

/***************************************************************************
 *      Framework Method start
 ***************************************************************************/
PRIVATE int mt_start(hgobj gobj)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    if(priv->gobj_srv)
        gobj_start(priv->gobj_srv);
    return 0;
}

/***************************************************************************
 *      Framework Method stop
 ***************************************************************************/
PRIVATE int mt_stop(hgobj gobj)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    if(priv->gobj_srv)
        gobj_stop(priv->gobj_srv);
    return 0;
}




            /***************************
             *      Local Methods
             ***************************/




            /***************************
             *      Actions
             ***************************/




/***************************************************************************
 *
 ***************************************************************************/
PRIVATE int ac_connected(hgobj gobj, const char *event, json_t *kw, hgobj src)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    priv->gobj_clisrv = src;
    gobj_publish_event(gobj, event, kw); // reuse kw
    return 0;
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE int ac_refuse_connected(hgobj gobj, const char *event, json_t *kw, hgobj src)
{
    gobj_send_event(src, "EV_DROP", 0, gobj);
    JSON_DECREF(kw);
    return 0;
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE int ac_disconnected(hgobj gobj, const char *event, json_t *kw, hgobj src)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    if(src == priv->gobj_clisrv) {
        priv->gobj_clisrv = 0;
        gobj_change_state(gobj, "ST_DISCONNECTED");
        gobj_publish_event(gobj, event, kw); // reuse kw
    }
    return 0;
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE int ac_rx_data(hgobj gobj, const char *event, json_t *kw, hgobj src)
{
    gobj_publish_event(gobj, event, kw); // reuse kw
    return 0;
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE int ac_tx_data(hgobj gobj, const char *event, json_t *kw, hgobj src)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    gobj_send_event(priv->gobj_clisrv, event, kw, gobj); // reuse kw

    return 0;
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE int ac_drop(hgobj gobj, const char *event, json_t *kw, hgobj src)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    gobj_send_event(priv->gobj_clisrv, event, kw, gobj); // reuse kw

    return 0;
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE int ac_timeout(hgobj gobj, const char *event, json_t *kw, hgobj src)
{
    JSON_DECREF(kw);
    return 0;
}

/***************************************************************************
 *                          FSM
 ***************************************************************************/
PRIVATE const EVENT input_events[] = {
    {"EV_RX_DATA",          0},
    {"EV_DROP",             0},
    {"EV_TX_DATA",        0},
    {"EV_CONNECTED",        0},
    {"EV_DISCONNECTED",     0},
    {"EV_TX_READY",         0},
    {"EV_TIMEOUT",          0},
    {NULL, 0}
};
PRIVATE const EVENT output_events[] = {
    {"EV_RX_DATA",          0},
    {"EV_CONNECTED",        0},
    {"EV_DISCONNECTED",     0},
    {NULL, 0}
};
PRIVATE const char *state_names[] = {
    "ST_DISCONNECTED",
    "ST_CONNECTED",
    NULL
};

PRIVATE EV_ACTION ST_DISCONNECTED[] = {
    {"EV_CONNECTED",        ac_connected,           "ST_CONNECTED"},
    {0,0,0}
};
PRIVATE EV_ACTION ST_CONNECTED[] = {
    {"EV_RX_DATA",          ac_rx_data,             0},
    {"EV_TX_DATA",          ac_tx_data,             0},
    {"EV_DROP",             ac_drop,                0},
    {"EV_CONNECTED",        ac_refuse_connected,    0},
    {"EV_DISCONNECTED",     ac_disconnected,        0},
    {"EV_TX_READY",         0,                      0},
    {"EV_TIMEOUT",          ac_timeout,             0},
    {0,0,0}
};

PRIVATE EV_ACTION *states[] = {
    ST_DISCONNECTED,
    ST_CONNECTED,
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
    GCLASS_GSERVERLINK_NAME,      // CHANGE WITH each gclass
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
        0, //mt_update_resource,
        0, //mt_delete_resource,
        0, //mt_add_child_resource_link
        0, //mt_delete_child_resource_link
        0, //mt_get_resource
        0, //mt_future24,
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
        0, //mt_future38,
        0, //mt_future39,
        0, //mt_future40,
        0, //mt_future41,
        0, //mt_future42,
        0, //mt_future43,
        0, //mt_future44,
        0, //mt_future45,
        0, //mt_future46,
        0, //mt_future47,
        0, //mt_future48,
        0, //mt_future49,
        0, //mt_future50,
        0, //mt_future51,
        0, //mt_future52,
        0, //mt_future53,
        0, //mt_future54,
        0, //mt_future55,
        0, //mt_future56,
        0, //mt_future57,
        0, //mt_future58,
        0, //mt_future59,
        0, //mt_future60,
        0, //mt_future61,
        0, //mt_future62,
        0, //mt_future63,
        0, //mt_future64
    },
    lmt,
    tattr_desc,
    sizeof(PRIVATE_DATA),
    0,  // acl
    s_user_trace_level,
    0, // cmds
    0, // gcflag
};

/***************************************************************************
 *              Public access
 ***************************************************************************/
PUBLIC GCLASS *gclass_gserverlink(void)
{
    return &_gclass;
}
