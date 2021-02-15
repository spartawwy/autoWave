#include <cassert>

#include "trenddown_strategy.h"
#include "trend_distinguish.h"

#include <Tlib/core/tsystem_core_common.h>

#include "exchange_calendar.h"
#include "futures_forecast_app.h"
#include "position_account.h"
#include "strategy_man.h"
#include "float_stop_profit.h"

#define TREND_DOWN_ENABLE_SHORT  0

#define IS_USE_AIM 0

double SiteProcDecimal(double val, unsigned int decimal=1);

static const int cst_box_knum_threshold = 10;

using namespace TSystem;
TrendDownStrategy::TrendDownStrategy(FuturesForecastApp &app, AccountInfo &account_info, std::shared_ptr<TrendDistinguish> &trend_distinguish) : Strategy(app, account_info)
    , trend_distinguish_(trend_distinguish)
{

}

void TrendDownStrategy::Handle(const T_QuoteData &quote)
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
        Data3pForcastInnerContainer & main_trends_downs_copy = *main_trends_downs_;
        std::vector<RelForcastInfo> rel_infos;
        std::unordered_map<unsigned int, T_Data3pForcast *>  rel_forcasts;
        for( auto iter = main_trends_downs_copy.begin(); iter != main_trends_downs_copy.end(); ++iter )
        {
            if( (*iter)->opened_short_d3 )
                continue;
            if( (*iter)->break_d3_n > 0 && !(*iter)->bounce_over_d3 )
            {
                if( quote.price > (*iter)->d3 + 0.3 )
                    (*iter)->bounce_over_d3 = true;
            } 
        }

        // -----------consider stop loss of short position
        unsigned int short_pos_qty = account_info_.position.ShortPosQty(); 
        unsigned int long_pos_qty = account_info_.position.LongPosQty();
 
        //JudgeStopShortLoss(quote, short_pos_qty, long_pos_qty);
 
        JudgeStopLongProfit(quote);
 
        //JudgeOpenShort(quote, short_pos_qty, long_pos_qty);
 

    } // if( quote.price > pre_price_ )
    else // price fall
    { 
        unsigned int short_pos_qty = account_info_.position.ShortPosQty();
        unsigned int long_pos_qty = account_info_.position.LongPosQty();
 
        JudgeStopLongLoss(quote, short_pos_qty, long_pos_qty);
 
        //JudgeStopShortProfit(quote);
 
        JudgeOpenLong(quote, short_pos_qty, long_pos_qty);
 
    }

    pre_price_ = quote.price;
}

