/***********************************************************************
 *          C_AUTHZ.C
 *          Authz GClass.
 *
 *          Authorization Manager
 *
 *          Copyright (c) 2020 Niyamaka.
 *          All Rights Reserved.
 ***********************************************************************/
#include <unistd.h>
#include <sys/types.h>
#include <grp.h>
#include <string.h>
#include <stdio.h>
#include "c_authz.h"

#include "treedb_schema_authzs.c"

/***************************************************************************
 *              Constants
 ***************************************************************************/

/***************************************************************************
 *              Structures
 ***************************************************************************/

/***************************************************************************
 *              Prototypes
 ***************************************************************************/
PRIVATE json_t *identify_system_user(
    hgobj gobj,
    const char *username,
    BOOL include_groups,
    BOOL verbose
);

/***************************************************************************
 *          Data: config, public data, private data
 ***************************************************************************/
PRIVATE json_t *cmd_help(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
PRIVATE json_t *cmd_authzs(hgobj gobj, const char *cmd, json_t *kw, hgobj src);

PRIVATE sdata_desc_t pm_help[] = {
/*-PM----type-----------name------------flag------------default-----description---------- */
SDATAPM (ASN_OCTET_STR, "cmd",          0,              0,          "command about you want help."),
SDATAPM (ASN_UNSIGNED,  "level",        0,              0,          "command search level in childs"),
SDATA_END()
};
PRIVATE sdata_desc_t pm_authzs[] = {
/*-PM----type-----------name------------flag------------default-----description---------- */
SDATAPM (ASN_OCTET_STR, "authz",        0,              0,          "authz about you want help"),
SDATA_END()
};

PRIVATE const char *a_help[] = {"h", "?", 0};

PRIVATE sdata_desc_t command_table[] = {
/*-CMD---type-----------name----------------alias---------------items-----------json_fn---------description---------- */
SDATACM (ASN_SCHEMA,    "help",             a_help,             pm_help,        cmd_help,       "Command's help"),
SDATACM (ASN_SCHEMA,    "authzs",           0,                  pm_authzs,      cmd_authzs,     "Authorization's help"),
SDATA_END()
};

/*---------------------------------------------*
 *      Attributes - order affect to oid's
 *---------------------------------------------*/
PRIVATE sdata_desc_t tattr_desc[] = {
/*-ATTR-type------------name----------------flag----------------default---------description---------- */
SDATA (ASN_JSON,        "initial_load",     SDF_RD,             0,              "Initial data for treedb"),
SDATA (ASN_INTEGER,     "timeout",          SDF_RD,             1*1000,         "Timeout"),
SDATA (ASN_COUNTER64,   "txMsgs",           SDF_RD|SDF_PSTATS,  0,              "Messages transmitted"),
SDATA (ASN_COUNTER64,   "rxMsgs",           SDF_RD|SDF_RSTATS,  0,              "Messages receiveds"),

SDATA (ASN_COUNTER64,   "txMsgsec",         SDF_RD|SDF_RSTATS,  0,              "Messages by second"),
SDATA (ASN_COUNTER64,   "rxMsgsec",         SDF_RD|SDF_RSTATS,  0,              "Messages by second"),
SDATA (ASN_COUNTER64,   "maxtxMsgsec",      SDF_WR|SDF_RSTATS|SDF_AUTHZ_W, 0,   "Max Tx Messages by second"),
SDATA (ASN_COUNTER64,   "maxrxMsgsec",      SDF_WR|SDF_RSTATS|SDF_AUTHZ_W, 0,   "Max Rx Messages by second"),
SDATA (ASN_POINTER,     "user_data",        0,                  0,          "user data"),
SDATA (ASN_POINTER,     "user_data2",       0,                  0,          "more user data"),
SDATA (ASN_POINTER,     "subscriber",       0,                  0,          "subscriber of output-events. Not a child gobj."),
SDATA_END()
};

/*---------------------------------------------*
 *      GClass trace levels
 *  HACK strict ascendent value!
 *  required paired correlative strings
 *  in s_user_trace_level
 *---------------------------------------------*/
enum {
    TRACE_MESSAGES = 0x0001,
};
PRIVATE const trace_level_t s_user_trace_level[16] = {
{"messages",        "Trace messages"},
{0, 0},
};

/*---------------------------------------------*
 *      GClass authz levels
 *---------------------------------------------*/
PRIVATE sdata_desc_t pm_authz_sample[] = {
/*-PM-----type--------------name----------------flag------------description---------- */
SDATAPM0 (ASN_OCTET_STR,    "param sample",     0,              "Param ..."),
SDATA_END()
};

PRIVATE sdata_desc_t authz_table[] = {
/*-AUTHZ-- type---------name------------flag----alias---items---------------description--*/
SDATAAUTHZ (ASN_SCHEMA, "sample",       0,      0,      pm_authz_sample,    "Permission to ..."),
SDATA_END()
};

/*---------------------------------------------*
 *              Private data
 *---------------------------------------------*/
typedef struct _PRIVATE_DATA {
    int32_t timeout;
    hgobj timer;
    uint64_t *ptxMsgs;
    uint64_t *prxMsgs;
    uint64_t txMsgsec;
    uint64_t rxMsgsec;

    hgobj gobj_tranger;
    hgobj gobj_treedb;
    json_t *tranger;

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

    helper_quote2doublequote(treedb_schema_authzs);

    /*
     *  Chequea schema fichador, exit si falla.
     */
    json_t *jn_treedb_schema = legalstring2json(treedb_schema_authzs, TRUE);
    if(!jn_treedb_schema) {
        exit(-1);
    }

    priv->timer = gobj_create(gobj_name(gobj), GCLASS_TIMER, 0, gobj);
    priv->ptxMsgs = gobj_danger_attr_ptr(gobj, "txMsgs");
    priv->prxMsgs = gobj_danger_attr_ptr(gobj, "rxMsgs");

    /*---------------------------*
     *  Create Timeranger
     *---------------------------*/
    char subpath[NAME_MAX];
    snprintf(subpath, sizeof(subpath),
        "%s/%s",
        gobj_yuno_realm_domain(),
        gobj_yuno_role_plus_name()
    );
    char path[PATH_MAX];
    yuneta_store_dir(path, sizeof(path), "authzs", subpath, TRUE);
    json_t *kw_tranger = json_pack("{s:s, s:s, s:b}",
        "path", path,
        "filename_mask", "%Y",
        "master", 1
    );
    priv->gobj_tranger = gobj_create_service(
        "tranger_authz",
        GCLASS_TRANGER,
        kw_tranger,
        gobj
    );

    /*----------------------*
     *  Create Treedb
     *----------------------*/
    const char *treedb_name = kw_get_str(
        jn_treedb_schema,
        "id",
        "authzs",
        KW_REQUIRED
    );
    json_t *kw_resource = json_pack("{s:I, s:s, s:o, s:i}",
        "tranger", (json_int_t)(size_t)priv->tranger,
        "treedb_name", treedb_name,
        "treedb_schema", jn_treedb_schema,
        "exit_on_error", LOG_OPT_EXIT_ZERO
    );

    priv->gobj_treedb = gobj_create_service(
        treedb_name,
        GCLASS_NODE,
        kw_resource,
        gobj
    );

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
    SET_PRIV(timeout,               gobj_read_int32_attr)
}

/***************************************************************************
 *      Framework Method writing
 ***************************************************************************/
PRIVATE void mt_writing(hgobj gobj, const char *path)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    IF_EQ_SET_PRIV(timeout,             gobj_read_int32_attr)
    END_EQ_SET_PRIV()
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

    gobj_start(priv->gobj_tranger);
    priv->tranger = gobj_read_pointer_attr(priv->gobj_tranger, "tranger");

    gobj_write_pointer_attr(priv->gobj_treedb, "tranger", priv->tranger);
    gobj_start(priv->gobj_treedb);

    if(gobj_topic_size(priv->gobj_treedb, "roles")==0 &&
        gobj_topic_size(priv->gobj_treedb, "users")==0 &&
        gobj_topic_size(priv->gobj_treedb, "authorizations")==0
    ) {
        /*------------------------------------*
         *  Empty treedb? initialize treedb
         *-----------------------------------*/
        json_t *initial_load = gobj_read_json_attr(gobj, "initial_load");
        const char *topic_name;
        json_t *topic_records;
        json_object_foreach(initial_load, topic_name, topic_records) {
            int idx; json_t *record;
            json_array_foreach(topic_records, idx, record) {
                json_t *kw_update_node = json_pack("{s:s, s:O, s:{s:b}}",
                    "topic_name", topic_name,
                    "record", record,
                    "options",
                        "create", 1
                );
                gobj_send_event(
                    priv->gobj_treedb,
                    "EV_TREEDB_UPDATE_NODE",
                    kw_update_node,
                    gobj
                );
            }
        }
    }

    /*
     *  Periodic timer for tasks
     */
    gobj_start(priv->timer);
    set_timeout_periodic(priv->timer, priv->timeout); // La verdadera


    struct passwd *pw = getpwuid(getuid()); // TODO get curr id
    if(pw) {
        json_t *user = identify_system_user(gobj, pw->pw_name, TRUE, FALSE);
        log_debug_json(0, user, "FOUND %s?", pw->pw_name);
    }


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

    gobj_stop(priv->gobj_treedb);
    gobj_stop(priv->gobj_tranger);

    priv->tranger = 0;

    return 0;
}




            /***************************
             *      Commands
             ***************************/




