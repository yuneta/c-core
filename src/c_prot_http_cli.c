/***********************************************************************
 *          C_PROT_HTTP_CLI.C
 *          Prot_http_cli GClass.
 *
 *          Protocol http as client
 *
 *          Copyright (c) 2017-2021 Niyamaka.
 *          All Rights Reserved.
 ***********************************************************************/
#include <string.h>
#include <inttypes.h>
#include "c_prot_http_cli.h"

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
/*-ATTR-type------------name----------------flag------------default---------description---------- */
SDATA (ASN_OCTET_STR,   "url",              SDF_RD,         "",             "url to connect"),
SDATA (ASN_INTEGER,     "timeout_inactivity",SDF_WR,        1*60*1000,      "Timeout inactivity"),
SDATA (ASN_BOOLEAN,     "connected",         SDF_RD,        0,              "Connection state. Important filter!"),
SDATA (ASN_OCTET_STR,   "on_open_event_name",SDF_RD,        "EV_ON_OPEN",   "Must be empty if you don't want receive this event"),
SDATA (ASN_OCTET_STR,   "on_close_event_name",SDF_RD,       "EV_ON_CLOSE",  "Must be empty if you don't want receive this event"),
SDATA (ASN_OCTET_STR,   "on_message_event_name",SDF_RD,     "EV_ON_MESSAGE","Must be empty if you don't want receive this event"),
SDATA (ASN_POINTER,     "user_data",        0,              0,              "user data"),
SDATA (ASN_POINTER,     "user_data2",       0,              0,              "more user data"),
SDATA (ASN_POINTER,     "subscriber",       0,              0,              "subscriber of output-events. If it's null then subscriber is the parent."),
SDATA_END()
};

/*---------------------------------------------*
 *      GClass trace levels
 *---------------------------------------------*/
enum {
    TRAFFIC         = 0x0001,
};
PRIVATE const trace_level_t s_user_trace_level[16] = {
{"traffic",         "Trace traffic"},
{0, 0},
};


/*---------------------------------------------*
 *              Private data
 *---------------------------------------------*/
typedef struct _PRIVATE_DATA {
    hgobj timer;
    GHTTP_PARSER *parsing_response;     // A response parser instance

    int timeout_inactivity;
    const char *url;
    const char *on_open_event_name;
    const char *on_close_event_name;
    const char *on_message_event_name;
    char schema[64];
    char host[NAME_MAX];
    int port;

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
    priv->parsing_response = ghttp_parser_create(
        gobj,
        HTTP_RESPONSE,
        "EV_ON_MESSAGE",
        FALSE
    );

    /*
     *  CHILD subscription model
     */
    hgobj subscriber = (hgobj)gobj_read_pointer_attr(gobj, "subscriber");
    if(!subscriber)
        subscriber = gobj_parent(gobj);
    gobj_subscribe_event(gobj, NULL, NULL, subscriber);

    /*
     *  Do copy of heavy used parameters, for quick access.
     *  HACK The writable attributes must be repeated in mt_writing method.
     */
    SET_PRIV(url,                   gobj_read_str_attr)
    SET_PRIV(on_open_event_name,    gobj_read_str_attr)
    SET_PRIV(on_close_event_name,   gobj_read_str_attr)
    SET_PRIV(on_message_event_name, gobj_read_str_attr)
    SET_PRIV(timeout_inactivity,    gobj_read_int32_attr)

    if(empty_string(priv->url)) {
        log_error(0,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_INTERNAL_ERROR,
            "msg",          "%s", "url EMPTY",
            NULL
        );
    } else {
        char port[64];
        parse_http_url(
            priv->url,
            priv->schema, sizeof(priv->schema),
            priv->host, sizeof(priv->host),
            port, sizeof(port),
            FALSE
        );
        priv->port = atoi(port);
    }

    if(empty_string(priv->host)) {
        log_error(0,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_INTERNAL_ERROR,
            "msg",          "%s", "host EMPTY",
            NULL
        );
    }
    if(!priv->port) {
        log_error(0,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_INTERNAL_ERROR,
            "msg",          "%s", "port EMPTY",
            NULL
        );
    }
}

/***************************************************************************
 *      Framework Method writing
 ***************************************************************************/
PRIVATE void mt_writing(hgobj gobj, const char *path)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    IF_EQ_SET_PRIV(timeout_inactivity,          gobj_read_int32_attr)
    ELIF_EQ_SET_PRIV(url,                       gobj_read_str_attr)
        // TODO recalcula host,port
    END_EQ_SET_PRIV()
}

/***************************************************************************
 *      Framework Method start
 ***************************************************************************/