void TrendDownStrategy::JudgeOpenLong(const T_QuoteData &quote, unsigned int short_pos_qty, INOUT unsigned int &long_pos_qty)
{
    static auto set_open_flag = [](T_Data3pForcast &frcst, ForcastSiteType site)
    {
        switch(site)
        {    
        case ForcastSiteType::D3: frcst.opened_long_d3 = true;break;
        case ForcastSiteType::D2p5: frcst.opened_long_d2p5 = true;break;
        case ForcastSiteType::D2: frcst.opened_long_d2 = true;break;
        case ForcastSiteType::D1: frcst.opened_long_d1 = true;break;
        } 
    };

    T_HisDataItemContainer &k_hisdata = *main_hisdatas_;
    const auto danger_type_open_long = trend_distinguish_->danger_type_open_long();
    const auto subw_trend_type = trend_distinguish_->sub_cur_trend();
    const bool is_subw_bias_allow = trend_distinguish_->is_sub_bias_allow(true);
    const bool is_mainw_bias_allow = trend_distinguish_->is_main_bias_allow(true);
    
    unsigned int net_qty = MAX_VAL(short_pos_qty, long_pos_qty);
    auto frozen_capital = net_qty > 0 ? CaculateOpenPositionFreezeCapital(quote.price, net_qty) : 0.0;
    auto pos_capital_allow_n = (unsigned int)CalculateMaxQtyAllowOpen(account_info_.capital.avaliable + frozen_capital, quote.price);
    assert(pos_capital_allow_n >= long_pos_qty);
    auto remain_long_allow = pos_capital_allow_n - long_pos_qty;
    assert(cst_position_max >= long_pos_qty);
    unsigned int real_long_pos_allow = 0;
    if( subw_trend_type >= StrategyTrendType::SSTRONG_SHORT )
        real_long_pos_allow = (long_pos_qty == 0 ? MIN_VAL(remain_long_allow, 1) : 0);
    else
        real_long_pos_allow = MIN_VAL(remain_long_allow, cst_position_max - long_pos_qty);
     
    //debug
    const int k_cur_index = k_hisdata.size() - 1;
    if( k_cur_index == 2042 )
    {
        int val = k_cur_index;
        if( quote.price < 414.2 )
            val = val;
    }
    //--------------------1.update state; 2.consider open long position--------------------
    Data3pForcastInnerContainer & main_trends_downs_ref = *main_trends_downs_;
    //bool only_allow_d2_d3_1_qty = false;
    bool only_allow_1_qty = false;

    typedef std::tuple<std::shared_ptr<T_Data3pForcast>, ForcastSiteType, double>  T_Data3pForcastAndSite;
    std::vector<T_Data3pForcastAndSite> prepare_forcasts;
    for( auto iter_in = main_trends_downs_ref.begin(); iter_in != main_trends_downs_ref.end(); ++iter_in )
    {
        std::shared_ptr<T_Data3pForcast> &iter = *iter_in;
        if( iter->opened_long_d3 || iter->breaked_a || iter->break_d3_n > 0 )
            continue; 
        const auto d3_result = JudgePriceTrendDownNear(iter->price_spread_type, quote.price, SiteProcDecimal(iter->d3));
        if( d3_result == PriceNearType::BREAK ) // break down
        {
            iter->opened_long_d1 = iter->opened_long_d2 = iter->opened_long_d2p5 = iter->opened_long_d3 = true; // filter it
            continue;
        }
        if( d3_result == PriceNearType::FIT )
        {
            iter->opened_long_d1 = iter->opened_long_d2 = iter->opened_long_d2p5 = true; 
            prepare_forcasts.push_back(std::make_tuple(iter, ForcastSiteType::D3, iter->d3));
            continue;
        }
        if( iter->price_spread_type >= PriceSpreadType::BIG && !iter->opened_long_d2p5 )
        {
            const auto d2p5_result = JudgePriceTrendDownNear(iter->price_spread_type, quote.price, SiteProcDecimal((iter->d2 + iter->d3)/2));
            if( d2p5_result == PriceNearType::BREAK ) // break down
            {
                iter->opened_long_d1 = iter->opened_long_d2 = iter->opened_long_d2p5 = true; // filter  
                continue;
            }
            if( d2p5_result == PriceNearType::FIT )
            {
                iter->opened_long_d1 = iter->opened_long_d2 = true;  
                prepare_forcasts.push_back(std::make_tuple(iter, ForcastSiteType::D2p5, (iter->d2 + iter->d3)/2));
                continue;
            } 
        }
        //--------------------d2---------------
        if( iter->opened_long_d2 )
            continue;
        const auto d2_result = JudgePriceTrendDownNear(iter->price_spread_type, quote.price, SiteProcDecimal(iter->d2));
        if( d2_result == PriceNearType::BREAK ) // break down
        {
            iter->opened_long_d1 = iter->opened_long_d2 = true; // filter it
            continue;
        }
        if( d2_result == PriceNearType::FIT )
        {
            iter->opened_long_d1 = true;  
            prepare_forcasts.push_back(std::make_tuple(iter, ForcastSiteType::D2, iter->d2));
            continue;
        }
        //--------------------d1---------------
        if( iter->opened_long_d1 )
            continue;
        if( danger_type_open_long != OpenPosDangerType::SAFE )
        {
            iter->opened_long_d1 = true; // filter it
            continue;
        }
        const auto d1_result = JudgePriceTrendDownNear(iter->price_spread_type, quote.price, ProcDecimal(iter->d1));
        if( d1_result == PriceNearType::BREAK ) // break down
        {
            iter->opened_long_d1 = true; // filter it
            continue;
        }
        if( d1_result == PriceNearType::FIT )
        {
            prepare_forcasts.push_back(std::make_tuple(iter, ForcastSiteType::D1, iter->d1));
            continue;
        }
    }
    if( prepare_forcasts.empty() )
        return;
    std::sort(prepare_forcasts.begin(), prepare_forcasts.end(), [](T_Data3pForcastAndSite &lh, T_Data3pForcastAndSite &rh)
    {
        return std::get<2>(lh) < std::get<2>(rh);
    });
     
    for( unsigned int i = 1; i < prepare_forcasts.size(); ++i ) //filter upper forecast
        set_open_flag(*std::get<0>(prepare_forcasts[i]), std::get<1>(prepare_forcasts[i]));

    std::shared_ptr<T_Data3pForcast> &iter = std::get<0>(prepare_forcasts[0]);
    const auto target_frct_site = std::get<1>(prepare_forcasts[0]);
    const double target_price = std::get<2>(prepare_forcasts[0]);
    const double below_step1_price = target_price - 0.5;
    const double below_step2_price = target_price - 1.0;
    for( auto iter_in = main_trends_downs_ref.begin(); iter_in != main_trends_downs_ref.end(); ++iter_in )
    { 
        if( (*iter_in)->opened_long_d3 || (*iter_in)->breaked_a || (*iter_in)->break_d3_n > 0 )
            continue; 
        if( (*iter_in)->d1 < target_price && (*iter_in)->d1 > below_step1_price - EPSINON 
            /*|| (*iter_in)->d2 < target_price && (*iter_in)->d2 > below_step1_price - EPSINON
            || (*iter_in)->d3 < target_price && (*iter_in)->d3 > below_step1_price - EPSINON */
            )
        {
            set_open_flag(*iter, target_frct_site); 
            return;
        } 
        if( (*iter_in)->d1 < target_price && (*iter_in)->d1 > below_step2_price - EPSINON 
            /*|| (*iter_in)->d2 < target_price && (*iter_in)->d2 > below_step2_price - EPSINON
            || (*iter_in)->d3 < target_price && (*iter_in)->d3 > below_step2_price - EPSINON */
            )
        {
            only_allow_1_qty = true;
        } 
    }
    if( danger_type_open_long == OpenPosDangerType::DANGER )
    {
        set_open_flag(*iter, target_frct_site); 
        return;
    }
    //--------------------------------------
    std::vector<RelForcastInfo> rel_infos;
    std::unordered_map<unsigned int, T_Data3pForcast *>  rel_forcasts;
     
    //----------consider k num in box ------------------- 
    const int k_num_in_box = iter->index_break - iter->index_a;
    assert(k_num_in_box > 0);
    if( k_num_in_box >= cst_box_knum_threshold )
    {
        if( iter->price_spread_type < PriceSpreadType::SMALL && target_frct_site < ForcastSiteType::D3 )
        {
            iter->opened_long_d1 = iter->opened_long_d2 = iter->opened_long_d2p5 = true; // filter it
            return;
        }
    }
        //----------consider parent 3p forecast -------------------
#if 0
        bool filter_by_parent = false;
        for( auto parent_iter = iter->parents.begin(); parent_iter != iter->parents.end(); ++parent_iter )
        { 
            assert(iter->d3 > parent_iter->second->d3 - EPSINON);
            if( parent_iter->second->price_spread_type < PriceSpreadType::BIG 
                && (iter->d3 > parent_iter->second->d2 || iter->d3 - parent_iter->second->d3 < 1.1) )
            {
                filter_by_parent = true;
                break;
            }
        }
        if( filter_by_parent )
            return;
#endif
    //---------------------------------------------------------
         
    double ab_distance = k_hisdata.at(iter->index_a)->stk_item.high_price - k_hisdata.at(iter->index_b)->stk_item.low_price;
         
    if( iter->price_spread_type <= PriceSpreadType::MICRO ) // only USE D2 D3
    { 
        if( target_frct_site < ForcastSiteType::D2 )
        {
            set_open_flag(*iter, target_frct_site); 
            return;
        } 
        rel_infos.emplace_back(ForcastType::TREND_DOWN, iter->id, target_frct_site, ab_distance);
        rel_forcasts.insert(std::make_pair(iter->id, std::addressof(*iter)));

    }else if( iter->price_spread_type < PriceSpreadType::BIG ) // small and mid
    { 
        //if( subw_trend_type < StrategyTrendType::LONG_TO_DOWN || is_subw_bias_allow )
        {
            rel_infos.emplace_back(ForcastType::TREND_DOWN, iter->id, target_frct_site, ab_distance);
            rel_forcasts.insert(std::make_pair(iter->id, std::addressof(*iter)));
        } 

    }else  // >= BIG  ONLY OPEN D2P5 AND D3
    {
        //if( subw_trend_type < StrategyTrendType::LONG_MAY_DOWN || is_subw_bias_allow 
        //    || (subw_trend_type >= StrategyTrendType::SSTRONG_SHORT && trend_distinguish_->main_bias() < -4.0) )
        {
            rel_infos.emplace_back(ForcastType::TREND_DOWN, iter->id, target_frct_site, ab_distance);
            rel_forcasts.insert(std::make_pair(iter->id, std::addressof(*iter)));
        } 
    }  

    if( real_long_pos_allow < 1 )
        return;

    if( !rel_infos.empty() )
    {
        //std::sort(rel_infos.begin(), rel_infos.end(), compare_sort_desc); // sort by ab distance desc
        // open LONG ------------------------------------
        bool is_long = true;
        //int i = 0;
        auto rel_info = rel_infos[0];  
        auto spread_type = JudgeTrendPriceSpreadType(main_k_type_, rel_info.ab_distance);
        //int targ_pos_count = (real_long_pos_allow > 1 ? 2 : 1); 
        int targ_pos_count = 1;
        if( only_allow_1_qty || danger_type_open_long >= OpenPosDangerType::HALF_DANGER )
            targ_pos_count = 1;
        else if( is_subw_bias_allow || subw_trend_type >= StrategyTrendType::SHORT && subw_trend_type < StrategyTrendType::SSTRONG_SHORT )
            targ_pos_count = 1;
        else
        {
            if( target_frct_site == ForcastSiteType::D1 )
                targ_pos_count = 1;
            else if( target_frct_site == ForcastSiteType::D2 ) 
                targ_pos_count = (real_long_pos_allow > 2 ? 2 : MAX_VAL(real_long_pos_allow, 1));
            else
                targ_pos_count = (real_long_pos_allow > 2 ? 3 : MAX_VAL(real_long_pos_allow, 1));
        }
        
        for( int j = 1; j <= targ_pos_count; ++j )
        {
            auto p_forcast = rel_forcasts.find(rel_info.forcast_id); 
            const double price_d1 = SiteProcDecimal(p_forcast->second->d1);
            const double price_d2 = SiteProcDecimal(p_forcast->second->d2);
            const double price_d3 = SiteProcDecimal(p_forcast->second->d3);

            auto positon_atom = std::make_shared<PositionAtom>();
            positon_atom->qty_available = 1;
            positon_atom->rel_forcast_info = rel_info;
            positon_atom->price = quote.price;
            positon_atom->is_long = is_long;
            set_open_flag(*p_forcast->second, rel_info.site_type);
            // set stop loss price----------
            if( spread_type <= PriceSpreadType::MICRO )
            {  
                if( rel_info.site_type == ForcastSiteType::D3 )
                    positon_atom->stop_loss_price = p_forcast->second->d3 - 0.3;
                else if( rel_info.site_type == ForcastSiteType::D2 )
                    positon_atom->stop_loss_price = p_forcast->second->d2 - 0.3;
                else
                    positon_atom->stop_loss_price = p_forcast->second->d1 - 0.2;
            }
            else if( spread_type < PriceSpreadType::BIG )
            { 
                if( rel_info.site_type == ForcastSiteType::D3 )
                    positon_atom->stop_loss_price = p_forcast->second->d3 - 0.4;
                else if( rel_info.site_type == ForcastSiteType::D2 )
                    positon_atom->stop_loss_price = p_forcast->second->d2 - 0.3;
                else
                    positon_atom->stop_loss_price = p_forcast->second->d1 - 0.2;
            }else  // big
            {
                double real_stop_price = quote.price - cst_default_stop_price_distance;
                if( rel_info.site_type == ForcastSiteType::D3 )
                {  
                    if( is_subw_bias_allow )
                        real_stop_price = p_forcast->second->d3 - 0.6;
                    else
                        real_stop_price = p_forcast->second->d3 - 0.4;
                }else if( rel_info.site_type == ForcastSiteType::D2p5 )
                { 
                    double mid_price = (price_d2 + price_d3)/2; 
                    if( fabs(mid_price - price_d3) > cst_default_stop_price_distance )
                        real_stop_price = mid_price - 0.3;
                    else
                        real_stop_price = p_forcast->second->d3 - 0.3;
                }else if( rel_info.site_type == ForcastSiteType::D2 )
                { 
                    if( fabs(price_d3 - price_d2) > cst_default_stop_price_distance )
                        real_stop_price = p_forcast->second->d2 - 0.3;
                    else
                        real_stop_price = p_forcast->second->d3 - 0.3;
                }else
                    real_stop_price = price_d1 - 0.2;
                positon_atom->stop_loss_price = real_stop_price;
            } 
            // set stop profit price----------
            double max_back_rate = 0.5;
            std::tuple<double, double, double> bounce_targets 
                = ForcastC_ABDown(k_hisdata[p_forcast->second->index_a]->stk_item.high_price
                , rel_info.site_type == ForcastSiteType::D1 ? price_d1 : (rel_info.site_type == ForcastSiteType::D2 ? price_d2 : price_d3)); 
            
            StepProfitPara profit_para;
            positon_atom->stop_profit_price = 9999.9;  
            if( spread_type <= PriceSpreadType::MICRO )
            {  
                if( long_pos_qty == 0 ) 
                {
                    if( account_info_.position.PositionCountRemainedForRunProfit(true) < 1 )
                        positon_atom->help_info.is_remain = true; 
                    if( is_subw_bias_allow )
                        profit_para.profit_target_0 = 1.2;
                    else
                        profit_para.profit_target_0 = std::get<2>(bounce_targets) - positon_atom->price; // c3 
                }else if( long_pos_qty == 1 ) // 2nd
                    profit_para.profit_target_0 = std::get<2>(bounce_targets) - positon_atom->price; // c3 
                else // 3rd
                    profit_para.profit_target_0 = std::get<1>(bounce_targets) - positon_atom->price; // c2 
#if IS_USE_AIM
                positon_atom->help_info.small_aim = std::get<0>(bounce_targets);
                positon_atom->help_info.mid_aim = std::get<2>(bounce_targets);
                positon_atom->help_info.big_aim = 9999.9;
#endif
            }else if( spread_type <= PriceSpreadType::SMALL )
            {   
                if( long_pos_qty == 0 ) 
                {
                    if( account_info_.position.PositionCountRemainedForRunProfit(true) < 1 )
                        positon_atom->help_info.is_remain = true; 
                }else if( long_pos_qty == 1 ) // 2nd
                    profit_para.profit_target_0 = std::get<0>(bounce_targets) - positon_atom->price; // c1 
                else
                    profit_para.profit_target_0 = (std::get<1>(bounce_targets) + std::get<2>(bounce_targets))/2 - positon_atom->price;  
#if IS_USE_AIM
                positon_atom->help_info.small_aim = std::get<0>(bounce_targets);
                positon_atom->help_info.mid_aim = (std::get<1>(bounce_targets) + std::get<2>(bounce_targets))/2; // 
                positon_atom->help_info.big_aim = std::get<2>(bounce_targets);
#endif 
            }else if( spread_type <= PriceSpreadType::MID )
            {
                if( long_pos_qty == 0 ) //1st
                {
                    if( account_info_.position.PositionCountRemainedForRunProfit(true) < 1 )
                        positon_atom->help_info.is_remain = true; 
                }else if( long_pos_qty == 1 ) //2nd
                    profit_para.profit_target_0 = std::get<0>(bounce_targets) - positon_atom->price; // c1 
                else
                    profit_para.profit_target_0 = (std::get<0>(bounce_targets) + std::get<1>(bounce_targets))/2 - positon_atom->price;  
#if IS_USE_AIM
                positon_atom->help_info.small_aim = std::get<0>(bounce_targets);
                positon_atom->help_info.mid_aim = (std::get<0>(bounce_targets) + std::get<1>(bounce_targets))/2; // 
                positon_atom->help_info.big_aim = std::get<2>(bounce_targets) - 0.2; // c3
#endif
            }else // big or super
            {
                std::tuple<double, double, double> bounce_targets_big
                    = ForcastC_ABDown(k_hisdata[p_forcast->second->index_c]->stk_item.high_price
                    , rel_info.site_type == ForcastSiteType::D2 ? price_d2 : price_d3); 
                if( long_pos_qty == 0 )// 1st
                {
                    if( account_info_.position.PositionCountRemainedForRunProfit(true) < 1 )
                        positon_atom->help_info.is_remain = true; 
                }else if( long_pos_qty == 1 ) //2nd
                { 
                    profit_para.profit_target_0 = std::get<0>(bounce_targets_big) - positon_atom->price; // c1 
                }else 
                    profit_para.profit_target_0 = std::get<1>(bounce_targets_big) - positon_atom->price; // c2 
#if IS_USE_AIM
                positon_atom->help_info.small_aim = std::get<0>(bounce_targets);
                positon_atom->help_info.mid_aim = std::get<1>(bounce_targets);
                positon_atom->help_info.big_aim = std::get<2>(bounce_targets) - 0.2;
#endif
            }
            positon_atom->float_stop_profit = std::make_shared<StepStopProfit>(*positon_atom, profit_para);

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
            app_.strategy_man()->AppendTradeRecord(OrderAction::OPEN, *positon_atom, quote, main_hisdatas_->size() - 1);
            Strategy_Log(utility::FormatStr("open LONG pos: %s (trend:%s bias:%.2f) fee:%.1f frozen:%.1f ava:%.1f"
                , positon_atom->String().c_str(), ToString(subw_trend_type).c_str(), trend_distinguish_->sub_bias(), fee, account_info_.capital.frozen, account_info_.capital.avaliable), &quote);
        }
    }
}


