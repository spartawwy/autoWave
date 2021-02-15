#include "kline_wall.h"
#include <cassert>
#include <qdebug.h>

#include <TLib/core/tsystem_utility_functions.h>

#include "mainwindow.h"
#include "stkfo_common.h"
#include "futures_forecast_app.h"
#include "exchange_calendar.h"

const static std::string cst_logf_pretag = "autoforcast";
// (c1, c2, c3) or (d1, d2, d3)
typedef std::tuple<double, double, double> T_TargetPrices;

void append_abc_index(T_HisDataItemContainer &k_datas, OUT std::vector<AbcIndexType> &abc_indexs, int index_a, int index_b, int index_c, int index_break);
using namespace TSystem;

struct ForecastPara
{
    unsigned int id;
    T_StockHisDataItem  item_a;
    int index_a;
    T_StockHisDataItem  item_b;
    int index_b;
    T_StockHisDataItem  item_c;
    int index_c;
    int index_break;
    bool is_ab_down;
};
static T_TargetPrices append_forcast_c(const std::string &code, TypePeriod k_type, ForecastPara &para, ForcastMan &forcast_man)
{
    T_Data2pForcast data_2pdown_fcst(para.id, para.is_ab_down);
    data_2pdown_fcst.stock_code = code;
    data_2pdown_fcst.date_a = para.item_a.date; 
    data_2pdown_fcst.hhmm_a = para.item_a.hhmmss;
    data_2pdown_fcst.index_a = para.index_a;
    data_2pdown_fcst.date_b = para.item_b.date; 
    data_2pdown_fcst.hhmm_b = para.item_b.hhmmss;
    data_2pdown_fcst.index_b = para.index_b;

    auto c1_c2_c3 = std::make_tuple(0.0, 0.0, 0.0);
    double spread = 0.0;
    double price_a = 0.0;
    double price_b = 0.0;
    if( para.is_ab_down )
    {
        c1_c2_c3 = ForcastC_ABDown(para.item_a.high_price, para.item_b.low_price);
        spread = fabs(para.item_a.high_price - para.item_b.low_price);
        price_a = para.item_a.high_price;
        price_b = para.item_b.low_price;
    }else
    {
        c1_c2_c3 = ForcastC_ABUp(para.item_a.low_price, para.item_b.high_price);
        spread = fabs(para.item_a.low_price - para.item_b.high_price);
        price_a = para.item_a.low_price;
        price_b = para.item_b.high_price;
    }
    data_2pdown_fcst.c1 = std::get<0>(c1_c2_c3);
    data_2pdown_fcst.c2 = std::get<1>(c1_c2_c3);
    data_2pdown_fcst.c3 = std::get<2>(c1_c2_c3);
    data_2pdown_fcst.price_spread_type = JudgePriceSpreadType(k_type, spread);
    data_2pdown_fcst.price_a = price_a;
    data_2pdown_fcst.price_b = price_b;
    forcast_man.Append(k_type, code, para.is_ab_down, data_2pdown_fcst);
    return c1_c2_c3;
}

static T_TargetPrices forcast_2p(const T_StockHisDataItem &item_a, const T_StockHisDataItem &item_b, bool is_ab_down)
{
    auto c1_c2_c3 = std::make_tuple(0.0, 0.0, 0.0);
    if( is_ab_down )
        c1_c2_c3 = ForcastC_ABDown(item_a.high_price, item_b.low_price);
    else
        c1_c2_c3 = ForcastC_ABUp(item_a.low_price, item_b.high_price);
    return c1_c2_c3;
}

static void append_forcast_d(const std::string &code, TypePeriod k_type, ForecastPara &para, ForcastMan &forcast_man)
{
    T_Data3pForcast data_3pdown_fcst(para.id, para.is_ab_down);
    data_3pdown_fcst.stock_code = code;
    data_3pdown_fcst.date_a = para.item_a.date; 
    data_3pdown_fcst.hhmm_a = para.item_a.hhmmss;
    data_3pdown_fcst.index_a = para.index_a;
    data_3pdown_fcst.date_b = para.item_b.date; 
    data_3pdown_fcst.hhmm_b = para.item_b.hhmmss;
    data_3pdown_fcst.index_b = para.index_b;
    data_3pdown_fcst.date_c = para.item_c.date; 
    data_3pdown_fcst.hhmm_c = para.item_c.hhmmss;
    data_3pdown_fcst.index_c = para.index_c;
    data_3pdown_fcst.index_break = para.index_break;
    auto d1_d2_d3 = std::make_tuple(0.0, 0.0, 0.0);
    double spread = 0.0;
    double price_a = 0.0;
    double price_b = 0.0;
    if( para.is_ab_down )
    {
        d1_d2_d3 = ForcastD_ABC_Down(para.item_a.high_price, para.item_b.low_price);
        spread = fabs(para.item_a.high_price - para.item_b.low_price);
        price_a = para.item_a.high_price;
        price_b = para.item_b.low_price;
    }else
    {
        d1_d2_d3 = ForcastD_ABC_Up(para.item_a.low_price, para.item_b.high_price);
        spread = fabs(para.item_a.low_price - para.item_b.high_price);
        price_a = para.item_a.low_price;
        price_b = para.item_b.high_price;
    }
    data_3pdown_fcst.d1 = std::get<0>(d1_d2_d3);
    data_3pdown_fcst.d2 = std::get<1>(d1_d2_d3);
    data_3pdown_fcst.d3 = std::get<2>(d1_d2_d3);
    data_3pdown_fcst.price_spread_type = JudgeTrendPriceSpreadType(k_type, spread);
    data_3pdown_fcst.price_a = price_a;
    data_3pdown_fcst.price_b = price_b;
    forcast_man.Append(k_type, code, para.is_ab_down, data_3pdown_fcst);
}

