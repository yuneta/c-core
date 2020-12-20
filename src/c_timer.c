/***********************************************************************
 *          C_TIMER.C
 *          GClass of TIMER uv-mixin
 *
 *  Este gobj usa el 2 nivel de operación (play/pause)
 *  para arrancar/parar el timer.
 *  Usar las funciones set_timeout* y clear_timeout que enmascaran el play/pause
 *  Las funciones start/stop se usan para activar o desactivar el uv handler
 *
 *          Copyright (c) 2013-2014 Niyamaka.
 *          All Rights Reserved.
 ***********************************************************************/
#include <uv.h>
#include "c_timer.h"

/***************************************************************************
 *              Constants
 ***************************************************************************/

/***************************************************************************
 *              Prototypes
 ***************************************************************************/
PRIVATE void on_close_cb(uv_handle_t* handle);
PRIVATE void on_timer_cb(uv_timer_t* handle);

/***************************************************************************
 *          Data: config, public data, private data
 ***************************************************************************/

/*---------------------------------------------*
 *      Attributes - order affect to oid's
 *---------------------------------------------*/
PRIVATE sdata_desc_t tattr_desc[] = {
SDATA (ASN_INTEGER,     "msec",                 SDF_RD,  0, "Timeout in miliseconds"),
SDATA (ASN_BOOLEAN,     "periodic",             SDF_RD,  0, "True for periodic timeouts"),
SDATA (ASN_POINTER,     "user_data",            0,  0, "user data"),
SDATA (ASN_POINTER,     "user_data2",           0,  0, "more user data"),
SDATA (ASN_OCTET_STR,   "timeout_event_name",   SDF_RD,  "EV_TIMEOUT", "Timeout event name"),
SDATA (ASN_OCTET_STR,   "stopped_event_name",   SDF_RD,  "EV_STOPPED", "Stopped event name"),
SDATA_END()
};

/*---------------------------------------------*
 *      GClass trace levels
 *---------------------------------------------*/
enum {
    TRACE_XXX         = 0x0001,
};
PRIVATE const trace_level_t s_user_trace_level[16] = {
//{"traffic",             "Trace dump traffic"},
{0, 0},
};

/*---------------------------------------------*
 *              Private data
 *---------------------------------------------*/
typedef struct _PRIVATE_DATA {
    uv_timer_t uv_timer;
} PRIVATE_DATA;




            /***************************
             *      Framework Methods
             ***************************/




/***************************************************************************
 *      Framework Method create
 ***************************************************************************/
PRIVATE void mt_create(hgobj gobj)
{
    /*
     *  Do copy of heavy used parameters, for quick access.
     *  HACK The writable attributes must be repeated in mt_writing method.
     */
}

/***************************************************************************
 *      Framework Method destroy
 ***************************************************************************/
PRIVATE void mt_destroy(hgobj gobj)
{
    if(!gobj_in_this_state(gobj, "ST_STOPPED")) {
        log_error(LOG_OPT_TRACE_STACK,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_LIBUV_ERROR,
            "msg",          "%s", "GObj NOT STOPPED. UV handler ACTIVE!",
            NULL
        );
    }
}

/***************************************************************************
 *      Framework Method start
 ***************************************************************************/
PRIVATE int mt_start(hgobj gobj)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    if(gobj_trace_level(gobj) & TRACE_UV) {
        log_debug_printf(0, ">>> uv_init timer p=%p", &priv->uv_timer);
    }
    uv_timer_init(yuno_uv_event_loop(), &priv->uv_timer);
    priv->uv_timer.data = gobj;
    gobj_change_state(gobj, "ST_IDLE");

    return 0;
}

/***************************************************************************
 *      Framework Method stop
 ***************************************************************************/
PRIVATE int mt_stop(hgobj gobj)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    if(gobj_trace_level(gobj) & TRACE_UV) {
        log_debug_printf(0,  ">>> uv_close timer p=%p", &priv->uv_timer);
    }
    gobj_change_state(gobj, "ST_WAIT_STOPPED");
    uv_close((uv_handle_t *)&priv->uv_timer, on_close_cb);

    return 0;
}

/***************************************************************************
 *      Framework Method play
 ***************************************************************************/
PRIVATE int mt_play(hgobj gobj)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    int msec = gobj_read_int32_attr(gobj, "msec");
    BOOL periodic = gobj_read_bool_attr(gobj, "periodic");

    gobj_change_state(gobj, "ST_COUNTDOWN");

    if(gobj_trace_level(gobj) & TRACE_UV) {
        log_debug_printf(0,  ">>> uv_time_start %d p=%p", msec, &priv->uv_timer);
    }
    uv_timer_start(
        &priv->uv_timer,
        on_timer_cb,
        (uint64_t)msec,
        periodic?(uint64_t)msec:0
    );

    return 0;
}

