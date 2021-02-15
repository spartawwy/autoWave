#include "forcast_man.h"
#include "stock_data_man.h"

#include <Tlib/core/tsystem_core_common.h>

//#include "exchange_calendar.h"
#include "futures_forecast_app.h"

 
using namespace TSystem;
ForcastMan::ForcastMan(FuturesForecastApp *app, int wall_index)
    : app_(app)
    , wall_index_(wall_index)
    , stock_2pdown_forcast_1m_(512)
    , stock_2pdown_forcast_5m_(512)
    , stock_2pdown_forcast_15m_(512)
    , stock_2pdown_forcast_30m_(512)
    , stock_2pdown_forcast_h_(512)
    , stock_2pdown_forcast_d_(512)
    , stock_2pdown_forcast_w_(512)
    , stock_2pdown_forcast_mon_(512)
    , stock_2pup_forcast_1m_(512)
    , stock_2pup_forcast_5m_(512)
    , stock_2pup_forcast_15m_(512)
    , stock_2pup_forcast_30m_(512)
    , stock_2pup_forcast_h_(512)
    , stock_2pup_forcast_d_(512)
    , stock_2pup_forcast_w_(512)
    , stock_2pup_forcast_mon_(512)
    , stock_3pdown_forcast_1m_(512)
    , stock_3pdown_forcast_5m_(512)
    , stock_3pdown_forcast_15m_(512)
    , stock_3pdown_forcast_30m_(512)
    , stock_3pdown_forcast_h_(512)
    , stock_3pdown_forcast_d_(512)
    , stock_3pdown_forcast_w_(512)
    , stock_3pdown_forcast_mon_(512)
    , stock_3pup_forcast_1m_(512)
    , stock_3pup_forcast_5m_(512)
    , stock_3pup_forcast_15m_(512)
    , stock_3pup_forcast_30m_(512)
    , stock_3pup_forcast_h_(512)
    , stock_3pup_forcast_d_(512)
    , stock_3pup_forcast_w_(512)
    , stock_3pup_forcast_mon_(512)
    , df_no_use_(1)
    , uf_no_use_(1)
{

}

#if 0 
void ForcastMan::Append(TypePeriod type_period, const std::string &code, T_Data2pDownForcast &forcast_data)
{
    _Append2pForcast(type_period, code, forcast_data); 
}


void ForcastMan::Append(TypePeriod type_period, const std::string &code, T_Data2pUpForcast &forcast_data)
{
    _Append2pForcast(type_period, code, forcast_data);
}

std::vector<T_Data2pDownForcast> * ForcastMan::Find2pDownForcastVector(const std::string &code, TypePeriod type_period)
{
    Code2pDownForcastType & holder_ref = Get2pDownDataHolder(type_period);
    if( holder_ref.empty() )
        return nullptr;
    auto vector_iter = holder_ref.find(code);
    if( vector_iter == holder_ref.end() )
        return nullptr;
    return std::addressof(vector_iter->second);
}

std::vector<T_Data2pUpForcast> * ForcastMan::Find2pUpForcastVector(const std::string &code, TypePeriod type_period)
{
    Code2pUpForcastType & holder_ref = Get2pUpDataHolder(type_period);
    if( holder_ref.empty() )
        return nullptr;
    auto vector_iter = holder_ref.find(code);
    if( vector_iter == holder_ref.end() )
        return nullptr;
    return std::addressof(vector_iter->second);
}

bool ForcastMan::HasIn2pDownwardForcast(const std::string &code, TypePeriod type_period, T_KlineDataItem &item_a, T_KlineDataItem &item_b)
{
    static auto has_in2pforcasts = [](std::vector<T_Data2pDownForcast>& data_vector, int date_a, int hhmm_a, int date_b, int hhmm_b)->bool
    { 
        if( data_vector.empty() ) return false;
        unsigned int i = 0;
        for( ; i < data_vector.size(); ++i )
        {
            if( data_vector.at(i).date_a == date_a && data_vector.at(i).date_b == date_b
                && data_vector.at(i).hhmm_a == hhmm_a && data_vector.at(i).hhmm_b == hhmm_b)
                break;
        }
        return i != data_vector.size();
    };

    Code2pDownForcastType & code_2pdown_fcst = Get2pDownDataHolder(type_period);
    auto vector_iter = code_2pdown_fcst.find(code);
    if( vector_iter != code_2pdown_fcst.end() )
        return has_in2pforcasts(vector_iter->second, item_a.kline_posdata(wall_index_).date, item_a.stk_item.hhmmss, item_b.kline_posdata(wall_index_).date, item_b.stk_item.hhmmss);
    else 
        return false;
}

