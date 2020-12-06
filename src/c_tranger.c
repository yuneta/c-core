/***********************************************************************
 *          C_TRANGER.C
 *          Tranger GClass.
 *
 *          Trangers: resources with tranger
 *
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
PRIVATE json_t *cmd_open_list(hgobj gobj, const char *cmd, json_t *kw, hgobj src);

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
PRIVATE sdata_desc_t pm_open_list[] = {
/*-PM----type-----------name--------------------flag----default-description---------- */
SDATAPM (ASN_OCTET_STR, "id",                   0,      0,      "Id of list"),
SDATAPM (ASN_BOOLEAN,   "return_data",          0,      0,      "True for return list data"),
SDATAPM (ASN_OCTET_STR, "topic_name",           0,      0,      "Topic name"),
SDATAPM (ASN_OCTET_STR, "fields",               0,      0,      "match_cond: Only this fields"),
SDATAPM (ASN_BOOLEAN,   "backward",             0,      0,      "match_cond:"),
SDATAPM (ASN_BOOLEAN,   "only_md",              0,      0,      "match_cond: don't load jn_record on loading disk, by default TRUE"),
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

PRIVATE const char *a_help[] = {"h", "?", 0};

PRIVATE sdata_desc_t command_table[] = {
/*-CMD---type-----------name----------------alias-------items-------json_fn---------description--*/
SDATACM (ASN_SCHEMA,    "help",             a_help,     pm_help,    cmd_help,       "Command's help"),

/*-CMD---type-----------name----------------alias---items---------------json_fn-------------description--*/
SDATACM (ASN_SCHEMA,    "print-tranger",    0,      pm_print_tranger,   cmd_print_tranger,  "Print tranger"),
SDATACM (ASN_SCHEMA,    "topics",           0,      0,                  cmd_topics,         "List topics"),
SDATACM (ASN_SCHEMA,    "desc",             0,      pm_desc,            cmd_desc,           "Schema of topic or full"),
SDATACM (ASN_SCHEMA,    "open-list",        0,      pm_open_list,       cmd_open_list,      "Open list"),
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
 *              Private data
 *---------------------------------------------*/
typedef struct _PRIVATE_DATA {
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
    //SET_PRIV(xxx,                   gobj_read_pointer_attr)
}

/***************************************************************************
 *      Framework Method writing
 ***************************************************************************/
PRIVATE void mt_writing(hgobj gobj, const char *path)
{
//     PRIVATE_DATA *priv = gobj_priv_data(gobj);
//
//     IF_EQ_SET_PRIV(xxx,             gobj_read_pointer_attr)
//     END_EQ_SET_PRIV()
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

    return 0;
}

/***************************************************************************
 *      Framework Method stop
 ***************************************************************************/
PRIVATE int mt_stop(hgobj gobj)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);
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
    BOOL first_shot = kw_get_bool(__config__, "__first_shot__", TRUE, 0);
    if(!first_shot) {
        return 0;
    }
    hgobj subscriber = sdata_read_pointer(subs, "subscriber");
    const char *event = sdata_read_str(subs, "event");
    json_t *__global__ = sdata_read_json(subs, "__global__");
    json_t *__filter__ = sdata_read_json(subs, "__filter__");

    const char *__list_id__ = kw_get_str(__config__, "__list_id__", "", 0);
    if(empty_string(__list_id__)) {
        return gobj_send_event(
            subscriber,
            event,
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
            event,
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

    if(strcasecmp(event, "EV_TRANGER_NEW_RECORD")==0) {
        json_t *match_cond = kw_get_dict(list, "match_cond", 0, KW_REQUIRED);
        const char ** keys = 0;
        if(kw_has_key(match_cond, "fields")) {
            const char *fields = kw_get_str(match_cond, "fields", "", 0);
            keys = split2(fields, ", ", 0);
        }

        json_t *jn_data = json_array();
        size_t idx;
        json_t *jn_record;
        json_array_foreach(kw_get_list(list, "data", 0, KW_REQUIRED), idx, jn_record) {
            if(__filter__) {
                JSON_INCREF(__filter__);
                if(!kw_match_simple(jn_record, __filter__)) {
                    continue;
                }
                if(keys) {
                    json_t *jn_record_with_fields = kw_clone_by_path(
                        jn_record,   // owned
                        keys
                    );
                    jn_record = jn_record_with_fields;
                }
                json_array_append(jn_data, jn_record);
            }
        }

        split_free2(keys);

        /*
         *  Inform
         */
        return gobj_send_event(
            subscriber,
            event,
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

    const char *id = kw_get_str(kw, "id", "", 0);
    BOOL return_data = kw_get_bool(kw, "return_data", 0, 0);

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
    if(empty_string(id)) {
        return msg_iev_build_webix(
            gobj,
            -1,
            json_sprintf("An id to the list is required"),
            0,
            0,
            kw  // owned
        );
    }

    json_t *list = tranger_get_list(priv->tranger, id);
    if(list) {
        return msg_iev_build_webix(
            gobj,
            0,
            json_sprintf("List '%s' is already open", id),
            0,
            0,
            kw  // owned
        );
    }

    BOOL  backward = kw_get_bool(kw, "backward", 0, 0);
    BOOL  only_md = kw_get_bool(kw, "only_md", TRUE, 0); // WARNING by default only_md
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
        "id", id,
        "topic_name", topic_name,
        "match_cond", match_cond,
        "load_record_callback", (json_int_t)(size_t)load_record_callback,
        "gobj", (json_int_t)(size_t)gobj
    );

    list = tranger_open_list(priv->tranger, jn_list);

    return msg_iev_build_webix(
        gobj,
        list?0:-1,
        list?json_sprintf("List '%s' open", id):json_string(log_last_message()),
        0,
        return_data?kw_get_list(list, "data", 0, KW_REQUIRED):0,
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
    if(!(md_record->__system_flag__ & sf_loading_from_disk)) {
        json_t *match_cond = kw_get_dict(list, "match_cond", 0, KW_REQUIRED);
        if(kw_has_key(match_cond, "fields")) {
            if(!jn_record) {
                jn_record = tranger_read_record_content(tranger, topic, md_record);
            }
            const char *fields = kw_get_str(match_cond, "fields", "", 0);
            const char ** keys = 0;
            keys = split2(fields, ", ", 0);
            json_t *jn_record_with_fields = kw_clone_by_path(
                jn_record,   // owned
                keys
            );
            split_free2(keys);
            jn_record = jn_record_with_fields;
        }

        hgobj gobj = (hgobj)kw_get_int(list, "gobj", 0, KW_REQUIRED);
        json_t *jn_data_ = json_array();
        json_t *jn_data = json_object();
        json_array_append_new(jn_data_, jn_data);
        json_object_set_new(
            jn_data,
            "__list_id__",
            json_string(kw_get_str(list, "id", "", KW_REQUIRED))
        );
        json_object_update(jn_data, jn_record);
        gobj_publish_event(gobj, "EV_TRANGER_NEW_RECORD", jn_data_);
    }

    JSON_DECREF(jn_record);

    return 1; // HACK add record to list.data
}




            /***************************
             *      Actions
             ***************************/




/***************************************************************************
 *                          FSM
 ***************************************************************************/
PRIVATE const EVENT input_events[] = {
    {NULL, 0, 0, 0}
};
PRIVATE const EVENT output_events[] = {
    {"EV_TRANGER_NEW_RECORD",  EVF_PUBLIC_EVENT,   0,  0},
    {NULL, 0, 0, 0}
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
    0,  // lmt
    tattr_desc,
    sizeof(PRIVATE_DATA),
    0,  // acl
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
