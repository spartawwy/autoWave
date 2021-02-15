#include "kline_wall.h"

#include <cassert>
#include <tuple>

#include <qdebug.h>
#include <qdatetime.h>

#include <TLib/core/tsystem_utility_functions.h>

#include "mainwindow.h"
#include "stkfo_common.h"
#include "futures_forecast_app.h"
#include "exchange_calendar.h"

using namespace TSystem;

void KLineWall::ResetTypePeriodTrain(TypePeriod  type, int start_date, int end_date)
{
    Reset_Stock_Train(stock_code_.c_str(), type, is_index_, nmarket_, start_date, end_date);
}

T_HisDataItemContainer* KLineWall::AppendDataForTrain(int start_date, int end_date)
{
    assert( start_date > MIN_TRADE_DATE && start_date < MAX_TRADE_DATE );
    assert( end_date > MIN_TRADE_DATE && end_date < MAX_TRADE_DATE );
    assert(start_date <= end_date);

    int cur_day = QDateTime::currentDateTime().toString("yyyyMMdd").toInt(); //default 
    if( end_date > cur_day ) end_date = cur_day;

    auto p_container = app_->stock_data_man().AppendStockData(ToPeriodType(k_type_), nmarket_, stock_code_, start_date, end_date, is_index_); 
    return p_container;
}

T_StockHisDataItem* KLineWall::SetTrainByDateTime(int date, int hhmm)
{
    T_StockHisDataItem* ret_item = nullptr;
    int target_r_end_index = FindKRendIndexInHighPeriodContain(k_type_, *p_hisdata_container_, *app_->exchange_calendar(), date, hhmm);
    if( target_r_end_index > -1 )
    {
        k_rend_index_ = target_r_end_index; 
        k_rend_index_for_train(target_r_end_index);
        ret_item = std::addressof((*(p_hisdata_container_->rbegin() + target_r_end_index))->stk_item);
    } 
    return ret_item; 
}

T_StockHisDataItem* KLineWall::SetTrainByRendIndex(int rend_index)
{
    assert(rend_index > -1);
    assert(p_hisdata_container_->size() > rend_index);

    T_StockHisDataItem* ret_item = nullptr;
    const int old_rend_index = k_rend_index_;
    k_rend_index_ = rend_index; 
    k_rend_index_for_train(rend_index); 
    ret_item = std::addressof((*(p_hisdata_container_->rbegin() + k_rend_index_for_train_))->stk_item);

    k_cur_train_date_ = (*(p_hisdata_container_->rbegin() + k_rend_index_for_train_))->stk_item.date;
    k_cur_train_hhmm_ = (*(p_hisdata_container_->rbegin() + k_rend_index_for_train_))->stk_item.hhmmss;

    if( old_rend_index != k_rend_index_ )
    {
        UpdateKwallMinMaxPrice();
        UpdatePosDatas();
        update();
    }
    return ret_item;
}

// ret is point to target item or nullptr
T_StockHisDataItem* KLineWall::SetTrainStartDateTime(TypePeriod tp_period, int date, int hhmm)
{
    T_StockHisDataItem* ret_item = nullptr;

    const int old_rend_index = k_rend_index_;
    const int old_k_num = k_num_;
    //int target_r_end_index = FindKRendIndex(p_hisdata_container_, date, hhmm);
    int target_r_end_index = FindKRendIndexInHighPeriodContain(tp_period, *p_hisdata_container_, *app_->exchange_calendar(), date, hhmm);
    if( target_r_end_index > -1 )
    {
        k_rend_index_ = target_r_end_index; 
        k_rend_index_for_train(target_r_end_index);
        ret_item = std::addressof((*(p_hisdata_container_->rbegin() + k_rend_index_for_train_))->stk_item);

    }else
    { 
        //QDate qdate_obj(date/10000, (date%10000)/100, date%100);
        int start_date = 0;
        int date_span = 1;
        if( tp_period >= TypePeriod::PERIOD_DAY ) 
            date_span = 4 * 30; 
        else if( tp_period == TypePeriod::PERIOD_HOUR ) 
            date_span = 10; 
        else if( tp_period >= TypePeriod::PERIOD_15M && tp_period <= TypePeriod::PERIOD_30M )
            date_span = 5;
        else if( tp_period <= TypePeriod::PERIOD_5M )
            date_span = 3;

        start_date = app_->exchange_calendar()->PreTradeDate(date, date_span);
        if( start_date < 1980 )
            start_date = date;
        auto p_container = AppendPreData(start_date, hhmm); 
        if( !p_container->empty() )
        {
            ret_item = SetTrainByDateTime(date, hhmm);
            assert(ret_item);
            app_->stock_data_man().TraverseSetFeatureData(stock_code_, ToPeriodType(k_type_), is_index_, k_rend_index_for_train_);
            //HandleAutoForcast();
        }
    }
    if( !p_hisdata_container_->empty() )
    {
        k_cur_train_date_ = (*(p_hisdata_container_->rbegin() + k_rend_index_for_train_))->stk_item.date;
        k_cur_train_hhmm_ = (*(p_hisdata_container_->rbegin() + k_rend_index_for_train_))->stk_item.hhmmss;
    }
    if( old_rend_index != k_rend_index_ || old_k_num != k_num_ )
    {
        UpdateKwallMinMaxPrice();
        UpdatePosDatas();
        update();
    }
    if( ret_item ) train_start_date_ = ret_item->date;
    else train_start_date_ = date;
    return ret_item;
}

