/***********************************************************************
 *          C_PERSISTENCE.C
 *          GClass of PERSISTENCE gobj.
 *
 *          Copyright (c) 2013 Niyamaka.
 *          All Rights Reserved.
 ***********************************************************************/
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <glob.h>
#include "c_store.h"

/***************************************************************************
 *              Constants
 ***************************************************************************/

/***************************************************************************
 *              Prototypes
 ***************************************************************************/

/***************************************************************************
 *              Structures
 ***************************************************************************/

/***************************************************************************
 *              Prototypes
 ***************************************************************************/
PRIVATE int delete_ievent_from_disk(hgobj gobj, const char* path);

/***************************************************************************
 *          Data: config, public data, private data
 ***************************************************************************/

/*---------------------------------------------*
 *      Attributes - order affect to oid's
 *---------------------------------------------*/
PRIVATE sdata_desc_t tattr_desc[] = {
//WARNING:  you must check persitence with maximum load of ivents in memory
//          in order to check if the memory is well dimensioned.
//          And check too to load ievents in disk until exhaust the disk capacity.
SDATA (ASN_UNSIGNED,    "timeout_agecheck",     0,  1*60*1000, "poll timeout for age check"),
SDATA_END()
};

/*---------------------------------------------*
 *      GClass trace levels
 *---------------------------------------------*/
enum {
    TRACE_PERSISTENT_IEVENTS = 0x0001,
};
PRIVATE const trace_level_t s_user_trace_level[16] = {
{"persistent_ievents",        "Trace save and load of persistent inter-events"},
{0, 0},
};

/*---------------------------------------------*
 *              Private data
 *---------------------------------------------*/
typedef struct _PRIVATE_DATA {
    int timeout_agecheck;
    const char *my_yuno_name;
    const char *my_role;
    hgobj timer;
} PRIVATE_DATA;




            /***************************
             *      Framework Methods
             ***************************/




/***************************************************************************
 *      Framework Method create
 ***************************************************************************/
PRIVATE void mt_create(hgobj gobj)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    priv->my_yuno_name = gobj_yuno_name();
    priv->my_role = gobj_yuno_role();

    /*
     *  Set the periodic timer to retry resend
     */
    priv->timer = gobj_create("", GCLASS_TIMER, 0, gobj);

    /*
     *  Do copy of heavy used parameters, for quick access.
     *  HACK The writable attributes must be repeated in mt_writing method.
     */
    SET_PRIV(timeout_agecheck,      gobj_read_uint32_attr)
}

/***************************************************************************
 *      Framework Method writing
 ***************************************************************************/
PRIVATE void mt_writing(hgobj gobj, const char *path)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    IF_EQ_SET_PRIV(timeout_agecheck,        gobj_read_uint32_attr)
    END_EQ_SET_PRIV()
}

/***************************************************************************
 *      Framework Method start
 ***************************************************************************/
PRIVATE int mt_start(hgobj gobj)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    gobj_start(priv->timer);
    set_timeout_periodic(priv->timer, priv->timeout_agecheck);
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




/***************************************************************************
 *  Los almacences se encuentran en el directorio
 *      .../store
 *
 *  Hay 3 niveles de clasificación:
 *      - store     (almacen)       OBLIGATORIO
 *      - room      (sala)          OPCIONAL
 *      - shelf     (estanteria)    OPCIONAL
 *      - reference (el documento)  OBLIGATORIO
 *
 *  TODO FUTURE
 *  Para los yunos clónicos, como su room/shelf será común para todos,
 *  usaremos `owner` para marcar de quien es el documento.
 *  La idea es que cada clónico posea el documento que tenga en memoria,
 *  pero que lo libere al terminar (quitandole la extensión .owner)
 *  De esta manera el documento queda libre para que lo pueda coger
 *  y procesar otro clon.
 *  Esto solo si la entrega es "a todos".
 *  Si la entrega es "con uno basta", entonces???
 ***************************************************************************/
