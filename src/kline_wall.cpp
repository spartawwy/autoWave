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

// ps: only invoke by mouse drag and zoom in
void KLineWall::AppendData()
{ 
    int oldest_day = QDateTime::currentDateTime().toString("yyyyMMdd").toInt();
    if( !p_hisdata_container_->empty() )
        oldest_day = p_hisdata_container_->front()->stk_item.date;

    QDate qdate_obj(oldest_day/10000, (oldest_day%10000)/100, oldest_day%100);
    int start_date = qdate_obj.addDays( -1 * (4 * 30) ).toString("yyyyMMdd").toInt(); 
    switch(k_type_)
    {
    case TypePeriod::PERIOD_YEAR: start_date = qdate_obj.addDays( -1 * (5 * 12 * 30) ).toString("yyyyMMdd").toInt(); break;
    case TypePeriod::PERIOD_MON:
    case TypePeriod::PERIOD_WEEK: start_date = qdate_obj.addDays( -1 * (2 * 12 * 30) ).toString("yyyyMMdd").toInt(); break;
    case TypePeriod::PERIOD_DAY: start_date = qdate_obj.addDays( -1 * (1 * 12 * 30) ).toString("yyyyMMdd").toInt(); break;
    case TypePeriod::PERIOD_HOUR:start_date = qdate_obj.addDays( -1 * (6 * 30) ).toString("yyyyMMdd").toInt(); break;
    case TypePeriod::PERIOD_30M: start_date = qdate_obj.addDays( -1 * (3 * 30) ).toString("yyyyMMdd").toInt(); break;
    case TypePeriod::PERIOD_15M: start_date = qdate_obj.addDays( -1 * (1 * 30) ).toString("yyyyMMdd").toInt(); break;
    case TypePeriod::PERIOD_5M: start_date = qdate_obj.addDays( -1 * (1 * 5) ).toString("yyyyMMdd").toInt(); break;
    case TypePeriod::PERIOD_1M: start_date = qdate_obj.addDays( -1 * (1 * 3) ).toString("yyyyMMdd").toInt(); break;
    default: break;
    }
     
    auto p_container = app_->stock_data_man().AppendStockData(ToPeriodType(k_type_), nmarket_, stock_code_, start_date, oldest_day, is_index_);
    if( !p_container->empty() )
    { 
        app_->stock_data_man().TraverseSetFeatureData(stock_code_, ToPeriodType(k_type_), is_index_,  k_rend_index_for_train_);
        //HandleAutoForcast();
    }
}

T_HisDataItemContainer* KLineWall::AppendPreData(int date, int /*hhmm*/)
{
    assert( date > 19800000 && date < 20500000 );

    int oldest_day = QDateTime::currentDateTime().toString("yyyyMMdd").toInt(); //default 
    if( !p_hisdata_container_->empty() )
        oldest_day = p_hisdata_container_->front()->stk_item.date;

    if( date > oldest_day )
    { 
        return p_hisdata_container_; 
    }

    auto p_container = app_->stock_data_man().AppendStockData(ToPeriodType(k_type_), nmarket_, stock_code_, date, oldest_day, is_index_); 
    return p_container;
}
 
T_HisDataItemContainer* KLineWall::AppendData(int date, int hhmm)
{
    assert( date > 19800000 && date < 20500000 );

    int today = QDateTime::currentDateTime().toString("yyyyMMdd").toInt(); //default 
    int back_date = app_->exchange_calendar()->PreTradeDate(today, 5);
    if( !p_hisdata_container_->empty() )
        back_date = p_hisdata_container_->back()->stk_item.date;

    if( back_date > date )
    {
        return p_hisdata_container_;
    }
    auto p_container = app_->stock_data_man().AppendStockData(ToPeriodType(k_type_), nmarket_, stock_code_, back_date, date, is_index_);
    
    return p_container;
}

void KLineWall::ResetTypePeriod(TypePeriod  type)
{ 
    if( k_type_ == type )
        return;
    ResetStock(stock_code_.c_str(), type, is_index_, nmarket_);
}