#if 0 
void KLineWall::HandleAutoForcast()
{ 
    static auto find_price_min_max_index = [](const T_HisDataItemContainer &k_datas, int beg_index, int end_index)
    {
        int min_price_index = beg_index;
        int max_price_index = beg_index;
        double min_price = MAX_PRICE;
        double max_price = MIN_PRICE;
        int i = beg_index;
        for( ; i <= end_index; ++i )
        {
            if( k_datas[i]->stk_item.low_price < min_price )
            {
                min_price = k_datas[i]->stk_item.low_price;
                min_price_index = i;
            }
            if( k_datas[i]->stk_item.high_price > max_price )
            {
                max_price = k_datas[i]->stk_item.high_price;
                max_price_index = i;
            }
        }
        return std::make_tuple(min_price_index, max_price_index);
    };

    if( !main_win_->is_show_autoforcast() )
        return;
    auto_forcast_man_.RemoveForcastItems(stock_code_, k_type_);

    double max_spread = 100.0;
    double min_spread = 1.0;
    switch(k_type_)
    {
    case TypePeriod::PERIOD_YEAR: max_spread = 500.0; min_spread = 50.0; break;
    case TypePeriod::PERIOD_MON:
    case TypePeriod::PERIOD_WEEK: max_spread = 300.0; min_spread = 50.0;  break;
    case TypePeriod::PERIOD_DAY: max_spread = 200; min_spread = 5.0; break;
    case TypePeriod::PERIOD_HOUR:max_spread = 100; min_spread = 3.0; break;
    case TypePeriod::PERIOD_30M: max_spread = 100; min_spread = 2.0; break;
    case TypePeriod::PERIOD_15M: max_spread = 100; min_spread = 1.0; break;
    case TypePeriod::PERIOD_5M: max_spread = 50.0; min_spread = 1.0; break;
    case TypePeriod::PERIOD_1M: max_spread = 30.0; min_spread = 1.0; break;
    default: break;
    }

    // begin_index on left; end_index on right
    T_StructLineContainer &line_datas = app_->stock_data_man().GetStructLineContainer(ToPeriodType(k_type_), stock_code_); 
    if( line_datas.empty() )
        return;

    T_HisDataItemContainer &k_datas = app_->stock_data_man().GetHisDataContainer(k_type_, stock_code_);
    assert(k_datas.size() > k_rend_index_for_train_);
    const int k_end_index_for_train = k_datas.size() - 1 - k_rend_index_for_train_;
    T_StockHisDataItem &item_cur = k_datas[k_end_index_for_train]->stk_item;
    //const double cur_price = item_cur.close_price;

    T_StockHisDataItem &item_a = k_datas[line_datas[0]->beg_index]->stk_item; // struct_line0_left_item
    T_StockHisDataItem &item_b = k_datas[line_datas[0]->end_index]->stk_item; // struct_line0_right_item

    bool is_ab_down = line_datas[0]->type == LineType::DOWN;

    /*  /a\
           \b
    */
    if( line_datas[0]->type == LineType::DOWN )
    {  
        if( line_datas.size() == 1 )
        {
            auto min_max_price_indexs = find_price_min_max_index(k_datas, line_datas[0]->end_index + 1, k_end_index_for_train);
            double max_price = k_datas[std::get<1>(min_max_price_indexs)]->stk_item.high_price;
            if( max_price > item_a.high_price )
            {
                T_StockHisDataItem &item_max_price = k_datas[std::get<1>(min_max_price_indexs)]->stk_item;
                append_forcast_c(stock_code_, k_type_, item_a, item_max_price, false, auto_forcast_man_);
            }else
                append_forcast_c(stock_code_, k_type_, item_a, item_b, is_ab_down, auto_forcast_man_);

            double min_price = k_datas[std::get<0>(min_max_price_indexs)]->stk_item.high_price;
            if( min_price < item_b.low_price )
            {
                // forcast d
                append_forcast_d(stock_code_, k_type_, item_a, item_b, true, auto_forcast_man_);
            }

        }else if( line_datas.size() == 2 )
        {
            app_->local_logger().LogLocal(cst_logf_pretag, utility::FormatStr("LineType::DOWN lines 2"));
            
        }else
        {
            // find target line-------------
            unsigned int target_line_index = 0;
            double highest_price = item_a.high_price;
            for( unsigned int i = 2; i < line_datas.size(); i += 2 ) // toward left
            {
                //assert(line_datas[i]->type == LineType::DOWN);
                if( line_datas[i]->type != LineType::DOWN )
                    break;
                T_StockHisDataItem &item_left = k_datas[line_datas[i]->beg_index]->stk_item; 
                T_StockHisDataItem &item_right = k_datas[line_datas[i]->end_index]->stk_item; 
                if( item_right.low_price < item_b.low_price )
                    break;
                if( item_left.high_price < highest_price )
                    break;
                if( item_left.high_price - item_b.high_price > max_spread )
                    break;
                if( item_left.high_price > highest_price )
                    highest_price = item_left.high_price;
                target_line_index = i;
            }
            append_forcast_c(stock_code_, k_type_, k_datas[line_datas[target_line_index]->beg_index]->stk_item, item_b, is_ab_down, auto_forcast_man_);
        }

    }else // [0] is LineType::UP
    {  
        /*     /b
            \a/
        */
        if( line_datas.size() == 1 )
        {
            auto min_max_price_indexs = find_price_min_max_index(k_datas, line_datas[0]->end_index + 1, k_end_index_for_train);
            double min_price = k_datas[std::get<0>(min_max_price_indexs)]->stk_item.low_price;
            if( min_price < item_a.low_price )
            { 
                T_StockHisDataItem &item_min_price = k_datas[std::get<0>(min_max_price_indexs)]->stk_item;
                append_forcast_c(stock_code_, k_type_, item_b, item_min_price, true, auto_forcast_man_);
            }else
                append_forcast_c(stock_code_, k_type_, item_a, item_b, is_ab_down, auto_forcast_man_);
             
            if( min_price < item_b.low_price && min_price > item_a.low_price )
            {
                // forcast d
                append_forcast_d(stock_code_, k_type_, item_a, item_b, false, auto_forcast_man_);
            }

        }else if( line_datas.size() == 2 )
        {
            app_->local_logger().LogLocal(cst_logf_pretag, utility::FormatStr("LineType::UP lines 2"));
            // assert [1] line is down line
            if( k_datas[line_datas[1]->beg_index]->stk_item.high_price > k_datas[line_datas[0]->end_index]->stk_item.high_price )
                append_forcast_c(stock_code_, k_type_, k_datas[line_datas[1]->beg_index]->stk_item, k_datas[line_datas[1]->end_index]->stk_item, true, auto_forcast_man_);

        }else if( line_datas.size() == 3 )
        {
            append_forcast_c(stock_code_, k_type_, item_a, item_b, is_ab_down, auto_forcast_man_);
        }else // > 3
        {
            // find target line-------------
            unsigned int target_line_index = 0;
            double lowest_price = item_a.low_price;
            for( unsigned int i = 2; i < line_datas.size(); i += 2 ) // leftward /../
            {
                //assert(line_datas[i]->type == LineType::UP);
                if( line_datas[i]->type != LineType::UP )
                    break;
                T_StockHisDataItem &item_left = k_datas[line_datas[i]->beg_index]->stk_item; 
                T_StockHisDataItem &item_right = k_datas[line_datas[i]->end_index]->stk_item; 
                if( item_right.high_price > item_b.high_price )
                    break;
                if( item_left.low_price > lowest_price )
                    break;
                if( item_b.high_price - item_left.low_price > max_spread )
                    break;
                if( item_left.low_price < lowest_price )
                    lowest_price = item_left.low_price;
                target_line_index = i;
                 
            } // for
            append_forcast_c(stock_code_, k_type_, k_datas[line_datas[target_line_index]->beg_index]->stk_item, item_b, is_ab_down, auto_forcast_man_);
        }
    }// LineType::UP
}
#endif



void KLineWall::HandleAutoForcast_large()
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

    BounceUpAutoForcast(k_type_, k_datas, left_end_index, right_end_index);
    //BounceDownAutoForcast(k_type_, k_datas, left_end_index, right_end_index);

    //TrendUpAutoForcast(k_type_, k_datas, left_end_index, right_end_index);
    //TrendDownAutoForcast(k_type_, k_datas, left_end_index, right_end_index);
     
#if 0
    static auto find_price_min_max_index = [](const T_HisDataItemContainer &k_datas, int beg_index, int end_index)
    {
        int min_price_index = beg_index;
        int max_price_index = beg_index;
        double min_price = MAX_PRICE;
        double max_price = MIN_PRICE;
        int i = beg_index;
        for( ; i <= end_index; ++i )
        {
            if( k_datas[i]->stk_item.low_price < min_price )
            {
                min_price = k_datas[i]->stk_item.low_price;
                min_price_index = i;
            }
            if( k_datas[i]->stk_item.high_price > max_price )
            {
                max_price = k_datas[i]->stk_item.high_price;
                max_price_index = i;
            }
        }
        return std::make_tuple(min_price_index, max_price_index);
    };

    auto_forcast_man_.RemoveForcastItems(stock_code_, k_type_);

    double max_spread = 100.0;
    double min_spread = 1.0;
    switch(k_type_)
    {
    case TypePeriod::PERIOD_WEEK: max_spread = 300.0; min_spread = 50.0;  break;
    case TypePeriod::PERIOD_DAY: max_spread = 200; min_spread = 5.0; break;
    case TypePeriod::PERIOD_HOUR:max_spread = 100; min_spread = 3.0; break;
    case TypePeriod::PERIOD_30M: max_spread = 100; min_spread = 2.0; break;
    case TypePeriod::PERIOD_15M: max_spread = 100; min_spread = 1.0; break;
    case TypePeriod::PERIOD_5M: max_spread = 50.0; min_spread = 1.0; break;
    case TypePeriod::PERIOD_1M: max_spread = 30.0; min_spread = 1.0; break;
    default: break;
    }
    int trend_top_index = 0, trend_mid_index = 0, trend_btm_index = 0;
    int start_index = k_datas.size() - 1;
    int toward_left_len = 5;
    TrendType trend_type = TrendType::NO;
    do
    {
        if( start_index - toward_left_len <= 0 )
            return;
        trend_type = JudgeTrendBaseonFxRightToLeft(k_datas, start_index, toward_left_len, trend_top_index, trend_mid_index, trend_btm_index);

        if( trend_type == TrendType::NO || trend_type == TrendType::SHOCK )
        {
            toward_left_len += 5; 
            continue;
        }
        if( trend_type == TrendType::DOWN )  // bounce up forecast
        {
            if( k_datas[start_index]->stk_item.high_price > k_datas[trend_top_index]->stk_item.high_price )
            {
                toward_left_len += 5; 
                continue;
            }else
            { 
                append_forcast_c(stock_code_, k_type_, k_datas[trend_top_index]->stk_item, k_datas[trend_btm_index]->stk_item, true/*is_ab_down*/, auto_forcast_man_);
                break;
            }
        }
    
    }while(true);
#endif
     
}
 
