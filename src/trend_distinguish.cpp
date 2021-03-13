#include "trend_distinguish.h"

#include <Tlib/core/tsystem_core_common.h>

#include "exchange_calendar.h"
#include "futures_forecast_app.h"
#include "position_account.h"

#define TAG_DISTINGUISH_TREND_LOG  "DistinguishTrend"

static const double cst_bias_threshold = 16.1;

using namespace TSystem;
TrendDistinguish::TrendDistinguish(FuturesForecastApp &app, AccountInfo &account_info):Strategy(app, account_info)
    , sub_cur_trend_(StrategyTrendType::UNKOWN)
    , main_cur_trend_(StrategyTrendType::UNKOWN)
    , is_trend_judged_(false)  
    , pre_judge_trend_stamp_(0)   
    , sub_bias_(0.0)
    , main_bias_(0.0)
    //, date_openlong_danger_tag_(0)
    , danger_type_open_long_(OpenPosDangerType::SAFE)
{

}

void TrendDistinguish::Init()
{
#ifdef MAKE_SUB_WALL
    if( sub_hisdatas_->size() < cst_ave_line_0_t )
        return;
    auto current_date = sub_hisdatas_->back()->stk_item.date;
    for( auto i = sub_hisdatas_->size() - 1; i > 0; --i) // towards left
    {
        if( app_.exchange_calendar()->DateTradingSpan(sub_hisdatas_->at(i)->stk_item.date, current_date) > 5 )
            break;
        T_StockHisDataItem &item = sub_hisdatas_->at(i)->stk_item;
        double val_line_0 = app_.stock_data_man().GetAveLineZhibiao(ToPeriodType(sub_k_type_), code_, cst_ave_line_0_t, i); 
        double val_line_1 = app_.stock_data_man().GetAveLineZhibiao(ToPeriodType(sub_k_type_), code_, cst_ave_line_1_t, i); 

        auto sub_lines_bias = val_line_0 - val_line_1;
        auto sub_bias = sub_hisdatas_->at(i)->stk_item.close_price - val_line_0;
        if( sub_hisdatas_->size() > 3 )
        {
            auto pre3_data_index = sub_hisdatas_->size() - 1 - 3 ;
            double val_line_0_pre3 = app_.stock_data_man().GetAveLineZhibiao(ToPeriodType(sub_k_type_), code_, cst_ave_line_0_t, pre3_data_index); 
            if( sub_lines_bias < 0 && val_line_0 > val_line_0_pre3 )
            {
                danger_type_open_long_ = OpenPosDangerType::SAFE;
                app_.local_logger().LogLocal(TAG_DISTINGUISH_TREND_LOG
                    , utility::FormatStr("\n[%04d %02d:%02d:00] danger_type_open_long_ switch to SAFE sub_lines_bias:%.2f sub_bias:%.2f"
                    , item.date, item.hhmmss/100, item.hhmmss%100, sub_lines_bias, sub_bias)); 
                break;
            }
        } 
        if( sub_lines_bias > 10 && sub_bias > 8 || sub_lines_bias > 15 )
        {
            //date_openlong_danger_tag_ = sub_hisdatas_->at(i)->stk_item.date; 
            danger_type_open_long_ = OpenPosDangerType::DANGER;  
            app_.local_logger().LogLocal(TAG_DISTINGUISH_TREND_LOG
                , utility::FormatStr("\n[%04d %02d:%02d:00] danger_type_open_long_ DANGER sub_lines_bias:%.2f sub_bias:%.2f"
                , item.date, item.hhmmss/100, item.hhmmss%100, sub_lines_bias, sub_bias)); 
            break;
        }
    }
#endif
}