void TrendDownStrategy::JudgeStopLongLoss(const T_QuoteData &quote, INOUT unsigned int &short_pos_qty, INOUT unsigned int &long_pos_qty)
{
#if 0 
    static auto close_pos_atom = 
        [](Strategy *strategy, AccountInfo &account_info_, const T_QuoteData &quote, int trade_id, PositionAtom *pos_atom, unsigned int short_pos_qty, unsigned int &long_pos_qty)
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

        strategy->Strategy_Log(utility::FormatStr("Stop loss or fprofit LONG: price:%.1f %s (%s) profit:%.1f capital:%.1f"
            , quote.price, pos_atom->String().c_str()
            , pos_atom->follow_stop_profit ? pos_atom->follow_stop_profit->String(quote.price).c_str() : ""
            , p_profit, account_info_.capital.total()), &quote);
         
    };
#endif
    auto pos_infos = account_info_.position.LongPosSizeInfo();
    for( auto iter = pos_infos.begin(); iter != pos_infos.end(); ++iter )
    {
        int trade_id = iter->first;
        auto pos_atom = account_info_.position.FindPositionAtom(trade_id);
        if( pos_atom )
        {
            if( quote.price < pos_atom->stop_loss_price + cst_tolerance_equal )
            { 
#if 0
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
                Strategy_Log(utility::FormatStr("order stop loss or float profit LONG pos: price:%.1f %s profit:%.1f capital:%.1f"
                    , quote.price, pos_atom->String().c_str(), p_profit, account_info_.capital.total()), &quote);
#else
                ClosePositionAtom(quote, trade_id, pos_atom, short_pos_qty, long_pos_qty);
#endif
                continue;
            } 
            if( pos_atom->float_stop_profit && pos_atom->float_stop_profit->UpdateAndJudgeStop(quote.price) )
            {
                ClosePositionAtom(quote, trade_id, pos_atom, short_pos_qty, long_pos_qty);
            }
        }
    }// for
