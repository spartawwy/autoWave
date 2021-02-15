#ifndef TREND_LINE_H_DSFDSKJK_
#define TREND_LINE_H_DSFDSKJK_

#include "stock_data_man.h"
#if 0 
void  CreateTrendLine()
{ 
    T_HisDataItemContainer &k_datas = app_->stock_data_man().GetHisDataContainer(k_type_, stock_code_);

    const int T = 120;
    int cst_cur_index = k_datas.size() - 1;

    int right_end_index = cst_cur_index;
    int left_end_index = MAX_VAL(cst_cur_index - T, 0);

    // find previous 2 trading period windows 's start index
    int temp_index = PreTradeDaysOrNightsStartIndex(*app_->exchange_calendar(), k_type_, k_datas, right_end_index, 3);
    if( temp_index > -1 )
        left_end_index = temp_index;
}
#endif
#endif