void TrendDistinguish::Handle(const T_QuoteData &quote)
{
    auto update_trend = [this](TypePeriod k_type, T_HisDataItemContainer *hisdatas, StrategyTrendType &cur_trend, double &bias, double &lines_bias)
    {
        // line_0 is quick line line_1 is slow line-------
        auto pre3_data_index = hisdatas->size() - 1 - 3 ;
        double val_line_0_pre3 = app_.stock_data_man().GetAveLineZhibiao(ToPeriodType(k_type), code_, cst_ave_line_0_t, pre3_data_index); 
        double val_line_1_pre3 = app_.stock_data_man().GetAveLineZhibiao(ToPeriodType(k_type), code_, cst_ave_line_1_t, pre3_data_index); 

        auto cur_data_index = hisdatas->size() - 1;
        double val_line_0 = app_.stock_data_man().GetAveLineZhibiao(ToPeriodType(k_type), code_, cst_ave_line_0_t, cur_data_index); 
        double val_line_1 = app_.stock_data_man().GetAveLineZhibiao(ToPeriodType(k_type), code_, cst_ave_line_1_t, cur_data_index); 
        double cur_k_close = hisdatas->back()->stk_item.close_price;
        double distance_cur = val_line_1 - val_line_0;
        double distance_pre3 = val_line_1_pre3 - val_line_0_pre3;
        if( val_line_0 < val_line_1 )
        {
            if( cur_k_close < val_line_0 )
            {
                cur_trend = StrategyTrendType::SHORT;
                if( val_line_0 < val_line_0_pre3 )
                {
                    cur_trend = StrategyTrendType::STRONG_SHORT;
                    if( distance_cur > distance_pre3 )
                        cur_trend = StrategyTrendType::SSTRONG_SHORT;
                }else if( val_line_0 > val_line_0_pre3 )
                {
                    cur_trend = StrategyTrendType::SHORT_TO_UP;
                    if( distance_cur < distance_pre3 )
                        cur_trend = StrategyTrendType::SHORT_TO_UPUP;
                }
            }else
            {
                cur_trend = StrategyTrendType::SHORT_MAY_UP;
                if( val_line_0 > val_line_0_pre3 )
                {
                    cur_trend = StrategyTrendType::SHORT_TO_UP;
                    if( distance_cur < distance_pre3 )
                        cur_trend = StrategyTrendType::SHORT_TO_UPUP;
                } 
            }

        }else // val_line_0 >= val_line_1
        {
            if( cur_k_close > val_line_0 )
            {
                cur_trend = StrategyTrendType::LONG;
                if( val_line_0 > val_line_0_pre3 )
                {
                    cur_trend = StrategyTrendType::STRONG_LONG;
                    if( distance_cur < distance_pre3 )
                        cur_trend = StrategyTrendType::SSTRONG_LONG;
                }else if( val_line_0 < val_line_0_pre3 )
                {
                    cur_trend = StrategyTrendType::LONG_TO_DOWN;
                    if( distance_cur > distance_pre3 )
                        cur_trend = StrategyTrendType::LONG_TO_DOWNDOWN;
                }
            }else
            {
                cur_trend = StrategyTrendType::LONG_MAY_DOWN;
                if( val_line_0 < val_line_0_pre3 )
                {
                    cur_trend = StrategyTrendType::LONG_TO_DOWN;
                    if( distance_cur > distance_pre3 )
                        cur_trend = StrategyTrendType::LONG_TO_DOWNDOWN;
                }
            }
        }
        bias = sub_hisdatas_->back()->stk_item.close_price - val_line_0;
        lines_bias = val_line_0 - val_line_1; 
    };


    if( !sub_hisdatas_ || sub_hisdatas_->size() < 4 )
        return;

    const int minute = quote.hhmmss % 10000 / 100;
    const int second = quote.hhmmss % 100;
    if( !is_trend_judged_ && pre_judge_trend_stamp_ != minute && minute % 5 == 4 && second > 50 )
    {
        is_trend_judged_ = true;
        pre_judge_trend_stamp_ = minute;
        // line_0 is quick line line_1 is slow line-------
#ifdef MAKE_SUB_WALL
        update_trend(sub_k_type_, sub_hisdatas_, sub_cur_trend_, sub_bias_, sub_lines_bias_);
        app_.local_logger().LogLocal(TAG_DISTINGUISH_TREND_LOG
            , utility::FormatStr("\n%s sub_cur_trend:%d sub_lines_bias:%.2f sub_bias:%.2f", GetStamp(quote).c_str(), sub_cur_trend_, sub_lines_bias_, sub_bias_));
#endif
        update_trend(main_k_type_, main_hisdatas_, main_cur_trend_, main_bias_, main_lines_bias_);
        app_.local_logger().LogLocal(TAG_DISTINGUISH_TREND_LOG
            , utility::FormatStr("\n%s main_cur_trend:%d main_lines_bias:%.2f main_bias:%.2f", GetStamp(quote).c_str(), main_cur_trend_, main_lines_bias_, main_bias_));
#ifdef MAKE_SUB_WALL
        // FOR OPEN LONG POSITION DANGER INFO-----------------------
        if( sub_lines_bias_ > 15 && sub_bias_ > 10 || sub_lines_bias_ > 16 )
        { 
            danger_type_open_long_ = OpenPosDangerType::DANGER;
            app_.local_logger().LogLocal(TAG_DISTINGUISH_TREND_LOG
                , utility::FormatStr("\n%s danger_type_open_long_ DANGER sub_lines_bias:%.2f sub_bias:%.2f", GetStamp(quote).c_str(), sub_lines_bias_, sub_bias_));
        }else if( danger_type_open_long_ == OpenPosDangerType::DANGER )
        { 
            if( sub_bias_ < -4.0 || sub_lines_bias_ < -1.0 )
            {
                danger_type_open_long_ = OpenPosDangerType::HALF_DANGER;
                app_.local_logger().LogLocal(TAG_DISTINGUISH_TREND_LOG
                    , utility::FormatStr("\n%s danger_type_open_long_ switch to HALF_DANGER sub_lines_bias:%.2f sub_bias:%.2f", GetStamp(quote).c_str(), sub_lines_bias_, sub_bias_));
            }

        }else if( danger_type_open_long_ == OpenPosDangerType::HALF_DANGER )
        {
            if( sub_hisdatas_->size() < 4 )
                return;
            auto pre3_data_index = sub_hisdatas_->size() - 1 - 3 ;
            double val_line_0_pre3 = app_.stock_data_man().GetAveLineZhibiao(ToPeriodType(sub_k_type_), code_, cst_ave_line_0_t, pre3_data_index); 
            auto cur_data_index = sub_hisdatas_->size() - 1;
            double val_line_0 = app_.stock_data_man().GetAveLineZhibiao(ToPeriodType(sub_k_type_), code_, cst_ave_line_0_t, cur_data_index); 
            if( sub_lines_bias_ < 0 && val_line_0 > val_line_0_pre3 )
            {
                danger_type_open_long_ = OpenPosDangerType::SAFE;
                app_.local_logger().LogLocal(TAG_DISTINGUISH_TREND_LOG
                , utility::FormatStr("\n%s danger_type_open_long_ switch to SAFE sub_lines_bias:%.2f sub_bias:%.2f", GetStamp(quote).c_str(), sub_lines_bias_, sub_bias_));
            }
        }
#endif
        return;
    }else
        is_trend_judged_ = false;

    if( second % 10 == 0 )
    {
#ifdef MAKE_SUB_WALL
        double sub_val_line_0 = app_.stock_data_man().GetAveLineZhibiao(ToPeriodType(sub_k_type_), code_, cst_ave_line_0_t, sub_hisdatas_->size() - 1); 
        sub_bias_ = sub_hisdatas_->back()->stk_item.close_price - sub_val_line_0;
#endif
        double main_val_line_0 = app_.stock_data_man().GetAveLineZhibiao(ToPeriodType(main_k_type_), code_, cst_ave_line_0_t, main_hisdatas_->size() - 1); 
        main_bias_ = main_hisdatas_->back()->stk_item.close_price - main_val_line_0;
    }
 
    //sub_cur_trend_ = StrategyTrendType::UNKOWN;
    //main_cur_trend_ = StrategyTrendType::UNKOWN;
}

