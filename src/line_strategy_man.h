#ifndef LINE_STRATEGY_MAN_SDF_H_
#define LINE_STRATEGY_MAN_SDF_H_

#include "strategy.h"

class TrendUpLineStrategy;
class TrendLine;
class LineStrategyMan : public Strategy
{
public:
    LineStrategyMan(FuturesForecastApp &app, AccountInfo &account_info);
    virtual ~LineStrategyMan(){}

    virtual void Handle(const T_QuoteData &quote) override;

    void AppendStrategy(std::shared_ptr<TrendLine> &trend_line);

private:

    //FuturesForecastApp &app_;
    //AccountInfo &account_info_;
    LineStrategyMan();

    std::deque<std::shared_ptr<TrendUpLineStrategy> > up_line_strategys_;
    std::deque<std::shared_ptr<TrendUpLineStrategy> > new_up_line_strategys_;

};

#endif // LINE_STRATEGY_MAN_SDF_H_