/***************************************************************************
 *      Framework Method pause
 ***************************************************************************/
PRIVATE int mt_pause(hgobj gobj)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    gobj_change_state(gobj, "ST_IDLE");

    uv_timer_stop(&priv->uv_timer);

    return 0;
}




            /***************************
             *      Local Methods
             ***************************/




/***************************************************************************
 *
 ***************************************************************************/
PRIVATE void on_close_cb(uv_handle_t* handle)
{
    hgobj gobj = handle->data;
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    if(gobj_trace_level(gobj) & TRACE_UV) {
        log_debug_printf(0,  "<<< on_close_cb timer p=%p", &priv->uv_timer);
    }
    gobj_change_state(gobj, "ST_STOPPED");

    /*
     *  Only NOW you can destroy this gobj,
     *  when uv has released the handler.
     */
    const char *stopped_event_name = gobj_read_str_attr(
        gobj,
        "stopped_event_name"
    );
    if(!empty_string(stopped_event_name)) {
        gobj_send_event(
            gobj_parent(gobj),
            stopped_event_name ,
            0,
            gobj
        );
    }
}

/***************************************************************************
 *  timer callback
 ***************************************************************************/
PRIVATE void on_timer_cb(uv_timer_t* handle)
{
    hgobj gobj = handle->data;
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    BOOL periodic = gobj_read_bool_attr(gobj, "periodic");
    if(!periodic) {
        if(gobj_trace_level(gobj) & TRACE_UV) {
            log_debug_printf(0,  "<<< on_timer_cb p=%p", &priv->uv_timer);
        }
        gobj_pause(gobj);
    }
    gobj_send_event(
        gobj_parent(gobj),
        gobj_read_str_attr(gobj, "timeout_event_name"),
        0,
        gobj
    );
}




            /***************************
             *      Actions
             ***************************/




/***************************************************************************
 *                          FSM
 ***************************************************************************/
PRIVATE const EVENT input_events[] = {
    {NULL,          0, 0, 0}
};
PRIVATE const EVENT output_events[] = {
    {"EV_TIMEOUT",  0, 0, 0},
    {"EV_STOPPED",  0, 0, 0},
    {NULL, 0}
};
PRIVATE const char *state_names[] = {
    "ST_STOPPED",
    "ST_WAIT_STOPPED",
    "ST_IDLE",          /* H2UV handler for UV */
    "ST_COUNTDOWN",
    NULL
};

PRIVATE EV_ACTION ST_STOPPED[] = {
    {0,0,0}
};
PRIVATE EV_ACTION ST_WAIT_STOPPED[] = {
    {0,0,0}
};
PRIVATE EV_ACTION ST_IDLE[] = {
    {0,0,0}
};
PRIVATE EV_ACTION ST_COUNTDOWN[] = {
    {0,0,0}
};

PRIVATE EV_ACTION *states[] = {
    ST_STOPPED,
    ST_WAIT_STOPPED,
    ST_IDLE,
    ST_COUNTDOWN,
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
    GCLASS_TIMER_NAME,
    &fsm,
    {
        mt_create,
        0, //mt_create2,
        mt_destroy,
        mt_start,
        mt_stop,
        mt_play,
        mt_pause,
        0, //mt_writing,
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
        0, //mt_future38,
        0, //mt_authzs,
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
    gcflag_manual_start, // gcflag
};

/***************************************************************************
 *              Public access
 ***************************************************************************/
PUBLIC GCLASS *gclass_timer(void)
{
    return &_gclass;
}




            /***************************
             *      Helpers
             ***************************/




/***************************************************************************
 *  Set timeout
 ***************************************************************************/
PUBLIC void set_timeout(hgobj gobj, int msec)
{
    clear_timeout(gobj);
    gobj_write_int32_attr(gobj, "msec", msec);
    gobj_write_bool_attr(gobj, "periodic", FALSE);
    gobj_play(gobj);
}

/***************************************************************************
 *  Set periodic timeout
 ***************************************************************************/
PUBLIC void set_timeout_periodic(hgobj gobj, int msec)
{
    clear_timeout(gobj);
    gobj_write_int32_attr(gobj, "msec", msec);
    gobj_write_bool_attr(gobj, "periodic", TRUE);
    gobj_play(gobj);
}

/***************************************************************************
 *  Clear timeout
 ***************************************************************************/
PUBLIC void clear_timeout(hgobj gobj)
{
    if(gobj_is_playing(gobj))
        gobj_pause(gobj);
}
