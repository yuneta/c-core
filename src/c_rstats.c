/***********************************************************************
 *          C_RSTATS.C
 *          Rstats GClass.
 *
 *          Read Statistics Files uv-mixin

Sample of kw create parameter "metric"

{
    "metric_id": "last_week_in_seconds",
    "variable": "flow_rate",
    "period": "WDAY",
    "units": "SEC",
    "compute": "",
    "real": true,
    "data": [
        {
            "fr_t": 1547541724,
            "to_t": 1547592763,
            "fr_d": "2019-01-15T08:42:04.0+0000",
            "to_d": "2019-01-15T22:52:43.0+0000",
            "file": "/yuneta/store/stats/Rtu_caudal^rtu_caudal/flow_rate/last_week_in_seconds-2.dat"
        },
        {
            "fr_t": 1547627256,
            "to_t": 1547683199,
            "fr_d": "2019-01-16T08:27:36.0+0000",
            "to_d": "2019-01-16T23:59:59.0+0000",
            "file": "/yuneta/store/stats/Rtu_caudal^rtu_caudal/flow_rate/last_week_in_seconds-3.dat"
        },
        {
            "fr_t": 1547683200,
            "to_t": 1547764644,
            "fr_d": "2019-01-17T00:00:00.0+0000",
            "to_d": "2019-01-17T22:37:24.0+0000",
            "file": "/yuneta/store/stats/Rtu_caudal^rtu_caudal/flow_rate/last_week_in_seconds-4.dat"
        },
        {
            "fr_t": 1547798296,
            "to_t": 1547853924,
            "fr_d": "2019-01-18T07:58:16.0+0000",
            "to_d": "2019-01-18T23:25:24.0+0000",
            "file": "/yuneta/store/stats/Rtu_caudal^rtu_caudal/flow_rate/last_week_in_seconds-5.dat"
        },
        {
            "fr_t": 1547914905,
            "to_t": 1547915778,
            "fr_d": "2019-01-19T16:21:45.0+0000",
            "to_d": "2019-01-19T16:36:18.0+0000",
            "file": "/yuneta/store/stats/Rtu_caudal^rtu_caudal/flow_rate/last_week_in_seconds-6.dat"
        },
        {
            "fr_t": 1547979235,
            "to_t": 1548009251,
            "fr_d": "2019-01-20T10:13:55.0+0000",
            "to_d": "2019-01-20T18:34:11.0+0000",
            "file": "/yuneta/store/stats/Rtu_caudal^rtu_caudal/flow_rate/last_week_in_seconds-7.dat"
        },
        {
            "fr_t": 1548054447,
            "to_t": 1548074821,
            "fr_d": "2019-01-21T07:07:27.0+0000",
            "to_d": "2019-01-21T12:47:01.0+0000",
            "file": "/yuneta/store/stats/Rtu_caudal^rtu_caudal/flow_rate/last_week_in_seconds-1.dat"
        }
    ]
}

 *          Copyright (c) 2019 Niyamaka.
 *          All Rights Reserved.
 ***********************************************************************/
#include <string.h>
#include "c_rstats.h"

/***************************************************************************
 *              Constants
 ***************************************************************************/

/***************************************************************************
 *              Structures
 ***************************************************************************/
typedef struct _PATH_WATCH {
    DL_ITEM_FIELDS
    uv_fs_event_t uv_fs;
    hgobj gobj;
    FILE *file;
    uint64_t size;
    BOOL real;
} PATH_WATCH;

/***************************************************************************
 *              Prototypes
 ***************************************************************************/
PRIVATE PATH_WATCH * create_path_watch(hgobj gobj, const char *path, BOOL real);
PRIVATE void destroy_path_watch(PATH_WATCH * sw);
PRIVATE void on_fs_event_cb(uv_fs_event_t* handle, const char* filename, int events, int status);
PRIVATE void on_close_cb(uv_handle_t* handle);


/***************************************************************************
 *          Data: config, public data, private data
 ***************************************************************************/

/*---------------------------------------------*
 *      Attributes - order affect to oid's
 *---------------------------------------------*/
PRIVATE sdata_desc_t tattr_desc[] = {
/*-ATTR-type------------name----------------flag----------------default---------description---------- */
SDATA (ASN_JSON,        "metric",           SDF_RD,             0,              "Metric to read"),
SDATA (ASN_POINTER,     "user_data",        0,                  0,              "user data"),
SDATA (ASN_POINTER,     "user_data2",       0,                  0,              "more user data"),
SDATA (ASN_POINTER,     "subscriber",       0,                  0,              "subscriber of output-events. If it's null then subscriber is the parent."),
SDATA_END()
};

/*---------------------------------------------*
 *      GClass trace levels
 *---------------------------------------------*/
enum {
    TRACE_MESSAGES = 0x0001,
};
PRIVATE const trace_level_t s_user_trace_level[16] = {
{"messages",        "Trace messages"},
{0, 0},
};


/*---------------------------------------------*
 *              Private data
 *---------------------------------------------*/