// return start item
T_StockHisDataItem* KLineWall::SetTrainStartEnd(TypePeriod tp_period, int start_date, int star_hhmm, int end_date, int end_hhmm)
{
    T_StockHisDataItem* ret_item = nullptr;

    const int old_rend_index = k_rend_index_;
    const int old_k_num = k_num_;
    //int target_r_end_index = FindKRendIndex(p_hisdata_container_, date, hhmm);
    int target_r_end_index = FindKRendIndexInHighPeriodContain(tp_period, *p_hisdata_container_, *app_->exchange_calendar(), start_date, star_hhmm);
    int end_date_r_index = FindKRendIndexInHighPeriodContain(tp_period, *p_hisdata_container_, *app_->exchange_calendar(), end_date, end_hhmm);
    if( target_r_end_index > -1 && end_date_r_index > -1 )
    {
        k_rend_index_ = target_r_end_index; 
        k_rend_index_for_train(target_r_end_index);
        ret_item = std::addressof((*(p_hisdata_container_->rbegin() + k_rend_index_for_train_))->stk_item);

    }else
    { 
        //QDate qdate_obj(date/10000, (date%10000)/100, date%100);
        int date_span = 1;
        if( tp_period >= TypePeriod::PERIOD_DAY ) 
            date_span = 4 * 30; 
        else if( tp_period == TypePeriod::PERIOD_HOUR ) 
            date_span = 10; 
        else if( tp_period >= TypePeriod::PERIOD_15M && tp_period <= TypePeriod::PERIOD_30M )
            date_span = 5;
        else if( tp_period <= TypePeriod::PERIOD_5M )
            date_span = 3;

        int tart_start_date = start_date;
        int temp_val = app_->exchange_calendar()->PreTradeDate(start_date, date_span);
        if( temp_val > 1980 && temp_val < start_date )
            tart_start_date = temp_val;
        auto p_container = AppendDataForTrain(tart_start_date, end_date); 
        if( !p_container->empty() )
        {
            ret_item = SetTrainByDateTime(start_date, star_hhmm);
            assert(ret_item);
            app_->stock_data_man().TraverseSetFeatureData(stock_code_, ToPeriodType(k_type_), is_index_, k_rend_index_for_train_);
            //HandleAutoForcast();
        }
    }
    if( !p_hisdata_container_->empty() )
    {
        k_cur_train_date_ = (*(p_hisdata_container_->rbegin() + k_rend_index_for_train_))->stk_item.date;
        k_cur_train_hhmm_ = (*(p_hisdata_container_->rbegin() + k_rend_index_for_train_))->stk_item.hhmmss;
    }
    if( old_rend_index != k_rend_index_ || old_k_num != k_num_ )
    {
        UpdateKwallMinMaxPrice();
        UpdatePosDatas();
        update();
    }
    if( ret_item ) train_start_date_ = ret_item->date;
    else train_start_date_ = start_date;
    train_end_date_ = end_date;
     
    return ret_item;
}