bool ForcastMan::HasIn2pUpForcast(const std::string &code, TypePeriod type_period, T_KlineDataItem &item_a, T_KlineDataItem &item_b)
{
    static auto has_in2pforcasts = [](std::vector<T_Data2pUpForcast>& data_vector, int date_a, int hhmm_a, int date_b, int hhmm_b)->bool
    { 
        if( data_vector.empty() ) return false;
        unsigned int i = 0;
        for( ; i < data_vector.size(); ++i )
        {
             if( data_vector.at(i).date_a == date_a && data_vector.at(i).date_b == date_b
                && data_vector.at(i).hhmm_a == hhmm_a && data_vector.at(i).hhmm_b == hhmm_b)
                break;
        }
        return i != data_vector.size();
    };
    Code2pUpForcastType &code_2pup_fcst = Get2pUpDataHolder(type_period);
    auto vector_iter = code_2pup_fcst.find(code);
    if( vector_iter != code_2pup_fcst.end() )
        return has_in2pforcasts(vector_iter->second, item_a.kline_posdata(wall_index_).date, item_a.stk_item.hhmmss, item_b.kline_posdata(wall_index_).date, item_b.stk_item.hhmmss);
    else 
        return false;
}
#else 

void ForcastMan::Append(TypePeriod type_period, const std::string &code,  bool is_down_forward, T_Data2pForcast& forcast_data)
{ 
    Code2pForcastType & holder_ref = Get2pDataHolder(type_period, is_down_forward);
    auto vector_iter = holder_ref.find(code);
    if( vector_iter == holder_ref.end() )
        vector_iter = holder_ref.insert(std::make_pair(code, Data2pForcastInnerContainer())).first;
    vector_iter->second.push_back( *(T_Data2pForcast*)(&forcast_data) );
    app_->local_logger().LogLocal(forcast_data.is_down ? TAG_FORCAST_BOUNCE_UP_LOG : TAG_FORCAST_BOUNCE_DOWN_LOG, forcast_data.String().c_str());
}

Data2pForcastInnerContainer * ForcastMan::Find2pForcastVector(const std::string &code, TypePeriod type_period, bool is_down_forward)
{
    Code2pForcastType & holder_ref = Get2pDataHolder(type_period, is_down_forward);
    if( holder_ref.empty() )
        return nullptr;
    auto vector_iter = holder_ref.find(code);
    if( vector_iter == holder_ref.end() )
        return nullptr;
    return std::addressof(vector_iter->second);
}
#endif 
Data2pForcastInnerContainer & ForcastMan::Get2pForcastVector(const std::string &code, TypePeriod type_period, bool is_down_forward)
{
    Code2pForcastType & holder_ref = Get2pDataHolder(type_period, is_down_forward);
    auto vector_iter = holder_ref.find(code);
    if( vector_iter == holder_ref.end() )
        vector_iter = holder_ref.insert(std::make_pair(code, Data2pForcastInnerContainer())).first;
    return vector_iter->second;
}

Code2pForcastType & ForcastMan::Get2pDataHolder(TypePeriod type_period, bool is_down_forward)
{
    if( is_down_forward )
        return Get2pDownDataHolder(type_period);
    else
        return Get2pUpDataHolder(type_period);
}

Code2pForcastType & ForcastMan::Get2pUpDataHolder(TypePeriod type_period)
{
    switch (type_period)
    {
    case TypePeriod::PERIOD_1M: return stock_2pup_forcast_1m_;
    case TypePeriod::PERIOD_5M: return stock_2pup_forcast_5m_;
    case TypePeriod::PERIOD_15M: return stock_2pup_forcast_15m_;
    case TypePeriod::PERIOD_30M: return stock_2pup_forcast_30m_;
    case TypePeriod::PERIOD_HOUR: return stock_2pup_forcast_h_;
    case TypePeriod::PERIOD_DAY: return stock_2pup_forcast_d_;
    case TypePeriod::PERIOD_WEEK: return stock_2pup_forcast_w_;
    case TypePeriod::PERIOD_MON: return stock_2pup_forcast_mon_;
    default: assert(false);
    }
    return uf_no_use_;
}

