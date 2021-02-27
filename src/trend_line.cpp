#include "trend_line.h"

#include <cfloat>

#include <Tlib/core/tsystem_core_common.h>

#include "futures_forecast_app.h"
#include "exchange_calendar.h"

#define  IS_DEL_UNLIVE_LINE 0
#define  TAG_TREND_LINE_LOG  "trendline"

const static double cst_up_line_slop_min = 0.015;

using namespace TSystem;
void TrendLine::SetDead()
{ 
    is_alive_ = false;
}
TrendLine * TrendLineMan::CreateTrendUpLine(const std::string &code, TypePeriod type_period, TrendLine *p_last_trend_up_line)
{ 
    //TrendLine * p_ret_line = nullptr;
    T_HisDataItemContainer &k_datas = app_.stock_data_man().GetHisDataContainer(type_period, code);
    if( k_datas.size() < 2 )
        return nullptr;

    int cst_cur_index = k_datas.size() - 1;

    int left_end_index = GetLeftEndIndex(code, type_period);

    
    int lowest_index = Lowest(k_datas, cst_cur_index, left_end_index - cst_cur_index);
    int highest_index = Highest(k_datas, cst_cur_index, left_end_index - cst_cur_index);
    if( left_end_index >= highest_index )
        return nullptr;
#if 1
    return __CreateTrendUpLine(code, type_period, left_end_index, highest_index, false, p_last_trend_up_line);
#else 
    if( lowest_index == highest_index )
        return nullptr;

    const double mid_axis_slop = CaculateMidAxisSlop(code, type_period);
    if( lowest_index > highest_index )
    { 
        auto trend_down_line = std::make_shared<TrendLine>(TrendLineType::DOWN);
        // todo:
        return trend_down_line.get();
    }else
    {
        auto trend_up_line = std::make_shared<TrendLine>(TrendLineType::UP);
        /****************
                b
               /|
              / |
             /  |
          a /___|c
        ***************/
#if 1 
        double first_slop = 0.0;
        int first_low_end_i = SolveTrendUpLineEndIndex(k_datas, lowest_index, highest_index, first_slop);
        if( first_low_end_i < 0 ) // fail
            return nullptr;
        //----------find first_low_end_i which slop > 0.01 -----------
        int temp_low_index = lowest_index;
        while( first_slop < cst_up_line_slop_min && temp_low_index < highest_index )
        {
            auto btms = FindFractalsBetween(k_datas, FractalGeneralType::BTM_FRACTAL, highest_index, temp_low_index+1);
            int lowest_i_in_btms = 0;
            double min_price_in_btms = MAX_PRICE;
            for( unsigned int i = 0; i < btms.size(); ++i )
            {
                auto price = KRef(k_datas, btms[i], KAttributeType::LOW);
                if( price < min_price_in_btms + EPSINON )
                {
                    lowest_i_in_btms = btms[i];
                    min_price_in_btms = price;
                }
            }
            if( lowest_i_in_btms > 0 )
            { 
                first_low_end_i = SolveTrendUpLineEndIndex(k_datas, lowest_i_in_btms, highest_index, first_slop);
                if( first_low_end_i > -1 && first_slop > cst_up_line_slop_min - EPSINON )
                {
                    lowest_index = lowest_i_in_btms;
                    break;
                }else
                {
                    temp_low_index = lowest_i_in_btms;
                }
            }else
                return nullptr;
        }//while 
        assert(first_low_end_i > lowest_index);
        trend_up_line->slope_ = first_slop;
        trend_up_line->beg_ = lowest_index;
        trend_up_line->end_ = first_low_end_i;
        trend_up_line->h_price_index_ = highest_index;
        trend_up_line->l_price_index_ = lowest_index;
        // consider second low i :考虑次低点
        do
        {
            if( mid_axis_slop < cst_up_line_slop_min )
                break; 
            auto btms = FindFractalsBetween(k_datas, FractalGeneralType::BTM_FRACTAL, highest_index, lowest_index+1);
            int lowest_i_in_btms = 0;
            double min_price_in_btms = MAX_PRICE;
            for( unsigned int i = 0; i < btms.size(); ++i )
            {
                auto price = KRef(k_datas, btms[i], KAttributeType::LOW);
                if( price < min_price_in_btms + EPSINON )
                {
                    lowest_i_in_btms = btms[i];
                    min_price_in_btms = price;
                }
            }
            if( lowest_i_in_btms <= 0 )
                break;
            double second_slot = 0.0;
            int second_low_end_i = SolveTrendUpLineEndIndex(k_datas, lowest_i_in_btms, highest_index, second_slot);
            if( second_low_end_i < 0 )
                break;
            if( fabs(first_slop - mid_axis_slop) < fabs(second_slot - mid_axis_slop) + 0.005 )
                break;

            auto line_use_second_low = std::make_shared<TrendLine>(TrendLineType::UP);
            line_use_second_low->slope_ = second_slot;
            line_use_second_low->beg_ = lowest_i_in_btms;
            line_use_second_low->end_ = second_low_end_i;
            line_use_second_low->h_price_index_ = highest_index;
            line_use_second_low->l_price_index_ = lowest_i_in_btms;

            double distance = fabs(TrendLineValue(k_datas, *trend_up_line, cst_cur_index) - TrendLineValue(k_datas, *line_use_second_low, cst_cur_index));
            if( distance < 0.5 )
                break;
             
            if( p_last_trend_up_line && p_last_trend_up_line->beg_ == lowest_i_in_btms && p_last_trend_up_line->end_ == second_low_end_i )
            {
                p_last_trend_up_line->h_price_index_ = highest_index;
                p_last_trend_up_line->is_double_top_ = false;
                return p_last_trend_up_line;
            }else 
            {
                auto p_line = AppendTrendLine(code, type_period, line_use_second_low, true);
                return p_line;
            } 
        }while(0);// consider second low 

        if( p_last_trend_up_line && p_last_trend_up_line->beg_ == lowest_index && p_last_trend_up_line->end_ == first_low_end_i )
        {
            p_last_trend_up_line->h_price_index_ = highest_index;
            p_last_trend_up_line->is_double_top_ = false;
            return p_last_trend_up_line;
        }else
        { 
            auto p_line = AppendTrendLine(code, type_period, trend_up_line);
            return p_line;
        }
         
#endif
    } // trend up line
     
    return nullptr;
#endif
}