//ps: may cause append data
T_StockHisDataItem* KLineWall::SetTrainEndDateTime(TypePeriod tp_period, int date, int hhmm)
{
    T_StockHisDataItem* ret_item = nullptr;
    const int old_size = p_hisdata_container_->size();
    int target_r_end_index = FindKRendIndexInHighPeriodContain(tp_period, *p_hisdata_container_, *app_->exchange_calendar(), date, hhmm);
    if( target_r_end_index > -1 )
    { 
        ret_item = std::addressof((*(p_hisdata_container_->rbegin() + target_r_end_index))->stk_item);
    }else
    {  
        auto p_container = AppendData(date, hhmm); 
        if( p_container->size() > old_size )
        {
        target_r_end_index = FindKRendIndexInHighPeriodContain(k_type_, *p_hisdata_container_, *app_->exchange_calendar(), k_cur_train_date_, k_cur_train_hhmm_);
        k_rend_index_for_train(target_r_end_index);
        k_rend_index_ = target_r_end_index;
        app_->stock_data_man().TraverseSetFeatureData(stock_code_, ToPeriodType(k_type_), is_index_, k_rend_index_for_train_);
        //HandleAutoForcast();
        }
    }
    if( ret_item ) train_end_date_ = ret_item->date;
    else train_end_date_ = date;
    return ret_item;
}
 
T_StockHisDataItem KLineWall::Train_NextStep()
{ 
    T_StockHisDataItem  ret_item;
    if( p_hisdata_container_->empty() || k_rend_index_for_train_ <= 0 )
        return ret_item;// std::make_tuple(0, 0); 
    const int old_k_rend_index = k_rend_index_; 
    const int old_date = ( *(p_hisdata_container_->rbegin() + k_rend_index_for_train_) )->stk_item.date;
    const int old_hhmm = ( *(p_hisdata_container_->rbegin() + k_rend_index_for_train_) )->stk_item.hhmmss;
     
    if( k_type_ == DEFAULT_ORI_STEP_TYPE_PERIOD )
    {
        //return update_next_step();
        k_rend_index_for_train(k_rend_index_for_train_ - 1 > -1 ? k_rend_index_for_train_ - 1 : 0);
        T_HisDataItemContainer::reference target_item = *(p_hisdata_container_->rbegin() + k_rend_index_for_train_);
        k_cur_train_date_ = target_item->stk_item.date;
        k_cur_train_hhmm_ = target_item->stk_item.hhmmss; 
        k_rend_index_ = k_rend_index_for_train_;
        if( old_k_rend_index != k_rend_index_ )
        {
            T_HisDataItemContainer::reference pre_item = *(p_hisdata_container_->rbegin() + old_k_rend_index);
            if( target_item->stk_item.low_price < pre_item->stk_item.low_price  
                || target_item->stk_item.high_price > pre_item->stk_item.high_price)
            {
                app_->stock_data_man().TraverseSetFeatureData(stock_code_, ToPeriodType(k_type_), is_index_,  k_rend_index_for_train_, DEFAULT_TRAVERSE_LEFT_K_NUM);
            }
            if( wall_index_ != (int)WallIndex::ORISTEP )
            {
                const unsigned int left_index = p_hisdata_container_->size() - k_rend_index_for_train_ - 1;
                app_->stock_data_man().ReCaculateZhibiaoItem(*p_hisdata_container_, left_index);
                UpdateKwallMinMaxPrice();
                UpdatePosDatas();
                update();
            }
        }
        return target_item->stk_item;
    }
    
    return ret_item; // std::make_tuple(k_cur_train_date_, k_cur_train_hhmm_);
}