Code2pForcastType & ForcastMan::Get2pDownDataHolder(TypePeriod type_period)
{
    switch (type_period)
    {
    case TypePeriod::PERIOD_1M:  return stock_2pdown_forcast_1m_;
    case TypePeriod::PERIOD_5M:  return stock_2pdown_forcast_5m_;
    case TypePeriod::PERIOD_15M: return stock_2pdown_forcast_15m_;
    case TypePeriod::PERIOD_30M: return stock_2pdown_forcast_30m_;
    case TypePeriod::PERIOD_HOUR: return stock_2pdown_forcast_h_;
    case TypePeriod::PERIOD_DAY: return stock_2pdown_forcast_d_;
    case TypePeriod::PERIOD_WEEK: return stock_2pdown_forcast_w_;
    case TypePeriod::PERIOD_MON: return stock_2pdown_forcast_mon_;
    default: assert(false);
    }
    return df_no_use_;
}

Code3pForcastType & ForcastMan::Get3pDataHolder(TypePeriod type_period, bool is_down)
{
    switch (type_period)
    {
    case TypePeriod::PERIOD_1M:  return  is_down ? stock_3pdown_forcast_1m_ : stock_3pup_forcast_1m_;
    case TypePeriod::PERIOD_5M:  return  is_down ? stock_3pdown_forcast_5m_ : stock_3pup_forcast_5m_;
    case TypePeriod::PERIOD_15M:  return is_down ? stock_3pdown_forcast_15m_ : stock_3pup_forcast_15m_;
    case TypePeriod::PERIOD_30M:  return is_down ? stock_3pdown_forcast_30m_ : stock_3pup_forcast_30m_;
    case TypePeriod::PERIOD_HOUR: return is_down ? stock_3pdown_forcast_h_ : stock_3pup_forcast_h_;
    case TypePeriod::PERIOD_DAY:  return is_down ? stock_3pdown_forcast_d_ : stock_3pup_forcast_d_;
    case TypePeriod::PERIOD_WEEK: return is_down ? stock_3pdown_forcast_w_ : stock_3pup_forcast_w_;
    case TypePeriod::PERIOD_MON:  return is_down ? stock_3pdown_forcast_mon_ : stock_3pup_forcast_mon_;
    default: assert(false);
    }
    return no_use_3p_;
}

T_Data2pForcast * ForcastMan::Find2pForcast(const std::string &code, TypePeriod type_period, bool is_down_forward, T_KlineDataItem &item_a, T_KlineDataItem &item_b)
{
    Code2pForcastType &code_2p_fcst = Get2pDataHolder(type_period, is_down_forward);
    auto vector_iter = code_2p_fcst.find(code);
    if( vector_iter == code_2p_fcst.end() )
         return nullptr;
    if( vector_iter->second.empty() ) 
        return nullptr;
    //unsigned int i = 0;
    //for( ; i < vector_iter->second.size(); ++i )
    auto iter = vector_iter->second.begin();
    for( ; iter != vector_iter->second.end(); ++iter )
    {
        if( (*iter).date_a == item_a.stk_item.date && (*iter).hhmm_a == item_a.stk_item.hhmmss 
            && (*iter).date_b == item_b.stk_item.date && (*iter).hhmm_b == item_b.stk_item.hhmmss  )
            break;
    }
    if( iter != vector_iter->second.end() )
        return std::addressof(*iter); 
    else
        return nullptr;
}
//
//Data2pForcastInnerContainer::iterator * ForcastMan::Find2pForcastIter(const std::string &code, TypePeriod type_period, bool is_down_forward, unsigned int id)
//{
//    Data2pForcastInnerContainer fake_container;
//    Code2pForcastType &code_2p_fcst = Get2pDataHolder(type_period, is_down_forward);
//    auto vector_iter = code_2p_fcst.find(code);
//    if( vector_iter == code_2p_fcst.end() )
//        return nullptr;
//    if( vector_iter->second.empty() ) 
//        return nullptr; 
//    auto iter = vector_iter->second.begin();
//
//    for( ; iter != vector_iter->second.end(); ++iter )
//    {
//        if( (*iter).id == id )
//            break;
//    }
//    if( iter != vector_iter->second.end() )
//        return std::addressof(iter);
//    else
//        return nullptr;
//}

