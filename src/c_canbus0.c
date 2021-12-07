/***********************************************************************
 *          C_CANBUS0.C
 *          Canbus0 GClass.
 *
 *          Canbus (socketcan CAN_RAW) uv-mixin for Yuneta
 *
 *          Copyright (c) 2021 Niyamaka.
 *          All Rights Reserved.
 ***********************************************************************/
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <net/if.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include "c_canbus0.h"

/***************************************************************************
 *              Constants
 ***************************************************************************/

/***************************************************************************
 *              Structures
 ***************************************************************************/

/***************************************************************************
 *              Prototypes
 ***************************************************************************/
PRIVATE void on_poll_cb(uv_poll_t *req, int status, int events);
PRIVATE void on_close_cb(uv_handle_t* handle);

/***************************************************************************
 *          Data: config, public data, private data
 ***************************************************************************/

/*---------------------------------------------*
 *      Attributes - order affect to oid's
 *---------------------------------------------*/
PRIVATE sdata_desc_t tattr_desc[] = {
/*-ATTR-type------------name----------------flag------------default-----description---------- */
SDATA (ASN_OCTET_STR,   "device",           SDF_RD,         "",         "interface device, ex: can0"),
SDATA (ASN_COUNTER64,   "txBytes",          SDF_RD,         0,          "Bytes transmitted by this socket"),
SDATA (ASN_COUNTER64,   "rxBytes",          SDF_RD,         0,          "Bytes received by this socket"),
SDATA (ASN_BOOLEAN,     "exitOnError",      SDF_RD,         1,          "Exit if Listen failed"),
SDATA (ASN_INTEGER,     "timeout_response", SDF_WR,         0,          "TODO Timeout response"),
SDATA (ASN_POINTER,     "user_data",        0,              0,          "user data"),
SDATA (ASN_POINTER,     "user_data2",       0,              0,          "more user data"),
SDATA (ASN_POINTER,     "subscriber",       0,              0,          "subscriber of output-events. Not a child gobj."),
SDATA_END()
};

/*---------------------------------------------*
 *      GClass trace levels
 *  HACK strict ascendent value!
 *  required paired correlative strings
 *  in s_user_trace_level
 *---------------------------------------------*/
enum {
    TRACE_TRAFFIC   = 0x0001,
};
PRIVATE const trace_level_t s_user_trace_level[16] = {
{"traffic",         "Trace dump traffic"},
{0, 0},
};

/*---------------------------------------------*
 *              Private data
 *---------------------------------------------*/
#define BFINPUT_SIZE (2*1024)

typedef struct _PRIVATE_DATA {
    int32_t timeout_response;
    hgobj timer;

    // Conf
    BOOL exitOnError;

    // Data oid
    uint64_t *ptxBytes;
    uint64_t *prxBytes;

    uv_poll_t uv_poll;
    int m_socket;
    BOOL inform_disconnection;

    struct can_frame m_recvBuffer;
    struct can_frame m_sendBuffer;

    char bfinput[BFINPUT_SIZE];
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

    priv->timer = gobj_create(gobj_name(gobj), GCLASS_TIMER, 0, gobj);

    /*
     *  SERVICE subscription model
     */
    hgobj subscriber = (hgobj)gobj_read_pointer_attr(gobj, "subscriber");
    if(subscriber) {
        gobj_subscribe_event(gobj, NULL, NULL, subscriber);
    }

    /*
     *  Do copy of heavy used parameters, for quick access.
     *  HACK The writable attributes must be repeated in mt_writing method.
     */
    SET_PRIV(exitOnError,               gobj_read_bool_attr)
    SET_PRIV(timeout_response,            gobj_read_int32_attr)

    priv->ptxBytes = gobj_danger_attr_ptr(gobj, "txBytes");
    priv->prxBytes = gobj_danger_attr_ptr(gobj, "rxBytes");
}

/***************************************************************************
 *      Framework Method writing
 ***************************************************************************/
PRIVATE void mt_writing(hgobj gobj, const char *path)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    IF_EQ_SET_PRIV(timeout_response,              gobj_read_int32_attr)
    END_EQ_SET_PRIV()
}

/***************************************************************************
 *      Framework Method destroy
 ***************************************************************************/
