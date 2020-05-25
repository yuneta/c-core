/***********************************************************************
 *          C_IEVENT_CLI.C
 *          IEvent_cli GClass.
 *
 *          Inter-event (client side)
 *          Simulate a remote service like a local gobj.
 *
 *          Copyright (c) 2016 Niyamaka.
 *          All Rights Reserved.
***********************************************************************/
#include <unistd.h>
#include <string.h>
#include "c_ievent_cli.h"

/***************************************************************************
 *              Constants
 ***************************************************************************/

/***************************************************************************
 *              Structures
 ***************************************************************************/

/***************************************************************************
 *              Prototypes
 ***************************************************************************/
PRIVATE int send_static_iev(
    hgobj gobj,
    const char *event,
    json_t *kw, // owned and serialized,
    hgobj src
);
PRIVATE json_t *build_ievent_request(
    hgobj gobj,
    const char *src_service
);


/***************************************************************************
 *          Data: config, public data, private data
 ***************************************************************************/
/*---------------------------------------------*
 *      Attributes - order affect to oid's
 *---------------------------------------------*/
PRIVATE sdata_desc_t tattr_desc[] = {
SDATA (ASN_OCTET_STR,   "wanted_yuno_role",     SDF_RD, 0, "wanted yuno role"),
SDATA (ASN_OCTET_STR,   "wanted_yuno_name",     SDF_RD, 0, "wanted yuno name"),
SDATA (ASN_OCTET_STR,   "wanted_yuno_service",  SDF_RD, 0, "wanted yuno service"),

SDATA (ASN_OCTET_STR,   "remote_yuno_role",     SDF_RD, 0, "confirmed remote yuno role"),
SDATA (ASN_OCTET_STR,   "remote_yuno_name",     SDF_RD, 0, "confirmed remote yuno name"),
SDATA (ASN_OCTET_STR,   "remote_yuno_service",  SDF_RD, 0, "confirmed remote yuno service"),
SDATA (ASN_JSON,        "extra_info",           SDF_RD, 0, "dict data set by user, added to the identity card msg."),

SDATA (ASN_UNSIGNED,    "timeout_idack",        SDF_RD, 5*1000, "timeout waiting idAck"),

SDATA (ASN_POINTER,     "user_data",            0, 0, "user data"),
SDATA (ASN_POINTER,     "user_data2",           0, 0, "more user data"),
SDATA (ASN_POINTER,     "subscriber",           0, 0, "subscriber of output-events. Not a child gobj."),
SDATA_END()
};

/*---------------------------------------------*
 *      GClass trace levels
 *---------------------------------------------*/
enum {
    TRACE_IEVENTS       = 0x0001,
    TRACE_IEVENTS2      = 0x0002,
    TRACE_IDENTITY_CARD = 0x0004,
};
PRIVATE const trace_level_t s_user_trace_level[16] = {
{"ievents",        "Trace inter-events with metadata of kw"},
{"ievents2",       "Trace inter-events with full kw"},
{"identity-card",  "Trace identity_card messages"},
{0, 0},
};

/*---------------------------------------------*
 *              Private data
 *---------------------------------------------*/
typedef struct _PRIVATE_DATA {
    const char *remote_yuno_name;
    const char *remote_yuno_role;
    const char *remote_yuno_service;
    hgobj timer;
    BOOL inform_on_close;
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

    priv->timer = gobj_create("ievent", GCLASS_TIMER, 0, gobj);

    SET_PRIV(remote_yuno_name,          gobj_read_str_attr)
    SET_PRIV(remote_yuno_role,          gobj_read_str_attr)
    SET_PRIV(remote_yuno_service,       gobj_read_str_attr)
    gobj_write_str_attr(gobj, "wanted_yuno_name", priv->remote_yuno_name?priv->remote_yuno_name:"");
    gobj_write_str_attr(gobj, "wanted_yuno_role", priv->remote_yuno_role?priv->remote_yuno_role:"");
    gobj_write_str_attr(gobj, "wanted_yuno_service", priv->remote_yuno_service?priv->remote_yuno_service:"");

    /*
     *  SERVICE subscription model
     */
    hgobj subscriber = (hgobj)gobj_read_pointer_attr(gobj, "subscriber");
    if(subscriber) {
        gobj_subscribe_event(gobj, NULL, NULL, subscriber);
    }
}

/***************************************************************************
 *      Framework Method writing
 ***************************************************************************/
PRIVATE void mt_writing(hgobj gobj, const char *path)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    IF_EQ_SET_PRIV(remote_yuno_name,        gobj_read_str_attr)
    ELIF_EQ_SET_PRIV(remote_yuno_role,      gobj_read_str_attr)
    ELIF_EQ_SET_PRIV(remote_yuno_service,   gobj_read_str_attr)
    END_EQ_SET_PRIV()
}

/***************************************************************************
 *      Framework Method start
 ***************************************************************************/
PRIVATE int mt_start(hgobj gobj)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    gobj_start(priv->timer);

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

    return 0;
}