#if  TREND_DOWN_ENABLE_SHORT
    //-----------------consider stop short profit-------------
    T_HisDataItemContainer &k_hisdata = *main_hisdatas_;
    //unsigned int long_pos_qty = account_info_.position.LongPosQty();
    //unsigned int short_pos_qty = account_info_.position.ShortPosQty();
    // consider stop profit of short position --------------
    auto short_pos_infos = account_info_.position.ShortPosSizeInfo();
    for( auto iter = short_pos_infos.begin(); iter != short_pos_infos.end(); ++iter )
    {
        int trade_id = iter->first;
        auto pos_atom = account_info_.position.FindPositionAtom(trade_id);
        if( !pos_atom )
            continue;
        assert(pos_atom->qty_all() == 1);
        if( pos_atom->rel_forcast_info.forcast_type != ForcastType::TREND_DOWN )
            continue;
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
            Strategy_Log(utility::FormatStr("order stop profit SHORT pos: price:%.1f %s profit:%.1f capital:%.1f"
                , quote.price, pos_atom->String().c_str(), p_profit, account_info_.capital.total()), &quote);
        }
    }

    //----------------consider open short------------------
    unsigned int net_qty = MAX_VAL(short_pos_qty, long_pos_qty);
    auto frozen_capital = net_qty > 0 ? CaculateOpenPositionFreezeCapital(quote.price, net_qty) : 0.0;
    auto pos_capital_allow_n = (unsigned int)CalculateMaxQtyAllowOpen(account_info_.capital.avaliable + frozen_capital, quote.price);
    assert(pos_capital_allow_n >= short_pos_qty);
    auto remain_short_allow = pos_capital_allow_n - short_pos_qty;
    assert(cst_position_max >= short_pos_qty);
    unsigned int real_short_pos_allow = MIN_VAL(remain_short_allow, cst_position_max - short_pos_qty);

    if( real_short_pos_allow < 1 )
        return;
    //T_HisDataItemContainer &k_hisdata = *main_hisdatas_;
    Data3pForcastInnerContainer & main_trends_downs_copy = *main_trends_downs_;

    std::vector<RelForcastInfo> rel_infos;
    std::unordered_map<unsigned int, T_Data3pForcast *>  rel_forcasts;
    for( auto iter = main_trends_downs_copy.begin(); iter != main_trends_downs_copy.end(); ++iter )
    { 
        if( iter->opened_short_d3 )
            continue;
        double ab_distance = k_hisdata.at(iter->index_a)->stk_item.high_price - k_hisdata.at(iter->index_b)->stk_item.low_price;
        auto spread_type = JudgeTrendPriceSpreadType(main_k_type_, ab_distance);
        if( spread_type > PriceSpreadType::SMALL )
            continue;
        if( iter->break_d3_n == 0 )
        {
            if( quote.price < iter->d3 - 0.1 + cst_tolerance_equal )
                iter->break_d3_n = 1;
        }else if( iter->bounce_over_d3 && quote.price < iter->d3 - 0.2 + cst_tolerance_equal ) // break again
        {
            iter->break_d3_n += 1;
            iter->bounce_over_d3 = false; //reset 
        }

        if( iter->break_d3_n != 2 )
            continue;

        if( quote.price < iter->d3 - 0.1 + cst_tolerance_equal
            && quote.price > iter->d3 - 0.3 )
        {
            double ab_distance = k_hisdata.at(iter->index_a)->stk_item.high_price - k_hisdata.at(iter->index_b)->stk_item.low_price;
            rel_infos.emplace_back(ForcastType::TREND_DOWN, iter->id, ForcastSiteType::D3, ab_distance);
            rel_forcasts.insert(std::make_pair(iter->id, std::addressof(*iter)));
        }
    }

    if( !rel_infos.empty() )
    {
        std::sort(rel_infos.begin(), rel_infos.end(), compare_sort_desc);
        // open short ------------------------------------
        bool is_long = false;
        int i = 0;
        auto rel_info = rel_infos[i];
        auto spread_type = JudgePriceSpreadType(main_k_type_, rel_info.ab_distance);
        int targ_pos_count = 0;
         
        targ_pos_count = (real_short_pos_allow > 2 ? 3 : MAX_VAL(real_short_pos_allow, 1));

        for( int j = 1; j <= targ_pos_count; ++j )
        {
            auto p_forcast = rel_forcasts.find(rel_info.forcast_id);
            p_forcast->second->opened_short_d3 = true;

            auto positon_atom = std::make_shared<PositionAtom>();
            positon_atom->qty_available = 1;
            positon_atom->rel_forcast_info = rel_info;
            positon_atom->price = quote.price;
            positon_atom->is_long = is_long;
            positon_atom->stop_loss_price = p_forcast->second->d3 + 0.2;

            auto targes = ForcastD_ABC_Down(k_hisdata[p_forcast->second->index_c]->stk_item.high_price, p_forcast->second->d3); 

            if(j == 1)
            {
                positon_atom->stop_profit_price = quote.price - cst_default_stop_price_distance;
            }else if(j == 2)
            {
                positon_atom->stop_profit_price = quote.price - cst_default_stop_price_distance; 
            }else 
            {
                positon_atom->stop_profit_price = (std::get<1>(targes) + std::get<2>(targes)) / 2; // 
            }

            positon_atom->help_info.small_aim = quote.price - cst_default_stop_price_distance;
            positon_atom->help_info.mid_aim = quote.price - 2*cst_default_stop_price_distance;
            positon_atom->help_info.big_aim = quote.price - 3*cst_default_stop_price_distance;

            positon_atom->trade_id = account_info_.position.GenerateTradeId();
            account_info_.position.PushBack(is_long, positon_atom);
            double margin = CaculateOpenPositionFreezeCapital(positon_atom->price, positon_atom->qty_available);
            double fee = CalculateFee(positon_atom->qty_available, positon_atom->price, false);
            if( short_pos_qty >= long_pos_qty ) // ps: 考虑多空 共享保证金
            {
                account_info_.capital.avaliable -= margin + fee;
                account_info_.capital.frozen += margin;
            }else
                account_info_.capital.avaliable -= fee;
            short_pos_qty += 1;
            app_.strategy_man()->AppendTradeRecord(OrderAction::OPEN, *positon_atom, quote, main_hisdatas_->size() - 1);
            Strategy_Log(utility::FormatStr("OPEN SHORT pos: %s fee:%.1f frozen:%.1f ava:%.1f"
                , positon_atom->String().c_str(), fee, account_info_.capital.frozen, account_info_.capital.avaliable), &quote);
        }
    }