void KLineWall::Train_NextStep(T_StockHisDataItem & input_item)
{  
    if( p_hisdata_container_->empty() || k_rend_index_for_train_ <= 0 )
        return; 
     
    if( k_type_ == DEFAULT_ORI_STEP_TYPE_PERIOD )
    {
        k_rend_index_for_train(k_rend_index_for_train_ - 1 > -1 ? k_rend_index_for_train_ - 1 : 0);
        T_HisDataItemContainer::reference target_item = *(p_hisdata_container_->rbegin() + k_rend_index_for_train_);

        k_cur_train_date_ = target_item->stk_item.date;  
        k_cur_train_hhmm_ = target_item->stk_item.hhmmss;  
        target_item->stk_item.open_price = input_item.open_price;
        target_item->stk_item.close_price = input_item.close_price;
        target_item->stk_item.high_price = input_item.high_price;
        target_item->stk_item.low_price = input_item.low_price;
        target_item->stk_item.vol = input_item.vol;

        k_rend_index_ = k_rend_index_for_train_;
        ///HandleAutoForcast();
    }else 
    {
        T_HisDataItemContainer::reference cur_item = *(p_hisdata_container_->rbegin() + k_rend_index_for_train_);
        bool is_to_move = false;
        switch( k_type_ )
        {
        case TypePeriod::PERIOD_5M:  
        /*case TypePeriod::PERIOD_15M: 
        case TypePeriod::PERIOD_30M:  
        case TypePeriod::PERIOD_HOUR:  
        case TypePeriod::PERIOD_DAY:  */
            {
                int old_k_rend_index_for_train = k_rend_index_for_train_;
                int target_r_end_index = FindKRendIndexInHighContain_FromRStart2Right(k_type_, *p_hisdata_container_, *app_->exchange_calendar(), input_item.date, input_item.hhmmss, k_rend_index_for_train_);
                if( target_r_end_index > -1 )
                { 
                    if( old_k_rend_index_for_train != target_r_end_index )
                    {
                        k_rend_index_for_train(target_r_end_index);
                        k_rend_index_ = target_r_end_index;
                        T_HisDataItemContainer::reference target_item = *(p_hisdata_container_->rbegin() + k_rend_index_for_train_);

                        k_cur_train_date_ = target_item->stk_item.date; //ndchk
                        k_cur_train_hhmm_ = target_item->stk_item.hhmmss; //ndchk
                        target_item->stk_item.open_price = input_item.open_price;
                        target_item->stk_item.close_price = input_item.close_price;
                        target_item->stk_item.high_price = input_item.high_price;
                        target_item->stk_item.low_price = input_item.low_price;
                        target_item->stk_item.vol = input_item.vol;
                        app_->stock_data_man().TraverseSetFeatureData(stock_code_, ToPeriodType(k_type_), is_index_,  k_rend_index_for_train_, DEFAULT_TRAVERSE_LEFT_K_NUM);
                        //HandleAutoForcast();
                    }else
                    {
                        // update cur item
                        cur_item->stk_item.close_price = input_item.close_price;
                        if( input_item.high_price > cur_item->stk_item.high_price ) 
                        {
                            cur_item->stk_item.high_price = input_item.high_price;
                            app_->stock_data_man().TraverseSetFeatureData(stock_code_, ToPeriodType(k_type_), is_index_,  k_rend_index_for_train_, DEFAULT_TRAVERSE_LEFT_K_NUM);
                            //HandleAutoForcast();
                        }
                        if( input_item.low_price < cur_item->stk_item.low_price )
                        {
                            cur_item->stk_item.low_price = input_item.low_price;
                            app_->stock_data_man().TraverseSetFeatureData(stock_code_, ToPeriodType(k_type_), is_index_,  k_rend_index_for_train_, DEFAULT_TRAVERSE_LEFT_K_NUM);
                            //HandleAutoForcast();
                        }
                        cur_item->stk_item.vol += input_item.vol;
                    }
                }
                 
            }
        default: break;
        }
         
    } 
    if( wall_index_ != (int)WallIndex::ORISTEP )
    {
        const unsigned int left_index = p_hisdata_container_->size() - k_rend_index_for_train_ - 1;
        app_->stock_data_man().ReCaculateZhibiaoItem(*p_hisdata_container_, left_index);
        UpdateKwallMinMaxPrice();
        UpdatePosDatas();
        update();
    }
}

const T_StockHisDataItem & KLineWall::CurTrainStockDataItem()
{
    static T_StockHisDataItem no_use_item;

    if( p_hisdata_container_->size() <= 0 || p_hisdata_container_->size() - 1 < k_rend_index_for_train_ 
        || k_rend_index_for_train_ < 0 )
        return no_use_item;

    return p_hisdata_container_->at(p_hisdata_container_->size() - 1 - k_rend_index_for_train_)->stk_item;
}

 
 