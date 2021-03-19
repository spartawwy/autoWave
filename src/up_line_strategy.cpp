#include "up_line_strategy.h"

#include <Tlib/core/tsystem_core_common.h>

#include "futures_forecast_app.h"
#include "strategy_man.h"
#include "trend_line.h"


using namespace TSystem;
TrendUpLineStrategy::TrendUpLineStrategy(FuturesForecastApp &app, AccountInfo &account_info, std::shared_ptr<TrendLine> &trend_line) : Strategy(app, account_info)
    , trend_line_(trend_line)
{
    
}

void TrendUpLineStrategy::Handle(const T_QuoteData &quote)
{
    if( pre_price_ < 0.0 )
        goto END_PROC;

    if( Equal(pre_price_, quote.price) )
        goto END_PROC;

    if( ProdIfNearTradingEndTime(quote) )
        return;

    T_HisDataItemContainer &k_hisdata = *main_hisdatas_;

    if( quote.price > pre_price_ ) // price rise
    { 
        JudgeStopLongProfit(quote);  

        unsigned int short_pos_qty = account_info_.position.ShortPosQty();
        unsigned int long_pos_qty = account_info_.position.LongPosQty();
        JudgeStopShortLoss(quote, short_pos_qty, long_pos_qty);
        JudgeOpenShort(quote, short_pos_qty, long_pos_qty);
    } 
    else // price fall
    {  
        JudgeStopShortProfit(quote);
        
        unsigned int short_pos_qty = account_info_.position.ShortPosQty();
        unsigned int long_pos_qty = account_info_.position.LongPosQty();
        JudgeStopLongLoss(quote, short_pos_qty, long_pos_qty);

        JudgeOpenLong(quote, short_pos_qty, long_pos_qty);
        
    }
END_PROC:

    pre_price_ = quote.price;
}
 
void TrendUpLineStrategy::JudgeOpenLong(const T_QuoteData &quote, unsigned int short_pos_qty, INOUT unsigned int &long_pos_qty)
{
    assert(trend_line_);
    if( !trend_line_->is_alive_ || !trend_line_->is_last_ )
        return;
    if( long_pos_qty >= cst_position_max )
        return;
    T_HisDataItemContainer &k_hisdata = *main_hisdatas_;
    int cur_k_index = k_hisdata.size() - 1;
    if( cur_k_index < 2 )
        return;
    if( long_records_.find(cur_k_index) != long_records_.end() )
        return;
    const double line_value = TrendLineValue(k_hisdata, *trend_line_, cur_k_index);

    const double pre_pre_index_line_value = TrendLineValue(k_hisdata, *trend_line_, cur_k_index-2);
    const double pre_index_line_value = TrendLineValue(k_hisdata, *trend_line_, cur_k_index-1);
    const double cur_line_value = TrendLineValue(k_hisdata, *trend_line_, cur_k_index);

    if( k_hisdata[cur_k_index-2]->stk_item.low_price > pre_pre_index_line_value 
        && k_hisdata[cur_k_index-1]->stk_item.low_price > pre_index_line_value  
        && quote.price < line_value + 0.2 + cst_tolerance_equal && quote.price > line_value - cst_tolerance_equal )
    {
        unsigned int net_qty = MAX_VAL(short_pos_qty, long_pos_qty);
        auto frozen_capital = net_qty > 0 ? CaculateOpenPositionFreezeCapital(quote.price, net_qty) : 0.0;
        auto pos_capital_allow_n = (unsigned int)CalculateMaxQtyAllowOpen(account_info_.capital.avaliable + frozen_capital, quote.price);
        assert(pos_capital_allow_n >= long_pos_qty);
        auto remain_long_allow = pos_capital_allow_n - long_pos_qty;
        assert(cst_position_max >= long_pos_qty);
        unsigned int real_long_pos_allow = MIN_VAL(remain_long_allow, cst_position_max - long_pos_qty);
        if( cur_k_index == 3107 || cur_k_index == 3942 )
            cur_k_index = cur_k_index;
        bool is_long = true;
        for( unsigned int j = 0; j < real_long_pos_allow; ++j )
        {
            auto positon_atom = std::make_shared<PositionAtom>();
            positon_atom->qty_available = 1;
            //positon_atom->rel_forcast_info = rel_info;
            positon_atom->price = quote.price;
            positon_atom->is_long = is_long;
            positon_atom->help_info.strategy_id = id_;
            switch(j)
            { 
            case 0:  
                positon_atom->stop_profit_price = positon_atom->price + 19.8; 
                if( account_info_.position.PositionCountRemainedForRunProfit(is_long) < 1 )
                    positon_atom->help_info.is_remain = true;
                break;
            case 1: positon_atom->stop_profit_price = positon_atom->price + 9.9; break;
            case 2: positon_atom->stop_profit_price = positon_atom->price + 4.9; break;
            default: positon_atom->stop_profit_price = positon_atom->price + 4.9; break;
            }
            positon_atom->trade_id = account_info_.position.GenerateTradeId();
            account_info_.position.PushBack(is_long, positon_atom);
            double margin = CaculateOpenPositionMargin(positon_atom->price, positon_atom->qty_available);
            double fee = CalculateFee(positon_atom->qty_available, positon_atom->price, false);
            if( long_pos_qty >= short_pos_qty ) // ps: 考虑多空 共享保证金
            {
                account_info_.capital.avaliable -= margin + fee;
                account_info_.capital.frozen += margin;
            }else
                account_info_.capital.avaliable -= fee;
            long_pos_qty += 1;
            app_.strategy_man()->AppendTradeRecord(OrderAction::OPEN, *positon_atom, quote, cur_k_index); 
            Strategy_Log(utility::FormatStr("order open LONG pos: %s fee:%.1f frozen:%.1f ava:%.1f line_id:%d line_value:%.1f index:%d"
                , positon_atom->String(false).c_str(), fee, account_info_.capital.frozen, account_info_.capital.avaliable, trend_line_->id_, line_value, cur_k_index), &quote);
        }
        if( real_long_pos_allow > 0 )
            long_records_[cur_k_index] = true;
    }
}

