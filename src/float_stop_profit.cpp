#include "float_stop_profit.h"
#include <Tlib/core/tsystem_core_common.h>
//#include <TLib/core/tsystem_time.h>

#include "stkfo_common.h"
#include "position_account.h"

using namespace TSystem;
FloatStopProfit::FloatStopProfit(PositionAtom &postion_atom)
    : enabled_(false), postion_atom_(postion_atom), max_profit_price_touch_(postion_atom_.price) 
{
    //assert(enable_threshold_price > 0.01);
    //assert(max_back_rate > 0.01);
    assert(postion_atom_.price > 0.01);
    //assert(postion_atom.is_long ? max_target > postion_atom.price : max_target < postion_atom.price);
}


FollowStopProfit::FollowStopProfit(PositionAtom &postion_atom, double enable_threshold_price, double max_back_rate, double max_target)
    : FloatStopProfit(postion_atom), enable_threshold_price_(enable_threshold_price)
    /*, max_profit_price_touch_(postion_atom_.price)*/, max_back_rate_(max_back_rate), max_target_(max_target)
{
    //assert(enable_threshold_price > 0.01);
    assert(max_back_rate > 0.01);
    assert(postion_atom_.price > 0.01);
    assert(max_back_rate > 0.01); 
    assert(postion_atom.is_long ? max_target > postion_atom.price : max_target < postion_atom.price);
}
// ------------------------------------------------------
//return : is to stop profit or loss;
// ------------------------------------------------------
bool FollowStopProfit::UpdateAndJudgeStop(double price, std::string *p_ret_info)
{  
    const double max_target_profit = (postion_atom_.is_long ? max_target_ - postion_atom_.price : postion_atom_.price - max_target_); //10.0;

    double max_profit_touch = (postion_atom_.is_long ? max_profit_price_touch_ - postion_atom_.price : postion_atom_.price - max_profit_price_touch_);
#if 0  // 20210314
    // prevent has certain amount profit to loss    
    //if( max_profit_touch > 0.3 )//0.4
    if( max_profit_touch > 0.9 )// 20210314
    {
        const double cst_stop_loss_distance = 0.1;
        double stop_price = (postion_atom_.is_long ? postion_atom_.price - cst_stop_loss_distance : postion_atom_.price + cst_stop_loss_distance);
        bool is_to_stop = (postion_atom_.is_long ? price < stop_price + EPSINON : price > stop_price - EPSINON);
        if( is_to_stop )
            return true;
    }
#endif
    bool is_max_profit_increase = postion_atom_.is_long ? price > max_profit_price_touch_ : price < max_profit_price_touch_;
    if( is_max_profit_increase )
    {
        max_profit_price_touch_ = price;
        return false;
    }else 
    {
        double dynamic_bake_rate = max_back_rate_;
        if( (postion_atom_.is_long && price - postion_atom_.price > max_target_profit - EPSINON) 
            || (!postion_atom_.is_long && postion_atom_.price - price > max_target_profit - EPSINON) )
            dynamic_bake_rate = 0.16;
        else if( max_profit_touch > 2.0 - EPSINON && max_profit_touch < 4.0 )
            dynamic_bake_rate = 0.618;
        else if( max_profit_touch > 4.0 && max_profit_touch < 8.0 )
            dynamic_bake_rate = 0.333;
        else if( max_profit_touch > 8.0 - EPSINON && max_profit_touch < 9.5 )
            dynamic_bake_rate = 0.233;
        else if( max_profit_touch > 9.5 - EPSINON )
            dynamic_bake_rate = 0.16;
        if( postion_atom_.is_long )
        {
            //if( price - postion_atom_.price > max_target_profit )
            //    return true;
            if( max_profit_price_touch_ < postion_atom_.price + enable_threshold_price_ )
                return false;
            else
                enabled_ = true;
            assert( !(max_profit_price_touch_ < price)  ); 
            if( max_profit_price_touch_ > postion_atom_.price ) // profit
            {
                bool is_stop = (max_profit_price_touch_ - price) / max_profit_touch > dynamic_bake_rate;
                if( is_stop && p_ret_info )
                   *p_ret_info = utility::FormatStr("(%.1f-%.1f)/%.1f > %.1f", max_profit_price_touch_, price, max_profit_touch, dynamic_bake_rate);
                return is_stop;
            }else // loss
                return false;
        }else // short position
        {
            //if( postion_atom_.price - price > max_target_profit )
            //   return true;
            if( max_profit_price_touch_ > postion_atom_.price - enable_threshold_price_ )
                return false;
            else
                enabled_ = true;
            assert( !(max_profit_price_touch_ > price)  ); 
            if( max_profit_price_touch_ < postion_atom_.price ) // profit
            {
                bool is_stop = (price - max_profit_price_touch_) / max_profit_touch > dynamic_bake_rate;
                if( is_stop && p_ret_info )
                    *p_ret_info = utility::FormatStr("(%.1f-%.1f)/%.1f > %.1f", price, max_profit_price_touch_, max_profit_touch, dynamic_bake_rate);
                return is_stop;
            }else // loss
                return false;
        }
    }
}