void KLineWall::BounceUpAutoForcast(TypePeriod type_period, T_HisDataItemContainer &k_datas, int left_end_index, int right_end_index)
{
    // follow codes is for PERIOD_5M -----------------------
    /*
    spread: micro <1.51;  1.5 < small < 3.01;  3.0 < mid < 5.01; 5.0 < big < 10.01;  10.0 < supper 
    
    enum class PriceSpreadType : unsigned char
    {
        MICRO=0, SMALL, MID, BIG, SUPPER,
    };*/
    //auto_forcast_man_.Remove2pForcastItems(stock_code_, type_period, true);
    assert(left_end_index <= right_end_index);
    int left_end_i = left_end_index;
    int latest_forcast_index = -1;
    bool small_range_flag = false;
    Data2pForcastInnerContainer * bounceup_forcasts = auto_forcast_man_.Find2pForcastVector(stock_code_, type_period, true);
    if( bounceup_forcasts )
    {
        if( bounceup_forcasts->size() > 50 ) // erase old forecast which before 5 trading days
        { 
            srand(time(0));
            if( rand() % 3 == 0 )
            {
                for( auto iter = bounceup_forcasts->begin(); iter != bounceup_forcasts->end(); )
                {
                    if(  (*iter).index_a + (type_period <= TypePeriod::PERIOD_5M ? 156 : 156/6) * 5 < right_end_index )
                        bounceup_forcasts->erase(iter++);
                    else 
                        ++iter;
                }
            }
        }

        std::vector<unsigned int> ids_need_remove;
        double price_a_min = 99999.9;
        double price_b_max = 0.0;
        std::for_each(bounceup_forcasts->begin(), bounceup_forcasts->end(), [right_end_index, &ids_need_remove, &k_datas, &price_a_min, &price_b_max](Data2pForcastInnerContainer::reference entry)
        {
            if( entry.price_a < price_a_min )
                price_a_min = entry.price_a;
            if( entry.price_b > price_b_max )
                price_b_max = entry.price_b;
            const double cst_tolerance = 0.3; // for breakup strategy. ndchk
            if( k_datas[right_end_index]->stk_item.high_price > entry.price_a + cst_tolerance )
            {
                entry.breaked_a = true;
                ids_need_remove.push_back(entry.id);
            }
            else if( k_datas[right_end_index]->stk_item.low_price < entry.price_b + EPSINON )
            {
                if( entry.index_b == right_end_index )
                {
                    // update forecast ---------
                    entry.price_b = k_datas[right_end_index]->stk_item.low_price;
                    auto c1_c2_c3 = ForcastC_ABDown(k_datas[entry.index_a]->stk_item.high_price, entry.price_b);
                    entry.c1 = std::get<0>(c1_c2_c3);
                    entry.c2 = std::get<1>(c1_c2_c3);
                    entry.c3 = std::get<2>(c1_c2_c3);

                }else if( k_datas[right_end_index]->stk_item.low_price < entry.price_b )
                {
                    ids_need_remove.push_back(entry.id);
                    entry.breaked_b = true;
                }
            }
        });
        if( !bounceup_forcasts->empty() && price_b_max < k_datas[right_end_index]->stk_item.low_price && k_datas[right_end_index]->stk_item.high_price < price_a_min )
        {
            small_range_flag = true;
#if 0
            T_KlineDataItem item;
            item.stk_item.date = bounceup_forcasts->rbegin()->date_a;
            item.stk_item.hhmmss = bounceup_forcasts->rbegin()->hhmm_a;
            latest_forcast_index = Lower_Bound(k_datas, left_end_index, right_end_index, item);
#else 
            latest_forcast_index = bounceup_forcasts->rbegin()->index_a;
#endif
            assert(latest_forcast_index > -1);
            left_end_i = latest_forcast_index + 1;
        }else
        {
#if 0
            for( int i = 0; i < ids_need_remove.size(); ++i )
            {
                auto_forcast_man_.Erase2pForcast(stock_code_, type_period, true, ids_need_remove[i]);
            }
#endif
        }
    }
    
    // bounce up forecast---------------
    
    bool try_left = true;

    int highest_p_i = 0; // highest price index

    const int e_try_distance = 6;
    highest_p_i = Highest(k_datas, right_end_index, left_end_i - right_end_index);
    if( !small_range_flag && try_left )// try left a distance untill no higher price
    { 
        do 
        {
            if( highest_p_i < 1 )
                break;
            int h_i = Highest(k_datas, highest_p_i - 1, -1 * e_try_distance);
            if( KRef(k_datas, h_i, KAttributeType::HIGH) < KRef(k_datas, highest_p_i, KAttributeType::HIGH) )
                break;
            highest_p_i = h_i; 
        } while (true);
    }
    assert(highest_p_i <= right_end_index);
    int lowest_p_i = Lowest(k_datas, highest_p_i, right_end_index - highest_p_i);
    //int lowest_price = KRef(k_datas, lowest_p_i, KAttributeType::LOW);
    if( highest_p_i == right_end_index )
    {
        if( KRef(k_datas, highest_p_i, KAttributeType::CLOSE) < KRef(k_datas, highest_p_i, KAttributeType::OPEN)
            && JudgePriceSpreadType(type_period, KRef(k_datas, highest_p_i, KAttributeType::HIGH) - KRef(k_datas, highest_p_i, KAttributeType::LOW)) >= PriceSpreadType::MID )
        {
            ForecastPara para;
            para.is_ab_down = true;
            para.item_a = k_datas[highest_p_i]->stk_item;
            para.index_a = highest_p_i;
            para.item_b = k_datas[lowest_p_i]->stk_item;
            para.index_b = lowest_p_i;
            if( !auto_forcast_man_.Find2pForcast(stock_code_, type_period, para.is_ab_down, *k_datas[highest_p_i], *k_datas[lowest_p_i]) )
            {
                para.id = app_->GenerateForecastId();
                append_forcast_c(stock_code_, type_period, para, auto_forcast_man_);
            }
        }
    }else
    { 
        std::vector<int> top_tractals;
        if( lowest_p_i == highest_p_i )
            top_tractals.push_back(highest_p_i);
        else // find top fractal between[highest_index, right_end_index): top_fenx_indexs
            top_tractals = FindFractalsBetween(k_datas, FractalGeneralType::TOP_FRACTAL, right_end_index, highest_p_i);
        
        // filter which high price <=  right_end_index's high price------
        double right_end_p_h = KRef(k_datas, right_end_index, KAttributeType::HIGH);
        std::vector<int> top_tractals0;
        for( int i = 0; i < top_tractals.size(); ++i )
        {
            if( KRef(k_datas, top_tractals[i], KAttributeType::HIGH) > right_end_p_h )
                top_tractals0.push_back(top_tractals[i]);
        }
        double tag_price = 0.0;
 
        // filter mid price top fenxin ----------------------------
        std::vector<int> top_tractals1;
        tag_price = 0.0;
        for( int i = 0; i < top_tractals0.size(); ++i )
        {
            if( KRef(k_datas, top_tractals0[i], KAttributeType::HIGH) > tag_price )
            {
                tag_price = KRef(k_datas, top_tractals0[i], KAttributeType::HIGH);
                top_tractals1.push_back(top_tractals0[i]);
            }
        }
        if( small_range_flag )
            top_tractals1.push_back(latest_forcast_index); 
        // filter top fenxin which is nearby price; ---------
        std::vector<int> top_tractals2; //lay from high to low 
        if( small_range_flag )
        {
            top_tractals2.push_back(latest_forcast_index);
            tag_price = KRef(k_datas, latest_forcast_index, KAttributeType::HIGH);
        }else if( !top_tractals1.empty() )
        {
            top_tractals2.push_back(top_tractals1.back());
            tag_price = KRef(k_datas, top_tractals1.back(), KAttributeType::HIGH);
        }
        //walk from high price to low(also is from ago to near righ_end_index)
        for( int i = (int)top_tractals1.size() - 2; i >= 0; --i )
        {
            auto spread_type = JudgePriceSpreadType(type_period, tag_price - KRef(k_datas, top_tractals1[i], KAttributeType::HIGH));
#if 0
            // enter new trade day and has already walk 12 k
            if( i+1 < top_tractals1.size() && k_datas[top_tractals1[i+1]]->stk_item.hhmmss < 2100 && k_datas[top_tractals1[i]]->stk_item.hhmmss > 2100 
                && abs(top_tractals1[i+1] - top_tractals1[i]) > 12 )
            {
                if( spread_type > PriceSpreadType::SMALL ) 
                {
                    if( !top_tractals2.empty() )
                        top_tractals2.pop_back();
                    top_tractals2.push_back(top_tractals1[i]);
                    tag_price = KRef(k_datas, top_tractals1[i], KAttributeType::HIGH);
                    continue;
                }
            }
#endif
            if( abs(top_tractals1[i+1] - top_tractals1[i]) > 12 && spread_type > PriceSpreadType::MINI )
            {
                top_tractals2.push_back(top_tractals1[i]);
                tag_price = KRef(k_datas, top_tractals1[i], KAttributeType::HIGH);
                continue;
            }
            if( spread_type > PriceSpreadType::MICRO )
            {
                if( spread_type > PriceSpreadType::SMALL )
                {
                    tag_price = KRef(k_datas, top_tractals1[i], KAttributeType::HIGH);
                    top_tractals2.push_back(top_tractals1[i]);
                }else // small type
                {
                    if( top_tractals2.empty() )
                        top_tractals2.push_back(top_tractals1[i]);
                    else if( abs(top_tractals2.back() - top_tractals1[i]) > 5 ) // to previous top distance > 5
                    {
                        tag_price = KRef(k_datas, top_tractals1[i], KAttributeType::HIGH);
                        top_tractals2.push_back(top_tractals1[i]);
                    }
                } 
            }
        }

        auto Limit_Level = PriceSpreadType::SMALL;
        // filter and create forecast. //top_tractals2[0..n] from high price to low(also is from ago to near righ_end_index)
        for( int i = 0; i < top_tractals2.size(); ++i )
        {
            int lowest_price_index = Lowest(k_datas, top_tractals2[i], right_end_index - top_tractals2[i]);
            auto lowest_price = KRef(k_datas, lowest_price_index, KAttributeType::LOW);
            if( JudgePriceSpreadType(type_period
                , KRef(k_datas, top_tractals2[i], KAttributeType::HIGH)- lowest_price) >= Limit_Level )
            { 
                ForecastPara para;
                para.is_ab_down = true;
                para.index_a = top_tractals2[i];
                para.item_a = k_datas[para.index_a]->stk_item;
                para.index_b = lowest_price_index;
                para.item_b = k_datas[para.index_b]->stk_item;
                if( !auto_forcast_man_.Find2pForcast(stock_code_, type_period, para.is_ab_down, *k_datas[para.index_a], *k_datas[para.index_b]) )
                {
                    para.id = app_->GenerateForecastId();
                    append_forcast_c(stock_code_, type_period, para, auto_forcast_man_);
                }
            }
        }

    } 
}