T_Data2pForcast * ForcastMan::Find2pForcast(const std::string &code, TypePeriod type_period, bool is_down_forward, unsigned int id)
{
    Code2pForcastType &code_2p_fcst = Get2pDataHolder(type_period, is_down_forward);
    auto vector_iter = code_2p_fcst.find(code);
    if( vector_iter == code_2p_fcst.end() )
        return nullptr;
    if( vector_iter->second.empty() ) 
        return nullptr; 
    auto iter = vector_iter->second.begin();

    for( ; iter != vector_iter->second.end(); ++iter )
    {
        if( (*iter).id == id )
            break;
    }
    if( iter != vector_iter->second.end() )
        return std::addressof(*iter);
    else
        return nullptr;
}

void ForcastMan::Erase2pForcast(const std::string &code, TypePeriod type_period, bool is_down_forward, unsigned int id)
{
    Code2pForcastType &code_2p_fcst = Get2pDataHolder(type_period, is_down_forward);
    auto vector_iter = code_2p_fcst.find(code);
    if( vector_iter == code_2p_fcst.end() )
        return;
    if( vector_iter->second.empty() ) 
        return; 
    auto iter = vector_iter->second.begin();

    for( ; iter != vector_iter->second.end(); ++iter )
    {
        if( (*iter).id == id )
            break;
    }
    if( iter != vector_iter->second.end() )
        vector_iter->second.erase(iter); 
}

T_Data3pForcast * ForcastMan::Find3pForcast(const std::string &code, TypePeriod type_period, bool is_down_forward, unsigned int id)
{
    Code3pForcastType &code_3p_fcst = Get3pDataHolder(type_period, is_down_forward);
    auto vector_iter = code_3p_fcst.find(code);
    if( vector_iter == code_3p_fcst.end() )
        return nullptr;
    if( vector_iter->second.empty() ) 
        return nullptr; 
    auto iter = vector_iter->second.begin(); 
    for( ; iter != vector_iter->second.end(); ++iter )
    {
        if( (*iter)->id == id )
            return iter->get();//std::addressof(*iter);
    }
    return nullptr;
}

void ForcastMan::Erase3pForcast(const std::string &code, TypePeriod type_period, bool is_down_forward, unsigned int id)
{
    Code3pForcastType &code_3p_fcst = Get3pDataHolder(type_period, is_down_forward);
    auto vector_iter = code_3p_fcst.find(code);
    if( vector_iter == code_3p_fcst.end() )
        return;
    if( vector_iter->second.empty() ) 
        return; 
    auto iter = vector_iter->second.begin(); 
    for( ; iter != vector_iter->second.end(); ++iter )
    {
        if( (*iter)->id == id )
            break;
    }
    if( iter != vector_iter->second.end() )
    {
        vector_iter->second.erase(iter); 
        app_->local_logger().LogLocal(TAG_FORCAST_LOG
            , utility::FormatStr("Erase %s id:%d", (is_down_forward ? TAG_FORCAST_TREND_DOWN_LOG : TAG_FORCAST_TREND_UP_LOG)
            , id));
    }
}

T_Data3pForcast * ForcastMan::Find3pForcast(const std::string &code, TypePeriod type_period, bool is_down_forward, T_KlineDataItem &item_a, T_KlineDataItem &item_b)
{
    Code3pForcastType &code_3p_fcst = Get3pDataHolder(type_period, is_down_forward);
    auto vector_iter = code_3p_fcst.find(code);
    if( vector_iter == code_3p_fcst.end() )
         return nullptr;
    if( vector_iter->second.empty() ) 
        return nullptr;
     
    auto iter = vector_iter->second.begin();
    for( ; iter != vector_iter->second.end(); ++iter )
    {
        if( (*iter)->date_a == item_a.stk_item.date && (*iter)->hhmm_a == item_a.stk_item.hhmmss 
            && (*iter)->date_b == item_b.stk_item.date && (*iter)->hhmm_b == item_b.stk_item.hhmmss  )
            break;
    }
    if( iter != vector_iter->second.end() )
        return iter->get();//std::addressof(*iter); 
    else
        return nullptr;
}

void ForcastMan::Remove3pForcastItem(const std::string &code, TypePeriod type_period, bool is_down_forward, T_KlineDataItem &item_a, T_KlineDataItem &item_b)
{
    T_Data3pForcast * data = Find3pForcast(code, type_period, is_down_forward, item_a, item_b);
    if( data )
        data->Clear();
}

