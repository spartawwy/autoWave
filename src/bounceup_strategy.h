#ifndef BOUNCEUP_STRATETY_SDF3DFS_H_
#define BOUNCEUP_STRATETY_SDF3DFS_H_
#include "strategy.h"

class TrendDistinguish;
class BounceUpStrategy : public Strategy
{
public:
    BounceUpStrategy(FuturesForecastApp &app, AccountInfo &account_info, std::shared_ptr<TrendDistinguish> &trend_distinguish);
    ~BounceUpStrategy(){}
    //virtual void Initiate(){}
    virtual void Handle(const T_QuoteData &quote) override;

private:
    void JudgeOpenShort(const T_QuoteData &quote, INOUT unsigned int &short_pos_qty, unsigned int long_pos_qty);
    void JudgeStopShortLoss(const T_QuoteData &quote, INOUT unsigned int &short_pos_qty, unsigned int long_pos_qty);
    void JudgeStopShortProfit(const T_QuoteData &quote);

    std::shared_ptr<TrendDistinguish> trend_distinguish_;
};

#endif // BOUNCEUP_STRATETY_SDF3DFS_H_