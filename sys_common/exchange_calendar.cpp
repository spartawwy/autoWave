#include "exchange_calendar.h"

#include <ctime>
#include <chrono>
#include <TLib/core/tsystem_time.h>
 
ExchangeCalendar::ExchangeCalendar() : min_trade_date_(0), max_trade_date_(0)
{ 
    trade_dates_ = std::make_shared<T_DateMapIsopen>(10*1024);
}

bool ExchangeCalendar::IsTradeDate(int date)
{
     assert(trade_dates_->size() > 0);

     T_DateMapIsopen &date_map_opend = *trade_dates_;
     auto iter = date_map_opend.find(date);
     return iter != date_map_opend.end() && iter->second;
}

bool ExchangeCalendar::IsTradeTime(int hhmm)
{ 
    //return (hhmm >= 900 &&  hhmm <= 1500) || IsMidNightTradeTime(hhmm);
    return (hhmm >= 900 &&  hhmm <= 1130) || (hhmm >= 1330 && hhmm <= 1500) || IsMidNightTradeTime(hhmm);
}

bool ExchangeCalendar::IsMidNightTradeTime(int hhmm)
{
    return hhmm >= 2100 || hhmm <= 230;
}


// ps: ceiling trade date may be bigger then param date. if fail return 0
int ExchangeCalendar::CeilingTradeDate(int date)
{
    assert(trade_dates_->size() > 0); 
    T_DateMapIsopen &date_map_opend = *trade_dates_;
    int a = 0;
    for( int i = 0; i < 30; ++i )
    {
        a = DateAddDays(date, i);
        if( a > max_trade_date_ )
            return 0;
        auto iter = date_map_opend.find(a);
        if( iter != date_map_opend.end() && iter->second )
           return a;
    }
    return 0;
}

// ps: ceiling trade date may be smaller then param date. if fail return 0
int ExchangeCalendar::FloorTradeDate(int date)
{
    assert(trade_dates_->size() > 0); 
    T_DateMapIsopen &date_map_opend = *trade_dates_;
    int a = 0;
    for( int i = 0; i < 30; ++i )
    {
        a = DateAddDays(date, -1 * i);
        if( a < min_trade_date_ )
            return 0;
        auto iter = date_map_opend.find(a);
        if( iter != date_map_opend.end() && iter->second )
            return a;
    }
    return 0;
}

// pre n trade date . ps: each day of n is trading day
int ExchangeCalendar::PreTradeDate(int date, unsigned int n)
{   
    assert(trade_dates_->size() > 0);
    // auto date_time_point = TSystem::MakeTimePoint(date/10000, (date % 10000) / 100, date % 100);
    unsigned int count = 0;
    int i = 1;
    T_DateMapIsopen &date_map_opend = *trade_dates_;
    int a = 0;
    while( count < n )
    {
        a = DateAddDays(date, -1 * i);  
        if( a < min_trade_date_ )
            return 0;
        auto iter = date_map_opend.find(a);
        if( iter != date_map_opend.end() && iter->second )
            ++count;
        ++i;
    }
    return a;
}

// each day of n is trading day
int ExchangeCalendar::NextTradeDate(int date, unsigned int n)
{   
    assert(trade_dates_->size() > 0);
    //auto date_time_point = TSystem::MakeTimePoint(date/10000, (date % 10000) / 100, date % 100);
    unsigned int count = 0;
    int i = 1;
    T_DateMapIsopen &date_map_opend = *trade_dates_;
    int a = 0;
    if( n > 0 )
    {
        while( count < n )
        {
            a = DateAddDays(date, i);  
            if( a > max_trade_date_ )
                return 0;
            auto iter = date_map_opend.find(a);
            if( iter != date_map_opend.end() && iter->second )
                ++count;
            ++i;
        }
    }else
    {
        if( IsTradeDate(date) )
            a = date;
        else
            a = CeilingTradeDate(date);
    }
    return a;
}