void KLineWall::BounceDownAutoForcast(TypePeriod type_period, T_HisDataItemContainer &k_datas, int left_end_index, int right_end_index)
{
    // follow codes is for PERIOD_5M -----------------------
    
    //auto_forcast_man_.Remove2pForcastItems(stock_code_, type_period, true);
    assert(left_end_index <= right_end_index);
    //bool is_ab_down = false;
    int left_end_i = left_end_index;
    int latest_forcast_index = -1;
    bool small_range_flag = false;
    Data2pForcastInnerContainer * bouncedown_forcasts = auto_forcast_man_.Find2pForcastVector(stock_code_, type_period, false);
    if( bouncedown_forcasts )
    {
        std::vector<unsigned int> ids_need_remove;
        double price_a_max = 0.0;
        double price_b_min = 99999.9;
        std::for_each(bouncedown_forcasts->begin(), bouncedown_forcasts->end(), [right_end_index, &ids_need_remove, &k_datas, &price_a_max, &price_b_min](Data2pForcastInnerContainer::reference entry)
        {
            if( entry.price_a > price_a_max )
                price_a_max = entry.price_a;
            if( entry.price_b < price_b_min )
                price_b_min = entry.price_b;
            if( k_datas[right_end_index]->stk_item.low_price < entry.price_a )
                ids_need_remove.push_back(entry.id);
            else if( k_datas[right_end_index]->stk_item.high_price > entry.price_b )
                ids_need_remove.push_back(entry.id);
        });
        if( !bouncedown_forcasts->empty() && price_a_max < k_datas[right_end_index]->stk_item.low_price && k_datas[right_end_index]->stk_item.high_price < price_b_min )
        {
            small_range_flag = true;
#if 0
            T_KlineDataItem item;
            item.stk_item.date = bouncedown_forcasts->rbegin()->date_a;
            item.stk_item.hhmmss = bouncedown_forcasts->rbegin()->hhmm_a;
            latest_forcast_index = Lower_Bound(k_datas, left_end_index, right_end_index, item);
#else
            latest_forcast_index = bouncedown_forcasts->rbegin()->index_a;
#endif
            assert(latest_forcast_index > -1);
            left_end_i = latest_forcast_index + 1;
        }else
        {
            for( int i = 0; i < ids_need_remove.size(); ++i )
            {
                auto_forcast_man_.Erase2pForcast(stock_code_, type_period, false, ids_need_remove[i]);
            }
        }
    }
    
    // bounce up forecast---------------
    
    bool try_left = true;

    int lowest_p_i = 0; // lowest price index

    const int e_try_distance = 6;
    //highest_p_i = Highest(k_datas, right_end_index, left_end_i - right_end_index);
    lowest_p_i = Lowest(k_datas, right_end_index, left_end_i - right_end_index);
    if( !small_range_flag && try_left )// try left a distance untill no lower price
    { 
        do 
        {
            if( lowest_p_i < 1 )
                break;
            int l_i = Lowest(k_datas, lowest_p_i - 1, -1 * e_try_distance);
            if( KRef(k_datas, l_i, KAttributeType::LOW) > KRef(k_datas, lowest_p_i, KAttributeType::LOW) )
                break;
            lowest_p_i = l_i; 
        } while (true);
    }
    assert(lowest_p_i <= right_end_index);
    //int lowest_p_i = Lowest(k_datas, highest_p_i, right_end_index - highest_p_i);
    int highest_p_i = Highest(k_datas, lowest_p_i, right_end_index - lowest_p_i);
     
    if( lowest_p_i == right_end_index ) // lowest price is current(right_end_index)
    {
        // 判断对当前K(一根大幅度阳)做向下弹预测
        if( KRef(k_datas, lowest_p_i, KAttributeType::CLOSE) > KRef(k_datas, lowest_p_i, KAttributeType::OPEN)
            && JudgePriceSpreadType(type_period, KRef(k_datas, lowest_p_i, KAttributeType::HIGH) - KRef(k_datas, lowest_p_i, KAttributeType::LOW)) >= PriceSpreadType::MID )
        {
            ForecastPara para;
            para.is_ab_down = false;
            para.index_a = lowest_p_i;
            para.item_a = k_datas[para.index_a]->stk_item;
            para.index_b = highest_p_i;
            para.item_b = k_datas[para.index_b]->stk_item;
            if( !auto_forcast_man_.Find2pForcast(stock_code_, type_period, para.is_ab_down, *k_datas[para.index_a], *k_datas[para.index_b]) )
            {
                para.id = app_->GenerateForecastId();
                append_forcast_c(stock_code_, type_period, para, auto_forcast_man_);
            }
        }
    }else
    { 
        std::vector<int> btm_tractals;
        if( lowest_p_i == highest_p_i ) // same k
            btm_tractals.push_back(lowest_p_i);
        else // find btm fractal between[lowest_p_i, right_end_index): btm_fenx_indexs, layed from low to lower
            btm_tractals = FindFractalsBetween(k_datas, FractalGeneralType::BTM_FRACTAL, right_end_index, lowest_p_i);
         
        // filter which low price >=  right_end_index's low price------
        double right_end_p_l = KRef(k_datas, right_end_index, KAttributeType::LOW);
        std::vector<int> btm_tractals0; // layed from low to lower
        for( int i = 0; i < btm_tractals.size(); ++i )
        {
            if( !(KRef(k_datas, btm_tractals[i], KAttributeType::LOW) < right_end_p_l) )
                continue;
            btm_tractals0.push_back(btm_tractals[i]);
        }
        double tag_price = 0.0;
 
        // filter mid price btm fenxin ----------------------------
        std::vector<int> btm_tractals1; // layed from low to lower
        tag_price = 99999.9;
        for( int i = 0; i < btm_tractals0.size(); ++i )
        {
            if( KRef(k_datas, btm_tractals0[i], KAttributeType::LOW) < tag_price )
            {
                tag_price = KRef(k_datas, btm_tractals0[i], KAttributeType::LOW);
                btm_tractals1.push_back(btm_tractals0[i]);
            }
        }
        if( small_range_flag )
            btm_tractals1.push_back(latest_forcast_index); 
        // filter top fenxin which is nearby price;  ---------
        std::vector<int> btm_tractals2; //lay from low to high
        if( small_range_flag )
        {
            btm_tractals2.push_back(latest_forcast_index);
            tag_price = KRef(k_datas, latest_forcast_index, KAttributeType::LOW);
        }else if( !btm_tractals1.empty() )
        {
            btm_tractals2.push_back(btm_tractals1.back());
            tag_price = KRef(k_datas, btm_tractals1.back(), KAttributeType::LOW);
        }
        //walk from low price to high(also is from ago to near righ_end_index)
        for( int i = (int)btm_tractals1.size() - 2; i >= 0; --i )
        {
            auto spread_type = JudgePriceSpreadType(type_period, tag_price - KRef(k_datas, btm_tractals1[i], KAttributeType::LOW));
 
            if( abs(btm_tractals1[i+1] - btm_tractals1[i]) > 12 && spread_type > PriceSpreadType::MINI )
            {
                btm_tractals2.push_back(btm_tractals1[i]);
                tag_price = KRef(k_datas, btm_tractals1[i], KAttributeType::LOW);
                continue;
            }
            if( spread_type > PriceSpreadType::MICRO )
            {
                if( spread_type > PriceSpreadType::SMALL )
                {
                    tag_price = KRef(k_datas, btm_tractals1[i], KAttributeType::LOW);
                    btm_tractals2.push_back(btm_tractals1[i]);
                }else // small type
                {
                    if( btm_tractals2.empty() )
                        btm_tractals2.push_back(btm_tractals1[i]);
                    else if( abs(btm_tractals2.back() - btm_tractals1[i]) > 5 ) // to previous btm distance > 5
                    {
                        tag_price = KRef(k_datas, btm_tractals1[i], KAttributeType::LOW);
                        btm_tractals2.push_back(btm_tractals1[i]);
                    }
                } 
            }
        }

        // filter and create forecast. //btm_tractals2[0..n] from low price to high(also is from ago to near righ_end_index)
        auto Limit_Level = PriceSpreadType::SMALL;
        for( int i = 0; i < btm_tractals2.size(); ++i )
        {
            int highest_price_index = Highest(k_datas, btm_tractals2[i], right_end_index - btm_tractals2[i]);
            auto highest_price = KRef(k_datas, highest_price_index, KAttributeType::HIGH);
            if( JudgePriceSpreadType(type_period
                , highest_price - KRef(k_datas, btm_tractals2[i], KAttributeType::LOW)) >= Limit_Level )
            { 
                ForecastPara para;
                para.is_ab_down = false;
                para.index_a = btm_tractals2[i];
                para.item_a = k_datas[para.index_a]->stk_item;
                para.index_b = highest_price_index;
                para.item_b = k_datas[para.index_b]->stk_item;
                if( !auto_forcast_man_.Find2pForcast(stock_code_, type_period, para.is_ab_down, *k_datas[para.index_a], *k_datas[para.index_b]) )
                {
                    para.id = app_->GenerateForecastId();
                    append_forcast_c(stock_code_, type_period, para, auto_forcast_man_);
                }
            }
        }
    } 
}

// toward right [start, right_end_index]
// ps: if no inflection price also will return false
bool FindTrendUpIndexBCnotUseFractal(T_HisDataItemContainer &k_datas, int index_a, int right_end_index, OUT int &index_b, OUT int &index_c)
{
    if( index_a >= right_end_index ) 
        return false;

    int index_h = index_a;
    double price_h = KRef(k_datas, index_a, KAttributeType::HIGH);

    int i = index_a; 
    while( i < right_end_index)
    {
        int l_p_i = Lowest(k_datas, i, right_end_index - i);
        // break down index a's low price
        if( KRef(k_datas, l_p_i, KAttributeType::LOW) < KRef(k_datas, index_a, KAttributeType::LOW) )
            return false;

        double next_i_h = KRef(k_datas, i+1, KAttributeType::HIGH);
        if( next_i_h > KRef(k_datas, i, KAttributeType::HIGH) ) // 高点抬高
        {     
            if( KRef(k_datas, i+1, KAttributeType::LOW) < KRef(k_datas, i, KAttributeType::LOW) )// i +1 occur lower price
            { 
                index_b = index_h;
                index_c = i + 1;
                return true;
            }else if( i + 1 == right_end_index && KRef(k_datas, i+1, KAttributeType::LOW) < KRef(k_datas, i, KAttributeType::LOW) 
                && next_i_h > price_h ) //next index is last and next's low price < b's low and next's low break up
            {
                index_b = index_h;
                index_c = i + 1;
                return true;
            } 
            if( next_i_h > price_h ) 
            {  // update high price and related index
                price_h = next_i_h; 
                index_h = i + 1;
            }
            ++i; 
        }else // 高点未继续抬高
        {
            if( KRef(k_datas, i+1, KAttributeType::LOW) < KRef(k_datas, i, KAttributeType::LOW) )//occur lower price
            {
                index_b = index_h;
                index_c = i + 1;
                return true;
            }else
                ++i; //left k contain right k
        }
    }
    return false;
}

