#ifndef STRATEGY_MAN_SDFS_H_
#define STRATEGY_MAN_SDFS_H_

#include <memory>
#include "position_account.h"

class FuturesForecastApp;
class Strategy;
class TrendDistinguish;
class StrategyMan
{
public:
    StrategyMan(FuturesForecastApp &app);
    ~StrategyMan(){}

    bool Init();

    void Handle(const T_QuoteData &quote);

    void AppendTradeRecord(OrderAction action, PositionAtom &positon_atom, const T_QuoteData &quote, int index);

    AccountInfo & account_info(){ return account_info_; }

protected:
    std::vector<std::shared_ptr<Strategy> > GetEnabledStrategys();

private:
    FuturesForecastApp &app_;
    AccountInfo account_info_;
    std::shared_ptr<TrendDistinguish> trend_distinguish_;

    std::vector<std::shared_ptr<Strategy> > strategys_;
};

#endif // STRATEGY_MAN_SDFS_H_