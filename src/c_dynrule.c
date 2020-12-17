/***********************************************************************
 *          C_DYNRULE.C
 *          DynRule GClass.
 *          Copyright (c) 2015 Niyamaka.
 *          All Rights Reserved.
***********************************************************************/
#include <string.h>
#include <regex.h>
#include "c_dynrule.h"

/***************************************************************************
 *              Constants
 ***************************************************************************/

/***************************************************************************
 *              Structures
 ***************************************************************************/
typedef struct _IFUNCTION {
    const char *name;
    BOOL boolean;   // TRUE boolean, FALSE assign function
    int (*fn)(hgobj gobj, struct _IFUNCTION *pfn, json_t *jn_parameters, json_t *kw_io);
    int (*init_fn)(hgobj gobj, struct _IFUNCTION *pfn, void *data);
    void (*end_fn)(hgobj gobj, struct _IFUNCTION *pfn);
    BOOL initiated;
    regex_t *regex_match;
} IFUNCTION;

/***************************************************************************
 *              Prototypes
 ***************************************************************************/
PRIVATE int clear_rule_functions(hgobj gobj, rc_resource_t * hr_rule_);
PRIVATE int init_match(hgobj gobj, struct _IFUNCTION *pfn, void *data);
PRIVATE void end_match(hgobj gobj, struct _IFUNCTION *pfn);
PRIVATE int b_match(hgobj gobj, IFUNCTION *pfn, json_t *jn_parameters, json_t *kw_io);
PRIVATE int b_equal(hgobj gobj, IFUNCTION *pfn, json_t *jn_parameters, json_t *kw_io);
PRIVATE int b_not_equal(hgobj gobj, IFUNCTION *pfn, json_t *jn_parameters, json_t *kw_io);
PRIVATE int b_greater(hgobj gobj, IFUNCTION *pfn, json_t *jn_parameters, json_t *kw_io);
PRIVATE int b_less(hgobj gobj, IFUNCTION *pfn, json_t *jn_parameters, json_t *kw_io);
PRIVATE int a_assign(hgobj gobj, IFUNCTION *pfn, json_t *jn_parameters, json_t *kw_io);
PRIVATE int a_append(hgobj gobj, IFUNCTION *pfn, json_t *jn_parameters, json_t *kw_io);

/***************************************************************************
 *          Data: config, public data, private data
 ***************************************************************************/
PRIVATE IFUNCTION ipfn[] = {
    {"match",           TRUE,   b_match,            init_match,     end_match},
    {"equal",           TRUE,   b_equal,            0,              0},
    {"not_equal",       TRUE,   b_not_equal,        0,              0},
    {"greater",         TRUE,   b_greater,          0,              0},
    {"less",            TRUE,   b_less,             0,              0},
    {"assign",          FALSE,  a_assign,           0,              0},
    {"append",          FALSE,  a_append,           0,              0},
    {0, 0}
};


PRIVATE sdata_desc_t funcTb_it[] = {
/*-ATTR-type------------name----------------flag------------------------default---------description----------*/
SDATA (ASN_UNSIGNED,    "index",            SDF_NOTACCESS,              0,              "function table index"),
SDATA (ASN_OCTET_STR,   "function",         SDF_PKEY|SDF_REQUIRED,      0,              "function"),
SDATA (ASN_JSON,        "parameters",       0,                          0,              "Json array with function parameters"),
SDATA (ASN_POINTER,     "pfn",              0,                          0,              "pointer to internal function"),
SDATA (ASN_POINTER,     "re_compiled",      0,                          0,              "re compiled"),
SDATA_END()
};

PRIVATE sdata_desc_t dynrulesTb_it[] = {
/*-ATTR-type------------name----------------flag------------------------default---------description----------*/
SDATA (ASN_OCTET_STR,   "name",             SDF_RD|SDF_PKEY|SDF_REQUIRED,0,             "rule name"),
SDATA (ASN_OCTET_STR,   "description",      0,                          0,              "rule description"),

/*-DB----type-----------name----------------flag------------------------schema----------free_fn---------header-----------*/
SDATADB (ASN_ITER,      "input",            0,                          funcTb_it,      sdata_destroy,   "`bool` functions to decide if the input json must be processed"),
SDATADB (ASN_ITER,      "condition",        SDF_REQUIRED,               funcTb_it,      sdata_destroy,   "`bool` functions to apply in the input json"),
SDATADB (ASN_ITER,      "true",             SDF_REQUIRED,               funcTb_it,      sdata_destroy,   "`assign` functions to execute if *condition* results in true"),
SDATADB (ASN_ITER,      "false",            0,                          funcTb_it,      sdata_destroy,   "`assign` functions to execute if *condition* results in false"),
/* HACK behaviour of "input" "condition" and "true"
 *
 *  if `bool` "input" exist:
 *      execute `bool` "input" functions
 *      if result is true:
 *          execute "condition"
 *      else:
 *          exit (return, nothing to do)
 *
 *  execute `bool` "condition" functions
 *  if result is true:
 *      execute `assign` "true" functions
 *      exit (return)
 *  else:
 *      if "false" exists
 *          execute `assign` "false" functions
 *          exit (return)
 *      else:
 *          continue with next rule
 */
SDATA_END()
};


