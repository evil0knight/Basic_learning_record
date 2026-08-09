#include "Debug.h"
#include "elog.h"
#include "stdio.h"
void app_elog_init(void){
    elog_init();
    // elog_set_fmt(level, fmt_flags)
    // level: ELOG_LVL_ASSERT > ERROR > WARN > INFO > DEBUG > VERBOSE
    // fmt_flags 按位或:
    //   ELOG_FMT_TIME  — 时间
    //   ELOG_FMT_LVL   — 日志级别
    //   ELOG_FMT_TAG   — 标签
    //   ELOG_FMT_DIR   — 文件路径
    //   ELOG_FMT_LINE  — 行号
    //   ELOG_FMT_FUNC  — 函数名
    elog_set_fmt(ELOG_LVL_ASSERT, ELOG_FMT_TIME | ELOG_FMT_LVL | ELOG_FMT_TAG | ELOG_FMT_DIR | ELOG_FMT_LINE | ELOG_FMT_FUNC); 
    elog_set_fmt(ELOG_LVL_DEBUG, ELOG_FMT_TIME | ELOG_FMT_LVL | ELOG_FMT_TAG | ELOG_FMT_DIR | ELOG_FMT_LINE | ELOG_FMT_FUNC); 
    elog_set_fmt(ELOG_LVL_ERROR, ELOG_FMT_TIME | ELOG_FMT_LVL | ELOG_FMT_TAG | ELOG_FMT_DIR | ELOG_FMT_LINE | ELOG_FMT_FUNC); 
    elog_set_fmt(ELOG_LVL_WARN,  ELOG_FMT_TIME | ELOG_FMT_LVL | ELOG_FMT_TAG | ELOG_FMT_DIR | ELOG_FMT_LINE | ELOG_FMT_FUNC); 
    elog_set_fmt(ELOG_LVL_INFO,  ELOG_FMT_TIME | ELOG_FMT_LVL | ELOG_FMT_TAG | ELOG_FMT_DIR | ELOG_FMT_LINE | ELOG_FMT_FUNC); 

    elog_start();
}