void TrendUpLineStrategy::JudgeStopLongLoss(const T_QuoteData &quote, unsigned int short_pos_qty, INOUT unsigned int &long_pos_qty)
{
    T_HisDataItemContainer &k_hisdata = *main_hisdatas_;
    int cur_k_index = k_hisdata.size() - 1;
     
    const double line_value = TrendLineValue(k_hisdata, *trend_line_, cur_k_index);

    double key_price = MIN_PRICE;
    for( unsigned int i = 0; i < trend_line_->breakdown_infos_.size(); ++i )
    {
        if( trend_line_->breakdown_infos_[i].k_index == cur_k_index )
            continue;
        key_price = MAX_VAL(k_hisdata[trend_line_->breakdown_infos_[i].k_index]->stk_item.low_price, key_price);
    }
    bool is_to_close = quote.price < key_price;
    if( !is_to_close )
    {
        if( IsNearCloseKTime(quote.hhmmss, main_k_type_) && quote.price < line_value - 0.6 + EPSINON )
            is_to_close = true;
    }

    auto pos_infos = account_info_.position.LongPosSizeInfo();
    for( auto iter = pos_infos.begin(); iter != pos_infos.end(); ++iter )
    {
        int trade_id = iter->first;
        auto pos_atom = account_info_.position.FindPositionAtom(trade_id);
        if( !pos_atom || pos_atom->help_info.strategy_id != id_ )
            continue;
       
        if( is_to_close || quote.price < line_value - 1.0 + EPSINON )
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
            Strategy_Log(utility::FormatStr("order stop loss or float profit LONG pos: price:%.1f %s profit:%.1f capital:%.1f line_id:%d line_value:%.1f index:%d"
                , quote.price, pos_atom->String(false).c_str(), p_profit, account_info_.capital.total(), trend_line_->id_, line_value, cur_k_index), &quote);
        } 
    } 
    
}

void TrendUpLineStrategy::JudgeStopLongProfit(const T_QuoteData &quote)
{
    T_HisDataItemContainer &k_hisdata = *main_hisdatas_;
    unsigned int long_pos_qty = account_info_.position.LongPosQty();
    unsigned int short_pos_qty = account_info_.position.ShortPosQty();
    // consider stop profit of long position --------------
    auto pos_infos = account_info_.position.LongPosSizeInfo();
    for( auto iter = pos_infos.begin(); iter != pos_infos.end(); ++iter )
    {
        int trade_id = iter->first;
        auto pos_atom = account_info_.position.FindPositionAtom(trade_id);
        if( !pos_atom || pos_atom->help_info.strategy_id != id_ )
            continue;
        assert(pos_atom->qty_all() == 1);
        if( quote.price > pos_atom->stop_profit_price - cst_tolerance_equal )
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
            long_pos_qty -= 1;
            app_.strategy_man()->AppendTradeRecord(OrderAction::CLOSE, *pos_atom, quote, main_hisdatas_->size() - 1);
            Strategy_Log(utility::FormatStr("order stop profit LONG pos: price:%.1f %s profit:%.1f capital:%.1f"
                , quote.price, pos_atom->String(false).c_str(), p_profit, account_info_.capital.total()), &quote);
        }
    }
}


