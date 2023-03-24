/***********************************************************************
 *          YUNO_DEFAULT.C
 *          Container of yunos GClass.
 *          Supplies the main event loop in a process context.
 *
 *          Copyright (c) 2013-2015 Niyamaka.
 *          All Rights Reserved.
 ***********************************************************************/
#include <libintl.h>
#include <locale.h>
#include <unistd.h>
#include <syslog.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>
#include <sys/statvfs.h>
#include <sys/utsname.h>
#include "c_yuno.h"

/***************************************************************************
 *              Constants
 ***************************************************************************/

/***************************************************************************
 *              Structures
 ***************************************************************************/

/***************************************************************************
 *              Prototypes
 ***************************************************************************/
PRIVATE int save_pid_in_file(hgobj gobj);
PRIVATE int set_user_gclass_traces(hgobj gobj);
PRIVATE int set_user_gclass_no_traces(hgobj gobj);
PRIVATE int set_user_gobj_traces(hgobj gobj);
PRIVATE int set_user_gobj_no_traces(hgobj gobj);

PRIVATE int save_global_trace(
    hgobj gobj,
    const char *level,
    BOOL set,
    BOOL persistent
);
PRIVATE int save_trace_filters(
    hgobj gobj
);
PRIVATE int save_user_trace(
    hgobj gobj,
    const char *name,
    const char *level,
    BOOL set,
    BOOL persistent
);
PRIVATE int save_user_no_trace(
    hgobj gobj,
    const char *name,
    const char *level,
    BOOL set,
    BOOL persistent
);
PRIVATE void inform_cb(int priority, uint32_t count, void *user_data);

/***************************************************************************
 *          Data: config, public data, private data
 ***************************************************************************/
PRIVATE int atexit_registered = 0; /* Register atexit just 1 time. */
PRIVATE char pidfile[NAME_MAX] = {0};