PRIVATE char *store_path(
    hgobj gobj,
    char *bf,
    int bflen,
    const char *store,
    const char *room,
    const char *shelf,
    const char *reference,
    int owner,
    BOOL create_directories)
{
    /*
     *  store, room and shelf are required
     */
    if(empty_string(store)) {
        log_error(0,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_SYSTEM_ERROR,
            "msg",          "%s", "store EMPTY",
            NULL
        );
        return 0;
    }
    if(empty_string(reference)) {
        log_error(0,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_SYSTEM_ERROR,
            "msg",          "%s", "reference EMPTY",
            NULL
        );
        return 0;
    }

    char subdir[180];

    if(empty_string(room)) {
        snprintf(subdir, sizeof(subdir), "%s", store);
    } else if(empty_string(shelf)) {
        snprintf(subdir, sizeof(subdir), "%s/%s", store, room);
    } else {
        snprintf(subdir, sizeof(subdir), "%s/%s/%s", store, room, shelf);
    }

    char filename[80];
    if(!owner)
        snprintf(filename, sizeof(filename), "%s.json", reference);
    else
        snprintf(filename, sizeof(filename), "%s-%d.json", reference, owner);


    snprintf(bf, bflen, "%s/%s/%s/%s",
        yuneta_global_store_path(),
        gobj_yuno_role_plus_name(),
        subdir,
        filename
    );

    if(create_directories) {
        if(access(bf, 0)!=0) {
            mkrdir(bf, 0, yuneta_xpermission());
            if(access(bf, 0)!=0) {
                log_error(0,
                    "gobj",         "%s", gobj_full_name(gobj),
                    "function",     "%s", __FUNCTION__,
                    "msgset",       "%s", MSGSET_SYSTEM_ERROR,
                    "msg",          "%s", "Cannot create directory",
                    "path",         "%s", bf,
                    NULL
                );
                return 0;
            }
        }
    }
    return bf;
}

/***************************************************************************
 *  time with usec.
 *  in nsec with 8 bytes is enough, there is no more accuracy
 *
 *  in sec with  8 bytes we cover until 2106-02-07  (perhaps NOT ENOUGH ☺)
 *               9 bytes we cover until 4147-08-20  (ENOUGH!!!) long live for yuneta!
 *              10 bytes we cover until 36812-02-20 (ENOUGH!!!)
 *
 *  Buffer size: at least 18+1 bytes!
 ***************************************************************************/
PRIVATE char *get_time(char *temp, size_t temp_size)
{
    struct timespec ts;

    clock_gettime(CLOCK_REALTIME, &ts);
    snprintf(temp, temp_size, "%09lX-%08lX", ts.tv_sec, ts.tv_nsec);

    return temp;
}

/***************************************************************************
 *  make the reference of a persistent event
 *  The schema is: priority+currenttime+pointer
 *                 9-FFFFFFFFF-FFFFFFFF-FFFFFFFFFFFFFFFF-FFFFFFFF       (47+1 bytes)
 *                 9-FFFFFFFFF-FFFFFFFF-FFFFFFFFFFFFFFFF-FFFFFFFF.json  (52+1 bytes for file)
 *  To know the priority you must order by reference:
 *      -- the low item is the highest priority --
 *
 *  bf MUST be AT LEAST of 38+1 bytes.
 ***************************************************************************/
PRIVATE char *make_reference(hgobj gobj,
    char *bf,
    size_t bfsize,
    hsdata inter_event)
{
    static uint32_t counter = 0;
    char stime[20];

    unsigned priority = (unsigned)kw_get_int(
        sdata_read_kw(inter_event, "iev_kw"),
        "__event_priority__",
        5,
        FALSE
    );
    if(!priority) {
        priority = 5;
    }
    if(priority > 9) {
        priority = 9;
    }
    get_time(stime, sizeof(stime));
    counter++;
    snprintf(bf, bfsize,
        "%d-%s-%016lX-%08X",
        priority,
        stime,
        (long unsigned)inter_event,
        counter
    );
    return bf;
}

/***************************************************************************
 *  Save event
 *  It would be convenient save safe? encripted?
 *  inter_event is twin and decref
 ***************************************************************************/