TrendLine * TrendLineMan::__CreateTrendUpLine(const std::string &code, TypePeriod type_period, int left_index, int right_index, bool is_db_top, TrendLine *p_last_trend_up_line)
{
    T_HisDataItemContainer &k_datas = app_.stock_data_man().GetHisDataContainer(type_period, code);
    if( k_datas.size() < 2 )
        return nullptr;

    int cst_cur_index = k_datas.size() - 1;

    assert(left_index < right_index); 

    int lowest_index = Lowest(k_datas, cst_cur_index, left_index - cst_cur_index);
    int highest_index = Highest(k_datas, cst_cur_index, left_index - cst_cur_index);

    if( lowest_index >= right_index )
        return nullptr;

    const double mid_axis_slop = CaculateMidAxisSlop(code, type_period);
     
    auto trend_up_line = std::make_shared<TrendLine>(TrendLineType::UP);
    /****************
            b
           /|
          / |
         /  |
      a /___|c
    ***************/
#if 1 
    double first_slop = 0.0;
    int first_low_end_i = SolveTrendUpLineEndIndex(k_datas, lowest_index, right_index, first_slop);
    if( first_low_end_i < 0 ) // fail
        return nullptr;
    //----------find first_low_end_i which slop > 0.01 -----------
    int temp_low_index = lowest_index;
    while( first_slop < cst_up_line_slop_min && temp_low_index < right_index )
    {
        auto btms = FindFractalsBetween(k_datas, FractalGeneralType::BTM_FRACTAL, right_index, temp_low_index+1);
        int lowest_i_in_btms = 0;
        double min_price_in_btms = MAX_PRICE;
        for( unsigned int i = 0; i < btms.size(); ++i )
        {
            auto price = KRef(k_datas, btms[i], KAttributeType::LOW);
            if( price < min_price_in_btms + EPSINON )
            {
                lowest_i_in_btms = btms[i];
                min_price_in_btms = price;
            }
        }
        if( lowest_i_in_btms > 0 )
        { 
            first_low_end_i = SolveTrendUpLineEndIndex(k_datas, lowest_i_in_btms, right_index, first_slop);
            if( first_low_end_i > -1 && first_slop > cst_up_line_slop_min - EPSINON )
            {
                lowest_index = lowest_i_in_btms;
                break;
            }else
            {
                temp_low_index = lowest_i_in_btms;
            }
        }else
            return nullptr;
    }//while 
    assert(first_low_end_i > lowest_index);
    trend_up_line->slope_ = first_slop;
    trend_up_line->beg_ = lowest_index;
    trend_up_line->end_ = first_low_end_i;
    trend_up_line->h_price_index_ = highest_index;
    trend_up_line->l_price_index_ = lowest_index;
    trend_up_line->is_double_top_ = is_db_top;
    // consider second low i :考虑次低点
    do
    {
        if( mid_axis_slop < cst_up_line_slop_min )
            break; 
        auto btms = FindFractalsBetween(k_datas, FractalGeneralType::BTM_FRACTAL, right_index, lowest_index+1);
        int lowest_i_in_btms = 0;
        double min_price_in_btms = MAX_PRICE;
        for( unsigned int i = 0; i < btms.size(); ++i )
        {
            auto price = KRef(k_datas, btms[i], KAttributeType::LOW);
            if( price < min_price_in_btms + EPSINON )
            {
                lowest_i_in_btms = btms[i];
                min_price_in_btms = price;
            }
        }
        if( lowest_i_in_btms <= 0 )
            break;
        double second_slot = 0.0;
        int second_low_end_i = SolveTrendUpLineEndIndex(k_datas, lowest_i_in_btms, right_index, second_slot);
        if( second_low_end_i < 0 )
            break;
        if( fabs(first_slop - mid_axis_slop) < fabs(second_slot - mid_axis_slop) + 0.005 )
            break;

        auto line_use_second_low = std::make_shared<TrendLine>(TrendLineType::UP);
        line_use_second_low->slope_ = second_slot;
        line_use_second_low->beg_ = lowest_i_in_btms;
        line_use_second_low->end_ = second_low_end_i;
        line_use_second_low->h_price_index_ = highest_index;
        line_use_second_low->l_price_index_ = lowest_i_in_btms;
        line_use_second_low->is_double_top_ = is_db_top;

        double distance = fabs(TrendLineValue(k_datas, *trend_up_line, cst_cur_index) - TrendLineValue(k_datas, *line_use_second_low, cst_cur_index));
        if( distance < 0.5 )
            break;
             
        if( p_last_trend_up_line && p_last_trend_up_line->beg_ == lowest_i_in_btms && p_last_trend_up_line->end_ == second_low_end_i )
        {
            p_last_trend_up_line->h_price_index_ = highest_index;
            p_last_trend_up_line->is_double_top_ = is_db_top;
            return p_last_trend_up_line;
        }else 
        {
            auto p_line = AppendTrendLine(code, type_period, line_use_second_low, true);
            assert(p_line);
            if( is_db_top )
                p_line->is_double_top_ = is_db_top;
            return p_line;
        } 
    }while(0);// consider second low 

    if( p_last_trend_up_line && p_last_trend_up_line->beg_ == lowest_index && p_last_trend_up_line->end_ == first_low_end_i )
    {
        p_last_trend_up_line->h_price_index_ = highest_index;
        p_last_trend_up_line->is_double_top_ = is_db_top;
        return p_last_trend_up_line;
    }else
    {  
        auto p_line = AppendTrendLine(code, type_period, trend_up_line);
        assert(p_line);
        if( is_db_top )
            p_line->is_double_top_ = is_db_top;
        return p_line;
    }
#endif
    return nullptr;
}