PRIVATE void mt_destroy(hgobj gobj)
{
    if(!gobj_in_this_state(gobj, "ST_STOPPED")) {
        log_error(0,
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

    uv_loop_t *loop = yuno_uv_event_loop();

    priv->m_socket = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if(priv->m_socket < 0) {
        log_error(0,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_CONNECT_DISCONNECT,
            "msg",          "%s", "socket() FAILED",
            "error",        "%d", errno,
            "strerror",     "%s", strerror(errno),
            NULL
        );
        if(priv->exitOnError) {
            exit(0); // WARNING exit with 0 to stop daemon watcher!
        } else {
            return -1;
        }
    }
    int flags = fcntl(priv->m_socket, F_GETFL, 0);
    fcntl(priv->m_socket, F_SETFL, flags | O_NONBLOCK);

    struct sockaddr_can addr;
    struct ifreq ifr;

    const char *device = gobj_read_str_attr(gobj, "device");
    if(empty_string(device)) {
        log_error(0,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_PARAMETER_ERROR,
            "msg",          "%s", "What canbus device?",
            "device",       "%s", device,
            NULL
        );
        close(priv->m_socket);
        priv->m_socket = -1;
        if(priv->exitOnError) {
            exit(0); // WARNING exit with 0 to stop daemon watcher!
        } else {
            return -1;
        }
    }

    snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), "%s", device);
    ioctl(priv->m_socket, SIOCGIFINDEX, &ifr); /* ifr.ifr_ifindex gets filled */
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    if(bind(priv->m_socket, (struct sockaddr*)&addr, sizeof(addr))<0) {
        close(priv->m_socket);
        priv->m_socket = -1;
        if(priv->exitOnError) {
            exit(0); // WARNING exit with 0 to stop daemon watcher!
        } else {
            return -1;
        }
    }

    if(gobj_trace_level(gobj) & TRACE_UV) {
        log_debug_printf(0, ">>> uv_init canbus0 p=%p", &priv->m_socket);
    }
    uv_poll_init(loop, &priv->uv_poll, priv->m_socket);
    priv->uv_poll.data = gobj;

    if(gobj_trace_level(gobj) & TRACE_UV) {
        log_debug_printf(0, ">>> start_read canbus0 p=%p", &priv->uv_poll);
    }
    uv_poll_start(&priv->uv_poll, UV_READABLE, on_poll_cb);

    gobj_change_state(gobj, "ST_IDLE");

    gobj_start(priv->timer);

    gobj_publish_event(gobj, "EV_CONNECTED", 0);
    priv->inform_disconnection = TRUE;

    return 0;
}

/***************************************************************************
 *      Framework Method stop
 ***************************************************************************/
PRIVATE int mt_stop(hgobj gobj)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    if(priv->m_socket != -1) {
        if(gobj_trace_level(gobj) & TRACE_UV) {
            log_debug_printf(0, ">>> uv_poll_stop p=%p", &priv->uv_poll);
        }
        uv_poll_stop(&priv->uv_poll);
        uv_close((uv_handle_t*)&priv->uv_poll, on_close_cb);
        priv->m_socket = -1;
        gobj_change_state(gobj, "ST_WAIT_STOPPED");
    }

    clear_timeout(priv->timer);
    gobj_stop(priv->timer);

    return 0;
}




            /***************************
             *      Commands
             ***************************/




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
        log_debug_printf(0, "<<< on_close_cb canbus0 p=%p",
            &priv->uv_poll
        );
    }
//     priv->uv_handler_active = 0; TODO
//     priv->uv_req_connect_active = 0;
    gobj_change_state(gobj, "ST_STOPPED");

    if(priv->inform_disconnection) {
        priv->inform_disconnection = FALSE;
        gobj_publish_event(gobj, "EV_DISCONNECTED", 0);
    }

    if(gobj_is_volatil(gobj)) {
        gobj_destroy(gobj);
    } else {
        gobj_publish_event(gobj, "EV_STOPPED", 0);
    }
}

/***************************************************************************
 *  on poll callback
 ***************************************************************************/
