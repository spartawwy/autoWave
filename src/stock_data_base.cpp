#include "stock_data_man.h"

//#include <fstream>
//#include <iostream>
//#include <sstream>
//#include <set>
//#include <memory>
#include <cassert>
//#include <algorithm>
#include <xutility>
#include <TLib/core/tsystem_utility_functions.h>
#include <TLib/core/tsystem_time.h>
#include "exchange_calendar.h"

int KRef(const T_HisDataItemContainer & items, int start, int pre_or_back_range, QueryKHandlerType &&handler)
{
    return -1;
}

T_KlineDataItem * KRef(T_HisDataItemContainer & items, int start, int distance)
{
    //assert(start + distance > -1);
    //assert(start + distance < items.size());
    if( start + distance < 0 )
        return nullptr;
    if( start + distance > items.size() - 1 )
        return nullptr;
    //T_KlineDataItem *p_ret_item = nullptr;
    return items[start + distance].get();
}

double KRef(T_HisDataItemContainer & items, int index, KAttributeType attr)
{
    assert(index > -1);
    assert(index < items.size());
    switch (attr)
    {
    case KAttributeType::OPEN: return items[index]->stk_item.open_price;
        break;
    case KAttributeType::CLOSE:return items[index]->stk_item.close_price;
        break;
    case KAttributeType::HIGH:return items[index]->stk_item.high_price;
        break;
    case KAttributeType::LOW:return items[index]->stk_item.low_price;
        break;
    default:
        break;
    } 
    return 0.0;
}

// include start. distance < 0 : toward left; distance > 0 : toward right; distance==0 : only start
//ps: start <= range <= start +distance or start + distance <= range <= start 
int Lowest(T_HisDataItemContainer & items, int start, int distance)
{
    assert(items.size() > 0); 
    int start_index = MIN_VAL(start, start+distance);
    start_index = MAX_VAL(start_index, 0);
    int end_index = MAX_VAL(start, start+distance);
    end_index = MIN_VAL(end_index, items.size() - 1);

    double lowest = 99999.9;
    int target_index = start_index;
    for( int i = start_index; i <= end_index; ++i )
    {
        if( items.at(i)->stk_item.low_price < lowest )
        {
            lowest = items.at(i)->stk_item.low_price;
            target_index = i;
        }
    }
    return target_index;
}

// // include start. if distance > 0 towards right <0: towards left; distance==0 : only start
//ps: start <= range <= start +distance or start + distance <= range <= start 
int Highest(T_HisDataItemContainer & items, int start, int distance)
{
    assert(items.size() > 0); 
    int start_index = MIN_VAL(start, start+distance);
    start_index = MAX_VAL(start_index, 0);
    int end_index = MAX_VAL(start, start+distance);
    end_index = MIN_VAL(end_index, items.size() - 1);

    double highest = 0.0;
    int target_index = start_index;
    for( int i = start_index; i <= end_index; ++i )
    {
        if( items.at(i)->stk_item.high_price > highest )
        {
            highest = items.at(i)->stk_item.high_price;
            target_index = i;
        }
    }
    return target_index;
}

int BarLast(T_HisDataItemContainer & items, int start, KAttributeType attr, CompareType compare_type, double val)
{
    assert(start > -1);
    assert(start < items.size());

    for( int i = start; i >= 0; --i )
    {
        double attr_val = 0.0;
        switch (attr)
        {
        case KAttributeType::OPEN: attr_val = items[i]->stk_item.open_price;
            break;
        case KAttributeType::CLOSE:attr_val = items[i]->stk_item.close_price;
            break;
        case KAttributeType::HIGH: attr_val = items[i]->stk_item.high_price;
            break;
        case KAttributeType::LOW:  attr_val = items[i]->stk_item.low_price;
            break;
        default: assert(false);
            break;
        } 
        switch (compare_type)
        {
        case CompareType::BIGEQUAL: if( !(attr_val < val) ) return i;
            break;
        case CompareType::SMALLEQUAL: if( !(attr_val > val) ) return i;
            break;
        case CompareType::EQUAL: if( Equal(attr_val, val) ) return i;
            break;
        case CompareType::BIG: if( attr_val > val ) return i;
            break;
        case CompareType::SMALL: if( attr_val < val ) return i;
            break;
        default:
            break;
        }
    }
    return -1;
}