/***************************************************************************
 *
 ***************************************************************************/
PRIVATE json_t *cmd_help(hgobj gobj, const char *cmd, json_t *kw, hgobj src)
{
    KW_INCREF(kw);
    json_t *jn_resp = gobj_build_cmds_doc(gobj, kw);
    return msg_iev_build_webix(
        gobj,
        0,
        jn_resp,
        0,
        0,
        kw  // owned
    );
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE json_t *cmd_authzs(hgobj gobj, const char *cmd, json_t *kw, hgobj src)
{
    return gobj_build_authzs_doc(gobj, cmd, kw, src);
}




            /***************************
             *      Local Methods
             ***************************/




/***************************************************************************
 *
 ***************************************************************************/
PRIVATE json_t *identify_system_user(
    hgobj gobj,
    const char *username,
    BOOL include_groups,
    BOOL verbose
)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    json_t *user = gobj_get_node(priv->gobj_treedb, "users", username, 0, gobj);
    if(user) {
        return user;
    }

    if(include_groups) {
        /*-------------------------------*
         *  HACK user's group is valid
         *-------------------------------*/
        gid_t groups[10];
        int ngroups = sizeof(groups)/sizeof(groups[0]);

        getgrouplist(username, 0, groups, &ngroups);
        for(int i=0; i<ngroups; i++) {
            struct group *gr = getgrgid(groups[i]);
            if(gr) {
                user = gobj_get_node(priv->gobj_treedb, "users", gr->gr_name, 0, gobj);
                if(user) {
                    return user;
                }
            }
        }
    }

    if(verbose) {
        log_warning(0,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_INFO,
            "msg",          "%s", "username not found in system",
            "username",     "%s", username,
            NULL
        );
    }
    return 0; // username as user or group not found
}




            /***************************
             *      Actions
             ***************************/




