#ifndef TREND_UP_LINE_STRATEGY_SDFDK_H_
#define TREND_UP_LINE_STRATEGY_SDFDK_H_

#include "strategy.h"
 
class TrendUpLineStrategy : public Strategy
{
public:
    TrendUpLineStrategy(FuturesForecastApp &app, AccountInfo &account_info);
    ~TrendUpLineStrategy(){}
    //virtual void Initiate(){}
    virtual void Handle(const T_QuoteData &quote) override;

private:
    void JudgeOpenShort(const T_QuoteData &quote, INOUT unsigned int &short_pos_qty, unsigned int long_pos_qty);
    void JudgeStopShortLoss(const T_QuoteData &quote, INOUT unsigned int &short_pos_qty, unsigned int long_pos_qty);
    void JudgeStopShortProfit(const T_QuoteData &quote);

    //std::shared_ptr<TrendDistinguish> trend_distinguish_;
};

#endif // TREND_UP_LINE_STRATEGY_SDFDK_H_