PRIVATE void on_poll_cb(uv_poll_t *req, int status, int events)
{
    hgobj gobj = req->data;
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    if(gobj_trace_level(gobj) & TRACE_UV) {
        log_debug_printf(0, "<<<< on_poll_cb status %d, events %d, fd %d",
            status, events, priv->m_socket
        );
    }

    if(status < 0) {
        log_error(0,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_LIBUV_ERROR,
            "msg",          "%s", "poll FAILED",
            "uv_error",     "%s", uv_err_name(status),
            NULL
        );
        gobj_stop(gobj);
        return;
    }

    if(events & UV_READABLE) {
        int ret = read(priv->m_socket, &priv->m_recvBuffer, sizeof(priv->m_recvBuffer));
        if(ret < 0) {
            log_error(0,
                "gobj",         "%s", gobj_full_name(gobj),
                "function",     "%s", __FUNCTION__,
                "msgset",       "%s", MSGSET_SYSTEM_ERROR,
                "msg",          "%s", "read FAILED",
                "error",        "%d", errno,
                "strerror",     "%s", strerror(errno),
                NULL
            );
            gobj_stop(gobj);
            return;
        }
    }

    if(events & UV_WRITABLE) {
        // No more data to send, put off UV_WRITABLE
        uv_poll_start(&priv->uv_poll, UV_READABLE, on_poll_cb);
        gobj_publish_event(gobj, "EV_TX_READY", 0);
    }
}




            /***************************
             *      Actions
             ***************************/




/***************************************************************************
 *  udp_channel is "ip:port" and it's in the label of gbuff.
 ***************************************************************************/
PRIVATE int ac_tx_data(hgobj gobj, const char *event, json_t *kw, hgobj src)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    GBUFFER *gbuf = (GBUFFER *)(size_t)kw_get_int(kw, "gbuffer", 0, FALSE);
    char *p = gbuf_cur_rd_pointer(gbuf);

    /* Fill the CANopen data */
    struct can_frame frame; // TODO
//     priv->m_sendBuffer.can_id = ((0x100*pdoid)+0x200) | con->node_id;
    memcpy(priv->m_sendBuffer.data, p, 1); // TODO
//     priv->m_sendBuffer.can_dlc = jslen;

    if(write(priv->m_socket, &frame, sizeof(struct can_frame)) < 0) {
        log_error(0,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_SYSTEM_ERROR,
            "msg",          "%s", "read FAILED",
            "error",        "%d", errno,
            "strerror",     "%s", strerror(errno),
            NULL
        );
        gobj_stop(gobj);
        KW_DECREF(kw);
        return -1;
    }

    // TODO    send_data(gobj, gbuf);
    uv_poll_start(&priv->uv_poll, UV_READABLE|UV_WRITABLE, on_poll_cb);
    if (priv->timeout_response > 0) {
        set_timeout(priv->timer, priv->timeout_response);
    }

    KW_DECREF(kw);
    return 1;
}

/***************************************************************************
 *                          FSM
 ***************************************************************************/
PRIVATE const EVENT input_events[] = {
    {"EV_TX_DATA",      0},
    {NULL, 0}
};
PRIVATE const EVENT output_events[] = {
    {"EV_CONNECTED",        0},
    {"EV_RX_DATA",          0},
    {"EV_TX_READY",         0},
    {"EV_DISCONNECTED",     0},
    {"EV_STOPPED",          0},
    {NULL, 0}
};
PRIVATE const char *state_names[] = {
    "ST_STOPPED",
    "ST_WAIT_STOPPED",
    "ST_IDLE",          /* H2UV handler for UV */
    NULL
};

PRIVATE EV_ACTION ST_STOPPED[] = {
    {0,0,0}
};

PRIVATE EV_ACTION ST_WAIT_STOPPED[] = {
    {0,0,0}
};

PRIVATE EV_ACTION ST_IDLE[] = {
    {"EV_TX_DATA",        ac_tx_data,       0},
    {0,0,0}
};

PRIVATE EV_ACTION *states[] = {
    ST_STOPPED,
    ST_WAIT_STOPPED,
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
    GCLASS_CANBUS0_NAME,
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
        0, //mt_command_parser,
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
    0, //authz_table,
    s_user_trace_level,
    0, //command_table,  // command_table
    0,  // gcflag
};

/***************************************************************************
 *              Public access
 ***************************************************************************/
PUBLIC GCLASS *gclass_canbus0(void)
{
    return &_gclass;
}