// return <date, hhmm>
std::tuple<int, int> ExchangeCalendar::NextTradeDateHHmm(int date, int hhmm)
{
    //struct tm t1,t2;    //结构体 
    //t1.tm_year -= 1900;
    //t1.tm_mon -= 1;
    //t1.tm_sec = 0; 
    //time_t t = mktime(&t1);
     
     static auto is_no_midnight_date = [](int date)->bool
     {
         // ps: sort from small to big
         int special_dates[] = {20180427, 20180615, 20180928, 20181228, 20190430, 20190930, 20191231, 20200403
                                , 20200430, 20200930};
         bool is_exists = std::binary_search(special_dates, special_dates + sizeof(special_dates)/sizeof(special_dates[0]), date);
         /*for( int i = 0; i < sizeof(special_no_midnight)/sizeof(special_no_midnight[0]); ++i )
         {

         }*/
         return is_exists;
     };

     int target_date = 0;
     int target_hhmm = 0;
     if( !IsTradeDate(date) )
     {
         target_date = NextTradeDate(date, 1);
         target_hhmm = 900;
     }
     else
     {
         if( is_no_midnight_date(date) && hhmm >= 1500 )
         {
             target_date = NextTradeDate(date, 1);
             target_hhmm = 900;
         }else if( hhmm == 2359 )
         {
             target_date = NextTradeDate(date, 1);
             target_hhmm = 0;
         }else
         {
             int hour = hhmm / 100;
             int minute = hhmm % 100;
             auto t_point = TSystem::MakeTimePoint(date/10000, date%10000/100, date%100, hhmm/100, hhmm%100);
             auto target_point = t_point + std::chrono::minutes(1);
             TSystem::TimePoint tp(target_point);
             target_date = tp.year() * 10000 + tp.month() * 100 + tp.day();
             target_hhmm = tp.hour() * 100 + tp.minute(); 
         }
     }
    return std::make_tuple(target_date, target_hhmm);
}

// 获得当前交易窗口(日盘或夜盘)剩余5m K线数目
int Get5mLackKnum(int hhmm)
{
    int lack_k_num = 0;
    // calculate Cur tranding day's now k num
    if( hhmm <= 230 )
    { 
        int already_num = ((hhmm / 100) * 60 / 5) + ((hhmm % 100) + 5) / 5;
        lack_k_num = 111 - 66 + (30 - already_num);
    }else if( hhmm < 900 )
    {
        // such as 05:55
        lack_k_num = 111 - 66; 
    }else if( hhmm <= 1015 )
    {
        lack_k_num = 111 - 66 - (hhmm/100 - 9) * 12 - (hhmm%100 / 5 + 1);
    }else if( hhmm <= 1130 )
    {
        if( hhmm/100 < 11 ) // 1035-1059
            lack_k_num = 111 - 66 - 15 - ( (hhmm%100 - 30) / 5 + 1);
        else
            lack_k_num = 111 - 66 - 15 - 5 - ( hhmm%100 / 5 + 1);
    }else if( hhmm < 1330 )
    {
        lack_k_num = 111- 66 -27;
    }else if( hhmm < 1400 )
    {
        lack_k_num = 111- 66 - 27 - ((hhmm%100 - 30) / 5  + 1);
    }else if( hhmm < 1500 )
    {
        lack_k_num = 111- 66 - 27 - 5 - (hhmm%100 / 5 + 1);
    }else if ( hhmm < 2100 )
    {
        lack_k_num = 0;
    }else  // 2100-2359
    {
        int already_num = ((hhmm / 100 - 21) * 60 / 5) + ((hhmm % 100) + 5) / 5;
        lack_k_num = 111 - already_num;
    }
    return lack_k_num;
}


