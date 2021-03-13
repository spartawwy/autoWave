
#include "strategy.h"
#include <Tlib/core/tsystem_core_common.h>

#include "exchange_calendar.h"
#include "futures_forecast_app.h"
#include "mainwindow.h"
#include "position_account.h"
#include "strategy_man.h"
#include "float_stop_profit.h"

#define TAG_STRATEGY_LOG  "strategy"
 

using namespace TSystem;
Strategy::Strategy(FuturesForecastApp &app, AccountInfo &account_info)
    : app_(app)
    , account_info_(account_info)
    , main_forcast_(nullptr)
    , sub_forcast_(nullptr)
    , main_hisdatas_(nullptr)
    , sub_hisdatas_(nullptr)
    , pre_price_(-1.0)
{
    account_info_.capital.avaliable = cst_default_ori_capital;

    main_forcast_ = app_.main_window()->MainKlineWall()->auto_forcast_man();

    code_ = app_.main_window()->MainKlineWall()->stock_code();
    main_k_type_ = app_.main_window()->MainKlineWall()->k_type();
    
    main_hisdatas_ = std::addressof(app_.stock_data_man().GetHisDataContainer(ToPeriodType(main_k_type_), code_));

    main_bounce_ups_ = std::addressof(main_forcast_->Get2pForcastVector(code_, main_k_type_, true));
    main_bounce_downs_ = std::addressof(main_forcast_->Get2pForcastVector(code_, main_k_type_, false));
    main_trends_ups_ = std::addressof(main_forcast_->Get3pForcastVector(code_, main_k_type_, false));
    main_trends_downs_ = std::addressof(main_forcast_->Get3pForcastVector(code_, main_k_type_, true));
#ifdef MAKE_SUB_WALL
    sub_forcast_ = app_.main_window()->SubKlineWall()->auto_forcast_man();
    sub_k_type_ = app_.main_window()->SubKlineWall()->k_type(); 
    sub_hisdatas_ = std::addressof(app_.stock_data_man().GetHisDataContainer(ToPeriodType(sub_k_type_), code_));
    sub_bounce_ups_ = std::addressof(sub_forcast_->Get2pForcastVector(code_, sub_k_type_, true));
    sub_bounce_downs_ = std::addressof(sub_forcast_->Get2pForcastVector(code_, sub_k_type_, false));
    sub_trends_ups_ = std::addressof(sub_forcast_->Get3pForcastVector(code_, sub_k_type_, false));
    sub_trends_downs_ = std::addressof(sub_forcast_->Get3pForcastVector(code_, sub_k_type_, true));
#endif
}

Strategy::~Strategy()
{ 
}


bool Strategy::ProdIfNearTradingEndTime(const T_QuoteData &quote)
{
    //const int hhmm = quote.hhmmss/100;
    //// close position when near by trading end time
    //bool is_near_day_trade_time_end = hhmm >= 1455 && hhmm < 2000;
    //bool is_near_night_trade_time_end = hhmm >= 225 && hhmm < 800;
    if( IsNearDayTradeTimeEnd(quote) || IsNearNightTradeTimeEnd(quote) )
    {
        bool is_next_day_rest = app_.exchange_calendar()->NextTradeDate(quote.date, 1) != app_.exchange_calendar()->DateAddDays(quote.date, 1);
        unsigned int short_pos_qty = account_info_.position.ShortPosQty();
        unsigned int long_pos_qty = account_info_.position.LongPosQty();
        // close short position----------------------------
        auto short_pos_infos = account_info_.position.ShortPosSizeInfo();
        for( auto iter = short_pos_infos.begin(); iter != short_pos_infos.end(); ++iter )
        {
            int trade_id = iter->first;
            auto pos_atom = account_info_.position.FindPositionAtom(trade_id);
            if( pos_atom )
            {
                if( pos_atom->help_info.is_remain && !is_next_day_rest )
                    continue;
                assert(pos_atom->qty_all() == 1);
                double margin_ret = 0.0;
                double p_profit = 0.0;
                account_info_.position.ClosePositionAtom(trade_id, quote.price, &margin_ret, &p_profit);
                if( short_pos_qty > long_pos_qty )
                {
                    account_info_.capital.frozen -= margin_ret;
                    account_info_.capital.avaliable += margin_ret + p_profit;
                }
                assert(short_pos_qty > 0);
                short_pos_qty -= 1; 
                app_.strategy_man()->AppendTradeRecord(OrderAction::CLOSE, *pos_atom, quote, main_hisdatas_->size() - 1);
                Strategy_Log(utility::FormatStr("[near end trading time] order stop loss or float profit SHORT pos: price:%.1f %s profit:%.1f capital:%.1f"
                    , quote.price, pos_atom->String().c_str(), p_profit, account_info_.capital.total()), &quote);
            }
        }
        // close long position----------------------------
        auto long_pos_infos = account_info_.position.LongPosSizeInfo();
        for( auto iter = long_pos_infos.begin(); iter != long_pos_infos.end(); ++iter )
        {
            int trade_id = iter->first;
            auto pos_atom = account_info_.position.FindPositionAtom(trade_id);
            if( pos_atom )
            {
                if( pos_atom->help_info.is_remain && !is_next_day_rest )
                    continue;
                assert(pos_atom->qty_all() == 1);
                double margin_ret = 0.0;
                double p_profit = 0.0;
                account_info_.position.ClosePositionAtom(trade_id, quote.price, &margin_ret, &p_profit);
                if( long_pos_qty > short_pos_qty )
                {
                    account_info_.capital.frozen -= margin_ret;
                    account_info_.capital.avaliable += margin_ret + p_profit;
                }
                assert(long_pos_qty > 0);
                long_pos_qty -= 1; 
                app_.strategy_man()->AppendTradeRecord(OrderAction::CLOSE, *pos_atom, quote, main_hisdatas_->size() - 1);
                Strategy_Log(utility::FormatStr("[near end trading time] order stop loss or float profit LONG pos: price:%.1f %s profit:%.1f capital:%.1f"
                    , quote.price, pos_atom->String().c_str(), p_profit, account_info_.capital.total()), &quote);
            }
        }
        return true;
    }else
        return false;

}
 