TrendLine * TrendLineMan::CreateTrendUpLineByDoubleTop(const std::string &code, TypePeriod type_period, int left_index, int right_index)
{ 
#if 1
    return __CreateTrendUpLine(code, type_period, left_index, right_index, true);
#else
    T_HisDataItemContainer &k_datas = app_.stock_data_man().GetHisDataContainer(type_period, code);
    if( k_datas.size() < 2 )
        return nullptr;
    assert(left_index < right_index);
    int cst_cur_index = k_datas.size() - 1;
     
    int lowest_index = Lowest(k_datas, cst_cur_index, left_index - cst_cur_index);
    int highest_index = Highest(k_datas, cst_cur_index, left_index - cst_cur_index);
 
    if( lowest_index >= right_index )
        return nullptr;

    const double mid_axis_slop = CaculateMidAxisSlop(code, type_period);
       
    auto trend_up_line = std::make_shared<TrendLine>(TrendLineType::UP);
         
        /****************
                b
               /|
              / |
             /  |
          a /___|c
        ***************/ 
    double first_slop = 0.0;
    int first_low_end_i = SolveTrendUpLineEndIndex(k_datas, lowest_index, right_index, first_slop);
    if( first_low_end_i < 0 ) // fail
        return nullptr;
    //----------find first_low_end_i which slop > 0.01 -----------
    int temp_low_index = lowest_index;
    while( first_slop < 0.01 && temp_low_index < right_index )
    {
        auto btms = FindFractalsBetween(k_datas, FractalGeneralType::BTM_FRACTAL, right_index, temp_low_index+1);
        int lowest_i_in_btms = 0;
        double min_price_in_btms = MAX_PRICE;
        for( unsigned int i = 0; i < btms.size(); ++i )
        {
            auto price = KRef(k_datas, btms[i], KAttributeType::LOW);
            if( price < min_price_in_btms + EPSINON )
            {
                lowest_i_in_btms = btms[i];
                min_price_in_btms = price;
            }
        }
        if( lowest_i_in_btms > 0 )
        { 
            first_low_end_i = SolveTrendUpLineEndIndex(k_datas, lowest_i_in_btms, right_index, first_slop);
            if( first_low_end_i > -1 && first_slop > cst_up_line_slop_min - EPSINON )
            {
                lowest_index = lowest_i_in_btms;
                break;
            }else
            {
                temp_low_index = lowest_i_in_btms;
            }
        }else
            return nullptr;
    }
    assert(first_low_end_i > lowest_index);
    // consider second low i :考虑次低点
    if( mid_axis_slop > cst_up_line_slop_min - EPSINON /*&& first_low_end_i + 1 < right_index*/ )
    { 
        auto btms = FindFractalsBetween(k_datas, FractalGeneralType::BTM_FRACTAL, right_index, lowest_index+1);
        int lowest_i_in_btms = 0;
        double min_price_in_btms = MAX_PRICE;
        for( unsigned int i = 0; i < btms.size(); ++i )
        {
            auto price = KRef(k_datas, btms[i], KAttributeType::LOW);
            if( price < min_price_in_btms + EPSINON )
            {
                lowest_i_in_btms = btms[i];
                min_price_in_btms = price;
            }
        }
        if( lowest_i_in_btms > 0 )
        {
            double second_slot = 0.0;
            int second_low_end_i = SolveTrendUpLineEndIndex(k_datas, lowest_i_in_btms, right_index, second_slot);
            if( second_low_end_i > -1 && fabs(second_slot - mid_axis_slop) < fabs(first_slop - mid_axis_slop) )
            { 
                trend_up_line->slope_ = second_slot;
                trend_up_line->beg_ = lowest_i_in_btms;
                trend_up_line->end_ = second_low_end_i;
                trend_up_line->h_price_index_ = highest_index;
                trend_up_line->l_price_index_ = lowest_i_in_btms;
                trend_up_line->is_double_top_ = true;
                auto p_line = AppendTrendLine(code, type_period, trend_up_line, true);
                p_line->is_double_top_ = true;
                return p_line;
            }
        } // if( lowest_i_in_btms > 0 )
    }
       
    trend_up_line->slope_ = first_slop;
    trend_up_line->beg_ = lowest_index;
    trend_up_line->end_ = first_low_end_i;
    trend_up_line->h_price_index_ = highest_index;
    trend_up_line->l_price_index_ = lowest_index;
    trend_up_line->is_double_top_ = true;
    auto p_line = AppendTrendLine(code, type_period, trend_up_line);
    p_line->is_double_top_ = true;
    return p_line;
#endif
}