// toward right [start, right_end_index]
// ps: if no inflection price also will return false
bool FindTrendDownIndexBCnotUseFractal(T_HisDataItemContainer &k_datas, int index_a, int right_end_index, OUT int &index_b, OUT int &index_c)
{
    if( index_a >= right_end_index ) 
        return false;
    //debug ------------
    if( index_a == 1595 )
        index_a = index_a;
    if( index_a == 1597 )
        index_a = index_a;

    int index_l = index_a;
    double price_l = KRef(k_datas, index_a, KAttributeType::LOW);

    int i = index_a; 
    int h_p_i = Highest(k_datas, i, right_end_index - i);
    // break up index a's high price
    if( KRef(k_datas, h_p_i, KAttributeType::HIGH) > KRef(k_datas, index_a, KAttributeType::HIGH) )
        return false;
    while( i < right_end_index)
    { 
        double next_i_l = KRef(k_datas, i+1, KAttributeType::LOW);
        if( next_i_l < KRef(k_datas, i, KAttributeType::LOW) ) // 低点降低
        {      
            if( KRef(k_datas, i+1, KAttributeType::HIGH) > KRef(k_datas, i, KAttributeType::HIGH) )// i +1 occur high price
            { 
                index_b = (i + 1 == right_end_index ? index_l : i + 1);
                index_c = i + 1;
                //debug ------
                if( index_a == 1597 )
                    index_a = index_a;
                //debug ------------
                if( index_a == 1595 )
                    index_a = index_a;
                //----------------
                return true;
            }
#if 1       
            else if( i + 1 == right_end_index
                && KRef(k_datas, i, KAttributeType::LOW) > price_l && KRef(k_datas, i, KAttributeType::CLOSE) > KRef(k_datas, i, KAttributeType::OPEN)
                && next_i_l < price_l ) // next index is last AND break the price_l
            {
                index_b = index_l;
                index_c = i + 1;
                //debug ------
                if( index_a == 1597 )
                    index_a = index_a;
                //----------------
                return true;
            } 
#endif
            if( next_i_l < price_l + EPSINON ) 
            {  // update low price and related index
                price_l = next_i_l; 
                index_l = i + 1;
            }
            ++i; 
        }else // 低点未继续降低
        {
            if( KRef(k_datas, i+1, KAttributeType::HIGH) > KRef(k_datas, i, KAttributeType::HIGH) )//occur higher price
            {
                index_b = index_l;
                index_c = i + 1;
                //debug ------
                if( index_a == 1597 )
                    index_a = index_a;
                //----------------
                return true;
            }else //left k contain right k
                ++i; 
        }
    }
    return false;
}

double TrendABspreadAllowMin(TypePeriod type_period)
{
    double ab_spread_min = 0.0;
    switch (type_period)
    {
    case TypePeriod::PERIOD_1M: ab_spread_min = 0.49;break;
    case TypePeriod::PERIOD_5M: ab_spread_min = 0.99;break;
    case TypePeriod::PERIOD_15M:ab_spread_min = 2.0;break;
    case TypePeriod::PERIOD_30M:ab_spread_min = 4.0;break;
    case TypePeriod::PERIOD_HOUR:ab_spread_min = 3.0;break;
    case TypePeriod::PERIOD_DAY: ab_spread_min = 5.0;break;
    default: assert(false);  break;
    }
    return ab_spread_min;
}

bool is_trend_ab_spread_allow(T_HisDataItemContainer &k_datas, TypePeriod type_period, bool is_down_forward, int index_a, int index_b)
{
    bool ret = false;
    double ab_spread_min = TrendABspreadAllowMin(type_period);
    if( is_down_forward )
        ret = fabs(KRef(k_datas, index_a, KAttributeType::HIGH) - KRef(k_datas, index_b, KAttributeType::LOW)) > ab_spread_min;
    else
        ret = fabs(KRef(k_datas, index_b, KAttributeType::HIGH) - KRef(k_datas, index_a, KAttributeType::LOW)) > ab_spread_min;
    return ret;
}

T_Data3pForcast * Fin3pForcast(Data3pForcastInnerContainer * up_forcasts, T_HisDataItemContainer &k_datas, int index_a, int index_b)
{
    assert(index_a > -1 && index_a < k_datas.size());
    assert(index_b > -1 && index_b < k_datas.size());
    if( up_forcasts )
    { 
        auto iter = std::find_if(up_forcasts->begin(), up_forcasts->end(), [index_a, index_b, &k_datas](Data3pForcastInnerContainer::reference in)
        {
            return in->date_a == k_datas[index_a]->stk_item.date && in->hhmm_a == k_datas[index_a]->stk_item.hhmmss 
                && in->date_b == k_datas[index_b]->stk_item.date && in->hhmm_b == k_datas[index_b]->stk_item.hhmmss;
        });
        if( iter != up_forcasts->end() )
            return  iter->get();
    }
    return nullptr;
}

std::string GetForcastContent(T_HisDataItemContainer &k_datas, Data3pForcastInnerContainer * up_forcasts)
{
    std::string ret;
    if( up_forcasts )
    {
    std::for_each(up_forcasts->begin(), up_forcasts->end(), [&ret, up_forcasts, &k_datas ](Data3pForcastInnerContainer::reference entry)
    {
        ret += "3pforcast";
        T_KlineDataItem item;
        item.stk_item.date = entry->date_a;  item.stk_item.hhmmss = entry->hhmm_a;
        int index_a = Lower_Bound(k_datas, 0, k_datas.size()-1, item);
        item.stk_item.date = entry->date_b;  item.stk_item.hhmmss = entry->hhmm_b;
        int index_b = Lower_Bound(k_datas, 0, k_datas.size()-1, item);

        ret += utility::FormatStr("\n id:%d a:04%d %04d(%d) b:%04d %04d(%d) "
            , entry->id, entry->date_a, entry->hhmm_a, index_a, entry->date_b,entry->hhmm_b, index_b);
    });
    }
    return ret;
}

// from small to big
bool dompare_abc_type( AbcIndexType &lh, AbcIndexType &rh)
{
    return (lh.date_a < rh.date_a) || (lh.date_a == rh.date_a && lh.hhmm_a < rh.hhmm_a);
}
void append_abc_index(T_HisDataItemContainer &k_datas, OUT std::vector<AbcIndexType> &abc_indexs, int index_a, int index_b, int index_c, int index_break)
{
    //debug----------
    if( index_a == 1597 )
        index_a = index_a;
    AbcIndexType abc;
    abc.index_a = index_a; abc.index_b = index_b; abc.index_c = index_c; 
    abc.index_break = index_break;
    abc.date_a = k_datas[index_a]->stk_item.date;
    abc.hhmm_a = k_datas[index_a]->stk_item.hhmmss;
    abc.date_b = k_datas[index_b]->stk_item.date;
    abc.hhmm_b = k_datas[index_b]->stk_item.hhmmss; 
    abc_indexs.push_back(abc);  
}