/***************************************************************************
 *
 ***************************************************************************/
PRIVATE int ac_sample(hgobj gobj, const char *event, json_t *kw, hgobj src)
{

    KW_DECREF(kw);
    return 0;
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE int ac_timeout(hgobj gobj, const char *event, json_t *kw, hgobj src)
{

    KW_DECREF(kw);
    return 0;
}


/***************************************************************************
 *                          FSM
 ***************************************************************************/
PRIVATE const EVENT input_events[] = {
    // top input
    {"EV_SAMPLE",       0,  0,  "Description of resource"},
    // bottom input
    {"EV_TIMEOUT",      0,  0,  ""},
    {"EV_STOPPED",      0,  0,  ""},
    // internal
    {NULL, 0, 0, ""}
};
PRIVATE const EVENT output_events[] = {
    {"EV_ON_SAMPLE1",       0,  0,  "Sample1"},
    {"EV_ON_SAMPLE2",       0,  0,  "Sample2"},
    {NULL, 0, 0, ""}
};
PRIVATE const char *state_names[] = {
    "ST_IDLE",
    NULL
};

PRIVATE EV_ACTION ST_IDLE[] = {
    {"EV_SAMPLE",               ac_sample,              0},
    {"EV_TIMEOUT",              ac_timeout,             0},
    {"EV_STOPPED",              0,                      0},
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
    GCLASS_AUTHZ_NAME,
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
    authz_table,
    s_user_trace_level,
    command_table,  // command_table
    0,  // gcflag
};

/***************************************************************************
 *              Public access
 ***************************************************************************/
PUBLIC GCLASS *gclass_authz(void)
{
    return &_gclass;
}

/***************************************************************************
   Check user authz
 ***************************************************************************/
PUBLIC BOOL authz_checker(hgobj gobj_to_check, const char *authz, json_t *kw, hgobj src)
{
    hgobj gobj = gobj_find_gclass_service(GCLASS_AUTHZ_NAME, TRUE);
    if(!gobj) {
        /*
         *  HACK if this function is called is because the authz system is configured in setup.
         *  If the service is not found deny all.
         */
        return FALSE;
    }

    json_t *authzs_list = gobj_authzs(gobj_to_check, authz);
    // TODO
print_json2("=====================>", authzs_list);

    JSON_DECREF(authzs_list);
    KW_DECREF(kw);
    return TRUE;
}