#endif
}


void TrendDownStrategy::JudgeStopLongProfit(const T_QuoteData &quote)
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
            Strategy_Log(utility::FormatStr("STOP profit LONG: price:%.1f %s profit:%.1f capital:%.1f"
                , quote.price, pos_atom->String().c_str(), p_profit, account_info_.capital.total()), &quote);
        }
#if IS_USE_AIM        
        else
        {   //consider ajust stop_loss_price
            if( IsTrendDownPriceNear(quote.price, pos_atom->help_info.big_aim, *pos_atom, main_k_type_) )
            { 
                pos_atom->help_info.has_near_big_aim = pos_atom->help_info.has_near_mid_aim = pos_atom->help_info.has_near_small_aim = true;
                if( pos_atom->follow_stop_profit )
                {
                    pos_atom->stop_loss_price = pos_atom->help_info.small_aim;
                }else
                {
                    if( fabs(pos_atom->help_info.big_aim - pos_atom->help_info.mid_aim) > 0.6 )
                        pos_atom->stop_loss_price = pos_atom->help_info.mid_aim;
                    else
                        pos_atom->stop_loss_price = pos_atom->help_info.small_aim;
                }
                
            }
#if 1
            else if( IsTrendDownPriceNear(quote.price, pos_atom->help_info.mid_aim, *pos_atom, main_k_type_) )
            {
                pos_atom->help_info.has_near_mid_aim = pos_atom->help_info.has_near_small_aim = true;
                if( pos_atom->help_info.is_remain || pos_atom->follow_stop_profit )
                    pos_atom->stop_loss_price = pos_atom->price;
                else
                {
                    if( fabs(pos_atom->help_info.mid_aim - pos_atom->help_info.small_aim) > 0.6 )
                        pos_atom->stop_loss_price = pos_atom->help_info.small_aim;
                    else
                        pos_atom->stop_loss_price = pos_atom->price;
                }

            }else if( IsTrendDownPriceNear(quote.price, pos_atom->help_info.small_aim, *pos_atom, main_k_type_) )
            {    
                pos_atom->help_info.has_near_small_aim = true;
                //if( !pos_atom->help_info.is_remain )
                pos_atom->stop_loss_price = pos_atom->price;

            }
#endif 
        }