void KLineWall::TrendUpAutoForcast(TypePeriod type_period, T_HisDataItemContainer &k_datas, int left_end_index, int right_end_index)
{  
   // auto_forcast_man_.Remove3pForcastItems(stock_code_, type_period, false);
    // trend up forecast---------------
    assert(left_end_index <= right_end_index);
    int left_end_i = left_end_index;
    int latest_forcast_index = -1;
    bool small_range_flag = false;

    Data3pForcastInnerContainer * up_forcasts = auto_forcast_man_.Find3pForcastVector(stock_code_, type_period, false);
    //app_->local_logger().LogLocal("Debug", utility::FormatStr("begin %s", GetForcastContent(k_datas, up_forcasts).c_str()));

    auto  do_trendup_index_notusefractal = [up_forcasts, this](TypePeriod type_period,T_HisDataItemContainer &k_datas,int index_a,int  right_end_index
        ,OUT int &index_b, OUT int &index_c, OUT std::vector<AbcIndexType> &abc_indexs)->int
    {
        if( FindTrendUpIndexBCnotUseFractal(k_datas, index_a, right_end_index, index_b, index_c) )
        {
            if( is_trend_ab_spread_allow(k_datas, type_period, false, index_a, index_b) )
            { 
                double b_h = KRef(k_datas, index_b, KAttributeType::HIGH);
                int index_h = BarFirst(k_datas, index_b, right_end_index, KAttributeType::HIGH, CompareType::BIG, b_h);
                if( index_h > -1 ) // exists price > b's high, means breakup
                {
                    auto p_fct = auto_forcast_man_.Find3pForcast(stock_code_, type_period, false, *k_datas[index_a], *k_datas[index_b]);
#if 0
                    bool is_duplicate = false;
                    if( up_forcasts )
                    {
                        auto iter = std::find_if(up_forcasts->begin(), up_forcasts->end(), [index_a, index_b, &k_datas](Data3pForcastInnerContainer::reference in)
                        {
                            return in.date_a == k_datas[index_a]->stk_item.date && in.hhmm_a == k_datas[index_a]->stk_item.hhmmss 
                                && in.date_b == k_datas[index_b]->stk_item.date && in.hhmm_b == k_datas[index_b]->stk_item.hhmmss;
                        });
                        is_duplicate = iter != up_forcasts->end();
                    } 
#endif
                    if( !p_fct ) // not duplicate
                    { 
                       append_abc_index(k_datas, abc_indexs, index_a, index_b, index_c, index_h);
                       return index_a;
                    }else
                       return index_b;
                }
            }
        } 
        return -1;
    };
    if( up_forcasts )
    {
        std::vector<unsigned int> ids_need_remove;
        double price_a_max = 0.0;
        double price_b_min = 99999.9;
        std::for_each(up_forcasts->begin(), up_forcasts->end(), [right_end_index, &ids_need_remove, &k_datas, &price_a_max, &price_b_min](Data3pForcastInnerContainer::reference entry)
        {
            if( entry->price_a > price_a_max )
                price_a_max = entry->price_a;
            if( entry->price_b < price_b_min )
                price_b_min = entry->price_b;
            if( k_datas[right_end_index]->stk_item.low_price < entry->price_a )
            {
                entry->breaked_a = true;
                ids_need_remove.push_back(entry->id);
            }
            else if( k_datas[right_end_index]->stk_item.high_price > entry->d3 )
            {
                entry->break_d3_n = 1;
                ids_need_remove.push_back(entry->id);
            }
        });
        if( !up_forcasts->empty() && price_a_max < k_datas[right_end_index]->stk_item.low_price && k_datas[right_end_index]->stk_item.high_price < price_b_min )
        {
            small_range_flag = true;
            T_KlineDataItem item;
            item.stk_item.date = (*up_forcasts->rbegin())->date_b;
            item.stk_item.hhmmss = (*up_forcasts->rbegin())->hhmm_b;
            latest_forcast_index = Lower_Bound(k_datas, left_end_index, right_end_index, item);
            assert(latest_forcast_index > -1);
            left_end_i = latest_forcast_index + 1;
        }else
        {
            for( int i = 0; i < ids_need_remove.size(); ++i )
            {
                auto_forcast_man_.Erase3pForcast(stock_code_, type_period, false, ids_need_remove[i]);
                app_->local_logger().LogLocal(TAG_FORCAST_TREND_UP_LOG, utility::FormatStr("erase 3pforcast %d", ids_need_remove[i]));
            }
        }
    }
     
    app_->local_logger().LogLocal("Debug", utility::FormatStr("small_range_falg:%d left_end_i:%d left_end_index:%d",small_range_flag, left_end_i, left_end_index));
    bool try_left = !small_range_flag;
    int lowest_p_i = 0; // lowest price index

    const int e_try_distance = 6;
    lowest_p_i = Lowest(k_datas, right_end_index, left_end_i - right_end_index);
    if( try_left )// try left a distance till no lower price
    { 
        do 
        {
            if( lowest_p_i < 1 )
                break;
            int l_i = Lowest(k_datas, lowest_p_i - 1, -1 * e_try_distance);
            if( KRef(k_datas, l_i, KAttributeType::LOW) > KRef(k_datas, lowest_p_i, KAttributeType::LOW) )
                break;
            lowest_p_i = l_i; 
        } while (true);
    }
    assert(lowest_p_i <= right_end_index);
    
    //debug ----
    if( right_end_index == 183 )
        right_end_index = right_end_index;
    // load from old forecast. ps: not to set index
    std::vector<AbcIndexType> abc_indexs;
    if( up_forcasts && !up_forcasts->empty() )
    { 
        std::for_each(up_forcasts->cbegin(), up_forcasts->cend(), [&abc_indexs](Data3pForcastInnerContainer::const_reference in)
        {
            AbcIndexType abc_obj; 
            abc_obj.id = in->id;
            abc_obj.date_a = in->date_a; abc_obj.hhmm_a = in->hhmm_a;
            abc_obj.index_a = in->index_a;
            abc_obj.date_b = in->date_b; abc_obj.hhmm_b = in->hhmm_b;
            abc_obj.index_b = in->index_b;
            abc_obj.index_c = in->index_c;
            abc_indexs.push_back(abc_obj);
        }); 
        std::sort(abc_indexs.begin(), abc_indexs.end(), dompare_abc_type); 
    }
     
    int index_a = lowest_p_i;

    // --------------find all trend up---towards right-----------------------
    while( index_a < right_end_index )
    { 
        //find point B C---------------
        int index_b = -1;
        int index_c = -1;
        if( !IsBtmFractal(k_datas[index_a]->type) )
        {  
            int ret = do_trendup_index_notusefractal(type_period, k_datas, index_a, right_end_index, index_b, index_c, abc_indexs);
            if( ret > 0 )
            {
                //index_a = ret + 1;
                index_a = Lowest(k_datas, index_c, MAX_VAL(right_end_index - index_c, 0));
                continue;
            }
            //if( FindTrendUpIndexBCnotUseFractal(k_datas, index_a, right_end_index, index_b, index_c) )
            //{
            //    if( is_ab_spread_allow(k_datas, type_period, index_a, index_b) )
            //    { 
            //    double b_h = KRef(k_datas, index_b, KAttributeType::HIGH);
            //    int index_h = BarFirst(k_datas, index_b, right_end_index, KAttributeType::HIGH, CompareType::BIG, b_h);
            //    if( index_h > -1 ) // exists price > b's high, means breakup
            //    {
            //        AbcIndexType abc = std::make_tuple(index_a, index_b, index_c);
            //        abc_indexs.push_back(abc);
            //        index_a = Lowest(k_datas, index_c, MAX_VAL(right_end_index - index_c, 0));
            //        continue;
            //    }
            //    }
            //} 
        }else // A is btm fenxin
        {
            index_b = FractalFirst(k_datas, FractalGeneralType::TOP_FRACTAL, index_a, right_end_index);
            if( index_b > -1 )
            {
                bool allow = is_trend_ab_spread_allow(k_datas, type_period, false, index_a, index_b);
                if( !allow ) // try once again from index_b+1
                {
                    index_b = FractalFirst(k_datas, FractalGeneralType::TOP_FRACTAL, index_b+1, right_end_index);
                    if( index_b > -1 )
                        allow = is_trend_ab_spread_allow(k_datas, type_period, false, index_a, index_b);
                }
                if( allow )
                { 
                    double b_h = KRef(k_datas, index_b, KAttributeType::HIGH);
                    int index_h = BarFirst(k_datas, index_b, right_end_index, KAttributeType::HIGH, CompareType::BIG, b_h);
                    if( index_h > -1 ) // exists price > b's high ,means breakup
                    {
                        index_c = index_b < right_end_index ?  index_b + 1 : index_b;
                        bool is_duplicate = false;
                        if( up_forcasts )
                        {
                            auto p_3pf = auto_forcast_man_.Find3pForcast(stock_code_, k_type_, false, *k_datas[index_a], *k_datas[index_b]);
                            is_duplicate = p_3pf;
                        } 
                        if( !is_duplicate )
                            append_abc_index(k_datas, abc_indexs, index_a, index_b, index_c, index_h); 
                        index_a = Lowest(k_datas, index_c, MAX_VAL(right_end_index - index_c, 0));
                        continue;
                    }else
                    {
                        int ret = do_trendup_index_notusefractal(type_period, k_datas, index_a, right_end_index, index_b, index_c, abc_indexs);
                        if( ret > 0 )
                        {
                            index_a = Lowest(k_datas, index_c, MAX_VAL(right_end_index - index_c, 0));
                            continue;
                        }
                    }
                }
            }else // not exists TOP_FRACTAL point B
            {
                int ret = do_trendup_index_notusefractal(type_period, k_datas, index_a, right_end_index, index_b, index_c, abc_indexs);
                if( ret > 0 )
                {
                    index_a = Lowest(k_datas, index_c, MAX_VAL(right_end_index - index_c, 0));
                    continue;
                }
            }
        }

        index_a = Lowest(k_datas, index_a + 1, MAX_VAL(right_end_index - (index_a + 1), 0));
    }// while
    
   std::sort(abc_indexs.begin(), abc_indexs.end(), dompare_abc_type); 
    // create 3p up forcast ----------- 
#if 0
   for( int i = 0; i < (int)abc_indexs.size(); ++i )
   {
       if( abc_indexs[i].index_a == 887 )
           break;
   }
#endif
   // procedure which a_index'low price nearby(other condition distance and b's price)
    const int distance_index_5m = 12;
    const double distance_price_5m = 0.5;
    for( int p = 0; p < (int)abc_indexs.size() - 1; ++p )
    {
        if( abc_indexs[p].index_a < 0 ) // has filter
            continue;
        double p_a_low_price = 0.0;
        int p_a_index = abc_indexs[p].index_a;
        int p_b_index = abc_indexs[p].index_b;
        if( abc_indexs[p].id > 0 ) // old, already in up_forcasts
        {
            auto p_3pf = auto_forcast_man_.Find3pForcast(stock_code_, k_type_, false, abc_indexs[p].id);
            if( p_3pf )
            {
                p_a_low_price = p_3pf->price_a;
                //p_a_index = FindDateHHmmRelIndex(k_datas, left_end_index, right_end_index, abc_indexs[p].date_a, abc_indexs[p].hhmm_a);
               
            }
            else 
                continue;
        }else
            p_a_low_price = KRef(k_datas, abc_indexs[p].index_a, KAttributeType::LOW);
        //int b_index_0 = abc_indexs[p].index_b;
        assert( p_a_index > -1);
        assert( p_b_index > -1);
        for( int q = p+1; q < abc_indexs.size(); ++q )
        {
            if( abc_indexs[q].index_a < 0 )
                continue;
            double a_1_low_price = KRef(k_datas, abc_indexs[q].index_a, KAttributeType::LOW);
            
            if( abs(abc_indexs[q].index_a - p_a_index) < distance_index_5m 
                //&& fabs(KRef(k_datas, abc_indexs[q].index_a, KAttributeType::LOW) - KRef(k_datas, p_a_index, KAttributeType::LOW)) < distance_price_5m 
                && JudgeTrendPriceSpreadType(type_period, a_1_low_price - p_a_low_price) <= PriceSpreadType::MINI
                && KRef(k_datas, abc_indexs[q].index_b, KAttributeType::HIGH) > KRef(k_datas, p_b_index, KAttributeType::HIGH) )
            { 
                if( !(p_a_low_price > a_1_low_price) ) // p's a low <= q's a low
                {
                    auto p_f = Fin3pForcast(up_forcasts, k_datas, p_a_index, p_b_index);
                    if( p_f ) //del p's old forecast in order to create new p's forecast
                        auto_forcast_man_.Erase3pForcast(stock_code_, k_type_, false, p_f->id);
                    abc_indexs[p].index_b = abc_indexs[q].index_b;
                    abc_indexs[p].index_c = abc_indexs[q].index_c;
                    if( abc_indexs[q].id > 0 ) //del q
                        auto_forcast_man_.Erase3pForcast(stock_code_, k_type_, false, abc_indexs[q].id);
                    // filter q
                    abc_indexs[q].index_a = abc_indexs[q].index_b = abc_indexs[q].index_c = -1;
                    
                }else
                { 
                } 
            }
        }
    } 

    for( int i = 0; i < abc_indexs.size(); ++i )
    {
        if( abc_indexs[i].index_a < 0 )
            continue;
        if( auto_forcast_man_.Find3pForcast(stock_code_, type_period, false, *k_datas[abc_indexs[i].index_a], *k_datas[abc_indexs[i].index_b]) )
            continue;
        auto d1_d2_d3 = ForcastD_ABC_Up(k_datas[abc_indexs[i].index_a]->stk_item.low_price, k_datas[abc_indexs[i].index_b]->stk_item.high_price);
        int p_i = Highest(k_datas, abc_indexs[i].index_b, MAX_VAL(right_end_index - abc_indexs[i].index_b, 0));
        if( KRef(k_datas, p_i, KAttributeType::HIGH) > std::get<2>(d1_d2_d3) )
            continue;
        ForecastPara para;
        para.id = app_->GenerateForecastId();
        para.is_ab_down = false;
        para.index_a = abc_indexs[i].index_a;
        para.item_a = k_datas[para.index_a]->stk_item;
        para.index_b = abc_indexs[i].index_b;
        para.item_b = k_datas[para.index_b]->stk_item;
       
        para.index_c = abc_indexs[i].index_c;
         if( para.index_c < 0 )
             para.index_c = para.index_c;

        para.item_c = k_datas[para.index_c]->stk_item;
        para.index_break = abc_indexs[i].index_break;
        append_forcast_d(stock_code_, type_period, para, auto_forcast_man_);
        //app_->local_logger().LogLocal("Debug", utility::FormatStr("\n append 3pforcast id:%d a:04%d %04d(%d) b:%04d %04d(%d) "
        //     , para.id, k_datas[abc_indexs[i].index_a]->stk_item.date, k_datas[abc_indexs[i].index_a]->stk_item.hhmmss, abc_indexs[i].index_a
        //    , k_datas[abc_indexs[i].index_b]->stk_item.date, k_datas[abc_indexs[i].index_b]->stk_item.hhmmss, abc_indexs[i].index_b));
    }
    
}