PRIVATE int save_ievent_to_disk(hgobj gobj, const char *path, hsdata inter_event)
{
    hsdata twin_inter_event = iev_twin(inter_event);
    GBUFFER *gbuf = iev2gbuffer(twin_inter_event, TRUE); // inter_event decref

    /*----------------------------*
     *  Create the file
     *----------------------------*/
    int fd = newfile(path, yuneta_rpermission(), TRUE);
    if(fd<0) {
        log_error(0,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_SYSTEM_ERROR,
            "msg",          "%s", "newfile() FAILED",
            "path",         "%s", path,
            "errno",        "%d", errno,
            "strerror",     "%s", strerror(errno),
            NULL
        );
        GBUF_DECREF(gbuf);
        iev_decref(inter_event);
        return -1;
    }

    /*----------------------------*
     *  Write the data
     *----------------------------*/
    int ret = 0;
    size_t len;
    gbuf_reset_rd(gbuf); //TODO DANGER deja el pointer donde estaba
    while((len=gbuf_chunk(gbuf))>0) {
        char *p = gbuf_get(gbuf, len);
        if(write(fd, p, len)!=len) {
            log_error(0,
                "gobj",         "%s", gobj_full_name(gobj),
                "function",     "%s", __FUNCTION__,
                "msgset",       "%s", MSGSET_SYSTEM_ERROR,
                "msg",          "%s", "write() FAILED",
                "errno",        "%d", errno,
                "strerror",     "%s", strerror(errno),
                NULL
            );
            ret = -1;
            break;
        }
    }
    close(fd);
    gbuf_decref(gbuf);
    if(ret == 0) {
        if(gobj_trace_level(gobj) & TRACE_PERSISTENT_IEVENTS) {
            log_info(0,
                "gobj",         "%s", gobj_full_name(gobj),
                "function",     "%s", __FUNCTION__,
                "msgset",       "%s", MSGSET_PERSISTENT_IEVENTS,
                "msg",          "%s", "IEvent saved",
                "path",         "%s", path,
                NULL
            );
        }
    }
    iev_decref(inter_event);

    return ret;
}

/***************************************************************************
 *  Delete intra event from disk
 ***************************************************************************/
PRIVATE int delete_ievent_from_disk(hgobj gobj, const char *path)
{
    if(empty_string(path)) {
        log_error(0,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_INTERNAL_ERROR,
            "msg",          "%s", "path EMPTY",
            NULL
        );
        return -1;
    }
    if(access(path, 0)!=0) {
        log_error(0,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_INTERNAL_ERROR,
            "msg",          "%s", "path NOT FOUND",
            "path",         "%s", path,
            NULL
        );
        return -1;
    }
    if(unlink(path)<0) {
        log_error(0,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_SYSTEM_ERROR,
            "msg",          "%s", "unlink() FAILED",
            "path",         "%s", path,
            "errno",        "%d", errno,
            "strerror",     "%s", strerror(errno),
            NULL
        );
        return -1;
    }

    if(gobj_trace_level(gobj) & TRACE_PERSISTENT_IEVENTS) {
        log_info(0,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_PERSISTENT_IEVENTS,
            "msg",          "%s", "IEvent removed",
            "path",         "%s", path,
            NULL
        );
    }
    return 0;
}

/***************************************************************************
 *  Print errors found by glob()
 ***************************************************************************/
PRIVATE int print_error_glob(const char *epath, int eerrno)
{
    log_error(0,
        "gobj",         "%s", "c_persistence",
        "function",     "%s", __FUNCTION__,
        "msgset",       "%s", MSGSET_SYSTEM_ERROR,
        "msg",          "%s", "glob() FAILED",
        "path",         "%s", epath,
        "error",        "%d", eerrno,
        "strerror",     "%s", strerror(eerrno),
        NULL
    );
    return 0;
}

/***************************************************************************
 *  Print errors found by glob()
 ***************************************************************************/
PRIVATE int print_error_glob2(const char *epath, int eerrno)
{
    // Ignore No such file or directory
    if(eerrno != ENOENT) {
        log_error(0,
            "gobj",         "%s", "c_persistence",
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_SYSTEM_ERROR,
            "msg",          "%s", "glob() FAILED",
            "path",         "%s", epath,
            "error",        "%d", eerrno,
            "strerror",     "%s", strerror(eerrno),
            NULL
        );
    }
    return 0;
}




            /***************************
             *      Actions
             ***************************/




/***************************************************************************
 *  Store a ievent
 ***************************************************************************/
