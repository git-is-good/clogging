#include "hashtable_logfile.h"
#include "logger.h"
#include <assert.h>
#include <string.h>
#include <stdlib.h>

#define TESTECHO printf("%s() starts...\n", __func__);

void test_basic(){
    TESTECHO;
    logging_init("hello.log", 0, logger_level_info);

    logger_t *lg = logging_get_logger(NULL);
//    logging_log(lg, logger_level_debug, "debug is open");
//    logging_log(lg, logger_level_info, "info is going is open");
//    logging_log(lg, logger_level_warning, "oooops.. warning in");
//    logging_log(lg, logger_level_error, "ERROR");

    logger_t *nlg = logging_get_logger("phone.pear");
    assert( lg->nchilds == 1 );
    assert( strcmp(nlg->name, "phone.pear") == 0 );

    logger_t *nnlg = logging_get_logger("phone.pear.piano");
    assert( nlg->parent == lg );
    assert( nnlg->parent == nlg );
    assert( nlg->nchilds == 1 );
    assert( lg->nchilds == 1 );

    logger_t *mlg = logging_get_logger("phone");
    assert( nlg->parent == mlg );
    assert( lg->nchilds == 1 );
    assert( lg->childs[0] == mlg );
    assert( mlg->nchilds == 1 );
    assert( mlg->childs[0] == nlg );

    logger_t *fakelg = logging_get_logger("phone.pear");
    assert( fakelg == nlg );
    assert( fakelg->ref_count == 2 );
    logging_giveup_logger(fakelg);
    assert( fakelg->ref_count == 1 );

    logger_t *klg = logging_get_logger("phone.arch");
    logging_set_level(nlg, logger_level_warning);
    assert( nlg->type == logger_type_self );
    assert( nnlg->type == logger_type_auto );
    assert( nlg->level == logger_level_warning );
    assert( nnlg->level == logger_level_warning );
    assert( klg->level == logger_level_info );

    logging_set_level(mlg, logger_level_error);
    assert( nlg->level == logger_level_warning );
    assert( nnlg->level == logger_level_warning );
    assert( mlg->level == logger_level_error );
    assert( klg->level == logger_level_error );

//    logging_log(nlg, logger_level_warning, "phone oops");
//    logging_log(nlg, logger_level_info, "someone is dancing");
//    logging_log(nlg, logger_level_debug, "eone is dancing");

//    for ( size_t i = 0; i < 10000000; i++ ){
//        logger_t *tmp1 = logging_get_logger("phone.arch");
//        logger_t *tmp2 = logging_get_logger("phone.arch.second");
//        logging_giveup_logger(tmp1);
//        logging_giveup_logger(tmp2);
//    }
    

    logging_finalize();
}

#ifdef DEBUG
void logger_inplace_test();
HashTable *logger_get_file_dict();

void test_output(){
    TESTECHO;
    logging_init("hello.log", 0, logger_level_info);
    HashTable *file_dict = logger_get_file_dict();
    assert( logging_get_logger(NULL)->filetype == logger_filetype_normal );

    logfile_withref_t *fwr1 = findValueByKey(file_dict, "hello.log");
    assert( fwr1->ref_count == 1 );

    logger_t *a = logging_get_logger("hel");
    assert( a->filetype == logger_filetype_normal );
    assert( fwr1->ref_count == 2 );
    logging_set_output(a, "hel.log", 0);
    assert( a->type == logger_type_self );
    assert( fwr1->ref_count == 1 );

    logfile_withref_t *fwr2 = findValueByKey(file_dict, "hel.log");
    assert( fwr2->ref_count == 1 );

    logger_t *b = logging_get_logger("hel.wor");
    assert( a->filetype == logger_filetype_normal );
    assert( fwr2->ref_count == 2 );

    logging_giveup_logger(b);
    assert( fwr2->ref_count == 1 );
    logging_giveup_logger(a);
    assert( findValueByKey(file_dict, "hel.log") == NULL );
    
    a = logging_get_logger("goo");
    assert( fwr1->ref_count == 2 );
    logging_set_output(a, "#1", 0);
    assert( fwr1->ref_count == 1 );
    assert( a->filetype == logger_filetype_stdout );
    logging_log(a, logger_level_info, "hello from a");
    logging_log(a, logger_level_debug, "hello from a");
    logging_debug(a, "from a macro");
    b = logging_get_logger("goo.hoo");
    assert( b->filetype == logger_filetype_stdout );
    assert( logging_get_logger(NULL)->filetype == logger_filetype_normal );
    

    logging_finalize();
}
#endif

#define mylog_error(lg, fmt, ...) logging_error(lg, "%s:%d:%s:"fmt, __FILE__, __LINE__, __func__, ##__VA_ARGS__)
void demousage(){
    logging_init("#1", 0, logger_level_debug);

    logger_t *lg = logging_get_logger(NULL);
    logging_debug(lg, "demo running");
    logging_warning(lg, "something weild");

    logger_t *faclg = logging_get_logger("facsubsystem");
    logging_error(faclg, "%s:%d:%s:""unexpected power off", __FILE__, __LINE__, __func__);
    mylog_error(faclg, "what is wrong");
    logging_finalize();
}

int main(){
#ifdef DEBUG
    logger_inplace_test();
    for ( size_t i = 0; i < 100000000; i++ ) test_output();
    test_output();
#endif
//    for ( size_t i = 0; i < 10000000; i++ ) test_basic();
    test_basic();
    demousage();
}


