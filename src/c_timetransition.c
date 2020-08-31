/***********************************************************************
 *          C_TIMETRANSITION.C
 *          Timetransition GClass.
 *          Copyright (c) 2016 Niyamaka.
 *          All Rights Reserved.
***********************************************************************/
#include <string.h>
#include <time.h>
#include <regex.h>
#include <ghelpers.h>
#include "c_timer.h"
#include "c_timetransition.h"

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
/*-ATTR-type------------name----------------flag------------------------default---------description---------- */
SDATA (ASN_INTEGER,     "timeout",          SDF_RD,                     1*1000,         "Timeout polling"),

SDATA (ASN_OCTET_STR,   "time_transition_event_name", SDF_RD,           "EV_TIME_TRANSITION", "Configurable event"),
SDATA (ASN_OCTET_STR,   "time_mask",        SDF_RD,                     "hh",           "Time transition mask: DD/MM/CCYY-hh:mm:ss"),
SDATA (ASN_OCTET_STR,   "last_time",        SDF_PERSIST|SDF_WR,         "",             "Last transition"),

SDATA (ASN_POINTER,     "user_data",        0,                          0,              "user data"),
SDATA (ASN_POINTER,     "user_data2",       0,                          0,              "more user data"),
SDATA (ASN_POINTER,     "subscriber",       0,                          0,              "subscriber of output-events. Default if null is parent."),
SDATA_END()
};

/*---------------------------------------------*
 *      GClass trace levels
 *---------------------------------------------*/
enum {
    TRACE_CHECK_TRANSITION          = 0x0001,
};
PRIVATE const trace_level_t s_user_trace_level[16] = {
{"check-transitions",       "Display calls to check_transitions()"},
{0, 0},
};

/*---------------------------------------------*
 *              Private data
 *---------------------------------------------*/
typedef struct _PRIVATE_DATA {
    hgobj timer;
    const char *time_transition_event_name;

    int32_t timeout;
    hgobj subscriber;
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

    SET_PRIV(timeout,                       gobj_read_int32_attr)
    SET_PRIV(time_transition_event_name,    gobj_read_str_attr)

    /*
     *  Child gobj
     */
    hgobj subscriber = (hgobj)gobj_read_pointer_attr(gobj, "subscriber");
    if(!subscriber)
        subscriber = gobj_parent(gobj);
    gobj_subscribe_event(gobj, NULL, NULL, subscriber);
}

/***************************************************************************
 *      Framework Method start
 ***************************************************************************/
PRIVATE int mt_start(hgobj gobj)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    gobj_start(priv->timer);
    set_timeout_periodic(priv->timer, priv->timeout);

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




            /***************************
             *      Local Methods
             ***************************/




/*****************************************************************
 *
 *****************************************************************/
PRIVATE int _translate_mask(char *new_time, int new_time_size, const char *time_mask)
{
    time_t t;
    struct tm *tm;
    char sfechahora[32];

    time(&t);
    tm = localtime(&t);

    /* Pon en formato DD/MM/CCYY-hh:mm:ss */
    snprintf(sfechahora, sizeof(sfechahora), "%02d/%02d/%4d-%02d:%02d:%02d",
        tm->tm_mday,
        tm->tm_mon+1,
        tm->tm_year + 1900,
        tm->tm_hour,
        tm->tm_min,
        tm->tm_sec
    );

    translate_string(
        new_time,
        new_time_size,
        sfechahora,
        time_mask,
        "DD/MM/CCYY-hh:mm:ss"
    );

    return 0;
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE int check_transitions(hgobj gobj)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);
    int transitions = 0;

    const char *time_mask = gobj_read_str_attr(gobj, "time_mask");
    char new_time[32];
    _translate_mask(new_time, sizeof(new_time), time_mask);
    const char *last_time = gobj_read_str_attr(gobj, "last_time");

    if(gobj_trace_level(gobj) & TRACE_CHECK_TRANSITION) {
        trace_msg("Check -> last_time: %s, new_time: %s",
            new_time,
            last_time
        );
    }

    if(strcmp(new_time, last_time)!=0) {

        json_t *kw = json_pack("{s:s, s:s}",
            "last_time", last_time,
            "new_time", new_time
        );
        gobj_write_str_attr(gobj, "last_time", new_time);

        if(gobj_trace_level(gobj) & TRACE_CHECK_TRANSITION) {
            trace_msg("DONE! ----> last_time: %s, new_time: %s. Publishing '%s' event.",
                new_time,
                last_time,
                priv->time_transition_event_name
            );
        }

        gobj_publish_event(gobj, priv->time_transition_event_name, kw);
        transitions++;
    }

    return transitions;
}




            /***************************
             *      Actions
             ***************************/




/***************************************************************************
 *
 ***************************************************************************/
PRIVATE int ac_timeout(hgobj gobj, const char *event, json_t *kw, hgobj src)
{
    if(check_transitions(gobj)>0) {
        gobj_save_persistent_attrs(gobj);
    }

    JSON_DECREF(kw);
    return 0;
}

/***************************************************************************
 *  If die timer, die self
 ***************************************************************************/
PRIVATE int ac_stopped(hgobj gobj, const char *event, json_t *kw, hgobj src)
{
    JSON_DECREF(kw);
    return 0;
}

/***************************************************************************
 *                          FSM
 ***************************************************************************/
PRIVATE const EVENT input_events[] = {
    {"EV_TIMEOUT",      0},
    {"EV_STOPPED",      0},
    {NULL, 0}
};
PRIVATE const EVENT output_events[] = {
    {"EV_MINUTE_TRANSITION",    0},
    {"EV_HOUR_TRANSITION",      0},
    {"EV_DAY_TRANSITION",       0},
    {"EV_MONTH_TRANSITION",     0},
    {"EV_YEAR_TRANSITION",      0},
    {NULL, 0}
};
PRIVATE const char *state_names[] = {
    "ST_IDLE",
    NULL
};

PRIVATE EV_ACTION ST_IDLE[] = {
    {"EV_TIMEOUT",      ac_timeout,     0},
    {"EV_STOPPED",      ac_stopped,     0},
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
    GCLASS_TIMETRANSITION_NAME,      // CHANGE WITH each gclass
    &fsm,
    {
        mt_create,
        0, //mt_create2,
        0, //mt_destroy,
        mt_start,
        mt_stop,
        0, //mt_play,
        0, //mt_pause,
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
        0, //mt_future61,
        0, //mt_future62,
        0, //mt_future63,
        0, //mt_future64
    },
    lmt,
    tattr_desc,
    sizeof(PRIVATE_DATA),
    0, // __acl__
    s_user_trace_level,
    0, // cmds
    gcflag_no_check_ouput_events, // gcflag
};

/***************************************************************************
 *              Public access
 ***************************************************************************/
PUBLIC GCLASS *gclass_timetransition(void)
{
    return &_gclass;
}