int Get1mLackKnum(int hhmm)
{
    int lack_k_num = 0;
    // calculate Cur tranding day's now k num
    if( hhmm <= 230 )
    { 
        int already_num = ((hhmm / 100) * 60) + (hhmm % 100);
        lack_k_num = (111 - 66)*5 + (30*5 - already_num);
    }else if( hhmm < 900 )
    {
        // such as 05:55
        lack_k_num = (111 - 66)*5; 
    }else if( hhmm <= 1015 )
    {
        lack_k_num = (111 - 66)*5 - (hhmm/100 - 9) * 60 - hhmm%100;
    }else if( hhmm <= 1130 ) // 1035-1130
    {
        if( hhmm/100 < 11 ) // 1035-1059
            lack_k_num = (111 - 66 - 15)*5 - (hhmm%100 - 30);
        else // 1100-1130
            lack_k_num = (111 - 66 - 15 - 5)*5 - hhmm%100;
    }else if( hhmm < 1330 )
    {
        lack_k_num = (111- 66 -27)*5;
    }else if( hhmm < 1400 )
    {
        lack_k_num = (111- 66 -27)*5 - (hhmm%100 - 30);
    }else if( hhmm < 1500 )
    {
        lack_k_num = (111- 66 - 27 - 5)*5 - hhmm%100;
    }else if ( hhmm < 2100 )
    {
        lack_k_num = 0;
    }else  // 2100-2359
    {
        int already_num = ((hhmm / 100 - 21) * 60) + (hhmm % 100);
        lack_k_num = 111*5 - already_num;
    }
    return lack_k_num;
}

// ps: end_date <= today .  inner use current time
T_TupleIndexLen ExchangeCalendar::GetStartIndexAndLen_backforward(TypePeriod type_period, int start_date, int end_date)
{ 
    assert(trade_dates_->size() > 0);
    assert(start_date <= end_date);

    time_t rawtime;
    struct tm timeinfo;
    time( &rawtime );
    localtime_s(&timeinfo, &rawtime); 
    const int hhmm = timeinfo.tm_hour * 100 + timeinfo.tm_min;

    int today = TSystem::Today();
    
   
    int lack_k_num = 0;
    if( IsTradeDate(today) )
    {
        if( type_period == TypePeriod::PERIOD_5M )
        {
            lack_k_num = Get5mLackKnum(hhmm);
        }else if( type_period == TypePeriod::PERIOD_1M )
        {
            lack_k_num = Get1mLackKnum(hhmm);
        }
    }
    // after 2100 procedure as next trade day
    if( hhmm >= 2100 )
    {
        today = DateAddDays(today, 1);
    }
    const int latest_trade_date = FloorTradeDate(today);

    int actual_start_date = CeilingTradeDate(start_date);

    int actual_end_date = end_date;
    if( actual_end_date >= today )
        actual_end_date = today;
    actual_end_date = FloorTradeDate(actual_end_date);

    int start_index = DateTradingSpan(actual_end_date, latest_trade_date);

    int span_len = DateTradingSpan(actual_start_date, actual_end_date) + 1;
     
    switch( type_period )
    {
    case TypePeriod::PERIOD_DAY: break;
    case TypePeriod::PERIOD_WEEK:
        if( start_index > 5 )
            start_index /= 5;
        if( span_len > 5 )
            span_len /= 5;
        break;
    case TypePeriod::PERIOD_HOUR:
       /* start_index *= 4;
        span_len *= 4;*/
         start_index *= 10;
        span_len *= 10;
        break;
    case TypePeriod::PERIOD_30M:
        /* start_index *= 8;
        span_len *= 8;*/
        start_index *= 19;
        span_len *= 19;
        break;
    case TypePeriod::PERIOD_15M:
        /*start_index *= 16;
        span_len *= 16;*/
        start_index *= 37;
        span_len *= 37;
        break;
    case TypePeriod::PERIOD_5M:
        start_index *= 111;
        span_len *= 111;
        break;
    case TypePeriod::PERIOD_1M:
        start_index *= 555;
        span_len *= 555;
        break;
    }
    if( start_index >= lack_k_num )
    {
        start_index -= lack_k_num;
    }
    if( span_len >= lack_k_num )
    {
        span_len -= lack_k_num;
    }
    return std::make_tuple(start_index, span_len);
}

