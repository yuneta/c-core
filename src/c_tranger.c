/***********************************************************************
 *          C_TRANGER.C
 *          Tranger GClass.
 *
 *          Trangers: resources with tranger

To test with CLI:

command-yuno id=1911 service=tranger command=open-list list_id=pepe topic_name=pp fields=id,__md_tranger__

command-yuno id=1911 service=__yuno__ command=subscribe-event gobj_name=tranger event_name=EV_TRANGER_RECORD_ADDED rename_event_name=EV_MT_COMMAND_ANSWER first_shot=1 config={"__list_id__":"pepe"}

command-yuno id=1911 service=__yuno__ command=subscribe-event gobj_name=tranger event_name=EV_TRANGER_RECORD_ADDED rename_event_name=EV_MT_COMMAND_ANSWER  config={"__list_id__":"pepe"}

command-yuno id=1911 service=tranger command=add-record topic_name=pp record='{"id":"1","tm":0}'

command-yuno id=1911 service=tranger command=close-list list_id=pepe

command-yuno id=1911 service=tranger command=open-list list_id=pepe topic_name=pp only_md=1
command-yuno id=1911 service=tranger command=get-list-data list_id=pepe
command-yuno id=1911 service=tranger command=add-record topic_name=pp record='{"id":"1","tm":0}'
command-yuno id=1911 service=tranger command=close-list list_id=pepe

command-yuno id=1911 service=tranger command=open-list list_id=pepe topic_name=pp
command-yuno id=1911 service=tranger command=get-list-data list_id=pepe
command-yuno id=1911 service=tranger command=add-record topic_name=pp record='{"id":"1","tm":0}'
command-yuno id=1911 service=tranger command=close-list list_id=pepe


 *          Copyright (c) 2020 Niyamaka.
 *          All Rights Reserved.
 ***********************************************************************/
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include "c_tranger.h"

/***************************************************************************
 *              Constants
 ***************************************************************************/

/***************************************************************************
 *              Structures
 ***************************************************************************/

/***************************************************************************
 *              Prototypes
 ***************************************************************************/
PRIVATE int load_record_callback(
    json_t *tranger,
    json_t *topic,
    json_t *list,
    md_record_t *md_record,
    json_t *jn_record
);

/***************************************************************************
 *          Data: config, public data, private data
 ***************************************************************************/
