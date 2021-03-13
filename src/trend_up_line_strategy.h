#ifndef TREND_UP_LINE_STRATEGY_SDFDK_H_
#define TREND_UP_LINE_STRATEGY_SDFDK_H_

#include "strategy.h"
 
class TrendLine;
class TrendUpLineStrategy : public Strategy
{
public:
    TrendUpLineStrategy(FuturesForecastApp &app, AccountInfo &account_info, std::shared_ptr<TrendLine> &trend_line);
    ~TrendUpLineStrategy(){}
    //virtual void Initiate(){}
    virtual void Handle(const T_QuoteData &quote) override;

    const std::shared_ptr<TrendLine> & trend_line() { return trend_line_; }

private:
    TrendUpLineStrategy();
    void JudgeOpenLong(const T_QuoteData &quote, unsigned int short_pos_qty, INOUT unsigned int &long_pos_qty);
    void JudgeStopLongLoss(const T_QuoteData &quote, unsigned int short_pos_qty, INOUT unsigned int &long_pos_qty);
    void JudgeStopLongProfit(const T_QuoteData &quote);

    //std::shared_ptr<TrendDistinguish> trend_distinguish_;
    std::shared_ptr<TrendLine> trend_line_;
};

#endif // TREND_UP_LINE_STRATEGY_SDFDK_H_