/***********************************************************************
 *          C_GSS_UDP_S0.C
 *          GssUdpS0 GClass.
 *
 *          Gossamer UDP Server level 0
 *
             Api Gossamer
            ------------

            Input Events:
            - EV_SEND_MESSAGE

            Output Events:
            - EV_ON_OPEN
            - EV_ON_CLOSE
            - EV_ON_MESSAGE

*          Copyright (c) 2014 Niyamaka.
 *          All Rights Reserved.
***********************************************************************/
#include <string.h>
#include "c_gss_udp_s0.h"

/***************************************************************************
 *              Constants
 ***************************************************************************/

/***************************************************************************
 *              Structures
 ***************************************************************************/
typedef struct _UDP_CHANNEL {
    DL_ITEM_FIELDS

    const char *name;
    time_t t_inactivity;
    GBUFFER *gbuf;
} UDP_CHANNEL;

/***************************************************************************
 *              Prototypes
 ***************************************************************************/
PRIVATE UDP_CHANNEL *find_udp_channel(hgobj gobj, const char *name);
PRIVATE UDP_CHANNEL *new_udp_channel(hgobj gobj, const char *name);
PRIVATE void del_udp_channel(hgobj gobj, UDP_CHANNEL *ch);
PRIVATE void free_channels(hgobj gobj);


/***************************************************************************
 *          Data: config, public data, private data
 ***************************************************************************/

/*---------------------------------------------*
 *      Attributes - order affect to oid's
 *---------------------------------------------*/
PRIVATE sdata_desc_t tattr_desc[] = {
SDATA (ASN_OCTET_STR,   "url",                  SDF_RD, 0, "url of udp server"),
SDATA (ASN_COUNTER,     "txMsgs",               SDF_RD|SDF_RSTATS, 0, "Messages transmitted"),
SDATA (ASN_COUNTER,     "rxMsgs",               SDF_RD|SDF_RSTATS, 0, "Messages received"),
SDATA (ASN_INTEGER,     "timeout_base",         SDF_RD,  5*1000, "timeout base"),
SDATA (ASN_INTEGER,     "seconds_inactivity",   SDF_RD,  5*60, "Seconds to consider a gossamer close"),
SDATA (ASN_BOOLEAN,     "disable_end_of_frame", SDF_RD|SDF_STATS, 0, "Disable null as end of frame"),
SDATA (ASN_OCTET_STR,   "on_open_event_name",   SDF_RD,  "EV_ON_OPEN", "Must be empty if you don't want receive this event"),
SDATA (ASN_OCTET_STR,   "on_close_event_name",  SDF_RD,  "EV_ON_CLOSE", "Must be empty if you don't want receive this event"),
SDATA (ASN_OCTET_STR,   "on_message_event_name",SDF_RD,  "EV_ON_MESSAGE", "Must be empty if you don't want receive this event"),
SDATA (ASN_POINTER,     "user_data",            0,  0, "user data"),
SDATA (ASN_POINTER,     "user_data2",           0,  0, "more user data"),
SDATA (ASN_POINTER,     "subscriber",           0,  0, "subscriber of output-events. Default if null is parent."),
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
    // Conf
    const char *on_open_event_name;
    const char *on_close_event_name;
    const char *on_message_event_name;
    int32_t timeout_base;
    int32_t seconds_inactivity;

    // Data oid
    uint32_t *ptxMsgs;
    uint32_t *prxMsgs;
    BOOL disable_end_of_frame;

    hgobj gobj_udp_s;
    hgobj timer;
    dl_list_t dl_channel;
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

    priv->timer = gobj_create("", GCLASS_TIMER, 0, gobj);

    /*
     *  Do copy of heavy used parameters, for quick access.
     *  HACK The writable attributes must be repeated in mt_writing method.
     */
    SET_PRIV(timeout_base,          gobj_read_int32_attr)
    SET_PRIV(seconds_inactivity,    gobj_read_int32_attr)
    SET_PRIV(on_open_event_name,    gobj_read_str_attr)
    SET_PRIV(on_close_event_name,   gobj_read_str_attr)
    SET_PRIV(on_message_event_name, gobj_read_str_attr)
    SET_PRIV(disable_end_of_frame,  gobj_read_bool_attr)

    json_t *kw_udps = json_pack("{s:s}",
        "url", gobj_read_str_attr(gobj, "url")
    );
    priv->gobj_udp_s = gobj_create("", GCLASS_UDP_S0, kw_udps, gobj);

    dl_init(&priv->dl_channel);

    priv->ptxMsgs = gobj_danger_attr_ptr(gobj, "txMsgs");
    priv->prxMsgs = gobj_danger_attr_ptr(gobj, "rxMsgs");

    hgobj subscriber = (hgobj)gobj_read_pointer_attr(gobj, "subscriber");
    if(!subscriber)
        subscriber = gobj_parent(gobj);
    gobj_subscribe_event(gobj, NULL, NULL, subscriber);
}