typedef struct _PRIVATE_DATA {
    dl_list_t dl_watches;
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

    /*
     *  CHILD subscription model
     */
    hgobj subscriber = (hgobj)gobj_read_pointer_attr(gobj, "subscriber");
    if(!subscriber) {
        subscriber = gobj_parent(gobj);
    }
    gobj_subscribe_event(gobj, NULL, NULL, subscriber);

    dl_init(&priv->dl_watches);

    /*
     *  Do copy of heavy used parameters, for quick access.
     *  HACK The writable attributes must be repeated in mt_writing method.
     */
//     SET_PRIV(sample_int,            gobj_read_int32_attr)
//     SET_PRIV(sample_str,            gobj_read_str_attr)
}

/***************************************************************************
 *      Framework Method writing
 ***************************************************************************/
PRIVATE void mt_writing(hgobj gobj, const char *path)
{
//     PRIVATE_DATA *priv = gobj_priv_data(gobj);
//
//     IF_EQ_SET_PRIV(sample_int,              gobj_read_int32_attr)
//     ELIF_EQ_SET_PRIV(sample_str,            gobj_read_str_attr)
//     END_EQ_SET_PRIV()
}

/***************************************************************************
 *      Framework Method start
 ***************************************************************************/
PRIVATE int mt_start(hgobj gobj)
{
    json_t *jn_metric = gobj_read_json_attr(gobj, "metric");
    json_t *jn_data = kw_get_list(jn_metric, "data", 0, KW_REQUIRED);
    BOOL real = kw_get_bool(jn_metric, "real", 0, KW_REQUIRED);

    int idx;
    json_t *jn_file;
    json_array_foreach(jn_data, idx, jn_file) {
        const char *file = kw_get_str(jn_file, "file", "", KW_REQUIRED);
        create_path_watch(gobj, file, real);
    }

    return 0;
}

/***************************************************************************
 *      Framework Method stop
 ***************************************************************************/
PRIVATE int mt_stop(hgobj gobj)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    PATH_WATCH *sw = dl_first(&priv->dl_watches);
    while(sw) {
        destroy_path_watch(sw);
        sw = dl_next(sw);
    }

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
 *  Only NOW you can destroy this gobj,
 *  when uv has released the handler.
 ***************************************************************************/
PRIVATE void on_close_cb(uv_handle_t* handle)
{
    PATH_WATCH *sw = handle->data;
    hgobj gobj = sw->gobj;
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    if(gobj_trace_level(gobj) & TRACE_UV) {
        log_debug_printf(0, "<<< on_close_cb uv_fs p=%p",
            handle
        );
    }
    dl_delete(&priv->dl_watches, sw, gbmem_free);
    if(dl_size(&priv->dl_watches)==0) {
        gobj_publish_event(gobj, "EV_STOPPED", 0);
    }
}

/***************************************************************************
 *      Destroy a watch
 ***************************************************************************/
PRIVATE void destroy_path_watch(PATH_WATCH * sw)
{
    EXEC_AND_RESET(fclose, sw->file);
    uv_fs_event_stop(&sw->uv_fs);
    uv_close((uv_handle_t *)&sw->uv_fs, on_close_cb);
}

/***************************************************************************
 *      Create a watch
 ***************************************************************************/
PRIVATE PATH_WATCH * create_path_watch(hgobj gobj, const char *path, BOOL real)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);
    uv_loop_t *loop = yuno_uv_event_loop();
    PATH_WATCH *sw;

    sw = gbmem_malloc(sizeof(PATH_WATCH));
    if(!sw) {
        log_error(0,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_MEMORY_ERROR,
            "msg",          "%s", "gbmem_malloc() FAILED",
            NULL
        );
        return 0;
    }

    if(gobj_trace_level(gobj) & TRACE_UV) {
        log_debug_printf(0, ">>> uv_fs_event_init p=%p", &sw->uv_fs);
    }
    uv_fs_event_init(loop, &sw->uv_fs);
    sw->uv_fs.data = sw;
    sw->gobj = gobj;
    sw->real = real;

    int ret = uv_fs_event_start(
        &sw->uv_fs,
        on_fs_event_cb,
        path,
        0
    );
    if(ret < 0) {
        log_error(0,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_LIBUV_ERROR,
            "msg",          "%s", "uv_fs_event_start() FAILED",
            "uv_error",     "%s", uv_err_name(ret),
            "path",         "%s", path,
            NULL
        );
    }
    dl_add(&priv->dl_watches, sw);

    return sw;
}

/***************************************************************************
 *      fs events callback
 ***************************************************************************/