void ForcastMan::RemoveForcastItems(const std::string &code, TypePeriod type_period)
{
    Remove2pForcastItems(code ,type_period, true);
    Remove2pForcastItems(code ,type_period, false);
    Remove3pForcastItems(code ,type_period, true);
    Remove3pForcastItems(code ,type_period, false); 
}

void ForcastMan::Remove2pForcastItems(const std::string &code, TypePeriod type_period, bool is_down_forward)
{
    auto p_2p_fcst = Find2pForcastVector(code, type_period, is_down_forward);
    if( p_2p_fcst )
        p_2p_fcst->clear();
}

void ForcastMan::Remove3pForcastItems(const std::string &code, TypePeriod type_period, bool is_down_forward)
{
    Code3pForcastType &code_3p_fcst = Get3pDataHolder(type_period, is_down_forward);
    auto vector_iter = code_3p_fcst.find(code);
    if( vector_iter != code_3p_fcst.end() )
        vector_iter->second.clear();
}

Data3pForcastInnerContainer * ForcastMan::Find3pForcastVector(const std::string &code, TypePeriod type_period, bool is_down_forward)
{
    Code3pForcastType &code_3p_fcst = Get3pDataHolder(type_period, is_down_forward);
    auto vector_iter = code_3p_fcst.find(code);
    if( vector_iter == code_3p_fcst.end() )
        return nullptr;
    return std::addressof(vector_iter->second);
}

Data3pForcastInnerContainer & ForcastMan::Get3pForcastVector(const std::string &code, TypePeriod type_period, bool is_down_forward)
{
    Code3pForcastType &code_3p_fcst = Get3pDataHolder(type_period, is_down_forward);
    auto vector_iter = code_3p_fcst.find(code);
    if( vector_iter == code_3p_fcst.end() )
       vector_iter = code_3p_fcst.insert(std::make_pair(code, Data3pForcastInnerContainer())).first;
    return vector_iter->second;
}

double ForcastMan::FindMaxForcastPrice(const std::string &code, TypePeriod type_period, int start_date, int start_hhmm, int end_date, int end_hhmm)
{
    assert(start_date <= end_date);
    Code3pForcastType &code_3p_down_fcst = Get3pDataHolder(type_period, false);
    auto vector_iter = code_3p_down_fcst.find(code);
    if( vector_iter == code_3p_down_fcst.end() )
        return 0.0;
    Data3pForcastInnerContainer & forcasts = vector_iter->second;
    double max_price = MIN_PRICE;
    //for(int i = 0; i < forcasts.size(); ++i )
    for( auto iter = forcasts.begin(); iter != forcasts.end(); ++iter )
    {
        if( start_date == end_date )
        {
            if( (*iter)->date_c == start_date && (*iter)->hhmm_c >= start_hhmm && (*iter)->hhmm_c <= end_hhmm )
            {
                if( (*iter)->d3 > max_price )
                    max_price = (*iter)->d3;
            }
        }else 
        {
            if( (*iter)->date_c == start_date && (*iter)->hhmm_c >= start_hhmm )
            {
                if( (*iter)->d3 > max_price )
                    max_price = (*iter)->d3;
            }else
            {
                if( (*iter)->date_c > start_date && (*iter)->date_c < end_date 
                    || (*iter)->date_c == end_date && (*iter)->hhmm_c <= end_hhmm )
                {
                    if( (*iter)->d3 > max_price )
                        max_price = (*iter)->d3;
                }
            }
        } 
    }
    return max_price;
}

