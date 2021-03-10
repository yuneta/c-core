/****************************************************************************
 *          ENTRY_POINT.C
 *          Entry point for yunos.
 *
 *          Copyright (c) 2014-2018 Niyamaka.
 *          All Rights Reserved.
 ****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <signal.h>
#include <sys/stat.h>
#include <unistd.h>
#include <argp.h>
#include "entry_point.h"

/***************************************************************************
 *      Data
 ***************************************************************************/
PRIVATE hgobj __yuno_gobj__ = 0;
PRIVATE char __realm_id__[NAME_MAX] = {0};
PRIVATE char __realm_owner__[NAME_MAX] = {0};
PRIVATE char __realm_role__[NAME_MAX] = {0};
PRIVATE char __realm_name__[NAME_MAX] = {0};
PRIVATE char __realm_env__[NAME_MAX] = {0};
PRIVATE char __yuno_role__[NAME_MAX] = {0};
PRIVATE char __yuno_name__[NAME_MAX] = {0};
PRIVATE char __yuno_tag__[NAME_MAX] = {0};

PRIVATE char __app_name__[NAME_MAX] = {0};
PRIVATE char __yuno_version__[NAME_MAX] = {0};
PRIVATE char __argp_program_version__[NAME_MAX] = {0};
PRIVATE char __app_doc__[NAME_MAX] = {0};
PRIVATE char __app_datetime__[NAME_MAX] = {0};


PRIVATE json_t *__jn_config__ = 0;

PRIVATE int __auto_kill_time__ = 0;
PRIVATE int __as_daemon__ = 0;
PRIVATE int __assure_kill_time__ = 5;
PRIVATE BOOL __quick_death__ = 0;
PRIVATE BOOL __ordered_death__ = 1;  // WARNING Vamos a probar otra vez las muertes ordenadas 22/03/2017

PRIVATE int __print__ = 0;

PRIVATE void *(*__global_startup_persistent_attrs_fn__)(void) = 0;
PRIVATE void (*__global_end_persistent_attrs_fn__)(void) = 0;
PRIVATE int (*__global_load_persistent_attrs_fn__)(hgobj gobj) = 0;
PRIVATE int (*__global_save_persistent_attrs_fn__)(hgobj gobj) = 0;
PRIVATE int (*__global_remove_persistent_attrs_fn__)(hgobj gobj) = 0;
PRIVATE json_t * (*__global_list_persistent_attrs_fn__)(hgobj gobj) = 0;

PRIVATE json_t * (*__global_command_parser_fn__)(
    hgobj gobj,
    const char *command,
    json_t *kw,
    hgobj src
) = command_parser;
PRIVATE json_t * (*__global_stats_parser_fn__)(
    hgobj gobj,
    const char *stats,
    json_t *kw,
    hgobj src
) = stats_parser;

PRIVATE authz_checker_fn __global_authz_checker_fn__ = 0;
PRIVATE authenticate_parser_fn __global_authenticate_parser_fn__ = 0;

/***************************************************************************
 *      Structures
 ***************************************************************************/
/* Used by main to communicate with parse_opt. */
struct arguments {
    int start;
    int stop;
    int use_config_file;
    int print_verbose_config;
    int print_final_config;
    int print_role;
    const char *config_json_file;
    const char *parameter_config;
    int verbose_log;
};

/***************************************************************************
 *      Prototypes
 ***************************************************************************/
PRIVATE void daemon_catch_signals(void);
PRIVATE error_t parse_opt(int key, char *arg, struct argp_state *state);
PRIVATE void process(const char *process_name, const char *work_dir, const char *domain_dir);


/***************************************************************************
 *                      argp setup
 ***************************************************************************/
const char *argp_program_bug_address;                               // Public for argp
const char *argp_program_version = __argp_program_version__;        // Public for argp

/* A description of the arguments we accept. */
PRIVATE char args_doc[] = "[{json config}]";

