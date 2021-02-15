#ifndef TREND_LINE_H_DSFDSKJK_
#define TREND_LINE_H_DSFDSKJK_

#include <deque>
#include "stock_data_man.h"

enum class TrendLineType{ UP, DOWN};
class  TrendLine
{
public:
    TrendLine(TrendLineType type) : type_(type), beg_(-1), end_(-1), k_(0.0)
    {
    }
    TrendLine(TrendLineType type, int beg, int end, double k) : type_(type), beg_(beg), end_(end), k_(k)
    {
    }
    TrendLine(const TrendLine& lh) : type_(lh.type_), beg_(lh.beg_), end_(lh.end_), k_(lh.k_)
    {
    }
    TrendLine & operator = (const TrendLine& lh)
    {
        if( this == &lh )
            return *this;
        type_ = lh.type_;
        beg_ = lh.beg_;
        end_ = lh.end_;
        k_ = lh.k_;
        return *this;
    }
    int beg_;
    int end_;
    double k_;
    TrendLineType  type_;
};

class FuturesForecastApp;
class TrendLineMan
{
public:
    TrendLineMan(FuturesForecastApp &app) 
        : app_(app)/*, down_line_(TrendLine(TrendLineType::DOWN)), up_line_(TrendLine(TrendLineType::UP))*/
    {
    }

    void  CreateTrendLine(const std::string &code, TypePeriod type_period);

    TrendLine * FindTrendLine(const std::string &code, TypePeriod type_period, TrendLineType trend_line_type);
    TrendLine & GetTrendLine(const std::string &code, TypePeriod type_period, TrendLineType trend_line_type);

private:
    FuturesForecastApp &app_;
    //TrendLine down_line_;
    //TrendLine up_line_;

    //<code, ...>
    std::unordered_map<std::string, std::unordered_map<TypePeriod, std::unordered_map<TrendLineType, std::shared_ptr<TrendLine> > > > code_type_trend_lines_;

};

#endif // TREND_LINE_H_DSFDSKJK_