void  TrendLineMan::Update(const std::string &code, TypePeriod type_period, int k_index, const T_StockHisDataItem &quote_k)
{
    T_HisDataItemContainer &k_datas = app_.stock_data_man().GetHisDataContainer(type_period, code);
    if( k_datas.size() < 2 )
        return;
    int cst_cur_index = k_datas.size() - 1;
    double cst_cur_price = quote_k.close_price;

    static auto del_unnecessary_uplines = [](FuturesForecastApp &app, T_HisDataItemContainer &k_datas, int cst_cur_index, std::deque<std::shared_ptr<TrendLine> > &up_lines, TrendLine &line)
    {
        std::deque<std::shared_ptr<TrendLine> > alive_lines;
        for( unsigned int i = 0; i < up_lines.size(); ++i )
        {
            //auto &line = up_lines[i];
            if( !up_lines[i]->is_alive_ )
                continue;
            alive_lines.push_back(up_lines[i]);
        }
        SortTrendLines(k_datas, cst_cur_index, alive_lines, false);
        unsigned int w = 0;
        for( ; w < alive_lines.size(); ++w )
        {
            if( alive_lines[w]->id_ == line.id_ )
                break;
        }
        assert(w <= alive_lines.size() );

        for( unsigned int j = w + 2; j < alive_lines.size(); ++j )
        {
            app.local_logger().LogLocal(TAG_TREND_LINE_LOG
                , utility::FormatStr("set line(id:%d) dead cur_index:%d | line below new create line's down one", alive_lines[j]->id_, cst_cur_index));
            alive_lines[j]->SetDead();
        }
    };

    int last_date = k_datas.back()->stk_item.date;
    if( last_date_ == 0 )
        last_date_ = last_date;
    int threhold_old_date = app_.exchange_calendar()->PreTradeDate(last_date, (type_period < TypePeriod::PERIOD_30M ? 3 : 30) );

    auto p_trend_up_line_container = FindTrendLineContainer(code, type_period, TrendLineType::UP);
    auto p_trend_down_line_container = FindTrendLineContainer(code, type_period, TrendLineType::DOWN);
    std::deque<std::shared_ptr<TrendLine> > * line_container_arry[] = { p_trend_up_line_container, p_trend_down_line_container};
    // todo: add condition time 9:02
    if( last_date_ < last_date ) // date change
    {
        last_date_ = last_date; 
        for( unsigned int i = 0; i < sizeof(line_container_arry)/sizeof(line_container_arry[0]); ++i )
        {
            if( !line_container_arry[i] )
                continue;
            std::deque<std::shared_ptr<TrendLine> > useful_lines;
            for( unsigned int j = 0; j < line_container_arry[i]->size(); ++j ) 
            {
                auto & line = line_container_arry[i]->at(j);
                assert(line->end_ > -1 );
                if( line->is_alive_ )  
                {
                    if( k_datas[line->end_]->stk_item.date >= threhold_old_date )
                        useful_lines.push_back(line); 
                    else 
                    { 
                        if( IsBreakTrendLine(k_datas, *line, cst_cur_index, line->type_ == TrendLineType::DOWN, KRef(k_datas, cst_cur_index, KAttributeType::CLOSE) + 1.5) )
                        { 
                            app_.local_logger().LogLocal(TAG_TREND_LINE_LOG 
                                , utility::FormatStr("set line(id:%d) dead cur_index:%d | expire and been break", line->id_, cst_cur_index)); 
                            line->SetDead();
                        }
                    }
                }
                
            }
#if IS_DEL_UNLIVE_LINE
            line_container_arry[i]->swap(useful_lines);// del unnecessary trend lines
#endif
        }
    }
    //-----------------------del really break lines ----------------------------
    // find last alive trend up line
    last_trend_up_line_ = nullptr;
    if( p_trend_up_line_container )
    {
        for( unsigned int i = p_trend_up_line_container->size(); i > 0; --i)
        {
            if( p_trend_up_line_container->at(i-1)->is_alive_ )
            {
                last_trend_up_line_ = p_trend_up_line_container->at(i-1);
                break;
            }
        }
    }
    std::deque<std::shared_ptr<TrendLine> > useful_lines;
    for( unsigned int i = 0; p_trend_up_line_container && i < p_trend_up_line_container->size(); ++i )
    {
        auto & trend_up_line = p_trend_up_line_container->at(i);
        assert(trend_up_line->end_ > -1);
        if( trend_up_line->breakdown_infos_.empty() ) 
        {
            useful_lines.push_back(trend_up_line);
            continue;
        }
        // exists break down consider break up--------
        unsigned int j = 0;
        for( ; j < trend_up_line->breakup_infos_.size(); ++j )
        {
            int breakup_k_index = trend_up_line->breakup_infos_[j].k_index;
            if( IsBreakTrendLine(k_datas, *trend_up_line, breakup_k_index
                                , true/*is_break_up*/, k_datas[breakup_k_index]->stk_item.close_price)
                || IsBreakTrendLine(k_datas, *trend_up_line, breakup_k_index
                                , true/*is_break_up*/, k_datas[breakup_k_index]->stk_item.high_price - 1.5)
                )
            {
                // todo: log del 
                //trend_up_line->SetDead();
                break;
            } 
        }
        if( j == trend_up_line->breakup_infos_.size() )
            useful_lines.push_back(trend_up_line);
    }
#if IS_DEL_UNLIVE_LINE
    if( p_trend_up_line_container )
        p_trend_up_line_container->swap(useful_lines);
#endif
    useful_lines.clear();
    for( unsigned int i = 0; p_trend_down_line_container && i < p_trend_down_line_container->size(); ++i )
    {
        auto & trend_down_line = p_trend_down_line_container->at(i);
        assert(trend_down_line->end_ > -1);
        if( trend_down_line->breakup_infos_.empty() ) 
        {
            useful_lines.push_back(trend_down_line);
            continue;
        }
        // exists break up, consider break down
        unsigned int j = 0;
        for( ; j < trend_down_line->breakdown_infos_.size(); ++j )
        {
            int breakdown_k_index = trend_down_line->breakdown_infos_[j].k_index;
            if( IsBreakTrendLine(k_datas, *trend_down_line, breakdown_k_index
                            , false/*is_break_up*/, k_datas[breakdown_k_index]->stk_item.close_price)
                || IsBreakTrendLine(k_datas, *trend_down_line, breakdown_k_index
                            , false/*is_break_up*/, k_datas[breakdown_k_index]->stk_item.low_price + 1.5)
                )
            {
                // todo: log del 
                //trend_down_line->SetDead();
                break;
            } 
        }
        if( j == trend_down_line->breakdown_infos_.size() )
            useful_lines.push_back(trend_down_line);
    }
#if IS_DEL_UNLIVE_LINE
    if( p_trend_down_line_container )
        p_trend_down_line_container->swap(useful_lines);
#endif
    //------------------------record break info if necessary---------------
    for( unsigned int i = 0; i < sizeof(line_container_arry)/sizeof(line_container_arry[0]); ++i )
    {
        if( !line_container_arry[i] )
            continue;
        for( unsigned int j = 0; j < line_container_arry[i]->size(); ++j )
        {
            TrendLine & trend_line = *line_container_arry[i]->at(j);
            if( !trend_line.is_alive_ )
                continue;
            assert(trend_line.end_ > -1);
            if( trend_line.type_ == TrendLineType::UP && quote_k.low_price < KRef(k_datas, trend_line.beg_, KAttributeType::LOW) + EPSINON )
            {
                app_.local_logger().LogLocal(TAG_TREND_LINE_LOG 
                    , utility::FormatStr("set line(id:%d) dead cur_index:%d | beg_ been break", trend_line.id_, cst_cur_index)); 
                trend_line.SetDead();
                continue;
            }
            if( trend_line.type_ == TrendLineType::DOWN && quote_k.high_price > KRef(k_datas, trend_line.beg_, KAttributeType::HIGH) - EPSINON )
            {
                app_.local_logger().LogLocal(TAG_TREND_LINE_LOG 
                    , utility::FormatStr("set line(id:%d) dead cur_index:%d | beg_ been break", trend_line.id_, cst_cur_index)); 
                trend_line.SetDead();
                continue;
            }
            if( !trend_line.is_below_price_ && IsBreakTrendLine(k_datas, trend_line, k_index, true/*is_break_up*/, quote_k.high_price) )
            {
                if( !trend_line.FindBreakInfo(true, k_index) )
                    trend_line.breakup_infos_.emplace_back(k_index);
            }
            if( trend_line.is_below_price_ && IsBreakTrendLine(k_datas, trend_line, k_index, false/*is_break_up*/, quote_k.low_price) )
            {
                if( !trend_line.FindBreakInfo(false, k_index) )
                    trend_line.breakdown_infos_.emplace_back(k_index);
            }
            trend_line.is_below_price_ = quote_k.close_price < TrendLineValue(k_datas, trend_line, k_index) ? false : true;
        }
        
    }
     
#if 0 
    if( last_trend_up_line_ && last_trend_up_line_->is_alive_ )
    {
        if( quote_k.high_price > KRef(k_datas, last_trend_up_line_->h_price_index_, KAttributeType::HIGH) 
            && !last_trend_up_line_->breakdown_infos_.empty() )
        {
            bool ret = CreateTrendUpLine(code, type_period);
            return;
        }
    }
#endif
    bool is_k_close = true;

    if( is_k_close )
    { 
#if 0 
        int right_end_index = cst_cur_index;
        int left_end_index = MAX_VAL(cst_cur_index - 1 - 5, 0);
        double h_price_in_lefts = Highest(k_datas, cst_cur_index - 1, left_end_index - (cst_cur_index - 1));
        if( quote_k.high_price > h_price_in_lefts )
        {
            const double mid_axis_slop = CaculateMidAxisSlop(code, type_period);
            if( mid_axis_slop > 0.01 )
                bool ret = CreateTrendUpLine(code, type_period);
        }
#endif
        if( !last_trend_up_line_ )
        {
            int left_end_index = MAX_VAL(cst_cur_index - 20, 0);
            if( KRef(k_datas, cst_cur_index, KAttributeType::HIGH) > KRef(k_datas, cst_cur_index - 1, KAttributeType::HIGH)
                && FractalLast(k_datas, FractalGeneralType::BTM_FRACTAL, cst_cur_index, left_end_index) > -1 )
            {
                const double mid_axis_slop = CaculateMidAxisSlop(code, type_period);
                if( mid_axis_slop > 0.01 )
                {
                    app_.local_logger().LogLocal(TAG_TREND_LINE_LOG
                        , utility::FormatStr("last_trend_up_line_ null to judge CreateTrendUpLine"));
                    bool ret = CreateTrendUpLine(code, type_period);
                }
            }

            return;
        }
        assert(p_trend_up_line_container);
        assert(last_trend_up_line_);
        if( quote_k.high_price > KRef(k_datas, last_trend_up_line_->h_price_index_, KAttributeType::HIGH) )
        {
            app_.local_logger().LogLocal(TAG_TREND_LINE_LOG
                , utility::FormatStr("cur_index %d high price > last_trend_up_line_(id%d) h_price_index_:%d to judge CreateTrendUpLine"
                , cst_cur_index, last_trend_up_line_->id_, last_trend_up_line_->h_price_index_));
            auto p_line = CreateTrendUpLine(code, type_period, last_trend_up_line_.get());

            if( p_line && p_line->id_ != last_trend_up_line_->id_ )
            {
                del_unnecessary_uplines(app_, k_datas, cst_cur_index, *p_trend_up_line_container, *p_line);
#if 0
                std::deque<std::shared_ptr<TrendLine> > alive_lines;
                for( unsigned int i = 0; i < p_trend_up_line_container->size(); ++i )
                {
                    auto &line = p_trend_up_line_container->at(i);
                    if( !line->is_alive_ )
                        continue;
                    alive_lines.push_back(line);
                }
                SortTrendLines(k_datas, cst_cur_index, alive_lines, false);
                unsigned int w = 0;
                for( ; w < alive_lines.size(); ++w )
                {
                    if( alive_lines[w]->id_ == p_line->id_ )
                        break;
                }
                assert(w <= alive_lines.size() );

                for( unsigned int j = w + 2; j < alive_lines.size(); ++j )
                {
                    app_.local_logger().LogLocal(TAG_TREND_LINE_LOG
                        , utility::FormatStr("set line(id:%d) dead cur_index:%d | line below new create line's down one", alive_lines[j]->id_, cst_cur_index));
                    alive_lines[j]->SetDead();
                }
#endif
            }
        }else if( !last_trend_up_line_->is_double_top_ 
            && cst_cur_index > last_trend_up_line_->h_price_index_ + 2
            && quote_k.high_price > KRef(k_datas, last_trend_up_line_->h_price_index_, KAttributeType::HIGH) - 1.0 
             )
        {
            //if double top shape-------------
            auto btms = FindFractalsBetween(k_datas, FractalGeneralType::BTM_FRACTAL, cst_cur_index, last_trend_up_line_->h_price_index_ + 1);
            if( btms.empty() )
                return;
            unsigned int i = 0;
            double min_price = MAX_PRICE;
            unsigned int min_price_i = 0;
            for( ; i < btms.size(); ++i )
            {
                auto low_p = KRef(k_datas, btms[i], KAttributeType::LOW);
                if( low_p < min_price )
                {
                    min_price = low_p;
                    min_price_i = btms[i];
                }
            }
            //if( BtmestFractalType(k_datas[min_price_i]->type) > FractalType::BTM_AXIS_T_3 )
            if( IsBreakTrendLine(k_datas, *last_trend_up_line_, min_price_i, false, min_price) )
            {
                app_.local_logger().LogLocal(TAG_TREND_LINE_LOG
                    , utility::FormatStr("cur_index %d index:%d near or break last_trend_up_line_(id%d) h_price_index_:%d to judge CreateTrendUpLineByDoubleTop"
                    , cst_cur_index, min_price_i, last_trend_up_line_->id_, last_trend_up_line_->h_price_index_));
                auto p_line = CreateTrendUpLineByDoubleTop(code, type_period, last_trend_up_line_->beg_, cst_cur_index);
                if( p_line && p_line->id_ != last_trend_up_line_->id_ )
                {
                    del_unnecessary_uplines(app_, k_datas, cst_cur_index, *p_trend_up_line_container, *p_line);
                }
            }
        }// consider double top shape
    }
}