void TrendUpLineStrategy::JudgeOpenShort(const T_QuoteData &quote, INOUT unsigned int &short_pos_qty, unsigned int long_pos_qty)
{
    if( !trend_line_->is_alive_ || !trend_line_->is_last_ )
        return;
    if( short_pos_qty >= cst_position_max )
        return;
    T_HisDataItemContainer &k_hisdata = *main_hisdatas_;
    int cur_k_index = k_hisdata.size() - 1;
    if( cur_k_index < 2 )
        return;
    if( short_records_.find(cur_k_index) != short_records_.end() )
        return;
    double max_back_rate = 0.5;

    const double pre_pre_index_line_value = TrendLineValue(k_hisdata, *trend_line_, cur_k_index-2);
    const double pre_index_line_value = TrendLineValue(k_hisdata, *trend_line_, cur_k_index-1);
    const double cur_line_value = TrendLineValue(k_hisdata, *trend_line_, cur_k_index);

    if( k_hisdata[cur_k_index-2]->stk_item.close_price > pre_pre_index_line_value - EPSINON 
        && k_hisdata[cur_k_index-1]->stk_item.close_price < pre_index_line_value 
        && k_hisdata[cur_k_index]->stk_item.low_price < cur_line_value - 0.2 
       && quote.price > cur_line_value - 0.1 - cst_tolerance_equal && quote.price < cur_line_value + cst_tolerance_equal )
    {
        unsigned int net_qty = MAX_VAL(short_pos_qty, long_pos_qty);
        auto frozen_capital = net_qty > 0 ? CaculateOpenPositionFreezeCapital(quote.price, net_qty) : 0.0;
        auto pos_capital_allow_n = (unsigned int)CalculateMaxQtyAllowOpen(account_info_.capital.avaliable + frozen_capital, quote.price);
        assert(pos_capital_allow_n >= short_pos_qty);
        auto remain_short_allow = pos_capital_allow_n - short_pos_qty;
        assert(cst_position_max >= short_pos_qty);
        unsigned int real_short_pos_allow = MIN_VAL(remain_short_allow, cst_position_max - short_pos_qty);
         
        bool is_long = false;
        for( unsigned int j = 0; j < real_short_pos_allow; ++j )
        {
            auto positon_atom = std::make_shared<PositionAtom>();
            positon_atom->qty_available = 1;
            //positon_atom->rel_forcast_info = rel_info;
            positon_atom->price = quote.price;
            positon_atom->is_long = is_long;
            positon_atom->help_info.strategy_id = id_;
            switch(j)
            { 
            case 0:  
                //positon_atom->stop_profit_price = positon_atom->price - 19.8; 
                if( account_info_.position.PositionCountRemainedForRunProfit(is_long) < 1 )
                    positon_atom->help_info.is_remain = true;
                positon_atom->stop_profit_price = 1.0; //
                positon_atom->float_stop_profit = std::make_shared<FollowStopProfit>(*positon_atom, 3.9, max_back_rate, positon_atom->price - 19.8);
                break;
            case 1: 
                positon_atom->stop_profit_price = 1.0; //
                positon_atom->float_stop_profit = std::make_shared<FollowStopProfit>(*positon_atom, 2.0, max_back_rate, positon_atom->price - 4.9);
                break;
            case 2: positon_atom->stop_profit_price = positon_atom->price - 2.0; break;
            default: positon_atom->stop_profit_price = positon_atom->price - 2.0; break;
            }
            positon_atom->stop_loss_price = cur_line_value + 0.7; //positon_atom->price + 1.5;
            positon_atom->trade_id = account_info_.position.GenerateTradeId();
            account_info_.position.PushBack(is_long, positon_atom);
            double margin = CaculateOpenPositionMargin(positon_atom->price, positon_atom->qty_available);
            double fee = CalculateFee(positon_atom->qty_available, positon_atom->price, false);
            if( short_pos_qty >= long_pos_qty ) // ps: 考虑多空 共享保证金
            {
                account_info_.capital.avaliable -= margin + fee;
                account_info_.capital.frozen += margin;
            }else
                account_info_.capital.avaliable -= fee;
            short_pos_qty += 1;
            app_.strategy_man()->AppendTradeRecord(OrderAction::OPEN, *positon_atom, quote, cur_k_index); 
            Strategy_Log(utility::FormatStr("order open SHORT pos: %s fee:%.1f frozen:%.1f ava:%.1f "
                , positon_atom->String(false).c_str(), fee, account_info_.capital.frozen, account_info_.capital.avaliable), &quote);
        }//for
        if( real_short_pos_allow > 0 )
            short_records_[cur_k_index] = true;
    }
}
  
