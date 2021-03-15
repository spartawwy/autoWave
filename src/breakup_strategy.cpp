
#include <cassert>
#include <Tlib/core/tsystem_core_common.h>
#include "breakup_strategy.h"

#include "exchange_calendar.h"
#include "futures_forecast_app.h"
#include "position_account.h"
#include "strategy_man.h"
#include "trend_distinguish.h"
#include "float_stop_profit.h"

using namespace TSystem;
BreakUpStrategy::BreakUpStrategy(FuturesForecastApp &app, AccountInfo &account_info, std::shared_ptr<TrendDistinguish> &trend_distinguish):Strategy(app, account_info)
    , trend_distinguish_(trend_distinguish)
{

}
 
void BreakUpStrategy::Handle(const T_QuoteData &quote)
{
    assert(main_forcast_);
    assert(main_bounce_ups_);
    //assert(sub_forcast_);
    if( pre_price_ < 0.0 )
    {
        pre_price_ = quote.price;
        return;
    }

    ProdIfNearTradingEndTime(quote);
      
    if( Equal(pre_price_, quote.price) )
        return;

    T_HisDataItemContainer &k_hisdata = *main_hisdatas_;
    const int k_rend_index = k_hisdata.size() - 1;

    const bool is_time_allow_open_pos = !IsNightTradingBegTime(quote) && !IsNearDayTradeTimeEnd(quote) && !IsNearNightTradeTimeEnd(quote);
    //debug --------------
    int hhmm = quote.hhmmss/100;
    if( hhmm == 10 )
        hhmm = hhmm;
    if( k_rend_index == 757 ) //540
        int val = k_rend_index;
    //-------------------- --------------------
#if 0
    main_bounce_ups_copy.sort([](Data2pForcastInnerContainer::const_reference lh, Data2pForcastInnerContainer::const_reference rh)->bool
    {
        return lh.c3 < rh.c3; // order by asc   const T_Data2pForcast& lh
    });
#endif
    // clear breaked forecasts
    for( auto iter = observed_forcasts_.begin(); iter != observed_forcasts_.end();  )
    {
        ////debug ----------
        //if( iter->second->index_a == 35 )
        //    iter->second->index_a = iter->second->index_a;
        //-----------------
        if( quote.price < k_hisdata[iter->second->index_b]->stk_item.low_price ) // break b
        {
            forcasts_prepare_buy_.erase(iter->first);
            observed_forcasts_.erase(iter++);
        }else if( quote.price > k_hisdata[iter->second->index_a]->stk_item.high_price ) // break a
        {
            if( quote.price > k_hisdata[iter->second->index_a]->stk_item.high_price + 1.0 )
            {
                forcasts_prepare_buy_.erase(iter->first);
                observed_forcasts_.erase(iter++);
            }else
                ++iter;
        }else 
            ++iter;
    }
    
    if( quote.price > pre_price_ ) // price rise
    { 
        auto h_l_price = account_info_.position.GetHighestLowestPrice(true);
        const double highest_price_in_position = std::get<0>(h_l_price);
        //const double loweest_price_in_position = std::get<1>(h_l_price);
        JudgeStopLongProfit(quote);
        bool is_chase_high = ( highest_price_in_position > 0.1 && quote.price > highest_price_in_position );

        if( !is_chase_high && is_time_allow_open_pos && trend_distinguish_->is_breakup_strategy_allow() )
        { 
        for( auto iter = main_bounce_ups_->begin(); iter != main_bounce_ups_->end(); ++iter )
        {   
            if( iter->breaked_b || iter->breaked_a )
                continue;
            if( iter->opened_long_c3 > 1 )
                continue;
            if( account_info_.position.FindPositionAtomByRelForcast(iter->id, ForcastSiteType::C3, true) )
                continue;

            if( quote.price > iter->c3 )
            {  
                if( observed_forcasts_.find(iter->id) != observed_forcasts_.end() ) // already insert
                    continue;
                auto forcast = std::make_shared<T_Data2pForcast>(*iter);
                observed_forcasts_.insert(std::make_pair(iter->id, std::move(forcast)));
            }
        } // for
        }
    }
     
    // consider k line --------
    if( is_time_allow_open_pos && k_hisdata.size() >= 3 )
    { 
    for( auto iter = observed_forcasts_.begin(); iter != observed_forcasts_.end(); ++iter )
    {  
        if( k_hisdata.size() - 1 - 1 <= iter->second->index_a )
            continue;
        bool allow_buy = false;
        //const double c3 = iter->second->c3;
        if( iter->second->opened_long_c3 == 0 ) // first time
        { 
            if( k_hisdata[k_hisdata.size() - 1 - 1]->stk_item.close_price > iter->second->c3 + cst_tolerance_equal
            //&& k_hisdata[k_hisdata.size() - 1 - 2]->stk_item.close_price > iter->second->c3
            )
                allow_buy = true;
        }else if( iter->second->opened_long_c3 == 1 ) //second time
        {
            if( k_hisdata[k_hisdata.size() - 1 - 1]->stk_item.close_price < iter->second->c3 + cst_tolerance_equal )
                allow_buy = true;
        }
        
        if( allow_buy && forcasts_prepare_buy_.find(iter->first) == forcasts_prepare_buy_.end() )
            forcasts_prepare_buy_.insert(std::make_pair(iter->first, iter->second));
    }// for
    }
     
    if( quote.price < pre_price_ ) // price fall
    { 
        unsigned int short_pos_qty = account_info_.position.ShortPosQty();
        unsigned int long_pos_qty = account_info_.position.LongPosQty();
        JudgeStopLongLoss(quote, short_pos_qty, long_pos_qty);
        if( is_time_allow_open_pos )
            JudgeOpenLong(quote, short_pos_qty, long_pos_qty);
    }

    pre_price_ = quote.price;
}


