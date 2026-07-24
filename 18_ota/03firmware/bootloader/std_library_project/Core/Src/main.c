#include <stdio.h>
#include "elog.h"

void Easylogger_Configuration(void)
{
	elog_init();
	elog_set_fmt(ELOG_LVL_ASSERT, ELOG_FMT_LVL);
	elog_set_fmt(ELOG_LVL_ERROR, ELOG_FMT_ALL);
	elog_set_fmt(ELOG_LVL_WARN, ELOG_FMT_LVL | ELOG_FMT_P_INFO);
	elog_set_fmt(ELOG_LVL_INFO, ELOG_FMT_ALL);
	elog_set_fmt(ELOG_LVL_DEBUG, ELOG_FMT_ALL);
	elog_set_fmt(ELOG_LVL_VERBOSE, ELOG_FMT_ALL);
	
	elog_start();
}

int main(void)
{
	
	
	
}