void KLineWall::UpdateKwallMinMaxPrice()
{
    if( !painting_mutex_.try_lock() )
        return;
    std::tuple<double, double> price_tuple;
    std::tuple<int, int, int, int> date_times_tuple;
    if( GetContainerMaxMinPrice(ToPeriodType(k_type_), stock_code_, k_num_, price_tuple, date_times_tuple) )
    {
        float h_price = std::get<0>(price_tuple);
        float l_price = std::get<1>(price_tuple);
        double try_new_high = h_price * cst_k_mm_enlarge_times;
        if( h_price > 100.0 )
            try_new_high = h_price * 1.002;

        if( try_new_high < this->highestMaxPrice_ || try_new_high > this->highestMaxPrice_)
            SetHighestMaxPrice(try_new_high);

        double try_new_low = l_price * cst_k_mm_narrow_times;
        if( l_price > 100.0 )
            try_new_low = l_price * 0.998;
        if( try_new_low < this->lowestMinPrice_ || try_new_low > this->lowestMinPrice_)
            SetLowestMinPrice(try_new_low);
        highest_price_date_ = std::get<0>(date_times_tuple);
        highest_price_hhmm_ = std::get<1>(date_times_tuple);
        lowest_price_date_ = std::get<2>(date_times_tuple);
        lowest_price_hhmm_ = std::get<3>(date_times_tuple);
    }
    painting_mutex_.unlock();
}

PeriodType KLineWall::ToPeriodType(TypePeriod src)
{
    switch(src)
    {
    case TypePeriod::PERIOD_1M: return PeriodType::PERIOD_1M;
    case TypePeriod::PERIOD_5M: return PeriodType::PERIOD_5M;
    case TypePeriod::PERIOD_15M: return PeriodType::PERIOD_15M;
    case TypePeriod::PERIOD_30M: return PeriodType::PERIOD_30M;
    case TypePeriod::PERIOD_HOUR: return PeriodType::PERIOD_HOUR;
    case TypePeriod::PERIOD_DAY: return PeriodType::PERIOD_DAY;
    case TypePeriod::PERIOD_WEEK: return PeriodType::PERIOD_WEEK;
    case TypePeriod::PERIOD_MON: return PeriodType::PERIOD_MON;
    default: assert(false); 
    }
    return PeriodType::PERIOD_DAY;
}

double KLineWall::GetCurWinKLargetstVol()
{
    double largest_vol = 0.0;
    int k = k_num_;
    for( auto iter = p_hisdata_container_->rbegin() + k_rend_index_;
        iter != p_hisdata_container_->rend() && k > 0; 
        ++iter, --k)
        if( (*iter)->stk_item.vol > largest_vol ) 
            largest_vol = (*iter)->stk_item.vol;
    return largest_vol;
}

T_KlineDataItem * KLineWall::GetKLineDataItemByDate(int date, int hhmm)
{ 
    for( auto iter = p_hisdata_container_->rbegin();
        iter != p_hisdata_container_->rend(); 
        ++iter )
    {   
        if( iter->get()->stk_item.date == date && iter->get()->stk_item.hhmmss == hhmm )
            return iter->get(); 
    }
    return nullptr;
}

