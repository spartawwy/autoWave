#ifndef TREND_DOWN_LSDFHKSDFDK_H_
#define TREND_DOWN_LSDFHKSDFDK_H_
#include "strategy.h"

class TrendDistinguish;
class TrendDownStrategy : public Strategy
{
public:
    TrendDownStrategy(FuturesForecastApp &app, AccountInfo &account_info, std::shared_ptr<TrendDistinguish> &trend_distinguish);
    ~TrendDownStrategy(){}

    virtual void Handle(const T_QuoteData &quote) override;

protected:
    //virtual void Strategy_Log(const std::string &content, const T_QuoteData *quote=nullptr) override;

private:
    void JudgeOpenLong(const T_QuoteData &quote, unsigned int short_pos_qty, INOUT unsigned int &long_pos_qty);
    void JudgeStopLongLoss(const T_QuoteData &quote, INOUT unsigned int & short_pos_qty, INOUT unsigned int &long_pos_qty);
    void JudgeStopLongProfit(const T_QuoteData &quote);

    std::shared_ptr<TrendDistinguish> trend_distinguish_;
};

PriceNearType JudgePriceTrendDownNear(PriceSpreadType spread_type, double price, double tag_price);

bool IsTrendDownPriceNear(double price, double tag_price, const PositionAtom &pos_atom, TypePeriod type_period);
 

#endif // TREND_DOWN_LSDFHKSDFDK_H_