#endif
    }
#if TREND_DOWN_ENABLE_SHORT
    //----------------consider stop short loss---
    auto short_pos_infos = account_info_.position.ShortPosSizeInfo();
    for( auto iter = short_pos_infos.begin(); iter != short_pos_infos.end(); ++iter )
    {
        int trade_id = iter->first;
        auto pos_atom = account_info_.position.FindPositionAtom(trade_id);
        if( pos_atom )
        {
            if( pos_atom->rel_forcast_info.forcast_type != ForcastType::TREND_DOWN )
                continue;
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
                Strategy_Log(utility::FormatStr("STOP loss or float profit SHORT: price:%.1f %s profit:%.1f capital:%.1f"
                    , quote.price, pos_atom->String().c_str(), p_profit, account_info_.capital.total()), &quote);
            } 
        }
    }

    //--------------------------------
#endif
}


PriceNearType JudgePriceTrendDownNear(PriceSpreadType spread_type, double price, double tag_price)
{
    double ceil_tolerance_price = 0.0;
    double floor_tolerance_price = 0.0;
    if( spread_type <= PriceSpreadType::MICRO )
    {
        ceil_tolerance_price = 0.1;
        floor_tolerance_price = 0.1;
    }
    else if( spread_type <= PriceSpreadType::SMALL )
    {
        ceil_tolerance_price = 0.1;
        floor_tolerance_price = 0.1;
    }
    else if( spread_type <= PriceSpreadType::MID )
    {
        ceil_tolerance_price = 0.1;
        floor_tolerance_price = 0.1;
    }
    else // >= BIG
    {
        ceil_tolerance_price = 0.2;
        floor_tolerance_price = 0.1;
    }
    if( price > tag_price + ceil_tolerance_price ) // upper
        return PriceNearType::UNTOUCH;
    else if( price < tag_price + ceil_tolerance_price + cst_tolerance_equal
        && price > tag_price - floor_tolerance_price - cst_tolerance_equal )
        return PriceNearType::FIT;  // fit 
    if( price < tag_price - 0.2 + cst_tolerance_equal )
        return PriceNearType::BREAK;
    else 
        return PriceNearType::OTHER;
}

bool IsTrendDownPriceNear(double price, double tag_price, const PositionAtom &pos_atom, TypePeriod type_period )
{
    auto type = JudgePriceSpreadType(type_period, pos_atom.rel_forcast_info.ab_distance);
    if( type <= PriceSpreadType::MICRO )
        return fabs(price - tag_price) < 0.15;
    /*else if( type <= PriceSpreadType::SMALL )
        return fabs(price - tag_price) < 0.3;*/
    else if( type <= PriceSpreadType::MID )
        return fabs(price - tag_price) < 0.25;
    else
        return fabs(price - tag_price) < 0.35;
}

double SiteProcDecimal(double val, unsigned int decimal)
{
    int temp = pow(10, decimal);
    int64_t big_val = int64_t(fabs(val * temp) + 0.7) * (val < 0 ? - 1 : 1); //2舍3入
    return double(big_val) / temp;
}