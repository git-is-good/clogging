#include "logger.h"
#include "fatalerror.h"
#include "hashtable_logfile.h"
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>
#include <time.h>
#include <errno.h>

//TODO ADD MULTITHREAD SUPPORT

static logger_t root_logger;
static HashTable *file_dict;
const size_t init_nfile = 10;

static void _add_child(logger_t *lg, logger_t *child);
static void _remove_child(logger_t *lg, logger_t *child);

static logfile_withref_t*
_create_logfile_withref(FILE *fd, size_t ref_count)
{
    logfile_withref_t *res = (logfile_withref_t*)malloc(sizeof(logfile_withref_t));
    res->fd = fd;
    res->ref_count = ref_count;
    return res;
}

static void
_decrese_logfile_ref_count(const char *filename)
{
    logfile_withref_t *fwr = findValueByKey(file_dict, filename);
    assert( fwr && fwr->ref_count );
    if ( --fwr->ref_count == 0 ){
        fclose(fwr->fd);
        free(fwr);
        removeKey(file_dict, filename);
    }
}

static logger_filetype_t
_parse_logfile_type(const char *filename)
{
    if ( strcmp(filename, "#1") == 0 ){
        return logger_filetype_stdout;
    } else if ( strcmp(filename, "#2") == 0 ){
        return logger_filetype_stderr;
    } else {
        return logger_filetype_normal;
    }
}

static FILE*
_get_specialfd(logger_filetype_t filetype)
{
    switch (filetype){
        case logger_filetype_stdout:
            return stdout;
        case logger_filetype_stderr:
            return stderr;
        default:
            assert(0);
    }
}

void
logging_init(const char *filename, int is_trunc, logger_level_t default_level)
{
    file_dict = createHashTable(init_nfile);

    root_logger.name = strdup("(root)");
    root_logger.type = logger_type_root;

    /* useless */
    root_logger.ref_count = 1;

    root_logger.parent = NULL;
    root_logger.childs = NULL;
    root_logger.nchilds = 0;
    root_logger.len_childs = 0;

    root_logger.filetype = _parse_logfile_type(filename); 
    if ( root_logger.filetype == logger_filetype_normal ){
        const char *mode = is_trunc ? "w" : "a";
        FILE *fd = fopen(filename, mode);
        if ( !fd ) FATALERROR;
        root_logger.filename = realpath(filename, NULL);
        if ( !root_logger.filename ) FATALERROR;

        root_logger.fd = fd;
        logfile_withref_t *fwr = _create_logfile_withref(fd, 1);
        putKeyValue(file_dict, root_logger.filename, fwr);
    } else {
        root_logger.filename = strdup(filename);
        root_logger.fd = _get_specialfd(root_logger.filetype);
    }
    root_logger.level = default_level;
}

/* destroy all subtree except the node itself */
static void
_destroy_subtree(logger_t *lg)
{
    /* logger_name_t is always strduped */
    free(lg->name);

    for ( size_t i = 0; i < lg->nchilds; i++ ){
        _destroy_subtree(lg->childs[i]);
        free(lg->childs[i]);
    }
    if ( lg->len_childs ) free(lg->childs);

    if ( lg->filetype == logger_filetype_normal ) {
        _decrese_logfile_ref_count(lg->filename);
    }
    /* filename is always strduped */
    free(lg->filename);
}

void
logging_finalize()
{
    _destroy_subtree(&root_logger);
    assert( getHashTableSize(file_dict) == 0 );
    destroyHashTable(file_dict);
}

#define NAME_CHECK_FAILED               \
    do {                                \
        printf("name check failed\n");  \
        exit(-1);                       \
    } while (0)

static inline int
_isalpha_or_(int c){ return isalpha(c) || c == '_'; }

static inline int
_isalnum_or_(int c){ return isalnum(c) || c == '_'; }

static int
_is_valid_name(logger_name_t lgname)
{
    if ( !_isalpha_or_(*lgname) ) return 0;

    while ( _isalnum_or_(*++lgname) )
        ;

    while ( *lgname ) {
        if ( *lgname != '.' ) return 0;
        if ( !_isalpha_or_(*++lgname) ) return 0;
        while ( _isalnum_or_(*++lgname) )
            ;
    }
    return 1;
}