void TrendUpLineStrategy::JudgeStopShortLoss(const T_QuoteData &quote, INOUT unsigned int &short_pos_qty, unsigned int long_pos_qty)
{ 
    T_HisDataItemContainer &k_hisdata = *main_hisdatas_;
    int cur_k_index = k_hisdata.size() - 1;
    auto pos_infos = account_info_.position.ShortPosSizeInfo();
    for( auto iter = pos_infos.begin(); iter != pos_infos.end(); ++iter )
    {
        int trade_id = iter->first;
        auto pos_atom = account_info_.position.FindPositionAtom(trade_id);
        if( !pos_atom || pos_atom->help_info.strategy_id != id_ )
            continue; 
        std::string float_stop_ret;
        if( quote.price > pos_atom->stop_loss_price - cst_tolerance_equal )
        {
            double margin_ret = 0.0;
            double p_profit = 0.0;
            auto temp_val = short_pos_qty;
            account_info_.position.ClosePositionAtom(trade_id, quote.price, &margin_ret, &p_profit);
            if( temp_val > long_pos_qty )
            {
                account_info_.capital.frozen -= margin_ret;
                account_info_.capital.avaliable += margin_ret + p_profit;
            }else
                account_info_.capital.avaliable += p_profit;
            assert(short_pos_qty > 0);
            --short_pos_qty;
            app_.strategy_man()->AppendTradeRecord(OrderAction::CLOSE, *pos_atom, quote, main_hisdatas_->size() - 1);
            Strategy_Log(utility::FormatStr("order stop loss or float profit short pos: price:%.1f %s profit:%.1f capital:%.1f line_id:%d index:%d"
                , quote.price, pos_atom->String(false).c_str(), p_profit, account_info_.capital.total(), trend_line_->id_, cur_k_index), &quote);

        }else if( pos_atom->float_stop_profit && pos_atom->float_stop_profit->UpdateAndJudgeStop(quote.price, &float_stop_ret) )
        {
            Strategy_Log(utility::FormatStr("short pos UpdateAndJudgeStop %s ", float_stop_ret.c_str()));
            ClosePositionAtom(quote, trade_id, pos_atom, short_pos_qty, long_pos_qty);
        } 
    }//for
}


void TrendUpLineStrategy::JudgeStopShortProfit(const T_QuoteData &quote)
{
    T_HisDataItemContainer &k_hisdata = *main_hisdatas_;
    unsigned int long_pos_qty = account_info_.position.LongPosQty();
    unsigned int short_pos_qty = account_info_.position.ShortPosQty();
    // consider stop profit of short position --------------
    auto pos_infos = account_info_.position.ShortPosSizeInfo();
    for( auto iter = pos_infos.begin(); iter != pos_infos.end(); ++iter )
    {
        int trade_id = iter->first;
        auto pos_atom = account_info_.position.FindPositionAtom(trade_id);
        if( !pos_atom || pos_atom->help_info.strategy_id != id_ )
            continue; 
        assert(pos_atom->qty_all() == 1);
        std::string float_stop_ret;
        if( quote.price < pos_atom->stop_profit_price + cst_tolerance_equal )
        {
            double margin_ret = 0.0;
            double p_profit = 0.0;
            auto temp_val = short_pos_qty;
            account_info_.position.ClosePositionAtom(trade_id, quote.price, &margin_ret, &p_profit);
            if( temp_val > long_pos_qty )
            {
                account_info_.capital.frozen -= margin_ret;
                account_info_.capital.avaliable += margin_ret + p_profit;
            }else
                account_info_.capital.avaliable += p_profit;
            assert(short_pos_qty > 0);
            short_pos_qty -= 1;
            app_.strategy_man()->AppendTradeRecord(OrderAction::CLOSE, *pos_atom, quote, main_hisdatas_->size() - 1);
            Strategy_Log(utility::FormatStr("order stop profit short pos: price:%.1f %s profit:%.1f capital:%.1f"
                , quote.price, pos_atom->String(false).c_str(), p_profit, account_info_.capital.total()), &quote);

        }else if( pos_atom->float_stop_profit && pos_atom->float_stop_profit->UpdateAndJudgeStop(quote.price, &float_stop_ret) )
        {
            Strategy_Log(utility::FormatStr("short pos UpdateAndJudgeStop %s ", float_stop_ret.c_str()));
            ClosePositionAtom(quote, trade_id, pos_atom, short_pos_qty, long_pos_qty);
        } 
    }//for
}