double ForcastMan::FindMinForcastPrice(const std::string &code, TypePeriod type_period, int start_date, int start_hhmm, int end_date, int end_hhmm)
{
    assert(start_date <= end_date);
    Code3pForcastType &code_3p_down_fcst = Get3pDataHolder(type_period, true);
    auto vector_iter = code_3p_down_fcst.find(code);
    if( vector_iter == code_3p_down_fcst.end() )
        return MAX_PRICE;
    Data3pForcastInnerContainer & forcasts = vector_iter->second;
    double min_price = MAX_PRICE;
    //for(int i = 0; i < forcasts.size(); ++i )
    for( auto iter_in = forcasts.begin(); iter_in != forcasts.end(); ++iter_in )
    {
        std::shared_ptr<T_Data3pForcast> &iter = *iter_in;
#if 0
        if( (*iter).date_c >= start_date && (*iter).hhmm_c >= start_hhmm
            && (*iter).date_c <= end_date && (*iter).hhmm_c <= end_hhmm )
        {
            if( (*iter).d3 < min_price )
                min_price = (*iter).d3;
        }
#else
        if( start_date == end_date )
        {
            if( (*iter).date_c == start_date && (*iter).hhmm_c >= start_hhmm && (*iter).hhmm_c <= end_hhmm )
            {
                if( (*iter).d3 < min_price )
                    min_price = (*iter).d3;
            }
        }else 
        {
            if( (*iter).date_c == start_date && (*iter).hhmm_c >= start_hhmm )
            {
                if( (*iter).d3 < min_price )
                    min_price = (*iter).d3;
            }else
            {
                if( (*iter).date_c > start_date && (*iter).date_c < end_date 
                    || (*iter).date_c == end_date && (*iter).hhmm_c <= end_hhmm )
                {
                    if( (*iter).d3 < min_price )
                        min_price = (*iter).d3;
                }
            }
        } 
#endif
    }
    return min_price;
}

void ForcastMan::Append(TypePeriod type_period, const std::string &code, bool is_down_forward, T_Data3pForcast &data_3p )
{
    Code3pForcastType &code_3p_fcst = Get3pDataHolder(type_period, is_down_forward);

    auto vector_iter = code_3p_fcst.find(code);
    if( vector_iter == code_3p_fcst.end() )
        vector_iter = code_3p_fcst.insert(std::make_pair(code, Data3pForcastInnerContainer())).first;
    auto p_data_3p = std::make_shared<T_Data3pForcast>(data_3p);
    //DEBUG-----------
    if( data_3p.id == 11 )
        data_3p.id = data_3p.id;
    app_->local_logger().LogLocal(is_down_forward ? TAG_FORCAST_TREND_DOWN_LOG : TAG_FORCAST_TREND_UP_LOG, data_3p.String().c_str());
    if( is_down_forward )
    {
        T_HisDataItemContainer &k_hisdatas = app_->stock_data_man().GetHisDataContainer(ToPeriodType(type_period), code);
        
        for( auto frct_iter = vector_iter->second.begin(); frct_iter != vector_iter->second.end(); ++ frct_iter )
        {
            if( (*frct_iter)->index_a < 0 || (*frct_iter)->index_b < 0 )
                continue;
            if( (*frct_iter)->break_d3_n == 0 && !(*frct_iter)->breaked_a )
            {
                //parent forecasts
                if( (*frct_iter)->index_a <= data_3p.index_a
                    && k_hisdatas[(*frct_iter)->index_a]->stk_item.high_price > k_hisdatas[data_3p.index_a]->stk_item.high_price - EPSINON
                    && k_hisdatas[(*frct_iter)->index_b]->stk_item.low_price > data_3p.d3 
                    && (*frct_iter)->d3 < data_3p.d3 + EPSINON )
                {
                    p_data_3p->parents[(*frct_iter)->id] = (*frct_iter);
                    (*frct_iter)->childrens[p_data_3p->id] = p_data_3p;
                }
                //children forecasts
                else if( (*frct_iter)->index_a >= data_3p.index_a 
                    && k_hisdatas[(*frct_iter)->index_a]->stk_item.high_price < k_hisdatas[data_3p.index_a]->stk_item.high_price + EPSINON
                    && (*frct_iter)->d3 > data_3p.d3 - EPSINON )
                {
                    p_data_3p->childrens[(*frct_iter)->id] = (*frct_iter);
                    (*frct_iter)->parents[p_data_3p->id] = p_data_3p;
                }
            }
        }
    }
    vector_iter->second.emplace_back( std::move(p_data_3p) );
}