/***************************************************************************
 *      Framework Method writing
 ***************************************************************************/
PRIVATE void mt_writing(hgobj gobj, const char *path)
{
}

/***************************************************************************
 *      Framework Method start
 ***************************************************************************/
PRIVATE int mt_start(hgobj gobj)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    gobj_start(priv->timer);
    set_timeout_periodic(priv->timer, priv->timeout_base);
    gobj_start(priv->gobj_udp_s);
    return 0;
}

/***************************************************************************
 *      Framework Method stop
 ***************************************************************************/
PRIVATE int mt_stop(hgobj gobj)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    clear_timeout(priv->timer);
    gobj_stop(priv->timer);
    gobj_stop(priv->gobj_udp_s);
    free_channels(gobj);
    return 0;
}

/***************************************************************************
 *      Framework Method destroy
 ***************************************************************************/
PRIVATE void mt_destroy(hgobj gobj)
{
}




            /***************************
             *      Local Methods
             ***************************/




/***************************************************************************
 *
 ***************************************************************************/
PRIVATE UDP_CHANNEL *new_udp_channel(hgobj gobj, const char *name)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);
    UDP_CHANNEL *ch;

    ch = gbmem_malloc(sizeof(UDP_CHANNEL));
    if(!ch) {
        log_error(0,
            "gobj",                 "%s", gobj_full_name(gobj),
            "function",             "%s", __FUNCTION__,
            "msgset",               "%s", MSGSET_MEMORY_ERROR,
            "msg",                  "%s", "no memory",
            "sizeof(UDP_CHANNEL)",  "%d", sizeof(UDP_CHANNEL),
            NULL
        );
        return 0;
    }
    GBMEM_STR_DUP(ch->name, name);
    dl_add(&priv->dl_channel, ch);

    return ch;
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE void free_channels(hgobj gobj)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);
    UDP_CHANNEL *ch; ;
    while((ch=dl_first(&priv->dl_channel))) {
        del_udp_channel(gobj, ch);
    }
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE void del_udp_channel(hgobj gobj, UDP_CHANNEL *ch)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    dl_delete(&priv->dl_channel, ch, 0);
    GBUF_DECREF(ch->gbuf);
    GBMEM_FREE(ch->name);
    GBMEM_FREE(ch);
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE UDP_CHANNEL *find_udp_channel(hgobj gobj, const char *name)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);
    UDP_CHANNEL *ch;

    if(!name) {
        return 0;
    }
    ch = dl_first(&priv->dl_channel);
    while(ch) {
        if(strcmp(ch->name, name)==0) {
            return ch;
        }
        ch = dl_next(ch);
    }
    return 0;
}




            /***************************
             *      Actions
             ***************************/




/***************************************************************************
 *
 ***************************************************************************/
