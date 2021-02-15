#ifndef EXCHANGE_CALENDAR_SDF3SDFS_H_
#define EXCHANGE_CALENDAR_SDF3SDFS_H_

#include <vector>
#include <memory>
#include <unordered_map>
#include <tuple>

#include "sys_common.h"
 
typedef std::unordered_map<int, bool> T_DateMapIsopen;
typedef std::tuple<int, int> T_TupleIndexLen;

class ExchangeCalendar
{
public:

	 // return: yyyymmdd,days can be nigative -
    static int TodayAddDays(int days=0);
    // return: yyyymmdd,days can be nigative -
    static int DateAddDays(int date, int days);

    ExchangeCalendar();

    bool IsTradeDate(int date); // yyyymmdd
    bool IsTradeTime(int hhmm); // HHMM
    bool IsMidNightTradeTime(int hhmm); // HHMM 是否夜盘交易时间

    int CeilingTradeDate(int date);
    int FloorTradeDate(int date);
    int PreTradeDate(int date, unsigned int n);
    int NextTradeDate(int date, unsigned int n);
	 
    std::tuple<int, int> NextTradeDateHHmm(int date, int hhmm);

    int DateTradingSpan(int start_date, int end_date);
    T_TupleIndexLen GetStartIndexAndLen_backforward(TypePeriod type_period, int start_date, int end_date);

    int Translate2DateMaySmall(int val);
    int Translate2DateMayBig(int val);
    bool IsLeapYear(int Year);
    bool IsValidDate(int Year,int Month,int Day);

private:

    std::shared_ptr<T_DateMapIsopen> trade_dates_;

    //std::vector<T_CalendarDate>  calendar_date_;
    int min_trade_date_;
    int max_trade_date_;
    friend class DataBase;
};

// return: <date, hhmm> 
//pre_t: 间隔交易时段窗口数
std::tuple<int, int> PreTradeDaysOrNightsStart(ExchangeCalendar &calendar, TypePeriod type_period, int cur_date,  int cur_hhmm, int pre_t);
;
#endif