/***************************************************************************
 *      Framework Method stats
 ***************************************************************************/
PRIVATE json_t *mt_stats(hgobj gobj, const char *stats, json_t *kw, hgobj src)
{
    if(!gobj_in_this_state(gobj, "ST_SESSION")) {
        log_error(0,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_INTERNAL_ERROR,
            "msg",          "%s", "Not in session",
            "stats",        "%s", stats?stats:"",
            NULL
        );
        KW_DECREF(kw);
        return 0;
    }

    if(!kw) {
        kw = json_object();
    }

    /*
     *      __REQUEST__ __MESSAGE__
     */
    json_t *jn_ievent_id = build_ievent_request(
        gobj,
        gobj_name(src)
    );
    msg_iev_push_stack(
        kw,         // not owned
        IEVENT_MESSAGE_AREA_ID,
        jn_ievent_id   // owned
    );

    json_object_set_new(kw, "__stats__", json_string(stats));

    send_static_iev(gobj, "EV_MT_STATS", kw, src);
    return 0;   // return 0 on asychronous response.
}

/***************************************************************************
 *      Framework Method command
 ***************************************************************************/
PRIVATE json_t *mt_command(hgobj gobj, const char *command, json_t *kw, hgobj src)
{
    if(!gobj_in_this_state(gobj, "ST_SESSION")) {
        log_error(0,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_INTERNAL_ERROR,
            "msg",          "%s", "Not in session",
            "command",      "%s", command,
            NULL
        );
        KW_DECREF(kw);
        return 0;
    }

    if(!kw) {
        kw = json_object();
    }
    /*
     *      __REQUEST__ __MESSAGE__
     */
    json_t *jn_ievent_id = build_ievent_request(
        gobj,
        gobj_name(src)
    );
    msg_iev_push_stack(
        kw,         // not owned
        IEVENT_MESSAGE_AREA_ID,
        jn_ievent_id   // owned
    );
    json_object_set_new(kw, "__command__", json_string(command));
    send_static_iev(gobj, "EV_MT_COMMAND", kw, src);

    return 0;   // return 0 on asychronous response.
}

/***************************************************************************
 *      Framework Method inject_event
 ***************************************************************************/
PRIVATE int mt_inject_event(hgobj gobj, const char *event, json_t *kw, hgobj src)
{
    if(!gobj_in_this_state(gobj, "ST_SESSION")) {
        log_error(0,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_INTERNAL_ERROR,
            "msg",          "%s", "Not in session",
            "event",        "%s", event,
            NULL
        );
        KW_DECREF(kw);
        return -1;
    }
    if(!kw) {
        kw = json_object();
    }

    /*
     *      __MESSAGE__
     */
    json_t *jn_request = msg_iev_get_stack(kw, IEVENT_MESSAGE_AREA_ID, FALSE);
    if(!jn_request) {
        /*
         *  Pon el ievent si no viene con él,
         *  si lo trae es que será alguna respuesta/redirección
         */
        json_t *jn_ievent_id = build_ievent_request(
            gobj,
            gobj_name(src)
        );
        msg_iev_push_stack(
            kw,         // not owned
            IEVENT_MESSAGE_AREA_ID,
            jn_ievent_id   // owned
        );
    }
    return send_static_iev(gobj, event, kw, src);
}

/***************************************************************************
 *      Framework Method subscription_added
 ***************************************************************************/
PRIVATE int send_remote_subscription(
    hgobj gobj,
    hsdata subs)
{
    const char *event = sdata_read_str(subs, "event");
    if(empty_string(event)) {
        // HACK only resend explicit subscriptions
        return -1;
    }
    json_t *__config__ = sdata_read_json(subs, "__config__");
    json_t *__global__ = sdata_read_json(subs, "__global__");
    json_t *__filter__ = sdata_read_json(subs, "__filter__");
    hgobj subscriber = sdata_read_pointer(subs, "subscriber");

    /*
     *      __MESSAGE__
     */
    json_t *kw = json_object();
    if(__config__) {
        json_object_set_new(kw, "__config__", kw_duplicate(__config__));
    }
    if(__global__) {
        json_object_set_new(kw, "__global__", kw_duplicate(__global__));
    }
    if(__filter__) {
        json_object_set_new(kw, "__filter__", kw_duplicate(__filter__));
    }
    json_t *jn_ievent_id = build_ievent_request(
        gobj,
        gobj_name(subscriber)
    );
    msg_iev_push_stack(
        kw,         // not owned
        IEVENT_MESSAGE_AREA_ID,
        jn_ievent_id   // owned
    );
    kw_set_subdict_value(kw, "__md_iev__", "__msg_type__", json_string("__subscribing__"));

    return send_static_iev(gobj, event, kw, gobj);
}

/***************************************************************************
 *      Framework Method subscription_added
 ***************************************************************************/