int TrendLineMan::GetLeftEndIndex(const std::string &code, TypePeriod type_period)
{
    T_HisDataItemContainer &k_datas = app_.stock_data_man().GetHisDataContainer(type_period, code);
    assert(k_datas.size() > 0);

    const int T = 120;
    int cst_cur_index = k_datas.size() - 1;

    int right_end_index = cst_cur_index;
    int left_end_index = MAX_VAL(cst_cur_index - T, 0);

    // find previous 2 trading period windows 's start index
    int pre_windows = 4; //6
    int temp_index = PreTradeDaysOrNightsStartIndex(*app_.exchange_calendar(), type_period, k_datas, right_end_index, pre_windows);
    if( temp_index > -1 )
        left_end_index = temp_index;
    return left_end_index;
}

double TrendLineMan::CaculateMidAxisSlop(const std::string &code, TypePeriod type_period)
{
    T_HisDataItemContainer &his_data = app_.stock_data_man().GetHisDataContainer(type_period, code);
    if( his_data.size() < 2 )
        return 0.0;
    int left_end_index = GetLeftEndIndex(code, type_period);
    int right_end_index = his_data.size() - 1;

    int h_i = Highest(his_data, left_end_index, right_end_index - left_end_index);
    int l_i = Lowest(his_data, left_end_index, right_end_index - left_end_index);
    int mid_i = (h_i + l_i) / 2;
    double h_p = KRef(his_data, h_i, KAttributeType::HIGH);
    double l_p = KRef(his_data, l_i, KAttributeType::LOW);
    double mid_p = (h_p + l_p) / 2;

    double tgt_slop = 0.0;
    const bool is_tgt_slop_positive = h_i > l_i;

    int start_i = MIN_VAL(h_i, l_i);
    int end_i = MAX_VAL(h_i, l_i);
    double min_distance = FLT_MAX;
    for(double slop = -10; slop < 10.1; slop += 0.01 )
    {
        double total_distance = 0.0;

        for( int i = start_i; i <= end_i; ++i )
        {
            double line_value = (i - mid_i) * slop + mid_p;
            if( line_value > h_p || line_value < l_p )
            {
                total_distance = FLT_MAX;
                break;
            }

            if( is_tgt_slop_positive && (IsBtmFractal(his_data[i]->type) || i == l_i) )
            { 
                double price = KRef(his_data, i, KAttributeType::LOW);
                total_distance += price - line_value;
            }else if( !is_tgt_slop_positive && (IsTopFractal(his_data[i]->type) || i == h_i) )
            { 
                double price = KRef(his_data, i, KAttributeType::HIGH);
                total_distance += price - line_value;
            } 

        }
        if( fabs(total_distance) < fabs(min_distance) )
        {
            if( is_tgt_slop_positive && slop < 0 || !is_tgt_slop_positive && slop > 0 )
                continue;
            min_distance = total_distance;
            tgt_slop = slop;
        }
    }// for 
    return tgt_slop;
}