bool TrendDistinguish::is_breakup_strategy_allow()
{  
    bool an_forbid_condition = ( sub_cur_trend_ >= StrategyTrendType::LONG_MAY_DOWN && sub_lines_bias_ > 10.0 
        || sub_lines_bias_ > 14.6
        || main_bias_ > 4.4
        /*|| sub_bias_ > 6.0*/ );

    if( !an_forbid_condition ) 
        return true;
    else
    {
        int l_index = Lowest(*sub_hisdatas_, sub_hisdatas_->size() - 1, -3);
        double l_rel_val_line_0 = app_.stock_data_man().GetAveLineZhibiao(ToPeriodType(sub_k_type_), code_, cst_ave_line_0_t, l_index); 
        double val_line_0 = app_.stock_data_man().GetAveLineZhibiao(ToPeriodType(sub_k_type_), code_, cst_ave_line_0_t, sub_hisdatas_->size() - 1); 

        if( (*sub_hisdatas_)[l_index]->stk_item.low_price - l_rel_val_line_0 < -7.9
            && (*sub_hisdatas_)[sub_hisdatas_->size() - 1]->stk_item.close_price < val_line_0 )
            return true;
        else
            return false;
    }

}
 
bool TrendDistinguish::is_sub_bias_allow(bool open_long)
{
    auto cur_data_index = sub_hisdatas_->size() - 1;
    double val_line_0 = app_.stock_data_man().GetAveLineZhibiao(ToPeriodType(sub_k_type_), code_, cst_ave_line_0_t, cur_data_index); 
    double val_line_1 = app_.stock_data_man().GetAveLineZhibiao(ToPeriodType(sub_k_type_), code_, cst_ave_line_1_t, cur_data_index); 
    if( open_long )
    {
        if( val_line_0 < val_line_1 )
            return sub_bias_ < -1 * cst_bias_threshold;
        else
            return false;
    }else
    {
        if( val_line_0 > val_line_1 )
            return sub_bias_ > cst_bias_threshold;
        else
            return false;
    }
}