PRIVATE int ac_rx_data(hgobj gobj, const char *event, json_t *kw, hgobj src)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);
    GBUFFER *gbuf = (GBUFFER *)(size_t)kw_get_int(kw, "gbuffer", 0, FALSE);
    const char *udp_channel = gbuf_getlabel(gbuf);

    UDP_CHANNEL *ch = find_udp_channel(gobj, udp_channel);
    if(!ch) {
        ch = new_udp_channel(gobj, udp_channel);

        if(!empty_string(priv->on_open_event_name)) {
            gobj_publish_event(gobj, priv->on_open_event_name, 0);
        }
    }
    ch->t_inactivity = start_sectimer(priv->seconds_inactivity);

    if(priv->disable_end_of_frame) {
        return gobj_publish_event(gobj, priv->on_message_event_name, kw);
    }

    char *p;
    while((p=gbuf_get(gbuf, 1))) {
        if(!p) {
            break; // no more data in gbuf
        }
        /*
         *  Use a temporal gbuf, to save data until arrive of null.
         */
        if(!ch->gbuf) {
            size_t size = MIN(1*1024L*1024L, gbmem_get_maximum_block());
            ch->gbuf = gbuf_create(size, size, 0, 0);
            if(!ch->gbuf) {
                log_error(0,
                    "gobj",             "%s", gobj_full_name(gobj),
                    "function",         "%s", __FUNCTION__,
                    "msgset",           "%s", MSGSET_MEMORY_ERROR,
                    "msg",              "%s", "no memory",
                    "size",             "%d", size,
                    NULL
                );
                break;
            }
        }
        if(*p==0) {
            /*
             *  End of frame
             */
            json_t *kw_ev = json_pack("{s:I}",
                "gbuffer", (json_int_t)(size_t)ch->gbuf
            );
            ch->gbuf = 0;
            gobj_publish_event(gobj, priv->on_message_event_name, kw_ev);
        } else {
            if(gbuf_append(ch->gbuf, p, 1)!=1) {
                log_error(0,
                    "gobj",             "%s", gobj_full_name(gobj),
                    "function",         "%s", __FUNCTION__,
                    "msgset",           "%s", MSGSET_MEMORY_ERROR,
                    "msg",              "%s", "gbuf_append() FAILED",
                    NULL
                );
                /*
                 *  No space, process whatever received
                 */
                json_t *kw_ev = json_pack("{s:I}",
                    "gbuffer", (json_int_t)(size_t)ch->gbuf
                );
                ch->gbuf = 0;
                gobj_publish_event(gobj, priv->on_message_event_name, kw_ev);
                //break;
            }
        }
    }

    KW_DECREF(kw);
    return 0;
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE int ac_send_message(hgobj gobj, const char *event, json_t *kw, hgobj src)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);
    GBUFFER *gbuf = (GBUFFER *)(size_t)kw_get_int(kw, "gbuffer", 0, FALSE);
    const char *udp_channel = gbuf_getlabel(gbuf);

    UDP_CHANNEL *ch = find_udp_channel(gobj, udp_channel);
    if(ch) {
    } else {
        log_error(0,
            "gobj",                 "%s", gobj_full_name(gobj),
            "function",             "%s", __FUNCTION__,
            "msgset",               "%s", MSGSET_PARAMETER_ERROR,
            "msg",                  "%s", "UDP channel NOT FOUND",
            "udp_channel",          "%s", udp_channel?udp_channel:"",
            NULL
        );
    }
    return gobj_send_event(priv->gobj_udp_s, "EV_TX_DATA", kw, gobj);
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE int ac_transmit_ready(hgobj gobj, const char *event, json_t *kw, hgobj src)
{
    KW_DECREF(kw);
    return 0;
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE int ac_timeout(hgobj gobj, const char *event, json_t *kw, hgobj src)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);
    UDP_CHANNEL *ch, *nx;

    ch = dl_first(&priv->dl_channel);
    while(ch) {
        nx = dl_next(ch);
        if(test_sectimer(ch->t_inactivity)) {
            if(!empty_string(priv->on_close_event_name)) {
                gobj_publish_event(gobj, priv->on_close_event_name, 0);
            }
            del_udp_channel(gobj, ch);
        }
        ch = nx;
    }

    KW_DECREF(kw);
    return 0;
}

/***************************************************************************
 *                          FSM
 ***************************************************************************/
PRIVATE const EVENT input_events[] = {
    {"EV_RX_DATA",          0},
    {"EV_SEND_MESSAGE",     0},
    {"EV_TX_READY",         0},
    {"EV_TIMEOUT",          0},
    {"EV_STOPPED",          0},
    {NULL, 0}
};
PRIVATE const EVENT output_events[] = {
    {"EV_ON_OPEN",          0},
    {"EV_ON_CLOSE",         0},
    {"EV_ON_MESSAGE",       0},
    {NULL, 0}
};
PRIVATE const char *state_names[] = {
    "ST_IDLE",
    NULL
};

PRIVATE EV_ACTION ST_IDLE[] = {
    {"EV_RX_DATA",          ac_rx_data,         0},
    {"EV_SEND_MESSAGE",     ac_send_message,    0},
    {"EV_TX_READY",         ac_transmit_ready,  0},
    {"EV_TIMEOUT",          ac_timeout,         0},
    {"EV_STOPPED",          0,                  0},
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
    GCLASS_GSS_UDP_S0_NAME,     // CHANGE WITH each gclass
    &fsm,
    {
        mt_create,
        0, //mt_create2,
        mt_destroy,
        mt_start,
        mt_stop,
        0, //mt_play,
        0, //mt_pause,
        mt_writing,
        0, //mt_reading,
        0, //mt_subscription_added,
        0, //mt_subscription_deleted,
        0, //mt_child_added,
        0, //mt_child_removed,
        0, //mt_stats,
        0, //mt_command,
        0, //mt_inject_event,
        0, //mt_create_resource,
        0, //mt_list_resource,
        0, //mt_update_resource,
        0, //mt_delete_resource,
        0, //mt_add_child_resource_link
        0, //mt_delete_child_resource_link
        0, //mt_get_resource
        0, //mt_authorization_parser,
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
        0, //mt_link_nodes2,
        0, //mt_unlink_nodes,
        0, //mt_unlink_nodes2,
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
        0, //mt_node_instances,
        0, //mt_save_node,
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
    0, // cmds
    0, // gcflag
};

/***************************************************************************
 *              Public access
 ***************************************************************************/
PUBLIC GCLASS *gclass_gss_udp_s0(void)
{
    return &_gclass;
}
