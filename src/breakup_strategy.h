#ifndef BREAK_UP_STRATEGY_DSFDSKJKSDF_H_
#define BREAK_UP_STRATEGY_DSFDSKJKSDF_H_
#include "strategy.h"

class TrendDistinguish;
class BreakUpStrategy : public Strategy
{
public:
    BreakUpStrategy(FuturesForecastApp &app, AccountInfo &account_info, std::shared_ptr<TrendDistinguish> &trend_distinguish);
    ~BreakUpStrategy(){}
    //virtual void Initiate(){}
    virtual void Handle(const T_QuoteData &quote) override;
 
private:
    void JudgeOpenLong(const T_QuoteData &quote, unsigned int short_pos_qty, INOUT unsigned int &long_pos_qty);
    void JudgeStopLongLoss(const T_QuoteData &quote, INOUT unsigned int &short_pos_qty, INOUT unsigned int &long_pos_qty);
    void JudgeStopLongProfit(const T_QuoteData &quote);

private:
    std::shared_ptr<TrendDistinguish> trend_distinguish_;

    std::unordered_map<unsigned int, std::shared_ptr<T_Data2pForcast> > observed_forcasts_;
    std::unordered_map<unsigned int, std::shared_ptr<T_Data2pForcast> > forcasts_prepare_buy_;

};

PriceNearType JudgePriceFallNear(double price, double tag_price);
double ProcDecimal_1(double val, unsigned int decimal = 1);

#endif // BREAK_UP_STRATEGY_DSFDSKJKSDF_H_