// dates : tuple<highest related date, highest related hhmm, lowest related date, lowest related hhmm>
bool KLineWall::GetContainerMaxMinPrice(PeriodType period_type, const std::string& code, int k_num, std::tuple<double, double>& ret, std::tuple<int, int, int, int> &date_times)
{
    T_HisDataItemContainer &container = app_->stock_data_man().GetHisDataContainer(period_type, code);
    if( container.empty() )
        return false;
    assert( container.size() > k_rend_index_ );

    unsigned int start_index = container.size() - k_rend_index_ > k_num ? (container.size() - k_rend_index_ - k_num) : 0; 
    if( start_index == container.size() )
        start_index = container.size() - 1;
    unsigned int end_index = container.size() - 1 > 0 ? container.size() - 1 - k_rend_index_ : 0;
    if( end_index < 0 )
        end_index = 0;
    if( end_index < start_index )
    {
        unsigned int temp_val = end_index;
        end_index = start_index;
        start_index = temp_val;
    }
    double highest_price = MIN_PRICE;
    double lowest_price = MAX_PRICE;
    int highest_price_date = 0;
    int highest_price_hhmm = 0;
    int lowest_price_date = 0;
    int lowest_price_hhmm = 0;
    for( unsigned int i = start_index; i <= end_index; ++ i )
    {
        if( container.at(i)->stk_item.high_price > highest_price )
        {
            highest_price = container.at(i)->stk_item.high_price; 
            highest_price_date = container.at(i)->stk_item.date;
            highest_price_hhmm = container.at(i)->stk_item.hhmmss;
        }
        if( container.at(i)->stk_item.low_price < lowest_price )
        {
            lowest_price = container.at(i)->stk_item.low_price;
            lowest_price_date = container.at(i)->stk_item.date;
            lowest_price_hhmm = container.at(i)->stk_item.hhmmss;
        }
    } 
    // for forecast price ---------
    ForcastMan *forcast_mans[] = {&forcast_man_, &auto_forcast_man_};
    for( int i = 0; i < sizeof(forcast_mans)/sizeof(forcast_mans[0]); ++i )
    {
        double tmp_price = forcast_mans[i]->FindMaxForcastPrice(code, ToTypePeriod(period_type)
            , container.at(start_index)->stk_item.date, container.at(start_index)->stk_item.hhmmss
            , container.at(end_index)->stk_item.date, container.at(end_index)->stk_item.hhmmss);
        if( tmp_price > highest_price )
            highest_price = tmp_price;
        tmp_price = forcast_mans[i]->FindMinForcastPrice(code, ToTypePeriod(period_type)
            , container.at(start_index)->stk_item.date, container.at(start_index)->stk_item.hhmmss
            ,container.at(end_index)->stk_item.date, container.at(end_index)->stk_item.hhmmss);
        if( tmp_price < lowest_price )
            lowest_price = tmp_price;
    }

    ret = std::make_tuple(highest_price, lowest_price);
    date_times = std::make_tuple(highest_price_date, highest_price_hhmm, lowest_price_date, lowest_price_hhmm);
    return true;
}

// ps : contain iter
int KLineWall::FindTopItem_TowardLeft(T_HisDataItemContainer &his_data, T_HisDataItemContainer::reverse_iterator iter, int k_index, T_KlinePosData *&left_pos_data)
{
    auto left_tgt_iter = iter;
    int cp_j = k_index;
    for( ; left_tgt_iter != his_data.rend() && cp_j > 0; 
        ++left_tgt_iter, --cp_j)
    {
        int type = (*left_tgt_iter)->type;
        auto left_frac_type = MaxFractalType(type);
        if( left_frac_type >= FractalType::TOP_AXIS_T_3 )
            break;
    }
    if( left_tgt_iter != his_data.rend() && cp_j > 0 )
    {
        left_pos_data = std::addressof(left_tgt_iter->get()->kline_posdata(wall_index_));
        return cp_j;
    }else
        return 0;
}

// ps : contain iter
int KLineWall::FindTopFakeItem_TowardLeft(T_HisDataItemContainer &his_data, T_HisDataItemContainer::reverse_iterator iter, int k_index, T_KlinePosData *&left_pos_data)
{
    auto left_tgt_iter = iter;
    int cp_j = k_index - 1;
    for( ; left_tgt_iter != his_data.rend() && cp_j > 0; 
        ++left_tgt_iter, --cp_j)
    { 
        if( (*left_tgt_iter)->type == (int)FractalType::TOP_FAKE )
            break;
    }
    if( left_tgt_iter != his_data.rend() && cp_j > 0 )
    {
        left_pos_data = std::addressof(left_tgt_iter->get()->kline_posdata(wall_index_));
        return cp_j;
    }else
        return 0;
}