PRIVATE int ac_save_ievent(hgobj gobj, const char *event, json_t *kw, hgobj src)
{
    hsdata inter_event = (hsdata)kw_get_int(kw, "__inter_event__", 0, 0);
    const char *store = kw_get_str(kw, "store", 0, 0);
    const char *room = kw_get_str(kw, "room", 0, 0);
    const char *shelf = kw_get_str(kw, "shelf", 0, 0);
    int owner = kw_get_int(kw, "owner", 0, 0);

    /*-------------------------------------------------------*
     *  Build a reference:
     *  To know the priority you must order by reference:
     *      -- the low item is the highest priority --
     *-------------------------------------------------------*/
    char new_reference[60];
    json_t *iev_kw = sdata_read_kw(inter_event, "iev_kw");
    const char *reference = GET_PERSISTENT_REFERENCE(iev_kw);
    if(empty_string(reference)) {
        make_reference(gobj, new_reference, sizeof(new_reference), inter_event);
        json_object_set_new(
            iev_kw,
            "__persistence_reference__",
            json_string(new_reference)
        );
        json_object_set_new(
            iev_kw,
            "__event_retries__",
            json_integer(0)
        );
        reference = new_reference;
    }

    /*--------------------------------------*
     *  Build the ievent path in disk
     *--------------------------------------*/
    char path[256];
    if(!store_path(gobj, path, sizeof(path), store, room, shelf, reference, owner, TRUE)) {
        log_error(0,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_INTERNAL_ERROR,
            "msg",          "%s", "store_path() FAILED",
            "store",        "%s", store,
            "room",         "%s", room,
            "shelf",        "%s", shelf,
            "reference",    "%s", reference,
            "owner",        "%d", owner,
            NULL
        );
        iev_decref(inter_event);
        JSON_DECREF(kw);
        return -1;
    }

    /*----------------------------------------*
     *  Save the ievent with their reference
     *----------------------------------------*/
    save_ievent_to_disk(gobj, path, inter_event); // inter_event decref!

    JSON_DECREF(kw);
    return 1;
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE int ac_load_ievent(hgobj gobj, const char *event, json_t *kw, hgobj src)
{
    const char *event2send = kw_get_str(kw, "event2send", 0, 0);
    const char *store = kw_get_str(kw, "store", 0, 0);
    const char *room = kw_get_str(kw, "room", 0, 0);
    const char *shelf = kw_get_str(kw, "shelf", 0, 0);
    const char *reference = kw_get_str(kw, "reference", 0, 0);
    int maximum_events = kw_get_int(kw, "maximum_events", 0, 0);
    int owner = kw_get_int(kw, "owner", 0, 0);
    json_t *extra_kw = kw_get_dict(kw, "extra_kw", 0, 0);

    glob_t pglob;

    char path[256];
    if(!store_path(gobj, path, sizeof(path), store, room, shelf, reference, owner, FALSE)) {
        log_error(0,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_INTERNAL_ERROR,
            "msg",          "%s", "store_path() FAILED",
            "store",        "%s", store,
            "room",         "%s", room,
            "shelf",        "%s", shelf,
            "reference",    "%s", reference,
            "owner",        "%d", owner,
            NULL
        );
        JSON_DECREF(kw);
        return -1;
    }

    int ret = glob(path, 0, print_error_glob2, &pglob);
    if(ret!=0) {
        if(ret!=GLOB_NOMATCH) {
            log_error(0,
                "gobj",         "%s", gobj_full_name(gobj),
                "function",     "%s", __FUNCTION__,
                "msgset",       "%s", MSGSET_SYSTEM_ERROR,
                "msg",          "%s", "glob() FAILED",
                "error",        "%d", errno,
                "strerror",     "%s", strerror(errno),
                NULL
            );
            return -1;
        }
        globfree(&pglob);
        JSON_DECREF(kw);
        return 0;
    }
    int i;
    for(i = 0; i < pglob.gl_pathc; i++) {
        hsdata iev = iev_create_from_file(pglob.gl_pathv[i]);
        if(!iev) {
            log_error(0,
                "gobj",         "%s", gobj_full_name(gobj),
                "function",     "%s", __FUNCTION__,
                "msgset",       "%s", MSGSET_INTERNAL_ERROR,
                "msg",          "%s", "iev_create_from_file() FAILED",
                NULL
            );
            continue;
        }
        if(!iev_retry_timeout_elapsed(iev)) {
            iev_decref(iev);
            continue;
        }
        iev_inc_retries(iev);
        if(!iev_is_valid(iev)) {
            log_warning(0,
                "gobj",         "%s", gobj_full_name(gobj),
                "function",     "%s", __FUNCTION__,
                "msgset",       "%s", MSGSET_RUNTIME_ERROR,
                "msg",          "%s", "Deleting IEvent by not valid",
                NULL
            );
            trace_inter_event("Deleting IEvent by not valid", iev);
            delete_ievent_from_disk(gobj, pglob.gl_pathv[i]);
            iev_decref(iev);
            continue;
        } else {
            iev_incref(iev);
            save_ievent_to_disk(gobj, pglob.gl_pathv[i], iev); // inter_event decref!
        }

        /*
         *  Don't send directly although you know the route:
         *  must pass the retries and age filter
         *  ??? este servicio lo usarán otros además del router,
         *  así es que la afirmación anterior hay que revisarla.
         */
        json_t *iev_kw = sdata_read_kw(iev, "iev_kw");
        json_object_set_new(
            iev_kw,
            "__inter_event__",
            json_integer((json_int_t)(size_t)iev)
        );
        const char *ev = event2send;
        if(empty_string(ev)) {
            ev = sdata_read_str(iev, "iev_event");
        }
        if(extra_kw) {
            json_object_update(iev_kw, extra_kw);
        }
        kw_incref(iev_kw);
        gobj_send_event(src, ev, iev_kw, gobj);
        if(iev_lives(iev) > 0) {
            /*
             *  Si es el router quien carga los inter-eventos persistentes,
             *  como entra con el evento EV_SEND_INTER_EVENT,
             *  entonces borra el inter-evento, pues es el parametro que espera.
             */
            iev_decref(iev);
        }

        if(i >= maximum_events) {
            break;
        }
    }
    globfree(&pglob);

    JSON_DECREF(kw);
    return i;
}

/***************************************************************************
 *  Delete ievent from store and give back
 ***************************************************************************/
PRIVATE int ac_remove_ievent(hgobj gobj, const char *event, json_t *kw, hgobj src)
{
    const char *store = kw_get_str(kw, "store", 0, 0);
    const char *room = kw_get_str(kw, "room", 0, 0);
    const char *shelf = kw_get_str(kw, "shelf", 0, 0);
    const char *reference = kw_get_str(kw, "reference", 0, 0);
    int owner = kw_get_int(kw, "owner", 0, 0);

    glob_t pglob;

    char path[256];
    if(!store_path(gobj, path, sizeof(path), store, room, shelf, reference, owner, FALSE)) {
        log_error(0,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_INTERNAL_ERROR,
            "msg",          "%s", "store_path() FAILED",
            "store",        "%s", store,
            "room",         "%s", room,
            "shelf",        "%s", shelf,
            "reference",    "%s", reference,
            "owner",        "%d", owner,
            NULL
        );
        KW_DECREF(kw);
        return -1;
    }

    int ret = glob(path, 0, print_error_glob, &pglob);
    if(ret!=0) {
        { //if(ret!=GLOB_NOMATCH)
            log_error(0,
                "gobj",         "%s", gobj_full_name(gobj),
                "function",     "%s", __FUNCTION__,
                "msgset",       "%s", MSGSET_SYSTEM_ERROR,
                "msg",          "%s", "glob() FAILED",
                "path",         "%s", path,
                "error",        "%d", errno,
                "strerror",     "%s", strerror(errno),
                NULL
            );
            KW_DECREF(kw);
            return -1;
        }
        globfree(&pglob);
        KW_DECREF(kw);
        return -1;
    }

    for(int i = 0; i < pglob.gl_pathc; i++) {
        delete_ievent_from_disk(gobj, pglob.gl_pathv[i]);
    }
    ret = pglob.gl_pathc;
    globfree(&pglob);

    KW_DECREF(kw);
    return ret;
}

/***************************************************************************
 *  Increment retry of ievents
 ***************************************************************************/
PRIVATE int ac_incr_ievent_retry(hgobj gobj, const char *event, json_t *kw, hgobj src)
{
    const char *store = kw_get_str(kw, "store", 0, 0);
    const char *room = kw_get_str(kw, "room", 0, 0);
    const char *shelf = kw_get_str(kw, "shelf", 0, 0);
    const char *reference = kw_get_str(kw, "reference", 0, 0);
    int owner = kw_get_int(kw, "owner", 0, 0);

    glob_t pglob;

    char path[256];
    if(!store_path(gobj, path, sizeof(path), store, room, shelf, reference, owner, FALSE)) {
        log_error(0,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_INTERNAL_ERROR,
            "msg",          "%s", "store_path() FAILED",
            "store",        "%s", store,
            "room",         "%s", room,
            "shelf",        "%s", shelf,
            "reference",    "%s", reference,
            "owner",        "%d", owner,
            NULL
        );
        return -1;
    }

    int ret = glob(path, 0, print_error_glob, &pglob);
    if(ret!=0) {
        { //if(ret!=GLOB_NOMATCH)
            log_error(0,
                "gobj",         "%s", gobj_full_name(gobj),
                "function",     "%s", __FUNCTION__,
                "msgset",       "%s", MSGSET_SYSTEM_ERROR,
                "msg",          "%s", "glob() FAILED",
                "path",         "%s", path,
                "error",        "%d", errno,
                "strerror",     "%s", strerror(errno),
                NULL
            );
            return -1;
        }
        globfree(&pglob);
        return 0;
    }

    for(int i = 0; i < pglob.gl_pathc; i++) {
        hsdata iev = iev_create_from_file(pglob.gl_pathv[i]);
        if(!iev) {
            log_error(0,
                "gobj",         "%s", gobj_full_name(gobj),
                "function",     "%s", __FUNCTION__,
                "msgset",       "%s", MSGSET_INTERNAL_ERROR,
                "msg",          "%s", "iev_create_from_file() FAILED",
                NULL
            );
            continue;
        }
        if(!iev_is_valid(iev)) {
            log_warning(0,
                "gobj",         "%s", gobj_full_name(gobj),
                "function",     "%s", __FUNCTION__,
                "msgset",       "%s", MSGSET_RUNTIME_ERROR,
                "msg",          "%s", "Deleting IEvent by not valid",
                NULL
            );
            trace_inter_event("Deleting IEvent by not valid", iev);
            delete_ievent_from_disk(gobj, pglob.gl_pathv[i]);
            iev_decref(iev);
        } else {
            save_ievent_to_disk(gobj, path, iev); // inter_event decref!
        }
    }
    ret = pglob.gl_pathc;
    globfree(&pglob);

    JSON_DECREF(kw);
    return ret;
}

/***************************************************************************
 *  Remove old ievents
 ***************************************************************************/
PRIVATE int ac_timeout_agecheck(hgobj gobj, const char *event, json_t *kw, hgobj src)
{
    //TODO
    JSON_DECREF(kw);
    return 1;
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE int ac_stopped(hgobj gobj, const char *event, json_t *kw, hgobj src)
{
    /*
     *  priv->timer  has stopped
     */
    JSON_DECREF(kw);
    return 0;
}

/***************************************************************************
 *                          FSM
 ***************************************************************************/
PRIVATE const EVENT input_events[] = {
    {"EV_SAVE_IEVENT",          0},
    {"EV_LOAD_IEVENT",          0},
    {"EV_REMOVE_IEVENT",        0},
    {"EV_INCR_IEVENT_RETRY",    0},
    {"EV_TIMEOUT",              0},
    {"EV_STOPPED",              0},
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
    {"EV_SAVE_IEVENT",          ac_save_ievent,         0},
    {"EV_LOAD_IEVENT",          ac_load_ievent,         0},
    {"EV_REMOVE_IEVENT",        ac_remove_ievent,       0},
    {"EV_INCR_IEVENT_RETRY",    ac_incr_ievent_retry,   0},
    {"EV_STOPPED",              ac_stopped,             0},
    {"EV_TIMEOUT",              ac_timeout_agecheck,    0},
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
    GCLASS_PERSISTENCE_NAME,
    &fsm,
    {
        mt_create,
        0, //mt_destroy,
        mt_start,
        mt_stop,
        mt_writing,
        0, //mt_reading,
        0, //mt_subscription_added,
        0, //mt_subscription_deleted,
        0, //mt_resource_bin,
        0, //mt_resource_desc,
        0, //mt_play,
        0, //mt_pause,
        0, //mt_resource_data,
        0, //mt_child_added,
        0, //mt_child_removed,
        0, //mt_stats,
        0, //mt_command,
        0, //mt_create2,
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
        0, //mt_future60,
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
    0, // cmds
    0, // gcflag
};

/***************************************************************************
 *              Public access
 ***************************************************************************/
PUBLIC GCLASS *gclass_persistence(void)
{
    return &_gclass;
}