PRIVATE int mt_start(hgobj gobj)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    gobj_start(priv->timer);
    gobj_start_childs(gobj);
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
    gobj_stop_childs(gobj);
    return 0;
}

/***************************************************************************
 *      Framework Method destroy
 ***************************************************************************/
PRIVATE void mt_destroy(hgobj gobj)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    if(priv->parsing_response) {
        ghttp_parser_destroy(priv->parsing_response);
        priv->parsing_response = 0;
    }
}




            /***************************
             *      Local Methods
             ***************************/




/***************************************************************************
 *  Parse a http message
 *  Return -1 if error: you must close the socket.
 *  Return 0 if no new request.
 *  Return 1 if new request available in `request`.
 ***************************************************************************/
PRIVATE int parse_message(hgobj gobj, GBUFFER *gbuf, GHTTP_PARSER *parser)
{
    size_t ln;
    while((ln=gbuf_leftbytes(gbuf))>0) {
        char *bf = gbuf_cur_rd_pointer(gbuf);
        int n = ghttp_parser_received(parser, bf, ln);
        if (n == -1) {
            log_debug_full_gbuf(
                LOG_DUMP_OUTPUT,
                gbuf,
                "ghttp_parser_received() FAILED"
            );
            // Some error in parsing
            ghttp_parser_reset(parser);
            return -1;
        } else if (n > 0) {
            gbuf_get(gbuf, n);  // take out the bytes consumed
        }
    }
    return 0;
}




            /***************************
             *      Actions
             ***************************/




/***************************************************************************
 *
 ***************************************************************************/
PRIVATE int ac_connected(hgobj gobj, const char *event, json_t *kw, hgobj src)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    gobj_write_bool_attr(gobj, "connected", TRUE);

    ghttp_parser_reset(priv->parsing_response);

    clear_timeout(priv->timer);

    if(!empty_string(priv->on_open_event_name)) {
        gobj_publish_event(gobj, priv->on_open_event_name, 0);
    }

    KW_DECREF(kw);
    return 0;
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE int ac_disconnected(hgobj gobj, const char *event, json_t *kw, hgobj src)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    clear_timeout(priv->timer);

    gobj_write_bool_attr(gobj, "connected", FALSE);

    if(!empty_string(priv->on_close_event_name)) {
        gobj_publish_event(gobj, priv->on_close_event_name, 0);
    }

    KW_DECREF(kw);
    return 0;
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE int ac_rx_data(hgobj gobj, const char *event, json_t *kw, hgobj src)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    GBUFFER *gbuf = (GBUFFER *)(size_t)kw_get_int(kw, "gbuffer", 0, FALSE);

    if(gobj_trace_level(gobj) & TRAFFIC) {
        log_debug_gbuf(LOG_DUMP_INPUT, gbuf, gobj_short_name(gobj));
    }

    set_timeout(priv->timer, priv->timeout_inactivity);

    int result = parse_message(gobj, gbuf, priv->parsing_response);
    if (result < 0) {
        gobj_send_event(gobj_bottom_gobj(gobj), "EV_DROP", 0, gobj);
    }

    KW_DECREF(kw);
    return 0;
}

/********************************************************************
 *
 ********************************************************************/
PRIVATE int ac_send_message(hgobj gobj, const char *event, json_t *kw, hgobj src)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);
    const char *http_version = "1.1";
    int content_length = 0;
    char *content = 0;

    const char *method = kw_get_str(kw, "method", "GET", 0);
    const char *resource = kw_get_str(kw, "resource", "/", 0); // WARNING here it could come params
    json_t *headers = kw_get_dict(kw, "headers", 0, 0);
    json_t *params = kw_get_dict_value(kw, "params", 0, 0);


    //if(json_is_string(params)) -> copia tal cual
    //if(json_is_object(params)) -> transforma a params