// ps : not contain iter
int KLineWall::FindBtmItem_TowardLeft(T_HisDataItemContainer &his_data, T_HisDataItemContainer::reverse_iterator iter, int k_index, T_KlinePosData *&left_pos_data)
{
    auto left_tgt_iter = iter + 1;
    int cp_j = k_index - 1;
    for( ; left_tgt_iter != his_data.rend() && cp_j > 0; 
        ++left_tgt_iter, --cp_j) // find left btm_axis_iter 
    {
        auto left_frac_type = BtmestFractalType((*left_tgt_iter)->type);
        if( left_frac_type != FractalType::UNKNOW_FRACTAL )
            break;
    }

    if( left_tgt_iter != his_data.rend() && cp_j > 0 )
    {
        left_pos_data = std::addressof(left_tgt_iter->get()->kline_posdata(wall_index_));
        return cp_j;
    }else
        return 0;
}

// ps : contain iter
int KLineWall::FindBtmFakeItem_TowardLeft(T_HisDataItemContainer &his_data, T_HisDataItemContainer::reverse_iterator iter, int k_index, T_KlinePosData *&left_pos_data)
{
    auto left_tgt_iter = iter;
    int cp_j = k_index - 1;
    for( ; left_tgt_iter != his_data.rend() && cp_j > 0; 
        ++left_tgt_iter, --cp_j) // find left btm_axis_iter 
    {
        if( (*left_tgt_iter)->type == (int)FractalType::BTM_FAKE )
            break;
    }

    if( left_tgt_iter != his_data.rend() && cp_j > 0 )
    {
        left_pos_data = std::addressof(left_tgt_iter->get()->kline_posdata(wall_index_));
        return cp_j;
    }else
        return 0;
}

int KLineWall::Calculate_k_mm_h()
{
    auto val = this->height();
    int mm_h = this->height() - HeadHeight() - BottomHeight();
    for( unsigned int i = 0 ; i < zb_windows_.size(); ++i )
    {
        if( zb_windows_[i] )
            mm_h -= zb_windows_[i]->Height();
    }
    return mm_h;
}

void KLineWall::ClearForcastData()
{ 
    auto iter = forcast_man_.Find2pForcastVector(stock_code_, k_type_, true);
    if( iter )
        iter->clear();
    auto iter0 = forcast_man_.Find2pForcastVector(stock_code_, k_type_, false);
    if( iter0 )
        iter0->clear();
    auto iter_3pdown_vector = forcast_man_.Find3pForcastVector(stock_code_, k_type_, true);
    if( iter_3pdown_vector )
        iter_3pdown_vector->clear();
    auto iter_3pup_vector = forcast_man_.Find3pForcastVector(stock_code_, k_type_, false);
    if( iter_3pup_vector )
        iter_3pup_vector->clear();
}

// calculate the Higher period type datas' high and low price from low period type data
// ps: only allow to invoke by oristep k wall
bool KLineWall::CaculateHighLowPriceForHighPeriod(TypePeriod high_type, int k_date, int hhmm, int pre_k_date, int pre_k_hhmm, int r_start, std::tuple<double, double> &re_high_low)
{
    assert(k_type_ == DEFAULT_ORI_STEP_TYPE_PERIOD);
    int r_index = FindStartKRendIndexInLowContain(high_type, *p_hisdata_container_, k_date, hhmm, pre_k_date, pre_k_hhmm, r_start);
    if( r_index == -1 )
        return false;
    const int cur_hhmm = (*(p_hisdata_container_->rbegin() + r_start))->stk_item.hhmmss;
    bool ret = false;
    double high = MIN_PRICE;
    double low = MAX_PRICE;
    auto iter = p_hisdata_container_->rbegin() + r_index;
    for( ; r_index >= 0; --r_index ) // from left to right
    {
        if( iter->get()->stk_item.date == k_date 
            && (cur_hhmm != 0 && iter->get()->stk_item.hhmmss > cur_hhmm || cur_hhmm == 0 && iter->get()->stk_item.hhmmss == 0)
          ) // cur_hhmm == 0 means overday point
            break;

        if( iter->get()->stk_item.high_price > high ) 
            high = iter->get()->stk_item.high_price;
        if( iter->get()->stk_item.low_price < low )
            low = iter->get()->stk_item.low_price;
        ret = true;
        if( iter != p_hisdata_container_->rbegin())
            --iter;
        else
            break;
    }
    re_high_low = std::make_tuple(high, low);
    return ret;
}

