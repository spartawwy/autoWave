#include "strategy_man.h"

#include "trend_distinguish.h"
#include "trenddown_strategy.h"
#include "bounceup_strategy.h"
#include "bouncedown_strategy.h"
#include "breakup_strategy.h"
#include "line_strategy_man.h"

#include "futures_forecast_app.h"
#include "capital_curve.h"

StrategyMan::StrategyMan(FuturesForecastApp &app):app_(app)
    , trend_distinguish_(nullptr)
{
    account_info_.capital.avaliable = cst_default_ori_capital;
}

bool StrategyMan::Init()
{
    line_strategy_man_ = std::make_shared<LineStrategyMan>(app_, account_info_);

    trend_distinguish_ = std::make_shared<TrendDistinguish>(app_, account_info_);
    trend_distinguish_->Init();
#if 0
    auto strategy = std::make_shared<TrendDownStrategy>(app_, account_info_, trend_distinguish_);
    strategy->Initiate();
    strategys_.push_back(strategy);
#endif
#if 0
    auto strategy_bounceup = std::make_shared<BounceUpStrategy>(app_, account_info_, trend_distinguish_);
    strategy_bounceup->Initiate();
    strategys_.insert(std::make_pair(strategy_bounceup->id(), strategy_bounceup));
#endif
#if 0
    auto strategy_bouncedown = std::make_shared<BounceDownStrategy>(app_, account_info_, trend_distinguish_);
    strategy_bouncedown->Initiate();
    strategys_.push_back(strategy_bouncedown);
#endif
#if 0
    auto breakup_strategy = std::make_shared<BreakUpStrategy>(app_, account_info_, trend_distinguish_);
    breakup_strategy->Initiate();
    strategys_.push_back(breakup_strategy);
#endif
    return true;
}

void StrategyMan::Handle(const T_QuoteData &quote)
{  
    line_strategy_man_->Handle(quote);

    trend_distinguish_->Handle(quote);

    auto enabled_strategys = GetEnabledStrategys();
    for( unsigned i = 0; i < enabled_strategys.size(); ++i )
    {
        enabled_strategys[i]->Handle(quote);
    }

    if( (quote.hhmmss % 10000) / 100 >= 28 && (quote.hhmmss % 10000) / 100 < 31 ) // minute near 30
    { 
        int reg_hhmm = (quote.hhmmss / 10000) * 100 + 30;
        app_.capital_curve().Append(CapitalData(account_info_.capital.total(), quote.date, reg_hhmm));
    }
}

void StrategyMan::AppendTradeRecord(OrderAction action, PositionAtom &positon_atom, const T_QuoteData &quote, int index)
{
    SoundFilled(action == OrderAction::OPEN);
    auto trade_item = std::make_shared<TradeRecordSimple>();
    trade_item->trade_id = positon_atom.trade_id;
    trade_item->action = action;
    trade_item->pos_type = positon_atom.is_long ? PositionType::POS_LONG : PositionType::POS_SHORT;
    trade_item->index = index;
    trade_item->date = quote.date;
    trade_item->hhmm = quote.hhmmss/100;
    trade_item->price = quote.price;
    trade_item->quantity = positon_atom.qty_all();

    std::lock_guard<std::mutex> locker(account_info_.trade_info_mutex);
    auto trade_iter = account_info_.trade_info.find(trade_item->index);
    if( trade_iter == account_info_.trade_info.end() )
        trade_iter = account_info_.trade_info.insert(std::make_pair(trade_item->index, std::vector<std::shared_ptr<TradeRecordSimple>>())).first;
    trade_iter->second.push_back(std::move(trade_item));
}

std::vector<std::shared_ptr<Strategy> > StrategyMan::GetEnabledStrategys()
{
    std::vector<std::shared_ptr<Strategy> > ret;
    for( auto iter= strategys_.begin(); iter != strategys_.end(); ++iter )
        ret.push_back(iter->second);
    return ret;
}

void StrategyMan::AppendTrendLineStrategy(std::shared_ptr<TrendLine> &trend_line)
{
    line_strategy_man_->AppendStrategy(trend_line);
}