/* precondition: both names are valid */
/* 0 for not, 1 for real child, 2 for identical */
static int
_is_child_name(logger_name_t maybep, logger_name_t maybec)
{
    /* parent name is shorter */
    size_t plen = strlen(maybep);
    if ( strncmp(maybep, maybec, plen) != 0 ) return 0;

    return strlen(maybec) == plen ? 2 : 1;
}

logger_t*
_create_auto_logger(const char *name, logger_t *parent)
{
    logger_t *lg = (logger_t*) malloc(sizeof(logger_t));
    lg->name = strdup(name);
    lg->type = logger_type_auto;
    lg->ref_count = 1;

    lg->parent = parent;
    lg->childs = NULL;
    lg->len_childs = 0;
    lg->nchilds = 0;

    lg->filetype = parent->filetype;
    lg->filename = strdup(parent->filename);
    lg->fd = parent->fd;
    lg->level = parent->level;

    if ( lg->filetype == logger_filetype_normal ){
        logfile_withref_t *fwr = findValueByKey(file_dict, lg->filename);
        assert( fwr && fwr->ref_count );
        fwr->ref_count++;
    }
    return lg;
}

logger_t*
logging_get_logger(logger_name_t lgname)
{
    if ( !lgname ) return &root_logger;
    if ( !_is_valid_name(lgname) ) NAME_CHECK_FAILED;

    logger_t *wk = &root_logger;
    for ( ; ; ){
        size_t i;
        for ( i = 0; i < wk->nchilds; i++ ){
            int r = _is_child_name(wk->childs[i]->name, lgname);
            if ( !r ) {
                continue;

            /* real parent found */
            } else if ( r == 1 ) {
                break;

            /* identical */
            } else {
                assert( r == 2 );
                wk->childs[i]->ref_count++;
                return wk->childs[i];
            }
        }
        if ( i == wk->nchilds ) break;
        wk = wk->childs[i];
    }

    /* now wk is the direct real parent */
    logger_t *res = _create_auto_logger(lgname, wk);

    /* maybe it already has child */
    for ( size_t i = 0; i < wk->nchilds; i++ ){
        logger_t *sib = wk->childs[i];
        if ( _is_child_name(lgname, sib->name) ){
            sib->parent = res;
            _add_child(res, sib);
            _remove_child(wk, sib);
        }
    }
    _add_child(wk, res);
    return res;
}

static void
_add_child(logger_t *lg, logger_t *child)
{
    if ( lg->nchilds + 1 > lg->len_childs ){
        lg->len_childs = 2 * (lg->len_childs + 1);
        lg->childs = (logger_t**) realloc(lg->childs, sizeof(logger_t*) * lg->len_childs);
    }
    lg->childs[lg->nchilds++] = child;
}

static void
_remove_child(logger_t *lg, logger_t *child)
{
    size_t wk = 0;
    for ( ; wk < lg->nchilds; wk++ ){
        if ( lg->childs[wk] == child ) break;
    }
    for ( size_t i = wk; i < lg->nchilds; i++ ){
        lg->childs[i] = lg->childs[i+1];
    }
    lg->nchilds--;
}

void
logging_giveup_logger(logger_t *lg)
{
    if ( lg->type == logger_type_root ) return;
    if ( --lg->ref_count > 0 ) return;

    /* really destroy */
    _remove_child(lg->parent, lg);
    for ( size_t i = 0; i < lg->nchilds; i++ ){
        logger_t *child = lg->childs[i];
        child->parent = lg->parent;
        _add_child(lg->parent, child);
    }

    if ( lg->len_childs > 0 ) free(lg->childs);
    if ( lg->filetype == logger_filetype_normal ) _decrese_logfile_ref_count(lg->filename);
    free(lg->name);
    free(lg->filename);
    free(lg);
}

static void
_inform_subtree_level(logger_t *lg)
{
    for ( size_t i = 0; i < lg->nchilds; i++ ){
        logger_t *child = lg->childs[i];
        if ( child->type == logger_type_self ) continue;
        child->level = lg->level;
        _inform_subtree_level(child);
    }
}