// ret <date, hhmm>
std::tuple<int, int> KLineWall::NextKTradeDateHHmm()
{
    T_HisDataItemContainer & items_in_container = app_->stock_data_man().GetHisDataContainer(ToPeriodType(k_type_), stock_code_);
    assert( k_rend_index_ >= 0 && !items_in_container.empty() );
    assert( k_rend_index_ < items_in_container.size() );

    int cur_date = (*(items_in_container.rbegin() + k_rend_index_))->stk_item.date;
    int cur_hhmm = (*(items_in_container.rbegin() + k_rend_index_))->stk_item.hhmmss; 
    auto date_hhmm = app_->exchange_calendar()->NextTradeDateHHmm(cur_date, cur_hhmm);
    int next_k_date = std::get<0>(date_hhmm);
    int next_k_hhmm = GetRelKHhmmTag(k_type_, std::get<1>(date_hhmm));
    if( next_k_hhmm == 0 )
    {
        next_k_date = app_->exchange_calendar()->NextTradeDate(next_k_date, 1);
        next_k_hhmm = GetRelKHhmmTag(k_type_, 0);
    }
    return std::make_tuple(next_k_date, next_k_hhmm);
}

// ret: <date, hhmm>
std::tuple<int, int> GetKDataTargetDateTime(ExchangeCalendar &exch_calender, TypePeriod type_period, QDate & date, QTime &time, int k_count)
{
    return GetKDataTargetDateTime(exch_calender, type_period, date.toString("yyyyMMdd").toInt(), time.hour() * 100 + time.minute(), k_count);
}

int CalculateSpanDays(TypePeriod type_period, int k_count)
{
    int span_day = 0;
    switch( type_period )
    {
    case TypePeriod::PERIOD_YEAR: 
        {
            if( k_count < 2 )
                span_day = -365;
            else
                span_day = -1 * k_count * 365;
        }break;
    case TypePeriod::PERIOD_MON:
        {
            if( k_count < 2 )
                span_day = -31;
            else
                span_day = -1 * (k_count * 30);
        }break;
    case TypePeriod::PERIOD_DAY:
        span_day = -1 * (k_count * 3 / 2);
        break;
    case TypePeriod::PERIOD_WEEK:
        span_day = -1 * (k_count * 30 / 20);
        break;
    case TypePeriod::PERIOD_HOUR:  // ndchk 
        {
            if( k_count * 30 / (20 * 4) < 1 )
                span_day = -1;
            else
                span_day = -1 * (k_count * 30 / (20 * 4));
            break;
        }
    case TypePeriod::PERIOD_30M:
        {
            if( k_count * 30 / (20 * 4 * 2) < 1 )
                span_day = -1;
            else
                span_day = -1 * (k_count * 30 / (20 * 4 * 2)); 
            break;
        }
    case TypePeriod::PERIOD_15M:
        {
            if( k_count * 30 / (20 * 4 * 2 * 2) < 1 )
                span_day = -1;
            else
                span_day = -1 * (k_count * 30 / (20 * 4 * 2 * 2));
            break;
        }
    case TypePeriod::PERIOD_5M:
        {
            if( k_count * 30 / (20 * 4 * 2 * 2 * 3) < 1 )
                span_day = -1;
            else
                span_day = -1 * (k_count * 30 / (20 * 4 * 2 * 2 * 3)); 
            break;
        }
    case TypePeriod::PERIOD_1M:
        {
            if( k_count * 30 / (20 * 4 * 2 * 2 * 3 * 5) < 1 )
                span_day = -1;
            else
                span_day = -1 * (k_count * 30 / (20 * 4 * 2 * 2 * 3 * 5)); 
            break;
        }
    default:assert(false);
    }
    return span_day;
}