// return span of trading dates between
// ps: start_date <= end_date
int  ExchangeCalendar::DateTradingSpan(int start_date, int end_date)
{
    assert(start_date <= end_date);

    if( start_date == end_date ) 
        return 0;
    int target_end_date = end_date;
    if( !IsTradeDate(target_end_date) )
        target_end_date = PreTradeDate(target_end_date, 1);

    int target_start_date = start_date;
    if( !IsTradeDate(target_start_date) )
        target_start_date = NextTradeDate(target_start_date, 1);
    if( target_end_date <= target_start_date ) 
        return 0;

    T_DateMapIsopen &date_map_opend = *trade_dates_;
    int tmp_date = 0;
    unsigned int span = 0;
    int i = 1;
    do{
        tmp_date = DateAddDays(target_end_date, -1 * i++);  
        auto iter = date_map_opend.find(tmp_date);
        if( iter != date_map_opend.end() && iter->second )
            ++span;
    }while( target_start_date < tmp_date );

    return span;
}

int ExchangeCalendar::TodayAddDays(int days)
{
    std::time_t day_t = 0;
    using namespace std::chrono;
    system_clock::time_point now = system_clock::now();
    if( days >= 0 )
        day_t = system_clock::to_time_t(now + std::chrono::hours(24*days));
    else
        day_t = system_clock::to_time_t(now - std::chrono::hours(24*abs(days)));
    tm tm_day_t;
    _localtime64_s(&tm_day_t, &day_t);
    return (tm_day_t.tm_year + 1900) * 10000 + (tm_day_t.tm_mon + 1) * 100 + tm_day_t.tm_mday;
}

int ExchangeCalendar::DateAddDays(int date, int days)
{
    auto date_time_point = TSystem::MakeTimePoint(date/10000, (date % 10000) / 100, date % 100);
    std::time_t day_t = 0;

    using namespace std::chrono;
    if( days >= 0 )
        day_t = system_clock::to_time_t(date_time_point + std::chrono::hours(24*days));
    else
        day_t = system_clock::to_time_t(date_time_point - std::chrono::hours(24*abs(days)));
    tm tm_day_t;
    _localtime64_s(&tm_day_t, &day_t);
    return (tm_day_t.tm_year + 1900) * 10000 + (tm_day_t.tm_mon + 1) * 100 + tm_day_t.tm_mday;
}
 
// ret: <= val;  ret == 0 means false
int ExchangeCalendar::Translate2DateMaySmall(int val)
{   //date/10000, (date % 10000) / 100, date % 100
    int year = val / 10000;
    int mon = (val % 10000) / 100;
    int day = val % 100;
    if( year < 1700 )
        return 0;
    if( year > 3000 )
        return 0;
    if( mon < 1 )
    {
        year -= 1;
        mon = 12;
    }
    if( mon > 12 )
        mon = 12;
    if( day < 1 )
    {
        mon -= 1;
        if( mon < 1 )
        {
            year -= 1;
            mon = 12;
        } 
        day = 31;
    }
    if( day > 31 )
        day = 31;
    while( !IsValidDate(year, mon, day) )
    {
        day -= 1;
        if( day < 1 )
        {
            mon -= 1;
            if( mon < 1 )
            {
                year -= 1;
                mon = 12;
            } 
            day = 31;
        }
    }
    return year * 10000 + mon * 100 + day;
}
 
// ret: >= val;  ret == 0 means false
int ExchangeCalendar::Translate2DateMayBig(int val)
{   //date/10000, (date % 10000) / 100, date % 100
    int year = val / 10000;
    int mon = (val % 10000) / 100;
    int day = val % 100;
    if( year < 1700 )
        return 0;
    if( year > 3000 )
        return 0;
    if( mon < 1 || mon > 12 )
    {
        year += 1;
        mon = 1;
    }
     
    if( day < 1 )
    {
        mon += 1;
        if( mon > 12 )
        {
            year += 1;
            mon = 1;
        } 
        day = 1;
    }
    if( day > 31 )
        day = 31;
    while( !IsValidDate(year, mon, day) )
    {
        day += 1;
        if( day > 31 )
        {
            mon += 1;
            if( mon > 12 )
            {
                year += 1;
                mon = 1;
            } 
            day = 1;
        }
    }
    return year * 10000 + mon * 100 + day;
}