void KLineWall::TrendDownAutoForcast(TypePeriod type_period, T_HisDataItemContainer &k_datas, int left_end_index, int right_end_index)
{ 
    // auto_forcast_man_.Remove3pForcastItems(stock_code_, type_period, false);
    // trend down forecast---------------
    assert(left_end_index <= right_end_index);
    int left_end_i = left_end_index;
    int latest_forcast_index = -1;
    bool small_range_flag = false;

    Data3pForcastInnerContainer * old_down_forcasts = auto_forcast_man_.Find3pForcastVector(stock_code_, type_period, true);
    //app_->local_logger().LogLocal("Debug", utility::FormatStr("down forecast begin %s", GetForcastContent(k_datas, down_forcasts).c_str()));

    auto  do_trenddown_index_notusefractal = [/*old_down_forcasts*/ this](TypePeriod type_period,T_HisDataItemContainer &k_datas,int index_a,int  right_end_index
        ,OUT int &index_b, OUT int &index_c, OUT std::vector<AbcIndexType> &abc_indexs)->int
    {
        if( FindTrendDownIndexBCnotUseFractal(k_datas, index_a, right_end_index, index_b, index_c) )
        {
            if( is_trend_ab_spread_allow(k_datas, type_period, true, index_a, index_b) )
            { 
                double b_l = KRef(k_datas, index_b, KAttributeType::LOW);
                int index_l = BarFirst(k_datas, index_b, right_end_index, KAttributeType::LOW, CompareType::SMALL, b_l);
                if( index_l > -1 ) // exists price < b's low, means breakdown
                {
                    auto p_fct = auto_forcast_man_.Find3pForcast(stock_code_, type_period, true, *k_datas[index_a], *k_datas[index_b]);
                    if( !p_fct ) // not duplicate
                    { 
                        append_abc_index(k_datas, abc_indexs, index_a, index_b, index_c, index_l);
                        return index_a;
                    }else
                        return index_b;
                }
            }
        } 
        return -1;
    };
    if( old_down_forcasts )
    {
        if( old_down_forcasts->size() > 50 ) // erase old forecast which before 5 trading days
        { 
            srand(time(0));
            if( rand() % 3 == 0 )
            {
                for( auto iter = old_down_forcasts->begin(); iter != old_down_forcasts->end(); )
                {
                    if(  (*iter)->index_a + (type_period <= TypePeriod::PERIOD_5M ? 156 : 156/6) * 5 < right_end_index )
                        old_down_forcasts->erase(iter++);
                    else 
                        ++iter;
                }
            }
        }
        std::vector<unsigned int> ids_need_remove;
        double price_a_min = 99999.9;
        double price_b_max = 0.0;
        //std::for_each(old_down_forcasts->begin(), old_down_forcasts->end(), [right_end_index, &ids_need_remove, &k_datas, &price_a_min, &price_b_max](Data3pForcastInnerContainer::reference entry)
        for( auto iter = old_down_forcasts->begin(); iter != old_down_forcasts->end(); ++iter )
        {
            std::shared_ptr<T_Data3pForcast> &entry = *iter;
            if( entry->price_a < price_a_min )
                price_a_min = entry->price_a;
            if( entry->price_b > price_b_max )
                price_b_max = entry->price_b;
             
#if 1
            if( k_datas[right_end_index]->stk_item.high_price > entry->price_a )
            {
                entry->breaked_a = true;
                ids_need_remove.push_back(entry->id);
            }else if( entry->break_d3_index > -1 )
            {
                if( entry->price_spread_type < PriceSpreadType::SMALL )
                    continue;
                if( right_end_index <= entry->break_d3_index )
                    continue;
                // create new forecast --------------------------------------------
                int h_i = Highest(k_datas, entry->index_b, entry->break_d3_index - entry->index_b);
                if( auto_forcast_man_.Find3pForcast(stock_code_, type_period, true, *k_datas[h_i], *k_datas[entry->break_d3_index]) )
                    continue;
                ForecastPara para;
                para.id = app_->GenerateForecastId();
                para.is_ab_down = true;
                para.index_a = h_i;
                para.item_a = k_datas[h_i]->stk_item;
                para.index_b = entry->break_d3_index;
                para.item_b = k_datas[para.index_b]->stk_item;
                para.index_c = para.index_b;
                para.item_c = k_datas[para.index_c]->stk_item;
                para.index_break = entry->break_d3_index;
                append_forcast_d(stock_code_, type_period, para, auto_forcast_man_);
                entry->break_d3_index = -2;
                //-------------------------------------------

            }else if( k_datas[right_end_index]->stk_item.low_price < entry->d3 ) 
            {
                if( k_datas[right_end_index]->stk_item.low_price < entry->d3 - 1.0 )// -1.0 for delay del . need check
                {
                    entry->break_d3_n = 1;
                    ids_need_remove.push_back(entry->id);
                }
                if( entry->break_d3_index == -1 )
                    entry->break_d3_index = right_end_index;
            }
#endif
        }

        if( !old_down_forcasts->empty() && price_b_max < k_datas[right_end_index]->stk_item.low_price && k_datas[right_end_index]->stk_item.high_price < price_a_min )
        {
            small_range_flag = true;
            T_KlineDataItem item;
            item.stk_item.date = (*old_down_forcasts->rbegin())->date_b;
            item.stk_item.hhmmss = (*old_down_forcasts->rbegin())->hhmm_b;
            latest_forcast_index = Lower_Bound(k_datas, left_end_index, right_end_index, item);
            assert(latest_forcast_index > -1);
            left_end_i = latest_forcast_index + 1;
        }else
        {
#if 0
            for( int i = 0; i < ids_need_remove.size(); ++i )
            {
                auto_forcast_man_.Erase3pForcast(stock_code_, type_period, true, ids_need_remove[i]);
                app_->local_logger().LogLocal(TAG_FORCAST_TREND_DOWN_LOG, utility::FormatStr("erase 3pforcastdown %d", ids_need_remove[i]));
            }
#endif
        }
    }

    app_->local_logger().LogLocal("Debug", utility::FormatStr("small_range_falg:%d left_end_i:%d left_end_index:%d",small_range_flag, left_end_i, left_end_index));
    bool try_left = !small_range_flag;

    const int e_try_distance = 6;
    int highest_p_i = Highest(k_datas, right_end_index, left_end_i - right_end_index);
    if( try_left )// try left a distance till no higher price
    { 
        do 
        {
            if( highest_p_i < 1 )
                break;
            int h_i = Highest(k_datas, highest_p_i - 1, -1 * e_try_distance);
            if( KRef(k_datas, h_i, KAttributeType::HIGH) < KRef(k_datas, highest_p_i, KAttributeType::HIGH) )
                break;
            highest_p_i = h_i; 
        } while (true);
    }
    assert(highest_p_i <= right_end_index);

    std::vector<AbcIndexType> abc_indexs;
    if( old_down_forcasts && !old_down_forcasts->empty() ) //load from old forecast
    { 
        std::for_each(old_down_forcasts->cbegin(), old_down_forcasts->cend(), [&k_datas, right_end_index, &abc_indexs](Data3pForcastInnerContainer::const_reference in)
        {
            if( in->breaked_a )
                return; 
            if( k_datas[right_end_index]->stk_item.low_price < in->d3 ) // break d3
                return;
            if( in->break_d3_index == -2 )
                return;
            AbcIndexType abc_obj; 
            abc_obj.id = in->id;
            abc_obj.date_a = in->date_a; abc_obj.hhmm_a = in->hhmm_a;
            abc_obj.index_a = in->index_a;
            abc_obj.date_b = in->date_b; abc_obj.hhmm_b = in->hhmm_b;
            abc_obj.index_b = in->index_b;
            abc_obj.index_c = in->index_c;
            abc_indexs.push_back(abc_obj);
        }); 
        std::sort(abc_indexs.begin(), abc_indexs.end(), dompare_abc_type); 
    }
     
    int index_a = highest_p_i;

    // --------------find all trend down---towards right-----------------------
    while( index_a < right_end_index )
    { 
        //find point B C---------------
        int index_b = -1;
        int index_c = -1;
        //debug ------------
        if( index_a == 1595 )
            index_a = index_a;
        if( !IsTopFractal(k_datas[index_a]->type) )
        {  
            int ret = do_trenddown_index_notusefractal(type_period, k_datas, index_a, right_end_index, index_b, index_c, abc_indexs);
            if( ret > 0 )
            { 
                index_a = Highest(k_datas, index_c, MAX_VAL(right_end_index - index_c, 0));
                continue;
            } 
        }else // A is top fenxin
        {
            index_b = FractalFirst(k_datas, FractalGeneralType::BTM_FRACTAL, index_a, right_end_index);
            if( index_b > -1 )
            {
                bool allow = is_trend_ab_spread_allow(k_datas, type_period, true, index_a, index_b);
                if( !allow ) // try once again from index_b+1
                {
                    index_b = FractalFirst(k_datas, FractalGeneralType::BTM_FRACTAL, index_b+1, right_end_index);
                    if( index_b > -1 )
                        allow = is_trend_ab_spread_allow(k_datas, type_period, true, index_a, index_b);
                }
                if( allow )
                { 
                    double b_l = KRef(k_datas, index_b, KAttributeType::LOW);
                    int index_l = BarFirst(k_datas, index_b, right_end_index, KAttributeType::LOW, CompareType::SMALL, b_l);
                    if( index_l > -1 ) // exists price > b's low ,means breakdown
                    {
                        index_c = index_b < right_end_index ?  index_b + 1 : index_b;
                        bool is_duplicate = false;
                        if( old_down_forcasts )
                        {
                            auto p_3pf = auto_forcast_man_.Find3pForcast(stock_code_, k_type_, true, *k_datas[index_a], *k_datas[index_b]);
                            is_duplicate = p_3pf;
                        } 
                        if( !is_duplicate )
                            append_abc_index(k_datas, abc_indexs, index_a, index_b, index_c, index_l); 
                        index_a = Highest(k_datas, index_c, MAX_VAL(right_end_index - index_c, 0));
                        continue;
                    }else
                    {
                        int ret = do_trenddown_index_notusefractal(type_period, k_datas, index_a, right_end_index, index_b, index_c, abc_indexs);
                        if( ret > 0 )
                        {
                            index_a = Highest(k_datas, index_c, MAX_VAL(right_end_index - index_c, 0));
                            continue;
                        }
                    }
                }
            }else // not exists BTM_FRACTAL point B
            {
                int ret = do_trenddown_index_notusefractal(type_period, k_datas, index_a, right_end_index, index_b, index_c, abc_indexs);
                if( ret > 0 )
                {
                    index_a = Highest(k_datas, index_c, MAX_VAL(right_end_index - index_c, 0));
                    continue;
                }
            }
        }

        index_a = Highest(k_datas, index_a + 1, MAX_VAL(right_end_index - (index_a + 1), 0));
    }// while

    std::sort(abc_indexs.begin(), abc_indexs.end(), dompare_abc_type); 
 
    // procedure which a_index' high price nearby(other condition:distance and b's low price)----------
    const int distance_index_5m = 12;
    const double distance_price_5m = 0.5;
    for( int p = 0; p < (int)abc_indexs.size() - 1; ++p )
    {
        if( abc_indexs[p].index_a < 0 ) // has filter
            continue;
        double p_a_high_price = 0.0;
        int p_a_index = abc_indexs[p].index_a;
        int p_b_index = abc_indexs[p].index_b;
        if( abc_indexs[p].id > 0 ) // old, already in down_forcasts
        {
            auto p_3pf = auto_forcast_man_.Find3pForcast(stock_code_, k_type_, true, abc_indexs[p].id);
            if( p_3pf )
            {
                p_a_high_price = p_3pf->price_a;
            }
            else 
                continue;
        }else
            p_a_high_price = KRef(k_datas, abc_indexs[p].index_a, KAttributeType::HIGH);
        assert( p_a_index > -1);
        assert( p_b_index > -1);
        //int b_index_0 = abc_indexs[p].index_b;
        for( int q = p+1; q < abc_indexs.size(); ++q )
        { 
            if( abc_indexs[q].index_a < 0 ) // has filter
                continue;
            double a_q_high_price = KRef(k_datas, abc_indexs[q].index_a, KAttributeType::HIGH);
           
            if( abs(abc_indexs[q].index_a - p_a_index) < distance_index_5m 
                    && JudgeTrendPriceSpreadType(type_period, a_q_high_price - p_a_high_price) <= PriceSpreadType::MINI
                    && KRef(k_datas, abc_indexs[q].index_b, KAttributeType::LOW) < KRef(k_datas, p_b_index, KAttributeType::LOW) )
            {  
                if( !(p_a_high_price < a_q_high_price) ) // p's a high >= q's a high
                {
                    auto p_f = Fin3pForcast(old_down_forcasts, k_datas, p_a_index, p_b_index);
                    if( p_f )//del p's forecast in order to create new p's forecast
                        auto_forcast_man_.Erase3pForcast(stock_code_, k_type_, true, p_f->id);
                    abc_indexs[p].index_b = abc_indexs[q].index_b;
                    abc_indexs[p].index_c = abc_indexs[q].index_c;
                    if( abc_indexs[q].id > 0 ) // del q
                        auto_forcast_man_.Erase3pForcast(stock_code_, k_type_, true, abc_indexs[q].id);
                    // filter q
                    abc_indexs[q].index_a = abc_indexs[q].index_b = abc_indexs[q].index_c = -1;
                }else
                { 
                }
            }
        }
    } 

    for( int i = 0; i < abc_indexs.size(); ++i )
    {
        if( abc_indexs[i].index_a < 0 )
            continue;
        if( abc_indexs[i].id > 0 )
            continue;
        // it's necessary cause TrendDownAutoForcast called frequency
        if( auto_forcast_man_.Find3pForcast(stock_code_, type_period, true, *k_datas[abc_indexs[i].index_a], *k_datas[abc_indexs[i].index_b]) )
            continue;
        auto d1_d2_d3 = ForcastD_ABC_Down(k_datas[abc_indexs[i].index_a]->stk_item.high_price, k_datas[abc_indexs[i].index_b]->stk_item.low_price);
        int p_i = Lowest(k_datas, abc_indexs[i].index_b, MAX_VAL(right_end_index - abc_indexs[i].index_b, 0));
        if( KRef(k_datas, p_i, KAttributeType::LOW) < std::get<2>(d1_d2_d3) )
            continue;
        ForecastPara para;
        para.id = app_->GenerateForecastId();
        para.is_ab_down = true;
        para.index_a = abc_indexs[i].index_a;
        para.item_a = k_datas[para.index_a]->stk_item;
        para.index_b = abc_indexs[i].index_b;
        para.item_b = k_datas[para.index_b]->stk_item;
        para.index_c = abc_indexs[i].index_c;
        para.item_c = k_datas[para.index_c]->stk_item;
        para.index_break = abc_indexs[i].index_break;
        append_forcast_d(stock_code_, type_period, para, auto_forcast_man_);
        //app_->local_logger().LogLocal("Debug", utility::FormatStr("\n append 3pforcastdown id:%d a:04%d %04d(%d) b:%04d %04d(%d) "
        //    , para.id, k_datas[abc_indexs[i].index_a]->stk_item.date, k_datas[abc_indexs[i].index_a]->stk_item.hhmmss, abc_indexs[i].index_a
        //    , k_datas[abc_indexs[i].index_b]->stk_item.date, k_datas[abc_indexs[i].index_b]->stk_item.hhmmss, abc_indexs[i].index_b));
    }

}

