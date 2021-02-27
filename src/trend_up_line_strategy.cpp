#include "trend_up_line_strategy.h"

TrendUpLineStrategy::TrendUpLineStrategy(FuturesForecastApp &app, AccountInfo &account_info):Strategy(app, account_info)
{

}

void TrendUpLineStrategy::Handle(const T_QuoteData &quote)
{

}
 
void TrendUpLineStrategy::JudgeOpenShort(const T_QuoteData &quote, INOUT unsigned int &short_pos_qty, unsigned int long_pos_qty)
{

}
void TrendUpLineStrategy::JudgeStopShortLoss(const T_QuoteData &quote, INOUT unsigned int &short_pos_qty, unsigned int long_pos_qty)
{

}

void TrendUpLineStrategy::JudgeStopShortProfit(const T_QuoteData &quote)
{

}