bool ExchangeCalendar::IsLeapYear(int Year)
{
    return ((Year % 4 == 0 && Year % 100 != 0 ) || Year % 400 == 0);
}

bool ExchangeCalendar::IsValidDate(int Year,int Month,int Day)
{
    int nDay;
    if ( Year < 1 || Month >12 || Month < 1 || Day < 1 ) 
        return false;

    switch( Month ) {
    case 4:
    case 6:
    case 9:
    case 11:
        nDay = 30;
        break;
    case 2:
        if(IsLeapYear(Year))
            nDay = 29;
        else
            nDay = 28;
        break;
    default:
        nDay = 31;
    }

    if( Day > nDay )
        return false; 
    return true;
}


// return: <date, hhmm> 
//pre_t: 间隔交易时段窗口数
std::tuple<int, int> PreTradeDaysOrNightsStart(ExchangeCalendar &calendar, TypePeriod type_period, int cur_date,  int cur_hhmm, int pre_t)
{
    assert(pre_t >= 0);
    const int t = pre_t;
    int target_date = cur_date;
    int tartet_hhmm = 905;

    int tgt_day_start_hhmm = 905;
    int tgt_night_start_hhmm = 2105;
    switch (type_period)
    {
    case TypePeriod::PERIOD_1M: tgt_day_start_hhmm = 901; tgt_night_start_hhmm = 2101; break;
    case TypePeriod::PERIOD_5M: tgt_day_start_hhmm = 905; tgt_night_start_hhmm = 2105; break;
    case TypePeriod::PERIOD_15M: tgt_day_start_hhmm = 915; tgt_night_start_hhmm = 2115; break;
    case TypePeriod::PERIOD_30M:tgt_day_start_hhmm = 930; tgt_night_start_hhmm = 2130; break;
    case TypePeriod::PERIOD_HOUR:tgt_day_start_hhmm = 930; tgt_night_start_hhmm = 2200; break;
    case TypePeriod::PERIOD_DAY: tgt_day_start_hhmm = 0; tgt_night_start_hhmm = 0; break;
    default: break;
    }
    if( cur_hhmm > 2100 ) // night trading time
    {
        target_date = calendar.PreTradeDate(cur_date, t / 2);
        tartet_hhmm = (t % 2 == 0 ? tgt_night_start_hhmm : tgt_day_start_hhmm);

    }else if( cur_hhmm > 0 && cur_hhmm <= 230 ) // mid-night trading time
    {
        target_date = calendar.PreTradeDate(cur_date, t / 2 + 1);
        tartet_hhmm = (t % 2 == 0 ? tgt_night_start_hhmm : tgt_day_start_hhmm); 
    }else if( cur_hhmm >= 900 && cur_hhmm < 2100 ) //day trading time
    {
        target_date = calendar.PreTradeDate(cur_date, (t+1) / 2 );
        tartet_hhmm = (t % 2 == 0 ? tgt_day_start_hhmm : tgt_night_start_hhmm); 
        //if( t == 1 ) 
        //{    int pre_trade_date = calendar.PreTradeDate(cur_date, /*-1*/ (t+1)/2 *(-1) );
        //tartet_hhmm = 2105;}
        //if( t == 2 )
        // {   int pre_trade_date = calendar.PreTradeDate(cur_date, /*-1*/ (t+1)/2 *(-1) );
        //    tartet_hhmm = 905;}
        //if( t == 3 )
        //{        int pre_trade_date = calendar.PreTradeDate(cur_date, /*-2*/ (t+1)/2 *(-1) );
        //    tartet_hhmm = 2105;}
        // if( t == 4 )
        // {       int pre_trade_date = calendar.PreTradeDate(cur_date, /*-2*/ (t+1)/2 *(-1) );
        //    tartet_hhmm = 905;}
        //  if( t == 5 ) 
        //  {      int pre_trade_date = calendar.PreTradeDate(cur_date, /*-2*/ (t+1)/2 *(-1) );
        //    tartet_hhmm = 2105;
        //  } 
    }
    assert(target_date > 1985);
    return std::make_tuple(target_date, tartet_hhmm);
}