/*---------------------------------------------*
 *      Attributes - order affect to oid's
 *---------------------------------------------*/
PRIVATE sdata_desc_t tattr_desc[] = {
/*-DB----type-----------name----------------flag------------------------schema----------free_fn---------header-----------*/
SDATADB (ASN_ITER,      "dynrulesTb",       0,                          dynrulesTb_it,  sdata_destroy,  "dynamic rules table"),

/*-ATTR-type------------name----------------flag------------------------default---------description----------*/
SDATA (ASN_POINTER,     "user_data",        0,                          0,              "user data"),
SDATA (ASN_POINTER,     "user_data2",       0,                          0,              "more user data"),
SDATA (ASN_POINTER,     "subscriber",       0,                          0,              "subscriber of output-events. If it's null then subscriber is the parent."),
SDATA_END()
};

/*---------------------------------------------*
 *              Private data
 *---------------------------------------------*/
typedef struct _PRIVATE_DATA {
    dl_list_t *tb_dynrules;
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
    if(!subscriber)
        subscriber = gobj_parent(gobj);
    gobj_subscribe_event(gobj, NULL, NULL, subscriber);

    /*
     *  Do a copy of heavily used parameters into private data
     *  HACK The writable attributes must be repeated in mt_writing method.
     */
//     SET_PRIV(sample_int,            gobj_read_int32_attr)
//     SET_PRIV(sample_str,            gobj_read_str_attr)

    priv->tb_dynrules = gobj_read_iter_attr(gobj, "dynrulesTb");

}

/***************************************************************************
 *      Framework Method writing
 ***************************************************************************/
PRIVATE void mt_writing(hgobj gobj, const char *path)
{
//     PRIVATE_DATA *priv = gobj_priv_data(gobj);

//     IF_EQ_SET_PRIV(sample_int,              gobj_read_int32_attr)
//     ELIF_EQ_SET_PRIV(sample_str,            gobj_read_str_attr)
//     END_EQ_SET_PRIV()
}

/***************************************************************************
 *      Framework Method start
 ***************************************************************************/
PRIVATE int mt_start(hgobj gobj)
{
    return 0;
}

/***************************************************************************
 *      Framework Method stop
 ***************************************************************************/
PRIVATE int mt_stop(hgobj gobj)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);
    rc_free_iter2(
        priv->tb_dynrules,
        FALSE,
        sdata_destroy,
        clear_rule_functions,
        gobj
    );
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
 *
 ***************************************************************************/