// find first bar(kline) toward right [start, right_end] which fit condition (attr compare val)
int BarFirst(T_HisDataItemContainer & items, int start, int right_end, KAttributeType attr, CompareType compare_type, double val)
{
    assert(start > -1);
    assert(start < items.size());
    int r_end = MAX_VAL(right_end, items.size() - 1);
    for( int i = start; i <= r_end; ++i )
    {
        double attr_val = 0.0;
        switch (attr)
        {
        case KAttributeType::OPEN: attr_val = items[i]->stk_item.open_price;
            break;
        case KAttributeType::CLOSE:attr_val = items[i]->stk_item.close_price;
            break;
        case KAttributeType::HIGH: attr_val = items[i]->stk_item.high_price;
            break;
        case KAttributeType::LOW:  attr_val = items[i]->stk_item.low_price;
            break;
        default: assert(false);
            break;
        } 
        switch (compare_type)
        {
        case CompareType::BIGEQUAL: if( !(attr_val < val) ) return i;
            break;
        case CompareType::SMALLEQUAL: if( !(attr_val > val) ) return i;
            break;
        case CompareType::EQUAL: if( Equal(attr_val, val) ) return i;
            break;
        case CompareType::BIG: if( attr_val > val ) return i;
            break;
        case CompareType::SMALL: if( attr_val < val ) return i;
            break;
        default:
            break;
        }
    }
    return -1;
}

// find fenxin toward left [left_end, start)
std::tuple<int, FractalGeneralType> FractalLast(T_HisDataItemContainer & items, int start, int left_end)
{
    assert(start > -1 && start < items.size());  
    assert(left_end > -1 && left_end < items.size());  
    assert(start > left_end);
    for( int i = start-1; i >= left_end; i-- )
    {
        if( IsTopFractal(items[i]->type) )
        {
            return std::make_tuple(i, FractalGeneralType::TOP_FRACTAL);
        }
        else if( IsBtmFractal(items[i]->type) )
        {
            return std::make_tuple(i, FractalGeneralType::BTM_FRACTAL);
        }
    }
    return std::make_tuple(-1, FractalGeneralType::UNKNOW);
}

// find first special fenxin toward right [start, right_end]
// ret -1: not find
int FractalFirst(T_HisDataItemContainer & items, FractalGeneralType fractal_type, int start, int right_end)
{
    assert(start > -1 && start < items.size());  
    int r_end = MAX_VAL(right_end, items.size() - 1);
    for( int i = start; i <= r_end; ++i )
    {
        switch (fractal_type)
        {
        case FractalGeneralType::TOP_FRACTAL:
            if( IsTopFractal(items[i]->type) ) return i; break;
        case FractalGeneralType::BTM_FRACTAL:
            if( IsBtmFractal(items[i]->type) ) return i; break;
        case FractalGeneralType::UNKNOW:
            if( !IsTopFractal(items[i]->type) && !IsBtmFractal(items[i]->type) ) return i; break;
        default: assert(false);
            break;
        }
    }
    return -1;
}

// find fenxin toward left [left_end, start)
std::vector<int> FindFractalsBetween(T_HisDataItemContainer & items, FractalGeneralType type, int start, int left_end)
{
    assert(start > -1 && start < items.size());  
    assert(left_end > -1 && left_end < items.size());  
    assert(start > left_end);
    std::vector<int>  ret;
    if( type == FractalGeneralType::TOP_FRACTAL )
    {
        for( int i = start-1; i >= left_end; i-- )
        {
            if( IsTopFractal(items[i]->type) )
            {
                ret.push_back(i);
            }
        }
    }else if( type == FractalGeneralType::BTM_FRACTAL )
    {
        for( int i = start-1; i >= left_end; i-- )
        {
            if( IsBtmFractal(items[i]->type) )
            {
                ret.push_back(i);
            }
        }
    }
    return ret;
}

