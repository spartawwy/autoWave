
#include <cassert>
#include <Tlib/core/tsystem_core_common.h>
#include "bounceup_strategy.h"

#include "exchange_calendar.h"
#include "futures_forecast_app.h"
#include "position_account.h"
#include "strategy_man.h"
#include "trend_distinguish.h"

static const double cst_bounceup_stop_distance = 0.3;

using namespace TSystem;
BounceUpStrategy::BounceUpStrategy(FuturesForecastApp &app, AccountInfo &account_info, std::shared_ptr<TrendDistinguish> &trend_distinguish):Strategy(app, account_info)
    , trend_distinguish_(trend_distinguish)
{

}
 
void BounceUpStrategy::Handle(const T_QuoteData &quote)
{
    assert(main_forcast_);
    assert(main_bounce_ups_);
    //assert(sub_forcast_);
    if( pre_price_ < 0.0 )
        goto END_PROC;
    
    if( ProdIfNearTradingEndTime(quote) )
        return;

    if( Equal(pre_price_, quote.price) )
        goto END_PROC;
    // add on 20201226-----------
    if( IsNightTradingBegTime(quote) )
        goto END_PROC;
    if( trend_distinguish_->sub_bias() < -7 && trend_distinguish_->sub_lines_bias() < -8 )
        goto END_PROC;
    //-----------------------------

    T_HisDataItemContainer &k_hisdata = *main_hisdatas_;

    if( quote.price > pre_price_ ) // price rise
    { 
        // -----------consider stop loss of short position
         unsigned int short_pos_qty = account_info_.position.ShortPosQty();
        /* if( short_pos_qty > 3 )
        short_pos_qty = short_pos_qty;*/
        unsigned int long_pos_qty = account_info_.position.LongPosQty();
        JudgeStopShortLoss(quote, short_pos_qty, long_pos_qty);
 
        JudgeOpenShort(quote, short_pos_qty, long_pos_qty);

    } 
    else // price fall
    { 
        unsigned int short_pos_qty = account_info_.position.ShortPosQty();
        unsigned int long_pos_qty = account_info_.position.LongPosQty();
 
        JudgeStopShortProfit(quote);
 
    }

END_PROC:

    pre_price_ = quote.price;
}