PRIVATE json_t *cmd_help(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
PRIVATE json_t *cmd_print_tranger(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
PRIVATE json_t *cmd_topics(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
PRIVATE json_t *cmd_desc(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
PRIVATE json_t *cmd_create_topic(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
PRIVATE json_t *cmd_delete_topic(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
PRIVATE json_t *cmd_open_list(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
PRIVATE json_t *cmd_close_list(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
PRIVATE json_t *cmd_add_record(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
PRIVATE json_t *cmd_get_list_data(hgobj gobj, const char *cmd, json_t *kw, hgobj src);

PRIVATE sdata_desc_t pm_help[] = {
/*-PM----type-----------name------------flag------------default-----description---------- */
SDATAPM (ASN_OCTET_STR, "cmd",          0,              0,          "command about you want help."),
SDATAPM (ASN_UNSIGNED,  "level",        0,              0,          "command search level in childs"),
SDATA_END()
};

PRIVATE sdata_desc_t pm_print_tranger[] = {
/*-PM----type-----------name------------flag------------default-----description---------- */
SDATAPM (ASN_OCTET_STR, "path",         0,              "",         "Path"),
SDATAPM (ASN_BOOLEAN,   "expanded",     0,              0,          "No expanded (default) return [[size]]"),
SDATAPM (ASN_UNSIGNED,  "lists_limit",  0,              0,          "Expand lists only if size < limit. 0 no limit"),
SDATAPM (ASN_UNSIGNED,  "dicts_limit",  0,              0,          "Expand dicts only if size < limit. 0 no limit"),
SDATA_END()
};

PRIVATE sdata_desc_t pm_desc[] = {
SDATAPM (ASN_OCTET_STR, "topic_name",   0,              0,          "Topic name"),
SDATA_END()
};

PRIVATE sdata_desc_t pm_create_topic[] = {
SDATAPM (ASN_OCTET_STR, "topic_name",   0,              0,          "Topic name"),
SDATAPM (ASN_OCTET_STR, "pkey",         0,              "id",       "Primary Key"),
SDATAPM (ASN_OCTET_STR, "tkey",         0,              "tm",       "Time Key"),
SDATAPM (ASN_OCTET_STR, "system_flag",  0,              "sf_string_key|sf_t_ms", "System flag: sf_string_key|sf_rowid_key|sf_int_key|sf_t_ms|sf_tm_ms|sf_no_record_disk|sf_no_md_disk, future: sf_zip_record|sf_cipher_record"),
SDATAPM (ASN_JSON,      "jn_cols",      0,              0,          "Cols"),
SDATAPM (ASN_JSON,      "jn_var",       0,              0,          "Var"),
SDATA_END()
};
PRIVATE sdata_desc_t pm_delete_topic[] = {
SDATAPM (ASN_OCTET_STR, "topic_name",   0,              0,          "Topic name"),
SDATAPM (ASN_BOOLEAN,   "force",        0,              0,          "Force delete"),
SDATA_END()
};
PRIVATE sdata_desc_t pm_add_record[] = {
SDATAPM (ASN_OCTET_STR, "topic_name",   0,              0,          "Topic name"),
SDATAPM (ASN_COUNTER64, "__t__",        0,              0,          "Time of record"),
SDATAPM (ASN_UNSIGNED,  "user_flag",    0,              0,          "User flag of record"),
SDATAPM (ASN_JSON,      "record",       0,              0,          "Record json"),
SDATA_END()
};

PRIVATE sdata_desc_t pm_open_list[] = {
/*-PM----type-----------name--------------------flag----default-description---------- */
SDATAPM (ASN_OCTET_STR, "list_id",              0,      0,      "Id of list"),
SDATAPM (ASN_BOOLEAN,   "return_data",          0,      0,      "True for return list data"),
SDATAPM (ASN_OCTET_STR, "topic_name",           0,      0,      "Topic name"),
SDATAPM (ASN_OCTET_STR, "fields",               0,      0,      "match_cond: Only this fields"),
SDATAPM (ASN_BOOLEAN,   "backward",             0,      0,      "match_cond:"),
SDATAPM (ASN_BOOLEAN,   "only_md",              0,      0,      "match_cond: don't load jn_record on loading disk"),
SDATAPM (ASN_INTEGER64, "from_rowid",           0,      0,      "match_cond:"),
SDATAPM (ASN_INTEGER64, "to_rowid",             0,      0,      "match_cond:"),
SDATAPM (ASN_OCTET_STR, "from_t",               0,      0,      "match_cond:"),
SDATAPM (ASN_OCTET_STR, "to_t",                 0,      0,      "match_cond:"),
SDATAPM (ASN_UNSIGNED,  "user_flag",            0,      0,      "match_cond:"),
SDATAPM (ASN_UNSIGNED,  "not_user_flag",        0,      0,      "match_cond:"),
SDATAPM (ASN_UNSIGNED,  "user_flag_mask_set",   0,      0,      "match_cond:"),
SDATAPM (ASN_UNSIGNED,  "user_flag_mask_notset",0,      0,      "match_cond:"),
SDATAPM (ASN_OCTET_STR, "key",                  0,      0,      "match_cond:"),
SDATAPM (ASN_OCTET_STR, "notkey",               0,      0,      "match_cond:"),
SDATAPM (ASN_OCTET_STR, "from_tm",              0,      0,      "match_cond:"),
SDATAPM (ASN_OCTET_STR, "to_tm",                0,      0,      "match_cond:"),
SDATAPM (ASN_OCTET_STR, "rkey",                 0,      0,      "match_cond: regular expression of key"),
SDATA_END()
};

PRIVATE sdata_desc_t pm_close_list[] = {
/*-PM----type-----------name--------------------flag----default-description---------- */
SDATAPM (ASN_OCTET_STR, "list_id",              0,      0,      "Id of list"),
SDATA_END()
};

PRIVATE sdata_desc_t pm_get_list_data[] = {
/*-PM----type-----------name--------------------flag----default-description---------- */
SDATAPM (ASN_OCTET_STR, "list_id",              0,      0,      "Id of list"),
SDATA_END()
};

PRIVATE const char *a_help[] = {"h", "?", 0};

PRIVATE sdata_desc_t command_table[] = {
/*-CMD---type-----------name----------------alias-------items-------json_fn---------description--*/
SDATACM (ASN_SCHEMA,    "help",             a_help,     pm_help,    cmd_help,       "Command's help"),

/*-CMD---type-----------name----------------alias---items---------------json_fn-------------description--*/
SDATACM (ASN_SCHEMA,    "print-tranger",    0,      pm_print_tranger,   cmd_print_tranger,  "Print tranger"),
SDATACM (ASN_SCHEMA,    "topics",           0,      0,                  cmd_topics,         "List topics"),
SDATACM (ASN_SCHEMA,    "desc",             0,      pm_desc,            cmd_desc,           "Schema of topic or full"),
SDATACM (ASN_SCHEMA,    "create-topic",     0,      pm_create_topic,    cmd_create_topic,   "Create topic"),
SDATACM (ASN_SCHEMA,    "delete-topic",     0,      pm_delete_topic,    cmd_delete_topic,   "Delete topic"),
SDATACM (ASN_SCHEMA,    "open-list",        0,      pm_open_list,       cmd_open_list,      "Open list"),
SDATACM (ASN_SCHEMA,    "close-list",       0,      pm_close_list,      cmd_close_list,     "Close list"),
SDATACM (ASN_SCHEMA,    "add-record",       0,      pm_add_record,      cmd_add_record,     "Add record"),
SDATACM (ASN_SCHEMA,    "get-list-data",    0,      pm_get_list_data,   cmd_get_list_data,   "Get list data"),
SDATA_END()
};

/*---------------------------------------------*
 *      Attributes - order affect to oid's
 *---------------------------------------------*/
PRIVATE sdata_desc_t tattr_desc[] = {
/*-ATTR-type------------name----------------flag----------------default---------description---------- */
SDATA (ASN_POINTER,     "tranger",          SDF_RD,             0,              "Tranger handler"),
SDATA (ASN_OCTET_STR,   "path",             SDF_RD|SDF_REQUIRED,"",             "If database exists then only needs (path,[database]) params"),
SDATA (ASN_OCTET_STR,   "database",         SDF_RD,             "",             "If null, path must contains the 'database'"),
SDATA (ASN_OCTET_STR,   "filename_mask",    SDF_RD|SDF_REQUIRED,"%Y-%m-%d",    "Organization of tables (file name format, see strftime())"),

SDATA (ASN_INTEGER,     "xpermission",      SDF_RD,             02770,          "Use in creation, default 02770"),
SDATA (ASN_INTEGER,     "rpermission",      SDF_RD,             0660,           "Use in creation, default 0660"),
SDATA (ASN_INTEGER,     "on_critical_error",SDF_RD,             LOG_OPT_EXIT_ZERO,"exit on error (Zero to avoid restart)"),
SDATA (ASN_BOOLEAN,     "master",           SDF_RD,             FALSE,          "the master is the only that can write"),
SDATA (ASN_INTEGER,     "timeout",          SDF_RD,             1*1000,         "Timeout"),
SDATA (ASN_COUNTER64,   "txMsgs",           SDF_RD|SDF_PSTATS,  0,              "Messages transmitted"),
SDATA (ASN_COUNTER64,   "rxMsgs",           SDF_RD|SDF_RSTATS,  0,              "Messages receiveds"),

SDATA (ASN_COUNTER64,   "txMsgsec",         SDF_RD|SDF_RSTATS,  0,              "Messages by second"),
SDATA (ASN_COUNTER64,   "rxMsgsec",         SDF_RD|SDF_RSTATS,  0,              "Messages by second"),
SDATA (ASN_COUNTER64,   "maxtxMsgsec",      SDF_WR|SDF_RSTATS,  0,              "Max Tx Messages by second"),
SDATA (ASN_COUNTER64,   "maxrxMsgsec",      SDF_WR|SDF_RSTATS,  0,              "Max Rx Messages by second"),
SDATA (ASN_POINTER,     "user_data",        0,                  0,              "user data"),
SDATA (ASN_POINTER,     "user_data2",       0,                  0,              "more user data"),
SDATA (ASN_POINTER,     "subscriber",       0,                  0,              "subscriber of output-events. Not a child gobj."),
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
 *      GClass permission levels
 *---------------------------------------------*/
enum {
    PERMISSION_XXXX = 0x00000001,
};
PRIVATE const permission_level_t s_user_permission_level[32] = {
{"xxxx",        "Sample"},
{0, 0},
};

/*---------------------------------------------*
 *              Private data
 *---------------------------------------------*/
typedef struct _PRIVATE_DATA {

    json_t *tranger;

    int32_t timeout;
    hgobj timer;
    uint64_t *ptxMsgs;
    uint64_t *prxMsgs;
    uint64_t txMsgsec;
    uint64_t rxMsgsec;
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
    priv->ptxMsgs = gobj_danger_attr_ptr(gobj, "txMsgs");
    priv->prxMsgs = gobj_danger_attr_ptr(gobj, "rxMsgs");

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

    IF_EQ_SET_PRIV(timeout,         gobj_read_int32_attr)
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

    json_t *jn_tranger = json_pack("{s:s, s:s, s:s, s:i, s:i, s:i, s:b}",
        "path", gobj_read_str_attr(gobj, "path"),
        "database", gobj_read_str_attr(gobj, "database"),
        "filename_mask", gobj_read_str_attr(gobj, "filename_mask"),
        "xpermission", (int)gobj_read_int32_attr(gobj, "xpermission"),
        "rpermission", (int)gobj_read_int32_attr(gobj, "rpermission"),
        "on_critical_error", (int)gobj_read_int32_attr(gobj, "on_critical_error"),
        "master", gobj_read_bool_attr(gobj, "master")
    );

    priv->tranger = tranger_startup(
        jn_tranger // owned
    );
    gobj_write_pointer_attr(gobj, "tranger", priv->tranger);

    /*
     *  Periodic timer for tasks
     */
    gobj_start(priv->timer);
    set_timeout_periodic(priv->timer, priv->timeout); // La verdadera

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

    EXEC_AND_RESET(tranger_shutdown, priv->tranger);
    gobj_write_pointer_attr(gobj, "tranger", 0);

    return 0;
}

/***************************************************************************
 *      Framework Method subscription_added
 ***************************************************************************/
PRIVATE int mt_subscription_added(
    hgobj gobj,
    hsdata subs)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    json_t *__config__ = sdata_read_json(subs, "__config__");
    BOOL first_shot = kw_get_bool(__config__, "__first_shot__", FALSE, 0);
    if(!first_shot) {
        return 0;
    }
    hgobj subscriber = sdata_read_pointer(subs, "subscriber");
    const char *event = sdata_read_str(subs, "event");
    if(empty_string(event)) {
        log_warning(0,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_INFO,
            "msg",          "%s", "tranger subscription must be with explicit event",
            "subscriber",   "%s", gobj_full_name(subscriber),
            NULL
        );
        return 0;
    }

    json_t *__global__ = sdata_read_json(subs, "__global__");
    json_t *__filter__ = sdata_read_json(subs, "__filter__");

    const char *event_name = sdata_read_str(subs, "renamed_event");

    const char *__list_id__ = kw_get_str(__config__, "__list_id__", "", 0);
    if(empty_string(__list_id__)) {
        return gobj_send_event(
            subscriber,
            (!empty_string(event_name))?event_name:event,
            msg_iev_build_webix2_without_answer_filter(gobj,
                -1,
                json_sprintf("__list_id__ required"),
                0,
                0, // owned
                __global__?kw_duplicate(__global__):0,  // owned
                "__first_shot__"
            ),
            gobj
        );
    }

    json_t *list = tranger_get_list(priv->tranger, __list_id__);
    if(!list) {
        return gobj_send_event(
            subscriber,
            (!empty_string(event_name))?event_name:event,
            msg_iev_build_webix2_without_answer_filter(gobj,
                -1,
                json_sprintf("tranger list not found: '%s'", __list_id__),
                0,
                0, // owned
                __global__?kw_duplicate(__global__):0,  // owned
                "__first_shot__"
            ),
            gobj
        );
    }

    if(strcasecmp(event, "EV_TRANGER_RECORD_ADDED")==0) {
        json_t *jn_data = json_array();
        size_t idx;
        json_t *jn_record;
        json_array_foreach(kw_get_list(list, "data", 0, KW_REQUIRED), idx, jn_record) {
            JSON_INCREF(__filter__);
            if(!kw_match_simple(jn_record, __filter__)) {
                continue;
            }
            json_array_append(jn_data, jn_record);
        }

        /*
         *  Inform
         */
        return gobj_send_event(
            subscriber,
            (!empty_string(event_name))?event_name:event,
            msg_iev_build_webix2_without_answer_filter(gobj,
                0,
                0,
                0,
                jn_data, // owned
                __global__?kw_duplicate(__global__):0,  // owned
                "__first_shot__"
            ),
            gobj
        );

    }

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
PRIVATE json_t *cmd_print_tranger(hgobj gobj, const char *cmd, json_t *kw, hgobj src)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    BOOL expanded = kw_get_bool(kw, "expanded", 0, KW_WILD_NUMBER);
    int lists_limit = kw_get_int(kw, "lists_limit", 100, KW_WILD_NUMBER);
    int dicts_limit = kw_get_int(kw, "dicts_limit", 100, KW_WILD_NUMBER);
    const char *path = kw_get_str(kw, "path", "", 0);

    json_t *value = priv->tranger;

    if(!empty_string(path)) {
        value = kw_find_path(value, path, FALSE);
        if(!value) {
            return msg_iev_build_webix(gobj,
                -1,
                json_sprintf("Path not found: '%s'", path),
                0,
                0,
                kw  // owned
            );
        }
    }

    if(expanded) {
        if(!lists_limit && !dicts_limit) {
            kw_incref(value); // All
        } else {
            value = kw_collapse(value, lists_limit, dicts_limit);
        }
    } else {
        value = kw_collapse(value, 0, 0);
    }

    /*
     *  Inform
     */
    return msg_iev_build_webix(gobj,
        0,
        0,
        0,
        value,
        kw  // owned
    );
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE json_t *cmd_create_topic(hgobj gobj, const char *cmd, json_t *kw, hgobj src)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    const char *topic_name = kw_get_str(kw, "topic_name", "", 0);
    if(empty_string(topic_name)) {
        return msg_iev_build_webix(
            gobj,
            -1,
            json_sprintf("What topic_name?"),
            0,
            0,
            kw  // owned
        );
    }
    const char *pkey = kw_get_str(kw, "pkey", "", 0);
    const char *tkey = kw_get_str(kw, "tkey", "", 0);
    const char *system_flag_ = kw_get_str(kw, "system_flag", "", 0);
    json_t *jn_cols = kw_get_dict(kw, "jn_cols", 0, 0);
    json_t *jn_var = kw_get_dict(kw, "jn_var", 0, 0);

    system_flag_t system_flag = tranger_str2system_flag(system_flag_);

    json_t *topic = tranger_create_topic( // WARNING returned json IS NOT YOURS, HACK IDEMPOTENT function
        priv->tranger,      // If topic exists then only needs (tranger, topic_name) parameters
        topic_name,
        pkey,
        tkey,
        system_flag,
        json_incref(jn_cols),    // owned
        json_incref(jn_var)      // owned
    );

    return msg_iev_build_webix(gobj,
        topic?0:-1,
        topic?json_sprintf("Topic created: '%s'", topic_name):json_string(log_last_message()),
        0,
        json_incref(topic),
        kw  // owned
    );
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE json_t *cmd_delete_topic(hgobj gobj, const char *cmd, json_t *kw, hgobj src)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    const char *topic_name = kw_get_str(kw, "topic_name", "", 0);
    if(empty_string(topic_name)) {
        return msg_iev_build_webix(
            gobj,
            -1,
            json_sprintf("What topic_name?"),
            0,
            0,
            kw  // owned
        );
    }
    BOOL force = kw_get_bool(kw, "force", 0, 0);
    json_t *topic = tranger_topic(priv->tranger, topic_name);
    if(!topic) {
        return msg_iev_build_webix(
            gobj,
            -1,
            json_sprintf("Topic not found: '%s'", topic_name),
            0,
            0,
            kw  // owned
        );
    }

    json_int_t topic_size = tranger_topic_size(topic);
    if(topic_size != 0) {
        if(!force) {
            return msg_iev_build_webix(
                gobj,
                -1,
                json_sprintf("'%s' topic with records, you must force to delete", topic_name),
                0,
                0,
                kw  // owned
            );
        }
    }

    int ret = tranger_delete_topic(priv->tranger, topic_name);

    return msg_iev_build_webix(gobj,
        ret,
        ret>=0?json_sprintf("Topic deleted: '%s'", topic_name):json_string(log_last_message()),
        0,
        0,
        kw  // owned
    );
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE json_t *cmd_topics(hgobj gobj, const char *cmd, json_t *kw, hgobj src)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    json_t *topics = kw_get_dict(priv->tranger, "topics", 0, KW_REQUIRED);
    json_t *topic_list = json_array();

    const char *topic_name; json_t *topic;
    json_object_foreach(topics, topic_name, topic) {
        if(!json_is_object(topic)) {
            continue;
        }
        json_array_append_new(topic_list, json_string(topic_name));
    }

    return msg_iev_build_webix(gobj,
        topics?0:-1,
        topics?0:json_string(log_last_message()),
        0,
        topic_list,
        kw  // owned
    );
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE json_t *cmd_desc(hgobj gobj, const char *cmd, json_t *kw, hgobj src)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    const char *topic_name = kw_get_str(kw, "topic_name", "", 0);
    if(empty_string(topic_name)) {
        return msg_iev_build_webix(
            gobj,
            -1,
            json_sprintf("What topic_name?"),
            0,
            0,
            kw  // owned
        );
    }

    json_t *topic = tranger_topic(priv->tranger, topic_name);
    if(!topic) {
        return msg_iev_build_webix(
            gobj,
            -1,
            json_sprintf("Topic not found: '%s'", topic_name),
            0,
            0,
            kw  // owned
        );
    }
    json_t *desc = kwid_new_dict("", priv->tranger, "topics`%s`cols", topic_name);

    return msg_iev_build_webix(gobj,
        desc?0:-1,
        desc?0:json_string(log_last_message()),
        0,
        desc,
        kw  // owned
    );
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE json_t *cmd_open_list(hgobj gobj, const char *cmd, json_t *kw, hgobj src)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    const char *list_id = kw_get_str(kw, "list_id", "", 0);
    BOOL return_data = kw_get_bool(kw, "return_data", 0, 0);

    const char *topic_name = kw_get_str(kw, "topic_name", "", 0);

    if(empty_string(list_id)) {
        return msg_iev_build_webix(
            gobj,
            -1,
            json_sprintf("What list_id?"),
            0,
            0,
            kw  // owned
        );
    }

    if(empty_string(topic_name)) {
        return msg_iev_build_webix(
            gobj,
            -1,
            json_sprintf("What topic_name?"),
            0,
            0,
            kw  // owned
        );
    }
    json_t *topic = tranger_topic(priv->tranger, topic_name);
    if(!topic) {
        return msg_iev_build_webix(
            gobj,
            -1,
            json_sprintf("Topic not found: '%s'", topic_name),
            0,
            0,
            kw  // owned
        );
    }

    json_t *list = tranger_get_list(priv->tranger, list_id);
    if(list) {
        return msg_iev_build_webix(
            gobj,
            0,
            json_sprintf("List is already open: '%s'", list_id),
            0,
            0,
            kw  // owned
        );
    }

    BOOL  backward = kw_get_bool(kw, "backward", 0, 0);
    BOOL  only_md = kw_get_bool(kw, "only_md", 0, 0);
    int64_t from_rowid = (int64_t)kw_get_int(kw, "from_rowid", 0, 0);
    int64_t to_rowid = (int64_t)kw_get_int(kw, "to_rowid", 0, 0);
    uint32_t user_flag = (uint32_t)kw_get_int(kw, "user_flag", 0, 0);
    uint32_t not_user_flag = (uint32_t)kw_get_int(kw, "not_user_flag", 0, 0);
    uint32_t user_flag_mask_set = (uint32_t)kw_get_int(kw, "user_flag_mask_set", 0, 0);
    uint32_t user_flag_mask_notset = (uint32_t)kw_get_int(kw, "user_flag_mask_notset", 0, 0);
    const char *key = kw_get_str(kw, "key", 0, 0);
    const char *notkey = kw_get_str(kw, "notkey", 0, 0);
    const char *rkey = kw_get_str(kw, "rkey", 0, 0);
    const char *from_t = kw_get_str(kw, "from_t", 0, 0);
    const char *to_t = kw_get_str(kw, "to_t", 0, 0);
    const char *from_tm = kw_get_str(kw, "from_tm", 0, 0);
    const char *to_tm = kw_get_str(kw, "to_tm", 0, 0);
    const char *fields = kw_get_str(kw, "fields", 0, 0);

    json_t *match_cond = json_pack("{s:b, s:b}",
        "backward", backward,
        "only_md", only_md
    );
    if(from_rowid) {
        json_object_set_new(match_cond, "from_rowid", json_integer(from_rowid));
    }
    if(to_rowid) {
        json_object_set_new(match_cond, "to_rowid", json_integer(to_rowid));
    }
    if(user_flag) {
        json_object_set_new(match_cond, "user_flag", json_integer(user_flag));
    }
    if(not_user_flag) {
        json_object_set_new(match_cond, "not_user_flag", json_integer(not_user_flag));
    }
    if(user_flag_mask_set) {
        json_object_set_new(match_cond, "user_flag_mask_set", json_integer(user_flag_mask_set));
    }
    if(user_flag_mask_notset) {
        json_object_set_new(match_cond, "user_flag_mask_notset", json_integer(user_flag_mask_notset));
    }
    if(key) {
        json_object_set_new(match_cond, "key", json_string(key));
    }
    if(notkey) {
        json_object_set_new(match_cond, "notkey", json_string(notkey));
    }
    if(rkey) {
        json_object_set_new(match_cond, "rkey", json_string(rkey));
    }
    if(from_t) {
        timestamp_t timestamp;
        int offset;
        if(all_numbers(from_t)) {
            timestamp = atoll(from_t);
        } else {
            parse_date_basic(from_t, &timestamp, &offset);
        }
        json_object_set_new(match_cond, "from_t", json_integer(timestamp));
    }
    if(to_t) {
        timestamp_t timestamp;
        int offset;
        if(all_numbers(to_t)) {
            timestamp = atoll(to_t);
        } else {
            parse_date_basic(to_t, &timestamp, &offset);
        }
        json_object_set_new(match_cond, "to_t", json_integer(timestamp));
    }
    if(from_tm) {
        timestamp_t timestamp;
        int offset;
        if(all_numbers(from_tm)) {
            timestamp = atoll(from_tm);
        } else {
            parse_date_basic(from_tm, &timestamp, &offset);
        }
        json_object_set_new(match_cond, "from_tm", json_integer(timestamp));
    }
    if(to_tm) {
        timestamp_t timestamp;
        int offset;
        if(all_numbers(to_tm)) {
            timestamp = atoll(to_tm);
        } else {
            parse_date_basic(to_tm, &timestamp, &offset);
        }
        json_object_set_new(match_cond, "to_tm", json_integer(timestamp));
    }
    if(!empty_string(fields)) {
        json_object_set_new(
            match_cond,
            "fields",
            json_string(fields)
        );
    }

    json_t *jn_list = json_pack("{s:s, s:s, s:o, s:I, s:I}",
        "id", list_id,
        "topic_name", topic_name,
        "match_cond", match_cond,
        "load_record_callback", (json_int_t)(size_t)load_record_callback,
        "gobj", (json_int_t)(size_t)gobj
    );

    list = tranger_open_list(priv->tranger, jn_list);

    return msg_iev_build_webix(
        gobj,
        list?0:-1,
        list?json_sprintf("List opened: '%s'", list_id):json_string(log_last_message()),
        0,
        return_data?json_incref(kw_get_list(list, "data", 0, KW_REQUIRED)):0,
        kw  // owned
    );
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE json_t *cmd_close_list(hgobj gobj, const char *cmd, json_t *kw, hgobj src)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    const char *list_id = kw_get_str(kw, "list_id", "", 0);

    if(empty_string(list_id)) {
        return msg_iev_build_webix(
            gobj,
            -1,
            json_sprintf("What list_id?"),
            0,
            0,
            kw  // owned
        );
    }

    json_t *list = tranger_get_list(priv->tranger, list_id);
    if(!list) {
        return msg_iev_build_webix(
            gobj,
            0,
            json_sprintf("List not found: '%s'", list_id),
            0,
            0,
            kw  // owned
        );
    }

    int result = tranger_close_list(priv->tranger, list);

    return msg_iev_build_webix(
        gobj,
        result,
        result>=0?json_sprintf("List closed: '%s'", list_id):json_string(log_last_message()),
        0,
        0,
        kw  // owned
    );
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE json_t *cmd_add_record(hgobj gobj, const char *cmd, json_t *kw, hgobj src)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    int result = 0;
    json_t *jn_comment = 0;

    //json_t *access_roles = kw_get_dict(jwt_payload, "access_roles", 0, KW_REQUIRED);
    //json_t *fichajes_roles = kw_get_list(access_roles, "fichajes", 0, 0);

    do {
        //if(!fichajes_roles || !json_str_in_list(fichajes_roles, "user", FALSE)) {
        //    jn_data = kw_incref(kw);
        //    jn_comment = json_string("User has not 'user' role");
        //    result = -1;
        //    break;
        //}

        /*
         *  Get parameters
         */
        const char *topic_name = kw_get_str(kw, "topic_name", "", 0);
        uint64_t __t__ = kw_get_int(kw, "__t__", 0, 0);
        uint32_t user_flag = kw_get_int(kw, "user_flag", 0, 0);
        json_t *record = kw_get_dict(kw, "record", 0, 0);

        /*
         *  Check parameters
         */
        if(empty_string(topic_name)) {
           jn_comment = json_sprintf("What topic_name?");
           result = -1;
           break;
        }
        json_t *topic = tranger_topic(priv->tranger, topic_name);
        if(!topic) {
           jn_comment = json_sprintf("Topic not found: '%s'", topic_name);
           result = -1;
           break;
        }
        if(!record) {
           jn_comment = json_sprintf("What record?");
           result = -1;
           break;
        }

        /*
         *  Append record to tranger topic
         */
        md_record_t md_record;
        result = tranger_append_record(
            priv->tranger,
            topic_name,
            __t__,                  // if 0 then the time will be set by TimeRanger with now time
            user_flag,
            &md_record,             // required
            json_incref(record)     // owned
        );

        if(result<0) {
            jn_comment = json_string(log_last_message());
            break;
        } else {
           jn_comment = json_sprintf("Record added");
        }
    } while(0);

    /*
     *  Response
     */
    return msg_iev_build_webix(
        gobj,
        result,
        jn_comment,
        0,
        0,
        kw  // owned
    );
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE json_t *cmd_get_list_data(hgobj gobj, const char *cmd, json_t *kw, hgobj src)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    const char *list_id = kw_get_str(kw, "list_id", "", 0);

    if(empty_string(list_id)) {
        return msg_iev_build_webix(
            gobj,
            -1,
            json_sprintf("What list_id?"),
            0,
            0,
            kw  // owned
        );
    }

    json_t *list = tranger_get_list(priv->tranger, list_id);
    if(!list) {
        return msg_iev_build_webix(
            gobj,
            0,
            json_sprintf("List not found: '%s'", list_id),
            0,
            0,
            kw  // owned
        );
    }

    return msg_iev_build_webix(
        gobj,
        0,
        0,
        0,
        json_incref(kw_get_list(list, "data", 0, KW_REQUIRED)),
        kw  // owned
    );
}




            /***************************
             *      Local Methods
             ***************************/




            /***************************
             *      Private Methods
             ***************************/




/***************************************************************************
 *
 ***************************************************************************/
PRIVATE int load_record_callback(
    json_t *tranger,
    json_t *topic,
    json_t *list,
    md_record_t *md_record,
    json_t *jn_record  // owned
)
{
    json_t *match_cond = kw_get_dict(list, "match_cond", 0, KW_REQUIRED);
    BOOL only_md = kw_get_bool(match_cond, "only_md", 0, 0);
    BOOL has_fields = kw_has_key(match_cond, "fields");
    if(has_fields) {
        only_md = FALSE;
    }

    if(jn_record) {
        json_object_set_new(jn_record, "__md_tranger__", tranger_md2json(md_record));
    } else {
        if(only_md) {
            jn_record = json_object();
        } else {
            jn_record = tranger_read_record_content(tranger, topic, md_record);
        }
        json_object_set_new(jn_record, "__md_tranger__", tranger_md2json(md_record));
    }

    json_t *list_data = kw_get_list(list, "data", 0, KW_REQUIRED);

    const char ** keys = 0;
    if(has_fields) {
        const char *fields = kw_get_str(match_cond, "fields", "", 0);
        keys = split2(fields, ", ", 0);
    }
    if(keys) {
        json_t *jn_record_with_fields = kw_clone_by_path(
            jn_record,   // owned
            keys
        );
        jn_record = jn_record_with_fields;
        json_array_append(
            list_data,
            jn_record
        );
        split_free2(keys);
    } else {
        json_array_append(
            list_data,
            jn_record
        );
    }

    if(!(md_record->__system_flag__ & sf_loading_from_disk)) {
        json_t *jn_data = json_array();

        if(only_md) {
            json_array_append(jn_data, kw_get_dict(jn_record, "__md_tranger__", 0, KW_REQUIRED));
        } else {
            json_array_append(jn_data, jn_record);
        }

        hgobj gobj = (hgobj)kw_get_int(list, "gobj", 0, KW_REQUIRED);
        gobj_publish_event(gobj, "EV_TRANGER_RECORD_ADDED", jn_data);
    }

    JSON_DECREF(jn_record);

    return 0; // HACK lest timeranger to add record to list.data
}




            /***************************
             *      Actions
             ***************************/




/***************************************************************************
 *
 ***************************************************************************/
PRIVATE int ac_tranger_add_record(hgobj gobj, const char *event, json_t *kw, hgobj src)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    (*priv->prxMsgs)++;
    priv->rxMsgsec++;

    json_t *__temp__ = kw_get_dict_value(kw, "__temp__", 0, KW_REQUIRED);
    JSON_INCREF(__temp__); // Save to __answer__

    /*
     *  Get user from jwt_payload TODO
     */
    //hgobj channel_gobj = (hgobj)(size_t)kw_get_int(kw, "__temp__`channel_gobj", 0, KW_REQUIRED);
    //json_t *jwt_payload = gobj_read_user_data(channel_gobj, "jwt_payload");

    int result = 0;
    json_t *jn_comment = 0;

    //json_t *access_roles = kw_get_dict(jwt_payload, "access_roles", 0, KW_REQUIRED);
    //json_t *fichajes_roles = kw_get_list(access_roles, "fichajes", 0, 0);

    do {
        //if(!fichajes_roles || !json_str_in_list(fichajes_roles, "user", FALSE)) {
        //    jn_data = kw_incref(kw);
        //    jn_comment = json_string("User has not 'user' role");
        //    result = -1;
        //    break;
        //}

        /*
         *  Get parameters
         */
        const char *topic_name = kw_get_str(kw, "topic_name", "", 0);
        uint64_t __t__ = kw_get_int(kw, "__t__", 0, 0);
        uint32_t user_flag = kw_get_int(kw, "user_flag", 0, 0);
        json_t *record = kw_get_dict(kw, "record", 0, 0);

        /*
         *  Check parameters
         */
        json_t *topic = tranger_topic(priv->tranger, topic_name);
        if(!topic) {
           jn_comment = json_sprintf("Topic not found: '%s'", topic_name);
           result = -1;
           break;
        }
        if(!record) {
           jn_comment = json_sprintf("What record?");
           result = -1;
           break;
        }

        /*
         *  Append record to tranger topic
         */
        md_record_t md_record;
        result = tranger_append_record(
            priv->tranger,
            topic_name,
            __t__,                  // if 0 then the time will be set by TimeRanger with now time
            user_flag,
            &md_record,             // required
            json_incref(record)     // owned
        );

        if(result<0) {
            jn_comment = json_string(log_last_message());
            break;
        } else {
           jn_comment = json_sprintf("Record added");
        }
    } while(0);

    /*
     *  Response
     */
    json_t *iev = iev_create(
        event,
        msg_iev_build_webix2(gobj,
            result,
            jn_comment,
            0,
            0,  // owned
            kw,  // owned
            "__answer__"
        )
    );
    json_object_set_new(iev, "__temp__", __temp__);  // Set the channel

    /*
     *  Inform
     */
    return gobj_send_event(
        src,
        "EV_SEND_IEV",
        iev,
        gobj
    );
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE int ac_timeout(hgobj gobj, const char *event, json_t *kw, hgobj src)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    uint64_t maxtxMsgsec = gobj_read_uint64_attr(gobj, "maxtxMsgsec");
    uint64_t maxrxMsgsec = gobj_read_uint64_attr(gobj, "maxrxMsgsec");
    if(priv->txMsgsec > maxtxMsgsec) {
        gobj_write_uint64_attr(gobj, "maxtxMsgsec", priv->txMsgsec);
    }
    if(priv->rxMsgsec > maxrxMsgsec) {
        gobj_write_uint64_attr(gobj, "maxrxMsgsec", priv->rxMsgsec);
    }

    gobj_write_uint64_attr(gobj, "txMsgsec", priv->txMsgsec);
    gobj_write_uint64_attr(gobj, "rxMsgsec", priv->rxMsgsec);

    priv->rxMsgsec = 0;
    priv->txMsgsec = 0;

    KW_DECREF(kw);
    return 0;
}


/***************************************************************************
 *                          FSM
 ***************************************************************************/
PRIVATE const EVENT input_events[] = {
    {"EV_TRANGER_ADD_RECORD",       EVF_PUBLIC_EVENT,   0,  0},
    // bottom input
    {"EV_TIMEOUT",              0,  0,  0},
    {"EV_STOPPED",              0,  0,  0},
    {NULL, 0, 0, 0}
};
PRIVATE const EVENT output_events[] = {
    {"EV_TRANGER_RECORD_ADDED",     EVF_PUBLIC_EVENT,   0,  0},
    {NULL, 0, 0, 0}
};
PRIVATE const char *state_names[] = {
    "ST_IDLE",
    NULL
};

PRIVATE EV_ACTION ST_IDLE[] = {
    {"EV_TRANGER_ADD_RECORD",       ac_tranger_add_record,      0},
    {"EV_TIMEOUT",                  ac_timeout,                 0},
    {"EV_STOPPED",                  0,                          0},
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
 *              GClass
 *---------------------------------------------*/
PRIVATE GCLASS _gclass = {
    0,  // base
    GCLASS_TRANGER_NAME,
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
        mt_subscription_added,
        0, //mt_subscription_deleted,
        0, //mt_child_added,
        0, //mt_child_removed,
        0,
        0, //mt_command,
        0, //mt_inject_event,
        0, //mt_create_resource,
        0, //mt_list_resource,
        0, //mt_update_resource,
        0, //mt_delete_resource,
        0, //mt_add_child_resource_link,
        0, //mt_delete_child_resource_link,
        0, //mt_get_resource,
        0, //mt_future24,
        0, //mt_authenticate,
        0, //mt_list_childs,
        0, //mt_stats_updated,
        0, //mt_disable,
        0, //mt_enable,
        0, //mt_trace_on,
        0, //mt_trace_off,
        0, //mt_gobj_created,
        0, //mt_permission_on,
        0, //mt_permission_off,
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
    0,  // lmt
    tattr_desc,
    sizeof(PRIVATE_DATA),
    s_user_permission_level,  // acl
    s_user_trace_level,
    command_table,  // command_table
    0,  // gcflag
};

/***************************************************************************
 *              Public access
 ***************************************************************************/
PUBLIC GCLASS *gclass_tranger(void)
{
    return &_gclass;
}