std::tuple<int, int> GetKDataTargetDateTime(ExchangeCalendar &exch_calender, TypePeriod type_period, int end_date, int tmp_hhmm, int max_k_count)
{ 
    int pre_days = CalculateSpanDays(type_period, max_k_count);

    int start_date = exch_calender.PreTradeDate(end_date, abs(pre_days));
     
    int hhmm = GetRelKHhmmTag(type_period, tmp_hhmm);
     
    /*QDate q_date(end_date/10000, (end_date%10000)/100, end_date%100);
    int start_date = q_date.addDays(span_day).toString("yyyyMMdd").toInt();*/
    return std::make_tuple(start_date, hhmm);
}

int GetKDataTargetStartTime(TypePeriod type_period, int para_hhmm)
{ 
    int hhmm = 0;  
    switch( type_period )
    {
    case TypePeriod::PERIOD_YEAR: 
    case TypePeriod::PERIOD_MON: 
    case TypePeriod::PERIOD_DAY:
    case TypePeriod::PERIOD_WEEK: 
        break;
    case TypePeriod::PERIOD_HOUR:   
        hhmm = 1030;
        break; 
    case TypePeriod::PERIOD_30M: 
        hhmm = 1000;
        break; 
    case TypePeriod::PERIOD_15M: 
        hhmm = 945;
        break; 
    case TypePeriod::PERIOD_5M: 
        if( para_hhmm < 900 )
            hhmm = 0;
        else
            hhmm = 905;
        break; 
    case TypePeriod::PERIOD_1M: 
        if( para_hhmm < 900 )
            hhmm = 0;
        else
            hhmm = 901;
        break; 
    }
    return hhmm;
}

// ps: from p_hisdata_container back to front
int FindKRendIndex(T_HisDataItemContainer *p_hisdata_container, int date_val, int hhmm)
{
    bool is_find = false;
    int j = 0;
    int near_span = 99999;
    int near_j = -1;
    for( auto iter = p_hisdata_container->rbegin();
        iter != p_hisdata_container->rend(); 
        ++iter, ++j )
    { 
        if( iter->get()->stk_item.date == date_val && iter->get()->stk_item.hhmmss == hhmm )
        {
            is_find = true;
            break;
        }else if( iter->get()->stk_item.date == date_val && iter->get()->stk_item.hhmmss < hhmm )
        {
            near_span = date_val - iter->get()->stk_item.date;
            near_j = j;
            break;
        }
        else if( iter->get()->stk_item.date < date_val )
        {
           /* near_span = date_val - iter->get()->stk_item.date;
            near_j = j;*/
            near_j = j - 1; // edit 20200405
            break;
        }

    }
    if( is_find )
        return j;
    else
        return near_j;
}

int GetOverDayPoint(TypePeriod tp_period, int hhmm)
{
    int over_day_point = 0;
    switch(tp_period)
    {
    case TypePeriod::PERIOD_5M: over_day_point = 2355; break;
    case TypePeriod::PERIOD_15M: over_day_point = 2345; break;
    case TypePeriod::PERIOD_30M: over_day_point = 2330; break;
    case TypePeriod::PERIOD_HOUR: over_day_point = 2300; break;
    case TypePeriod::PERIOD_1M: 
    case TypePeriod::PERIOD_DAY: 
    case TypePeriod::PERIOD_WEEK: 
    case TypePeriod::PERIOD_MON: 
        over_day_point = hhmm + 1; break;
    default: break;
    }
    return over_day_point;
}

int find_over_day_index(T_HisDataItemContainer &hisdata_container, int target_day)
{
    //bool is_find = false;
    int j = 0;
    //int near_span = 99999;
    //int near_j = -1;
    auto iter = hisdata_container.rbegin(); 
    for( auto iter = hisdata_container.rbegin();
        iter != hisdata_container.rend(); 
        ++iter, ++j )
    { 
        if( target_day == iter->get()->stk_item.date && iter->get()->stk_item.hhmmss == 0 )
            return j;
    }
    return -1;
}

int find_over_day_index2right(T_HisDataItemContainer &hisdata_container, int target_day)
{
    int i = 0; 
    for( auto iter = hisdata_container.begin();
        iter != hisdata_container.end(); 
        ++iter, ++i )
    { 
        if( target_day == iter->get()->stk_item.date && iter->get()->stk_item.hhmmss == 0 )
            return i;
    }
    return -1;
}