/* The options we understand. */
PRIVATE struct argp_option options[] = {
{"start",                   'S',    0,      0,  "Start the yuno (as daemon)"},
{"stop",                    'K',    0,      0,  "Stop the yuno (as daemon)"},
{"config-file",             'f',    "FILE", 0,  "Load settings from json config file or [files]"},
{"print-config",            'p',    0,      0,  "Print the final json config"},
{"print-verbose-config",    'P',    0,      0,  "Print verbose json config"},
{"print-role",              'r',    0,      0,  "Print the basic yuno's information"},
{"version",                 'v',    0,      0,  "Print yuno version"},
{"yuneta-version",          'V',    0,      0,  "Print yuneta version"},
{"verbose-log",             'l',    "LEVEL",0,  "Verbose log level"},
{0}
};

/* Our argp parser. */
PRIVATE struct argp argp = {
    options,
    parse_opt,
    args_doc,
    __app_doc__
};

/***************************************************************************
 *      argp parser
 ***************************************************************************/
PRIVATE error_t parse_opt(int key, char *arg, struct argp_state *state)
{
    /*
     *  Get the input argument from argp_parse,
     *  which we know is a pointer to our arguments structure.
     */
    struct arguments *arguments = state->input;

    switch (key) {
    case 'S':
        arguments->start = 1;
        break;
    case 'K':
        arguments->stop = 1;
        break;
    case 'v':
        printf("%s\n", argp_program_version);
        exit(0);
        break;
    case 'V':
        printf("%s\n", __yuneta_long_version__);
        exit(0);
        break;
    case 'f':
        arguments->config_json_file = arg;
        arguments->use_config_file = 1;
        break;
    case 'p':
        arguments->print_final_config = 1;
        break;
    case 'r':
        arguments->print_role = 1;
        break;
    case 'P':
        arguments->print_verbose_config = 1;
        break;
    case 'l':
        if(arg) {
            arguments->verbose_log = atoi(arg);
        }
        break;

    case ARGP_KEY_ARG:
        if (state->arg_num >= 1) {
            /* Too many arguments. */
            argp_usage (state);
        }
        arguments->parameter_config = arg;
        break;

    default:
        return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

/***************************************************************************
 *      Signal handlers
 ***************************************************************************/
PRIVATE void quit_sighandler(int sig)
{
    static int tries = 0;
    int trace = daemon_get_debug_mode();

    if(trace) {
        print_info("INFO YUNETA", "Sighandler signal %d, process %s, pid %d",
            sig,
            get_process_name(),
            getpid()
        );
    }

    /*
     *  __yuno_gobj__ is 0 for watcher fork, if we are running --stop
     */
    hgobj gobj = __yuno_gobj__;

    if(gobj) {
        if(__quick_death__ && !__ordered_death__) {
            _exit(0);
        }
        tries++;
        gobj_set_yuno_must_die();

        if(!__auto_kill_time__) {
            if(__assure_kill_time__ > 0)
                alarm(__assure_kill_time__); // que se muera a la fuerza en 5 o 10 seg.
        }
        if(tries > 1) {
            mt_clean(gobj);   // Display pending uv handlers
            _exit(-1);
        }
        return;
    }

    print_error(0, "ERROR YUNETA", "Signal handler without __yuno_gobj__, signal %d, pid %d", sig, getpid());
}

/***************************************************************************
 *      Signal handlers
 ***************************************************************************/
PRIVATE void raise_sighandler(int sig)
{
    /*
     *  __yuno_gobj__ is 0 for watcher fork, if we are running --stop
     */
    hgobj gobj = __yuno_gobj__;

    if(gobj) {
        gobj_set_panic_trace(1);
        log_error(LOG_OPT_TRACE_STACK,
            "gobj",         "%s", __FILE__,
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_RUNTIME_ERROR,
            "msg",          "%s", "Kill -10",
            NULL
        );
    }
}

PRIVATE void daemon_catch_signals(void)
{
    struct sigaction sigIntHandler;

    signal(SIGPIPE, SIG_IGN);

    memset(&sigIntHandler, 0, sizeof(sigIntHandler));
    sigIntHandler.sa_handler = quit_sighandler;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = SA_NODEFER|SA_RESTART;
    sigaction(SIGALRM, &sigIntHandler, NULL);   // to debug in kdevelop
    sigaction(SIGQUIT, &sigIntHandler, NULL);
    sigaction(SIGINT, &sigIntHandler, NULL);    // ctrl+c
    sigaction(SIGTERM, &sigIntHandler, NULL);

    sigIntHandler.sa_handler = raise_sighandler;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = SA_NODEFER|SA_RESTART;
    sigaction(SIGUSR1, &sigIntHandler, NULL);
}

/***************************************************************************
 *  DEPRECATED Set functions of gobj_start_up() function.
 ***************************************************************************/
PUBLIC int yuneta_set_gobj_startup_functions(
    int (*load_persistent_attrs)(hgobj gobj),
    int (*save_persistent_attrs)(hgobj gobj),
    int (*remove_persistent_attrs)(hgobj gobj),
    json_t * (*list_persistent_attrs)(hgobj gobj),
    json_function_t global_command_parser,
    json_function_t global_stats_parser
)
{
    __global_load_persistent_attrs_fn__ = load_persistent_attrs;
    __global_save_persistent_attrs_fn__ = save_persistent_attrs;
    __global_remove_persistent_attrs_fn__ = remove_persistent_attrs;
    __global_list_persistent_attrs_fn__ = list_persistent_attrs;
    __global_command_parser_fn__ = global_command_parser;
    __global_stats_parser_fn__ = global_stats_parser;

    return 0;
}

/***************************************************************************
 *  New yuneta setup function.
 ***************************************************************************/
PUBLIC int yuneta_setup(
    void *(*startup_persistent_attrs)(void),
    void (*end_persistent_attrs)(void),
    int (*load_persistent_attrs)(hgobj gobj),
    int (*save_persistent_attrs)(hgobj gobj),
    int (*remove_persistent_attrs)(hgobj gobj),
    json_t * (*list_persistent_attrs)(hgobj gobj),
    json_function_t global_command_parser,
    json_function_t global_stats_parser,
    authz_checker_fn global_authz_checker,
    authenticate_parser_fn global_authenticate_parser
)
{
    __global_startup_persistent_attrs_fn__ = startup_persistent_attrs;
    __global_end_persistent_attrs_fn__ = end_persistent_attrs;
    __global_load_persistent_attrs_fn__ = load_persistent_attrs;
    __global_save_persistent_attrs_fn__ = save_persistent_attrs;
    __global_remove_persistent_attrs_fn__ = remove_persistent_attrs;
    __global_list_persistent_attrs_fn__ = list_persistent_attrs;
    if(global_command_parser) {
        __global_command_parser_fn__ = global_command_parser;
    }
    if(global_stats_parser) {
        __global_stats_parser_fn__ = global_stats_parser;
    }
    __global_authz_checker_fn__ = global_authz_checker;
    __global_authenticate_parser_fn__ = global_authenticate_parser;

    return 0;
}

/***************************************************************************
 *                      Main
 ***************************************************************************/
PUBLIC int yuneta_entry_point(int argc, char *argv[],
    const char *APP_NAME,
    const char *APP_VERSION,
    const char *APP_SUPPORT,
    const char *APP_DOC,
    const char *APP_DATETIME,
    const char *fixed_config,
    const char *variable_config,
    void (*register_yuno_and_more)(void))
{
    snprintf(__argp_program_version__, sizeof(__argp_program_version__),
        "%s %s %s",
        APP_NAME,
        APP_VERSION,
        APP_DATETIME
    );
    argp_program_bug_address = APP_SUPPORT;
    strncpy(__yuno_version__, APP_VERSION, sizeof(__yuno_version__)-1);
    strncpy(__app_name__, APP_NAME, sizeof(__app_name__)-1);
    strncpy(__app_datetime__, APP_DATETIME, sizeof(__app_datetime__)-1);
    strncpy(__app_doc__, APP_DOC, sizeof(__app_doc__)-1);

    /*------------------------------------------------*
     *  Process name = yuno role
     *  This name is limited to 15 characters.
     *      - 15+null by Linux (longer names are not showed in top)
     *------------------------------------------------*/
    if(strlen(APP_NAME) > 15) {
        print_error(
            PEF_EXIT,
            "ERROR YUNETA",
            "role name '%s' TOO LONG!, maximum is 15 characters",
            APP_NAME
        );
    }

    /*------------------------------------------------*
     *  Process name = yuno role
     *------------------------------------------------*/
    const char *process_name = APP_NAME;

    /*------------------------------------------------*
     *  Init ghelpers
     *------------------------------------------------*/
    init_ghelpers_library(process_name);
    log_startup(APP_NAME, APP_VERSION, argv[0]);

    /*------------------------------------------------*
     *          Parse input arguments
     *------------------------------------------------*/
    struct arguments arguments;
    memset(&arguments, 0, sizeof(struct arguments));
    argp_parse(&argp, argc, argv, 0, 0, &arguments);

    if(arguments.stop) {
        daemon_shutdown(process_name);
        return 0;
    }
    if(arguments.start) {
        __as_daemon__ = 1;
        __quick_death__ = 1;
    }
    if(arguments.print_verbose_config ||
            arguments.print_final_config ||
            arguments.print_role) {
        __print__ = TRUE;
    }

    /*------------------------------------------------*
     *          Load json config
     *------------------------------------------------*/
    char *sconfig = json_config(
        arguments.print_verbose_config,     // WARNING if true will exit(0)
        arguments.print_final_config,       // WARNING if true will exit(0)
        fixed_config,
        variable_config,
        arguments.use_config_file? arguments.config_json_file: 0,
        arguments.parameter_config,
        PEF_EXIT
    );
    if(!sconfig) {
        print_error(
            PEF_EXIT,
            "ERROR YUNETA",
            "json_config() of '%s' failed",
            APP_NAME
        );
    }
    json_t *jn_temp_config = legalstring2json(sconfig, TRUE);
    if(!jn_temp_config) {
        print_error(
            PEF_EXIT,
            "ERROR YUNETA",
            "legalstring2json() of '%s' failed",
            APP_NAME
        );
    }

    /*------------------------------------------------*
     *          Load temp environment config
     *------------------------------------------------*/
    int xpermission = 02775;
    int rpermission = 0664;
    const char *work_dir = 0;           /* by default total silence */
    const char *domain_dir = 0;         /* by default total silence */
    size_t buffer_uv_read = 8*1024;
    json_int_t MEM_MIN_BLOCK = 32;                      /* smaller memory block */
    json_int_t MEM_MAX_BLOCK = 64*1024LL;               /* largest memory block */
    json_int_t MEM_SUPERBLOCK = 128*1024LL;             /* super-block size */
    json_int_t MEM_MAX_SYSTEM_MEMORY = 1*1024LL*1024LL; /* maximum core memory */
    BOOL use_system_memory = FALSE;

    json_t *jn_temp_environment = kw_get_dict(jn_temp_config, "environment", 0, 0);
    if(jn_temp_environment && !__print__) {
        /*
         *  WARNING
         *  In this point json config is temporal.
         *  Be care with pointers!
         */
        buffer_uv_read = kw_get_int(jn_temp_environment, "buffer_uv_read", buffer_uv_read, 0);
        xpermission = (int)kw_get_int(jn_temp_environment, "xpermission", xpermission, 0);
        rpermission = (int)kw_get_int(jn_temp_environment, "rpermission", rpermission, 0);
        work_dir = kw_get_str(jn_temp_environment, "work_dir", 0, 0);
        domain_dir = kw_get_str(jn_temp_environment, "domain_dir", 0, 0);
        register_yuneta_environment(
            work_dir,
            domain_dir,
            xpermission,
            rpermission,
            0
        );

        json_t *jn_log_handlers = kw_get_dict(
            jn_temp_environment,
            __as_daemon__? "daemon_log_handlers":"console_log_handlers",
            0,
            0
        );
        const char *key;
        json_t *kw;
        json_object_foreach(jn_log_handlers, key, kw) {
            const char *handler_type = kw_get_str(kw, "handler_type", "", 0);
            log_handler_opt_t handler_options = LOG_OPT_LOGGER;
            KW_GET(handler_options, handler_options, kw_get_int)
            if(strcmp(handler_type, "stdout")==0) {
                if(arguments.verbose_log) {
                    handler_options = arguments.verbose_log;
                }
                log_add_handler(key, handler_type, handler_options, 0);

            } else if(strcmp(handler_type, "file")==0) {
                const char *filename_mask = kw_get_str(kw, "filename_mask", 0, 0);
                if(!empty_string(filename_mask)) {
                    char temp[NAME_MAX];
                    char *path = yuneta_log_file(temp, sizeof(temp), filename_mask, TRUE);
                    if(path) {
                        size_t bf_size = 0;                     // 0 = default 64K
                        int max_megas_rotatoryfile_size = 0;    // 0 = default 8
                        int min_free_disk_percentage = 0;       // 0 = default 10

                        KW_GET(bf_size, bf_size, kw_get_int)
                        KW_GET(max_megas_rotatoryfile_size, max_megas_rotatoryfile_size, kw_get_int)
                        KW_GET(min_free_disk_percentage, min_free_disk_percentage, kw_get_int)

                        hrotatory_t hr = rotatory_open(
                            path,
                            bf_size,
                            max_megas_rotatoryfile_size,
                            min_free_disk_percentage,
                            xpermission,
                            rpermission,
                            TRUE    // exit on failure
                        );
                        log_add_handler(key, handler_type, handler_options, hr);
                    }
                }

            } else if(strcmp(handler_type, "udp")==0 ||
                    strcmp(handler_type, "upd")==0) { // "upd" is a sintax bug in all versions <= 2.2.5
                const char *url = kw_get_str(kw, "url", 0, 0);
                if(!empty_string(url)) {
                    size_t bf_size = 0;                 // 0 = default 64K
                    size_t udp_frame_size = 0;          // 0 = default 1500
                    ouput_format_t output_format = 0;   // 0 = default OUTPUT_FORMAT_YUNETA
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
                        log_add_handler(key, "udp", handler_options, udpc);
                    }
                }
            }
        }

        MEM_MIN_BLOCK = kw_get_int(
            jn_temp_environment,
            "MEM_MIN_BLOCK",
            MEM_MIN_BLOCK,
            0
        );
        MEM_MAX_BLOCK = kw_get_int(
            jn_temp_environment,
            "MEM_MAX_BLOCK",
            MEM_MAX_BLOCK,
            0
        );
        MEM_SUPERBLOCK = kw_get_int(
            jn_temp_environment,
            "MEM_SUPERBLOCK",
            MEM_SUPERBLOCK,
            0
        );
        MEM_MAX_SYSTEM_MEMORY = kw_get_int(
            jn_temp_environment,
            "MEM_MAX_SYSTEM_MEMORY",
            MEM_MAX_SYSTEM_MEMORY,
            0
        );
        use_system_memory = kw_get_bool(jn_temp_environment, "use_system_memory", 0, 0);
        BOOL log_gbmem_info = kw_get_bool(jn_temp_environment, "log_gbmem_info", 0, 0);
        gbmem_enable_log_info(log_gbmem_info);
        jn_temp_environment = 0; // protect, no more use
    }
    JSON_DECREF(jn_temp_config); // free allocated with default free

    /*------------------------------------------------*
     *          Setup memory
     *------------------------------------------------*/
    if(!__print__ && !use_system_memory) {
        gbmem_startup( /* Create memory core */
            MEM_MIN_BLOCK,
            MEM_MAX_BLOCK,
            MEM_SUPERBLOCK,
            MEM_MAX_SYSTEM_MEMORY,
            NULL,               /* system memory functions */
            0
        );

    } else {
        gbmem_startup_system(
            MEM_MAX_BLOCK,
            MEM_MAX_SYSTEM_MEMORY
        );
    }

    /*
     *  WARNING now all json is gbmem allocated
     */
    json_set_alloc_funcs(
        gbmem_malloc,
        gbmem_free
    );
    uv_replace_allocator(
        gbmem_malloc,
        gbmem_realloc,
        gbmem_calloc,
        gbmem_free
    );
    if(buffer_uv_read > gbmem_get_maximum_block()) {
        buffer_uv_read = gbmem_get_maximum_block();
    }
    init_growbf(
        buffer_uv_read,
        gbmem_malloc,
        gbmem_realloc,
        gbmem_free
    );

    /*------------------------------------------------*
     *          Re-alloc with gbmem
     *------------------------------------------------*/
    __jn_config__ = legalstring2json(sconfig, TRUE); //
    if(!__jn_config__) {
        print_error(
            PEF_EXIT,
            "ERROR YUNETA",
            "legalstring2json() of '%s' failed",
            APP_NAME
        );
    }
    free(sconfig);  // HACK I know that sconfig is malloc'ed

    /*------------------------------------------------*
     *      Re-read
     *------------------------------------------------*/
    work_dir = kw_get_str(__jn_config__, "environment`work_dir", "", 0);
    domain_dir = kw_get_str(__jn_config__, "environment`domain_dir", "", 0);
    const char *realm_id  = kw_get_str(__jn_config__, "environment`realm_id", "", 0);
    const char *realm_owner  = kw_get_str(__jn_config__, "environment`realm_owner", "", 0);
    const char *realm_role  = kw_get_str(__jn_config__, "environment`realm_role", "", 0);
    const char *realm_name  = kw_get_str(__jn_config__, "environment`realm_name", "", 0);
    const char *realm_env  = kw_get_str(__jn_config__, "environment`realm_env", "", 0);

    register_yuneta_environment(
        work_dir,
        domain_dir,
        xpermission,
        rpermission,
        __jn_config__
    );

    /*------------------------------------------------*
     *      Get yuno attributes.
     *------------------------------------------------*/
    json_t *jn_yuno = kw_get_dict(__jn_config__, "yuno", 0, 0);
    if(!jn_yuno) {
        print_error(
            PEF_EXIT,
            "ERROR YUNETA",
            "'yuno' dict NOT FOUND in json config"
        );
    }
    const char *yuno_role  = kw_get_str(jn_yuno, "yuno_role", "", 0);
    const char *yuno_name  = kw_get_str(jn_yuno, "yuno_name", "", 0);
    const char *yuno_tag  = kw_get_str(jn_yuno, "yuno_tag", "", 0);
    if(empty_string(yuno_role)) {
        print_error(
            PEF_EXIT,
            "ERROR YUNETA",
            "'yuno_role' is EMPTY"
        );
    }
    if(strcmp(yuno_role, APP_NAME)!=0) {
        print_error(
            PEF_EXIT,
            "ERROR YUNETA",
            "yuno_role '%s' and process name '%s' MUST MATCH!",
            yuno_role,
            APP_NAME
        );
    }
    snprintf(__realm_id__, sizeof(__realm_id__), "%s", realm_id);
    snprintf(__realm_owner__, sizeof(__realm_owner__), "%s", realm_owner);
    snprintf(__realm_role__, sizeof(__realm_role__), "%s", realm_role);
    snprintf(__realm_name__, sizeof(__realm_name__), "%s", realm_name);
    snprintf(__realm_env__, sizeof(__realm_env__), "%s", realm_env);
    snprintf(__yuno_role__, sizeof(__yuno_role__), "%s", yuno_role);
    snprintf(__yuno_name__, sizeof(__yuno_name__), "%s", yuno_name);
    snprintf(__yuno_tag__, sizeof(__yuno_tag__), "%s", yuno_tag);

    set_process_name2(__yuno_role__, __yuno_name__);

    /*----------------------------------------------*
     *  Check executable name versus yuno_role
     *  Paranoid!
     *----------------------------------------------*/
    if(1) {
        char *xx = strdup(argv[0]);
        if(xx) {
            char *bname = basename(xx);
            char *p = strrchr(bname, '.');
            if(p && (strcmp(p, ".pm")==0 || strcasecmp(p, ".exe")==0)) {
                *p = 0; // delete extensions (VOS and Windows)
            }
            p = strchr(bname, '^');
            if(p)
                *p = 0; // delete name segment
            if(strcmp(bname, yuno_role)!=0) {
                print_error(
                    PEF_EXIT,
                    "ERROR YUNETA",
                    "yuno role '%s' and executable name '%s' MUST MATCH!",
                    yuno_role,
                    bname
                );
            }
            free(xx);
        }
    }

    /*------------------------------------------------*
     *  Print basic infom
     *------------------------------------------------*/
    if(arguments.print_role) {
        json_t *jn_tags = kw_get_dict_value(jn_yuno, "tags", 0, 0);
        json_t *jn_required_services = kw_get_dict_value(jn_yuno, "required_services", 0, 0);
        json_t *jn_public_services = kw_get_dict_value(jn_yuno, "public_services", 0, 0);
        json_t *jn_service_descriptor = kw_get_dict_value(jn_yuno, "service_descriptor", 0, 0);

        json_t *jn_basic_info = json_pack("{s:s, s:s, s:s, s:s, s:s, s:s, s:s}",
            "role", __yuno_role__,
            "name", __yuno_name__,
            "alias", __yuno_tag__,
            "version", __yuno_version__,
            "date", __app_datetime__,
            "description", __app_doc__,
            "yuneta_version", __yuneta_version__
        );
        if(jn_basic_info) {
            size_t flags = JSON_INDENT(4);
            if(jn_tags) {
                json_object_set(jn_basic_info, "tags", jn_tags);
            } else {
                json_object_set_new(jn_basic_info, "tags", json_array());
            }
            if(jn_required_services) {
                json_object_set(jn_basic_info, "required_services", jn_required_services);
            } else {
                json_object_set_new(jn_basic_info, "required_services", json_array());
            }
            if(jn_public_services) {
                json_object_set(jn_basic_info, "public_services", jn_public_services);
            } else {
                json_object_set_new(jn_basic_info, "public_services", json_array());
            }
            if(jn_service_descriptor) {
                json_object_set(jn_basic_info, "service_descriptor", jn_service_descriptor);
            } else {
                json_object_set_new(jn_basic_info, "service_descriptor", json_array());
            }

            json_dumpf(jn_basic_info, stdout, flags);
            printf("\n");
            JSON_DECREF(jn_basic_info);
        }
        exit(0);
    }

    /*------------------------------------------------*
     *  Init ginsfsm and yuneta
     *------------------------------------------------*/
    init_ginsfsm_library();
    json_t *jn_global = kw_get_dict(__jn_config__, "global", 0, 0);
    gobj_start_up(
        jn_global,
        __global_startup_persistent_attrs_fn__,
        __global_end_persistent_attrs_fn__,
        __global_load_persistent_attrs_fn__,
        __global_save_persistent_attrs_fn__,
        __global_remove_persistent_attrs_fn__,
        __global_list_persistent_attrs_fn__,
        __global_command_parser_fn__,
        __global_stats_parser_fn__,
        __global_authz_checker_fn__,
        __global_authenticate_parser_fn__
    );
    yuneta_register_c_core();
    if(register_yuno_and_more) {
        register_yuno_and_more();
    }

    /*------------------------------------------------*
     *          Run
     *------------------------------------------------*/
    if(__as_daemon__) {
        daemon_run(process, get_process_name(), work_dir, domain_dir, daemon_catch_signals);
    } else {
        daemon_catch_signals();
        if(__auto_kill_time__) {
            /* kill in x seconds, to debug exit in kdevelop */
            alarm(__auto_kill_time__);
        }
        process(get_process_name(), work_dir, domain_dir);
    }

    /*------------------------------------------------*
     *          Finish
     *------------------------------------------------*/
    end_ginsfsm_library();
    log_debug(0,
        "gobj",         "%s", __FILE__,
        "msgset",       "%s", MSGSET_START_STOP,
        "msg",          "%s", "Finished",
        "role",         "%s", __yuno_role__,
        "name",         "%s", __yuno_name__,
        "alias",        "%s", __yuno_tag__,
        NULL
    );
    JSON_DECREF(__jn_config__);
    gbmem_shutdown();
    register_yuneta_environment(0, 0, 0, 0, 0);
    log_debug_printf("", "<===== Yuno '%s^%s' stopped\n",
        __yuno_role__,
        __yuno_name__
    );
    end_ghelpers_library();

    /*
     *  Restore default memory in jansson
     */
    json_set_alloc_funcs(
        malloc,
        free
    );
    uv_replace_allocator(
        malloc,
        realloc,
        calloc,
        free
    );

    exit(gobj_get_exit_code());
}

/***************************************************************************
 *                      Process
 ***************************************************************************/
PRIVATE void process(const char *process_name, const char *work_dir, const char *domain_dir)
{
    log_debug(0,
        "msgset",       "%s", MSGSET_START_STOP,
        "msg",          "%s", "Starting yuno",
        "work_dir",     "%s", work_dir,
        "domain_dir",   "%s", domain_dir,
        "realm_id",     "%s", __realm_id__,
        "realm_owner",  "%s", __realm_owner__,
        "realm_role",   "%s", __realm_role__,
        "realm_name",   "%s", __realm_name__,
        "realm_env",    "%s", __realm_env__,
        "yuno_role",    "%s", __yuno_role__,
        "yuno_name",    "%s", __yuno_name__,
        "yuno_tag",   "%s", __yuno_tag__,
        "version",      "%s", __yuno_version__,
        "date",         "%s", __app_datetime__,
        "pid",          "%d", getpid(),
        "watcher_pid",  "%d", get_watcher_pid(),
        "description",  "%s", __app_doc__,
        NULL
    );

    /*------------------------------------------------*
     *          Create main process yuno
     *------------------------------------------------*/
    json_t *jn_yuno = kw_get_dict(
        __jn_config__,
        "yuno",
        0,
        0
    );
    json_incref(jn_yuno);
    hgobj gobj = __yuno_gobj__ = gobj_yuno_factory(
        __realm_id__,
        __realm_owner__,
        __realm_role__,
        __realm_name__,
        __realm_env__,
        __yuno_name__,
        __yuno_tag__,
        jn_yuno
    );
    if(!gobj) {
        log_error(0,
            "gobj",         "%s", __FILE__,
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_RUNTIME_ERROR,
            "msg",          "%s", "gobj_yuno_factory() FAILED",
            "role",         "%s", __yuno_role__,
            NULL
        );
        exit(0); // Exit with 0 to avoid that watcher restart yuno.
    }
    gobj_write_str_attr(gobj, "appName", __app_name__);
    gobj_write_str_attr(gobj, "appDesc", __app_doc__);
    gobj_write_str_attr(gobj, "appDate", __app_datetime__);
    gobj_write_str_attr(gobj, "work_dir", work_dir);
    gobj_write_str_attr(gobj, "domain_dir", domain_dir);
    gobj_write_str_attr(gobj, "yuno_version", __yuno_version__);

    /*------------------------------------------------*
     *          Create services
     *------------------------------------------------*/
    json_t *jn_services = kw_get_list(
        __jn_config__,
        "services",
        0,
        0
    );
    if(jn_services) {
        size_t index;
        json_t *jn_service_tree;
        json_array_foreach(jn_services, index, jn_service_tree) {
            if(!json_is_object(jn_service_tree)) {
                log_error(0,
                    "gobj",         "%s", __FILE__,
                    "function",     "%s", __FUNCTION__,
                    "msgset",       "%s", MSGSET_PARAMETER_ERROR,
                    "msg",          "%s", "service config MUST BE an json object",
                    "index",        "%d", index,
                    NULL
                );
                continue;
            }
            const char *service_name = kw_get_str(jn_service_tree, "name", 0, 0);
            if(empty_string(service_name)) {
                log_error(0,
                    "gobj",         "%s", __FILE__,
                    "function",     "%s", __FUNCTION__,
                    "msgset",       "%s", MSGSET_PARAMETER_ERROR,
                    "msg",          "%s", "service without name",
                    "index",        "%d", index,
                    NULL
                );
                continue;
            }
            json_incref(jn_service_tree);
            if(!gobj_service_factory(service_name, jn_service_tree)) {
                log_error(0,
                    "gobj",         "%s", __FILE__,
                    "function",     "%s", __FUNCTION__,
                    "msgset",       "%s", MSGSET_INTERNAL_ERROR,
                    "msg",          "%s", "gobj_service_factory() FAILED",
                    "service",      "%s", service_name,
                    NULL
                );
            }
        }
    }

    /*------------------------*
     *      Start main
     *------------------------*/
    gobj_start(gobj);

    /*---------------------------------*
     *      Auto services
     *---------------------------------*/
    gobj_autostart_services();
    gobj_autoplay_services();

    /*-----------------------------------*
     *      Run main event loop
     *-----------------------------------*/
    mt_run(gobj);       // Forever loop. Returning is because someone order to stop
    mt_clean(gobj);     // Before destroying check that uv handlers are closed.

    /*---------------------------*
     *      Destroy all
     *---------------------------*/
    gobj_shutdown();
    gobj_end();
}

/***************************************************************************
 *
 ***************************************************************************/
PUBLIC void set_assure_kill_time(int seconds)
{
    __assure_kill_time__ = seconds;
}

/***************************************************************************
 *
 ***************************************************************************/
PUBLIC BOOL running_as_daemon(void)
{
    return __as_daemon__;
}

/***************************************************************************
 *
 ***************************************************************************/
PUBLIC void set_auto_kill_time(int seconds)
{
    __auto_kill_time__ = seconds;
}

/***************************************************************************
 *
 ***************************************************************************/
PUBLIC void set_ordered_death(BOOL ordered_death)
{
    __ordered_death__ = ordered_death;
}

