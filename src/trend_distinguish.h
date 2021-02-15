#ifndef TREND_DISTINGUISH_SDFDS_H_
#define TREND_DISTINGUISH_SDFDS_H_

#include "strategy.h"

enum class StrategyTrendType : unsigned char
{
    SSTRONG_LONG = 1,
    STRONG_LONG,
    LONG,
    LONG_MAY_DOWN, // close break quick line
    LONG_TO_DOWN, 
    LONG_TO_DOWNDOWN, // quick line inflect down
    SHORT_TO_UPUP, // quick line inflect up
    SHORT_TO_UP,
    SHORT_MAY_UP, // close stand up quick line
    SHORT,
    STRONG_SHORT,
    SSTRONG_SHORT,
    UNKOWN = 0,
};

enum class OpenPosDangerType : unsigned char
{
    SAFE   = 0,
    HALF_DANGER,
    DANGER,
}; 

std::string ToString(StrategyTrendType type);

class TrendDistinguish : public Strategy
{
public:
    TrendDistinguish(FuturesForecastApp &app, AccountInfo &account_info);
    ~TrendDistinguish(){}
    void Init();

    virtual void Handle(const T_QuoteData &quote) override;
    StrategyTrendType  sub_cur_trend() { return sub_cur_trend_; }
    StrategyTrendType  main_cur_trend() { return main_cur_trend_; }
    double sub_bias() { return sub_bias_; }
    double main_bias() { return main_bias_; }

    double sub_lines_bias() { return sub_lines_bias_; }
    double main_lines_bias() { return main_lines_bias_; }

    bool is_sub_bias_allow(bool open_long);
    bool is_main_bias_allow(bool open_long);

    bool is_breakup_strategy_allow();

    OpenPosDangerType danger_type_open_long(){ return danger_type_open_long_; }

private:

    StrategyTrendType  sub_cur_trend_;
    StrategyTrendType  main_cur_trend_;

    bool is_trend_judged_;
    int pre_judge_trend_stamp_;
    // bias is close price - ave_line_0
    double sub_bias_;
    double main_bias_;
    double main_lines_bias_;
    double sub_lines_bias_;
    
    //int date_openlong_danger_tag_;
    OpenPosDangerType  danger_type_open_long_;
};

#endif // TREND_DISTINGUISH_SDFDS_H_