PRIVATE IFUNCTION * clone_function(hgobj gobj, const char *function)
{
    if(!function) {
        log_error(0,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_PARAMETER_ERROR,
            "msg",          "%s", "function is NULL",
            NULL
        );
        return 0;
    }

    IFUNCTION *pfn = ipfn;
    while(pfn->name) {
        if(strcasecmp(function, pfn->name)==0) {
            IFUNCTION *pfn_ = gbmem_malloc(sizeof(IFUNCTION));
            memcpy(pfn_, pfn, sizeof(IFUNCTION));
            return pfn_;
        }
        pfn++;
    }
    log_error(0,
        "gobj",         "%s", gobj_full_name(gobj),
        "function",     "%s", __FUNCTION__,
        "msgset",       "%s", MSGSET_PARAMETER_ERROR,
        "msg",          "%s", "function NOT FOUND",
        "fuction",      "%s", function,
        NULL
    );
    return 0;
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE void free_function(hgobj gobj, IFUNCTION * pfn)
{
    if(pfn->end_fn) {
        (pfn->end_fn)(gobj, pfn);
    }
    gbmem_free(pfn);
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE void clear_table_functions(hgobj gobj, dl_list_t *iter)
{
    if(iter) {
        hsdata hr_func; rc_instance_t *i_hs;
        i_hs = rc_first_instance(iter, (rc_resource_t **)&hr_func);
        while(i_hs) {
            IFUNCTION *pfn_ = sdata_read_pointer(hr_func, "pfn");
            if(pfn_) {
                free_function(gobj, pfn_);
            }
            sdata_write_pointer(hr_func, "pfn", 0);

            /*
             *  next function row
             */
            i_hs = rc_next_instance(i_hs, (rc_resource_t **)&hr_func);
        }
    }
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE int clear_rule_functions(hgobj gobj, rc_resource_t * hr_rule_)
{
    hsdata hr_rule = hr_rule_;
    /*---------------------------------------*
     *  [(input)?]
     *---------------------------------------*/
    dl_list_t *ht_input = sdata_read_iter(hr_rule, "input");
    clear_table_functions(gobj, ht_input);

    /*---------------------------------------*
     *  (condition)?
     *---------------------------------------*/
    dl_list_t *ht_condition = sdata_read_iter(hr_rule, "condition");
    clear_table_functions(gobj, ht_condition);

    /*---------------------------------------*
     *   -true-> assign and quit
     *---------------------------------------*/
    dl_list_t *ht_true = sdata_read_iter(hr_rule, "true");
    clear_table_functions(gobj, ht_true);

    /*---------------------------------------*
     *   [-false-> assign and quit]
     *---------------------------------------*/
    dl_list_t *ht_false = sdata_read_iter(hr_rule, "false");
    clear_table_functions(gobj, ht_false);

    return 0;
}

/***************************************************************************
 *  Fill all the functions in the table
 ***************************************************************************/
PRIVATE int fill_table_functions(hgobj gobj, dl_list_t *iter, BOOL boolean_functions)
{
    if(iter) {
        hsdata hr_func; rc_instance_t *i_hs;
        i_hs = rc_first_instance(iter, (rc_resource_t **)&hr_func);
        while(i_hs) {

            const char *function = sdata_read_str(hr_func, "function");
            IFUNCTION *pfn = clone_function(gobj, function);
            if(!pfn) {
                // error already logged
                return -1;
            }
            if(boolean_functions && !pfn->boolean) {
                log_error(0,
                    "gobj",         "%s", gobj_full_name(gobj),
                    "function",     "%s", __FUNCTION__,
                    "msgset",       "%s", MSGSET_PARAMETER_ERROR,
                    "msg",          "%s", "function MUST BE BOOLEAN",
                    "fuction",      "%s", function,
                    NULL
                );
                return -1;
            }

            if(!boolean_functions && pfn->boolean) {
                log_error(0,
                    "gobj",         "%s", gobj_full_name(gobj),
                    "function",     "%s", __FUNCTION__,
                    "msgset",       "%s", MSGSET_PARAMETER_ERROR,
                    "msg",          "%s", "function MUST BE ASSIGN",
                    "fuction",      "%s", function,
                    NULL
                );
                return -1;
            }

            IFUNCTION *pfn_ = sdata_read_pointer(hr_func, "pfn");
            if(pfn_) {
                free_function(gobj, pfn_);
            }
            sdata_write_pointer(hr_func, "pfn", pfn);

            /*
             *  next function row
             */
            i_hs = rc_next_instance(i_hs, (rc_resource_t **)&hr_func);
        }
    }
    return 0;
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE int fill_rule_functions(hgobj gobj, hsdata hr_rule)
{
    /*---------------------------------------*
     *  [(input)?]
     *---------------------------------------*/
    dl_list_t * ht_input = sdata_read_iter(hr_rule, "input");
    fill_table_functions(gobj, ht_input, TRUE);

    /*---------------------------------------*
     *  (condition)?
     *---------------------------------------*/
    dl_list_t * ht_condition = sdata_read_iter(hr_rule, "condition");
    fill_table_functions(gobj, ht_condition, TRUE);

    /*---------------------------------------*
     *   -true-> assign and quit
     *---------------------------------------*/
    dl_list_t * ht_true = sdata_read_iter(hr_rule, "true");
    fill_table_functions(gobj, ht_true, FALSE);

    /*---------------------------------------*
     *   [-false-> assign and quit]
     *---------------------------------------*/
    dl_list_t * ht_false = sdata_read_iter(hr_rule, "false");
    fill_table_functions(gobj, ht_false, FALSE);

    return 0;
}

/***************************************************************************
 *
 ***************************************************************************/
typedef enum {
    OP_EQUAL = 1,
    OP_NOT_EQUAL,
    OP_LESS,
    OP_GREATHER,
} OP;
PRIVATE BOOL compare(hgobj gobj, json_t *jn_var1, json_t *jn_var2, OP op)
{
    /*
     *  Check nulls
     */
    if(!jn_var1) {
        log_error(0,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_PARAMETER_ERROR,
            "msg",          "%s", "var1 NULL",
            NULL
        );
        return FALSE;
    }
    if(!jn_var2) {
        log_error(0,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_PARAMETER_ERROR,
            "msg",          "%s", "var1 NULL",
            NULL
        );
        return FALSE;
    }

    /*
     *  Discard complex types
     */
    if(json_is_object(jn_var1)) {
        log_error(0,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_PARAMETER_ERROR,
            "msg",          "%s", "var1 needs elemental json types",
            NULL
        );
        log_debug_json(0, jn_var1, "needed elemental json types");
        return FALSE;
    } else if(json_is_object(jn_var2)) {
        log_error(0,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_PARAMETER_ERROR,
            "msg",          "%s", "var2 needs elemental json types",
            NULL
        );
        log_debug_json(0, jn_var2, "needed elemental json types");
        return FALSE;
    }

    if(json_is_array(jn_var1)) {
        log_error(0,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_PARAMETER_ERROR,
            "msg",          "%s", "var1 needs elemental json types",
            NULL
        );
        log_debug_json(0, jn_var1, "needed elemental json types");
        return FALSE;
    } else if(json_is_array(jn_var2)) {
        log_error(0,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_PARAMETER_ERROR,
            "msg",          "%s", "var2 needs elemental json types",
            NULL
        );
        log_debug_json(0, jn_var2, "needed elemental json types");
        return FALSE;
    }

    /*
     *  First try real
     */
    if(json_is_real(jn_var1) || json_is_real(jn_var2)) {
        double val1 = jn2real(jn_var1);
        double val2 = jn2real(jn_var2);
        switch(op) {
        case OP_EQUAL:
            if(val1 == val2)
                return TRUE;
            else
                return FALSE;
            break;
        case OP_NOT_EQUAL:
            if(val1 != val2)
                return TRUE;
            else
                return FALSE;
            break;
        case OP_LESS:
            if(val1 < val2)
                return TRUE;
            else
                return FALSE;
            break;
        case OP_GREATHER:
            if(val1 > val2)
                return TRUE;
            else
                return FALSE;
            break;
        }
    }

    /*
     *  Try integer
     */
    if(json_is_integer(jn_var1) || json_is_integer(jn_var2)) {
        int64_t val1 = jn2integer(jn_var1);
        int64_t val2 = jn2integer(jn_var2);
        switch(op) {
        case OP_EQUAL:
            if(val1 == val2)
                return TRUE;
            else
                return FALSE;
            break;
        case OP_NOT_EQUAL:
            if(val1 != val2)
                return TRUE;
            else
                return FALSE;
            break;
        case OP_LESS:
            if(val1 < val2)
                return TRUE;
            else
                return FALSE;
            break;
        case OP_GREATHER:
            if(val1 > val2)
                return TRUE;
            else
                return FALSE;
            break;
        }
    }

    /*
     *  Try boolean
     */
    if(json_is_boolean(jn_var1) || json_is_boolean(jn_var2)) {
        int64_t val1 = jn2integer(jn_var1);
        int64_t val2 = jn2integer(jn_var2);
        switch(op) {
        case OP_EQUAL:
            if(val1 == val2)
                return TRUE;
            else
                return FALSE;
            break;
        case OP_NOT_EQUAL:
            if(val1 != val2)
                return TRUE;
            else
                return FALSE;
            break;
        case OP_LESS:
            if(val1 < val2)
                return TRUE;
            else
                return FALSE;
            break;
        case OP_GREATHER:
            if(val1 > val2)
                return TRUE;
            else
                return FALSE;
            break;
        }
    }

    /*
     *  Try string
     */
    {
        char *val1 = jn2string(jn_var1);
        char *val2 = jn2string(jn_var2);
        int ret;
        switch(op) {
        case OP_EQUAL:
            ret = strcmp(val1, val2);
            gbmem_free(val1);
            gbmem_free(val2);
            if(ret==0)
                return TRUE;
            return FALSE;

        case OP_NOT_EQUAL:
            ret = strcmp(val1, val2);
            gbmem_free(val1);
            gbmem_free(val2);
            if(ret!=0)
                return TRUE;
            return FALSE;

        case OP_LESS:
            ret = strcmp(val1, val2);
            gbmem_free(val1);
            gbmem_free(val2);
            if(ret<0)
                return TRUE;
            return FALSE;

        case OP_GREATHER:
            ret = strcmp(val1, val2);
            gbmem_free(val1);
            gbmem_free(val2);
            if(ret>0)
                return TRUE;
            return FALSE;
        }
    }

    return FALSE;
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE json_t *get_variable(hgobj gobj, json_t *jn_variable, json_t *kw_io)
{
    if(!jn_variable) {
        return 0;
    }
    json_t *jn_variable_ = 0;
    const char *str = 0;
    if(json_is_string(jn_variable)) {
        str = json_string_value(jn_variable);
        if(strchr(str, '.')) {
            /*
             *  It's a path-to-dict
             */
            jn_variable_ = kw_get_dict_value(kw_io, str, 0 , 0);
        } else {
            jn_variable_ = jn_variable;
        }
    } else {
        jn_variable_ = jn_variable;
    }
    return jn_variable_;
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE int init_match(hgobj gobj, struct _IFUNCTION *pfn, void *data)
{
    const char *re = data;
    if(empty_string(re)) {
        return 0;
    }
    if(pfn->initiated) {
        end_match(gobj, pfn);
    } else {
        pfn->regex_match = gbmem_malloc(sizeof(regex_t));
    }
    int reti = regcomp(pfn->regex_match, re, 0);
    if(reti!=0) {
        log_error(0,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_PARAMETER_ERROR,
            "msg",          "%s", "regcomp() FAILED",
            "re",           "%s", re,
            NULL
        );
        return -1;
    }
    pfn->initiated = TRUE;
    return 0;
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE void end_match(hgobj gobj, struct _IFUNCTION *pfn)
{
    if(pfn->regex_match) {
        regfree(pfn->regex_match);
        gbmem_free(pfn->regex_match);
        pfn->regex_match = 0;
    }
    pfn->initiated = FALSE;
}

/***************************************************************************
 *  `bool` function: match
 *  First parameter:   re (json string)
 *  Second parameter:  variable (json string or path-to-dict (kw_io))
 ***************************************************************************/
PRIVATE int b_match(hgobj gobj, IFUNCTION* pfn, json_t* jn_parameters, json_t* kw_io)
{
    const char *re = json_string_value(json_array_get(jn_parameters, 0));
    if(empty_string(re)) {
        log_error(0,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_PARAMETER_ERROR,
            "msg",          "%s", "match function needs a regular expresion string for first parameter",
            "function",     "%s", pfn->name,
            NULL
        );
        return FALSE;
    }
    if(!pfn->initiated) {
        pfn->init_fn(gobj, pfn, (void *)re);
    }

    json_t *jn_variable2 = json_array_get(jn_parameters, 1);
    jn_variable2 = get_variable(gobj, jn_variable2, kw_io);

    const char *variable2;
    if(json_is_string(jn_variable2)) {
        variable2 = json_string_value(jn_variable2);
    } else {
        log_error(0,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_PARAMETER_ERROR,
            "msg",          "%s", "function needs a string for second parameter",
            "function",     "%s", pfn->name,
            NULL
        );
        log_debug_json(0, jn_variable2, "function needs a string for second parameter");
        return FALSE;
    }

    int reti = regexec(pfn->regex_match, variable2, 0, NULL, 0);
    if(reti!=0) {
        // No Match
        return FALSE;
    }
    return TRUE;
}

/***************************************************************************
 *  `bool` function: equal
 *  First parameter:   variable (json string/number or path-to-dict (kw_io))
 *  Second parameter:  variable (json string/number or path-to-dict (kw_io))
 ***************************************************************************/
PRIVATE int b_equal(hgobj gobj, IFUNCTION* pfn, json_t* jn_parameters, json_t* kw_io)
{
    json_t *jn_variable1 = json_array_get(jn_parameters, 0);
    jn_variable1 = get_variable(gobj, jn_variable1, kw_io);
    if(!jn_variable1) {
        log_error(0,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_PARAMETER_ERROR,
            "msg",          "%s", "function without first parameter",
            "function",     "%s", pfn->name,
            NULL
        );
        return FALSE;
    }

    json_t *jn_variable2 = json_array_get(jn_parameters, 1);
    jn_variable2 = get_variable(gobj, jn_variable2, kw_io);
    if(!jn_variable2) {
        log_error(0,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_PARAMETER_ERROR,
            "msg",          "%s", "function without second parameter",
            "function",     "%s", pfn->name,
            NULL
        );
        return FALSE;
    }

    BOOL result = compare(gobj, jn_variable1, jn_variable2, OP_EQUAL);
    return result;
}

/***************************************************************************
 *  `bool` function: not_equal
 ***************************************************************************/
PRIVATE int b_not_equal(hgobj gobj, IFUNCTION* pfn, json_t* jn_parameters, json_t* kw_io)
{
    json_t *jn_variable1 = json_array_get(jn_parameters, 0);
    jn_variable1 = get_variable(gobj, jn_variable1, kw_io);
    if(!jn_variable1) {
        log_error(0,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_PARAMETER_ERROR,
            "msg",          "%s", "function without first parameter",
            "function",     "%s", pfn->name,
            NULL
        );
        return FALSE;
    }

    json_t *jn_variable2 = json_array_get(jn_parameters, 1);
    jn_variable2 = get_variable(gobj, jn_variable2, kw_io);
    if(!jn_variable2) {
        log_error(0,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_PARAMETER_ERROR,
            "msg",          "%s", "function without second parameter",
            "function",     "%s", pfn->name,
            NULL
        );
        return FALSE;
    }

    BOOL result = compare(gobj, jn_variable1, jn_variable2, OP_NOT_EQUAL);
    return result;
}

/***************************************************************************
 *  `bool` function: greater
 ***************************************************************************/
PRIVATE int b_greater(hgobj gobj, IFUNCTION* pfn, json_t* jn_parameters, json_t* kw_io)
{
    json_t *jn_variable1 = json_array_get(jn_parameters, 0);
    jn_variable1 = get_variable(gobj, jn_variable1, kw_io);
    if(!jn_variable1) {
        log_error(0,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_PARAMETER_ERROR,
            "msg",          "%s", "function without first parameter",
            "function",     "%s", pfn->name,
            NULL
        );
        return FALSE;
    }

    json_t *jn_variable2 = json_array_get(jn_parameters, 1);
    jn_variable2 = get_variable(gobj, jn_variable2, kw_io);
    if(!jn_variable2) {
        log_error(0,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_PARAMETER_ERROR,
            "msg",          "%s", "function without second parameter",
            "function",     "%s", pfn->name,
            NULL
        );
        return FALSE;
    }

    BOOL result = compare(gobj, jn_variable1, jn_variable2, OP_GREATHER);
    return result;
}

/***************************************************************************
 *  `bool` function: less
 ***************************************************************************/
PRIVATE int b_less(hgobj gobj, IFUNCTION* pfn, json_t* jn_parameters, json_t* kw_io)
{
    json_t *jn_variable1 = json_array_get(jn_parameters, 0);
    jn_variable1 = get_variable(gobj, jn_variable1, kw_io);
    if(!jn_variable1) {
        log_error(0,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_PARAMETER_ERROR,
            "msg",          "%s", "function without first parameter",
            "function",     "%s", pfn->name,
            NULL
        );
        return FALSE;
    }

    json_t *jn_variable2 = json_array_get(jn_parameters, 1);
    jn_variable2 = get_variable(gobj, jn_variable2, kw_io);
    if(!jn_variable2) {
        log_error(0,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_PARAMETER_ERROR,
            "msg",          "%s", "function without second parameter",
            "function",     "%s", pfn->name,
            NULL
        );
        return FALSE;
    }

    BOOL result = compare(gobj, jn_variable1, jn_variable2, OP_LESS);
    return result;
}

/***************************************************************************
 *  `assign` function: assign
 ***************************************************************************/
PRIVATE int a_assign(hgobj gobj, IFUNCTION* pfn, json_t* jn_parameters, json_t* kw_io)
{
    json_t *jn_variable1 = json_array_get(jn_parameters, 0);
    if(!json_is_string(jn_variable1)) {
        log_error(0,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_PARAMETER_ERROR,
            "msg",          "%s", "parameter1 must be path-to-dict",
            NULL
        );
        log_debug_json(0, jn_variable1, "parameter1 must be path-to-dict");
        return -1;
    }
    const char *path2dict = json_string_value(jn_variable1);

    json_t *jn_variable2 = json_array_get(jn_parameters, 1);
    jn_variable2 = get_variable(gobj, jn_variable2, kw_io);
    if(!jn_variable2) {
        log_error(0,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_PARAMETER_ERROR,
            "msg",          "%s", "function without second parameter",
            "function",     "%s", pfn->name,
            NULL
        );
        return FALSE;
    }
    json_incref(jn_variable2);
    return kw_set_dict_value(kw_io, path2dict, jn_variable2);
}

/***************************************************************************
 *  `assign` function: append
 ***************************************************************************/
PRIVATE int a_append(hgobj gobj, IFUNCTION* pfn, json_t* jn_parameters, json_t* kw_io)
{
    json_t *jn_variable1 = json_array_get(jn_parameters, 0);
    if(!json_is_string(jn_variable1)) {
        log_error(0,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_PARAMETER_ERROR,
            "msg",          "%s", "parameter1 must be path-to-dict",
            NULL
        );
        log_debug_json(0, jn_variable1, "parameter1 must be path-to-dict");
        return -1;
    }
    const char *path2dict = json_string_value(jn_variable1);

    json_t *jn_variable2 = json_array_get(jn_parameters, 1);
    jn_variable2 = get_variable(gobj, jn_variable2, kw_io);
    if(!jn_variable2) {
        log_error(0,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_PARAMETER_ERROR,
            "msg",          "%s", "function without second parameter",
            "function",     "%s", pfn->name,
            NULL
        );
        return FALSE;
    }

    return kw_list_append(kw_io, path2dict, jn_variable2);
}




            /***************************
             *      Actions
             ***************************/




/***************************************************************************
 *  kw must be a dict
 ***************************************************************************/
// PRIVATE int ac_add_rule(hgobj gobj, const char *event, json_t *kw, hgobj src)
// {
//     PRIVATE_DATA *priv = gobj_priv_data(gobj);
//     const char *name = kw_get_str(kw, "name", 0, 0);
//     hsdata hr_rule = iter_load_from_dict(priv->tb_dynrules, dynrulesTb_it, kw, TRUE);
//     if(!hr_rule) {
//         log_error(0,
//             "gobj",         "%s", gobj_full_name(gobj),
//             "function",     "%s", __FUNCTION__,
//             "msgset",       "%s", MSGSET_PARAMETER_ERROR,
//             "msg",          "%s", "iter_load_from_dict() FAILED",
//             "name",         "%s", name?name:"",
//             NULL
//         );
//         log_debug_json(0, "iter_load_from_dict() FAILED", kw);
//         JSON_DECREF(kw);
//         return -1;
//     }
//     fill_rule_functions(gobj, hr_rule);
//     JSON_DECREF(kw);
//     return 0;
// }

/***************************************************************************
 *  kw must be an array
 ***************************************************************************/
PRIVATE int print_attr_not_found(void *user_data, const char *attr)
{
    hsdata hs = user_data;
    log_error(0,
        "gobj",         "%s", "Resource GCLASS",
        "function",     "%s", __FUNCTION__,
        "msgset",       "%s", MSGSET_PARAMETER_ERROR,
        "msg",          "%s", "Resource Attribute NOT FOUND",
        "resource",     "%s", sdata_resource(hs),
        "attr",         "%s", attr,
        NULL
    );
    return 0;
}
PRIVATE int ac_add_rules(hgobj gobj, const char *event, json_t *kw, hgobj src)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);
    int ret = sdata_iter_load_from_dict_list(
        priv->tb_dynrules,
        dynrulesTb_it,
        kw,
        print_attr_not_found,
        fill_rule_functions,
        gobj
    );
    if(ret < 0) {
        log_error(0,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_PARAMETER_ERROR,
            "msg",          "%s", "sdata_iter_load_from_dict_list() FAILED",
            NULL
        );
        log_debug_json(0, kw, "sdata_iter_load_from_dict_list() FAILED");
    }
    JSON_DECREF(kw);
    return ret;
}

/***************************************************************************
 *
 ***************************************************************************/
// PRIVATE int ac_remove_rule(hgobj gobj, const char *event, json_t *kw, hgobj src)
// {
//     PRIVATE_DATA *priv = gobj_priv_data(gobj);
//     const char *name = kw_get_str(kw, "name", "", 0);
//     hsdata hr_rule = sdata_iter_search_by_pkey(priv->tb_dynrules, name);
//     if(!hr_rule) {
//         log_error(0,
//             "gobj",         "%s", gobj_full_name(gobj),
//             "function",     "%s", __FUNCTION__,
//             "msgset",       "%s", MSGSET_PARAMETER_ERROR,
//             "msg",          "%s", "RULE NOT FOUND",
//             "name",         "%s", name,
//             NULL
//         );
//         return -1;
//     }
//     free_function(gobj, hr_rule);
//     rc_delete_resource(hr_rule, sdata_destroy); // pop row from table and free the record
//
//     JSON_DECREF(kw);
//     return 0;
// }

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE int ac_remove_rules(hgobj gobj, const char *event, json_t *kw, hgobj src)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    rc_free_iter2(
        priv->tb_dynrules,
        FALSE,
        sdata_destroy,
        clear_rule_functions,
        gobj
    );

    JSON_DECREF(kw);
    return 0;
}

/***************************************************************************
 *
    Example of rule:

    {
        "name": "generica",
        "input": [
            {
                "func": "match",
                "params": [".*", "$this.gds"]
            }
        ],
        "condition": [
            {
                "func": "lower",
                "params": ["$this.curmsg", "$this.maxmsgs"]
            }
        ],
        "true": [
            {
                "func": "assign",
                "params": ["$this.prov_dst", "IB"]
            },
            {
                "func": "append",
                "params": ["$this.prov_fake", ["ITA", "OWN"]]
            }
        ],
        "false": [
        ]
    }
 *
 ***************************************************************************/
PRIVATE int ac_run_rule(hgobj gobj, const char *event, json_t *kw_io, hgobj src)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    hsdata hr_rule; rc_instance_t *i_rule;
    i_rule = rc_first_instance(priv->tb_dynrules, (rc_resource_t **)&hr_rule);
    while(i_rule) {
        /*
         *
         *  if `bool` "input" exist:
         *      execute `bool` "input" ARRAY of functions
         *      if result is true:
         *          execute "condition"
         *      else:
         *          next-rule
         *
         */
        BOOL boolean_input = TRUE; // is input not exist then it's true
        dl_list_t *ht_input = sdata_read_iter(hr_rule, "input");
        if(ht_input) {
            hsdata hr_func; rc_instance_t *i_func;
            i_func = rc_first_instance(ht_input, (rc_resource_t **)&hr_func);
            while(i_func) {
                IFUNCTION *pfn = sdata_read_pointer(hr_func, "pfn");
                if(!pfn) {
                    log_error(0,
                        "gobj",         "%s", gobj_full_name(gobj),
                        "function",     "%s", __FUNCTION__,
                        "msgset",       "%s", MSGSET_INTERNAL_ERROR,
                        "msg",          "%s", "input pfn is NULL",
                        NULL
                    );
                    KW_DECREF(kw_io);
                    return -1;
                }
                json_t *jn_parameters = sdata_read_json(hr_func, "parameters");
                if(!jn_parameters) {
                    log_error(0,
                        "gobj",         "%s", gobj_full_name(gobj),
                        "function",     "%s", __FUNCTION__,
                        "msgset",       "%s", MSGSET_INTERNAL_ERROR,
                        "msg",          "%s", "pfn parameters is NULL",
                        "function",     "%s", pfn->name,
                        NULL
                    );
                    KW_DECREF(kw_io);
                    return -1;
                }
                boolean_input = (pfn->fn)(gobj, pfn, jn_parameters, kw_io);
                if(!boolean_input) {
                    /*
                     *  Exit from rule. Don't execute next "condition"
                     */
                    break;
                }

                /*
                 *  next function
                 */
                i_func = rc_next_instance(i_func, (rc_resource_t **)&hr_func);
            }
        }
        if(!boolean_input) {
            /*
             *  Next rule
             */
            i_rule = rc_next_instance(i_rule, (rc_resource_t **)&hr_rule);
            continue;
        }

        /*
         *  execute `bool` "condition" ARRAY of functions
         *  if result is true:
         *      execute `assign` "true" functions
         *      exit (return)
         *  else:
         *      if "false" exists
         *          execute `assign` "false" functions
         *          exit (return)
         *      else:
         *          continue with next rule
         */
        int boolean_condition = TRUE;
        dl_list_t *ht_condition = sdata_read_iter(hr_rule, "condition");
        if(ht_condition) {
            hsdata hr_func; rc_instance_t *i_func;
            i_func = rc_first_instance(ht_condition, (rc_resource_t **)&hr_func);
            while(i_func) {
                IFUNCTION *pfn = sdata_read_pointer(hr_func, "pfn");
                if(!pfn) {
                    log_error(0,
                        "gobj",         "%s", gobj_full_name(gobj),
                        "function",     "%s", __FUNCTION__,
                        "msgset",       "%s", MSGSET_INTERNAL_ERROR,
                        "msg",          "%s", "condition pfn is NULL",
                        NULL
                    );
                    KW_DECREF(kw_io);
                    return -1;
                }
                json_t *jn_parameters = sdata_read_json(hr_func, "parameters");
                if(!jn_parameters) {
                    log_error(0,
                        "gobj",         "%s", gobj_full_name(gobj),
                        "function",     "%s", __FUNCTION__,
                        "msgset",       "%s", MSGSET_INTERNAL_ERROR,
                        "msg",          "%s", "pfn parameters is NULL",
                        "function",     "%s", pfn->name,
                        NULL
                    );
                    KW_DECREF(kw_io);
                    return -1;
                }
                boolean_condition = (pfn->fn)(gobj, pfn, jn_parameters, kw_io);
                if(!boolean_condition) {
                    boolean_condition = FALSE;
                    break;
                }

                /*
                 *  next function
                 */
                i_func = rc_next_instance(i_func, (rc_resource_t **)&hr_func);
            }
        }

        /*
         *  if result is true:
         *      execute `assign` "true" functions
         *      exit (return)
         */
        if(boolean_condition) {
            dl_list_t *ht_true = sdata_read_iter(hr_rule, "true");
            if(ht_true) {
                hsdata hr_func; rc_instance_t *i_func;
                i_func = rc_first_instance(ht_true, (rc_resource_t **)&hr_func);
                while(i_func) {
                    IFUNCTION *pfn = sdata_read_pointer(hr_func, "pfn");
                    if(!pfn) {
                        log_error(0,
                            "gobj",         "%s", gobj_full_name(gobj),
                            "function",     "%s", __FUNCTION__,
                            "msgset",       "%s", MSGSET_INTERNAL_ERROR,
                            "msg",          "%s", "true pfn is NULL",
                            NULL
                        );
                        KW_DECREF(kw_io);
                        return -1;
                    }
                    json_t *jn_parameters = sdata_read_json(hr_func, "parameters");
                    if(!jn_parameters) {
                        log_error(0,
                            "gobj",         "%s", gobj_full_name(gobj),
                            "function",     "%s", __FUNCTION__,
                            "msgset",       "%s", MSGSET_INTERNAL_ERROR,
                            "msg",          "%s", "pfn parameters is NULL",
                            "function",     "%s", pfn->name,
                            NULL
                        );
                        KW_DECREF(kw_io);
                        return -1;
                    }
                    (pfn->fn)(gobj, pfn, jn_parameters, kw_io);

                    /*
                     *  next function
                     */
                    i_func = rc_next_instance(i_func, (rc_resource_t **)&hr_func);
                }
            }

            KW_DECREF(kw_io);
            return 0;
        }

        /*
         *  else:
         *      if "false" exists
         *          execute `assign` "false" functions
         *          exit (return)
         *      else:
         *          continue with next rule
         */
        dl_list_t *ht_false = sdata_read_iter(hr_rule, "false");
        if(ht_false && dl_size(ht_false)>0) {

            hsdata hr_func; rc_instance_t *i_func;
            i_func = rc_first_instance(ht_false, (rc_resource_t **)&hr_func);
            while(i_func) {
                IFUNCTION *pfn = sdata_read_pointer(hr_func, "pfn");
                if(!pfn) {
                    log_error(0,
                        "gobj",         "%s", gobj_full_name(gobj),
                        "function",     "%s", __FUNCTION__,
                        "msgset",       "%s", MSGSET_INTERNAL_ERROR,
                        "msg",          "%s", "true pfn is NULL",
                        NULL
                    );
                    KW_DECREF(kw_io);
                    return -1;
                }
                json_t *jn_parameters = sdata_read_json(hr_func, "parameters");
                if(!jn_parameters) {
                    log_error(0,
                        "gobj",         "%s", gobj_full_name(gobj),
                        "function",     "%s", __FUNCTION__,
                        "msgset",       "%s", MSGSET_INTERNAL_ERROR,
                        "msg",          "%s", "pfn parameters is NULL",
                        "function",     "%s", pfn->name,
                        NULL
                    );
                    KW_DECREF(kw_io);
                    return -1;
                }
                (pfn->fn)(gobj, pfn, jn_parameters, kw_io);

                /*
                 *  next function
                 */
                i_func = rc_next_instance(i_func, (rc_resource_t **)&hr_func);
            }

            KW_DECREF(kw_io);
            return 0;
        }

        /*
         *  Next rule
         */
        i_rule = rc_next_instance(i_rule, (rc_resource_t **)&hr_rule);
    }

    KW_DECREF(kw_io);
    return 0;
}

/***************************************************************************
 *                          FSM
 ***************************************************************************/
PRIVATE const EVENT input_events[] = {
    {"EV_RUN_RULE",         EVF_KW_WRITING,  0,  0},
    {"EV_ADD_RULES",        0},
    {"EV_REMOVE_RULES",     0},
    {NULL, 0}
};
PRIVATE const EVENT output_events[] = {
    {NULL, 0}
};
PRIVATE const char *state_names[] = {
    "ST_IDLE",
    NULL
};

PRIVATE EV_ACTION ST_IDLE[] = {
    {"EV_RUN_RULE",         ac_run_rule,        0},
    {"EV_ADD_RULES",        ac_add_rules,       0},
    {"EV_REMOVE_RULES",     ac_remove_rules,    0},
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
    GCLASS_DYNRULE_NAME,
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
        0, //mt_authz_allow,
        0, //mt_authz_deny,
        0, //mt_publish_event,
        0, //mt_publication_pre_filter,
        0, //mt_publication_filter,
        0, //mt_future38,
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
        0, //mt_future61,
        0, //mt_future62,
        0, //mt_future63,
        0, //mt_future64
    },
    lmt,
    tattr_desc,
    sizeof(PRIVATE_DATA),
    0,  // acl
    0,  // s_user_trace_level,
    0,  // command_table
    0,  // gcflag
};

/***************************************************************************
 *              Public access
 ***************************************************************************/
PUBLIC GCLASS *gclass_dynrule(void)
{
    return &_gclass;
}