void BreakUpStrategy::JudgeOpenLong(const T_QuoteData &quote, unsigned int short_pos_qty, INOUT unsigned int &long_pos_qty)
{
    T_HisDataItemContainer &k_hisdata = *main_hisdatas_;

    unsigned int net_qty = MAX_VAL(short_pos_qty, long_pos_qty);
    auto frozen_capital = net_qty > 0 ? CaculateOpenPositionFreezeCapital(quote.price, net_qty) : 0.0;
    auto pos_capital_allow_n = (unsigned int)CalculateMaxQtyAllowOpen(account_info_.capital.avaliable + frozen_capital, quote.price);
    assert(pos_capital_allow_n >= long_pos_qty);
    auto remain_long_allow = pos_capital_allow_n - long_pos_qty;
    assert(cst_position_max >= long_pos_qty);
    unsigned int real_long_pos_allow = MIN_VAL(remain_long_allow, cst_position_max - long_pos_qty);
    if( real_long_pos_allow < 1 )
        return;

    std::vector<RelForcastInfo> rel_infos;
    std::unordered_map<unsigned int, std::shared_ptr<T_Data2pForcast> >  rel_forcasts;
    for( auto iter = forcasts_prepare_buy_.begin(); iter != forcasts_prepare_buy_.end(); ++iter )
    {
        /*if( !iter->second )
        continue;*/
        if( JudgePriceFallNear(quote.price, ProcDecimal_1(iter->second->c3) ) == PriceNearType::FIT )
        {
            double ab_distance = k_hisdata.at(iter->second->index_a)->stk_item.high_price - k_hisdata.at(iter->second->index_b)->stk_item.low_price;
            rel_infos.emplace_back(ForcastType::BOUNCE_UP, iter->first, ForcastSiteType::C3, ab_distance);
            rel_forcasts.insert(std::make_pair(iter->first, iter->second));
            iter->second->opened_long_c3++;
            T_Data2pForcast *p_frct = main_forcast_->Find2pForcast(code_, main_k_type_, true, iter->first);
            if( p_frct ) 
                p_frct->opened_long_c3++;
        }
    }
    if( rel_infos.empty() )
        return;

    std::sort(rel_infos.begin(), rel_infos.end(), compare_sort_desc);

    double max_back_rate = 0.5;
    unsigned int targ_pos_count = real_long_pos_allow;
    for( int j = 1; j <= targ_pos_count; ++j )
    { 
        auto positon_atom = std::make_shared<PositionAtom>();
        positon_atom->qty_available = 1;
        positon_atom->rel_forcast_info = rel_infos.back(); 
        positon_atom->price = quote.price;
        positon_atom->is_long = true; 
        forcasts_prepare_buy_.erase(positon_atom->rel_forcast_info.forcast_id);

        positon_atom->stop_loss_price = ProcDecimal_1(rel_forcasts[positon_atom->rel_forcast_info.forcast_id]->c3) - 0.3; // 0.2

        const int index_a = rel_forcasts[rel_infos.back().forcast_id]->index_a;
        const int index_b = rel_forcasts[rel_infos.back().forcast_id]->index_b;
        if( j == 1 )
        {
            if( account_info_.position.PositionCountRemainedForRunProfit(true) < 1 )
                positon_atom->help_info.is_remain = true; 
            positon_atom->stop_profit_price = 9999.9; //
            positon_atom->float_stop_profit = std::make_shared<FollowStopProfit>(*positon_atom, 2.0, max_back_rate, positon_atom->price + 10.0);
        }else if( j == 2 )
        {
            double a_price = k_hisdata[index_a]->stk_item.high_price;
            double stop_profit_price = (a_price - quote.price > 0.6 ? a_price - 0.1 : quote.price + 0.6);
#if 0
            positon_atom->stop_profit_price = stop_profit_price;
#else
            positon_atom->stop_profit_price = 9999.9; //
            positon_atom->float_stop_profit = std::make_shared<FollowStopProfit>(*positon_atom, 2.0, max_back_rate, stop_profit_price);

#endif
        }else 
        { 
            //int lowest_index = Lowest(k_hisdata, index_a, k_hisdata.size() - 1 - index_a);
            auto targets = ForcastD_ABC_Up(k_hisdata[index_b]->stk_item.low_price, k_hisdata[index_a]->stk_item.high_price); 
            positon_atom->stop_profit_price = 9999.9; //
            double price_distance = std::get<1>(targets) - quote.price - 0.1;
            positon_atom->float_stop_profit = std::make_shared<FollowStopProfit>(*positon_atom, 1.2, max_back_rate, positon_atom->price + price_distance);
        }

        positon_atom->trade_id = account_info_.position.GenerateTradeId();
        account_info_.position.PushBack(positon_atom->is_long, positon_atom);
        double margin = CaculateOpenPositionMargin(positon_atom->price, positon_atom->qty_available);
        double fee = CalculateFee(positon_atom->qty_available, positon_atom->price, false);
        if( long_pos_qty >= short_pos_qty ) // ps: 考虑多空 共享保证金
        {
            account_info_.capital.avaliable -= margin + fee;
            account_info_.capital.frozen += margin;
        }else
            account_info_.capital.avaliable -= fee;
        long_pos_qty += 1;
        app_.strategy_man()->AppendTradeRecord(OrderAction::OPEN, *positon_atom, quote, main_hisdatas_->size() - 1);
        Strategy_Log(utility::FormatStr("open breakup LONG pos: %s fee:%.1f frozen:%.1f ava:%.1f"
            , positon_atom->String().c_str(), fee, account_info_.capital.frozen, account_info_.capital.avaliable), &quote);
    } 

}