PRIVATE int mt_subscription_added(
    hgobj gobj,
    hsdata subs)
{
    if(!gobj_in_this_state(gobj, "ST_SESSION")) {
        // on_open will send all subscriptions
        return -1;
    }
    return send_remote_subscription(gobj, subs);
}

/***************************************************************************
 *      Framework Method subscription_deleted
 ***************************************************************************/
PRIVATE int mt_subscription_deleted(
    hgobj gobj,
    hsdata subs)
{
    if(!gobj_in_this_state(gobj, "ST_SESSION")) {
        // Nothing to do. On open this subscription will be not sent.
        return -1;
    }

    const char *event = sdata_read_str(subs, "event");
    if(empty_string(event)) {
        // HACK only resend explicit subscriptions
        return -1;
    }

    json_t *__config__ = sdata_read_json(subs, "__config__");
    json_t *__global__ = sdata_read_json(subs, "__global__");
    json_t *__filter__ = sdata_read_json(subs, "__filter__");
    hgobj subscriber = sdata_read_pointer(subs, "subscriber");

    /*
     *      __MESSAGE__
     */
    json_t *kw = json_object();
    if(__config__) {
        json_object_set_new(kw, "__config__", kw_duplicate(__config__));
    }
    if(__global__) {
        json_object_set_new(kw, "__global__", kw_duplicate(__global__));
    }
    if(__filter__) {
        json_object_set_new(kw, "__filter__", kw_duplicate(__filter__));
    }
    json_t *jn_ievent_id = build_ievent_request(
        gobj,
        gobj_name(subscriber)
    );
    msg_iev_push_stack(
        kw,         // not owned
        IEVENT_MESSAGE_AREA_ID,
        jn_ievent_id   // owned
    );
    kw_set_subdict_value(kw, "__md_iev__", "__msg_type__", json_string("__unsubscribing__"));
    return send_static_iev(gobj, event, kw, gobj);
}




            /***************************
             *      Local Methods
             ***************************/




/***************************************************************************
 *  __MESSAGE__
 ***************************************************************************/
PRIVATE json_t *build_ievent_request(
    hgobj gobj,
    const char *src_service)
{
    json_t *jn_ievent_chain = json_pack("{s:s, s:s, s:s, s:s, s:s, s:s, s:s, s:s}",
        "dst_yuno", gobj_read_str_attr(gobj, "wanted_yuno_name"),
        "dst_role", gobj_read_str_attr(gobj, "wanted_yuno_role"),
        "dst_service", gobj_read_str_attr(gobj, "wanted_yuno_service"),
        "src_yuno", gobj_yuno_name(),
        "src_role", gobj_yuno_role(),
        "src_service", src_service,
        "user", get_user_name(),
        "host", get_host_name()
    );
    return jn_ievent_chain;
}

/***************************************************************************
 *  send our identity card if iam client
 *  we only send the identity card in static routes!
 ***************************************************************************/
PRIVATE int send_identity_card(hgobj gobj)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    BOOL playing = gobj_is_playing(gobj_yuno());
    const char *yuno_version = gobj_read_str_attr(gobj_yuno(), "yuno_version");
    const char *yuno_release = gobj_read_str_attr(gobj_yuno(), "yuno_release");
    const char *yuno_alias = gobj_read_str_attr(gobj_yuno(), "yuno_alias");
    json_int_t launch_id = gobj_read_uint64_attr(gobj_yuno(), "launch_id");
    json_t *kw = json_pack(
        "{s:s, s:s, s:s, s:s, s:s, s:s, s:s, s:b, s:i, s:s, s:I, s:s}",
        "realm_name", gobj_yuno_realm_name(),
        "yuno_role", gobj_yuno_role(),
        "yuno_name", gobj_yuno_name(),
        "yuno_alias", yuno_alias?yuno_alias:"",
        "yuno_version", yuno_version?yuno_version:"",
        "yuno_release", yuno_release?yuno_release:"",
        "yuneta_version", __yuneta_version__,
        "playing", playing,
        "pid", (int)getpid(),
        "jwt", "", // TODO Json Web Token
        "launch_id", launch_id,
        "yuno_startdate", gobj_read_str_attr(gobj_yuno(), "start_date")
    );

    json_t *jn_required_services = gobj_read_json_attr(gobj_yuno(), "required_services");
    if(jn_required_services) {
        json_object_set(kw, "required_services", jn_required_services);
    } else {
        json_object_set_new(kw, "required_services", json_array());
    }

    json_t *jn_extra_info = gobj_read_json_attr(gobj, "extra_info");
    if(jn_extra_info) {
        // Información extra que puede añadir el usuario,
        // para adjuntar a la tarjeta de presentación.
        // No voy a permitir modificar los datos propios del yuno
        // Only update missing
        json_object_update_missing(kw, jn_extra_info);
    }

    /*
     *      __REQUEST__ __MESSAGE__
     */
    json_t *jn_ievent_id = build_ievent_request(
        gobj,
        gobj_name(gobj)
    );
    msg_iev_push_stack(
        kw,         // not owned
        IEVENT_MESSAGE_AREA_ID,
        jn_ievent_id   // owned
    );

    set_timeout(priv->timer, gobj_read_uint32_attr(gobj, "timeout_idack"));
    return send_static_iev(
        gobj,
        "EV_IDENTITY_CARD",
        kw,
        gobj
    );
}

