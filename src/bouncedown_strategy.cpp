
#include <cassert>
#include <Tlib/core/tsystem_core_common.h>
#include "bouncedown_strategy.h"

#include "exchange_calendar.h"
#include "futures_forecast_app.h"
#include "position_account.h"
#include "strategy_man.h"

static const double cst_bouncedown_stop_distance = 0.3;

using namespace TSystem;
BounceDownStrategy::BounceDownStrategy(FuturesForecastApp &app, AccountInfo &account_info, std::shared_ptr<TrendDistinguish> &trend_distinguish):Strategy(app, account_info)
    , trend_distinguish_(trend_distinguish)
{

}


void BounceDownStrategy::Handle(const T_QuoteData &quote)
{
    assert(main_forcast_);
    assert(main_bounce_ups_);
    //assert(sub_forcast_);
    if( pre_price_ < 0.0 )
    {
        pre_price_ = quote.price;
        return;
    }
    
    if( ProdIfNearTradingEndTime(quote) )
        return;

    if( Equal(pre_price_, quote.price) )
        return;

    T_HisDataItemContainer &k_hisdata = *main_hisdatas_;

    if( quote.price > pre_price_ ) // price rise
    { 
        JudgeStopLongProfit(quote);  
    } 
    else // price fall
    {  
        // -----------consider stop loss of short position
        unsigned int short_pos_qty = account_info_.position.ShortPosQty();
        unsigned int long_pos_qty = account_info_.position.LongPosQty();
        JudgeStopLongLoss(quote, short_pos_qty, long_pos_qty);
 
        JudgeOpenLong(quote, short_pos_qty, long_pos_qty);
    }
    pre_price_ = quote.price;
}