// ps: from p_hisdata_container back to front
int FindKRendIndexInHighPeriodContain(TypePeriod tp_period, T_HisDataItemContainer &p_hisdata_container, ExchangeCalendar &calender, int date_val, int hhmm)
{ 
    int over_day_point = GetOverDayPoint(tp_period, hhmm);
    bool is_find = false;
    int j = 0;
    int near_span = 99999;
    int near_j = -1;
    auto iter = p_hisdata_container.rbegin();
     
    for( ;iter != p_hisdata_container.rend(); ++iter, ++j ) // data has been sort by left small(date and hhmm) to right big(date and hhmm)
    { 
        if( iter->get()->stk_item.date < date_val )
        {
            near_span = date_val - iter->get()->stk_item.date;
            near_j = j - 1;
            break;
        }
        if( iter->get()->stk_item.date != date_val )
            continue;

        auto my_pre_iter = iter;
        bool pre_item_exist = (iter + 1) != p_hisdata_container.rend();
        bool next_item_exist = iter != p_hisdata_container.rbegin();
        if( pre_item_exist )
        {
            my_pre_iter = iter + 1; 
        }else if( hhmm <= iter->get()->stk_item.hhmmss ) // first k 
        {
            is_find = true;
            break;
        }
        if( iter->get()->stk_item.hhmmss == hhmm )
        {
            is_find = true;
            break;
        }else if( hhmm > over_day_point )
        {
            int target_day = calender.NextTradeDate(date_val, 1);
            int targ_index = find_over_day_index(p_hisdata_container, target_day);
            return targ_index;

        }else if( pre_item_exist 
            &&   ( 
            (my_pre_iter->get()->stk_item.date == date_val && (my_pre_iter->get()->stk_item.hhmmss < hhmm && hhmm < iter->get()->stk_item.hhmmss) )
            || my_pre_iter->get()->stk_item.date < date_val 
                  ) // hhmm in current k's duration
            ) 
        {
            int k_date = iter->get()->stk_item.date;
            int k_hhmm = iter->get()->stk_item.hhmmss;
            int pre_k_date = my_pre_iter->get()->stk_item.date;
            int pre_k_hhmm = my_pre_iter->get()->stk_item.hhmmss;
            is_find = true;
            break;
        }

    } // for
    if( is_find )
        return j;
    else
        return near_j;
}


// ps: from p_hisdata_container  front + (p_hisdata_container.size() - 1 - r_start) to right forward
int FindKRendIndexInHighContain_FromRStart2Right(TypePeriod tp_period, T_HisDataItemContainer &p_hisdata_container, ExchangeCalendar &calender, int date_val, int hhmm, int r_start)
{  
    assert(r_start >= 0);
    assert(!p_hisdata_container.empty());
    assert(r_start < p_hisdata_container.size());
    bool is_find = false;
    int over_day_point = GetOverDayPoint(tp_period, hhmm);
    int near_span = 99999;
    int near_i = -1; 
    int index = p_hisdata_container.size() - 1 - r_start; 
    for( auto iter = p_hisdata_container.begin() + index; iter != p_hisdata_container.end(); ++iter, ++index ) // data has been sort by left small(date and hhmm) to right big(date and hhmm)
    { 
        if( iter->get()->stk_item.date > date_val )
        {
            near_span = iter->get()->stk_item.date - date_val;
            near_i = p_hisdata_container.size() - 1 - index;
            break;
        }
        if( iter->get()->stk_item.date != date_val )
            continue;

        auto pre_iter = iter; 
        bool pre_item_exist = iter != p_hisdata_container.begin();
        if( pre_item_exist )
            pre_iter = iter - 1;
        else if( hhmm <= iter->get()->stk_item.hhmmss ) // first k 
        {
            is_find = true;
            break;
        }
        auto next_iter = iter; 
        bool next_item_exist = (iter + 1) != p_hisdata_container.end();
        if( next_item_exist )
            next_iter = iter + 1;
        if( pre_item_exist )
            pre_iter = iter - 1;
        if( iter->get()->stk_item.hhmmss == hhmm )
        {
            is_find = true;
            break;
        }else if( hhmm > over_day_point )
        {
            int target_day = calender.NextTradeDate(date_val, 1);
            int targ_index = find_over_day_index2right(p_hisdata_container, target_day);
            if( targ_index > -1 )
                return p_hisdata_container.size() - 1 - targ_index;
            else 
                return -1;
        }else if( pre_item_exist 
                 && (
                 (pre_iter->get()->stk_item.date == date_val && (pre_iter->get()->stk_item.hhmmss < hhmm && hhmm < iter->get()->stk_item.hhmmss))
                 || (pre_iter->get()->stk_item.date < date_val && hhmm < iter->get()->stk_item.hhmmss) // consider none night trading
                 || (tp_period >= TypePeriod::PERIOD_DAY && next_item_exist && next_iter->get()->stk_item.date > date_val)
                    )
                 ) // hhmm in current k's duration
        { 
            is_find = true;
            break;
        }

    } // for
    if( is_find )
        return p_hisdata_container.size() - 1 - index;
    else
        return near_i;
}