void ForcastMan::ResetIndexs(const std::string &code, TypePeriod type_period, T_HisDataItemContainer &container)
{
    auto reset_2p_indexs = [this](const std::string &code, TypePeriod type_period, T_HisDataItemContainer &container, bool is_down_forward)
    {
        auto p_2p_forcasts = Find2pForcastVector(code, type_period, is_down_forward);
        if( p_2p_forcasts )
        {
            for( auto iter = p_2p_forcasts->begin(); iter != p_2p_forcasts->end(); ++iter )
            {
                iter->index_a = FindDateHHmmRelIndex(container, 0, container.size()-1, iter->date_a, iter->hhmm_a);
                assert(iter->index_a > -1);
                iter->index_b = FindDateHHmmRelIndex(container, 0, container.size()-1, iter->date_b, iter->hhmm_b);
                assert(iter->index_b > -1);
            }
        }
    };
    auto reset_3p_indexs = [this](const std::string &code, TypePeriod type_period, T_HisDataItemContainer &container, bool is_down_forward)
    {
        auto p_3p_forcasts = Find3pForcastVector(code, type_period, is_down_forward);
        if( p_3p_forcasts )
        {
            for( auto iter = p_3p_forcasts->begin(); iter != p_3p_forcasts->end(); ++iter )
            {
                (*iter)->index_a = FindDateHHmmRelIndex(container, 0, container.size()-1, (*iter)->date_a, (*iter)->hhmm_a);
                assert((*iter)->index_a > -1);
                (*iter)->index_b = FindDateHHmmRelIndex(container, 0, container.size()-1, (*iter)->date_b, (*iter)->hhmm_b);
                assert((*iter)->index_b > -1);
                (*iter)->index_c = FindDateHHmmRelIndex(container, 0, container.size()-1, (*iter)->date_c, (*iter)->hhmm_c);
                assert((*iter)->index_c > -1);
            }
        }
    };
    reset_2p_indexs(code, type_period, container, true);
    reset_2p_indexs(code, type_period, container, false);
    reset_3p_indexs(code, type_period, container, true);
    reset_3p_indexs(code, type_period, container, false);
}

//void Forcast_Log()
//{
//
//}


std::string T_Data2pForcast::String()
{
    return utility::FormatStr("id:%d %s A(%d %04d i:%d) B(%d %04d i:%d) C1:%.2f C2:%.2f C3:%.2f short_opend(%d %d %d) long_opend(%d %d %d)"
        , this->id, this->is_down ? ToString(ForcastType::BOUNCE_UP).c_str() : ToString(ForcastType::BOUNCE_DOWN).c_str()
        , date_a, hhmm_a, index_a, date_b, hhmm_b, index_b
        , c1, c2, c3, opened_short_c1, opened_short_c2, opened_short_c3
        , opened_long_c1, opened_long_c2, opened_long_c3);
}

std::string T_Data3pForcast::String()
{
    return utility::FormatStr("id:%d %s A(%d %04d i:%d) B(%d %04d i:%d) C(%d %04d i:%d) D1:%.2f D2:%.2f D3:%.2f short_opend(%d %d mid:%d %d) long_opend(%d %d mid:%d %d)"
        , this->id, this->is_down ? ToString(ForcastType::TREND_DOWN).c_str() : ToString(ForcastType::TREND_UP).c_str()
        , date_a, hhmm_a, index_a, date_b, hhmm_b, index_b, date_c, hhmm_c, index_c
        , d1, d2, d3, opened_short_d1, opened_short_d2, opened_short_d2p5, opened_short_d3
        , opened_long_d1, opened_long_d2, opened_long_d2p5, opened_long_d3);
}

std::tuple<double, double, double>  ForcastC_ABDown(double a, double b)
{
    double c2 = sqrt(a * b);
    double c1 = sqrt(b * c2);
    double c3 = c1 + c2 - b;
    return std::make_tuple(c1, c2, c3);
}

std::tuple<double, double, double>  ForcastC_ABUp(double a, double b)
{
    double c2 = sqrt(a * b);
    double c1 = sqrt(b * c2);
    double c3 = 0.7279*a + 0.207*b + 0.06488*c1;
    return std::make_tuple(c1, c2, c3);
}

// ps: make sure a > b > 0
std::tuple<double, double, double>  ForcastD_ABC_Down(double A, double B)
{
    double D2 = B * sqrt( (B/A) + (1.309571 - B) / pow(pow(A,2), 2.10075) );
    double D1= sqrt(B*D2);
    double D3 = 0.254283*B + 0.745551*B*B/A;
    return std::make_tuple(D1, D2, D3);
}

// ps: make sure 0 < a < b
std::tuple<double, double, double>  ForcastD_ABC_Up(double A, double B)
{  
    double D2 = B + B - A;
    double D1 = 0.419913*D2 + 0.41571*B + (0.039433 + 0.164428*D2*D2)/B;
    double D3 = 1.089025*D2 + 0.214334*D1 - 0.30335*B;
    return std::make_tuple(D1, D2, D3);
}