std::deque<std::shared_ptr<TrendLine> > * TrendLineMan::FindTrendLineContainer(const std::string &code, TypePeriod type_period, TrendLineType trend_line_type)
{
    auto iter0 = code_type_trend_lines_.find(code);
    if( iter0 == code_type_trend_lines_.end() )
        return nullptr;
    auto iter1 = iter0->second.find(type_period);
    if( iter1 == iter0->second.end() )
        return nullptr;
    auto iter2 = iter1->second.find(trend_line_type);
    if( iter2 == iter1->second.end() )
        return nullptr;
      
    return std::addressof(iter2->second);
}

TrendLine * TrendLineMan::FindLastTrendLine(const std::string &code, TypePeriod type_period, TrendLineType trend_line_type)
{
    auto p_container = FindTrendLineContainer(code, type_period, trend_line_type);
    if( !p_container || p_container->empty() )
        return nullptr;
    return p_container->back().get();
}

TrendLine & TrendLineMan::GetLastTrendLine(const std::string &code, TypePeriod type_period, TrendLineType trend_line_type)
{
    auto iter0 = code_type_trend_lines_.find(code);
    if( iter0 == code_type_trend_lines_.end() )
        iter0 = code_type_trend_lines_.insert(std::make_pair(code, std::unordered_map<TypePeriod, std::unordered_map<TrendLineType, std::deque<std::shared_ptr<TrendLine> > > >())).first;
    auto iter1 = iter0->second.find(type_period);
    if( iter1 == iter0->second.end() )
        iter1 = iter0->second.insert(std::make_pair(type_period, std::unordered_map<TrendLineType, std::deque<std::shared_ptr<TrendLine> > >())).first;
    auto iter2 = iter1->second.find(trend_line_type);
    if( iter2 == iter1->second.end() )
        iter2 = iter1->second.insert(std::make_pair(trend_line_type, std::deque<std::shared_ptr<TrendLine> >())).first;
    //iter2 = iter1->second.insert(std::make_pair(trend_line_type, std::make_shared<TrendLine>(trend_line_type))).first;
    if( iter2->second.empty() )
        iter2->second.push_back(std::make_shared<TrendLine>(trend_line_type));
     
    return *(iter2->second.back());
}