bool TrendDistinguish::is_main_bias_allow(bool open_long)
{
    auto cur_data_index = main_hisdatas_->size() - 1;
    double val_line_0 = app_.stock_data_man().GetAveLineZhibiao(ToPeriodType(main_k_type_), code_, cst_ave_line_0_t, cur_data_index); 
    double val_line_1 = app_.stock_data_man().GetAveLineZhibiao(ToPeriodType(main_k_type_), code_, cst_ave_line_1_t, cur_data_index); 
    if( open_long )
    {
        if( val_line_0 < val_line_1 )
            return main_bias_ < -1 * 4.0;
        else
            return false;
    }else
    {
        if( val_line_0 > val_line_1 )
            return main_bias_ > 4.0;
        else
            return false;
    }
}

std::string ToString(StrategyTrendType type)
{ 
    switch(type)
    {
    case StrategyTrendType::SSTRONG_LONG : return "SSTRONG_LONG";
    case StrategyTrendType::STRONG_LONG : return "STRONG_LONG";
    case StrategyTrendType::LONG :        return "LONG";
    case StrategyTrendType::LONG_MAY_DOWN: return "LONG_MAY_DOWN";
    case StrategyTrendType::LONG_TO_DOWN : return "LONG_TO_DOWN";
    case StrategyTrendType::LONG_TO_DOWNDOWN : return "LONG_TO_DOWNDOWN";
    case StrategyTrendType::SHORT_TO_UPUP : return "SHORT_TO_UPUP";
    case StrategyTrendType::SHORT_TO_UP :   return "SHORT_TO_UP";
    case StrategyTrendType::SHORT_MAY_UP: return "SHORT_MAY_UP";
    case StrategyTrendType::SHORT :         return "SHORT";
    case StrategyTrendType::STRONG_SHORT :  return "STRONG_SHORT";
    case StrategyTrendType::SSTRONG_SHORT : return "SSTRONG_SHORT";
    default :        return "UNKOWN";
    }
}