/***************************************************************************
 *  Get bottom level gobj
 ***************************************************************************/
PRIVATE hgobj get_bottom_gobj(hgobj gobj)
{
    hgobj bottom_gobj = gobj_bottom_gobj(gobj);
    if(bottom_gobj) {
        return bottom_gobj;
    } else {
        log_error(0,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_INTERNAL_ERROR,
            "msg",          "%s", "NOT below gobj",
            NULL
        );
    }
    return 0;
}

/***************************************************************************
 *  Route must be connected
 ***************************************************************************/
PRIVATE int send_static_iev(
    hgobj gobj,
    const char *event,
    json_t *kw, // owned and serialized,
    hgobj src
)
{
    hgobj below_gobj = get_bottom_gobj(gobj);
    if(!below_gobj) {
        // Error already logged
        return -1;
    }
    json_t *jn_iev = iev_create(
        event,
        kw  // owned and serialized
    );
    if(!jn_iev) {
        // error already logged
        return -1;
    }
    uint32_t trace_level = gobj_trace_level(gobj);
    if(trace_level) {
        char prefix[256];
        snprintf(prefix, sizeof(prefix),
            "INTRA-EVENT(%s^%s) %s ==> %s",
            gobj_yuno_role(),
            gobj_yuno_name(),
            gobj_short_name(src),
            gobj_short_name(below_gobj)
        );
        const char *event_ = kw_get_str(jn_iev, "event", "?", 0);
        json_t *kw_ = kw_get_dict_value(jn_iev, "kw", 0, 0);
        if((trace_level & TRACE_IEVENTS2)) {
            trace_inter_event2(prefix, event_, kw_);
        } else if((trace_level & TRACE_IEVENTS)) {
            trace_inter_event(prefix, event_, kw_);
        } else if((trace_level & TRACE_IDENTITY_CARD)) {
            if(strcmp(event, "EV_IDENTITY_CARD")==0 ||
                    strcmp(event, "EV_IDENTITY_CARD_ACK")==0) {
                trace_inter_event2(prefix, event_, kw_);
            }
        }
    }

    GBUFFER *gbuf = iev2gbuffer(jn_iev, FALSE); // inter_event decref
    json_t *kw_send = json_pack("{s:I}",
        "gbuffer", (json_int_t)(size_t)gbuf
    );
    return gobj_send_event(below_gobj,
        "EV_SEND_MESSAGE",
        kw_send,
        gobj
    );
}

/***************************************************************************
 *  resend subscriptions
 ***************************************************************************/
PRIVATE int resend_subscriptions(hgobj gobj)
{
    dl_list_t *dl_subs = gobj_find_subscriptions(gobj, 0, 0, 0);

    rc_instance_t *i_subs; hsdata subs;
    i_subs = rc_first_instance(dl_subs, (rc_resource_t **)&subs);
    while(i_subs) {
        send_remote_subscription(gobj, subs);
        i_subs = rc_next_instance(i_subs, (rc_resource_t **)&subs);
    }
    rc_free_iter(dl_subs, TRUE, FALSE);
    return 0;
}






            /***************************
             *      Actions
             ***************************/




/***************************************************************************
 *
 ***************************************************************************/