// judge fractal(fenxin) general trend (判断位置start之前的分形总体趋势:若是只有一个分形点
// 则底分形视为下降趋势, 反之视为上升趋势)
// ps: not include start, make sure toward_left_len > 0
TrendType JudgeTrendBaseonFxRightToLeft(T_HisDataItemContainer & items, int start, int toward_left_len
                                        , OUT int &top_index, OUT int &mid_index, OUT int &btm_index)
{
    auto set_ret_index = [&top_index, &mid_index, &btm_index](int top_index_p, int mid_index_p, int btm_index_p)
    {
        top_index = top_index_p; mid_index = mid_index_p; btm_index = btm_index_p;
    };
    assert(start > -1 && start < items.size()); 
    assert(toward_left_len > 0);
    //const int cst_T = 120;
    int l_end_index = MAX_VAL(0, start-toward_left_len);
    T_StructLineContainer struct_line_data;
    Traverse_GetStuctLines(items, start, toward_left_len, struct_line_data);
    if( struct_line_data.size() > 0 )
    {
        T_StockHisDataItem &item_cur = items[start]->stk_item;
        //const double cur_price = item_cur.close_price;

        T_StockHisDataItem &item_a = items[struct_line_data[0]->beg_index]->stk_item; // struct_line0_left_item
        T_StockHisDataItem &item_b = items[struct_line_data[0]->end_index]->stk_item; // struct_line0_right_item

        if( struct_line_data[0]->type == LineType::DOWN )
        {  
            set_ret_index(struct_line_data[0]->beg_index, -1, struct_line_data[0]->end_index);
            return TrendType::DOWN;
        }
        else
        { 
            set_ret_index(struct_line_data[0]->end_index, -1, struct_line_data[0]->beg_index);
            return TrendType::UP;
        }
    } 
    // ps: fenxin index n,...,1,0
    auto index_data0 = FractalLast(items, start, l_end_index);
    FractalGeneralType frac_gen_type0 = std::get<1>(index_data0);
    int left_fenxin_point0 = std::get<0>(index_data0);
    if( left_fenxin_point0 < 0 )
        return TrendType::NO;

    auto index_data1 = FractalLast(items, left_fenxin_point0, l_end_index);
    FractalGeneralType frac_gen_type1 = std::get<1>(index_data1);
    int left_fenxin_point1 = std::get<0>(index_data1);
    if( left_fenxin_point1 < 0 ) // only one fenxin point
    {
        switch (frac_gen_type0)
        {
        case FractalGeneralType::BTM_FRACTAL: 
        {   
            int h_index = Highest(items, left_fenxin_point0, l_end_index - left_fenxin_point0);
            set_ret_index(h_index, -1, left_fenxin_point0);
            return TrendType::DOWN;break;
        }
        case FractalGeneralType::TOP_FRACTAL: 
        {
            int l_index = Lowest(items, left_fenxin_point0, l_end_index - left_fenxin_point0);
            set_ret_index(left_fenxin_point0, -1, l_index);
            return TrendType::UP; break;
         }
        default:  return TrendType::NO; break;
        }
    }
    auto index_data2 = FractalLast(items, left_fenxin_point1, l_end_index);
    FractalGeneralType frac_gen_type2 = std::get<1>(index_data2);
    int left_fenxin_point2 = std::get<0>(index_data2);
    if( left_fenxin_point2 < 0 ) // only two fenxin point
    {
        switch (frac_gen_type1)
        {
        case FractalGeneralType::BTM_FRACTAL: 
        {
            if( frac_gen_type0 == FractalGeneralType::BTM_FRACTAL )
                return TrendType::NO; 
            else if( frac_gen_type0 == FractalGeneralType::TOP_FRACTAL )
            {
                set_ret_index(left_fenxin_point0, -1, left_fenxin_point1);
                return TrendType::UP; //ndedt:
            }
        }
        case FractalGeneralType::TOP_FRACTAL: 
        {
            if( frac_gen_type0 == FractalGeneralType::BTM_FRACTAL )
            {
                set_ret_index(left_fenxin_point1, -1, left_fenxin_point0);
                return TrendType::DOWN; //ndedt:
            }
            else if( frac_gen_type0 == FractalGeneralType::TOP_FRACTAL )
                return TrendType::NO;  
        }
        default: assert(false); break;
        }
    }

    auto index_data3 = FractalLast(items, left_fenxin_point2, l_end_index);
    FractalGeneralType frac_gen_type3 = std::get<1>(index_data3);
    int left_fenxin_point3 = std::get<0>(index_data3);
    if( left_fenxin_point3 < 0 ) // only 3 fenxin point 2, 1, 0
    {
        if( frac_gen_type1 == frac_gen_type2 ) 
            return TrendType::NO;
        if(frac_gen_type0 == FractalGeneralType::BTM_FRACTAL )
        {
            if( frac_gen_type2 == FractalGeneralType::BTM_FRACTAL )
            {   // 低点从左向右抬高
                if( items[left_fenxin_point2]->stk_item.low_price < items[left_fenxin_point0]->stk_item.low_price )
                {
                    set_ret_index(left_fenxin_point0, -1, left_fenxin_point2);
                    return TrendType::UP;
                }else
                {
                    set_ret_index(left_fenxin_point2, -1, left_fenxin_point0);
                    return TrendType::DOWN;
                }
            }else
            {
                set_ret_index(left_fenxin_point2, -1, left_fenxin_point0);
                return TrendType::DOWN;
            }
        }else //fenxin poin0 is top fractal
        {
            if( frac_gen_type2 == FractalGeneralType::TOP_FRACTAL )
            {   // 高点从左向右抬高
                if( items[left_fenxin_point0]->stk_item.high_price > items[left_fenxin_point2]->stk_item.high_price )
                {
                    set_ret_index(left_fenxin_point0, -1, left_fenxin_point2);
                    return TrendType::UP;
                }else
                {
                    set_ret_index(left_fenxin_point2, -1, left_fenxin_point0);
                    return TrendType::DOWN;
                }
            }else
            {
                set_ret_index(left_fenxin_point0, -1, left_fenxin_point2);
                return TrendType::UP;
            }
        }
    }else // fenxin point 3, 2, 1, 0
    {
        double lowest_price = Lowest(items, left_fenxin_point0, left_fenxin_point3-left_fenxin_point0);
        double highest_price = Highest(items, left_fenxin_point0, left_fenxin_point3-left_fenxin_point0);
        if(frac_gen_type0 == FractalGeneralType::BTM_FRACTAL )
        {
            if( frac_gen_type3 == FractalGeneralType::BTM_FRACTAL )
                return TrendType::NO;
            else
            {
                if( items[left_fenxin_point3]->stk_item.high_price < items[left_fenxin_point1]->stk_item.high_price )
                {
                    if( !(items[left_fenxin_point3]->stk_item.low_price < lowest_price) )
                        return TrendType::SHOCK;
                    else
                    {
                        set_ret_index(left_fenxin_point0, -1, left_fenxin_point3);
                        return TrendType::UP;
                    }
                }else
                {
                    set_ret_index(left_fenxin_point3, -1, left_fenxin_point0);
                    return TrendType::DOWN;
                }
            }
        }else // fenxin point 0 is Top fractal, so point 3 is Btm fractal
        {
            if( Equal(items[left_fenxin_point0]->stk_item.high_price, highest_price) )
            {
                if( items[left_fenxin_point3]->stk_item.high_price < items[left_fenxin_point2]->stk_item.high_price )
                {
                    set_ret_index(left_fenxin_point0, -1, left_fenxin_point3);
                    return TrendType::UP;
                }
                else
                    return TrendType::SHOCK; 
            }else
            {
                if( Equal(items[left_fenxin_point3]->stk_item.high_price, highest_price) ) // 高点依次降低
                {
                    set_ret_index(left_fenxin_point3, -1, left_fenxin_point0);
                    return TrendType::DOWN;
                }
                else
                {
                    if( Equal(items[left_fenxin_point0]->stk_item.low_price, lowest_price) ) // 低点依次降低
                    {
                        set_ret_index(left_fenxin_point3, -1, left_fenxin_point0);
                        return TrendType::DOWN;
                    }
                    else
                        return TrendType::SHOCK; 
                }
            }
        }
    }
     
    return TrendType::NO; // tmp code
}


