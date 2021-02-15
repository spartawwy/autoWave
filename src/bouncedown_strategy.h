#ifndef BREAK_DOWN_STRATEGY_DSFSDFS3JKSDF_H_
#define BREAK_DOWN_STRATEGY_DSFSDFS3JKSDF_H_

#include "strategy.h"

class TrendDistinguish;
class BounceDownStrategy : public Strategy
{
public:
    BounceDownStrategy(FuturesForecastApp &app, AccountInfo &account_info, std::shared_ptr<TrendDistinguish> &trend_distinguish);
    ~BounceDownStrategy(){}
    //virtual void Initiate(){}
    virtual void Handle(const T_QuoteData &quote) override;

private:
    void JudgeOpenLong(const T_QuoteData &quote, unsigned int short_pos_qty, INOUT unsigned int &long_pos_qty);
    void JudgeStopLongLoss(const T_QuoteData &quote, unsigned int short_pos_qty, INOUT unsigned int &long_pos_qty);
    void JudgeStopLongProfit(const T_QuoteData &quote);

    std::shared_ptr<TrendDistinguish> trend_distinguish_;
};
#endif