PRIVATE int ac_on_open(hgobj gobj, const char *event, json_t *kw, hgobj src)
{
    /*
     *  Static route
     *  Send our identity card as iam client
     */
    send_identity_card(gobj);

    KW_DECREF(kw);
    return 0;
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE int ac_on_close(hgobj gobj, const char *event, json_t *kw, hgobj src)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    /*
     *  Route (channel) close.
     */
    if(priv->inform_on_close) {
        priv->inform_on_close = FALSE;
        json_t *kw_on_close = json_pack("{s:s, s:s, s:s}",
            "remote_yuno_name", gobj_read_str_attr(gobj, "remote_yuno_name"),
            "remote_yuno_role", gobj_read_str_attr(gobj, "remote_yuno_role"),
            "remote_yuno_service", gobj_read_str_attr(gobj, "remote_yuno_service")
        );
        gobj_publish_event(gobj, "EV_ON_CLOSE", kw_on_close);
    }

    KW_DECREF(kw);
    return 0;
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE int ac_timeout_wait_idAck(hgobj gobj, const char *event, json_t *kw, hgobj src)
{
    gobj_send_event(get_bottom_gobj(gobj), "EV_DROP", 0, gobj);

    KW_DECREF(kw);
    return 0;
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE int ac_identity_card_ack(hgobj gobj, const char *event, json_t *kw, hgobj src)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    /*---------------------------------------*
     *  Clear timeout
     *---------------------------------------*/
    clear_timeout(priv->timer);

    /*---------------------------------------*
     *  Update remote values
     *---------------------------------------*/
    /*
     *  Here is the end point of the request.
     *  Don't pop the request, because in the
     *  the event can be publish to serveral users.
     */
    /*
     *      __ANSWER__ __MESSAGE__
     */
    json_t *jn_ievent_id = msg_iev_get_stack(kw, IEVENT_MESSAGE_AREA_ID, TRUE);
    const char *src_yuno = kw_get_str(jn_ievent_id, "src_yuno", "", 0);
    const char *src_role = kw_get_str(jn_ievent_id, "src_role", "", 0);
    const char *src_service = kw_get_str(jn_ievent_id, "src_service", "", 0);
    gobj_write_str_attr(gobj, "remote_yuno_name", src_yuno);
    gobj_write_str_attr(gobj, "remote_yuno_role", src_role);
    gobj_write_str_attr(gobj, "remote_yuno_service", src_service);

    // WARNING comprueba result, ahora puede venir negativo
    int result = kw_get_int(kw, "result", -1, 0);
    if(result < 0) {
        gobj_send_event(get_bottom_gobj(gobj), "EV_DROP", 0, gobj);
    } else {
        json_t *jn_data = kw_get_dict_value(kw, "data", 0, 0);

        gobj_change_state(gobj, "ST_SESSION");

        if(!priv->inform_on_close) {
            priv->inform_on_close = TRUE;
            json_t *kw_on_open = json_pack("{s:s, s:s, s:s, s:O}",
                "remote_yuno_name", gobj_read_str_attr(gobj, "remote_yuno_name"),
                "remote_yuno_role", gobj_read_str_attr(gobj, "remote_yuno_role"),
                "remote_yuno_service", gobj_read_str_attr(gobj, "remote_yuno_service"),
                "data", jn_data?jn_data:json_null()
            );
            gobj_publish_event(gobj, "EV_ON_OPEN", kw_on_open);
        }

        /*
        *  Resend subscriptions
        */
        resend_subscriptions(gobj);
    }

    JSON_DECREF(jn_ievent_id);
    KW_DECREF(kw);
    return 0;
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE int ac_on_message(hgobj gobj, const char *event, json_t *kw, hgobj src)
{
    GBUFFER *gbuf = (GBUFFER *)(size_t)kw_get_int(kw, "gbuffer", 0, FALSE);

    /*---------------------------------------*
     *  Create inter_event from gbuf
     *---------------------------------------*/
    gbuf_incref(gbuf);
    iev_msg_t iev_msg;
    if(iev_create_from_gbuffer(&iev_msg, gbuf, 0)<0) {
        log_error(0,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_INTERNAL_ERROR,
            "msg",          "%s", "iev_create_from_gbuffer() FAILED",
            NULL
        );
        gobj_send_event(get_bottom_gobj(gobj), "EV_DROP", 0, gobj);
        KW_DECREF(kw);
        return -1;
    }

    /*---------------------------------------*
     *          trace inter_event
     *---------------------------------------*/
    char *iev_event = iev_msg.event;
    json_t *iev_kw = iev_msg.kw;

    uint32_t trace_level = gobj_trace_level(gobj);
    if(trace_level) {
        char prefix[256];
        snprintf(prefix, sizeof(prefix),
            "INTRA-EVENT(%s^%s) %s <== %s",
            gobj_yuno_role(),
            gobj_yuno_name(),
            gobj_short_name(gobj),
            gobj_short_name(src)
        );
        if((trace_level & TRACE_IEVENTS2)) {
            trace_inter_event2(prefix, iev_event, iev_kw);
        } else if((trace_level & TRACE_IEVENTS)) {
            trace_inter_event(prefix, iev_event, iev_kw);
        } else if((trace_level & TRACE_IDENTITY_CARD)) {
            if(strcmp(iev_event, "EV_IDENTITY_CARD")==0 ||
                    strcmp(iev_event, "EV_IDENTITY_CARD_ACK")==0) {
                trace_inter_event2(prefix, iev_event, iev_kw);
            }
        }
    }

    /*---------------------------------------*
     *  If state is not SESSION send self.
     *  Mainly process EV_IDENTITY_CARD_ACK
     *---------------------------------------*/
    if(!gobj_in_this_state(gobj, "ST_SESSION")){
        if(gobj_event_in_input_event_list(gobj, iev_event, EVF_PUBLIC_EVENT)) {
            if(gobj_send_event(gobj, iev_event, iev_kw, gobj)==0) {
                KW_DECREF(kw);
                return 0;
            }
        }
        log_error(0,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_INTERNAL_ERROR,
            "msg",          "%s", "event failed in not session state",
            "event",        "%s", iev_event,
            NULL
        );
        gobj_send_event(get_bottom_gobj(gobj), "EV_DROP", 0, gobj);
        KW_DECREF(kw);
        return 0;
    }

    /*------------------------------------*
     *   Analyze inter_event
     *------------------------------------*/
    const char *msg_type = kw_get_str(iev_kw, "__md_iev__`__msg_type__", "", 0);

    /*----------------------------------------*
     *  Get inter-event routing information.
     *----------------------------------------*/
    json_t *jn_ievent_id = msg_iev_get_stack(iev_kw, IEVENT_MESSAGE_AREA_ID, TRUE);

    /*----------------------------------------*
     *  Check dst role^name
     *----------------------------------------*/
    const char *iev_dst_role = kw_get_str(jn_ievent_id, "dst_role", "", 0);
    if(!empty_string(iev_dst_role)) {
        if(strcmp(iev_dst_role, gobj_yuno_role())!=0) {
            log_error(0,
                "gobj",         "%s", gobj_full_name(gobj),
                "function",     "%s", __FUNCTION__,
                "msgset",       "%s", MSGSET_PARAMETER_ERROR,
                "msg",          "%s", "It's not my role",
                "yuno_role",    "%s", iev_dst_role,
                "my_role",      "%s", gobj_yuno_role(),
                NULL
            );
            gobj_send_event(get_bottom_gobj(gobj), "EV_DROP", 0, gobj);
            KW_DECREF(iev_kw);
            KW_DECREF(kw);
            return -1;
        }
    }
    const char *iev_dst_yuno = kw_get_str(jn_ievent_id, "dst_yuno", "", 0);
    if(!empty_string(iev_dst_yuno)) {
        char *px = strchr(iev_dst_yuno, '^');
        if(px) {
            *px = 0;
        }
        int ret = strcmp(iev_dst_yuno, gobj_yuno_name());
        if(px) {
            *px = '^';
        }
        if(ret!=0) {
            log_error(0,
                "gobj",         "%s", gobj_full_name(gobj),
                "function",     "%s", __FUNCTION__,
                "msgset",       "%s", MSGSET_PARAMETER_ERROR,
                "msg",          "%s", "It's not my name",
                "yuno_name",    "%s", iev_dst_yuno,
                "my_name",      "%s", gobj_yuno_name(),
                NULL
            );
            gobj_send_event(get_bottom_gobj(gobj), "EV_DROP", 0, gobj);
            KW_DECREF(iev_kw);
            KW_DECREF(kw);
            return -1;
        }
    }

    /*----------------------------------------*
     *  Check dst service
     *----------------------------------------*/
    const char *iev_dst_service = kw_get_str(jn_ievent_id, "dst_service", "", 0);
    // TODO de momento pasa todo, multi-servicio.
    // Obligado al servicio acordado en el identity_card.
    // (priv->gobj_service)
    // Puede venir empty si se autoriza a buscar el evento publico en otros servicios

    /*------------------------------------*
     *   Dispatch event
     *------------------------------------*/
    if(strcasecmp(msg_type, "__subscribing__")==0) {
        /*-----------------------------------*
         *  It's a external subscription
         *-----------------------------------*/
        log_error(0,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_PARAMETER_ERROR,
            "msg",          "%s", "subscription event ignored, I'm client",
            "service",      "%s", iev_dst_service,
            "event",        "%s", iev_event,
            NULL
        );
        gobj_send_event(get_bottom_gobj(gobj), "EV_DROP", 0, gobj);
        KW_DECREF(iev_kw);
        KW_DECREF(kw);
        return -1;

    } else if(strcasecmp(msg_type, "__unsubscribing__")==0) {
        /*-----------------------------------*
         *  It's a external unsubscription
         *-----------------------------------*/
        log_error(0,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_PARAMETER_ERROR,
            "msg",          "%s", "subscription event ignored, I'm client",
            "service",      "%s", iev_dst_service,
            "event",        "%s", iev_event,
            NULL
        );
        gobj_send_event(get_bottom_gobj(gobj), "EV_DROP", 0, gobj);
        KW_DECREF(iev_kw);
        KW_DECREF(kw);
        return -1;

    } else {
        /*-------------------------------------------------------*
         *  Publish the event
         *  Graph documentation in:
         *  https://www.draw.io/#G0B-uHwiqttlN9UG5xTm50ODBFbnM
         *  Este documento estaba hecho pensando en los antiguos
         *  inter-eventos. Hay que rehacerlo sobre los nuevos.
         *-------------------------------------------------------*/
        /*-------------------------------------------------------*
         *  Filter public events of this gobj
         *-------------------------------------------------------*/
        if(gobj_event_in_input_event_list(gobj, iev_event, EVF_PUBLIC_EVENT)) {
            /*
             *  It's mine (I manage inter-command and inter-stats)
             */
            gobj_send_event(
                gobj,
                iev_event,
                iev_kw,
                gobj
            );
            KW_DECREF(kw);
            return 0;
        }

        /*-------------------------*
         *  Dispatch the event
         *-------------------------*/
        hgobj gobj_service = gobj_find_unique_gobj(iev_dst_service, FALSE);
        if(gobj_service) {
            if(gobj_event_in_input_event_list(gobj_service, iev_event, EVF_PUBLIC_EVENT)) {
                gobj_send_event(gobj_service, iev_event, iev_kw, gobj);
            } else {
                log_error(0,
                    "gobj",         "%s", gobj_full_name(gobj),
                    "function",     "%s", __FUNCTION__,
                    "msgset",       "%s", MSGSET_INTERNAL_ERROR,
                    "msg",          "%s", "event ignored, not in input_event_list or not PUBLIC event",
                    "service",      "%s", iev_dst_service,
                    "gobj_service", "%s", gobj_short_name(gobj_service),
                    "event",        "%s", iev_event,
                    NULL
                );
                KW_DECREF(iev_kw);
            }
        } else {
            gobj_publish_event( /* NOTE original behaviour */
                gobj,
                iev_event,
                iev_kw
            );
        }
    }

    KW_DECREF(kw);
    return 0;
}

/***************************************************************************
 *  Play yuno
 ***************************************************************************/
PRIVATE int ac_play_yuno(hgobj gobj, const char *event, json_t *kw, hgobj src)
{
    int ret = gobj_play(gobj_yuno());
    json_t *jn_result = json_pack("{s:i}",
        "result",
        ret
    );
    json_t *kw2resp = msg_iev_answer(gobj, kw, jn_result, 0);

    return send_static_iev(gobj,
        "EV_PLAY_YUNO_ACK",
        kw2resp,
        src
    );
}

/***************************************************************************
 *  Pause yuno
 ***************************************************************************/
PRIVATE int ac_pause_yuno(hgobj gobj, const char *event, json_t *kw, hgobj src)
{
    int ret = gobj_pause(gobj_yuno());
    json_t *jn_result = json_pack("{s:i}",
        "result",
        ret
    );
    json_t *kw2resp = msg_iev_answer(gobj, kw, jn_result, 0);

    return send_static_iev(gobj,
        "EV_PAUSE_YUNO_ACK",
        kw2resp,
        src
    );
}

/***************************************************************************
 *  remote ask for stats
 ***************************************************************************/
PRIVATE int ac_mt_stats(hgobj gobj, const char *event, json_t *kw, hgobj src)
{
    /*
     *      __MESSAGE__
     */
    //json_t *jn_request = msg_iev_get_stack(kw, IEVENT_MESSAGE_AREA_ID);

    const char *stats = kw_get_str(kw, "__stats__", 0, 0);
    const char *service = kw_get_str(kw, "service", "", 0);

    hgobj service_gobj;
    if(empty_string(service)) {
        service_gobj = gobj_default_service();
    } else {
        service_gobj = gobj_find_service(service, FALSE);
        if(!service_gobj) {
            return send_static_iev(gobj,
                "EV_MT_COMMAND_ANSWER",
                msg_iev_build_webix(
                    gobj,
                    -100,
                    json_local_sprintf("Service '%s' not found.", service),
                    0,
                    0,
                    kw
                ),
                src
            );
        }
    }
    KW_INCREF(kw);
    json_t *webix = gobj_stats(
        service_gobj,
        stats,
        kw,
        src
    );
    if(!webix) {
       // Asynchronous response
    } else {
        json_t * kw2 = msg_iev_answer(gobj, kw, webix, 0);
        return send_static_iev(gobj,
            "EV_MT_STATS_ANSWER",
            kw2,
            src
        );
    }

    KW_DECREF(kw);
    return 0;
}

/***************************************************************************
 *  remote ask for command
 ***************************************************************************/
PRIVATE int ac_mt_command(hgobj gobj, const char *event, json_t *kw, hgobj src)
{
    /*
     *      __MESSAGE__
     */
    //json_t *jn_request = msg_iev_get_stack(kw, IEVENT_MESSAGE_AREA_ID);

    const char *command = kw_get_str(kw, "__command__", 0, 0);
    const char *service = kw_get_str(kw, "service", "", 0);

    hgobj service_gobj;
    if(empty_string(service)) {
        service_gobj = gobj_default_service();
    } else {
        service_gobj = gobj_find_service(service, FALSE);
        if(!service_gobj) {
            service_gobj = gobj_find_gobj(service);
            if(!service_gobj) {
                return send_static_iev(gobj,
                    "EV_MT_COMMAND_ANSWER",
                    msg_iev_build_webix(
                        gobj,
                        -100,
                        json_local_sprintf("Service '%s' not found.", service),
                        0,
                        0,
                        kw
                    ),
                    src
                );
            }
        }
    }
    KW_INCREF(kw);
    json_t *webix = gobj_command(
        service_gobj,
        command,
        kw,
        src
    );
    if(!webix) {
       // Asynchronous response
    } else {
        json_t * kw2 = msg_iev_answer(gobj, kw, webix, 0);
        return send_static_iev(gobj,
            "EV_MT_COMMAND_ANSWER",
            kw2,
            src
        );
    }

    KW_DECREF(kw);
    return 0;
}

/***************************************************************************
 *  For asynchronous responses
 ***************************************************************************/
PRIVATE int ac_send_command_answer(hgobj gobj, const char *event, json_t *kw, hgobj src)
{
    return send_static_iev(gobj,
        "EV_MT_COMMAND_ANSWER",
        kw,
        src
    );
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE int ac_stopped(hgobj gobj, const char *event, json_t *kw, hgobj src)
{
    /*
     *  someone or ... has stopped
     */
    KW_DECREF(kw);
    return 0;
}


/***************************************************************************
 *                          FSM
 ***************************************************************************/
PRIVATE const EVENT input_events[] = {
    // top input
    // bottom input
    {"EV_ON_MESSAGE",       0,  0,  0},
    {"EV_ON_OPEN",          0,  0,  0},
    {"EV_ON_CLOSE",         0,  0,  0},
    {"EV_IDENTITY_CARD_ACK",        EVF_PUBLIC_EVENT,  0,  0},
    {"EV_PLAY_YUNO",                EVF_PUBLIC_EVENT,  0,  0},  // Extra events to let agent
    {"EV_PAUSE_YUNO",               EVF_PUBLIC_EVENT,  0,  0},  // request clients
    {"EV_MT_STATS",                 EVF_PUBLIC_EVENT,  0,  0},
    {"EV_MT_COMMAND",               EVF_PUBLIC_EVENT,  0,  0},
    {"EV_SEND_COMMAND_ANSWER",      EVF_PUBLIC_EVENT,  0,  0},
    // internal
    {"EV_TIMEOUT",                  0,  0,  0},
    {"EV_STOPPED",                  0,  0,  0},
    {NULL, 0}
};
PRIVATE const EVENT output_events[] = {
    {"EV_ON_OPEN",          EVF_NO_WARN_SUBS,  0,  0},
    {"EV_ON_CLOSE",         EVF_NO_WARN_SUBS,  0,  0},
    {NULL, 0}
};
PRIVATE const char *state_names[] = {
    "ST_DISCONNECTED",
    "ST_WAIT_IDENTITY_CARD_ACK",
    "ST_SESSION",
    NULL
};

PRIVATE EV_ACTION ST_DISCONNECTED[] = {
    {"EV_ON_OPEN",              ac_on_open,             "ST_WAIT_IDENTITY_CARD_ACK"},
    {"EV_STOPPED",              ac_stopped,             0},
    {0,0,0}
};
PRIVATE EV_ACTION ST_WAIT_IDENTITY_CARD_ACK[] = {
    {"EV_ON_MESSAGE",           ac_on_message,          0},
    {"EV_IDENTITY_CARD_ACK",    ac_identity_card_ack,   0},
    {"EV_ON_CLOSE",             ac_on_close,            "ST_DISCONNECTED"},
    {"EV_TIMEOUT",              ac_timeout_wait_idAck,  0},
    {0,0,0}
};
PRIVATE EV_ACTION ST_SESSION[] = {
    {"EV_ON_MESSAGE",           ac_on_message,          0},
    {"EV_MT_STATS",             ac_mt_stats,            0},
    {"EV_MT_COMMAND",           ac_mt_command,          0},
    {"EV_SEND_COMMAND_ANSWER",  ac_send_command_answer, 0},
    {"EV_PLAY_YUNO",            ac_play_yuno,           0},
    {"EV_PAUSE_YUNO",           ac_pause_yuno,          0},
    {"EV_ON_CLOSE",             ac_on_close,            "ST_DISCONNECTED"},
    {"EV_STOPPED",              0,                      0}, // fuck libuv
    {0,0,0}
};


PRIVATE EV_ACTION *states[] = {
    ST_DISCONNECTED,
    ST_WAIT_IDENTITY_CARD_ACK,
    ST_SESSION,
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
    GCLASS_IEVENT_NAME,      // CHANGE WITH each gclass
    &fsm,
    {
        mt_create,
        0, //mt_create2,
        0, //mt_destroy,
        mt_start,
        mt_stop,
        0, //mt_play,
        0, //mt_pause,
        mt_writing,
        0, //mt_reading,
        mt_subscription_added,
        mt_subscription_deleted,
        0, //mt_child_added,
        0, //mt_child_removed,
        mt_stats,
        mt_command,
        mt_inject_event,
        0, //mt_create_resource,
        0, //mt_list_resource,
        0, //mt_update_resource,
        0, //mt_delete_resource,
        0, //mt_add_child_resource_link
        0, //mt_delete_child_resource_link
        0, //mt_get_resource
        0, //mt_future24,
        0, //mt_future33,
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
        0, //treedb_topics,
        0, //topic_desc,
        0, //topic_links,
        0, //topic_hooks,
        0, //node_parents,
        0, //node_childs,
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
    0, // acl,
    s_user_trace_level,
    0, // cmds,
    gcflag_no_check_ouput_events, // gcflag,
};

/***************************************************************************
 *              Public access
 ***************************************************************************/
PUBLIC GCLASS *gclass_ievent(void)
{
    return &_gclass;
}
