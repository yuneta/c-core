/***********************************************************************
 *          STATS_PARSER.C
 *
 *          Stats parser
 *
 *          Copyright (c) 2017 Niyamaka.
 *          All Rights Reserved.
***********************************************************************/
#include "stats_parser.h"

/***************************************************************
 *              Constants
 ***************************************************************/

/***************************************************************
 *              Structures
 ***************************************************************/

/***************************************************************
 *              Prototypes
 ***************************************************************/

/***************************************************************
 *              Data
 ***************************************************************/

/***************************************************************************
 *
 ***************************************************************************/
PUBLIC json_t * stats_parser(hgobj gobj,
    const char *stats,
    json_t *kw,
    hgobj src
)
{
    /*--------------------------------------*
     *  Build standard stats
     *--------------------------------------*/
    KW_INCREF(kw);
    json_t *jn_data = build_stats(
        gobj,
        stats,
        kw,     // owned
        src
    );
    append_yuno_metadata(gobj, jn_data, stats);

    return msg_iev_build_webix(
        gobj,
        0,
        0,
        0,
        jn_data,  // owned
        kw // owned
    );
}

/****************************************************************************
 *  Build stats from gobj's attributes with SFD_STATS flag.
 ****************************************************************************/
PRIVATE int reset_stats_callback(json_t *kw, const char *key, json_t *value)
{
    // TODO uso un default? de un schema? gobj_read_default_attr_value(gobj, it->name);
    if(json_is_array(value)) {
        json_object_set_new(kw, key, json_array());
    } else if(json_is_integer(value)) {
        json_object_set_new(kw, key, json_integer(0));
    } else if(json_is_boolean(value)) {
        json_object_set_new(kw, key, json_false());
    } else if(json_is_real(value)) {
        json_object_set_new(kw, key, json_real(0));
    } else if(json_is_null(value)) {
    }
    return 0;
}
PUBLIC json_t * build_stats(hgobj gobj, const char *stats, json_t *kw, hgobj src)
{
    if(!gobj) {
        KW_DECREF(kw);
        return 0;
    }

    const sdata_desc_t *sdesc = sdata_schema(gobj_hsdata(gobj));
    const sdata_desc_t *it;
    if(stats && strcmp(stats, "__reset__")==0) {
        /*-------------------------*
         *  Reset Stats in sdata
         *-------------------------*/
        it = sdesc;
        while(it && it->name) {
            if(!ASN_IS_NUMBER(it->type)) {
                it++;
                continue;
            }
            if(it->flag & SDF_RSTATS) {
                SData_Value_t v = gobj_read_default_attr_value(gobj, it->name);
                if(ASN_IS_REAL_NUMBER(it->type)) {
                    gobj_write_real_attr(gobj, it->name, v.f);
                } else if(ASN_IS_NATURAL_NUMBER(it->type)) {
                    gobj_write_integer_attr(gobj, it->name, v.u64);
                }
            }
            it++;
        }

        /*----------------------------*
         *  Reset Stats in jn_stats
         *----------------------------*/
        json_t *jn_stats = gobj_jn_stats(gobj);
        kw_walk(jn_stats, reset_stats_callback);

        stats = "";
    }

    /*-------------------------*
     *  Create stats json
     *-------------------------*/
    json_t *jn_data = json_object();

    /*-------------------------*
     *      Stats in sdata
     *-------------------------*/
    it = sdesc;
    while(it && it->name) {
        if(!(ASN_IS_NUMBER(it->type) || ASN_IS_STRING(it->type))) {
            it++;
            continue;
        }
        if(!empty_string(stats)) {
            if(!strstr(stats, it->name)) { // Parece que es para seleccionar por prefijo
                char prefix[32];
                char *p = strchr(it->name, '_');
                if(p) {
                    int ln = p - it->name;
                    snprintf(prefix, sizeof(prefix), "%.*s", ln, it->name);
                    p = strstr(stats, prefix);
                    if(!p) {
                        it++;
                        continue;
                    }
                }
            }
        }
        if(it->flag & (SDF_STATS|SDF_RSTATS|SDF_PSTATS)) {
            if(ASN_IS_NATURAL_NUMBER(it->type)) {
                json_object_set_new(
                    jn_data,
                    it->name,
                    json_integer(gobj_read_integer_attr(gobj, it->name))
                );
            } else if(ASN_IS_REAL_NUMBER(it->type)) {
                json_object_set_new(
                    jn_data,
                    it->name,
                    json_real(gobj_read_real_attr(gobj, it->name))
                );
            } else if(ASN_IS_STRING(it->type)) {
                json_object_set_new(
                    jn_data,
                    it->name,
                    json_string(gobj_read_str_attr(gobj, it->name))
                );
            }
        }
        it++;
    }

    /*----------------------------*
     *      Stats in jn_stats
     *----------------------------*/
    const char *key;
    json_t *v;
    json_t *jn_stats = gobj_jn_stats(gobj);
    json_object_foreach(jn_stats, key, v) {
        if(!empty_string(stats)) {
            if(strstr(stats, key)==0) {
                continue;
            }
        }
        json_object_set(jn_data, key, v);
    }

    KW_DECREF(kw);
    return jn_data;
}

