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
PUBLIC json_t * build_stats(hgobj gobj, const char *stats, json_t *kw, hgobj src)
{
    if(!gobj) {
        KW_DECREF(kw);
        return 0;
    }

    const sdata_desc_t *sdesc = sdata_schema(gobj_hsdata(gobj));
    const sdata_desc_t *it;
    if(stats && strcmp(stats, "__reset__")==0) {
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
        stats = "";
    }

    json_t *jn_data = json_object();

    it = sdesc;
    while(it && it->name) {
        if(!(ASN_IS_NUMBER(it->type) || ASN_IS_STRING(it->type))) {
            it++;
            continue;
        }
        if(!empty_string(stats)) {
            if(!strstr(stats, it->name)) {
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
    KW_DECREF(kw);
    return jn_data;
}

