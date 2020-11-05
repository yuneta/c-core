/***********************************************************************
 *          C_FS.C
 *          Watch file system events uv-mixin.
 *
 *          Copyright (c) 2014 Niyamaka.
 *          All Rights Reserved.
 ***********************************************************************/
#include <unistd.h>
#include "c_fs.h"

/***************************************************************************
 *              Constants
 ***************************************************************************/

/***************************************************************************
 *              Structures
 ***************************************************************************/
typedef struct _SUBDIR_WATCH {
    DL_ITEM_FIELDS
    uv_fs_event_t uv_fs;
    hgobj gobj;
} SUBDIR_WATCH;


/***************************************************************************
 *              Prototypes
 ***************************************************************************/
PRIVATE SUBDIR_WATCH * create_subdir_watch(hgobj gobj, const char *path);
PRIVATE void destroy_subdir_watch(SUBDIR_WATCH * sw);
PRIVATE BOOL _locate_subdirs_cb(
    void *user_data,
    wd_found_type type,
    char *fullpath,
    const char *directory,
    char *name,             // dname[255]
    int level,
    int index
);
PRIVATE void on_fs_event_cb(uv_fs_event_t* handle, const char* filename, int events, int status);
PRIVATE void on_close_cb(uv_handle_t* handle);



/***************************************************************************
 *          Data: config, public data, private data
 ***************************************************************************/

/*---------------------------------------------*
 *      Attributes - order affect to oid's
 *---------------------------------------------*/
PRIVATE sdata_desc_t tattr_desc[] = {
SDATA (ASN_OCTET_STR,   "path",                 SDF_RD,  0, "Path to watch"),
SDATA (ASN_BOOLEAN,     "recursive",            SDF_RD,  0, "Watch on all sub-directory tree"),
SDATA (ASN_BOOLEAN,     "info",                 SDF_RD,  0, "Inform of found subdirectories"),
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
    dl_list_t dl_watches;
    BOOL info;
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

    priv->info = gobj_read_bool_attr(gobj, "info");

    dl_init(&priv->dl_watches);
}

/***************************************************************************
 *      Framework Method start
 ***************************************************************************/
PRIVATE int mt_start(hgobj gobj)
{
    const char *path = gobj_read_str_attr(gobj, "path");
    if(!path || access(path, 0)!=0) {
        log_error(0,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_PARAMETER_ERROR,
            "msg",          "%s", "path NOT EXIST",
            "path",         "%s", path,
            NULL
        );
        return -1;
    }

    if(gobj_read_bool_attr(gobj, "recursive")) {
        // WARNING: this doesn't work in linux:
        // flag |= UV_FS_EVENT_RECURSIVE;
        create_subdir_watch(gobj, path); // walk_dir_tree() not return "."
        walk_dir_tree(
            path,
            ".*",
            WD_RECURSIVE|WD_MATCH_DIRECTORY,
            _locate_subdirs_cb,
            gobj
        );
    } else {
        create_subdir_watch(gobj, path);
    }
    return 0;
}

/***************************************************************************
 *      Framework Method stop
 ***************************************************************************/
PRIVATE int mt_stop(hgobj gobj)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    dl_flush(&priv->dl_watches, (fnfree)destroy_subdir_watch);

    return 0;
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
    SUBDIR_WATCH *sw = handle->data;
    hgobj gobj = sw->gobj;

    if(gobj_trace_level(gobj) & TRACE_UV) {
        log_debug_printf(0, "<<< on_close_cb uv_fs p=%p",
            handle
        );
    }
    gobj_publish_event(gobj, "EV_STOPPED", 0);
    gbmem_free(sw);
}

/***************************************************************************
 *      Create a watch
 ***************************************************************************/
PRIVATE SUBDIR_WATCH * create_subdir_watch(hgobj gobj, const char *path)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);
    uv_loop_t *loop = yuno_uv_event_loop();
    SUBDIR_WATCH *sw;

    if(priv->info) {
        log_info(0,
            "gobj",         "%s", gobj_short_name(gobj),
            "msgset",       "%s", MSGSET_MONITORING,
            "msg",          "%s", "watching directory",
            "path",         "%s", path,
            NULL
        );
    }

    sw = gbmem_malloc(sizeof(SUBDIR_WATCH));
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
 *      Destroy a watch
 ***************************************************************************/
PRIVATE void destroy_subdir_watch(SUBDIR_WATCH * sw)
{
    uv_fs_event_stop(&sw->uv_fs);
//     if(gobj_get_user_trace_level(gobj)) {
//         trace_machine(">>> close(%s:%s)\n",
//             gobj_gclass_name(gobj), gobj_name(gobj)
//         );
//     }
    uv_close((uv_handle_t *)&sw->uv_fs, on_close_cb);
}

/***************************************************************************
 *  Located directories
 ***************************************************************************/
PRIVATE BOOL _locate_subdirs_cb(
    void *user_data,
    wd_found_type type,
    char *fullpath,
    const char *directory,
    char *name,             // dname[255]
    int level,
    int index)
{
    hgobj gobj = user_data;

    create_subdir_watch(gobj, fullpath);

    return TRUE; // continue traverse tree
}

/***************************************************************************
 *      fs events callback
 ***************************************************************************/
PRIVATE void on_fs_event_cb(uv_fs_event_t* handle, const char* filename, int events, int status)
{
    SUBDIR_WATCH *sw = handle->data;
    hgobj gobj = sw->gobj;

    json_t *kw = json_pack("{s:s, s:s}",
        "path", handle->path,
        "filename", filename
    );

    if (events == UV_RENAME) {
        gobj_publish_event(gobj, "EV_RENAMED", kw);
    }
    if (events == UV_CHANGE) {
        gobj_publish_event(gobj, "EV_CHANGED", kw);
    }

}




            /***************************
             *      Actions
             ***************************/




/***************************************************************************
 *                          FSM
 ***************************************************************************/
PRIVATE const EVENT input_events[] = {
    {NULL, 0}
};
PRIVATE const EVENT output_events[] = {
    {"EV_RENAMED",  0, 0, 0},
    {"EV_CHANGED",  0, 0, 0},
    {"EV_STOPPED",  0, 0, 0},
    {NULL, 0}
};
PRIVATE const char *state_names[] = {
    "ST_IDLE",
    NULL
};

PRIVATE EV_ACTION ST_IDLE[] = {
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
    GCLASS_FS_NAME,
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
    0,  // acl
    s_user_trace_level,
    0,  // command_table
    0,  // gcflag
};

/***************************************************************************
 *              Public access
 ***************************************************************************/
PUBLIC GCLASS *gclass_fs(void)
{
    return &_gclass;
}