void BounceDownStrategy::JudgeOpenLong(const T_QuoteData &quote, unsigned int short_pos_qty, INOUT unsigned int &long_pos_qty)
{
    T_HisDataItemContainer &k_hisdata = *main_hisdatas_;
 
    //--------------------choice forecasts which c3 is nearby quote  price--------------------
    std::vector<RelForcastInfo> rel_infos;
    std::unordered_map<unsigned int, T_Data2pForcast *>  rel_forcasts;
    for( auto iter = main_bounce_downs_->begin(); iter != main_bounce_downs_->end(); ++iter )
    {
        if( iter->index_a == iter->index_b )
            continue;
        if( IsPriceFallNear(quote.price, iter->c3) )
        {
            if( iter->opened_long_c3 )
                continue;
            if( iter->breaked_b || iter->breaked_a )
                continue;
            double ab_distance = main_hisdatas_->at(iter->index_b)->stk_item.high_price - main_hisdatas_->at(iter->index_a)->stk_item.low_price;
            rel_infos.emplace_back(ForcastType::BOUNCE_DOWN, iter->id, ForcastSiteType::C3, ab_distance);
            rel_forcasts.insert(std::make_pair(iter->id, std::addressof(*iter)));
            iter->opened_long_c3 = true;
        }
    }
    if( !rel_infos.empty() )
    {
        unsigned int net_qty = MAX_VAL(short_pos_qty, long_pos_qty);
        auto frozen_capital = net_qty > 0 ? CaculateOpenPositionFreezeCapital(quote.price, net_qty) : 0.0;
        auto pos_capital_allow_n = (unsigned int)CalculateMaxQtyAllowOpen(account_info_.capital.avaliable + frozen_capital, quote.price);
        assert(pos_capital_allow_n >= long_pos_qty);
        auto remain_long_allow = pos_capital_allow_n - long_pos_qty;
        assert(cst_position_max >= long_pos_qty);
        unsigned int real_long_pos_allow = MIN_VAL(remain_long_allow, cst_position_max - long_pos_qty);

        if( real_long_pos_allow < 1 )
            return;

        std::sort(rel_infos.begin(), rel_infos.end(), compare_sort_desc);
        // open long ------------------------------------
        bool is_long = true; 
        auto rel_info = rel_infos[0]; // choice the forecast which ab_distance is biggest
        auto spread_type = JudgePriceSpreadType(main_k_type_, rel_info.ab_distance);
        int targ_pos_count = 0;
        if( spread_type <= PriceSpreadType::MICRO )
            targ_pos_count = 1;
        //else if( spread_type <= PriceSpreadType::SMALL )
        //    targ_pos_count = (real_short_pos_allow > 1 ? 2 : 1);
        //else // > SMALL 
            targ_pos_count = (real_long_pos_allow > 2 ? 3 : MAX_VAL(real_long_pos_allow, 1));

        for( int j = 1; j <= targ_pos_count; ++j )
        {
            auto p_forcast = rel_forcasts.find(rel_info.forcast_id);

            auto positon_atom = std::make_shared<PositionAtom>();
            positon_atom->qty_available = 1;
            positon_atom->rel_forcast_info = rel_info;
            positon_atom->price = quote.price;
            positon_atom->is_long = is_long;
            if( spread_type <= PriceSpreadType::MICRO )
            {
                positon_atom->stop_loss_price = quote.price - cst_bouncedown_stop_distance;

            }else /*if( spread_type <= PriceSpreadType::SMALL )*/
            {
                positon_atom->stop_loss_price = quote.price - cst_bouncedown_stop_distance;
            }/*else
                positon_atom->stop_loss_price = quote.price + 0.8;*/
            double a_low = k_hisdata[p_forcast->second->index_a]->stk_item.low_price;
            double b_high = k_hisdata[p_forcast->second->index_b]->stk_item.high_price;
            if( spread_type <= PriceSpreadType::SMALL )
            {
                if( j == 1 )
                {
                    if( account_info_.position.PositionCountRemainedForRunProfit(is_long) < 1 )
                    {
                        positon_atom->help_info.is_remain = true;
                        auto targes = ForcastD_ABC_Up(a_low, b_high); 
                        positon_atom->stop_profit_price = std::get<2>(targes) - 0.2; // d3
                    }else
                        positon_atom->stop_profit_price = quote.price + cst_default_stop_price_distance;
                }else if( j == 2 )
                    positon_atom->stop_profit_price = quote.price + cst_default_stop_price_distance;
                else
                    positon_atom->stop_profit_price = k_hisdata[p_forcast->second->index_b]->stk_item.high_price - 0.2;

                positon_atom->help_info.small_aim = quote.price + cst_default_stop_price_distance;
                positon_atom->help_info.mid_aim = quote.price + 2*cst_default_stop_price_distance;
                positon_atom->help_info.big_aim = quote.price + 3*cst_default_stop_price_distance;
            }else if( spread_type <= PriceSpreadType::MID )
            {
                auto targes = ForcastD_ABC_Up(a_low, b_high); 
                double big_p = std::get<1>(targes) - 0.2; // d2
                double mid_p = p_forcast->second->c2;
                if( j == 1 )
                {
                    if(  account_info_.position.PositionCountRemainedForRunProfit(is_long) < 1 )
                    {
                        positon_atom->help_info.is_remain = true;
                        positon_atom->stop_profit_price = big_p; 
                    }else
                        positon_atom->stop_profit_price = quote.price + cst_default_stop_price_distance;
                }else if( j == 2 )
                { 
                    positon_atom->stop_profit_price = quote.price + cst_default_stop_price_distance;
                }else
                    positon_atom->stop_profit_price = mid_p;

                positon_atom->help_info.small_aim = quote.price + cst_default_stop_price_distance;
                positon_atom->help_info.mid_aim = mid_p;
                positon_atom->help_info.big_aim = big_p;
            }else // big
            {
                double big_p = (p_forcast->second->c1 + p_forcast->second->c2) / 2;
                double mid_p = p_forcast->second->c2;
                if( account_info_.position.PositionCountRemainedForRunProfit(is_long) < 1 )
                {
                    positon_atom->help_info.is_remain = true;
                    positon_atom->stop_profit_price = (p_forcast->second->c1 + p_forcast->second->c2) / 2;
                }else
                {
                    if( j % 2 == 0 )
                        positon_atom->stop_profit_price = quote.price + cst_default_stop_price_distance;
                    else
                        positon_atom->stop_profit_price = p_forcast->second->c2;
                }
                positon_atom->help_info.small_aim = quote.price + cst_default_stop_price_distance;
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
            if( long_pos_qty >= short_pos_qty ) // ps: 考虑多空 共享保证金
            {
                account_info_.capital.avaliable -= margin + fee;
                account_info_.capital.frozen += margin;
            }else
                account_info_.capital.avaliable -= fee;
            long_pos_qty += 1;

            app_.strategy_man()->AppendTradeRecord(OrderAction::OPEN, *positon_atom, quote, k_hisdata.size() - 1); 
            Strategy_Log(utility::FormatStr("order open LONG pos: %s fee:%.1f frozen:%.1f ava:%.1f"
                , positon_atom->String().c_str(), fee, account_info_.capital.frozen, account_info_.capital.avaliable), &quote);
        } 
    }
}

void BounceDownStrategy::JudgeStopLongLoss(const T_QuoteData &quote, unsigned int short_pos_qty, INOUT unsigned int &long_pos_qty)
{
    auto pos_infos = account_info_.position.LongPosSizeInfo();
    for( auto iter = pos_infos.begin(); iter != pos_infos.end(); ++iter )
    {
        int trade_id = iter->first;
        auto pos_atom = account_info_.position.FindPositionAtom(trade_id);
        if( pos_atom )
        {
            if( quote.price < pos_atom->stop_loss_price + cst_tolerance_equal )
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
                Strategy_Log(utility::FormatStr("order stop loss or float profit LONG pos: price:%.1f %s profit:%.1f capital:%.1f"
                    , quote.price, pos_atom->String().c_str(), p_profit, account_info_.capital.total()), &quote);
            } 
        }
    }
}

void BounceDownStrategy::JudgeStopLongProfit(const T_QuoteData &quote)
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
        if( !pos_atom )
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
                , quote.price, pos_atom->String().c_str(), p_profit, account_info_.capital.total()), &quote);
        }
    }
}
