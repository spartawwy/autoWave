#include "line_strategy_man.h"
#include "trend_line.h"

#include "up_line_strategy.h"
#include "futures_forecast_app.h"

LineStrategyMan::LineStrategyMan(FuturesForecastApp &app, AccountInfo &account_info) : Strategy(app, account_info)
{

}

void LineStrategyMan::Handle(const T_QuoteData &quote)
{
    for( unsigned int i = 0; i < up_line_strategys_.size(); ++ i )
    {
        up_line_strategys_[i]->Handle(quote);
    }

    //const int hhmm = quote.hhmmss/100; 
    if( IsNearCloseKTime(quote.hhmmss, TypePeriod::PERIOD_5M) )
    {
        for( unsigned int j = 0; j < up_line_strategys_.size(); ++j )
        {
            if( (!up_line_strategys_[j]->trend_line()->is_alive_ || !up_line_strategys_[j]->trend_line()->is_last_)
               && !up_line_strategys_[j]->account_info().position.IsExistPositionAtomByStrategyId(up_line_strategys_[j]->id())
               )
               continue;
            new_up_line_strategys_.push_front(up_line_strategys_[j]);
        }
        up_line_strategys_.swap(new_up_line_strategys_);
        //up_line_strategys_.insert(up_line_strategys_.end(), new_up_line_strategys_.begin(), new_up_line_strategys_.end());
        new_up_line_strategys_.clear();
    }
}

void LineStrategyMan::AppendStrategy(std::shared_ptr<TrendLine> &trend_line)
{
    auto iter = std::find_if(up_line_strategys_.begin(), up_line_strategys_.end(),[this, &trend_line](std::shared_ptr<TrendUpLineStrategy> &entry)
    {
        return entry->trend_line()->id_ == trend_line->id_;
    });
    assert(iter == up_line_strategys_.end());
    auto iter0 = std::find_if(new_up_line_strategys_.begin(), new_up_line_strategys_.end(),[this, &trend_line](std::shared_ptr<TrendUpLineStrategy> &entry)
    {
        return entry->trend_line()->id_ == trend_line->id_;
    });
    assert(iter0 == new_up_line_strategys_.end());
    if( trend_line->type_ == TrendLineType::UP )
    {
        auto line_strategy = std::make_shared<TrendUpLineStrategy>(app_, account_info_, trend_line);
        new_up_line_strategys_.push_back(line_strategy);
    }else
    {
        //auto line_strategy = std::make_shared<TrendDownStrategy>(app_, account_info_, trend_line);
        //new_up_line_strategys_.push_back(line_strategy);
    }
}