void BreakUpStrategy::JudgeStopLongLoss(const T_QuoteData &quote, INOUT unsigned int &short_pos_qty, INOUT unsigned int &long_pos_qty)
{ 
    auto pos_infos = account_info_.position.LongPosSizeInfo();
    for( auto iter = pos_infos.begin(); iter != pos_infos.end(); ++iter )
    {
        int trade_id = iter->first;
        auto pos_atom = account_info_.position.FindPositionAtom(trade_id);
        if( !pos_atom || pos_atom->help_info.strategy_id != id_ )
            continue; 
        {
            if( quote.price < pos_atom->stop_loss_price + cst_tolerance_equal )
            {  
                ClosePositionAtom(quote, trade_id, pos_atom, short_pos_qty, long_pos_qty);
                continue;
            } 
            if( pos_atom->float_stop_profit && pos_atom->float_stop_profit->UpdateAndJudgeStop(quote.price) )
            {
                ClosePositionAtom(quote, trade_id, pos_atom, short_pos_qty, long_pos_qty);
            }
        }
    }// for 
}
 

void BreakUpStrategy::JudgeStopLongProfit(const T_QuoteData &quote)
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
            Strategy_Log(utility::FormatStr("STOP break up profit LONG: price:%.1f %s profit:%.1f capital:%.1f"
                , quote.price, pos_atom->String().c_str(), p_profit, account_info_.capital.total()), &quote);
        }
    }
}

PriceNearType JudgePriceFallNear(double price, double tag_price)
{
    double ceil_tolerance_price = 0.1;
    double floor_tolerance_price = 0.1;
     
    if( price > tag_price + ceil_tolerance_price - cst_tolerance_equal ) // upper
        return PriceNearType::UNTOUCH;
    else if( price < tag_price + ceil_tolerance_price + cst_tolerance_equal
        && price > tag_price - floor_tolerance_price - cst_tolerance_equal )
        return PriceNearType::FIT;  // fit 
    if( price < tag_price - 0.2 + cst_tolerance_equal )
        return PriceNearType::BREAK;
    else 
        return PriceNearType::OTHER;
}

double ProcDecimal_1(double val, unsigned int decimal)
{
    int temp = pow(10, decimal);
    int64_t big_val = int64_t(fabs(val * temp) + 0.7) * (val < 0 ? - 1 : 1); //2舍3入
    return double(big_val) / temp;
}