PRIVATE void on_fs_event_cb(uv_fs_event_t* handle, const char* filename, int events, int status)
{
    PATH_WATCH *sw = handle->data;
    hgobj gobj = sw->gobj;

    if (events == UV_CHANGE) {
        if(!sw->file) {
            sw->file = fopen64(handle->path, "r");
            if(!sw->file) {
                log_error(0,
                    "gobj",         "%s", __FILE__,
                    "function",     "%s", __FUNCTION__,
                    "process",      "%s", get_process_name(),
                    "hostname",     "%s", get_host_name(),
                    "pid",          "%d", get_pid(),
                    "msgset",       "%s", MSGSET_SYSTEM_ERROR,
                    "msg",          "%s", "Cannot open file",
                    "path",         "%s", handle->path,
                    NULL
                );
                return;
            }
            sw->size = filesize2(fileno(sw->file));
        } else {
            uint64_t size = filesize2(fileno(sw->file));
            if(size > sw->size) {
                if(fseeko64(sw->file, sw->size, SEEK_SET)<0) {
                    log_error(0,
                        "gobj",         "%s", __FILE__,
                        "function",     "%s", __FUNCTION__,
                        "process",      "%s", get_process_name(),
                        "hostname",     "%s", get_host_name(),
                        "pid",          "%d", get_pid(),
                        "msgset",       "%s", MSGSET_SYSTEM_ERROR,
                        "msg",          "%s", "Cannot fseek to last_stats_record",
                        "path",         "%s", handle->path,
                        "errno",        "%s", strerror(errno),
                        NULL
                    );
                    EXEC_AND_RESET(fclose, sw->file);
                    return;
                }

                uint64_t n_new_records = (size - sw->size)/sizeof(stats_record_t);
                if(!n_new_records) {
                    return;
                }

                json_t *jn_data = json_object();
                kw_get_str(
                    jn_data,
                    "variable",
                    kw_get_str(gobj_read_json_attr(gobj, "metric"), "variable", "", KW_REQUIRED),
                    KW_CREATE
                );
                kw_get_str(
                    jn_data,
                    "units",
                    kw_get_str(gobj_read_json_attr(gobj, "metric"), "units", "", KW_REQUIRED),
                    KW_CREATE
                );
                json_t *data = kw_get_list(jn_data, "data", json_array(), KW_CREATE);

                stats_record_t stats_record;
                while(n_new_records > 0) {
                    if(fread(&stats_record, sizeof(stats_record_t), 1, sw->file)!=1) {
                        log_error(0,
                            "gobj",         "%s", __FILE__,
                            "function",     "%s", __FUNCTION__,
                            "process",      "%s", get_process_name(),
                            "hostname",     "%s", get_host_name(),
                            "pid",          "%d", get_pid(),
                            "msgset",       "%s", MSGSET_SYSTEM_ERROR,
                            "msg",          "%s", "Cannot read last last_stats_record",
                            "path",         "%s", handle->path,
                            "errno",        "%s", strerror(errno),
                            NULL
                        );
                        EXEC_AND_RESET(fclose, sw->file);
                        json_decref(jn_data);
                        return;
                    }

                    json_t *jn_d = json_object();
                    json_array_append_new(data, jn_d);

                    json_object_set_new(jn_d, "t", json_integer(stats_record.t));

                    char d[80];
                    struct tm *tm = gmtime((time_t *)&stats_record.t);
                    tm2timestamp(d, sizeof(d), tm);
                    json_object_set_new(jn_d, "t2", json_string(d));

                    if(sw->real) {
                        json_object_set_new(jn_d, "y", json_real(stats_record.v.d));
                    } else {
                        json_object_set_new(jn_d, "y", json_integer(stats_record.v.i64));
                    }

                    n_new_records--;
                }
                sw->size = size;

                gobj_publish_event(gobj, "EV_STATS_CHANGE", jn_data);
            }
        }
    }

}




            /***************************
             *      Actions
             ***************************/




/***************************************************************************
 *
 ***************************************************************************/
PRIVATE int ac_sample(hgobj gobj, const char *event, json_t *kw, hgobj src)
{
//     if(gobj_trace_level(gobj) & TRACE_MESSAGES) {
//         log_debug_printf("", "sample");
//     }
    KW_DECREF(kw);
    return 0;
}

/***************************************************************************
 *                          FSM
 ***************************************************************************/
PRIVATE const EVENT input_events[] = {
    // top input
    {"EV_SAMPLE",       0,  0,  ""},
    // bottom input
    // internal
    {NULL, 0, 0, ""}
};
PRIVATE const EVENT output_events[] = {
    {"EV_STATS_CHANGE",     EVF_PUBLIC_EVENT,   0,  0}, // TODO pon permisos
    {"EV_STOPPED",          EVF_PUBLIC_EVENT,   0,  0}, // TODO pon permisos
    {NULL, 0, 0, ""}
};
PRIVATE const char *state_names[] = {
    "ST_IDLE",
    NULL
};

PRIVATE EV_ACTION ST_IDLE[] = {
    {"EV_SAMPLE",       ac_sample,      0},
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
    GCLASS_RSTATS_NAME,
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
    0,  // cmds
    0,  // gcflag
};

/***************************************************************************
 *              Public access
 ***************************************************************************/
PUBLIC GCLASS *gclass_rstats(void)
{
    return &_gclass;
}