PRIVATE json_t *cmd_help(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
PRIVATE json_t *cmd_authzs(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
PRIVATE json_t *cmd_view_config(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
PRIVATE json_t *cmd_view_mem(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
PRIVATE json_t *cmd_view_mem2(hgobj gobj, const char *cmd, json_t *kw, hgobj src);

PRIVATE json_t *cmd_view_gclass(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
PRIVATE json_t *cmd_view_gobj(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
PRIVATE json_t *cmd_public_attrs(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
PRIVATE json_t *cmd_view_attrs(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
PRIVATE json_t *cmd_view_attrs2(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
PRIVATE json_t *cmd_webix_gobj_tree(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
PRIVATE json_t *cmd_view_gobj_tree(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
PRIVATE json_t *cmd_view_gobj_treedb(hgobj gobj, const char *cmd, json_t *kw, hgobj src);

PRIVATE json_t *cmd_view_gclass_register(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
PRIVATE json_t *cmd_view_service_register(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
PRIVATE json_t *cmd_view_unique_register(hgobj gobj, const char *cmd, json_t *kw, hgobj src);

PRIVATE json_t *cmd_list_childs(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
PRIVATE json_t *cmd_list_channels(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
PRIVATE json_t *cmd_write_bool(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
PRIVATE json_t *cmd_write_str(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
PRIVATE json_t *cmd_write_json(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
PRIVATE json_t *cmd_write_num(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
PRIVATE json_t *cmd_read_num(hgobj gobj, const char *cmd, json_t *kw, hgobj src);

PRIVATE json_t *cmd_info_cpus(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
PRIVATE json_t *cmd_info_ifs(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
PRIVATE json_t *cmd_info_os(hgobj gobj, const char *cmd, json_t *kw, hgobj src);

PRIVATE json_t *cmd_info_global_trace(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
PRIVATE json_t *cmd_get_global_trace(hgobj gobj, const char *cmd, json_t *kw, hgobj src);

PRIVATE json_t *cmd_info_gclass_trace(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
PRIVATE json_t *cmd_get_gclass_trace(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
PRIVATE json_t *cmd_get_gclass_no_trace(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
PRIVATE json_t *cmd_info_gobj_trace(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
PRIVATE json_t *cmd_get_gobj_trace(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
PRIVATE json_t *cmd_get_gobj_no_trace(hgobj gobj, const char *cmd, json_t *kw, hgobj src);

PRIVATE json_t *cmd_add_trace_filter(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
PRIVATE json_t *cmd_remove_trace_filter(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
PRIVATE json_t *cmd_get_trace_filter(hgobj gobj, const char *cmd, json_t *kw, hgobj src);

PRIVATE json_t *cmd_set_global_trace(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
PRIVATE json_t *cmd_set_gclass_trace(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
PRIVATE json_t *cmd_set_no_gclass_trace(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
PRIVATE json_t *cmd_set_gobj_trace(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
PRIVATE json_t *cmd_reset_all_traces(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
PRIVATE json_t *cmd_set_no_gobj_trace(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
PRIVATE json_t *cmd_set_daemon_debug(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
PRIVATE json_t *cmd_set_deep_trace(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
PRIVATE json_t *cmd_spawn(hgobj gobj, const char *cmd, json_t *kw, hgobj src);

PRIVATE json_t *cmd_trunk_rotatory_file(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
PRIVATE json_t *cmd_reset_log_counters(hgobj gobj, const char *cmd, json_t *kw, hgobj src);
PRIVATE json_t* cmd_add_log_handler(hgobj gobj, const char* cmd, json_t* kw, hgobj src);
PRIVATE json_t* cmd_del_log_handler(hgobj gobj, const char* cmd, json_t* kw, hgobj src);
PRIVATE json_t* cmd_list_log_handler(hgobj gobj, const char* cmd, json_t* kw, hgobj src);
PRIVATE json_t* cmd_list_persistent_attrs(hgobj gobj, const char* cmd, json_t* kw, hgobj src);
PRIVATE json_t* cmd_remove_persistent_attrs(hgobj gobj, const char* cmd, json_t* kw, hgobj src);
PRIVATE json_t* cmd_start_gobj_tree(hgobj gobj, const char* cmd, json_t* kw, hgobj src);
PRIVATE json_t* cmd_stop_gobj_tree(hgobj gobj, const char* cmd, json_t* kw, hgobj src);
PRIVATE json_t* cmd_enable_gobj(hgobj gobj, const char* cmd, json_t* kw, hgobj src);
PRIVATE json_t* cmd_disable_gobj(hgobj gobj, const char* cmd, json_t* kw, hgobj src);
PRIVATE json_t* cmd_subscribe_event(hgobj gobj, const char* cmd, json_t* kw, hgobj src);
PRIVATE json_t* cmd_unsubscribe_event(hgobj gobj, const char* cmd, json_t* kw, hgobj src);
PRIVATE json_t* cmd_list_subscriptions(hgobj gobj, const char* cmd, json_t* kw, hgobj src);
PRIVATE json_t* cmd_list_subscribings(hgobj gobj, const char* cmd, json_t* kw, hgobj src);
PRIVATE json_t* cmd_list_allowed_ips(hgobj gobj, const char* cmd, json_t* kw, hgobj src);
PRIVATE json_t* cmd_add_allowed_ip(hgobj gobj, const char* cmd, json_t* kw, hgobj src);
PRIVATE json_t* cmd_remove_allowed_ip(hgobj gobj, const char* cmd, json_t* kw, hgobj src);
PRIVATE json_t* cmd_list_denied_ips(hgobj gobj, const char* cmd, json_t* kw, hgobj src);
PRIVATE json_t* cmd_add_denied_ip(hgobj gobj, const char* cmd, json_t* kw, hgobj src);
PRIVATE json_t* cmd_remove_denied_ip(hgobj gobj, const char* cmd, json_t* kw, hgobj src);
PRIVATE json_t* cmd_2key_get_schema(hgobj gobj, const char* cmd, json_t* kw, hgobj src);
PRIVATE json_t* cmd_2key_get_value(hgobj gobj, const char* cmd, json_t* kw, hgobj src);
PRIVATE json_t* cmd_2key_get_subvalue(hgobj gobj, const char* cmd, json_t* kw, hgobj src);
PRIVATE json_t* cmd_system_topic_schema(hgobj gobj, const char* cmd, json_t* kw, hgobj src);
PRIVATE json_t* cmd_global_variables(hgobj gobj, const char* cmd, json_t* kw, hgobj src);

PRIVATE sdata_desc_t pm_help[] = {
/*-PM----type-----------name------------flag------------default-----description---------- */
SDATAPM (ASN_OCTET_STR, "cmd",          0,              0,          "command about you want help"),
SDATAPM (ASN_UNSIGNED,  "level",        0,              0,          "command search level in childs"),
SDATA_END()
};
PRIVATE sdata_desc_t pm_authzs[] = {
/*-PM----type-----------name------------flag------------default-----description---------- */
SDATAPM (ASN_OCTET_STR, "authz",        0,              0,          "permission to search"),
SDATAPM (ASN_OCTET_STR, "service",      0,              0,          "Service where to search the permission. If empty print all service's permissions"),
SDATA_END()
};

PRIVATE sdata_desc_t pm_gclass_name[] = {
/*-PM----type-----------name------------flag------------default-----description---------- */
SDATAPM (ASN_OCTET_STR, "gclass_name",  0,              0,          "gclass-name"),
SDATAPM (ASN_OCTET_STR, "gclass",       0,              0,          "gclass-name"),
SDATA_END()
};
PRIVATE sdata_desc_t pm_gobj_root_name[] = {
/*-PM----type-----------name------------flag------------default-----description---------- */
SDATAPM (ASN_OCTET_STR, "gobj_name",    0,              0,          "named-gobj or full gobj name"),
SDATAPM (ASN_OCTET_STR, "gobj",         0,              "__yuno__", "named-gobj or full gobj name"),
SDATA_END()
};
PRIVATE sdata_desc_t pm_gobj_def_name[] = {
/*-PM----type-----------name------------flag------------default-----description---------- */
SDATAPM (ASN_OCTET_STR, "gobj_name",    0,              0,          "named-gobj or full gobj name"),
SDATAPM (ASN_OCTET_STR, "gobj",         0,              "__default_service__", "named-gobj or full gobj name"),
SDATA_END()
};
PRIVATE sdata_desc_t pm_list_persistent_attrs[] = {
/*-PM----type-----------name------------flag------------default-----description---------- */
SDATAPM (ASN_OCTET_STR, "gobj_name",    0,              0,          "named-gobj or full gobj"),
SDATAPM (ASN_OCTET_STR, "gobj",         0,              0,          "named-gobj or full gobj"),
SDATAPM (ASN_OCTET_STR, "attribute",    0,              0,          "Attribute to list/remove"),
SDATA_END()
};
PRIVATE sdata_desc_t pm_remove_persistent_attrs[] = {
/*-PM----type-----------name------------flag------------default-----description---------- */
SDATAPM (ASN_OCTET_STR, "gobj_name",    0,              0,          "named-gobj or full gobj"),
SDATAPM (ASN_OCTET_STR, "gobj",         0,              0,          "named-gobj or full gobj"),
SDATAPM (ASN_OCTET_STR, "attribute",    0,              0,          "Attribute to list/remove"),
SDATAPM (ASN_BOOLEAN,   "__all__",      0,              0,          "Remove all persistent attrs"),
SDATA_END()
};
PRIVATE sdata_desc_t pm_list_childs[] = {
/*-PM----type-----------name------------flag------------default-----description---------- */
SDATAPM (ASN_OCTET_STR, "gobj_name",    0,              0,          "named-gobj"),
SDATAPM (ASN_OCTET_STR, "gobj",         0,              0,          "named-gobj"),
SDATAPM (ASN_OCTET_STR, "child_gclass", 0,              0,          "Child gclass-name (list separated by blank or |,;)"),
SDATAPM (ASN_OCTET_STR, "attributes",   0,              0,          "Attributes to see (list separated by blank or |,;)"),
SDATA_END()
};
PRIVATE sdata_desc_t pm_list_channels[] = {
/*-PM----type-----------name------------flag------------default-----description---------- */
SDATAPM (ASN_BOOLEAN,   "opened",       0,              0,          "Show only opened channels"),
SDATA_END()
};

PRIVATE sdata_desc_t pm_wr_attr[] = {
/*-PM----type-----------name------------flag------------default-----description---------- */
SDATAPM (ASN_OCTET_STR, "gobj_name",    0,              0,          "named-gobj or full gobj name"),
SDATAPM (ASN_OCTET_STR, "gobj",         0,              "__default_service__", "named-gobj or full gobj name"),
SDATAPM (ASN_OCTET_STR, "attribute",    0,              0,          "attribute name"),
SDATAPM (ASN_OCTET_STR, "value",        0,              0,          "value"),
SDATA_END()
};
PRIVATE sdata_desc_t pm_rd_attr[] = {
/*-PM----type-----------name------------flag------------default-----description---------- */
SDATAPM (ASN_OCTET_STR, "gobj_name",    0,              0,          "named-gobj or full gobj name"),
SDATAPM (ASN_OCTET_STR, "gobj",         0,              "__default_service__", "named-gobj or full gobj name"),
SDATAPM (ASN_OCTET_STR, "attribute",    0,              0,          "attribute name"),
SDATA_END()
};
PRIVATE sdata_desc_t pm_set_gobj_tr[] = {
/*-PM----type-----------name------------flag------------default-----description---------- */
SDATAPM (ASN_OCTET_STR, "gobj_name",    0,              0,          "named-gobj or full gobj name"),
SDATAPM (ASN_OCTET_STR, "gobj",         0,              "",         "named-gobj or full gobj name"),
SDATAPM (ASN_OCTET_STR, "level",        0,              0,          "attribute name"),
SDATAPM (ASN_OCTET_STR, "set",          0,              0,          "value"),
SDATA_END()
};

PRIVATE sdata_desc_t pm_add_trace_filter[] = {
/*-PM----type-----------name------------flag------------default-----description---------- */
SDATAPM (ASN_OCTET_STR, "attr",         0,              "",         "Attribute of gobj to filter"),
SDATAPM (ASN_OCTET_STR, "value",        0,              "",         "Value of attribute to filer"),
SDATA_END()
};
PRIVATE sdata_desc_t pm_remove_trace_filter[] = {
/*-PM----type-----------name------------flag------------default-----description---------- */
SDATAPM (ASN_OCTET_STR, "attr",         0,              "",         "Attribute of gobj to filter"),
SDATAPM (ASN_OCTET_STR, "value",        0,              "",         "Value of attribute to filer"),
SDATAPM (ASN_BOOLEAN,   "all",          0,              0,          "Remove all filters"),
SDATA_END()
};

PRIVATE sdata_desc_t pm_set_global_tr[] = {
/*-PM----type-----------name------------flag------------default-----description---------- */
SDATAPM (ASN_OCTET_STR, "level",        0,              0,          "attribute name"),
SDATAPM (ASN_OCTET_STR, "set",          0,              0,          "value"),
SDATA_END()
};
PRIVATE sdata_desc_t pm_set_gclass_tr[] = {
/*-PM----type-----------name------------flag------------default-----description---------- */
SDATAPM (ASN_OCTET_STR, "gclass_name",  0,              0,          "gclass-name"),
SDATAPM (ASN_OCTET_STR, "gclass",       0,              0,          "gclass-name"),
SDATAPM (ASN_OCTET_STR, "level",        0,              0,          "attribute name"),
SDATAPM (ASN_OCTET_STR, "set",          0,              0,          "value"),
SDATA_END()
};
PRIVATE sdata_desc_t pm_reset_all_tr[] = {
/*-PM----type-----------name------------flag------------default-----description---------- */
SDATAPM (ASN_OCTET_STR, "gobj",         0,              "",         "named-gobj or full gobj name"),
SDATAPM (ASN_OCTET_STR, "gclass",       0,              0,          "gclass-name"),
SDATA_END()
};

PRIVATE sdata_desc_t pm_spawn[] = {
/*-PM----type-----------name------------flag------------default-----description---------- */
SDATAPM (ASN_OCTET_STR, "process",      0,              0,          "Process to execute"),
SDATA_END()
};
PRIVATE sdata_desc_t pm_set_daemon_debug[] = {
/*-PM----type-----------name------------flag------------default-----description---------- */
SDATAPM (ASN_OCTET_STR, "set",          0,              0,          "value"),
SDATA_END()
};
PRIVATE sdata_desc_t pm_set_deep_trace[] = {
/*-PM----type-----------name------------flag------------default-----description---------- */
SDATAPM (ASN_OCTET_STR, "set",          0,              0,          "value"),
SDATA_END()
};

PRIVATE sdata_desc_t pm_add_log_handler[] = {
/*-PM----type-----------name------------flag------------default-----description---------- */
SDATAPM (ASN_OCTET_STR, "name",         0,              0,          "Handler log name"),
SDATAPM (ASN_OCTET_STR, "type",         0,              0,          "Handler log type"),
SDATAPM (ASN_OCTET_STR, "options",      0,              0,          "Handler log options"),
SDATAPM (ASN_OCTET_STR, "url",          0,              0,          "Url for log 'udp' type"),
SDATA_END()
};
PRIVATE sdata_desc_t pm_del_log_handler[] = {
/*-PM----type-----------name------------flag------------default-----description---------- */
SDATAPM (ASN_OCTET_STR, "name",         0,              0,          "Handler name"),
SDATA_END()
};
PRIVATE sdata_desc_t pm_subscribe_event[] = {
/*-PM----type-----------name------------flag------------default-----description---------- */
SDATAPM (ASN_OCTET_STR, "gobj_name",    0,              0,          "named-gobj or full gobj name"),
SDATAPM (ASN_OCTET_STR, "gobj",         0,              "__default_service__", "named-gobj or full gobj name"),
SDATAPM (ASN_OCTET_STR, "event_name",   0,              0,          "Event name"),
SDATAPM (ASN_OCTET_STR, "rename_event_name",0,          0,          "Rename event name"),
SDATAPM (ASN_BOOLEAN,   "first_shot",   0,              0,          "First shot (send all records on subcribing"),
SDATAPM (ASN_JSON,      "config",       0,              0,          "subcription config"),
SDATAPM (ASN_JSON,      "filter",       0,              0,          "subcription filter"),
SDATA_END()
};
PRIVATE sdata_desc_t pm_list_subscriptions[] = {
/*-PM----type-----------name------------flag------------default-----description---------- */
SDATAPM (ASN_OCTET_STR, "gobj_name",    0,              0,          "named-gobj or full gobj name"),
SDATAPM (ASN_OCTET_STR, "gobj",         0,              "__default_service__", "named-gobj or full gobj name"),
SDATAPM (ASN_BOOLEAN,   "recursive",    0,              0,          "Walk over childs"),
SDATAPM (ASN_BOOLEAN,   "full-names",   0,              0,          "Show full names"),
SDATA_END()
};
PRIVATE sdata_desc_t pm_add_allowed_ip[] = {
/*-PM----type-----------name------------flag------------default-----description---------- */
SDATAPM (ASN_OCTET_STR, "ip",           0,              "",         "ip"),
SDATAPM (ASN_BOOLEAN,   "allowed",      0,              0,          "TRUE allowed, FALSE denied"),
SDATA_END()
};
PRIVATE sdata_desc_t pm_remove_allowed_ip[] = {
/*-PM----type-----------name------------flag------------default-----description---------- */
SDATAPM (ASN_OCTET_STR, "ip",           0,              "",         "ip"),
SDATA_END()
};
PRIVATE sdata_desc_t pm_add_denied_ip[] = {
/*-PM----type-----------name------------flag------------default-----description---------- */
SDATAPM (ASN_OCTET_STR, "ip",           0,              "",         "ip"),
SDATAPM (ASN_BOOLEAN,   "denied",       0,              0,          "TRUE denied, FALSE denied"),
SDATA_END()
};
PRIVATE sdata_desc_t pm_remove_denied_ip[] = {
/*-PM----type-----------name------------flag------------default-----description---------- */
SDATAPM (ASN_OCTET_STR, "ip",           0,              "",         "ip"),
SDATA_END()
};
PRIVATE sdata_desc_t pm_2key_get_value[] = {
/*-PM----type-----------name------------flag------------default-----description---------- */
SDATAPM (ASN_OCTET_STR, "key1",         0,              "",         "Key1"),
SDATAPM (ASN_OCTET_STR, "key2",         0,              "",         "Key2"),
SDATAPM (ASN_OCTET_STR, "path",         0,              "",         "Path"),
SDATAPM (ASN_BOOLEAN,   "expanded",     0,              0,          "No expanded (default) return [[size]]"),
SDATAPM (ASN_UNSIGNED,  "lists_limit",  0,              0,          "Expand lists only if size < limit. 0 no limit"),
SDATAPM (ASN_UNSIGNED,  "dicts_limit",  0,              0,          "Expand dicts only if size < limit. 0 no limit"),
SDATA_END()
};

PRIVATE const char *a_help[] = {"h", "?", 0};
PRIVATE const char *a_services[] = {"services", "list-services", 0};
PRIVATE const char *a_pers_attrs[] = {"persistent-attrs", 0};
PRIVATE const char *a_read_attrs[] = {"read-attrs", 0};
PRIVATE const char *a_read_attrs2[] = {"read-attrs2", 0};

PRIVATE sdata_desc_t command_table[] = {
/*-CMD---type-----------name------------------------alias---items-----------json_fn----------------------description---------- */
SDATACM (ASN_SCHEMA,    "help",                     a_help, pm_help,        cmd_help,                   "Command's help"),
SDATACM (ASN_SCHEMA,    "authzs",                   0,      pm_authzs,      cmd_authzs,     "Authorization's help"),
SDATACM (ASN_SCHEMA,    "view-config",              0,      0,              cmd_view_config,            "View final json configuration"),
SDATACM (ASN_SCHEMA,    "view-mem",                 0,      0,              cmd_view_mem,               "View yuno memory"),
SDATACM (ASN_SCHEMA,    "view-mem2",                0,      0,              cmd_view_mem2,              "View yuno memory with segmented free blocks"),
SDATACM (ASN_SCHEMA,    "view-gclass-register",     0,      0,              cmd_view_gclass_register,   "View gclass's register"),
SDATACM (ASN_SCHEMA,    "view-service-register",    a_services,pm_gclass_name,cmd_view_service_register,  "View service's register"),
SDATACM (ASN_SCHEMA,    "view-unique-register",     0,      0,              cmd_view_unique_register,   "View unique-name's register"),

SDATACM (ASN_SCHEMA,    "view-gclass",              0,      pm_gclass_name, cmd_view_gclass,            "View gclass description"),
SDATACM (ASN_SCHEMA,    "view-gobj",                0,      pm_gobj_def_name, cmd_view_gobj,            "View gobj"),

SDATACM (ASN_SCHEMA,    "public-attrs",             0,      pm_gclass_name, cmd_public_attrs,           "List public attrs of gclasses"),

SDATACM (ASN_SCHEMA,    "view-attrs",               a_read_attrs,pm_gobj_def_name, cmd_view_attrs,      "View gobj's attrs"),
SDATACM (ASN_SCHEMA,    "view-attrs2",              a_read_attrs2,pm_gobj_def_name, cmd_view_attrs2,    "View gobj's attrs with details"),

SDATACM (ASN_SCHEMA,    "webix-gobj-tree",          0,      pm_gobj_root_name,cmd_webix_gobj_tree,      "View webix style gobj tree"),
SDATACM (ASN_SCHEMA,    "view-gobj-tree",           0,      pm_gobj_root_name,cmd_view_gobj_tree,       "View gobj tree"),
SDATACM (ASN_SCHEMA,    "view-gobj-treedb",         0,      pm_gobj_root_name,cmd_view_gobj_treedb,     "View gobj treedb"),

SDATACM (ASN_SCHEMA,    "list-childs",              0,      pm_list_childs, cmd_list_childs,            "List childs of the specified gclass"),
SDATACM (ASN_SCHEMA,    "list-channels",            0,      pm_list_channels,cmd_list_channels,      "List channels (GUI usage)."),

SDATACM (ASN_SCHEMA,    "write-bool",               0,      pm_wr_attr,     cmd_write_bool,             "Write a boolean attribute)"),
SDATACM (ASN_SCHEMA,    "write-str",                0,      pm_wr_attr,     cmd_write_str,              "Write a string attribute"),
SDATACM (ASN_SCHEMA,    "write-json",               0,      pm_wr_attr,     cmd_write_json,             "Write a json attribute"),
SDATACM (ASN_SCHEMA,    "write-number",             0,      pm_wr_attr,     cmd_write_num,              "Write a numeric attribute)"),
SDATACM (ASN_SCHEMA,    "read-number",              0,      pm_rd_attr,     cmd_read_num,               "Read a numeric attribute"),

SDATACM (ASN_SCHEMA,    "info-cpus",                0,      0,              cmd_info_cpus,              "Info of cpus"),
SDATACM (ASN_SCHEMA,    "info-ifs",                 0,      0,              cmd_info_ifs,               "Info of ifs"),
SDATACM (ASN_SCHEMA,    "info-os",                  0,      0,              cmd_info_os,                "Info os"),

SDATACM (ASN_SCHEMA,    "info-global-trace",        0,      0,              cmd_info_global_trace,      "Info of global trace levels"),
SDATACM (ASN_SCHEMA,    "get-global-trace",         0,      0,              cmd_get_global_trace,       "Get global trace levels"),
SDATACM (ASN_SCHEMA,    "set-global-trace",         0,      pm_set_global_tr,cmd_set_global_trace,      "Set global trace level"),

SDATACM (ASN_SCHEMA,    "info-gclass-trace",        0,      pm_gclass_name, cmd_info_gclass_trace,      "Info of class's trace levels"),
SDATACM (ASN_SCHEMA,    "get-gclass-trace",         0,      pm_gclass_name, cmd_get_gclass_trace,       "Get gclass' trace"),
SDATACM (ASN_SCHEMA,    "set-gclass-trace",         0,      pm_set_gclass_tr,cmd_set_gclass_trace,      "Set trace of a gclass)"),
SDATACM (ASN_SCHEMA,    "get-gclass-no-trace",      0,      pm_gclass_name, cmd_get_gclass_no_trace,    "Get no gclass' trace"),
SDATACM (ASN_SCHEMA,    "set-gclass-no-trace",      0,      pm_set_gclass_tr,cmd_set_no_gclass_trace,   "Set no-trace of a gclass)"),

SDATACM (ASN_SCHEMA,    "info-gobj-trace",          0,      pm_gobj_root_name, cmd_info_gobj_trace,      "Info gobj's trace"),
SDATACM (ASN_SCHEMA,    "get-gobj-trace",           0,      pm_gobj_root_name, cmd_get_gobj_trace,       "Get gobj's trace and his childs"),
SDATACM (ASN_SCHEMA,    "get-gobj-no-trace",        0,      pm_gobj_root_name, cmd_get_gobj_no_trace,    "Get no gobj's trace  and his childs"),
SDATACM (ASN_SCHEMA,    "set-gobj-trace",           0,      pm_set_gobj_tr, cmd_set_gobj_trace,         "Set trace of a named-gobj"),
SDATACM (ASN_SCHEMA,    "set-gobj-no-trace",        0,      pm_set_gobj_tr, cmd_set_no_gobj_trace,      "Set no-trace of a named-gobj"),

SDATACM (ASN_SCHEMA,    "add-trace-filter",         0,      pm_add_trace_filter, cmd_add_trace_filter,  "Add a trace filter"),
SDATACM (ASN_SCHEMA,    "remove-trace-filter",      0,      pm_remove_trace_filter, cmd_remove_trace_filter, "Remove a trace filter"),
SDATACM (ASN_SCHEMA,    "get-trace-filter",         0,      0, cmd_get_trace_filter, "Get trace filters"),

SDATACM (ASN_SCHEMA,    "reset-all-traces",         0,      pm_reset_all_tr, cmd_reset_all_traces,         "Reset all traces of a named-gobj of gclass"),
SDATACM (ASN_SCHEMA,    "set-deep-trace",           0,      pm_set_deep_trace,cmd_set_deep_trace,   "Set deep trace, all traces active"),
SDATACM (ASN_SCHEMA,    "set-daemon-debug",         0,      pm_set_daemon_debug,cmd_set_daemon_debug,   "Set daemon debug"),

// HACK DANGER backdoor, use Yuneta only in private networks, or public but encrypted and assured connections.
SDATACM (ASN_SCHEMA,    "spawn",                    0,      pm_spawn,       cmd_spawn,                  "Spawn a new process"),

SDATACM (ASN_SCHEMA,    "trunk-rotatory-file",      0,      0,              cmd_trunk_rotatory_file,    "Trunk rotatory files"),
SDATACM (ASN_SCHEMA,    "reset-log-counters",       0,      0,              cmd_reset_log_counters,     "Reset log counters"),
SDATACM (ASN_SCHEMA,    "add-log-handler",          0,      pm_add_log_handler,cmd_add_log_handler,     "Add log handler"),
SDATACM (ASN_SCHEMA,    "delete-log-handler",       0,      pm_del_log_handler,cmd_del_log_handler,     "Delete log handler"),
SDATACM (ASN_SCHEMA,    "list-log-handler",         0,      0,              cmd_list_log_handler,       "List log handlers"),
SDATACM (ASN_SCHEMA,    "list-persistent-attrs",    a_pers_attrs,      pm_list_persistent_attrs,cmd_list_persistent_attrs,  "List persistent attributes of yuno"),
SDATACM (ASN_SCHEMA,    "remove-persistent-attrs",  0,      pm_remove_persistent_attrs,cmd_remove_persistent_attrs,  "List persistent attributes of yuno"),
SDATACM (ASN_SCHEMA,    "start-gobj-tree",          0,      pm_gobj_def_name,cmd_start_gobj_tree,       "Start named-gobj tree"),
SDATACM (ASN_SCHEMA,    "stop-gobj-tree",           0,      pm_gobj_def_name,cmd_stop_gobj_tree,        "Stop named-gobj tree"),
SDATACM (ASN_SCHEMA,    "enable-gobj",              0,      pm_gobj_def_name,cmd_enable_gobj,           "Enable named-gobj"),
SDATACM (ASN_SCHEMA,    "disable-gobj",             0,      pm_gobj_def_name,cmd_disable_gobj,          "Disable named-gobj"),
SDATACM (ASN_SCHEMA,    "subscribe-event",          0,      pm_subscribe_event,cmd_subscribe_event,          "Subscribe event [of __default_service__]"),
SDATACM (ASN_SCHEMA,    "unsubscribe-event",        0,      pm_subscribe_event,cmd_unsubscribe_event,          "Unsubscribe event [of __default_service__]"),
SDATACM (ASN_SCHEMA,    "list-subscriptions",       0,      pm_list_subscriptions,cmd_list_subscriptions,          "List subscriptions [of __default_service__]"),
SDATACM (ASN_SCHEMA,    "list-subscribings",        0,      pm_list_subscriptions,cmd_list_subscribings,          "List subcribings [of __default_service__]"),

SDATACM (ASN_SCHEMA,    "list-allowed-ips",         0,      0,              cmd_list_allowed_ips,          "List allowed ips"),
SDATACM (ASN_SCHEMA,    "add-allowed-ip",           0,      pm_add_allowed_ip,  cmd_add_allowed_ip,          "Add a ip to allowed list"),
SDATACM (ASN_SCHEMA,    "remove-allowed-ip",        0,      pm_remove_allowed_ip, cmd_remove_allowed_ip,          "Add a ip to allowed list"),
SDATACM (ASN_SCHEMA,    "list-denied-ips",          0,      0,              cmd_list_denied_ips,          "List denied ips"),
SDATACM (ASN_SCHEMA,    "add-denied-ip",            0,      pm_add_denied_ip,  cmd_add_denied_ip,          "Add a ip to denied list"),
SDATACM (ASN_SCHEMA,    "remove-denied-ip",         0,      pm_remove_denied_ip, cmd_remove_denied_ip,          "Add a ip to denied list"),
SDATACM (ASN_SCHEMA,    "get-2key-schema",          0,      0, cmd_2key_get_schema, "Get 2key schema"),
SDATACM (ASN_SCHEMA,    "get-2key-value",           0,      pm_2key_get_value, cmd_2key_get_value, "Get 2key value"),
SDATACM (ASN_SCHEMA,    "get-2key-subvalue",        0,      pm_2key_get_value, cmd_2key_get_subvalue, "Get 2key sub-value"),
SDATACM (ASN_SCHEMA,    "system-schema",            0,      0, cmd_system_topic_schema, "Get system topic schema"),
SDATACM (ASN_SCHEMA,    "global-variables",         0,      0, cmd_global_variables, "Get global variables"),
SDATA_END()
};

/*---------------------------------------------*
 *      Attributes - order affect to oid's
 *---------------------------------------------*/
PRIVATE sdata_desc_t tattr_desc[] = {
/*-ATTR-type------------name----------------flag------------------------default---------description---------- */
SDATA (ASN_OCTET_STR,   "appName",          SDF_RD,                     "",             "App name, must match the role"),
SDATA (ASN_OCTET_STR,   "appDesc",          SDF_RD,                     "",             "App Description"),
SDATA (ASN_OCTET_STR,   "appDate",          SDF_RD,                     "",             "App date/time"),
SDATA (ASN_OCTET_STR,   "work_dir",         SDF_RD,                     "",             "Work dir"),
SDATA (ASN_OCTET_STR,   "domain_dir",       SDF_RD,                     "",             "Domain dir"),
SDATA (ASN_OCTET_STR,   "yuno_role",        SDF_RD,                     "",             "Yuno Role"),
SDATA (ASN_OCTET_STR,   "yuno_id",          SDF_RD,                     "",             "Yuno Id. Set by agent"),
SDATA (ASN_OCTET_STR,   "yuno_name",        SDF_RD,                     "",             "Yuno Name. Set by agent"),
SDATA (ASN_OCTET_STR,   "yuno_tag",         SDF_RD,                     "",             "Yuno Alias. Set by agent"),
SDATA (ASN_OCTET_STR,   "yuno_release",     SDF_RD,                     "",             "Yuno Release. Set by agent"),
SDATA (ASN_OCTET_STR,   "yuno_version",     SDF_RD,                     "",             "Yuno version"),
SDATA (ASN_OCTET_STR,   "yuneta_version",   SDF_RD,                     __yuneta_version__, "Yuneta version"),
SDATA (ASN_OCTET_STR,   "bind_ip",          SDF_RD,                     "",             "Bind ip of yuno's realm. Set by agent"),
SDATA (ASN_BOOLEAN,     "yuno_multiple",    SDF_RD,                     0,              "True when yuno can open shared ports. Set by agent"),
SDATA (ASN_JSON,        "tags",             SDF_RD,                     0,              "tags"),
SDATA (ASN_JSON,        "required_services",SDF_RD,                     0,              "Required services"),
SDATA (ASN_JSON,        "public_services",  SDF_RD,                     0,              "Public services"),
SDATA (ASN_JSON,        "service_descriptor",SDF_RD,                    0,              "Public service descriptor"),
SDATA (ASN_OCTET_STR,   "i18n_dirname",     SDF_RD,                     "",             "dir_name parameter of bindtextdomain()"),
SDATA (ASN_OCTET_STR,   "i18n_domain",      SDF_RD,                     "",             "domain_name parameter of bindtextdomain() and textdomain()"),
SDATA (ASN_OCTET_STR,   "i18n_codeset",     SDF_RD,                     "UTF-8",        "codeset of textdomain"),
SDATA (ASN_UNSIGNED,    "timeout",          SDF_RD,                     1000,           "timeout (miliseconds) to check yuno_must_die, stats,"),
SDATA (ASN_UNSIGNED,    "timeout_stats",    SDF_RD,                     1,              "timeout (seconds) for publishing stats"),
SDATA (ASN_UNSIGNED,    "timeout_flush",    SDF_RD,                     2,              "timeout (seconds) for rotatory flush"),
SDATA (ASN_UNSIGNED,    "watcher_pid",      SDF_RD,                     0,              "Watcher pid"),
SDATA (ASN_OCTET_STR,   "info_msg",         SDF_RD,                     "",             "Info msg, like errno"),
SDATA (ASN_COUNTER64,   "launch_id",        SDF_RD,                     0,              "Launch Id. Set by agent"),
SDATA (ASN_OCTET_STR,   "dynamicDoc",       SDF_WR|SDF_PERSIST,         "",             "Dynamic documentation"),
SDATA (ASN_OCTET_STR,   "start_date",       SDF_RD|SDF_STATS,           "",             "Yuno starting date"),

SDATA (ASN_UNSIGNED64,  "max_proc_mem",     SDF_WR,                     0,              "NOT USED! If not zero, abort if proc memory is greater"),

SDATA (ASN_COUNTER64,   "uptime",           SDF_RD|SDF_STATS,           0,              "Yuno living time"),
SDATA (ASN_UNSIGNED,    "log_emergencies",  SDF_WR|SDF_STATS,           0,              "Emergencies log counter"),
SDATA (ASN_UNSIGNED,    "log_alerts",       SDF_WR|SDF_STATS,           0,              "Alerts counter"),
SDATA (ASN_UNSIGNED,    "log_criticals",    SDF_WR|SDF_STATS,           0,              "Criticals counter"),
SDATA (ASN_UNSIGNED,    "log_errors",       SDF_WR|SDF_STATS,           0,              "Errors counter"),
SDATA (ASN_UNSIGNED,    "log_warnings",     SDF_WR|SDF_STATS,           0,              "Warnings counter"),
SDATA (ASN_UNSIGNED,    "log_notices",      SDF_WR|SDF_STATS,           0,              "Notices counter"),
SDATA (ASN_UNSIGNED,    "log_infos",        SDF_WR|SDF_STATS,           0,              "Infos counter"),
SDATA (ASN_UNSIGNED,    "log_debugs",       SDF_WR|SDF_STATS,           0,              "Debugs counter"),
SDATA (ASN_UNSIGNED,    "log_audits",       SDF_WR|SDF_STATS,           0,              "Audits counter"),
SDATA (ASN_UNSIGNED,    "log_monitors",     SDF_WR|SDF_STATS,           0,              "Monitor counter"),

SDATA (ASN_COUNTER64,   "rusage_ru_utime_us",SDF_RD|SDF_STATS,          0,              "See getrusage()"),
SDATA (ASN_COUNTER64,   "rusage_ru_stime_us",SDF_RD|SDF_STATS,          0,              "See getrusage()"),
SDATA (ASN_COUNTER64,   "rusage_ru_maxrss", SDF_RD|SDF_STATS,           0,              "See getrusage()"),
SDATA (ASN_COUNTER64,   "rusage_ru_minflt", SDF_RD|SDF_STATS,           0,              "See getrusage()"),
SDATA (ASN_COUNTER64,   "rusage_ru_majflt", SDF_RD|SDF_STATS,           0,              "See getrusage()"),
SDATA (ASN_COUNTER64,   "rusage_ru_inblock",SDF_RD|SDF_STATS,           0,              "See getrusage()"),
SDATA (ASN_COUNTER64,   "rusage_ru_oublock",SDF_RD|SDF_STATS,           0,              "See getrusage()"),
SDATA (ASN_COUNTER64,   "rusage_ru_nvcsw",  SDF_RD|SDF_STATS,           0,              "See getrusage()"),
SDATA (ASN_COUNTER64,   "rusage_ru_nivcsw", SDF_RD|SDF_STATS,           0,              "See getrusage()"),

SDATA (ASN_COUNTER64,   "mem_memory_node_total_in_kb", SDF_RD|SDF_STATS,0,              "Mem"),
SDATA (ASN_COUNTER64,   "mem_memory_node_free_in_kb", SDF_RD|SDF_STATS, 0,              "Mem"),
SDATA (ASN_COUNTER64,   "mem_memory_rss_in_kb", SDF_RD|SDF_STATS,       0,              "Mem"),
SDATA (ASN_COUNTER64,   "mem_gbmem_total_in_kb", SDF_RD|SDF_STATS,      0,              "Mem"),
SDATA (ASN_COUNTER64,   "mem_gbmem_using_in_kb", SDF_RD|SDF_STATS,      0,              "Mem"),
SDATA (ASN_COUNTER64,   "mem_gbmem_max_sm_in_kb", SDF_RD|SDF_STATS,     0,              "Mem"),
SDATA (ASN_COUNTER64,   "mem_gbmem_cur_sm_in_kb", SDF_RD|SDF_STATS,     0,              "Mem"),

SDATA (ASN_COUNTER64,   "disk_size_in_gigas", SDF_RD|SDF_STATS,         0,              "Disk"),
SDATA (ASN_COUNTER64,   "disk_free_percent", SDF_RD|SDF_STATS,          0,              "Disk"),

SDATA (ASN_COUNTER64,   "qs_txmsgs",        SDF_RD|SDF_STATS,           0,              "Qs"),
SDATA (ASN_COUNTER64,   "qs_rxmsgs",        SDF_RD|SDF_STATS,           0,              "Qs"),
SDATA (ASN_COUNTER64,   "qs_txbytes",       SDF_RD|SDF_STATS,           0,              "Qs"),
SDATA (ASN_COUNTER64,   "qs_rxbytes",       SDF_RD|SDF_STATS,           0,              "Qs"),
SDATA (ASN_COUNTER64,   "qs_output_queue",  SDF_RD|SDF_STATS,           0,              "Qs"),
SDATA (ASN_COUNTER64,   "qs_input_queue",   SDF_RD|SDF_STATS,           0,              "Qs"),
SDATA (ASN_COUNTER64,   "qs_internal_queue",SDF_RD|SDF_STATS,           0,              "Qs"),
SDATA (ASN_COUNTER64,   "qs_drop_by_overflow",SDF_RD|SDF_STATS,         0,              "Qs"),
SDATA (ASN_COUNTER64,   "qs_drop_by_no_exit",SDF_RD|SDF_STATS,          0,              "Qs"),
SDATA (ASN_COUNTER64,   "qs_drop_by_down",  SDF_RD|SDF_STATS,           0,              "Qs"),
SDATA (ASN_COUNTER64,   "qs_lower_response_time", SDF_RD|SDF_STATS,     0,              "Qs"),
SDATA (ASN_COUNTER64,   "qs_medium_response_time", SDF_RD|SDF_STATS,    0,              "Qs"),
SDATA (ASN_COUNTER64,   "qs_higher_response_time", SDF_RD|SDF_STATS,    -1,              "Qs"),
SDATA (ASN_COUNTER64,   "qs_repeated",      SDF_RD|SDF_STATS,           0,              "Qs"),
SDATA (ASN_COUNTER64,   "cpu_ticks",        SDF_RD|SDF_STATS,           0,              "Cpu ticks"),
SDATA (ASN_UNSIGNED,    "cpu",              SDF_RD|SDF_STATS,           0,              "Cpu percent"),
SDATA (ASN_UNSIGNED,    "timeout_restart",  SDF_WR|SDF_STATS|SDF_PERSIST,0,             "timeout (seconds) to restart"),
SDATA (ASN_INTEGER,     "deep_trace",       SDF_WR|SDF_STATS|SDF_PERSIST,0,             "Deep trace set or not set"),
SDATA (ASN_JSON,        "trace_levels",     SDF_WR|SDF_PERSIST,         0,              "Trace levels"),
SDATA (ASN_JSON,        "no_trace_levels",  SDF_WR|SDF_PERSIST,         0,              "No trace levels"),
SDATA (ASN_JSON,        "allowed_ips",      SDF_RD|SDF_PERSIST,         "{}",           "Allowed peer ip's if true, false not allowed"),
SDATA (ASN_JSON,        "denied_ips",       SDF_RD|SDF_PERSIST,         "{}",           "Denied peer ip's if true, false not denied"),
SDATA (ASN_OCTET_STR,   "sys_system_name",  SDF_RD|SDF_STATS,           "",             "System name"),
SDATA (ASN_OCTET_STR,   "sys_node_name",    SDF_RD|SDF_STATS,           "",             "Node name"),
SDATA (ASN_OCTET_STR,   "sys_version",      SDF_RD|SDF_STATS,           "",             "Version"),
SDATA (ASN_OCTET_STR,   "sys_release",      SDF_RD|SDF_STATS,           "",             "Release"),
SDATA (ASN_OCTET_STR,   "sys_machine",      SDF_RD|SDF_STATS,           "",             "Machine"),
SDATA_END()
};

/*---------------------------------------------*
 *      Yuno stats
 *---------------------------------------------*/
// TODO Deberían ser punteros a los atributos, porque al resetear las stats no coge el valor por defecto.
// Afecta a QS_LOWER_RESPONSE_TIME que tiene por defecto -1
PRIVATE uint64_t __qs__[QS__LAST_ITEM__];

/*---------------------------------------------*
 *      GClass trace levels
 *---------------------------------------------*/
enum {
    GCLASS_TRACE_XXX        = 0x0001,
};

PRIVATE const trace_level_t s_user_trace_level[16] = {
{0, 0},
};

/*---------------------------------------------*
 *              Private data
 *---------------------------------------------*/
typedef struct _PRIVATE_DATA {
    const char *yuno_id;
    const char *yuno_name;
    const char *yuno_role;

    uv_loop_t uv_loop;
    hgobj timer;
    uv_process_t process;

    uint32_t *pemergencies;
    uint32_t *palerts;
    uint32_t *pcriticals;
    uint32_t *perrors;
    uint32_t *pwarnings;
    uint32_t *pnotices;
    uint32_t *pinfos;
    uint32_t *pdebugs;
    uint32_t *paudits;
    uint32_t *pmonitors;

    uint64_t last_cpu_ticks;
    uint64_t last_ms;

    size_t t_flush;
    size_t t_stats;
    size_t t_restart;
    uint32_t timeout_flush;
    uint32_t timeout_stats;
    uint32_t timeout_restart;
    uint32_t timeout;
} PRIVATE_DATA;

/***************************************************************************
 *              Prototypes
 ***************************************************************************/
PRIVATE void close_loop(uv_loop_t* loop);




            /***************************
             *      Framework Methods
             ***************************/




/***************************************************************************
 *
 ***************************************************************************/
PRIVATE void remove_pid_file(void)
{
    if(!empty_string(pidfile)) {
        unlink(pidfile);
    }
}

/***************************************************************************
 *      Framework Method create
 ***************************************************************************/
PRIVATE void mt_create(hgobj gobj)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    if (!atexit_registered) {
        atexit(remove_pid_file);
        atexit_registered = 1;
    }

    char bfdate[90];
    current_timestamp(bfdate, sizeof(bfdate));
    gobj_write_str_attr(gobj, "start_date", bfdate);

    priv->pemergencies = gobj_danger_attr_ptr(gobj, "log_emergencies");
    priv->palerts = gobj_danger_attr_ptr(gobj,      "log_alerts");
    priv->pcriticals = gobj_danger_attr_ptr(gobj,   "log_criticals");
    priv->perrors = gobj_danger_attr_ptr(gobj,      "log_errors");
    priv->pwarnings = gobj_danger_attr_ptr(gobj,    "log_warnings");
    priv->pnotices = gobj_danger_attr_ptr(gobj,     "log_notices");
    priv->pinfos = gobj_danger_attr_ptr(gobj,       "log_infos");
    priv->pdebugs = gobj_danger_attr_ptr(gobj,      "log_debugs");
    priv->paudits = gobj_danger_attr_ptr(gobj,      "log_audits");
    priv->pmonitors = gobj_danger_attr_ptr(gobj,    "log_monitors");

    log_set_inform_cb(inform_cb, gobj);

    const char *i18n_domain = gobj_read_str_attr(gobj, "i18n_domain");
    if(!empty_string(i18n_domain)) {
        const char *i18n_dirname = gobj_read_str_attr(gobj, "i18n_dirname");
        if(!empty_string(i18n_dirname)) {
            bindtextdomain(i18n_domain, i18n_dirname);
        }
        textdomain(i18n_domain);
        const char *i18n_codeset = gobj_read_str_attr(gobj, "i18n_codeset");
        if(!empty_string(i18n_codeset)) {
            bind_textdomain_codeset(i18n_domain, i18n_codeset);
        }
    }

    set_user_gclass_traces(gobj);
    set_user_gclass_no_traces(gobj);
    if(gobj_read_int32_attr(gobj, "deep_trace")) {
        gobj_set_deep_tracing(gobj_read_int32_attr(gobj, "deep_trace"));
    }

    /*
     *  Create the event loop
     */
    uv_loop_init(&priv->uv_loop);
    priv->uv_loop.data = gobj;

    /*
     *  Set the periodic timer to flush log
     */
    priv->timer = gobj_create("", GCLASS_TIMER, 0, gobj);

    SET_PRIV(timeout,               gobj_read_uint32_attr)
    SET_PRIV(timeout_stats,         gobj_read_uint32_attr)
    SET_PRIV(timeout_flush,         gobj_read_uint32_attr)
    SET_PRIV(timeout_restart,       gobj_read_uint32_attr)
    SET_PRIV(yuno_id,               gobj_read_str_attr)
    SET_PRIV(yuno_name,             gobj_read_str_attr)
    SET_PRIV(yuno_role,             gobj_read_str_attr)

    {
        struct utsname buf1;
        if(uname(&buf1)==0) {
            change_char(buf1.machine, '_', '-');
            gobj_write_str_attr(gobj, "sys_system_name", buf1.sysname);
            gobj_write_str_attr(gobj, "sys_node_name", buf1.nodename);
            gobj_write_str_attr(gobj, "sys_version", buf1.version);
            gobj_write_str_attr(gobj, "sys_release", buf1.release);
            gobj_write_str_attr(gobj, "sys_machine", buf1.machine);
        }
    }

    if(gobj_read_uint64_attr(gobj, "launch_id")) {
        save_pid_in_file(gobj);
    }
}

/***************************************************************************
 *      Framework Method writing
 ***************************************************************************/
PRIVATE void mt_writing(hgobj gobj, const char *path)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    IF_EQ_SET_PRIV(timeout,                 gobj_read_uint32_attr)
        if(gobj_is_running(gobj)) {
            set_timeout_periodic(priv->timer, priv->timeout);
        }
    ELIF_EQ_SET_PRIV(timeout_stats,         gobj_read_uint32_attr)
    ELIF_EQ_SET_PRIV(timeout_flush,         gobj_read_uint32_attr)
    ELIF_EQ_SET_PRIV(timeout_restart,       gobj_read_uint32_attr)
        if(priv->timeout_restart) {
            priv->t_restart = start_sectimer(priv->timeout_restart);
        } else {
            priv->t_restart = 0;
        }
    END_EQ_SET_PRIV()
}

/***************************************************************************
 *      Framework Method stats_updated
 ***************************************************************************/
PRIVATE int mt_stats_updated(
    hgobj gobj,
    hgobj stats_gobj,
    const char *attr_name,
    int type,
    SData_Value_t old_v,
    SData_Value_t new_v
)
{
    // TODO lee el __t__
    // TODO guarda el par (__t__, value(stats_gobj,attr_name))
    // TODO un resource para guardar los stats_gobj/attr_name
    // TODO un resource para formulas de stats (sumatorios, etc) (iters en memoria)?
    // TODO cuando se modifique un recurso, que exista una callback que avise a todos los iter
    //      que lo contengan,
    //      (para los sumatorios por ejemplo, que se rehagan automáticamente con cada modificación)


    // TODO PUBLICA la stat -> {stat:value} y el que quiera que se suscriba.
    // path: gobj_path(stats_obj)
    // attr: attr_name
    // value: str or int
    return 0;
}

/***************************************************************************
 *      Framework Method gobj_created
 ***************************************************************************/
PRIVATE void mt_gobj_created(hgobj gobj, hgobj gobj_created)
{
}

/***************************************************************************
 *      Framework Method destroy
 ***************************************************************************/
PRIVATE void mt_destroy(hgobj gobj)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    int ret = uv_loop_close(&priv->uv_loop);
    if(ret < 0) {
        log_error(0,
            "gobj",         "%s", __FILE__,
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_LIBUV_ERROR,
            "msg",          "%s", "uv_loop_close() failed UV_EBUSY",
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

    /*
     *  Set user traces
     *  Here set gclass/gobj traces
     */
    set_user_gobj_traces(gobj);
    set_user_gobj_no_traces(gobj);

    // TODO recorre resource stats y pon gobj_set_attr_stats()

    gobj_start(priv->timer);
    set_timeout_periodic(priv->timer, priv->timeout);

    if(priv->timeout_flush) {
        priv->t_flush = start_sectimer(priv->timeout_flush);
    }
    if(priv->timeout_stats) {
        priv->t_stats = start_sectimer(priv->timeout_stats);
    }
    if(priv->timeout_restart) {
        priv->t_restart = start_sectimer(priv->timeout_restart);
    }

    return 0;
}

/***************************************************************************
 *      run
 ***************************************************************************/
PUBLIC void mt_run(hgobj gobj)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    if(!gobj) {
        log_error(0,
            "gobj",         "%s", __FILE__,
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_PARAMETER_ERROR,
            "msg",          "%s", "gobj is NULL",
            NULL
        );
        return;
    }

    /*
     *  Run event loop
     */
    log_debug(0,
        "gobj",             "%s", gobj_name(gobj),
        "function",         "%s", __FUNCTION__,
        "msgset",           "%s", MSGSET_STARTUP,
        "msg",              "%s", "Running main",
        "yuno_id",          "%s", priv->yuno_id,
        "yuno_name",        "%s", priv->yuno_name,
        "yuno_role",        "%s", priv->yuno_role,
        NULL
    );

    uv_run(&priv->uv_loop, UV_RUN_DEFAULT);

    log_debug(0,
        "gobj",             "%s", gobj_name(gobj),
        "function",         "%s", __FUNCTION__,
        "msgset",           "%s", MSGSET_STARTUP,
        "msg",              "%s", "Stopped main",
        "yuno_role",        "%s", priv->yuno_role,
        NULL
    );
}

/***************************************************************************
 *      check cleaning of uv
 ***************************************************************************/
PUBLIC void mt_clean(hgobj gobj)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);
    if(!gobj) {
        return;
    }
    log_debug(0,
        "gobj",         "%s", __FILE__,
        "function",     "%s", __FUNCTION__,
        "msgset",       "%s", MSGSET_STARTUP,
        "msg",          "%s", "cleaning",
        NULL
    );
    int pending = uv_loop_alive(&priv->uv_loop);
    if(pending) {
        close_loop(&priv->uv_loop);
        uv_run(&priv->uv_loop, UV_RUN_DEFAULT);
    }

    pending = uv_loop_alive(&priv->uv_loop);
    if(pending) {
        log_error(0,
            "gobj",         "%s", __FILE__,
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_LIBUV_ERROR,
            "msg",          "%s", "uv_loop() with SOME PENDING",
            NULL
        );
    }
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
 *      Framework Method mt_publication_pre_filter
 ***************************************************************************/
PRIVATE BOOL mt_publication_pre_filter(
    hgobj gobj,
    hsdata subs,
    const char *event,
    json_t *kw  // kw NOT owned! you can modify this publishing kw
)
{
    BOOL to_publish = 0;

    json_t *__config__ = sdata_read_json(subs, "__config__");
    json_int_t __fire_time__ = kw_get_int(__config__, "__fire_time__", 0, 0);
    json_int_t refresh_time = kw_get_int(__config__, "refresh_time", 5, 0);
    if(!__fire_time__) {
        __fire_time__ =  start_sectimer((time_t)refresh_time);
        json_object_set_new(__config__, "__fire_time__", json_integer(__fire_time__));
    } else {
        to_publish = test_sectimer(__fire_time__);
    }
    if(to_publish) {
        hgobj subscriber = sdata_read_pointer(subs, "subscriber");
        const char *stats = kw_get_str(__config__, "stats", "", 0);
        JSON_INCREF(kw);
        json_t *jn_webix_response = gobj_stats(gobj, stats, kw, subscriber);
        json_object_update(kw, jn_webix_response);
        JSON_DECREF(jn_webix_response);
        __fire_time__ =  start_sectimer((time_t)refresh_time);
        json_object_set_new(__config__, "__fire_time__", json_integer(__fire_time__));
    }
    return to_publish;

}




            /***************************
             *      Command Methods
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

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE json_t *cmd_view_config(hgobj gobj, const char *cmd, json_t *kw, hgobj src)
{
    json_t *jn_data = yuneta_json_config();
    json_incref(jn_data);
    return msg_iev_build_webix(
        gobj,
        0,
        0,
        0,
        jn_data,
        kw  // owned
    );
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE json_t *cmd_view_mem(hgobj gobj, const char *cmd, json_t *kw, hgobj src)
{
    json_t *jn_data = gbmem_json_info(FALSE);
    return msg_iev_build_webix(
        gobj,
        0,
        0,
        0,
        jn_data,
        kw  // owned
    );
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE json_t *cmd_view_mem2(hgobj gobj, const char *cmd, json_t *kw, hgobj src)
{
    json_t *jn_data = gbmem_json_info(TRUE);
    return msg_iev_build_webix(
        gobj,
        0,
        0,
        0,
        jn_data,
        kw  // owned
    );
}

/***************************************************************************
 *  Get the gclass of a gobj
 ***************************************************************************/
PRIVATE GCLASS *_get_gclass_from_gobj(const char *gobj_name)
{
    hgobj gobj2view = gobj_find_unique_gobj(gobj_name, FALSE);
    if(!gobj2view) {
        gobj2view = gobj_find_gobj(gobj_name);
        if(!gobj2view) {
            return 0;
        }
    }
    return gobj_gclass(gobj2view);
}

/***************************************************************************
 *  Show a gclass description
 ***************************************************************************/
PRIVATE json_t *cmd_view_gclass(hgobj gobj, const char *cmd, json_t *kw, hgobj src)
{
    const char *gclass_name_ = kw_get_str(
        kw,
        "gclass_name",
        kw_get_str(kw, "gclass", "", 0),
        0
    );
    GCLASS *gclass = gobj_find_gclass(gclass_name_, FALSE);
    if(!gclass) {
        gclass = _get_gclass_from_gobj(gclass_name_);
        if(!gclass) {
            return msg_iev_build_webix(
                gobj,
                -1,
                json_sprintf(
                    "%s: what gclass is '%s'?", gobj_short_name(gobj), gclass_name_
                ),
                0,
                0,
                kw  // owned
            );
        }
    }

    json_t *jn_data = gclass2json(gclass);

    return msg_iev_build_webix(
        gobj,
        0,
        0,
        0, // TODO kw_incref(gobj_gobjs_treedb_schema("gclass")),
        jn_data,
        kw  // owned
    );
}

/***************************************************************************
 *  Show a gobj
 ***************************************************************************/
PRIVATE json_t *cmd_view_gobj(hgobj gobj, const char *cmd, json_t *kw, hgobj src)
{
    const char *gobj_name_ = kw_get_str( // __default_service__
        kw,
        "gobj_name",
        kw_get_str(kw, "gobj", "", 0),
        0
    );
    hgobj gobj2view = gobj_find_unique_gobj(gobj_name_, FALSE);
    if(!gobj2view) {
        gobj2view = gobj_find_gobj(gobj_name_);
        if(!gobj2view) {
            return msg_iev_build_webix(
                gobj,
                -1,
                json_sprintf(
                    "%s: gobj '%s' not found.", gobj_short_name(gobj), gobj_name_
                ),
                0,
                0,
                kw  // owned
            );
        }
    }

    json_t *jn_data = gobj2json(gobj2view);
    return msg_iev_build_webix(
        gobj,
        0,
        0,
        0,
        jn_data,
        kw  // owned
    );
}

/***************************************************************************
 *  Show a public attrs of gclasses
 ***************************************************************************/
PRIVATE json_t *cmd_public_attrs(hgobj gobj, const char *cmd, json_t *kw, hgobj src)
{
    const char *gclass_name_ = kw_get_str(
        kw,
        "gclass_name",
        kw_get_str(kw, "gclass", "", 0),
        0
    );
    GCLASS *gclass = gobj_find_gclass(gclass_name_, FALSE);
    if(!gclass) {
        gclass = _get_gclass_from_gobj(gclass_name_);
    }

    json_t *jn_data = gclass_public_attrs(gclass);

    return msg_iev_build_webix(
        gobj,
        0,
        0,
        0,
        jn_data,
        kw  // owned
    );
}

/***************************************************************************
 *  Show a gobj's attrs
 ***************************************************************************/
PRIVATE json_t *cmd_view_attrs(hgobj gobj, const char *cmd, json_t *kw, hgobj src)
{
    const char *gobj_name_ = kw_get_str( // __default_service__
        kw,
        "gobj_name",
        kw_get_str(kw, "gobj", "", 0),
        0
    );
    hgobj gobj2view = gobj_find_unique_gobj(gobj_name_, FALSE);
    if(!gobj2view) {
        gobj2view = gobj_find_gobj(gobj_name_);
        if(!gobj2view) {
            return msg_iev_build_webix(
                gobj,
                -1,
                json_sprintf(
                    "%s: gobj '%s' not found.", gobj_short_name(gobj), gobj_name_
                ),
                0,
                0,
                kw  // owned
            );
        }
    }

    json_t *jn_data = sdata2json(gobj_hsdata(gobj2view), ATTR_READABLE|ATTR_WRITABLE, 0);
    append_yuno_metadata(gobj2view, jn_data, cmd);
    return msg_iev_build_webix(
        gobj,
        0,
        0,
        0,
        jn_data,
        kw  // owned
    );
}

/***************************************************************************
 *  Show a gobj's attrs with detail
 ***************************************************************************/
PRIVATE json_t *cmd_view_attrs2(hgobj gobj, const char *cmd, json_t *kw, hgobj src)
{
    const char *gobj_name_ = kw_get_str( // __default_service__
        kw,
        "gobj_name",
        kw_get_str(kw, "gobj", "", 0),
        0
    );
    hgobj gobj2view = gobj_find_unique_gobj(gobj_name_, FALSE);
    if(!gobj2view) {
        gobj2view = gobj_find_gobj(gobj_name_);
        if(!gobj2view) {
            return msg_iev_build_webix(
                gobj,
                -1,
                json_sprintf(
                    "%s: gobj '%s' not found.", gobj_short_name(gobj), gobj_name_
                ),
                0,
                0,
                kw  // owned
            );
        }
    }

    json_t *jn_data = attr2json(gobj2view);
    return msg_iev_build_webix(
        gobj,
        0,
        0,
        0,
        jn_data,
        kw  // owned
    );
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE json_t *cmd_webix_gobj_tree(hgobj gobj, const char *cmd, json_t *kw, hgobj src)
{
    const char *gobj_name_ = kw_get_str( // __yuno__
        kw,
        "gobj_name",
        kw_get_str(kw, "gobj", "", 0),
        0
    );
    hgobj gobj2view = gobj_find_unique_gobj(gobj_name_, FALSE);
    if(!gobj2view) {
        gobj2view = gobj_find_gobj(gobj_name_);
        if(!gobj2view) {
            return msg_iev_build_webix(
                gobj,
                -1,
                json_sprintf(
                    "%s: gobj '%s' not found.", gobj_short_name(gobj), gobj_name_
                ),
                0,
                0,
                kw  // owned
            );
        }
    }

    json_t *jn_data = webix_gobj_tree(gobj2view);
    return msg_iev_build_webix(
        gobj,
        0,
        0,
        0,
        jn_data,
        kw  // owned
    );
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE json_t *cmd_view_gobj_tree(hgobj gobj, const char *cmd, json_t *kw, hgobj src)
{
    const char *gobj_name_ = kw_get_str( // __yuno__
        kw,
        "gobj_name",
        kw_get_str(kw, "gobj", "", 0),
        0
    );
    hgobj gobj2view = gobj_find_unique_gobj(gobj_name_, FALSE);
    if(!gobj2view) {
        gobj2view = gobj_find_gobj(gobj_name_);
        if(!gobj2view) {
            return msg_iev_build_webix(
                gobj,
                -1,
                json_sprintf(
                    "%s: gobj '%s' not found.", gobj_short_name(gobj), gobj_name_
                ),
                0,
                0,
                kw  // owned
            );
        }
    }

    json_t *jn_data = view_gobj_tree(gobj2view);
    return msg_iev_build_webix(
        gobj,
        0,
        0,
        0,
        jn_data,
        kw  // owned
    );
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE json_t *cmd_view_gobj_treedb(hgobj gobj, const char *cmd, json_t *kw, hgobj src)
{
    const char *gobj_name_ = kw_get_str( // __yuno__
        kw,
        "gobj_name",
        kw_get_str(kw, "gobj", "", 0),
        0
    );
    hgobj gobj2view = gobj_find_unique_gobj(gobj_name_, FALSE);
    if(!gobj2view) {
        gobj2view = gobj_find_gobj(gobj_name_);
        if(!gobj2view) {
            return msg_iev_build_webix(
                gobj,
                -1,
                json_sprintf(
                    "%s: gobj '%s' not found.", gobj_short_name(gobj), gobj_name_
                ),
                0,
                0,
                kw  // owned
            );
        }
    }

    json_t *jn_data = gobj_gobjs_treedb_data(gobj2view);
    return msg_iev_build_webix(
        gobj,
        0,
        0,
        0, // TODO kw_incref(gobj_gobjs_treedb_schema("gobj")),
        jn_data,
        kw  // owned
    );
}

/***************************************************************************
 *  Show register
 ***************************************************************************/
PRIVATE json_t *cmd_view_gclass_register(hgobj gobj, const char *cmd, json_t *kw, hgobj src)
{
    return msg_iev_build_webix(
        gobj,
        0,
        0,
        0,
        gobj_repr_gclass_register(),
        kw  // owned
    );
}

/***************************************************************************
 *  Show register
 ***************************************************************************/
PRIVATE json_t *cmd_view_service_register(hgobj gobj, const char *cmd, json_t *kw, hgobj src)
{
    const char *gclass_name = kw_get_str(
        kw,
        "gclass_name",
        kw_get_str(kw, "gclass", "", 0),
        0
    );


    return msg_iev_build_webix(
        gobj,
        0,
        0,
        0,
        gobj_repr_service_register(gclass_name),
        kw  // owned
    );
}

/***************************************************************************
 *  Show register
 ***************************************************************************/
PRIVATE json_t *cmd_view_unique_register(hgobj gobj, const char *cmd, json_t *kw, hgobj src)
{
    return msg_iev_build_webix(
        gobj,
        0,
        0,
        0,
        gobj_repr_unique_register(),
        kw  // owned
    );
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE json_t *cmd_list_childs(hgobj gobj, const char *cmd, json_t *kw, hgobj src)
{
    const char *gobj_name_ = kw_get_str(
        kw,
        "gobj_name",
        kw_get_str(kw, "gobj", "", 0),
        0
    );
    const char *child_gclass = kw_get_str(kw, "child_gclass", "", 0);
    const char *_attributes = kw_get_str(kw, "attributes", "", 0);

    if(empty_string(gobj_name_)) {
        return msg_iev_build_webix(
            gobj,
            -1,
            json_sprintf(
                "%s: what gobj?", gobj_short_name(gobj)
            ),
            0,
            0,
            kw  // owned
        );
    }
    if(empty_string(child_gclass)) {
        return msg_iev_build_webix(
            gobj,
            -1,
            json_sprintf(
                "%s: what child_gclass?", gobj_short_name(gobj)
            ),
            0,
            0,
            kw  // owned
        );
    }
    if(empty_string(_attributes)) {
        return msg_iev_build_webix(
            gobj,
            -1,
            json_sprintf(
                "%s: what attributes?", gobj_short_name(gobj)
            ),
            0,
            0,
            kw  // owned
        );
    }

    int max_attr_list = 32;
    char *attr_list[max_attr_list+1];
    memset(attr_list, 0, sizeof(attr_list));
    split(_attributes, "| ,;", attr_list, max_attr_list);

    json_t *jn_data = json_array();

    int max_gobj_list = 6;
    char *gobj_list[max_gobj_list+1];
    memset(gobj_list, 0, sizeof(gobj_list));
    int matched = split(gobj_name_, "| ,;", gobj_list, max_gobj_list);
    for(int i=0; i<matched; i++) {
        char *gobj_name = gobj_list[i];
        hgobj gobj2view = gobj_find_unique_gobj(gobj_name, FALSE);
        if(!gobj2view) {
            gobj2view = gobj_find_gobj(gobj_name);
            if(!gobj2view) {
                continue;
            }
        }
        json_t *jn_childs = gobj_list_childs(gobj2view, child_gclass, (const char **)attr_list);
        if(jn_childs) {
            json_array_extend(jn_data, jn_childs);
            json_decref(jn_childs);
        }
    }

    split_free(attr_list, max_attr_list);
    split_free(gobj_list, max_gobj_list);

    return msg_iev_build_webix(
        gobj,
        0,
        0,
        0,
        jn_data,
        kw  // owned
    );
}

/***************************************************************************
 *  GUI usage
 ***************************************************************************/
PRIVATE json_t *cmd_list_channels(hgobj gobj, const char *cmd, json_t *kw, hgobj src)
{
    BOOL opened = kw_get_bool(kw, "opened", 0, KW_WILD_NUMBER);

    json_t *jn_data = list_gclass_gobjs(gobj_yuno(), "Channel");
    if(opened) {
        json_t *jn_data2 = json_array();
        int idx; json_t *channel;
        json_array_foreach(jn_data, idx, channel) {
            BOOL opened_ = kw_get_bool(channel, "attrs`opened", 0, KW_REQUIRED);
            if(opened_) {
                json_array_append(jn_data2, channel);
            }
        }
        JSON_DECREF(jn_data);
        jn_data = jn_data2;
    }
    return msg_iev_build_webix(
        gobj,
        0,
        0,
        0,
        jn_data,
        kw  // owned
    );
}

/***************************************************************************
 *  Write a boolean attribute
 ***************************************************************************/
PRIVATE json_t *cmd_write_bool(hgobj gobj, const char *cmd, json_t *kw, hgobj src)
{
    const char *gobj_name_ = kw_get_str( // __default_service__
        kw,
        "gobj_name",
        kw_get_str(kw, "gobj", "", 0),
        0
    );
    hgobj gobj2write = gobj_find_unique_gobj(gobj_name_, FALSE);
    if(!gobj2write) {
        gobj2write = gobj_find_gobj(gobj_name_);
        if(!gobj2write) {
            return msg_iev_build_webix(
                gobj,
                -1,
                json_sprintf(
                    "%s: gobj '%s' not found.", gobj_short_name(gobj), gobj_name_
                ),
                0,
                0,
                kw  // owned
            );
        }
    }

    const char *attribute = kw_get_str(kw, "attribute", 0, 0);
    if(empty_string(attribute)) {
        return msg_iev_build_webix(
            gobj,
            -1,
            json_sprintf(
                "%s: what attribute?", gobj_short_name(gobj)
            ),
            0,
            0,
            kw  // owned
        );
    }
    const char *bool_value = kw_get_str(kw, "value", 0, 0);
    if(empty_string(bool_value)) {
        return msg_iev_build_webix(
            gobj,
            -1,
            json_sprintf(
                "%s: what value?", gobj_short_name(gobj)
            ),
            0,
            0,
            kw  // owned
        );
    }

    if(!gobj_has_attr(gobj2write, attribute)) {
        return msg_iev_build_webix(
            gobj,
            -1,
            json_sprintf(
                "%s: attr '%s.%s' non-existent",
                gobj_short_name(gobj),
                gobj_name_,
                attribute
            ),
            0,
            0,
            kw  // owned
        );
    }
    if(!gobj_is_writable_attr(gobj2write, attribute)) {
        return msg_iev_build_webix(
            gobj,
            -1,
            json_sprintf(
                "%s: '%s.%s' attr is not writable",
                gobj_short_name(gobj),
                gobj_name_,
                attribute
            ),
            0,
            0,
            kw  // owned
        );
    }

    BOOL value;
    if(strcasecmp(bool_value, "true")==0) {
        value = 1;
    } else if(strcasecmp(bool_value, "false")==0) {
        value = 0;
    } else {
        value = atoi(bool_value);
    }

    int ret = gobj_write_bool_attr(gobj2write, attribute, value);

    if(ret<0) {
        return msg_iev_build_webix(
            gobj,
            -1,
            json_sprintf(
                "%s: can't write '%s.%s'",
                gobj_short_name(gobj),
                gobj_name_,
                attribute
            ),
            0,
            0,
            kw  // owned
        );
    }
    gobj_save_persistent_attrs(gobj2write, json_string(attribute));

    json_t *jn_data = sdata2json(gobj_hsdata(gobj2write), ATTR_READABLE|ATTR_WRITABLE, 0);
    return msg_iev_build_webix(
        gobj,
        0,
        json_sprintf(
            "%s: %s.%s=%d done",
            gobj_short_name(gobj),
            gobj_name_,
            attribute,
            value
        ),
        0,
        jn_data,
        kw  // owned
    );
}

/***************************************************************************
 *  Write a str attribute
 ***************************************************************************/
PRIVATE json_t *cmd_write_str(hgobj gobj, const char *cmd, json_t *kw, hgobj src)
{
    const char *gobj_name_ = kw_get_str( // __default_service__
        kw,
        "gobj_name",
        kw_get_str(kw, "gobj", "", 0),
        0
    );
    hgobj gobj2write = gobj_find_unique_gobj(gobj_name_, FALSE);
    if(!gobj2write) {
        gobj2write = gobj_find_gobj(gobj_name_);
        if(!gobj2write) {
            return msg_iev_build_webix(
                gobj,
                -1,
                json_sprintf(
                    "%s: gobj '%s' not found.", gobj_short_name(gobj), gobj_name_
                ),
                0,
                0,
                kw  // owned
            );
        }
    }

    const char *attribute = kw_get_str(kw, "attribute", 0, 0);
    if(empty_string(attribute)) {
        return msg_iev_build_webix(
            gobj,
            -1,
            json_sprintf(
                "%s: what attribute?", gobj_short_name(gobj)
            ),
            0,
            0,
            kw  // owned
        );
    }
    const char *str_value = kw_get_str(kw, "value", 0, 0);
    if(!str_value) {
        return msg_iev_build_webix(
            gobj,
            -1,
            json_sprintf(
                "%s: what value?", gobj_short_name(gobj)
            ),
            0,
            0,
            kw  // owned
        );
    }

    if(!gobj_has_attr(gobj2write, attribute)) {
        return msg_iev_build_webix(
            gobj,
            -1,
            json_sprintf(
                "%s: attr '%s.%s' non-existent",
                gobj_short_name(gobj),
                gobj_name_,
                attribute
            ),
            0,
            0,
            kw  // owned
        );
    }
    if(!gobj_is_writable_attr(gobj2write, attribute)) {
        return msg_iev_build_webix(
            gobj,
            -1,
            json_sprintf(
                "%s: '%s.%s' attr is not writable",
                gobj_short_name(gobj),
                gobj_name_,
                attribute
            ),
            0,
            0,
            kw  // owned
        );
    }

    int ret = gobj_write_str_attr(gobj2write, attribute, str_value);

    if(ret<0) {
        return msg_iev_build_webix(
            gobj,
            -1,
            json_sprintf(
                "%s: can't write '%s.%s'",
                gobj_short_name(gobj),
                gobj_name_,
                attribute
            ),
            0,
            0,
            kw  // owned
        );
    }
    gobj_save_persistent_attrs(gobj2write, json_string(attribute));

    json_t *jn_data = sdata2json(gobj_hsdata(gobj2write), ATTR_READABLE|ATTR_WRITABLE, 0);
    return msg_iev_build_webix(
        gobj,
        0,
        json_sprintf(
            "%s: %s.%s=%s done",
            gobj_short_name(gobj),
            gobj_name_,
            attribute,
            str_value
        ),
        0,
        jn_data,
        kw  // owned
    );
}

/***************************************************************************
 *  Write a json attribute
 ***************************************************************************/
PRIVATE json_t *cmd_write_json(hgobj gobj, const char *cmd, json_t *kw, hgobj src)
{
    const char *gobj_name_ = kw_get_str( // __default_service__
        kw,
        "gobj_name",
        kw_get_str(kw, "gobj", "", 0),
        0
    );
    hgobj gobj2write = gobj_find_unique_gobj(gobj_name_, FALSE);
    if(!gobj2write) {
        gobj2write = gobj_find_gobj(gobj_name_);
        if(!gobj2write) {
            return msg_iev_build_webix(
                gobj,
                -1,
                json_sprintf(
                    "%s: gobj '%s' not found.", gobj_short_name(gobj), gobj_name_
                ),
                0,
                0,
                kw  // owned
            );
        }
    }

    const char *attribute = kw_get_str(kw, "attribute", 0, 0);
    if(empty_string(attribute)) {
        return msg_iev_build_webix(
            gobj,
            -1,
            json_sprintf(
                "%s: what attribute?", gobj_short_name(gobj)
            ),
            0,
            0,
            kw  // owned
        );
    }
    const char *str_value = kw_get_str(kw, "value", 0, 0);
    if(!str_value) {
        return msg_iev_build_webix(
            gobj,
            -1,
            json_sprintf(
                "%s: what value?", gobj_short_name(gobj)
            ),
            0,
            0,
            kw  // owned
        );
    }

    if(!gobj_has_attr(gobj2write, attribute)) {
        return msg_iev_build_webix(
            gobj,
            -1,
            json_sprintf(
                "%s: attr '%s.%s' non-existent",
                gobj_short_name(gobj),
                gobj_name_,
                attribute
            ),
            0,
            0,
            kw  // owned
        );
    }
    if(!gobj_is_writable_attr(gobj2write, attribute)) {
        return msg_iev_build_webix(
            gobj,
            -1,
            json_sprintf(
                "%s: '%s.%s' attr is not writable",
                gobj_short_name(gobj),
                gobj_name_,
                attribute
            ),
            0,
            0,
            kw  // owned
        );
    }

    json_t *jn_value = str2json(str_value, FALSE);
    if(!jn_value) {
        return msg_iev_build_webix(
            gobj,
            -1,
            json_sprintf(
                "%s: '%s.%s' cannot encode value string to json: '%s'",
                gobj_short_name(gobj),
                gobj_name_,
                attribute,
                str_value
            ),
            0,
            0,
            kw  // owned
        );
    }
    int ret = gobj_write_new_json_attr(gobj2write, attribute, jn_value);
    if(ret<0) {
        return msg_iev_build_webix(
            gobj,
            -1,
            json_sprintf(
                "%s: can't write '%s.%s'",
                gobj_short_name(gobj),
                gobj_name_,
                attribute
            ),
            0,
            0,
            kw  // owned
        );
    }
    gobj_save_persistent_attrs(gobj2write, json_string(attribute));

    json_t *jn_data = sdata2json(gobj_hsdata(gobj2write), ATTR_READABLE|ATTR_WRITABLE, 0);
    return msg_iev_build_webix(
        gobj,
        0,
        json_sprintf(
            "%s: %s.%s=%s done",
            gobj_short_name(gobj),
            gobj_name_,
            attribute,
            str_value
        ),
        0,
        jn_data,
        kw  // owned
    );
}

/***************************************************************************
 *  Write a numeric attribute
 ***************************************************************************/
PRIVATE json_t *cmd_write_num(hgobj gobj, const char *cmd, json_t *kw, hgobj src)
{
    const char *gobj_name_ = kw_get_str( // __default_service__
        kw,
        "gobj_name",
        kw_get_str(kw, "gobj", "", 0),
        0
    );
    hgobj gobj2write = gobj_find_unique_gobj(gobj_name_, FALSE);
    if(!gobj2write) {
        gobj2write = gobj_find_gobj(gobj_name_);
        if(!gobj2write) {
            return msg_iev_build_webix(
                gobj,
                -1,
                json_sprintf(
                    "%s: gobj '%s' not found.", gobj_short_name(gobj), gobj_name_
                ),
                0,
                0,
                kw  // owned
            );
        }
    }

    const char *attribute = kw_get_str(kw, "attribute", 0, 0);
    if(empty_string(attribute)) {
        return msg_iev_build_webix(
            gobj,
            -1,
            json_sprintf(
                "%s: what attribute?", gobj_short_name(gobj)
            ),
            0,
            0,
            kw  // owned
        );
    }
    const char *int_value = kw_get_str(kw, "value", 0, 0);
    if(empty_string(int_value)) {
        return msg_iev_build_webix(
            gobj,
            -1,
            json_sprintf(
                "%s: what value?", gobj_short_name(gobj)
            ),
            0,
            0,
            kw  // owned
        );
    }

    if(!gobj_has_attr(gobj2write, attribute)) {
        return msg_iev_build_webix(
            gobj,
            -1,
            json_sprintf(
                "%s: attr '%s.%s' non-existent",
                gobj_short_name(gobj),
                gobj_name_,
                attribute
            ),
            0,
            0,
            kw  // owned
        );
    }
    if(!gobj_is_writable_attr(gobj2write, attribute)) {
        return msg_iev_build_webix(
            gobj,
            -1,
            json_sprintf(
                "%s: '%s.%s' attr is not writable",
                gobj_short_name(gobj),
                gobj_name_,
                attribute
            ),
            0,
            0,
            kw  // owned
        );
    }

    int ret = -1;
    int type = gobj_attr_type(gobj2write, attribute);

    if(ASN_IS_UNSIGNED32(type)) {
        uint32_t value;
        value = strtoul(int_value, 0, 10);
        ret = gobj_write_uint32_attr(gobj2write, attribute, value);
    } else if(ASN_IS_SIGNED32(type)) {
        int32_t value;
        value = strtol(int_value, 0, 10);
        ret = gobj_write_int32_attr(gobj2write, attribute, value);
    } else if(ASN_IS_UNSIGNED64(type)) {
        uint64_t value;
        value = strtoull(int_value, 0, 10);
        ret = gobj_write_uint64_attr(gobj2write, attribute, value);
    } else if(ASN_IS_SIGNED64(type)) {
        int64_t value;
        value = strtoll(int_value, 0, 10);
        ret = gobj_write_int64_attr(gobj2write, attribute, value);
    } else if(ASN_IS_DOUBLE(type)) {
        double value;
        value = strtod(int_value, 0);
        ret = gobj_write_real_attr(gobj2write, attribute, value);
    }

    if(ret<0) {
        return msg_iev_build_webix(
            gobj,
            -1,
            json_sprintf(
                "%s: can't write '%s.%s'",
                gobj_short_name(gobj),
                gobj_name_,
                attribute
            ),
            0,
            0,
            kw  // owned
        );
    }
    gobj_save_persistent_attrs(gobj2write, json_string(attribute));

    json_t *jn_data = sdata2json(gobj_hsdata(gobj2write), ATTR_READABLE|ATTR_WRITABLE, 0);
    return msg_iev_build_webix(
        gobj,
        0,
        json_sprintf(
            "%s: %s.%s=%s done",
            gobj_short_name(gobj),
            gobj_name_,
            attribute,
            int_value
        ),
        0,
        jn_data,
        kw  // owned
    );
}

/***************************************************************************
 *  Read a numeric attribute
 ***************************************************************************/
PRIVATE json_t *cmd_read_num(hgobj gobj, const char *cmd, json_t *kw, hgobj src)
{
    const char *gobj_name_ = kw_get_str( // __default_service__
        kw,
        "gobj_name",
        kw_get_str(kw, "gobj", "", 0),
        0
    );
    hgobj gobj2read = gobj_find_unique_gobj(gobj_name_, FALSE);
    if(!gobj2read) {
        gobj2read = gobj_find_gobj(gobj_name_);
        if(!gobj2read) {
            return msg_iev_build_webix(
                gobj,
                -1,
                json_sprintf(
                    "%s: gobj '%s' not found.", gobj_short_name(gobj), gobj_name_
                ),
                0,
                0,
                kw  // owned
            );
        }
    }

    const char *attribute = kw_get_str(kw, "attribute", 0, 0);
    if(empty_string(attribute)) {
        return msg_iev_build_webix(
            gobj,
            -1,
            json_sprintf(
                "%s: what attribute?", gobj_short_name(gobj)
            ),
            0,
            0,
            kw  // owned
        );
    }

    if(!gobj_has_attr(gobj2read, attribute)) {
        return msg_iev_build_webix(
            gobj,
            -1,
            json_sprintf(
                "%s: '%s.%s' attribute not found",
                gobj_short_name(gobj),
                gobj_name_,
                attribute
            ),
            0,
            0,
            kw  // owned
        );
    }

    if(!gobj_is_readable_attr(gobj2read, attribute)) {
        return msg_iev_build_webix(
            gobj,
            -1,
            json_sprintf(
                "%s: '%s.%s' attr is not readable",
                gobj_short_name(gobj),
                gobj_name_,
                attribute
            ),
            0,
            0,
            kw  // owned
        );
    }

    int type = gobj_attr_type(gobj2read, attribute);

    json_t *jn_value = 0;
    if(ASN_IS_UNSIGNED32(type)) {
        uint32_t value;
        value = gobj_read_uint32_attr(gobj2read, attribute);
        jn_value = json_integer(value);
    } else if(ASN_IS_SIGNED32(type)) {
        int32_t value;
        value = gobj_read_int32_attr(gobj2read, attribute);
        jn_value = json_integer(value);
    } else if(ASN_IS_UNSIGNED64(type)) {
        uint64_t value;
        value = gobj_read_uint64_attr(gobj2read, attribute);
        jn_value = json_integer(value);
    } else if(ASN_IS_SIGNED64(type)) {
        int64_t value;
        value = gobj_read_int64_attr(gobj2read, attribute);
        jn_value = json_integer(value);
    } else if(ASN_IS_DOUBLE(type)) {
        double value;
        value = gobj_read_real_attr(gobj2read, attribute);
        jn_value = json_real(value);
    }

    if(!jn_value) {
        return msg_iev_build_webix(
            gobj,
            -1,
            json_sprintf(
                "%s: '%s.%s' attr is not numeric",
                gobj_short_name(gobj),
                gobj_name_,
                attribute
            ),
            0,
            0,
            kw  // owned
        );
    }

    json_t *jn_data = json_pack("{s:o}",
        attribute, jn_value
    );
    append_yuno_metadata(gobj, jn_data, attribute);
    return msg_iev_build_webix(
        gobj,
        0,
        0,
        0,
        jn_data,
        kw  // owned
    );
}

/***************************************************************************
 *  Cpu's info
 ***************************************************************************/
PRIVATE json_t *cmd_info_cpus(hgobj gobj, const char *cmd, json_t *kw, hgobj src)
{
    int count;
    uv_cpu_info_t *cpu_infos;
    uv_cpu_info(&cpu_infos, &count);

    json_t *jn_stats = json_array();
    uv_cpu_info_t *pcpu = cpu_infos;
    for(int i = 0; i<count; i++, pcpu++) {
        json_t *jn_cpu = json_object();
        json_array_append_new(
            jn_stats,
            jn_cpu
        );
        json_object_set_new(
            jn_cpu,
            "model",
            json_string(pcpu->model)
        );
        json_object_set_new(
            jn_cpu,
            "speed",
            json_integer(pcpu->speed)
        );
        /*
         *  This times are multiply of Clock system
         */
        json_object_set_new(
            jn_cpu,
            "time_user",
            json_integer(pcpu->cpu_times.user)
        );
        json_object_set_new(
            jn_cpu,
            "time_nice",
            json_integer(pcpu->cpu_times.nice)
        );
        json_object_set_new(
            jn_cpu,
            "time_sys",
            json_integer(pcpu->cpu_times.sys)
        );
        json_object_set_new(
            jn_cpu,
            "time_idle",
            json_integer(pcpu->cpu_times.idle)
        );
        json_object_set_new(
            jn_cpu,
            "time_irq",
            json_integer(pcpu->cpu_times.irq)
        );
    }
    uv_free_cpu_info(cpu_infos, count);

    return msg_iev_build_webix(
        gobj,
        0,
        json_sprintf("Number of cpus: %d", count),
        0,
        jn_stats, // owned
        kw  // owned
    );
}

/***************************************************************************
 *  Ifs's info
 ***************************************************************************/
PRIVATE json_t *cmd_info_ifs(hgobj gobj, const char *cmd, json_t *kw, hgobj src)
{
    int count;
    uv_interface_address_t *addresses;
    uv_interface_addresses(&addresses, &count);

    json_t *jn_data = json_array();
    uv_interface_address_t *pif = addresses;
    for(int i = 0; i<count; i++, pif++) {
        json_t *jn_if = json_object();
        json_array_append_new(
            jn_data,
            jn_if
        );
        json_object_set_new(
            jn_if,
            "name",
            json_string(pif->name)
        );
        json_object_set_new(
            jn_if,
            "is_internal",
            pif->is_internal?json_true():json_false()
        );

        char temp[64];

        bin2hex(temp, sizeof(temp), (uint8_t *)pif->phys_addr, sizeof(pif->phys_addr));
        json_object_set_new(
            jn_if,
            "phys_addr",
            json_string(temp)
        );

        // TODO how check if it's a ipv4 or ipv6?
        if(1 || !all_00(pif->address.address4.sin_zero, sizeof(pif->address.address4.sin_zero))) {
            uv_ip4_name(&pif->address.address4, temp, sizeof(temp));
            json_object_set_new(
                jn_if,
                "ip-v4",
                json_string(temp)
            );
        }
        if(1 || !all_00(pif->address.address6.sin6_addr.s6_addr, sizeof(pif->address.address6.sin6_addr.s6_addr))) {
            uv_ip6_name(&pif->address.address6, temp, sizeof(temp));
            json_object_set_new(
                jn_if,
                "ip-v6",
                json_string(temp)
            );
        }
        if(1 || !all_ff(pif->netmask.netmask4.sin_zero, sizeof(pif->netmask.netmask4.sin_zero))) {
            uv_ip4_name(&pif->netmask.netmask4, temp, sizeof(temp));
            json_object_set_new(
                jn_if,
                "mask-v4",
                json_string(temp)
            );
        }
        if(1 || !all_ff(pif->netmask.netmask6.sin6_addr.s6_addr, sizeof(pif->netmask.netmask6.sin6_addr.s6_addr))) {
            uv_ip6_name(&pif->netmask.netmask6, temp, sizeof(temp));
            json_object_set_new(
                jn_if,
                "mask-v6",
                json_string(temp)
            );
        }
    }
    uv_free_interface_addresses(addresses, count);

    return msg_iev_build_webix(
        gobj,
        0,
        json_sprintf("Number of ifs: %d", count),
        0,
        jn_data, // owned
        kw  // owned
    );
}

/***************************************************************************
 *  OS's info
 ***************************************************************************/
PRIVATE json_t *cmd_info_os(hgobj gobj, const char *cmd, json_t *kw, hgobj src)
{
    uv_utsname_t uname;
    uv_os_uname(&uname);

    json_t *jn_data = json_pack("{s:s, s:s, s:s, s:s}",
        "sysname", uname.sysname,
        "release", uname.release,
        "version", uname.version,
        "machine", uname.machine
    );

    return msg_iev_build_webix(
        gobj,
        0,
        0,
        0,
        jn_data, // owned
        kw  // owned
    );
}

/***************************************************************************
 *  Info of global gclass' trace levels
 ***************************************************************************/
PRIVATE json_t *cmd_info_global_trace(hgobj gobj, const char *cmd, json_t *kw, hgobj src)
{
    return msg_iev_build_webix(
        gobj,
        0,
        0,
        0,
        gobj_repr_global_trace_levels(),
        kw  // owned
    );
}

/***************************************************************************
 *  View global gclass' trace levels
 ***************************************************************************/
PRIVATE json_t *cmd_get_global_trace(hgobj gobj, const char *cmd, json_t *kw, hgobj src)
{
    return msg_iev_build_webix(
        gobj,
        0,
        0,
        0,
        gobj_get_global_trace_level(),
        kw  // owned
    );
}

/***************************************************************************
 *  Info of gclass' trace levels
 ***************************************************************************/
PRIVATE json_t *cmd_info_gclass_trace(hgobj gobj, const char *cmd, json_t *kw, hgobj src)
{
    const char *gclass_name_ = kw_get_str(
        kw,
        "gclass_name",
        kw_get_str(kw, "gclass", "", 0),
        0
    );
    if(!empty_string(gclass_name_)) {
        GCLASS *gclass = gobj_find_gclass(gclass_name_, FALSE);
        if(!gclass) {
            gclass = _get_gclass_from_gobj(gclass_name_);
            if(!gclass) {
                return msg_iev_build_webix(
                    gobj,
                    -1,
                    json_sprintf(
                        "%s: what gclass is '%s'?", gobj_short_name(gobj), gclass_name_
                    ),
                    0,
                    0,
                    kw  // owned
                );
            }
            gclass_name_ = gclass->gclass_name;
        }
    }

    return msg_iev_build_webix(
        gobj,
        0,
        0,
        0,
        gobj_repr_gclass_trace_levels(gclass_name_),
        kw  // owned
    );
}

/***************************************************************************
 *  View gclass trace
 ***************************************************************************/
PRIVATE json_t *cmd_get_gclass_trace(hgobj gobj, const char *cmd, json_t *kw, hgobj src)
{
    const char *gclass_name_ = kw_get_str(
        kw,
        "gclass_name",
        kw_get_str(kw, "gclass", "", 0),
        0
    );
    GCLASS *gclass = 0;
    if(!empty_string(gclass_name_)) {
        gclass = gobj_find_gclass(gclass_name_, FALSE);
        if(!gclass) {
            gclass = _get_gclass_from_gobj(gclass_name_);
            if(!gclass) {
                return msg_iev_build_webix(
                    gobj,
                    -1,
                    json_sprintf(
                        "%s: what gclass is '%s'?", gobj_short_name(gobj), gclass_name_
                    ),
                    0,
                    0,
                    kw  // owned
                );
            }
        }
    }

    json_t *jn_data = gobj_get_gclass_trace_level_list(gclass);
    return msg_iev_build_webix(
        gobj,
        0,
        json_sprintf("%d gclass with some trace", (int)json_array_size(jn_data)),
        0,
        jn_data,
        kw  // owned
    );
}

/***************************************************************************
 *  View gclass no-trace
 ***************************************************************************/
PRIVATE json_t *cmd_get_gclass_no_trace(hgobj gobj, const char *cmd, json_t *kw, hgobj src)
{
    const char *gclass_name_ = kw_get_str(
        kw,
        "gclass_name",
        kw_get_str(kw, "gclass", "", 0),
        0
    );
    GCLASS *gclass = 0;
    if(!empty_string(gclass_name_)) {
        gclass = gobj_find_gclass(gclass_name_, FALSE);
        if(!gclass) {
            gclass = _get_gclass_from_gobj(gclass_name_);
            if(!gclass) {
                return msg_iev_build_webix(
                    gobj,
                    -1,
                    json_sprintf(
                        "%s: what gclass is '%s'?", gobj_short_name(gobj), gclass_name_
                    ),
                    0,
                    0,
                    kw  // owned
                );
            }
        }
    }

    json_t *jn_data = gobj_get_gclass_no_trace_level_list(gclass);
    return msg_iev_build_webix(
        gobj,
        0,
        json_sprintf("%d gclass with some no trace", (int)json_array_size(jn_data)),
        0,
        jn_data,
        kw  // owned
    );
}

/***************************************************************************
 *  Info gobj trace
 ***************************************************************************/
PRIVATE json_t *cmd_info_gobj_trace(hgobj gobj, const char *cmd, json_t *kw, hgobj src)
{
    const char *gobj_name_ = kw_get_str( // __yuno__
        kw,
        "gobj_name",
        kw_get_str(kw, "gobj", "", 0),
        0
    );
    hgobj gobj2view = gobj_find_unique_gobj(gobj_name_, FALSE);
    if(!gobj2view) {
        gobj2view = gobj_find_gobj(gobj_name_);
        if(!gobj2view) {
            return msg_iev_build_webix(
                gobj,
                -1,
                json_sprintf(
                    "%s: gobj '%s' not found.", gobj_short_name(gobj), gobj_name_
                ),
                0,
                0,
                kw  // owned
            );
        }
    }

    return msg_iev_build_webix(
        gobj,
        0,
        0,
        0,
        gobj_repr_gclass_trace_levels(gobj_gclass_name(gobj2view)),
        kw  // owned
    );
}

/***************************************************************************
 *  View gobj trace
 ***************************************************************************/
PRIVATE json_t *cmd_get_gobj_trace(hgobj gobj, const char *cmd, json_t *kw, hgobj src)
{
    const char *gobj_name_ = kw_get_str( // __yuno__
        kw,
        "gobj_name",
        kw_get_str(kw, "gobj", "", 0),
        0
    );
    hgobj gobj2view = gobj_find_unique_gobj(gobj_name_, FALSE);
    if(!gobj2view) {
        gobj2view = gobj_find_gobj(gobj_name_);
        if(!gobj2view) {
            return msg_iev_build_webix(
                gobj,
                -1,
                json_sprintf(
                    "%s: gobj '%s' not found.", gobj_short_name(gobj), gobj_name_
                ),
                0,
                0,
                kw  // owned
            );
        }
    }
    json_t *jn_data = gobj_get_gobj_trace_level_tree(gobj2view);

    return msg_iev_build_webix(
        gobj,
        0,
        json_sprintf("%d gobjs with some trace", (int)json_array_size(jn_data)),
        0,
        jn_data,
        kw  // owned
    );
}

/***************************************************************************
 *  View gobj no-trace
 ***************************************************************************/
PRIVATE json_t *cmd_get_gobj_no_trace(hgobj gobj, const char *cmd, json_t *kw, hgobj src)
{
    const char *gobj_name_ = kw_get_str( // __yuno__
        kw,
        "gobj_name",
        kw_get_str(kw, "gobj", "", 0),
        0
    );
    hgobj gobj2view = gobj_find_unique_gobj(gobj_name_, FALSE);
    if(!gobj2view) {
        gobj2view = gobj_find_gobj(gobj_name_);
        if(!gobj2view) {
            return msg_iev_build_webix(
                gobj,
                -1,
                json_sprintf(
                    "%s: gobj '%s' not found.", gobj_short_name(gobj), gobj_name_
                ),
                0,
                0,
                kw  // owned
            );
        }
    }
    json_t *jn_data = gobj_get_gobj_no_trace_level_tree(gobj2view);

    return msg_iev_build_webix(
        gobj,
        0,
        json_sprintf("%d gobjs with some no trace", (int)json_array_size(jn_data)),
        0,
        jn_data,
        kw  // owned
    );
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE json_t *cmd_add_trace_filter(hgobj gobj, const char *cmd, json_t *kw, hgobj src)
{
    const char *attr = kw_get_str(kw, "attr", 0, 0);
    const char *value = kw_get_str(kw, "value", 0, 0);

    if(empty_string(attr)) {
        return msg_iev_build_webix(gobj,
            -1,
            json_sprintf("what attr?"),
            0,
            0,
            kw  // owned
        );
    }
    if(empty_string(value)) {
        return msg_iev_build_webix(gobj,
            -1,
            json_sprintf("what value?"),
            0,
            0,
            kw  // owned
        );
    }

    int ret = gobj_add_trace_filter(attr, value);
    save_trace_filters(gobj);

    json_t *jn_filters = gobj_get_trace_filter(); // Return is not YOURS

    return msg_iev_build_webix(
        gobj,
        ret,
        json_sprintf("Adding filter '%s':'%s' %s", attr, value, ret<0?"ERROR":"Ok"),
        0,
        json_incref(jn_filters),
        kw  // owned
    );
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE json_t *cmd_remove_trace_filter(hgobj gobj, const char *cmd, json_t *kw, hgobj src)
{
    const char *attr = kw_get_str(kw, "attr", 0, 0);
    const char *value = kw_get_str(kw, "value", 0, 0);
    BOOL all = kw_get_bool(kw, "all", 0, KW_WILD_NUMBER);

    if(empty_string(attr) && !all) {
        return msg_iev_build_webix(gobj,
            -1,
            json_sprintf("what attr?"),
            0,
            0,
            kw  // owned
        );
    }
    if(empty_string(value) && !all) {
        return msg_iev_build_webix(gobj,
            -1,
            json_sprintf("what value?"),
            0,
            0,
            kw  // owned
        );
    }

    // If attr is empty then remove all filters, if value is empty then remove all values of attr
    int ret = gobj_remove_trace_filter(attr, value);
    save_trace_filters(gobj);

    json_t *jn_filters = gobj_get_trace_filter(); // Return is not YOURS

    return msg_iev_build_webix(
        gobj,
        ret,
        json_sprintf("Removing filter '%s':'%s' %s", attr, value, ret<0?"ERROR":"Ok"),
        0,
        json_incref(jn_filters),
        kw  // owned
    );
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE json_t *cmd_get_trace_filter(hgobj gobj, const char *cmd, json_t *kw, hgobj src)
{
    json_t *jn_filters = gobj_get_trace_filter(); // Return is not YOURS

    return msg_iev_build_webix(
        gobj,
        0,
        0,
        0,
        json_incref(jn_filters),
        kw  // owned
    );
}

/***************************************************************************
 *  Set glocal trace level
 ***************************************************************************/
PRIVATE json_t *cmd_set_global_trace(hgobj gobj, const char *cmd, json_t *kw, hgobj src)
{
    const char *level = kw_get_str(kw, "level", 0, 0);
    if(empty_string(level)) {
        return msg_iev_build_webix(
            gobj,
            -1,
            json_sprintf(
                "%s: what level?", gobj_short_name(gobj)
            ),
            0,
            0,
            kw  // owned
        );
    }

    const char *trace_value = kw_get_str(kw, "set", 0, 0);
    if(empty_string(trace_value)) {
        return msg_iev_build_webix(
            gobj,
            -1,
            json_sprintf(
                "%s: bitmask set or re-set?", gobj_short_name(gobj)
            ),
            0,
            0,
            kw  // owned
        );
    }

    BOOL trace;
    if(strcasecmp(trace_value, "true")==0 || strcasecmp(trace_value, "set")==0) {
        trace = 1;
    } else if(strcasecmp(trace_value, "false")==0 || strcasecmp(trace_value, "reset")==0) {
        trace = 0;
    } else {
        trace = atoi(trace_value);
    }

    if(gobj_set_global_trace(level, trace)==0) {
        save_global_trace(gobj, level, trace?1:0, TRUE);
    }

    json_t *jn_data = gobj_get_global_trace_level();
    return msg_iev_build_webix(
        gobj,
        0,
        0,
        0,
        jn_data,
        kw  // owned
    );
}

/***************************************************************************
 *  Set gclass trace level
 ***************************************************************************/
PRIVATE json_t *cmd_set_gclass_trace(hgobj gobj, const char *cmd, json_t *kw, hgobj src)
{
    const char *gclass_name_ = kw_get_str(
        kw,
        "gclass_name",
        kw_get_str(kw, "gclass", "", 0),
        0
    );
    GCLASS *gclass = gobj_find_gclass(gclass_name_, FALSE);
    if(!gclass) {
        gclass = _get_gclass_from_gobj(gclass_name_);
        if(!gclass) {
            return msg_iev_build_webix(
                gobj,
                -1,
                json_sprintf(
                    "%s: what gclass is '%s'?", gobj_short_name(gobj), gclass_name_
                ),
                0,
                0,
                kw  // owned
            );
        }
    }

    const char *level = kw_get_str(kw, "level", 0, 0);
    if(empty_string(level)) {
        return msg_iev_build_webix(
            gobj,
            -1,
            json_sprintf(
                "%s: what level?", gobj_short_name(gobj)
            ),
            0,
            0,
            kw  // owned
        );
    }

    const char *trace_value = kw_get_str(kw, "set", 0, 0);
    if(empty_string(trace_value)) {
        return msg_iev_build_webix(
            gobj,
            -1,
            json_sprintf(
                "%s: bitmask set or re-set?", gobj_short_name(gobj)
            ),
            0,
            0,
            kw  // owned
        );
    }

    BOOL trace;
    if(strcasecmp(trace_value, "true")==0 || strcasecmp(trace_value, "set")==0) {
        trace = 1;
    } else if(strcasecmp(trace_value, "false")==0 || strcasecmp(trace_value, "reset")==0) {
        trace = 0;
    } else {
        trace = atoi(trace_value);
    }

    gobj_set_gclass_trace(gclass, level, trace);
    save_user_trace(gobj, gclass_name_, level, trace?1:0, TRUE);

    json_t *jn_data = gobj_get_gclass_trace_level(gclass);
    return msg_iev_build_webix(
        gobj,
        0,
        0,
        0,
        jn_data,
        kw  // owned
    );
}

/***************************************************************************
 *  Reset all gclass or gobj trace levels
 ***************************************************************************/
PRIVATE json_t *cmd_reset_all_traces(hgobj gobj, const char *cmd, json_t *kw, hgobj src)
{
    const char *gclass_name_ = kw_get_str(kw, "gclass", "", 0);
    const char *gobj_name_ = kw_get_str(kw, "gobj", "", 0);

    if(!empty_string(gclass_name_)) {
        GCLASS *gclass = gobj_find_gclass(gclass_name_, FALSE);
        if(!gclass) {
            gclass = _get_gclass_from_gobj(gclass_name_);
            if(!gclass) {
                return msg_iev_build_webix(
                    gobj,
                    -1,
                    json_sprintf(
                        "%s: what gclass is '%s'?", gobj_short_name(gobj), gclass_name_
                    ),
                    0,
                    0,
                    kw  // owned
                );
            }
        }

        json_t *levels = gobj_get_gclass_trace_level(gclass);
        int idx; json_t *jn_level;
        json_array_foreach(levels, idx, jn_level) {
            const char *level = json_string_value(jn_level);
            gobj_set_gclass_trace(gclass, level, 0);
            save_user_trace(gobj, gclass_name_, level, 0, FALSE);
        }
        json_decref(levels);

        gobj_save_persistent_attrs(gobj, json_string("trace_levels"));

        json_t *jn_data = gobj_get_gclass_trace_level(gclass);
        return msg_iev_build_webix(
            gobj,
            0,
            0,
            0,
            jn_data,
            kw  // owned
        );
    }

    if(!empty_string(gobj_name_)) {
        hgobj gobj2trace = gobj_find_unique_gobj(gobj_name_, FALSE);
        if(!gobj2trace) {
            gobj2trace = gobj_find_gobj(gobj_name_);
            if(!gobj2trace) {
                return msg_iev_build_webix(
                    gobj,
                    -1,
                    json_sprintf(
                        "%s: gobj '%s' not found.", gobj_short_name(gobj), gobj_name_
                    ),
                    0,
                    0,
                    kw  // owned
                );
            }
        }

        json_t *levels = gobj_get_gobj_trace_level(gobj2trace);
        int idx; json_t *jn_level;
        json_array_foreach(levels, idx, jn_level) {
            const char *level = json_string_value(jn_level);
            gobj_set_gobj_trace(gobj2trace, level, 0, 0);
            gobj_set_gclass_trace(gobj_gclass(gobj2trace), level, 0);
            save_user_trace(gobj, gobj_name_, level, 0, FALSE);
        }
        json_decref(levels);

        gobj_save_persistent_attrs(gobj, json_string("trace_levels"));

        json_t *jn_data = gobj_get_gobj_trace_level(gobj2trace);
        return msg_iev_build_webix(
            gobj,
            0,
            0,
            0,
            jn_data,
            kw  // owned
        );
    }

    return msg_iev_build_webix(
        gobj,
        -1,
        json_sprintf("What gclass or gobj?"),
        0,
        0,
        kw  // owned
    );
}

/***************************************************************************
 *  Set no gclass trace level
 ***************************************************************************/
PRIVATE json_t *cmd_set_no_gclass_trace(hgobj gobj, const char *cmd, json_t *kw, hgobj src)
{
    const char *gclass_name_ = kw_get_str(
        kw,
        "gclass_name",
        kw_get_str(kw, "gclass", "", 0),
        0
    );
    GCLASS *gclass = gobj_find_gclass(gclass_name_, FALSE);
    if(!gclass) {
        gclass = _get_gclass_from_gobj(gclass_name_);
        if(!gclass) {
            return msg_iev_build_webix(
                gobj,
                -1,
                json_sprintf(
                    "%s: what gclass is '%s'?", gobj_short_name(gobj), gclass_name_
                ),
                0,
                0,
                kw  // owned
            );
        }
    }

    const char *level = kw_get_str(kw, "level", 0, 0);
    if(empty_string(level)) {
        return msg_iev_build_webix(
            gobj,
            -1,
            json_sprintf(
                "%s: what level?", gobj_short_name(gobj)
            ),
            0,
            0,
            kw  // owned
        );
    }

    const char *trace_value = kw_get_str(kw, "set", 0, 0);
    if(empty_string(trace_value)) {
        return msg_iev_build_webix(
            gobj,
            -1,
            json_sprintf(
                "%s: bitmask set or re-set?", gobj_short_name(gobj)
            ),
            0,
            0,
            kw  // owned
        );
    }

    BOOL trace;
    if(strcasecmp(trace_value, "true")==0 || strcasecmp(trace_value, "set")==0) {
        trace = 1;
    } else if(strcasecmp(trace_value, "false")==0 || strcasecmp(trace_value, "reset")==0) {
        trace = 0;
    } else {
        trace = atoi(trace_value);
    }

    if(gobj_set_gclass_no_trace(gclass, level, trace)==0) {
        save_user_no_trace(gobj, gclass_name_, level, trace?1:0, TRUE);
    }

    json_t *jn_data = gobj_get_gclass_no_trace_level(gclass);
    return msg_iev_build_webix(
        gobj,
        0,
        0,
        0,
        jn_data,
        kw  // owned
    );
}

/***************************************************************************
 *  Set gobj trace level
 ***************************************************************************/
PRIVATE json_t *cmd_set_gobj_trace(hgobj gobj, const char *cmd, json_t *kw, hgobj src)
{
    const char *gobj_name_ = kw_get_str(
        kw,
        "gobj_name",
        kw_get_str(kw, "gobj", "", 0),
        0
    );
    hgobj gobj2trace = gobj_find_unique_gobj(gobj_name_, FALSE);
    if(!gobj2trace) {
        gobj2trace = gobj_find_gobj(gobj_name_);
        if(!gobj2trace) {
            return msg_iev_build_webix(
                gobj,
                -1,
                json_sprintf(
                    "%s: gobj '%s' not found.", gobj_short_name(gobj), gobj_name_
                ),
                0,
                0,
                kw  // owned
            );
        }
    }

    const char *level = kw_get_str(kw, "level", 0, 0);
    if(empty_string(level)) {
        return msg_iev_build_webix(
            gobj,
            -1,
            json_sprintf(
                "%s: what level?", gobj_short_name(gobj)
            ),
            0,
            0,
            kw  // owned
        );
    }

    const char *trace_value = kw_get_str(kw, "set", 0, 0);
    if(empty_string(trace_value)) {
        return msg_iev_build_webix(
            gobj,
            -1,
            json_sprintf(
                "%s: bitmask set or re-set?", gobj_short_name(gobj)
            ),
            0,
            0,
            kw  // owned
        );
    }

    BOOL trace;
    if(strcasecmp(trace_value, "true")==0 || strcasecmp(trace_value, "set")==0) {
        trace = 1;
    } else if(strcasecmp(trace_value, "false")==0 || strcasecmp(trace_value, "reset")==0) {
        trace = 0;
    } else {
        trace = atoi(trace_value);
    }

    KW_INCREF(kw);
    gobj_set_gobj_trace(gobj2trace, level, trace?1:0, kw);
    save_user_trace(gobj, gobj_name_, level, trace?1:0, TRUE);

    json_t *jn_data = gobj_get_gobj_trace_level(gobj2trace);
    return msg_iev_build_webix(
        gobj,
        0,
        0,
        0,
        jn_data,
        kw  // owned
    );
}

/***************************************************************************
 *  Set no gobj trace level
 ***************************************************************************/
PRIVATE json_t *cmd_set_no_gobj_trace(hgobj gobj, const char *cmd, json_t *kw, hgobj src)
{
    const char *gobj_name_ = kw_get_str(
        kw,
        "gobj_name",
        kw_get_str(kw, "gobj", "", 0),
        0
    );
    hgobj gobj2trace = gobj_find_unique_gobj(gobj_name_, FALSE);
    if(!gobj2trace) {
        gobj2trace = gobj_find_gobj(gobj_name_);
        if(!gobj2trace) {
            return msg_iev_build_webix(
                gobj,
                -1,
                json_sprintf(
                    "%s: gobj '%s' not found.", gobj_short_name(gobj), gobj_name_
                ),
                0,
                0,
                kw  // owned
            );
        }
    }

    const char *level = kw_get_str(kw, "level", 0, 0);
    if(empty_string(level)) {
        return msg_iev_build_webix(
            gobj,
            -1,
            json_sprintf(
                "%s: what level?", gobj_short_name(gobj)
            ),
            0,
            0,
            kw  // owned
        );
    }

    const char *trace_value = kw_get_str(kw, "set", 0, 0);
    if(empty_string(trace_value)) {
        return msg_iev_build_webix(
            gobj,
            -1,
            json_sprintf(
                "%s: bitmask set or re-set?", gobj_short_name(gobj)
            ),
            0,
            0,
            kw  // owned
        );
    }

    BOOL trace;
    if(strcasecmp(trace_value, "true")==0 || strcasecmp(trace_value, "set")==0) {
        trace = 1;
    } else if(strcasecmp(trace_value, "false")==0 || strcasecmp(trace_value, "reset")==0) {
        trace = 0;
    } else {
        trace = atoi(trace_value);
    }

    if(gobj_set_gobj_no_trace(gobj2trace, level, trace)==0) {
        save_user_no_trace(gobj, gobj_name_, level, trace?1:0, TRUE);
    }

    json_t *jn_data = gobj_get_gobj_no_trace_level(gobj2trace);
    return msg_iev_build_webix(
        gobj,
        0,
        0,
        0,
        jn_data,
        kw  // owned
    );
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE json_t *cmd_trunk_rotatory_file(hgobj gobj, const char *cmd, json_t *kw, hgobj src)
{
    rotatory_trunk(0); // WARNING trunk all files
    return msg_iev_build_webix(
        gobj,
        0,
        json_sprintf("%s: Trunk all rotatory files done.", gobj_short_name(gobj)),
        0,
        0,
        kw  // owned
    );
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE json_t *cmd_reset_log_counters(hgobj gobj, const char *cmd, json_t *kw, hgobj src)
{
    log_clear_counters();
    return msg_iev_build_webix(
        gobj,
        0,
        json_sprintf("%s: Log counters reset.", gobj_short_name(gobj)),
        0,
        0,
        kw  // owned
    );
}


/***************************************************************************
 *  Run a command
 ***************************************************************************/
PRIVATE json_t *cmd_spawn(hgobj gobj, const char *cmd, json_t *kw, hgobj src)
{
    const char *process = kw_get_str(kw, "process", 0, 0);
    if(empty_string(process)) {
        return msg_iev_build_webix(
            gobj,
            -1,
            json_sprintf("What process?"),
            0,
            0,
            kw  // owned
        );
    }
    int response_size = gbmem_get_maximum_block();
    char *response = gbmem_malloc(response_size);
    int ret = run_command(process, response, response_size);
    json_t *jn_response = json_sprintf("%s", response);
    gbmem_free(response);
    return msg_iev_build_webix(
        gobj,
        ret,
        jn_response,
        0,
        0,
        kw  // owned
    );
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE json_t* cmd_set_daemon_debug(hgobj gobj, const char* cmd, json_t* kw, hgobj src)
{
    const char *trace_value = kw_get_str(kw, "set", 0, 0);
    if(empty_string(trace_value)) {
        return msg_iev_build_webix(
            gobj,
            -1,
            json_sprintf(
                "%s: set or re-set?", gobj_short_name(gobj)
            ),
            0,
            0,
            kw  // owned
        );
    }
    BOOL trace;
    if(strcasecmp(trace_value, "true")==0 || strcasecmp(trace_value, "set")==0) {
        trace = 1;
    } else if(strcasecmp(trace_value, "false")==0 || strcasecmp(trace_value, "reset")==0) {
        trace = 0;
    } else {
        trace = atoi(trace_value);
    }

    daemon_set_debug_mode(trace);

    return msg_iev_build_webix(
        gobj,
        0,
        json_sprintf(
            "%s: daemon debug set to %d", gobj_short_name(gobj), trace
        ),
        0,
        0,
        kw  // owned
    );
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE json_t* cmd_set_deep_trace(hgobj gobj, const char* cmd, json_t* kw, hgobj src)
{
    const char *trace_value = kw_get_str(kw, "set", 0, 0);
    if(empty_string(trace_value)) {
        return msg_iev_build_webix(
            gobj,
            -1,
            json_sprintf(
                "%s: set or re-set?", gobj_short_name(gobj)
            ),
            0,
            0,
            kw  // owned
        );
    }
    BOOL trace;
    if(strcasecmp(trace_value, "true")==0 || strcasecmp(trace_value, "set")==0) {
        trace = 1;
    } else if(strcasecmp(trace_value, "false")==0 || strcasecmp(trace_value, "reset")==0) {
        trace = 0;
    } else {
        trace = atoi(trace_value);
    }

    gobj_write_int32_attr(gobj, "deep_trace", trace);
    gobj_set_deep_tracing(trace);
    gobj_save_persistent_attrs(gobj, json_string("deep_trace"));

    return msg_iev_build_webix(
        gobj,
        0,
        json_sprintf(
            "%s: daemon debug set to %d", gobj_short_name(gobj), trace
        ),
        0,
        0,
        kw  // owned
    );
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE json_t* cmd_add_log_handler(hgobj gobj, const char* cmd, json_t* kw, hgobj src)
{
    const char *handler_name = kw_get_str(kw, "name", "", KW_REQUIRED);
    const char *handler_type = kw_get_str(kw, "type", "", KW_REQUIRED);
    const char *handler_options_ = kw_get_str(kw, "options", "", 0);
    log_handler_opt_t handler_options = atoi(handler_options_);
    if(!handler_options) {
        handler_options = LOG_OPT_ALL;
    }

    int added = 0;

    if(empty_string(handler_name)) {
        return msg_iev_build_webix(gobj,
            -1,
            json_sprintf("What name?"),
            0,
            0,
            kw  // owned
        );
    }

    /*-------------------------------------*
     *      Check if already exists
     *-------------------------------------*/
    if(log_exist_handler(handler_name)) {
        return msg_iev_build_webix(gobj,
            -1,
            json_sprintf("Handler already exists: %s", handler_name),
            0,
            0,
            kw  // owned
        );
    }

    if(strcmp(handler_type, "file")==0) {
        return msg_iev_build_webix(gobj,
            -1,
            json_sprintf("Handler 'file' type not allowed"),
            0,
            0,
            kw  // owned
        );

    } else if(strcmp(handler_type, "udp")==0) {
        const char *url = kw_get_str(kw, "url", "", KW_REQUIRED);
        if(empty_string(url)) {
            return msg_iev_build_webix(gobj,
                -1,
                json_sprintf("What url?"),
                0,
                0,
                kw  // owned
            );
        }
        size_t bf_size = 0;                     // 0 = default 64K
        size_t udp_frame_size = 0;              // 0 = default 1500
        ouput_format_t output_format = 0;       // 0 = default OUTPUT_FORMAT_YUNETA
        const char *bindip = 0;

        KW_GET(bindip, bindip, kw_get_str)
        KW_GET(bf_size, bf_size, kw_get_int)
        KW_GET(udp_frame_size, udp_frame_size, kw_get_int)
        KW_GET(output_format, output_format, kw_get_int)

        udpc_t udpc = udpc_open(
            url,
            bindip,
            bf_size,
            udp_frame_size,
            output_format,
            TRUE    // exit on failure
        );
        if(udpc) {
            if(log_add_handler(handler_name, handler_type, handler_options, udpc)==0) {
                added++;
            } else {
                log_error(0,
                    "gobj",         "%s", gobj_full_name(gobj),
                    "function",     "%s", __FUNCTION__,
                    "msgset",       "%s", MSGSET_PARAMETER_ERROR,
                    "msg",          "%s", "log_add_handler() FAILED",
                    "handler_type", "%s", handler_type,
                    "url",          "%s", url,
                    NULL
                );
            }
        } else {
            log_error(0,
                "gobj",         "%s", gobj_full_name(gobj),
                "function",     "%s", __FUNCTION__,
                "msgset",       "%s", MSGSET_PARAMETER_ERROR,
                "msg",          "%s", "udpc_open() FAILED",
                "handler_type", "%s", handler_type,
                "url",          "%s", url,
                NULL
            );
        }
    } else {
        return msg_iev_build_webix(gobj,
            -1,
            json_sprintf("Unknown '%s' handler type.", handler_type),
            0,
            0,
            kw  // owned
        );
    }

    /*
     *  Inform
     */
    return msg_iev_build_webix(gobj,
        added>0?0:-1,
        json_sprintf("%s: %d handlers added.", gobj_short_name(gobj), added),
        0,
        0,
        kw  // owned
    );
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE json_t* cmd_del_log_handler(hgobj gobj, const char* cmd, json_t* kw, hgobj src)
{
    const char *handler_name = kw_get_str(kw, "name", "", KW_REQUIRED);
    if(empty_string(handler_name)) {
        return msg_iev_build_webix(gobj,
            -1,
            json_sprintf("%s: what name?", gobj_short_name(gobj)),
            0,
            0,
            kw  // owned
        );
    }
    int deletions = log_del_handler(handler_name);

    /*
     *  Inform
     */
    return msg_iev_build_webix(gobj,
        deletions>0?0:-1,
        json_sprintf("%s: %d handlers deleted.", gobj_short_name(gobj), deletions),
        0,
        0,
        kw  // owned
    );
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE json_t* cmd_list_log_handler(hgobj gobj, const char* cmd, json_t* kw, hgobj src)
{
    /*
     *  Inform
     */
    return msg_iev_build_webix(gobj,
        0,
        0,
        0,
        log_list_handlers(), // owned
        kw  // owned
    );
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE json_t* cmd_list_persistent_attrs(hgobj gobj, const char* cmd, json_t* kw, hgobj src)
{
    const char *gobj_name_ = kw_get_str(
        kw,
        "gobj_name",
        kw_get_str(kw, "gobj", "", 0),
        0
    );
    const char *attribute = kw_get_str(kw, "attribute", 0, 0);
    json_t *jn_attrs = attribute?json_string(attribute):0;

    if(empty_string(gobj_name_)) {
        /*
         *  Inform
         */
        return msg_iev_build_webix(gobj,
            0,
            0,
            0,
            gobj_list_persistent_attrs(0, jn_attrs), // owned
            kw  // owned
        );
    }

    hgobj gobj2view = gobj_find_unique_gobj(gobj_name_, FALSE);
    if(!gobj2view) {
        gobj2view = gobj_find_gobj(gobj_name_);
        if(!gobj2view) {
            JSON_DECREF(jn_attrs);
            return msg_iev_build_webix(
                gobj,
                -1,
                json_sprintf(
                    "%s: gobj '%s' not found.", gobj_short_name(gobj), gobj_name_
                ),
                0,
                0,
                kw  // owned
            );
        }
    }

    /*
     *  Inform
     */
    return msg_iev_build_webix(gobj,
        0,
        0,
        0,
        gobj_list_persistent_attrs(gobj2view, jn_attrs), // owned
        kw  // owned
    );
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE json_t* cmd_remove_persistent_attrs(hgobj gobj, const char* cmd, json_t* kw, hgobj src)
{
    const char *gobj_name_ = kw_get_str(
        kw,
        "gobj_name",
        kw_get_str(kw, "gobj", "", 0),
        0
    );
    const char *attribute = kw_get_str(kw, "attribute", 0, 0);
    BOOL all = kw_get_bool(kw, "__all__", 0, KW_WILD_NUMBER);

    if(empty_string(gobj_name_)) {
        return msg_iev_build_webix(gobj,
            -1,
            json_sprintf("what gobj_name?"),
            0,
            0,
            kw  // owned
        );
    }
    if(empty_string(attribute) && !all) {
        return msg_iev_build_webix(gobj,
            -1,
            json_sprintf("what attribute?"),
            0,
            0,
            kw  // owned
        );
    }

    hgobj gobj2view = gobj_find_unique_gobj(gobj_name_, FALSE);
    if(!gobj2view) {
        gobj2view = gobj_find_gobj(gobj_name_);
        if(!gobj2view) {
            return msg_iev_build_webix(
                gobj,
                -1,
                json_sprintf(
                    "%s: gobj '%s' not found.", gobj_short_name(gobj), gobj_name_
                ),
                0,
                0,
                kw  // owned
            );
        }
    }

    int ret = gobj_remove_persistent_attrs(gobj2view, json_string(attribute));

    /*
     *  Inform
     */
    return msg_iev_build_webix(gobj,
        ret,
        0,
        0,
        gobj_list_persistent_attrs(gobj2view, 0), // owned
        kw  // owned
    );
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE json_t* cmd_start_gobj_tree(hgobj gobj, const char* cmd, json_t* kw, hgobj src)
{
    const char *gobj_name_ = kw_get_str( // __default_service__
        kw,
        "gobj_name",
        kw_get_str(kw, "gobj", "", 0),
        0
    );
    hgobj gobj2view = gobj_find_unique_gobj(gobj_name_, FALSE);
    if(!gobj2view) {
        gobj2view = gobj_find_gobj(gobj_name_);
        if(!gobj2view) {
            return msg_iev_build_webix(
                gobj,
                -1,
                json_sprintf(
                    "%s: gobj '%s' not found.", gobj_short_name(gobj), gobj_name_
                ),
                0,
                0,
                kw  // owned
            );
        }
    }

    if(gobj2view == gobj_yuno()) {
        return msg_iev_build_webix(gobj,
            -1,
            json_sprintf("%s: __yuno__ forbidden.", gobj_short_name(gobj)),
            0,
            0,
            kw  // owned
        );
    }
    gobj_start_tree(gobj2view);

    /*
     *  Inform
     */
    return msg_iev_build_webix(gobj,
        0,
        json_sprintf("%s: '%s' tree started.", gobj_short_name(gobj), gobj_name_),
        0,
        0,
        kw  // owned
    );
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE json_t* cmd_stop_gobj_tree(hgobj gobj, const char* cmd, json_t* kw, hgobj src)
{
    const char *gobj_name_ = kw_get_str( // __default_service__
        kw,
        "gobj_name",
        kw_get_str(kw, "gobj", "", 0),
        0
    );
    hgobj gobj2view = gobj_find_unique_gobj(gobj_name_, FALSE);
    if(!gobj2view) {
        gobj2view = gobj_find_gobj(gobj_name_);
        if(!gobj2view) {
            return msg_iev_build_webix(
                gobj,
                -1,
                json_sprintf(
                    "%s: gobj '%s' not found.", gobj_short_name(gobj), gobj_name_
                ),
                0,
                0,
                kw  // owned
            );
        }
    }

    if(gobj2view == gobj_yuno()) {
        return msg_iev_build_webix(gobj,
            -1,
            json_sprintf("%s: __yuno__ forbidden.", gobj_short_name(gobj)),
            0,
            0,
            kw  // owned
        );
    }
    gobj_stop_tree(gobj2view);

    /*
     *  Inform
     */
    return msg_iev_build_webix(gobj,
        0,
        json_sprintf("%s: '%s' tree stopped.", gobj_short_name(gobj), gobj_name_),
        0,
        0,
        kw  // owned
    );
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE json_t* cmd_enable_gobj(hgobj gobj, const char* cmd, json_t* kw, hgobj src)
{
    const char *gobj_name_ = kw_get_str( // __default_service__
        kw,
        "gobj_name",
        kw_get_str(kw, "gobj", "", 0),
        0
    );
    hgobj gobj2view = gobj_find_unique_gobj(gobj_name_, FALSE);
    if(!gobj2view) {
        gobj2view = gobj_find_gobj(gobj_name_);
        if(!gobj2view) {
            return msg_iev_build_webix(
                gobj,
                -1,
                json_sprintf(
                    "%s: gobj '%s' not found.", gobj_short_name(gobj), gobj_name_
                ),
                0,
                0,
                kw  // owned
            );
        }
    }

    if(gobj2view == gobj_yuno()) {
        return msg_iev_build_webix(gobj,
            -1,
            json_sprintf("%s: __yuno__ forbidden.", gobj_short_name(gobj)),
            0,
            0,
            kw  // owned
        );
    }
    gobj_enable(gobj2view);

    /*
     *  Inform
     */
    return msg_iev_build_webix(gobj,
        0,
        json_sprintf("%s: '%s' enabled.", gobj_short_name(gobj), gobj_name_),
        0,
        0,
        kw  // owned
    );
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE json_t* cmd_disable_gobj(hgobj gobj, const char* cmd, json_t* kw, hgobj src)
{
    const char *gobj_name_ = kw_get_str( // __default_service__
        kw,
        "gobj_name",
        kw_get_str(kw, "gobj", "", 0),
        0
    );
    hgobj gobj2view = gobj_find_unique_gobj(gobj_name_, FALSE);
    if(!gobj2view) {
        gobj2view = gobj_find_gobj(gobj_name_);
        if(!gobj2view) {
            return msg_iev_build_webix(
                gobj,
                -1,
                json_sprintf(
                    "%s: gobj '%s' not found.", gobj_short_name(gobj), gobj_name_
                ),
                0,
                0,
                kw  // owned
            );
        }
    }

    if(gobj2view == gobj_yuno()) {
        return msg_iev_build_webix(gobj,
            -1,
            json_sprintf("%s: __yuno__ forbidden.", gobj_short_name(gobj)),
            0,
            0,
            kw  // owned
        );
    }
    gobj_disable(gobj2view);

    /*
     *  Inform
     */
    return msg_iev_build_webix(gobj,
        0,
        json_sprintf("%s: '%s' disabled.", gobj_short_name(gobj), gobj_name_),
        0,
        0,
        kw  // owned
    );
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE json_t* cmd_subscribe_event(hgobj gobj, const char* cmd, json_t* kw, hgobj src)
{
    const char *gobj_name_ = kw_get_str( // __default_service__
        kw,
        "gobj_name",
        kw_get_str(kw, "gobj", "", 0),
        0
    );
    const char *event_name = kw_get_str(kw, "event_name", 0, 0);
    const char *rename_event_name = kw_get_str(kw, "rename_event_name", 0, 0);
    BOOL first_shot = kw_get_bool(kw, "first_shot", 0, 0);
    json_t *filter = kw_get_dict(kw, "filter", 0, 0);
    json_t *config = kw_get_dict(kw, "config", 0, 0);

    hgobj gobj2view = gobj_find_unique_gobj(gobj_name_, FALSE);
    if(!gobj2view) {
        gobj2view = gobj_find_gobj(gobj_name_);
        if(!gobj2view) {
            return msg_iev_build_webix(
                gobj,
                -1,
                json_sprintf(
                    "%s: gobj '%s' not found.", gobj_short_name(gobj), gobj_name_
                ),
                0,
                0,
                kw  // owned
            );
        }
    }

    json_t *kw_sub = json_object();

    // Prepare the return of response
    json_t *__md_iev__ = kw_get_dict(kw, "__md_iev__", 0, 0);
    if(__md_iev__) {
        KW_INCREF(kw);
        json_t *kw3 = msg_iev_answer(gobj, kw, 0, 0);
        json_object_set_new(kw_sub, "__global__", kw3);
    }

    if(filter) {
        json_object_set(kw_sub, "__filter__", filter);
    }
    if(config) {
        json_object_set(kw_sub, "__config__", config);
    }

    // Remove locals
    kw_set_subdict_value(kw_sub, "__local__", "__temp__", json_null());

    // Configure webix response format
    kw_set_subdict_value(kw_sub, "__config__", "__trans_filter__", json_string("webix"));
    if(!empty_string(rename_event_name)) {
        kw_set_subdict_value(
            kw_sub,
            "__config__",
            "__rename_event_name__",
            json_string(rename_event_name)
        );
    }
    if(first_shot) {
        kw_set_subdict_value(
            kw_sub,
            "__config__",
            "__first_shot__",
            json_true()
        );
    }

    // Subscribe
    gobj_subscribe_event(gobj2view, event_name, kw_sub, src);

    /*
     *  Inform
     */
    return msg_iev_build_webix(gobj,
        0,
        json_sprintf("Subscribed to event '%s' of '%s'.",
            event_name?event_name:"", gobj_name_
        ),
        0,
        0,
        kw  // owned
    );
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE json_t* cmd_unsubscribe_event(hgobj gobj, const char* cmd, json_t* kw, hgobj src)
{
    const char *gobj_name_ = kw_get_str( // __default_service__
        kw,
        "gobj_name",
        kw_get_str(kw, "gobj", "", 0),
        0
    );
    const char *event_name = kw_get_str(kw, "event_name", 0, 0);
    json_t *__filter__ = kw_get_dict(kw, "__filter__", 0, 0);

    hgobj gobj2view = gobj_find_unique_gobj(gobj_name_, FALSE);
    if(!gobj2view) {
        gobj2view = gobj_find_gobj(gobj_name_);
        if(!gobj2view) {
            return msg_iev_build_webix(
                gobj,
                -1,
                json_sprintf(
                    "%s: gobj '%s' not found.", gobj_short_name(gobj), gobj_name_
                ),
                0,
                0,
                kw  // owned
            );
        }
    }

    json_t *kw_sub = json_object();

    // Prepare the return of response
    json_t *__md_iev__ = kw_get_dict(kw, "__md_iev__", 0, 0);
    if(__md_iev__) {
        KW_INCREF(kw);
        json_t *kw3 = msg_iev_answer(gobj, kw, 0, 0);
        json_object_set_new(kw_sub, "__global__", kw3);
    }

    if(__filter__) {
        json_object_set(kw_sub, "__filter__", __filter__);
    }

    gobj_unsubscribe_event(gobj2view, event_name, kw_sub, src);

    /*
     *  Inform
     */
    return msg_iev_build_webix(gobj,
        0,
        json_sprintf("Unsubscribed from event '%s' of '%s'.",
            event_name?event_name:"", gobj_name_
        ),
        0,
        0,
        kw  // owned
    );
}

/***************************************************************************
 *  list subscriptions
 ***************************************************************************/
PRIVATE json_t * _list_subscriptions(hgobj gobj2view, BOOL full_names)
{
    json_t *jn_subs = json_array();
    dl_list_t *dl_subs = gobj_find_subscriptions(gobj2view, 0, 0, 0);
    rc_instance_t *i_subs; hsdata subs;
    i_subs = rc_first_instance(dl_subs, (rc_resource_t **)&subs);
    while(i_subs) {
        json_t *jn_sub = sdata2json(subs, -1, 0);
        hgobj gobj_publisher = sdata_read_pointer(subs, "publisher");
        json_object_set_new(
            jn_sub,
            "publisher",
            json_string(
                full_names?gobj_full_name(gobj_publisher):gobj_short_name(gobj_publisher)
            )
        );
        hgobj gobj_subscriber = sdata_read_pointer(subs, "subscriber");
        json_object_set_new(
            jn_sub,
            "subscriber",
            json_string(
                full_names?gobj_full_name(gobj_subscriber):gobj_short_name(gobj_subscriber)
            )
        );
        json_array_append_new(jn_subs, jn_sub);
        i_subs = rc_next_instance(i_subs, (rc_resource_t **)&subs);
    }
    rc_free_iter(dl_subs, TRUE, FALSE);
    return jn_subs;
}

/***************************************************************************
 *  list subscriptions
 ***************************************************************************/
PRIVATE int cb_list_subscriptions(
    rc_instance_t *i_child,
    hgobj child,
    void *user_data,
    void *user_data2,
    void *user_data3)
{
    json_t *jn_data = user_data;
    BOOL full_names = (BOOL)(size_t)user_data2;

    if(gobj_subscriptions_size(child)>0) {
        json_object_set_new(jn_data, gobj_short_name(child), _list_subscriptions(child, full_names));
    }

    return 0;
}

/***************************************************************************
 *  list subscriptions
 ***************************************************************************/
PRIVATE json_t* cmd_list_subscriptions(hgobj gobj, const char* cmd, json_t* kw, hgobj src)
{
    const char *gobj_name_ = kw_get_str( // __default_service__
        kw,
        "gobj_name",
        kw_get_str(kw, "gobj", "", 0),
        0
    );
    BOOL recursive = kw_get_bool(kw, "recursive", 0, KW_WILD_NUMBER);
    BOOL full_names = kw_get_bool(kw, "full-names", 0, KW_WILD_NUMBER);

    hgobj gobj2view = gobj_find_unique_gobj(gobj_name_, FALSE);
    if(!gobj2view) {
        gobj2view = gobj_find_gobj(gobj_name_);
        if(!gobj2view) {
            return msg_iev_build_webix(
                gobj,
                -1,
                json_sprintf(
                    "%s: gobj '%s' not found.", gobj_short_name(gobj), gobj_name_
                ),
                0,
                0,
                kw  // owned
            );
        }
    }

    json_t *jn_data = json_object();

    json_object_set_new(jn_data, gobj_short_name(gobj2view), _list_subscriptions(gobj2view, full_names));

    if(recursive) {
        gobj_walk_gobj_childs_tree(
            gobj2view,
            WALK_TOP2BOTTOM,
            cb_list_subscriptions,
            jn_data,
            (void *)(size_t)full_names,
            0
        );

    }

    /*
     *  Inform
     */
    return msg_iev_build_webix(gobj,
        0,
        0,
        0,
        jn_data,
        kw  // owned
    );
}

/***************************************************************************
 *  list subscribings
 ***************************************************************************/
PRIVATE json_t * _list_subscribings(hgobj gobj2view, BOOL full_names)
{
    json_t *jn_subs = json_array();
    dl_list_t *dl_subs = gobj_find_subscribings(gobj2view, 0, 0, 0);
    rc_instance_t *i_subs; hsdata subs;
    i_subs = rc_first_instance(dl_subs, (rc_resource_t **)&subs);
    while(i_subs) {
        json_t *jn_sub = sdata2json(subs, -1, 0);
        hgobj gobj_subscriber = sdata_read_pointer(subs, "subscriber");
        json_object_set_new(
            jn_sub,
            "subscriber",
            json_string(
                full_names?gobj_full_name(gobj_subscriber):gobj_short_name(gobj_subscriber)
            )
        );
        hgobj gobj_publisher = sdata_read_pointer(subs, "publisher");
        json_object_set_new(
            jn_sub,
            "publisher",
            json_string(
                full_names?gobj_full_name(gobj_publisher):gobj_short_name(gobj_publisher)
            )
        );
        json_array_append_new(jn_subs, jn_sub);
        i_subs = rc_next_instance(i_subs, (rc_resource_t **)&subs);
    }
    rc_free_iter(dl_subs, TRUE, FALSE);
    return jn_subs;
}

/***************************************************************************
 *  list subscribings
 ***************************************************************************/
PRIVATE int cb_list_subscribings(
    rc_instance_t *i_child,
    hgobj child,
    void *user_data,
    void *user_data2,
    void *user_data3)
{
    json_t *jn_data = user_data;
    BOOL full_names = (BOOL)(size_t)user_data2;

    if(gobj_subscribings_size(child)>0) {
        json_object_set_new(jn_data, gobj_short_name(child), _list_subscribings(child, full_names));
    }

    return 0;
}

/***************************************************************************
 *  list subscribings
 ***************************************************************************/
PRIVATE json_t* cmd_list_subscribings(hgobj gobj, const char* cmd, json_t* kw, hgobj src)
{
    const char *gobj_name_ = kw_get_str( // __default_service__
        kw,
        "gobj_name",
        kw_get_str(kw, "gobj", "", 0),
        0
    );
    BOOL recursive = kw_get_bool(kw, "recursive", 0, KW_WILD_NUMBER);
    BOOL full_names = kw_get_bool(kw, "full-names", 0, KW_WILD_NUMBER);

    hgobj gobj2view = gobj_find_unique_gobj(gobj_name_, FALSE);
    if(!gobj2view) {
        gobj2view = gobj_find_gobj(gobj_name_);
        if(!gobj2view) {
            return msg_iev_build_webix(
                gobj,
                -1,
                json_sprintf(
                    "%s: gobj '%s' not found.", gobj_short_name(gobj), gobj_name_
                ),
                0,
                0,
                kw  // owned
            );
        }
    }

    json_t *jn_data = json_object();

    json_object_set_new(jn_data, gobj_short_name(gobj2view), _list_subscribings(gobj2view, full_names));

    if(recursive) {
        gobj_walk_gobj_childs_tree(
            gobj2view,
            WALK_TOP2BOTTOM,
            cb_list_subscribings,
            jn_data,
            (void *)(size_t)full_names,
            0
        );

    }

    /*
     *  Inform
     */
    return msg_iev_build_webix(gobj,
        0,
        0,
        0,
        jn_data,
        kw  // owned
    );
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE json_t* cmd_list_allowed_ips(hgobj gobj, const char* cmd, json_t* kw, hgobj src)
{
    /*
     *  Inform
     */
    return msg_iev_build_webix(gobj,
        0,
        0,
        0,
        json_incref(gobj_read_json_attr(gobj_yuno(), "allowed_ips")),
        kw  // owned
    );
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE json_t* cmd_add_allowed_ip(hgobj gobj, const char* cmd, json_t* kw, hgobj src)
{
    const char *ip = kw_get_str(kw, "ip", "", 0);
    BOOL allowed = kw_get_bool(kw, "allowed", 0, KW_WILD_NUMBER);

    if(empty_string(ip)) {
        return msg_iev_build_webix(gobj,
            -1,
            json_sprintf("What ip?"),
            0,
            0,
            kw  // owned
        );
    }
    if(!kw_has_key(kw, "allowed")) {
        return msg_iev_build_webix(gobj,
            -1,
            json_sprintf("Allowed, true or false?"),
            0,
            0,
            kw  // owned
        );
    }

    add_allowed_ip(ip, allowed);

    /*
     *  Inform
     */
    return msg_iev_build_webix(gobj,
        0,
        0,
        0,
        json_incref(gobj_read_json_attr(gobj_yuno(), "allowed_ips")),
        kw  // owned
    );
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE json_t* cmd_remove_allowed_ip(hgobj gobj, const char* cmd, json_t* kw, hgobj src)
{
    const char *ip = kw_get_str(kw, "ip", "", 0);
    if(empty_string(ip)) {
        return msg_iev_build_webix(gobj,
            -1,
            json_sprintf("What ip?"),
            0,
            0,
            kw  // owned
        );
    }

    remove_allowed_ip(ip);

    /*
     *  Inform
     */
    return msg_iev_build_webix(gobj,
        0,
        0,
        0,
        json_incref(gobj_read_json_attr(gobj_yuno(), "allowed_ips")),
        kw  // owned
    );
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE json_t* cmd_list_denied_ips(hgobj gobj, const char* cmd, json_t* kw, hgobj src)
{
    /*
     *  Inform
     */
    return msg_iev_build_webix(gobj,
        0,
        0,
        0,
        json_incref(gobj_read_json_attr(gobj_yuno(), "denied_ips")),
        kw  // owned
    );
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE json_t* cmd_add_denied_ip(hgobj gobj, const char* cmd, json_t* kw, hgobj src)
{
    const char *ip = kw_get_str(kw, "ip", "", 0);
    BOOL denied = kw_get_bool(kw, "denied", 0, KW_WILD_NUMBER);

    if(empty_string(ip)) {
        return msg_iev_build_webix(gobj,
            -1,
            json_sprintf("What ip?"),
            0,
            0,
            kw  // owned
        );
    }
    if(!kw_has_key(kw, "denied")) {
        return msg_iev_build_webix(gobj,
            -1,
            json_sprintf("Allowed, true or false?"),
            0,
            0,
            kw  // owned
        );
    }

    add_denied_ip(ip, denied);

    /*
     *  Inform
     */
    return msg_iev_build_webix(gobj,
        0,
        0,
        0,
        json_incref(gobj_read_json_attr(gobj_yuno(), "denied_ips")),
        kw  // owned
    );
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE json_t* cmd_remove_denied_ip(hgobj gobj, const char* cmd, json_t* kw, hgobj src)
{
    const char *ip = kw_get_str(kw, "ip", "", 0);
    if(empty_string(ip)) {
        return msg_iev_build_webix(gobj,
            -1,
            json_sprintf("What ip?"),
            0,
            0,
            kw  // owned
        );
    }

    remove_denied_ip(ip);

    /*
     *  Inform
     */
    return msg_iev_build_webix(gobj,
        0,
        0,
        0,
        json_incref(gobj_read_json_attr(gobj_yuno(), "denied_ips")),
        kw  // owned
    );
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE json_t* cmd_2key_get_schema(hgobj gobj, const char* cmd, json_t* kw, hgobj src)
{
    /*
     *  Inform
     */
    return msg_iev_build_webix(gobj,
        0,
        0,
        0,
        gobj_2key_get_schema(),
        kw  // owned
    );
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE json_t* cmd_2key_get_value(hgobj gobj, const char* cmd, json_t* kw, hgobj src)
{
    const char *key1 = kw_get_str(kw, "key1", "", 0);
    if(empty_string(key1)) {
        return msg_iev_build_webix(gobj,
            -1,
            json_sprintf("What key1?"),
            0,
            0,
            kw  // owned
        );
    }
    const char *key2 = kw_get_str(kw, "key2", "", 0);
    if(empty_string(key2)) {
        return msg_iev_build_webix(gobj,
            -1,
            json_sprintf("What key2?"),
            0,
            0,
            kw  // owned
        );
    }

    json_t *value = gobj_2key_get_value(key1, key2);
    if(!value) {
        return msg_iev_build_webix(gobj,
            -1,
            json_sprintf("2key not found: '%s','%s'", key1, key2),
            0,
            0,
            kw  // owned
        );
    }

    BOOL expanded = kw_get_bool(kw, "expanded", 0, KW_WILD_NUMBER);
    int lists_limit = kw_get_int(kw, "lists_limit", 100, KW_WILD_NUMBER);
    int dicts_limit = kw_get_int(kw, "dicts_limit", 100, KW_WILD_NUMBER);
    const char *path = kw_get_str(kw, "path", "", 0);
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
PRIVATE json_t* cmd_2key_get_subvalue(hgobj gobj, const char* cmd, json_t* kw, hgobj src)
{
    const char *path = kw_get_str(kw, "path", "", 0);
    if(empty_string(path)) {
        return msg_iev_build_webix(gobj,
            -1,
            json_sprintf("What path?"),
            0,
            0,
            kw  // owned
        );
    }
    return cmd_2key_get_value(gobj, cmd, kw, src);
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE json_t* cmd_system_topic_schema(hgobj gobj, const char* cmd, json_t* kw, hgobj src)
{
    /*
     *  Inform
     */
    return msg_iev_build_webix(gobj,
        0,
        0,
        0,
        _treedb_create_topic_cols_desc(),
        kw  // owned
    );
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE json_t* cmd_global_variables(hgobj gobj, const char* cmd, json_t* kw, hgobj src)
{
    /*
     *  Inform
     */
    return msg_iev_build_webix(gobj,
        0,
        0,
        0,
        gobj_global_variables(),
        kw  // owned
    );
}




            /***************************
             *      Local Methods
             ***************************/




/***************************************************************************
 *
 ***************************************************************************/
PRIVATE int save_pid_in_file(hgobj gobj)
{
    /*
     *  Let it create the bin_path. Can exist some zombi yuno.
     */
    unsigned int pid = getpid();
    yuneta_bin_file(pidfile, sizeof(pidfile), "yuno.pid", TRUE);
    FILE *file = fopen(pidfile, "w");
    fprintf(file, "%d\n", pid);
    fclose(file);
    return 0;
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE void load_stats(hgobj gobj)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    /*---------------------------------------*
     *      cpu
     *---------------------------------------*/
    {
        unsigned int pid = getpid();
        uint64_t cpu_ticks;
        cpu_usage(pid, 0, &cpu_ticks);
        //uint64_t ms = time_in_miliseconds();
        time_t ms;  // Usando segundos no se pierde el uso de cpu
        time(&ms);
        if(!priv->last_ms) {
            priv->last_ms = ms;
        }
        uint32_t cpu_delta = 0;
        uint64_t t = (ms - priv->last_ms);
        if(t>0) {
            cpu_delta = cpu_ticks - priv->last_cpu_ticks;
            //cpu_delta *= 1000;
            cpu_delta /= t;
            priv->last_ms = ms;
            priv->last_cpu_ticks = cpu_ticks;
            gobj_write_uint32_attr(gobj, "cpu", cpu_delta);
            gobj_write_uint64_attr(gobj, "cpu_ticks", cpu_ticks);
        }
    }
    /*---------------------------------------*
     *      uptime
     *---------------------------------------*/
    {
        double uptime;
        uv_uptime(&uptime);
        gobj_write_uint64_attr(
            gobj,
            "uptime",
            uptime
        );
    }
    /*---------------------------------------*
     *      rusage (See man getrusage)
     *---------------------------------------*/
    {
        uv_rusage_t rusage;
        uv_getrusage(&rusage);

    /*
        ru_utime:       user CPU time used
        ru_stime:       system CPU time used
        ru_maxrss:      maximum resident set size
        ru_ixrss:       integral shared memory size         // No used in linux
        ru_idrss:       integral unshared data size         // No used in linux
        ru_isrss:       integral unshared stack size        // No used in linux
        ru_minflt:      page reclaims (soft page faults)
        ru_majflt:      page faults (hard page faults)
        ru_nswap:       swaps                               // No used in linux
        ru_inblock:     block input operations
        ru_oublock:     block output operations
        ru_msgsnd:      IPC messages sent                   // No used in linux
        ru_msgrcv:      IPC messages received               // No used in linux
        ru_nsignals:    signals received                    // No used in linux
        ru_nvcsw:       voluntary context switches
        ru_nivcsw:      involuntary context switches
    */

        uint64_t time_user = rusage.ru_utime.tv_sec*1000000LL + rusage.ru_utime.tv_usec;
        gobj_write_uint64_attr(
            gobj,
            "rusage_ru_utime_us", // in microseconds!!
            time_user
        );
        uint64_t time_system = rusage.ru_stime.tv_sec*1000000LL + rusage.ru_stime.tv_usec;
        gobj_write_uint64_attr(
            gobj,
            "rusage_ru_stime_us", // in microseconds!!
            time_system
        );

        gobj_write_uint64_attr(
            gobj,
            "rusage_ru_maxrss",
            rusage.ru_maxrss
        );
        gobj_write_uint64_attr(
            gobj,
            "rusage_ru_minflt",
            rusage.ru_minflt
        );
        gobj_write_uint64_attr(
            gobj,
            "rusage_ru_majflt",
            rusage.ru_majflt
        );
        gobj_write_uint64_attr(
            gobj,
            "rusage_ru_inblock",
            rusage.ru_inblock
        );
        gobj_write_uint64_attr(
            gobj,
            "rusage_ru_oublock",
            rusage.ru_oublock
        );
        gobj_write_uint64_attr(
            gobj,
            "rusage_ru_nvcsw",
            rusage.ru_nvcsw
        );
        gobj_write_uint64_attr(
            gobj,
            "rusage_ru_nivcsw",
            rusage.ru_nivcsw
        );
    }

    /*---------------------------------------*
     *      Mem
     *---------------------------------------*/
    {
        size_t free_superblock_mem;
        size_t free_segmented_mem;
        size_t total_free_mem;
        size_t allocated_system_mem;
        size_t mem_in_use;
        size_t memory_yuno_cur_sm;
        size_t memory_yuno_max_sm;

        gbmem_stats(
            &free_superblock_mem,
            &free_segmented_mem,
            &total_free_mem,
            &allocated_system_mem,
            &mem_in_use,
            &memory_yuno_cur_sm,
            &memory_yuno_max_sm
        );

        gobj_write_uint64_attr(
            gobj,
            "mem_memory_node_total_in_kb",
            uv_get_total_memory()/1024
        );
        gobj_write_uint64_attr(
            gobj,
            "mem_memory_node_free_in_kb",
            free_ram_in_kb()
        );
        size_t rss;
        uv_resident_set_memory(&rss);
        gobj_write_uint64_attr(
            gobj,
            "mem_memory_rss_in_kb",
            rss/1024
        );

        gobj_write_uint64_attr(
            gobj,
            "mem_gbmem_total_in_kb",
            allocated_system_mem/1024
        );
        gobj_write_uint64_attr(
            gobj,
            "mem_gbmem_using_in_kb",
            mem_in_use/1024
        );

        gobj_write_uint64_attr(
            gobj,
            "mem_gbmem_max_sm_in_kb",
            memory_yuno_max_sm/1024
        );
        gobj_write_uint64_attr(
            gobj,
            "mem_gbmem_cur_sm_in_kb",
            memory_yuno_cur_sm/1024
        );
    }

    /*---------------------------------------*
     *      Mem
     *---------------------------------------*/
    {
        struct statvfs64 st;

        if(statvfs64("/yuneta", &st)==0) {
            int free_percent = (st.f_bavail * 100)/st.f_blocks;
            uint64_t total_size = (uint64_t)st.f_frsize * st.f_blocks;
            total_size = total_size/(1024LL*1024LL*1024LL);
            gobj_write_uint64_attr(
                gobj,
                "disk_size_in_gigas",
                total_size
            );
            gobj_write_uint64_attr(
                gobj,
                "disk_free_percent",
                free_percent
            );
        }
    }

    /*---------------------------------------*
     *      Queue Stats
     *---------------------------------------*/
    {
        gobj_write_uint64_attr(
            gobj,
            "qs_txmsgs",
            gobj_get_qs(QS_TXMSGS)
        );
        gobj_write_uint64_attr(
            gobj,
            "qs_rxmsgs",
            gobj_get_qs(QS_RXMSGS)
        );
        gobj_write_uint64_attr(
            gobj,
            "qs_txbytes",
            gobj_get_qs(QS_TXBYTES)
        );
        gobj_write_uint64_attr(
            gobj,
            "qs_rxbytes",
            gobj_get_qs(QS_RXBYTES)
        );
        gobj_write_uint64_attr(
            gobj,
            "qs_output_queue",
            gobj_get_qs(QS_OUPUT_QUEUE)
        );
        gobj_write_uint64_attr(
            gobj,
            "qs_input_queue",
            gobj_get_qs(QS_INPUT_QUEUE)
        );
        gobj_write_uint64_attr(
            gobj,
            "qs_internal_queue",
            gobj_get_qs(QS_INTERNAL_QUEUE)
        );
        gobj_write_uint64_attr(
            gobj,
            "qs_drop_by_overflow",
            gobj_get_qs(QS_DROP_BY_OVERFLOW)
        );
        gobj_write_uint64_attr(
            gobj,
            "qs_drop_by_no_exit",
            gobj_get_qs(QS_DROP_BY_NO_EXIT)
        );
        gobj_write_uint64_attr(
            gobj,
            "qs_drop_by_down",
            gobj_get_qs(QS_DROP_BY_DOWN)
        );
        gobj_write_uint64_attr(
            gobj,
            "qs_lower_response_time",
            gobj_get_qs(QS_LOWER_RESPONSE_TIME)
        );
        gobj_write_uint64_attr(
            gobj,
            "qs_medium_response_time",
            gobj_get_qs(QS_MEDIUM_RESPONSE_TIME)
        );
        gobj_write_uint64_attr(
            gobj,
            "qs_higher_response_time",
            gobj_get_qs(QS_HIGHER_RESPONSE_TIME)
        );
        gobj_write_uint64_attr(
            gobj,
            "qs_repeated",
            gobj_get_qs(QS_REPEATED)
        );
    }
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE int set_user_gclass_traces(hgobj gobj)
{
    json_t *jn_trace_levels = gobj_read_json_attr(gobj, "trace_levels");
    if(!jn_trace_levels) {
        jn_trace_levels = json_object();
        gobj_write_json_attr(gobj, "trace_levels", jn_trace_levels);
        json_decref(jn_trace_levels);
    }

    json_t *jn_global = json_object_get(jn_trace_levels, "__global_trace__");
    if(jn_global) {
        size_t idx;
        json_t *jn_level;
        json_array_foreach(jn_global, idx, jn_level) {
            const char *level = json_string_value(jn_level);
            gobj_set_global_trace(level, TRUE);
        }
    }

    json_t *jn_trace_filters = json_object_get(jn_trace_levels, "__trace_filters__");
    if(jn_trace_filters) {
        gobj_load_trace_filter(json_incref(jn_trace_filters));
    }

    const char *key;
    json_t *jn_name;
    json_object_foreach(jn_trace_levels, key, jn_name) {
        const char *name = key;
        GCLASS *gclass = gobj_find_gclass(name, FALSE);
        if(!gclass) {
            continue;
        }
        if(!json_is_array(jn_name)) {
            log_error(0,
                "gobj",         "%s", gobj_full_name(gobj),
                "function",     "%s", __FUNCTION__,
                "msgset",       "%s", MSGSET_PARAMETER_ERROR,
                "msg",          "%s", "value MUST be a json list",
                "name",         "%s", name,
                NULL
            );
            continue;
        }

        size_t idx;
        json_t *jn_level;
        json_array_foreach(jn_name, idx, jn_level) {
            const char *level = json_string_value(jn_level);
            gobj_set_gclass_trace(gclass, level, TRUE);
        }
    }

    return 0;
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE int set_user_gclass_no_traces(hgobj gobj)
{
    json_t *jn_no_trace_levels = gobj_read_json_attr(gobj, "no_trace_levels");
    if(!jn_no_trace_levels) {
        jn_no_trace_levels = json_object();
        gobj_write_json_attr(gobj, "no_trace_levels", jn_no_trace_levels);
        json_decref(jn_no_trace_levels);
    }
    const char *key;
    json_t *jn_name;
    json_object_foreach(jn_no_trace_levels, key, jn_name) {
        const char *name = key;
        GCLASS *gclass = gobj_find_gclass(name, FALSE);
        if(!gclass) {
            continue;
        }
        if(!json_is_array(jn_name)) {
            log_error(0,
                "gobj",         "%s", gobj_full_name(gobj),
                "function",     "%s", __FUNCTION__,
                "msgset",       "%s", MSGSET_PARAMETER_ERROR,
                "msg",          "%s", "value MUST be a json list",
                "name",         "%s", name,
                NULL
            );
            continue;
        }

        size_t idx;
        json_t *jn_level;
        json_array_foreach(jn_name, idx, jn_level) {
            const char *level = json_string_value(jn_level);
            gobj_set_gclass_no_trace(gclass, level, TRUE);
        }
    }

    return 0;
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE int set_user_gobj_traces(hgobj gobj)
{
    json_t *jn_trace_levels = gobj_read_json_attr(gobj, "trace_levels");
    if(!jn_trace_levels) {
        jn_trace_levels = json_object();
        gobj_write_json_attr(gobj, "trace_levels", jn_trace_levels);
        json_decref(jn_trace_levels);
    }

    json_t *jn_global = json_object_get(jn_trace_levels, "__global_trace__");
    if(jn_global) {
        size_t idx;
        json_t *jn_level;
        json_array_foreach(jn_global, idx, jn_level) {
            const char *level = json_string_value(jn_level);
            gobj_set_global_trace(level, TRUE);
        }
    }

    BOOL save = FALSE;
    const char *key;
    json_t *jn_name;
    void *n;
    json_object_foreach_safe(jn_trace_levels, n, key, jn_name) {
        const char *name = key;
        if(empty_string(name)) {
            continue;
        }
        if(strcmp(name, "__yuno__")==0) {
            continue;
        }
        if(strcmp(name, "__root__")==0) {
            continue;
        }
        if(strcmp(name, "__global_trace__")==0) {
            continue;
        }
        if(strcmp(name, "__trace_filters__")==0) {
            continue;
        }

        GCLASS *gclass = 0;
        hgobj namedgobj = 0;
        gclass = gobj_find_gclass(name, FALSE);
        if(!gclass) { // Check gclass to check if no gclass and no gobj
            namedgobj = gobj_find_unique_gobj(name, FALSE);
            if(!namedgobj) {
                char temp[256];
                snprintf(temp, sizeof(temp), "%s NOT FOUND: %s",
                    gclass?"named-gobj":"GClass",
                    name
                );
                log_error(0,
                    "gobj",         "%s", gobj_full_name(gobj),
                    "function",     "%s", __FUNCTION__,
                    "msgset",       "%s", MSGSET_PARAMETER_ERROR,
                    "msg",          "%s", temp,
                    "name",         "%s", name,
                    NULL
                );
                json_object_del(jn_trace_levels, name);
                save = TRUE;
                continue;
            }
        }
        if(!namedgobj) {
            // Must be gclass, already set
            continue;
        }

        if(!json_is_array(jn_name)) {
            log_error(0,
                "gobj",         "%s", gobj_full_name(gobj),
                "function",     "%s", __FUNCTION__,
                "msgset",       "%s", MSGSET_PARAMETER_ERROR,
                "msg",          "%s", "value MUST be a json list",
                "name",         "%s", name,
                NULL
            );
            continue;
        }

        size_t idx;
        json_t *jn_level;
        json_array_foreach(jn_name, idx, jn_level) {
            const char *level = json_string_value(jn_level);
            if(namedgobj) {
                gobj_set_gobj_trace(namedgobj, level, TRUE, 0); // Se pierden los "channel_name"!!!
            }
        }
    }

    if(save) {
        gobj_save_persistent_attrs(gobj, json_string("trace_levels"));
    }

    return 0;
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE int set_user_gobj_no_traces(hgobj gobj)
{
    json_t *jn_no_trace_levels = gobj_read_json_attr(gobj, "no_trace_levels");
    if(!jn_no_trace_levels) {
        jn_no_trace_levels = json_object();
        gobj_write_json_attr(gobj, "no_trace_levels", jn_no_trace_levels);
        json_decref(jn_no_trace_levels);
    }

    BOOL save = FALSE;
    const char *key;
    json_t *jn_name;
    void *n;
    json_object_foreach_safe(jn_no_trace_levels, n, key, jn_name) {
        const char *name = key;
        if(empty_string(name)) {
            continue;
        }
        if(strcmp(name, "__yuno__")==0) {
            continue;
        }
        if(strcmp(name, "__root__")==0) {
            continue;
        }
        if(strcmp(name, "__global_trace__")==0) {
            continue;
        }
        if(strcmp(name, "__trace_filters__")==0) {
            continue;
        }

        GCLASS *gclass = 0;
        hgobj namedgobj = 0;
        gclass = gobj_find_gclass(name, FALSE);
        if(!gclass) { // Check gclass to check if no gclass and no gobj
            namedgobj = gobj_find_unique_gobj(name, FALSE);
            if(!namedgobj) {
                char temp[256];
                snprintf(temp, sizeof(temp), "%s NOT FOUND: %s",
                    gclass?"named-gobj":"GClass",
                    name
                );
                log_error(0,
                    "gobj",         "%s", gobj_full_name(gobj),
                    "function",     "%s", __FUNCTION__,
                    "msgset",       "%s", MSGSET_PARAMETER_ERROR,
                    "msg",          "%s", temp,
                    "name",         "%s", name,
                    NULL
                );
                json_object_del(jn_no_trace_levels, name);
                save = TRUE;
                continue;
            }
        }
        if(!namedgobj) {
            // Must be gclass, already set
            continue;
        }
        if(!json_is_array(jn_name)) {
            log_error(0,
                "gobj",         "%s", gobj_full_name(gobj),
                "function",     "%s", __FUNCTION__,
                "msgset",       "%s", MSGSET_PARAMETER_ERROR,
                "msg",          "%s", "value MUST be a json list",
                "name",         "%s", name,
                NULL
            );
            continue;
        }

        size_t idx;
        json_t *jn_level;
        json_array_foreach(jn_name, idx, jn_level) {
            const char *level = json_string_value(jn_level);
            if(namedgobj) {
                gobj_set_gobj_no_trace(namedgobj, level, TRUE);
            }
        }
    }

    if(save) {
        gobj_save_persistent_attrs(gobj, json_string("no_trace_levels"));
    }

    return 0;
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE int save_global_trace(
    hgobj gobj,
    const char *level,
    BOOL set,
    BOOL persistent
)
{
    json_t *jn_trace_levels = gobj_read_json_attr(gobj, "trace_levels");

    if(!kw_has_key(jn_trace_levels, "__global_trace__")) {
        /*
         *  Create new row, only if new_bitmask is != 0
         */
        if(set) {
            /*
             *  Create row
             */
            json_object_set_new(jn_trace_levels, "__global_trace__", json_array());
            json_t *jn_levels = json_object_get(jn_trace_levels, "__global_trace__");
            json_array_append_new(jn_levels, json_string(level));
        }
    } else {
        json_t *jn_levels = json_object_get(jn_trace_levels, "__global_trace__");
        if(!set) {
            /*
             *  Delete level
             */
            int idx = json_list_str_index(jn_levels, level, FALSE);
            if(idx != -1) {
                json_array_remove(jn_levels, idx);
            }
        } else {
            /*
             *  Add level
             */
            int idx = json_list_str_index(jn_levels, level, FALSE);
            if(idx == -1) {
                json_array_append_new(jn_levels, json_string(level));
            }
        }
    }

    if(persistent) {
        return gobj_save_persistent_attrs(gobj, json_string("trace_levels"));
    } else {
        return 0;
    }
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE int save_trace_filters(hgobj gobj)
{
    json_t *jn_trace_levels = gobj_read_json_attr(gobj, "trace_levels");
    json_t *jn_trace_filters = gobj_get_trace_filter();
    json_object_set(jn_trace_levels, "__trace_filters__", jn_trace_filters);
    return gobj_save_persistent_attrs(gobj, json_string("trace_levels"));
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE int save_user_trace(
    hgobj gobj,
    const char* name,
    const char* level,
    BOOL set,
    BOOL persistent
)
{
    json_t *jn_trace_levels = gobj_read_json_attr(gobj, "trace_levels");

    if(!kw_has_key(jn_trace_levels, name)) {
        /*
         *  Create new row, only if new_bitmask is != 0
         */
        if(set) {
            /*
             *  Create row
             */
            json_object_set_new(jn_trace_levels, name, json_array());
            json_t *jn_levels = json_object_get(jn_trace_levels, name);
            json_array_append_new(jn_levels, json_string(level));
        }
    } else {
        json_t *jn_levels = json_object_get(jn_trace_levels, name);
        if(!set) {
            /*
             *  Delete level
             */
            int idx = json_list_str_index(jn_levels, level, FALSE);
            if(idx != -1) {
                json_array_remove(jn_levels, idx);
            }
        } else {
            /*
             *  Add level
             */
            int idx = json_list_str_index(jn_levels, level, FALSE);
            if(idx == -1) {
                json_array_append_new(jn_levels, json_string(level));
            }
        }
    }

    if(persistent) {
        return gobj_save_persistent_attrs(gobj, json_string("trace_levels"));
    } else {
        return 0;
    }
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE int save_user_no_trace(
    hgobj gobj,
    const char* name,
    const char* level,
    BOOL set,
    BOOL persistent
)
{
    json_t *jn_no_trace_levels = gobj_read_json_attr(gobj, "no_trace_levels");

    if(!kw_has_key(jn_no_trace_levels, name)) {
        /*
         *  Create new row, only if new_bitmask is != 0
         */
        if(set) {
            /*
             *  Create row
             */
            json_object_set_new(jn_no_trace_levels, name, json_array());
            json_t *jn_levels = json_object_get(jn_no_trace_levels, name);
            json_array_append_new(jn_levels, json_string(level));
        }
    } else {
        json_t *jn_no_levels = json_object_get(jn_no_trace_levels, name);
        if(!set) {
            /*
             *  Delete no-level
             */
            int idx = json_list_str_index(jn_no_levels, level, FALSE);
            if(idx != -1) {
                json_array_remove(jn_no_levels, idx);
            }
        } else {
            /*
             *  Add no-level
             */
            int idx = json_list_str_index(jn_no_levels, level, FALSE);
            if(idx == -1) {
                json_array_append_new(jn_no_levels, json_string(level));
            }
        }
    }

    if(persistent) {
        return gobj_save_persistent_attrs(gobj, json_string("no_trace_levels"));
    } else {
        return 0;
    }
}

/***************************************************************************
 *  close handlers
 ***************************************************************************/
PRIVATE void close_walk_cb(uv_handle_t* handle, void* arg)
{
    if (!uv_is_closing(handle)) {
        hgobj gobj = handle->data;

        log_error(0,
            "gobj",         "%s", "__yuno__",  // gobj can be destroying
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_INTERNAL_ERROR,
            "msg",          "%s", "PENDING some uv handler to close",
            "pointer",      "%p", handle,
            "gclass",       "%s", gobj_gclass_name(gobj),
            "name",         "%s", gobj_name(gobj),
            NULL
        );
        uv_close(handle, NULL);
    }
}

PRIVATE void close_loop(uv_loop_t* loop)
{
  uv_walk(loop, close_walk_cb, NULL);
  uv_run(loop, UV_RUN_DEFAULT);
}




            /***************************
             *      Actions
             ***************************/




/***************************************************************************
 *
 ***************************************************************************/
PRIVATE int ac_stopped(hgobj gobj, const char *event, json_t *kw, hgobj src)
{
    /*
     *  priv->timer has stopped
     */
    JSON_DECREF(kw);
    return 0;
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE int ac_timeout(hgobj gobj, const char *event, json_t *kw, hgobj src)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);
//     static int __otra_muerte__ = 0; // Otra modalidad para morir.
//     static size_t muere = 0;
//     if(__otra_muerte__) {
//         muere = start_sectimer(5);
//         __otra_muerte__ = 0;
//     }
//     if(test_sectimer(muere)) {
//         JSON_DECREF(kw);
//         gobj_shutdown(); // provoca gobj_pause y gobj_stop del gobj yuno
//         return 0;
//     }

    uint64_t max_proc_mem = gobj_read_uint64_attr(gobj, "max_proc_mem");
    if(max_proc_mem>0) {
        uint64_t x = 0; // TODO no encuentro un método rápido para saber la memoria del proceso.
        if(x > max_proc_mem) {
            log_error(LOG_OPT_ABORT,
                "gobj",             "%s", gobj_full_name(gobj),
                "function",         "%s", __FUNCTION__,
                "msgset",           "%s", MSGSET_LIBUV_ERROR,
                "msg",              "%s", "ABORTING by proc mem greater than max limit",
                "proc_mem",       "%lu", (unsigned long) x,
                "max_proc_mem",   "%lu", (unsigned long) max_proc_mem,
                NULL
            );
        }
    }

    if(priv->timeout_flush && test_sectimer(priv->t_flush)) {
        priv->t_flush = start_sectimer(priv->timeout_flush);
        rotatory_flush(0);
    }
    if(gobj_get_yuno_must_die()) {
        JSON_DECREF(kw);
        gobj_shutdown(); // provoca gobj_pause y gobj_stop del gobj yuno
        return 0;
    }
    if(priv->timeout_restart && test_sectimer(priv->t_restart)) {
        log_info(0,
            "gobj",         "%s", gobj_full_name(gobj),
            "msgset",       "%s", MSGSET_STARTUP,
            "msg",          "%s", "Exit to restart",
            NULL
        );
        gobj_set_exit_code(-1);
        JSON_DECREF(kw);
        gobj_shutdown(); // provoca gobj_pause y gobj_stop del gobj yuno
        return 0;
    }

    if(priv->timeout_stats && test_sectimer(priv->t_stats)) {
        priv->t_stats = start_sectimer(priv->timeout_stats);
        load_stats(gobj);
    }

//     if(gobj_subscriptions_size(gobj)>0) {
//         gobj_publish_event(gobj, "EV_ON_STATS", 0);
//     }

    JSON_DECREF(kw);
    return 0;
}


/***************************************************************************
 *                          FSM
 ***************************************************************************/
PRIVATE const EVENT input_events[] = {
    {"EV_STOPPED",              0},
    {"EV_TIMEOUT",              0},
    {NULL, 0}
};
PRIVATE const EVENT output_events[] = {
    {"EV_ON_STATS", EVF_PUBLIC_EVENT|EVF_NO_WARN_SUBS,  0,  "Stats by subcription."},
    {"EV_RAISE",    EVF_NO_WARN_SUBS,                   0,  "Order to raise. Check possible died elements."},

    {NULL, 0}
};
PRIVATE const char *state_names[] = {
    "ST_IDLE",
    NULL
};

PRIVATE EV_ACTION ST_IDLE[] = {
    {"EV_TIMEOUT",          ac_timeout,             0},
    {"EV_STOPPED",          ac_stopped,             0},
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
    GCLASS_DEFAULT_YUNO_NAME,
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
        0, //mt_command,
        0, //mt_inject_event,
        0, //mt_create_resource,
        0, //mt_list_resource,
        0, //mt_save_resource,
        0, //mt_delete_resource,
        0, //mt_future21
        0, //mt_future22
        0, //mt_get_resource
        0, //mt_state_changed,
        0, //mt_authenticate,
        0, //mt_list_childs,
        mt_stats_updated,
        0, //mt_disable,
        0, //mt_enable,
        0, //mt_trace_on,
        0, //mt_trace_off,
        mt_gobj_created,
        0, //mt_future33,
        0, //mt_future34,
        0, //mt_publish_event,
        mt_publication_pre_filter,
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
    command_table,  // command_table
    0, // gcflag
};

/*---------------------------------------------*
 *              Public access
 *---------------------------------------------*/
PUBLIC GCLASS *gclass_default_yuno(void)
{
    return &_gclass;
}

/***************************************************************************
 *
 ***************************************************************************/
PUBLIC void *yuno_uv_event_loop(void)
{
    PRIVATE_DATA *priv;
    hgobj yuno = gobj_yuno();
    priv = gobj_priv_data(yuno);

    return &priv->uv_loop;
}

/***************************************************************************
 *  This message is used in __inform_action_return__ responses
 ***************************************************************************/
PUBLIC int yuno_set_info_msg(const char *msg)
{
    hgobj gobj = gobj_yuno();

    if(!msg) {
        // delete attr content
        gobj_write_str_attr(gobj, "info_msg", 0);
        return 0;
    }

    gobj_write_str_attr(gobj, "info_msg", msg);
    return 0;
}

/***************************************************************************
 *  This message is used in __inform_action_return__ responses
 ***************************************************************************/
PUBLIC int yuno_printf_info_msg(const char *msg, ...)
{
    hgobj gobj = gobj_yuno();
    va_list ap;
    char temp[1024];

    if(!msg) {
        // delete attr content
        gobj_write_str_attr(gobj, "info_msg", 0);
        return 0;
    }

    va_start(ap, msg);
    vsnprintf(temp, sizeof(temp), msg, ap);

    gobj_write_str_attr(gobj, "info_msg", temp);
    va_end(ap);
    return 0;
}

/***************************************************************************
 *  This message is used in __inform_action_return__ responses
 ***************************************************************************/
PUBLIC const char *yuno_get_info_msg(void)
{
    hgobj gobj = gobj_yuno();
    return gobj_read_str_attr(gobj, "info_msg");
}

/***************************************************************************
 *  Get yuno app description
 ***************************************************************************/
PUBLIC const char *yuno_app_doc(void)
{
    return gobj_read_str_attr(gobj_yuno(), "appDesc");
}

/***************************************************************************
 *  Error's stats
 ***************************************************************************/
PRIVATE void inform_cb(int priority, uint32_t count, void *user_data)
{
    hgobj gobj = gobj_yuno();
    if(!gobj) {
        return;
    }
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    switch(priority) {
    case LOG_EMERG:
        *priv->pemergencies = count;
        break;
    case LOG_ALERT:
        *priv->palerts = count;
        break;
    case LOG_CRIT:
        *priv->pcriticals = count;
        break;
    case LOG_ERR:
        *priv->perrors = count;
        break;
    case LOG_WARNING:
        *priv->pwarnings = count;
        break;
    case LOG_NOTICE:
        break;
    case LOG_INFO:
        *priv->pinfos = count;
        break;
    case LOG_DEBUG:
        *priv->pdebugs = count;
        break;
    case LOG_AUDIT:
        *priv->paudits = count;
        break;
    case LOG_MONITOR:
        *priv->pmonitors = count;
        break;
    }
}

/***************************************************************************
 *  Exec with libuv, not enough checked
 ***************************************************************************/
void exit_cb(uv_process_t *process, int64_t exit_status, int term_signal)
{
    //printf("========> exit_status %d, term_signal %d\n", (int)exit_status, term_signal);
}
PUBLIC int run_executable(
    hgobj gobj,
    const char *bin_path,
    const char *parameters,
    const char *cwd)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    if(!cwd) {
        cwd = "/yuneta";
    }
    uv_process_options_t options;
    memset(&options, 0, sizeof(options));

    char *args[5];
    args[0] = (char *)bin_path;
    int i = 1;
    if(parameters) {
        args[i++] = (char *)parameters;
    }
    args[i] = NULL;

    options.file = bin_path;
    options.args = args;
    options.cwd = cwd;
    options.exit_cb = exit_cb;
    options.flags = UV_PROCESS_DETACHED|UV_PROCESS_WINDOWS_HIDE;

    priv->process.data = gobj;

    int ret = uv_spawn(&priv->uv_loop, &priv->process, &options);
    if(ret < 0) {
        log_error(0,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_LIBUV_ERROR,
            "msg",          "%s", "uv_spawn() FAILED",
            "error",        "%d", ret,
            "uv_error",     "%s", uv_err_name(ret),
            "uv_strerror",  "%s", uv_strerror(ret),
            NULL
        );
        return -1;
    }
    uv_unref((uv_handle_t*)&priv->process);
    return 0;
}

/***************************************************************************
 *
 ***************************************************************************/
PUBLIC int gobj_set_qs(qs_type_t qs_type, uint64_t value)
{
    if(qs_type < QS_TXMSGS || qs_type >= QS__LAST_ITEM__) {
        return -1; // fuck you
    }
    __qs__[qs_type-1] = value;

    return 0;
}

/***************************************************************************
 *
 ***************************************************************************/
PUBLIC int gobj_incr_qs(qs_type_t qs_type, uint64_t value)
{
    if(qs_type < QS_TXMSGS || qs_type >= QS__LAST_ITEM__) {
        return -1; // fuck you
    }
    __qs__[qs_type-1] += value;

    return 0;
}

/***************************************************************************
 *
 ***************************************************************************/
PUBLIC int gobj_decr_qs(qs_type_t qs_type, uint64_t value)
{
    if(qs_type < QS_TXMSGS || qs_type >= QS__LAST_ITEM__) {
        return -1; // fuck you
    }
    __qs__[qs_type-1] -= value;

    return 0;
}

/***************************************************************************
 *
 ***************************************************************************/
PUBLIC uint64_t gobj_get_qs(qs_type_t qs_type)
{
    if(qs_type < QS_TXMSGS || qs_type >= QS__LAST_ITEM__) {
        return -1; // fuck you
    }
    return __qs__[qs_type-1];
}

/***************************************************************************
 *  New Stats functions, using json
 ***************************************************************************/
PUBLIC json_int_t yuno_set_stat(const char *path, json_int_t value)
{
    return gobj_set_stat(gobj_yuno(), path, value);
}

/***************************************************************************
 *  New Stats functions, using json
 ***************************************************************************/
PUBLIC json_int_t yuno_incr_stat(const char *path, json_int_t value)
{
    return gobj_incr_stat(gobj_yuno(), path, value);
}

/***************************************************************************
 *  New Stats functions, using json
 ***************************************************************************/
PUBLIC json_int_t yuno_decr_stat(const char *path, json_int_t value)
{
    return gobj_decr_stat(gobj_yuno(), path, value);
}

/***************************************************************************
 *  New Stats functions, using json
 ***************************************************************************/
PUBLIC json_int_t yuno_get_stat(const char *path)
{
    return gobj_get_stat(gobj_yuno(), path);
}

/***************************************************************************
 *  New Stats functions, using json
 ***************************************************************************/
PUBLIC json_int_t default_service_set_stat(const char *path, json_int_t value)
{
    return gobj_set_stat(gobj_default_service(), path, value);
}

/***************************************************************************
 *  New Stats functions, using json
 ***************************************************************************/
PUBLIC json_int_t default_service_incr_stat(const char *path, json_int_t value)
{
    return gobj_incr_stat(gobj_default_service(), path, value);
}

/***************************************************************************
 *  New Stats functions, using json
 ***************************************************************************/
PUBLIC json_int_t default_service_decr_stat(const char *path, json_int_t value)
{
    return gobj_decr_stat(gobj_default_service(), path, value);
}

/***************************************************************************
 *  New Stats functions, using json
 ***************************************************************************/
PUBLIC json_int_t default_service_get_stat(const char *path)
{
    return gobj_get_stat(gobj_default_service(), path);
}

/***************************************************************************
 *  Is ip or peername allowed?
 *  TODO pueden meter ip's con nombre, traduce a numérico
 ***************************************************************************/
PUBLIC BOOL is_ip_allowed(const char *peername)
{
    char ip[NAME_MAX];
    snprintf(ip, sizeof(ip), "%s", peername);
    char *p = strchr(ip, ':');
    if(p) {
        *p = 0;
    }
    json_t *b = json_object_get(gobj_read_json_attr(gobj_yuno(), "allowed_ips"), ip);
    return json_is_true(b)?TRUE:FALSE;
}

/***************************************************************************
 * allowed: TRUE to allow, FALSE to deny
 ***************************************************************************/
PUBLIC int add_allowed_ip(const char *ip, BOOL allowed)
{
    if(json_object_set_new(
        gobj_read_json_attr(gobj_yuno(), "allowed_ips"),
        ip,
        allowed?json_true(): json_false()
    )==0) {
        return gobj_save_persistent_attrs(gobj_yuno(), json_string("allowed_ips"));
    } else {
        return -1;
    }
}

/***************************************************************************
 *  Remove from interna list (dict)
 ***************************************************************************/
PUBLIC int remove_allowed_ip(const char *ip)
{
    if(json_object_del(gobj_read_json_attr(gobj_yuno(), "allowed_ips"), ip)==0) {
        return gobj_save_persistent_attrs(gobj_yuno(), json_string("allowed_ips"));
    } else {
        return -1;
    }
}

/***************************************************************************
 *  Is ip or peername denied?
 ***************************************************************************/
PUBLIC BOOL is_ip_denied(const char *peername)
{
    char ip[NAME_MAX];
    snprintf(ip, sizeof(ip), "%s", peername);
    char *p = strchr(ip, ':');
    if(p) {
        *p = 0;
    }
    json_t *b = json_object_get(gobj_read_json_attr(gobj_yuno(), "denied_ips"), ip);
    return json_is_true(b)?TRUE:FALSE;
}

/***************************************************************************
 * denied: TRUE to deny, FALSE to deny
 ***************************************************************************/
PUBLIC int add_denied_ip(const char *ip, BOOL denied)
{
    if(json_object_set_new(
        gobj_read_json_attr(gobj_yuno(), "denied_ips"),
        ip,
        denied?json_true(): json_false()
    )==0) {
        return gobj_save_persistent_attrs(gobj_yuno(), json_string("denied_ips"));
    } else {
        return -1;
    }
}

/***************************************************************************
 *  Remove from interna list (dict)
 ***************************************************************************/
PUBLIC int remove_denied_ip(const char *ip)
{
    if(json_object_del(gobj_read_json_attr(gobj_yuno(), "denied_ips"), ip)==0) {
        return gobj_save_persistent_attrs(gobj_yuno(), json_string("denied_ips"));
    } else {
        return -1;
    }
}
