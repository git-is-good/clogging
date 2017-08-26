#ifndef _LOGGER_H_
#define _LOGGER_H_

#include <stdio.h>
#include <stddef.h>
#include <stdarg.h>

/* a hierachical logger */

/* logger naming convention:
 * nom[.nom]+               
 * where nom is [_a-zA-Z][_a-zA-Z0-9]* */

#define logging_debug(lg, ...)   logging_log(lg, logger_level_debug, ##__VA_ARGS__)
#define logging_info(lg, ...)    logging_log(lg, logger_level_info, ##__VA_ARGS__)
#define logging_warning(lg, ...) logging_log(lg, logger_level_warning, ##__VA_ARGS__)
#define logging_error(lg, ...)   logging_log(lg, logger_level_error, ##__VA_ARGS__)

typedef const char *logger_name_t;

typedef enum {
    logger_type_root,

    /* an auto logger inherits attributs from its parent */
    /* attributes are synchronized if parent or parent's attributes change */
    logger_type_auto,

    /* a self logger determines attributes by itself */
    /* its parent doesn't affect it */
    logger_type_self,
} logger_type_t;

typedef enum {
    logger_filetype_normal,
    logger_filetype_stdout,
    logger_filetype_stderr,
} logger_filetype_t;

typedef enum {
    logger_level_debug,
    logger_level_info,
    logger_level_warning,
    logger_level_error,
} logger_level_t;

typedef logger_level_t logging_event_level_t;

typedef struct logger_s {
    logger_name_t       name;
    logger_type_t       type;
    size_t              ref_count;

    struct logger_s     *parent;
    /* we assume the number of children is small */
    /* otherwise replace it by a hashtable */
    struct logger_s     **childs;
    /* length of the list malloced */
    size_t              len_childs;
    size_t              nchilds;
    
    logger_filetype_t   filetype;
    const char          *filename;
    FILE                *fd;
    logger_level_t      level;
} logger_t;

/* only logging_finalize will destroy the root_logger */
void logging_init(const char *filename, int is_trunc, logger_level_t default_level);
void logging_finalize();

/* get a logger by name */
/* 1, if lgname == NULL, return the root_logger */
/* 2, if this name already exists, return the old one, increment the ref count by 1 */
/* 3, if this name does not exist, return a new auto_logger */
logger_t *logging_get_logger(logger_name_t lgname);

/* decrement lg 's ref count by 1 */
/* if ref becomes 0, destroy it, and all its children will point to its parent */
void logging_giveup_logger(logger_t *lg);

/* after setting, a logger become a auto_logger */
void logging_set_level(logger_t *lg, logger_level_t lglevel);
/* mode will be ignored if the filename already open */
/* filename: "#1" for stdout, "#2" for stderr */
void logging_set_output(logger_t *lg, const char *filename, int is_trunc);

/* for logging */
void logging_vlog(logger_t *lg, logging_event_level_t elevel, const char *msg, va_list va);
void logging_log(logger_t *lg, logging_event_level_t elevel, const char *msg, ...);



#endif /* _LOGGER_H_ */
