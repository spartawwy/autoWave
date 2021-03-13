#include "trend_up_line_strategy.h"

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

    //if( ProdIfNearTradingEndTime(quote) )
        //return;

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
END_PROC:

    pre_price_ = quote.price;
}
 
void TrendUpLineStrategy::JudgeOpenLong(const T_QuoteData &quote, unsigned int short_pos_qty, INOUT unsigned int &long_pos_qty)
{

}
void TrendUpLineStrategy::JudgeStopLongLoss(const T_QuoteData &quote, unsigned int short_pos_qty, INOUT unsigned int &long_pos_qty)
{

}

void TrendUpLineStrategy::JudgeStopLongProfit(const T_QuoteData &quote)
{

}