int PreTradeDaysOrNightsStartIndex(ExchangeCalendar &calendar, TypePeriod type_period, T_HisDataItemContainer &k_datas, int index, int t)
{
    assert(index > -1 && index < k_datas.size());

    int cur_date = k_datas[index]->stk_item.date;
    int cur_hhmm = k_datas[index]->stk_item.hhmmss;
    auto date_hhmm = PreTradeDaysOrNightsStart(calendar, type_period, cur_date, cur_hhmm, t); 

    T_KlineDataItem item;
    item.stk_item.date = std::get<0>(date_hhmm);
    item.stk_item.hhmmss = std::get<1>(date_hhmm);
    int target_index = Lower_Bound(k_datas, 0, index,  item);
    return target_index;
}
//[start, end]
int Binary_Search(T_HisDataItemContainer &k_datas, int start, int end, T_KlineDataItem &item)
{
    int left, middle, right;

    left = start;
    right = end; 

    while( left <= right)
    {
        middle = (left + right) / 2;
        if( *k_datas[middle] < item )
            left = middle + 1;
        else if( *k_datas[middle] > item )
            right = middle - 1;
        else
            return middle;
    }
    return -1;
}

//[start, end]
int Lower_Bound(T_HisDataItemContainer &k_datas, int start, int end, T_KlineDataItem &item)
{
    int left, middle, right;

    left = start;
    right = end; 

    int bigger_near_by = -1;
    while( left <= right)
    {
        middle = (left + right) / 2;
        if( *k_datas[middle] < item )
            left = middle + 1;
        else if( *k_datas[middle] > item )
        {
            right = middle - 1;
            bigger_near_by = middle;
        }else
            return middle;
    }
    if( bigger_near_by > -1 )
        return bigger_near_by;
    else
        return -1;
}

int FindDateHHmmRelIndex(T_HisDataItemContainer &k_datas, int left_end_index, int right_end_index, int date, int hhmm)
{
    T_KlineDataItem item_a;
    item_a.stk_item.date = date;
    item_a.stk_item.hhmmss = hhmm;
    return Lower_Bound(k_datas, left_end_index, right_end_index, item_a);
}