// find related start rend_index in low container(e.g. 1m container)
// r_start is current k rend_index of low Contain
int FindStartKRendIndexInLowContain(TypePeriod high_type, T_HisDataItemContainer &container
                                               , int k_date, int hhmm, int pre_k_date, int pre_k_hhmm, int r_start)
{
    //e.g. input 30m  20200417 1345  find start rend index(rend index of 11:16) in 1m container
    //     return related index of 11:16 in 1m container 
    //   pre_k_date: 0417 1115;  0417 1145      30m:  |1115(pre_k_hhmm)|1345(hhmm)|1415;     1m : 1114|1115|1116(target)|1117
    //   pre_k_date: 0416 0230;  0417 0930;      1m:  |0230|0901(target)|0902
    //   pre_k_date: 0416 1500;  0416 2130;      1m:  |1500|2101(target)|2102
    //   pre_k_date: 0416 2330;  0417 0000       1m:  0416:2330(pre_k_hhmm)|0416:2331(target)|...|0417:0000 
    assert(r_start < container.size());
    assert(k_date > pre_k_date || k_date == pre_k_date && hhmm > pre_k_hhmm);
    
    bool is_find = false;
    int j = r_start;
    //int near_span = 99999;
    //int near_j = -1;
    auto iter = container.rbegin();
    
    iter += r_start;
    for( ;iter != container.rend(); ++iter, ++j ) // data has been sort by left small(date and hhmm) to right big(date and hhmm)
    { 
        T_StockHisDataItem &cur_item = iter->get()->stk_item;
        if( iter + 1 == container.rend() )
        {
            if( high_type >= TypePeriod::PERIOD_DAY)
            {
                if( cur_item.date == k_date )
                    return j;
                else 
                    return -1;
            }else
            {
                if( cur_item.date == k_date && cur_item.hhmmss < hhmm || cur_item.date < k_date )
                    return j;
                else 
                    return -1;
            }
        }
        T_StockHisDataItem &left_item = (iter + 1)->get()->stk_item;
        if( high_type >= TypePeriod::PERIOD_DAY)
        {
            if( left_item.date == pre_k_date && left_item.hhmmss == 1500 ) 
            {
                is_find = true;
                break;
            }
        }else
        {
            if( hhmm == 0 )// over day
            {
                if( left_item.date == pre_k_date && left_item.hhmmss <= pre_k_hhmm && cur_item.hhmmss > pre_k_hhmm )
                {
                    is_find = true;
                    break;
                }
            }else if( cur_item.date == k_date )
            { 
                if( left_item.hhmmss <= pre_k_hhmm /*&& cur_item.hhmmss > pre_k_hhmm*/ && cur_item.hhmmss < hhmm )
                {
                    is_find = true;
                    break;
                }
            }
        }
    }//for
    if( is_find )
        return j;
    else
        return -1;
}