//     for k in form_data:
//         v = form_data[k]
//         if not data:
//             data += """%s=%s""" % (k,v)
//         else:
//             data += """&%s=%s""" % (k,v)

    json_t *data = kw_get_dict(kw, "json", 0, 0);
    if(data) {
        content = json2uglystr(data);
        content_length = strlen(content);
    }

    // TODO calcula bien el size of header
    GBUFFER *gbuf = gbuf_create(1024+content_length, 1024+content_length, 0,0);
    if(!gbuf) {
        log_error(0,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_INTERNAL_ERROR,
            "msg",          "%s", "gbuf_create() FAILED",
            NULL
        );
        return -1;
    }
    gbuf_printf(gbuf, "%s %s HTTP/%s\r\n", method, resource, http_version);

    // TODO merge these own headers with passed headers
    if(priv->port == 80 || priv->port == 443) {
        gbuf_printf(gbuf, "Host: %s\r\n", priv->host);
    } else {
        gbuf_printf(gbuf, "Host: %s:%d\r\n", priv->host, priv->port);
    }
    gbuf_printf(gbuf, "User-Agent: yuneta-%s\r\n",  __yuneta_version__);
    gbuf_printf(gbuf, "Connection: keep-alive\r\n");
    gbuf_printf(gbuf, "Accept: */*\r\n");
    if(content) {
        gbuf_printf(gbuf, "Content-Type: application/json; charset=utf-8\r\n");
        gbuf_printf(gbuf, "Content-Length: %d\r\n", content_length);
    }
    gbuf_printf(gbuf, "\r\n");
    if(content) {
        gbuf_printf(gbuf, content, content_length);
        gbmem_free(content);
    }

    if(gobj_trace_level(gobj) & TRAFFIC) {
        log_debug_gbuf(
            LOG_DUMP_OUTPUT,
            gbuf,
            gobj_short_name(gobj_bottom_gobj(gobj))
        );
    }

    json_t *kw_response = json_pack("{s:I}",
        "gbuffer", (json_int_t)(size_t)gbuf
    );
    KW_DECREF(kw);
    return gobj_send_event(gobj_bottom_gobj(gobj), "EV_TX_DATA", kw_response, gobj);
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE int ac_drop(hgobj gobj, const char *event, json_t *kw, hgobj src)
{
    gobj_send_event(gobj_bottom_gobj(gobj), "EV_DROP", 0, gobj);

    KW_DECREF(kw);
    return 0;
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE int ac_timeout_inactivity(hgobj gobj, const char *event, json_t *kw, hgobj src)
{
    gobj_send_event(gobj_bottom_gobj(gobj), "EV_DROP", 0, gobj);
    KW_DECREF(kw);
    return 0;
}

/***************************************************************************
 *  Child stopped
 ***************************************************************************/
PRIVATE int ac_stopped(hgobj gobj, const char *event, json_t *kw, hgobj src)
{
    if(gobj_is_volatil(src)) {
        gobj_destroy(src);
    }
    KW_DECREF(kw);
    return 0;
}

/***************************************************************************
 *                          FSM
 ***************************************************************************/
PRIVATE const EVENT input_events[] = {
    // top input
    // bottom input
    {"EV_RX_DATA",          0,                  0,  0},
    {"EV_SEND_MESSAGE",     EVF_PUBLIC_EVENT,   0,  0},
    {"EV_CONNECTED",        0,                  0,  0},
    {"EV_DISCONNECTED",     0,                  0,  0},
    {"EV_DROP",             EVF_PUBLIC_EVENT,   0,  0},
    {"EV_TIMEOUT",          0,                  0,  0},
    {"EV_TX_READY",         0,                  0,  0},
    {"EV_STOPPED",          0,                  0,  0},
    // internal
    {NULL, 0, 0, 0}
};
PRIVATE const EVENT output_events[] = {
    {"EV_ON_OPEN",          EVF_PUBLIC_EVENT,   0,  0},
    {"EV_ON_CLOSE",         EVF_PUBLIC_EVENT,   0,  0},
    {"EV_ON_MESSAGE",       EVF_PUBLIC_EVENT,   0,  0},
    {NULL, 0, 0, 0}
};
PRIVATE const char *state_names[] = {
    "ST_DISCONNECTED",
    "ST_CONNECTED",
    NULL
};

PRIVATE EV_ACTION ST_DISCONNECTED[] = {
    {"EV_CONNECTED",        ac_connected,               "ST_CONNECTED"},
    {"EV_DISCONNECTED",     ac_disconnected,            0},
    {"EV_STOPPED",          ac_stopped,                 0},
    {0,0,0}
};
PRIVATE EV_ACTION ST_CONNECTED[] = {
    {"EV_RX_DATA",          ac_rx_data,                 0},
    {"EV_SEND_MESSAGE",     ac_send_message,            0},
    {"EV_TIMEOUT",          ac_timeout_inactivity,      0},
    {"EV_TX_READY",         0,                          0},
    {"EV_DROP",             ac_drop,                    0},
    {"EV_DISCONNECTED",     ac_disconnected,            "ST_DISCONNECTED"},
    {"EV_STOPPED",          ac_stopped,                 0},
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
    GCLASS_PROT_HTTP_CLI_NAME,      // CHANGE WITH each gclass
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
        0, //mt_snap_nodes,
        0, //mt_set_nodes_snap,
        0, //mt_list_nodes_snaps,
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
    0,  // cmds
    0,  // gcflag
};

/***************************************************************************
 *              Public access
 ***************************************************************************/
PUBLIC GCLASS *gclass_prot_http_cli(void)
{
    return &_gclass;
}