TrendLine * TrendLineMan::FindLastTrendLineById(unsigned int id)
{
    auto iter = id_trend_lines_.find(id);
    if( iter == id_trend_lines_.end() )
        return nullptr;
    return iter->second.get();
}

TrendLine * TrendLineMan::FindTrendLine(const std::string &code, TypePeriod type_period, TrendLineType trend_line_type, int line_beg, int line_end)
{
    auto p_container = FindTrendLineContainer(code, type_period, trend_line_type);
    if( !p_container || p_container->empty() )
        return nullptr;
    auto find_ret = std::find_if(p_container->begin(), p_container->end(), [&](const std::shared_ptr<TrendLine>& entry)
    {
        return entry->beg_ == line_beg && entry->end_ == line_end;         
    });
    if( find_ret == p_container->end() )
        return nullptr;
    return find_ret->get();
}

TrendLine * TrendLineMan::AppendTrendLine(const std::string &code, TypePeriod type_period, const std::shared_ptr<TrendLine> &trend_line, bool is_use_second_lh)
{
    auto iter0 = code_type_trend_lines_.find(code);
    if( iter0 == code_type_trend_lines_.end() )
        iter0 = code_type_trend_lines_.insert(std::make_pair(code, std::unordered_map<TypePeriod, std::unordered_map<TrendLineType, std::deque<std::shared_ptr<TrendLine> > > >())).first;
    auto iter1 = iter0->second.find(type_period);
    if( iter1 == iter0->second.end() )
        iter1 = iter0->second.insert(std::make_pair(type_period, std::unordered_map<TrendLineType, std::deque<std::shared_ptr<TrendLine> > >())).first;
    auto iter2 = iter1->second.find(trend_line->type_);
    if( iter2 == iter1->second.end() )
        iter2 = iter1->second.insert(std::make_pair(trend_line->type_, std::deque<std::shared_ptr<TrendLine> >())).first;

    T_HisDataItemContainer &his_data = app_.stock_data_man().GetHisDataContainer(type_period, code);
    int cur_index = his_data.size() - 1;

    auto p_line = FindTrendLine(code, type_period, trend_line->type_, trend_line->beg_, trend_line->end_);
    if( p_line )
    {
        app_.local_logger().LogLocal(TAG_TREND_LINE_LOG
            , utility::FormatStr("warning: line is_up:%d beg:%d end:%d slop:%.5f is already exist new slop:%.5f cur_index:%d use_seond_lh:%d db_top:%d"
            , trend_line->type_ == TrendLineType::UP, trend_line->beg_, trend_line->end_, p_line->slope_
            , trend_line->slope_, cur_index, is_use_second_lh, trend_line->is_double_top_));
        return p_line;
    }

    iter2->second.push_back(trend_line);

    assert( id_trend_lines_.find(trend_line->id_) == id_trend_lines_.end() );
    if( trend_line->id_ == 0 )
        trend_line->id_ = GenerateLineId();
    id_trend_lines_[trend_line->id_] = trend_line;
    
    app_.local_logger().LogLocal(TAG_TREND_LINE_LOG
        , utility::FormatStr("AppendTrendLine id:%d is_up:%d beg:%d end:%d slop:%.5f cur_index:%d use_seond_lh:%d db_top:%d"
        , trend_line->id_, trend_line->type_ == TrendLineType::UP, trend_line->beg_, trend_line->end_
        , trend_line->slope_, cur_index, is_use_second_lh, trend_line->is_double_top_));
    //debug --------------
    if( trend_line->beg_ == trend_line->end_ )
        trend_line->beg_ = trend_line->beg_;
    return trend_line.get();
}