unsigned int Strategy::NetPosition()
{
    auto long_pos = account_info_.position.LongPosQty();
    auto short_pos = account_info_.position.ShortPosQty();
    return MAX_VAL(long_pos, short_pos);
}

void Strategy::ClosePositionAtom(const T_QuoteData &quote, int trade_id, PositionAtom *pos_atom, unsigned int short_pos_qty, unsigned int &long_pos_qty)
{
    double margin_ret = 0.0;
    double p_profit = 0.0;
    auto temp_val = long_pos_qty;
    account_info_.position.ClosePositionAtom(trade_id, quote.price, &margin_ret, &p_profit);
    if( temp_val > short_pos_qty )
    {
        account_info_.capital.frozen -= margin_ret;
        account_info_.capital.avaliable += margin_ret + p_profit;
    }else
        account_info_.capital.avaliable += p_profit;
    assert(long_pos_qty > 0);
    --long_pos_qty;
    app_.strategy_man()->AppendTradeRecord(OrderAction::CLOSE, *pos_atom, quote, main_hisdatas_->size() - 1);
    this->Strategy_Log(utility::FormatStr("Stop loss or fprofit LONG: price:%.1f %s (%s) profit:%.1f capital:%.1f"
        , quote.price, pos_atom->String().c_str()
        , pos_atom->float_stop_profit ? pos_atom->float_stop_profit->String(quote.price).c_str() : ""
        , p_profit, account_info_.capital.total()), &quote);
}

void Strategy::Strategy_Log(const std::string &content, const T_QuoteData *quote)
{
    if( quote )
        app_.local_logger().LogLocal(TAG_STRATEGY_LOG
            , utility::FormatStr("\n%s %s", GetStamp(*quote).c_str(), content.c_str()));
    else
        app_.local_logger().LogLocal(TAG_STRATEGY_LOG, content);
}

bool IsPriceNear(double price, double tag_price, const PositionAtom &pos_atom, TypePeriod type_period )
{
    auto type = JudgePriceSpreadType(type_period, pos_atom.rel_forcast_info.ab_distance);
    if( type <= PriceSpreadType::MICRO )
        return fabs(price - tag_price) < 0.2;
    /*else if( type <= PriceSpreadType::SMALL )
        return fabs(price - tag_price) < 0.3;*/
    else if( type <= PriceSpreadType::MID )
        return fabs(price - tag_price) < 0.3;
    else
        return fabs(price - tag_price) < 0.6;
}

bool IsPriceRoseNear(double price, double tag_price)
{
    if( price > tag_price - cst_tolerance_min - cst_tolerance_equal
        && price < tag_price + cst_tolerance_max + cst_tolerance_equal )
        return true;
    else
        return false;
}

bool IsPriceFallNear(double price, double tag_price)
{
    if( price < tag_price + cst_tolerance_min + cst_tolerance_equal
        && price > tag_price - cst_tolerance_max - cst_tolerance_equal )
        return true;
    else
        return false;
}


std::string GetStamp(const T_QuoteData &quote)
{
    return utility::FormatStr("[%04d %02d:%02d:%02d %02d]"
        , quote.date, quote.hhmmss/10000, quote.hhmmss%10000/100, quote.hhmmss%100, quote.msecond);
}

bool compare_sort_desc(const RelForcastInfo &lh, const RelForcastInfo& rh)
{
    return lh.ab_distance > rh.ab_distance; // order by desc
}

bool compare_sort_asc(const RelForcastInfo &lh, const RelForcastInfo& rh)
{
    return lh.ab_distance < rh.ab_distance; // order by asc
}

bool IsNightTradingBegTime(const T_QuoteData &quote)
{
    const int hhmm = quote.hhmmss/100;
    return ( hhmm >= 2058 && hhmm < 2105 );
}

bool IsNearDayTradeTimeEnd(const T_QuoteData &quote)
{
    const int hhmm = quote.hhmmss/100;
    // close position when near by trading end time
    return hhmm >= 1455 && hhmm < 2000; 
}

bool IsNearNightTradeTimeEnd(const T_QuoteData &quote)
{
    const int hhmm = quote.hhmmss/100;
    // close position when near by trading end time 
    return hhmm >= 225 && hhmm < 800;
}