std::string FollowStopProfit::String(double price)
{
    double bake_rate = 0.0;
    if( !Equal(max_profit_price_touch_, postion_atom_.price) )
        bake_rate = (postion_atom_.is_long ? (max_profit_price_touch_ - price) / (max_profit_price_touch_ - postion_atom_.price) : (price - max_profit_price_touch_) / (max_profit_price_touch_ - postion_atom_.price) );
    return utility::FormatStr("mpp:%.2f br:%.2f enabled:%d", max_profit_price_touch_, bake_rate, enabled_);
}

StepStopProfit::StepStopProfit(PositionAtom &postion_atom, const StepProfitPara &para) : FloatStopProfit(postion_atom), para_(para)
{ 
    std::vector<double> targets;
    targets.push_back(para.profit_target_0);
    targets.push_back(para.profit_target_1);
    targets.push_back(para.profit_target_2);
    targets.push_back(para.profit_target_3);
    std::sort(targets.begin(), targets.end(), [](double lh, double rh){ return lh < rh;});
    para_.profit_target_0 = targets[0];
    para_.profit_target_1 = targets[1];
    para_.profit_target_2 = targets[2];
    para_.profit_target_3 = targets[3];
    /*{
        assert(para.profit_target_0 > 0.0);
        assert(para.profit_target_1 >= para.profit_target_0);
        assert(para.profit_target_2 >= para.profit_target_1);
        assert(para.profit_target_3 >= para.profit_target_2);
    } */
}

// ------------------------------------------------------
//return : is to stop profit or loss;
// ------------------------------------------------------
bool StepStopProfit::UpdateAndJudgeStop(double price, std::string *p_ret_info)
{  
    //const double max_target_profit = (postion_atom_.is_long ? max_target_ - postion_atom_.price : postion_atom_.price - max_target_); //10.0;
#if 1
    double max_profit_touch = (postion_atom_.is_long ? max_profit_price_touch_ - postion_atom_.price : postion_atom_.price - max_profit_price_touch_);
    if( max_profit_touch > para_.profit_target_0 - EPSINON )
        enabled_ = true;

    if( max_profit_touch > 0.3 )//0.4
    {
        const double cst_stop_loss_distance = 0.0;
        double stop_price = (postion_atom_.is_long ? postion_atom_.price - cst_stop_loss_distance : postion_atom_.price + cst_stop_loss_distance);
        bool is_to_stop = (postion_atom_.is_long ? price < stop_price + EPSINON : price > stop_price - EPSINON);
        if( is_to_stop )
            return true;
    }
#endif
    bool is_max_profit_increase = postion_atom_.is_long ? price > max_profit_price_touch_ : price < max_profit_price_touch_;
    if( is_max_profit_increase )
    {
        max_profit_price_touch_ = price;
        return false;
    }else if( enabled_ )
    {
        double dynamic_bake_rate = 0.5;
        if( max_profit_touch > para_.profit_target_0 - EPSINON && max_profit_touch < para_.profit_target_1 )
            dynamic_bake_rate = para_.back_rate_0;
        else if( max_profit_touch > para_.profit_target_1 - EPSINON && max_profit_touch < para_.profit_target_2 )
            dynamic_bake_rate = para_.back_rate_1;
        else if( max_profit_touch > para_.profit_target_2 - EPSINON && max_profit_touch < para_.profit_target_3 )
            dynamic_bake_rate = para_.back_rate_2;
        else if( max_profit_touch > para_.profit_target_3 - EPSINON )
            dynamic_bake_rate = para_.back_rate_3;
        if( postion_atom_.is_long )
        { 
            assert( !(max_profit_price_touch_ < price)  ); 
            if( max_profit_price_touch_ > postion_atom_.price ) // profit
                return (max_profit_price_touch_ - price) / max_profit_touch > dynamic_bake_rate;
            else // loss
                return false;
        }else // short position
        {
            assert( !(max_profit_price_touch_ > price)  ); 
            if( max_profit_price_touch_ < postion_atom_.price ) // profit
                return (price - max_profit_price_touch_) / max_profit_touch > dynamic_bake_rate;
            else // loss
                return false;
        }
    }else
        return false;
}

std::string StepStopProfit::String(double price)
{
    double bake_rate = 0.0;
    if( !Equal(max_profit_price_touch_, postion_atom_.price) )
        bake_rate = (postion_atom_.is_long ? (max_profit_price_touch_ - price) / (max_profit_price_touch_ - postion_atom_.price) : (price - max_profit_price_touch_) / (max_profit_price_touch_ - postion_atom_.price) );
    return utility::FormatStr("mpp:%.2f br:%.2f enabled:%d", max_profit_price_touch_, bake_rate, enabled_);
}