void BounceUpStrategy::JudgeOpenShort(const T_QuoteData &quote, INOUT unsigned int &short_pos_qty, unsigned int long_pos_qty)
{
    T_HisDataItemContainer &k_hisdata = *main_hisdatas_;
     
    //auto main_bounce_ups_copy = *main_bounce_ups_;
#if 0
    main_bounce_ups_copy.sort([](Data2pForcastInnerContainer::const_reference lh, Data2pForcastInnerContainer::const_reference rh)->bool
    {
        return lh.c3 < rh.c3; // order by asc   const T_Data2pForcast& lh
    });
#endif
    //--------------------choice forecasts which c3 is nearby quote  price--------------------
    std::vector<RelForcastInfo> rel_infos;
    std::unordered_map<unsigned int, T_Data2pForcast *>  rel_forcasts;
    for( auto iter = main_bounce_ups_->begin(); iter != main_bounce_ups_->end(); ++iter )
    {
        if( iter->index_a == iter->index_b )
            continue;
        if( IsPriceRoseNear(quote.price, iter->c3) )
        {
            if( iter->opened_short_c3 )
                continue;
            if( iter->breaked_b || iter->breaked_a )
                continue;
#if 0
            // filter which upper iter's c3 is near by
            auto iter_temp = iter;
            ++iter_temp; // point to upper c3 which is bigger than this c3
            if( iter_temp != main_bounce_ups_copy.end() && iter_temp->c3 - iter->c3 < cst_bounceup_stop_distance - cst_tolerance_min - cst_tolerance_equal )
            {
                iter->opened_short_c3 = true;
                continue;
            }
#endif
            //if( !account_info_.position.FindPositionAtomByRelForcast(iter->id, ForcastSiteType::C3) )
            {
                double ab_distance = main_hisdatas_->at(iter->index_a)->stk_item.high_price - main_hisdatas_->at(iter->index_b)->stk_item.low_price;
                rel_infos.emplace_back(ForcastType::BOUNCE_UP, iter->id, ForcastSiteType::C3, ab_distance);
                //T_Data2pForcast *p_fcast = std::addressof(*iter);
                rel_forcasts.insert(std::make_pair(iter->id, std::addressof(*iter)));
                iter->opened_short_c3 = true;
            }
        }
    }
    if( !rel_infos.empty() )
    {
        unsigned int net_qty = MAX_VAL(short_pos_qty, long_pos_qty);
        auto frozen_capital = net_qty > 0 ? CaculateOpenPositionFreezeCapital(quote.price, net_qty) : 0.0;
        auto pos_capital_allow_n = (unsigned int)CalculateMaxQtyAllowOpen(account_info_.capital.avaliable + frozen_capital, quote.price);
        assert(pos_capital_allow_n >= short_pos_qty);
        auto remain_short_allow = pos_capital_allow_n - short_pos_qty;
        assert(cst_position_max >= short_pos_qty);
        unsigned int real_short_pos_allow = MIN_VAL(remain_short_allow, cst_position_max - short_pos_qty);

        if( real_short_pos_allow < 1 )
            return;

        std::sort(rel_infos.begin(), rel_infos.end(), compare_sort_desc);
        // open short ------------------------------------
        bool is_long = false; 
        auto rel_info = rel_infos[0]; // choice the forecast which ab_distance is biggest
        auto spread_type = JudgePriceSpreadType(main_k_type_, rel_info.ab_distance);
        int targ_pos_count = 0;
        if( spread_type <= PriceSpreadType::MICRO )
            targ_pos_count = 1;
        //else if( spread_type <= PriceSpreadType::SMALL )
        //    targ_pos_count = (real_short_pos_allow > 1 ? 2 : 1);
        //else // > SMALL 
            targ_pos_count = (real_short_pos_allow > 2 ? 3 : MAX_VAL(real_short_pos_allow, 1));

        for( int j = 1; j <= targ_pos_count; ++j )
        {
            auto p_forcast = rel_forcasts.find(rel_info.forcast_id);

            auto positon_atom = std::make_shared<PositionAtom>();
            positon_atom->qty_available = 1;
            positon_atom->rel_forcast_info = rel_info;
            positon_atom->price = quote.price;
            positon_atom->is_long = is_long;
            positon_atom->help_info.strategy_id = id_;
            if( spread_type <= PriceSpreadType::MICRO )
            {
                positon_atom->stop_loss_price = quote.price + cst_bounceup_stop_distance;

            }else /*if( spread_type <= PriceSpreadType::SMALL )*/
            {
                positon_atom->stop_loss_price = quote.price + cst_bounceup_stop_distance;
            }/*else
                positon_atom->stop_loss_price = quote.price + 0.8;*/
            double a_high = k_hisdata[p_forcast->second->index_a]->stk_item.high_price;
            double b_low = k_hisdata[p_forcast->second->index_b]->stk_item.low_price;
            if( spread_type <= PriceSpreadType::SMALL )
            {
                if( j == 1 )
                {
                    if( account_info_.position.PositionCountRemainedForRunProfit(false) < 1 )
                    {
                        positon_atom->help_info.is_remain = true;
                        auto targes = ForcastD_ABC_Down(a_high, b_low); 
                        positon_atom->stop_profit_price = std::get<2>(targes) + 0.2; // d3
                    }else
                        positon_atom->stop_profit_price = quote.price - cst_default_stop_price_distance;
                }else if( j == 2 )
                    positon_atom->stop_profit_price = quote.price - cst_default_stop_price_distance;
                else
                    positon_atom->stop_profit_price = k_hisdata[p_forcast->second->index_b]->stk_item.low_price + 0.2;

                positon_atom->help_info.small_aim = quote.price - cst_default_stop_price_distance;
                positon_atom->help_info.mid_aim = quote.price - 2*cst_default_stop_price_distance;
                positon_atom->help_info.big_aim = quote.price - 3*cst_default_stop_price_distance;
            }else if( spread_type <= PriceSpreadType::MID )
            {
                auto targes = ForcastD_ABC_Down(k_hisdata[p_forcast->second->index_a]->stk_item.high_price, k_hisdata[p_forcast->second->index_b]->stk_item.low_price); 
                double big_p = std::get<1>(targes) + 0.2; // d2
                double mid_p = p_forcast->second->c2;
                if( j == 1 )
                {
                    if(  account_info_.position.PositionCountRemainedForRunProfit(false) < 1 )
                    {
                        positon_atom->help_info.is_remain = true;
                        positon_atom->stop_profit_price = big_p; 
                    }else
                        positon_atom->stop_profit_price = quote.price - cst_default_stop_price_distance;
                }else if( j == 2 )
                { 
                    positon_atom->stop_profit_price = quote.price - cst_default_stop_price_distance;
                }else
                    positon_atom->stop_profit_price = mid_p;

                positon_atom->help_info.small_aim = quote.price - cst_default_stop_price_distance;
                positon_atom->help_info.mid_aim = mid_p;
                positon_atom->help_info.big_aim = big_p;
            }else // big
            {
                double big_p = (p_forcast->second->c1 + p_forcast->second->c2) / 2;
                double mid_p = p_forcast->second->c2;
                if( account_info_.position.PositionCountRemainedForRunProfit(false) < 1 )
                {
                    positon_atom->help_info.is_remain = true;
                    positon_atom->stop_profit_price = (p_forcast->second->c1 + p_forcast->second->c2) / 2;
                }else
                {
                    if( j % 2 == 0 )
                        positon_atom->stop_profit_price = quote.price - cst_default_stop_price_distance;
                    else
                        positon_atom->stop_profit_price = p_forcast->second->c2;
                }
                positon_atom->help_info.small_aim = quote.price - cst_default_stop_price_distance;
                positon_atom->help_info.mid_aim = mid_p;
                positon_atom->help_info.big_aim = big_p;
            }
#if 0
            positon_atom->help_info.small_aim = MAX_VAL(quote.price - cst_default_stop_price_distance, p_forcast->second->c2);
            positon_atom->help_info.mid_aim = MIN_VAL(quote.price - cst_default_stop_price_distance, p_forcast->second->c2);
            positon_atom->help_info.big_aim = p_forcast->second->c1;
#else
            
#endif
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

            app_.strategy_man()->AppendTradeRecord(OrderAction::OPEN, *positon_atom, quote, k_hisdata.size() - 1); 
            Strategy_Log(utility::FormatStr("order open short pos: %s fee:%.1f frozen:%.1f ava:%.1f"
                , positon_atom->String().c_str(), fee, account_info_.capital.frozen, account_info_.capital.avaliable), &quote);
        }
                
    }
}

