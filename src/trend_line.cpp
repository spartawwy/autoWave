#include "trend_line.h"

#include "futures_forecast_app.h"
#include "exchange_calendar.h"

#if 1 
void  TrendLineMan::CreateTrendLine(const std::string &code, TypePeriod type_period)
{ 
    T_HisDataItemContainer &k_datas = app_.stock_data_man().GetHisDataContainer(type_period, code);
    if( k_datas.size() < 2 )
        return;
    const int T = 120;
    int cst_cur_index = k_datas.size() - 1;

    int right_end_index = cst_cur_index;
    int left_end_index = MAX_VAL(cst_cur_index - T, 0);

    // find previous 2 trading period windows 's start index
    int temp_index = PreTradeDaysOrNightsStartIndex(*app_.exchange_calendar(), type_period, k_datas, right_end_index, 3);
    if( temp_index > -1 )
        left_end_index = temp_index;

    int lowest_index = Lowest(k_datas, cst_cur_index, left_end_index - cst_cur_index);
    int highest_index = Highest(k_datas, cst_cur_index, left_end_index - cst_cur_index);
    if( lowest_index == highest_index )
        return;

    if( lowest_index > highest_index )
    {
        TrendLine & trend_down_line = GetTrendLine(code, type_period, TrendLineType::DOWN);
        trend_down_line.beg_ = highest_index;
        trend_down_line.end_ = lowest_index;

    }else
    {
        TrendLine & trend_up_line = GetTrendLine(code, type_period, TrendLineType::UP);
        //clear old ----
        trend_up_line.beg_ = trend_up_line.end_ = -1; 
        trend_up_line.k_ = 0.0;
        /****************
                b
               /|
              / |
             /  |
          a /___|c
        ***************/
        const double a_price = KRef(k_datas, lowest_index, KAttributeType::LOW);
        
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
            double a_edge_length = b_price - a_price;
            //double b_edge_length = i - lowest_index;
            double k = a_edge_length / (i - lowest_index);
            /* auto caculate_index_rel_distance_y = [&](int index)->double
            {
                return (index - lowest_index) * k ; 
            };*/

            double lowest_price = KRef(k_datas, i, KAttributeType::LOW);
            int j = i - 1;
            for( ; j > lowest_index; --j )
            {
                double j_low = KRef(k_datas, j, KAttributeType::LOW);
                if( !(j_low < lowest_price) )
                    continue;
                if( j_low < (j - lowest_index) * k + a_price ) // fail
                    break;
                lowest_price = j_low;
            }
            if( j == lowest_index ) // fit
            {
                trend_up_line.beg_ = lowest_index;
                trend_up_line.end_ = i;
                trend_up_line.k_ = k;
                break;
            }
            --i;
        } // while
    } 
}

TrendLine * TrendLineMan::FindTrendLine(const std::string &code, TypePeriod type_period, TrendLineType trend_line_type)
{
    auto iter0 = code_type_trend_lines_.find(code);
    if( iter0 == code_type_trend_lines_.end() )
        iter0 = code_type_trend_lines_.insert(std::make_pair(code, std::unordered_map<TypePeriod, std::unordered_map<TrendLineType, std::shared_ptr<TrendLine> > >())).first;
    auto iter1 = iter0->second.find(type_period);
    if( iter1 == iter0->second.end() )
        iter1 = iter0->second.insert(std::make_pair(type_period, std::unordered_map<TrendLineType, std::shared_ptr<TrendLine> >())).first;
    auto iter2 = iter1->second.find(trend_line_type);
    if( iter2 == iter1->second.end() )
        return nullptr;
    return iter2->second.get();
}

TrendLine & TrendLineMan::GetTrendLine(const std::string &code, TypePeriod type_period, TrendLineType trend_line_type)
{
    auto iter0 = code_type_trend_lines_.find(code);
    if( iter0 == code_type_trend_lines_.end() )
        iter0 = code_type_trend_lines_.insert(std::make_pair(code, std::unordered_map<TypePeriod, std::unordered_map<TrendLineType, std::shared_ptr<TrendLine> > >())).first;
    auto iter1 = iter0->second.find(type_period);
    if( iter1 == iter0->second.end() )
        iter1 = iter0->second.insert(std::make_pair(type_period, std::unordered_map<TrendLineType, std::shared_ptr<TrendLine> >())).first;
    auto iter2 = iter1->second.find(trend_line_type);
    if( iter2 == iter1->second.end() )
        iter2 = iter1->second.insert(std::make_pair(trend_line_type, std::make_shared<TrendLine>(trend_line_type))).first;
     
    return *(iter2->second);
}

#endif