static void
_inform_subtree_output(logger_t *lg, logfile_withref_t *fwr, logfile_withref_t *oldfwr)
{
    for ( size_t i = 0; i < lg->nchilds; i++ ){
        logger_t *child = lg->childs[i];
        if ( child->type == logger_type_self ) continue;

        child->filetype = lg->filetype;
        free(child->filename);
        child->filename = strdup(lg->filename);
        child->fd       = lg->fd;

        if ( fwr ) fwr->ref_count++;
        if ( oldfwr && --oldfwr->ref_count == 0 ){
            fclose(oldfwr->fd);
            free(oldfwr);
            removeKey(file_dict, lg->filename);
            return;
        }
        _inform_subtree_output(child, fwr, oldfwr);
    }
}

void
logging_set_level(logger_t *lg, logger_level_t lglevel)
{
    if ( lg->type != logger_type_root ) {
        lg->type = logger_type_self;
    }
    if ( lg->level == lglevel ) return;

    lg->level = lglevel;
    _inform_subtree_level(lg);
    return;
}

void
logging_set_output(logger_t *lg, const char *filename, int is_trunc) 
{
    if ( lg->type != logger_type_root ){
        lg->type = logger_type_self;
    }
    logger_filetype_t tp = _parse_logfile_type(filename);
    logfile_withref_t *fwr;
    char *realname;

    if ( tp != logger_filetype_normal ){
        realname = strdup(filename);
        fwr = NULL;
        lg->fd = _get_specialfd(tp);
    } else {
        /* normal logger_filetype */
        realname = realpath(filename, NULL);
        if ( !realname && errno != ENOENT ) FATALERROR;
    
        if ( !realname || !(fwr = findValueByKey(file_dict, realname)) ){
            /* open new file */
            const char *mode = is_trunc ? "w" : "a";
            lg->fd = fopen(filename, mode);
            if ( !lg->fd ) FATALERROR;
            if ( !realname ) realname = realpath(filename, NULL);
            if ( !realname ) FATALERROR;

            fwr = _create_logfile_withref(lg->fd, 0);
            putKeyValue(file_dict, realname, fwr);
        } else { 
            lg->fd = fwr->fd;
        }
        fwr->ref_count++;
    }

    logfile_withref_t *oldfwr;
    if ( lg->filetype != logger_filetype_normal ){
        oldfwr = NULL;
    } else {
        oldfwr = findValueByKey(file_dict, lg->filename);
        if ( --oldfwr->ref_count == 0 ){
            fclose(oldfwr->fd);
            free(oldfwr);
            removeKey(file_dict, lg->filename);
        }
    }
    lg->filetype = tp;
    /* filename is always strduped, even if it is "#1" or "#2 */
    free(lg->filename);
    lg->filename = realname;
    if ( !oldfwr || oldfwr->ref_count > 0 ) _inform_subtree_output(lg, fwr, oldfwr);
    return;
}

const char *logger_level_msg[] = {
    [logger_level_debug]    = "DEBUG",
    [logger_level_info]     = "INFO",
    [logger_level_warning]  = "WARNING",
    [logger_level_error]    = "ERROR",
};

void
logging_vlog(logger_t *lg, logging_event_level_t elevel, const char *msg, va_list va)
{
    if ( elevel >= lg->level ){
        char buf[128];
        time_t t = time(NULL);
        strftime(buf, 128, "%F:%T", localtime(&t));
        fprintf(lg->fd, "%s:%s:%s: ", buf, lg->name, logger_level_msg[elevel]);
        vfprintf(lg->fd, msg, va);
        fprintf(lg->fd, "\n");
        fflush(lg->fd);
    }
}

void
logging_log(logger_t *lg, logging_event_level_t elevel, const char *msg, ...)
{
    va_list va;
    va_start(va, msg);
    logging_vlog(lg, elevel, msg, va);
    va_end(va);
}

#ifdef DEBUG
HashTable*
logger_get_file_dict()
{
    return file_dict;
}

void 
logger_inplace_test()
{
    assert(!_is_valid_name(""));
    assert(_is_valid_name("_"));
    assert(_is_valid_name("_n23"));
    assert(!_is_valid_name("_n23.12k"));
    assert(_is_valid_name("m20._n23"));
    assert(_is_valid_name("m34.a43.bb"));
    assert(!_is_valid_name("m34.a43.bb."));

    assert(_is_child_name("abc.def", "abc.def.ghi") == 1);
    assert(_is_child_name("abc.def.ghi", "abc.def") == 0);
    assert(_is_child_name("abc.def.ghi", "abc.def.ghi") == 2);
}
#endif /* DEBUG defined */