void BounceUpStrategy::JudgeStopShortLoss(const T_QuoteData &quote, INOUT unsigned int &short_pos_qty, unsigned int long_pos_qty)
{
    //unsigned int short_pos_qty = account_info_.position.ShortPosQty();
    //unsigned int long_pos_qty = account_info_.position.LongPosQty();
    auto pos_infos = account_info_.position.ShortPosSizeInfo();
    for( auto iter = pos_infos.begin(); iter != pos_infos.end(); ++iter )
    {
        int trade_id = iter->first;
        auto pos_atom = account_info_.position.FindPositionAtom(trade_id);
        if( !pos_atom || pos_atom->help_info.strategy_id != id_ )
            continue; 
        {
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
                Strategy_Log(utility::FormatStr("order stop loss or float profit short pos: price:%.1f %s profit:%.1f capital:%.1f"
                    , quote.price, pos_atom->String().c_str(), p_profit, account_info_.capital.total()), &quote);
            } 
        }
    }
}


void BounceUpStrategy::JudgeStopShortProfit(const T_QuoteData &quote)
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
                , quote.price, pos_atom->String().c_str(), p_profit, account_info_.capital.total()), &quote);
        }else
        {  
#if 0
            //consider ajust stop_loss_price
            if( IsPriceNear(quote.price, pos_atom->help_info.big_aim, *pos_atom, main_k_type_) )
            {
                pos_atom->help_info.has_near_big_aim = pos_atom->help_info.has_near_mid_aim = pos_atom->help_info.has_near_small_aim = true;
                if( fabs(pos_atom->help_info.big_aim - pos_atom->help_info.mid_aim) > 0.6 )
                    pos_atom->stop_loss_price = pos_atom->help_info.mid_aim;
                else
                    pos_atom->stop_loss_price = pos_atom->help_info.small_aim;

            }
#endif
#if 0
            else if( IsPriceNear(quote.price, pos_atom->help_info.mid_aim, *pos_atom, main_k_type_) )
            {
                pos_atom->help_info.has_near_mid_aim = pos_atom->help_info.has_near_small_aim = true;
                if( pos_atom->help_info.is_remain )
                    pos_atom->stop_loss_price = pos_atom->price;
                else
                {
                    if( fabs(pos_atom->help_info.mid_aim - pos_atom->help_info.small_aim) > 0.6 )
                        pos_atom->stop_loss_price = pos_atom->help_info.small_aim;
                    else
                        pos_atom->stop_loss_price = pos_atom->price;
                }

            }else if( IsPriceNear(quote.price, pos_atom->help_info.small_aim, *pos_atom, main_k_type_) )
            {    
                pos_atom->help_info.has_near_small_aim = true;
                //if( !pos_atom->help_info.is_remain )
                    pos_atom->stop_loss_price = pos_atom->price;
                 
            }
#endif
        }
    }
}
