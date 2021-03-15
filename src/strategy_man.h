#ifndef STRATEGY_MAN_SDFS_H_
#define STRATEGY_MAN_SDFS_H_

#include <memory>
#include "position_account.h"

class FuturesForecastApp;
class Strategy;
class TrendDistinguish;
class TrendLine;
class LineStrategyMan;
class StrategyMan
{
public:
    StrategyMan(FuturesForecastApp &app);
    ~StrategyMan(){}

    bool Init();

    void Handle(const T_QuoteData &quote);

    void AppendTradeRecord(OrderAction action, PositionAtom &positon_atom, const T_QuoteData &quote, int index);
    void AppendTrendLineStrategy(std::shared_ptr<TrendLine> &);

    AccountInfo & account_info(){ return account_info_; }
    unsigned int GenerateId(){ return ++id_; }
protected:
    std::vector<std::shared_ptr<Strategy> > GetEnabledStrategys();

private:
    FuturesForecastApp &app_;
    AccountInfo account_info_;
    std::shared_ptr<TrendDistinguish> trend_distinguish_;

    std::unordered_map<unsigned int, std::shared_ptr<Strategy> > strategys_;
    std::shared_ptr<LineStrategyMan> line_strategy_man_;
    std::atomic<unsigned int> id_;

};

#endif // STRATEGY_MAN_SDFS_H_