auto TrendLineMan::SolveTrendUpLineEndIndex(T_HisDataItemContainer &k_datas, int lowest_index, int highest_index, OUT double &rel_slop) ->int
{
    const double a_price = KRef(k_datas, lowest_index, KAttributeType::LOW);
    // from highest_index to lowest_index (lowest_index, highest_index] search point c
    int i = highest_index;
    double pre_b_price = KRef(k_datas, i, KAttributeType::LOW);
    while( i > lowest_index )
    {
        double b_price = KRef(k_datas, i, KAttributeType::LOW);
        if( b_price > pre_b_price )
        {
            --i;
            continue;
        }
        pre_b_price = b_price;
        // use i as end_index to create temp trend up line 
        double a_edge_length = b_price - a_price;
        double slope = a_edge_length / (i - lowest_index);

        // judge if break the temp trend line ---
        double lowest_price = KRef(k_datas, i, KAttributeType::LOW);
        int j = i - 1;
        for( ; j > lowest_index; --j )
        {
            double j_low = KRef(k_datas, j, KAttributeType::LOW);
            if( !(j_low < lowest_price) )
                continue;
            if( j_low < (j - lowest_index) * slope + a_price ) // break down the temp trend up line
                break;
            lowest_price = j_low;
        }
        if( j == lowest_index ) // fit
        {
            rel_slop = slope;
            return i;
        }
        --i;
    } // while
    return -1;
}

double TrendLineValue(IN T_HisDataItemContainer & his_data, IN TrendLine &trend_line, int k_index)
{
    assert( trend_line.beg_ > -1);
    assert( trend_line.end_ > -1);
    return (k_index - trend_line.end_) * trend_line.slope_ + KRef(his_data, trend_line.end_, KAttributeType::LOW);
}

// is_judge_break_up: false--judge break down
bool IsBreakTrendLine(IN T_HisDataItemContainer & his_data, IN TrendLine &trend_line, int k_index, bool is_judge_break_up, double price)
{
    const double cst_break_price_distantce = 0.1;
     
    double line_price = TrendLineValue(his_data, trend_line, k_index);
    assert(line_price > 0.0);
    return is_judge_break_up ? price > line_price + cst_break_price_distantce - EPSINON
        :  price < line_price - cst_break_price_distantce + EPSINON;
}
 
struct TrendLineHelpe
{
    TrendLineHelpe(std::shared_ptr<TrendLine> &line_p, double value_p) : line(line_p), value(value_p){}
    TrendLineHelpe(const TrendLineHelpe &lh): line(lh.line), value(lh.value){}
    TrendLineHelpe & operator = (const TrendLineHelpe &lh)
    {
        if( this == &lh )
            return *this;
        line = lh.line; value = lh.value;
        return *this;
    }
    std::shared_ptr<TrendLine> line;
    double value;
};
void SortTrendLines(IN T_HisDataItemContainer &his_data, int k_index, INOUT std::deque<std::shared_ptr<TrendLine> > &lines, bool is_asc, INOUT std::vector<double> *p_line_values)
{ 
    std::vector<TrendLineHelpe> line_helps;
    for( unsigned int i = 0; i < lines.size(); ++i )
    { 
        line_helps.emplace_back(lines[i], TrendLineValue(his_data, *lines[i], k_index));
    }
    std::sort(line_helps.begin(), line_helps.end(), [&](const TrendLineHelpe &lh, const TrendLineHelpe &rh)
    { 
        return is_asc ? lh.value < rh.value : lh.value > rh.value;
    });
    for( unsigned int i = 0; i < lines.size(); ++i )
    { 
        lines[i] = line_helps[i].line;
    }
    if( p_line_values )
    {
        p_line_values->clear();
        //std::vector<double>  values;
        for( unsigned int i = 0; i < line_helps.size(); ++i )
        {
            p_line_values->push_back(line_helps[i].value);
        }
    }

#if 0
    std::sort(lines.begin(), lines.end(), [&](const std::shared_ptr<TrendLine> &lh, const std::shared_ptr<TrendLine> &rh)
    {
        double lh_v = TrendLineValue(his_data, *lh, k_index);
        double rh_v = TrendLineValue(his_data, *rh, k_index);
        return is_asc ? lh_v < rh_v : lh_v > rh_v;
    });
#endif
}