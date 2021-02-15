#ifndef FLOAT_STOP_PROFIT_H_
#define FLOAT_STOP_PROFIT_H_
#include <cassert>
#include <string>

class PositionAtom;
class FloatStopProfit
{
public:
    FloatStopProfit(PositionAtom &postion_atom);
    virtual ~FloatStopProfit(){}
    FloatStopProfit(const FloatStopProfit &lh):enabled_(lh.enabled_), postion_atom_(lh.postion_atom_), max_profit_price_touch_(lh.max_profit_price_touch_){}
    
    virtual bool UpdateAndJudgeStop(double price) = 0;
    virtual std::string String(double price) = 0;

protected:
    bool enabled_;
    PositionAtom &postion_atom_;
    double max_profit_price_touch_; 
    //const double enable_threshold_price_;
    //const double max_back_rate_; 
    //const double max_target_;
};


class FollowStopProfit : public FloatStopProfit
{
public:
    FollowStopProfit(PositionAtom &postion_atom, double enable_threshold_price, double max_back_rate, double max_target);

    FollowStopProfit(const FollowStopProfit &lh):FloatStopProfit(lh.postion_atom_), enable_threshold_price_(lh.enable_threshold_price_)
        , max_back_rate_(lh.max_back_rate_), max_target_(lh.max_target_){}
    ~FollowStopProfit(){}

    // ------------------------------------------------------
    //return : is to stop profit or loss;
    // ------------------------------------------------------
    virtual bool UpdateAndJudgeStop(double price) override;

    virtual std::string String(double price) override;

private:
    FollowStopProfit & operator = (const FollowStopProfit &lh);

    //bool enabled_;
    //PositionAtom &postion_atom_;
    //double max_profit_price_touch_; 
    const double enable_threshold_price_;
    const double max_back_rate_; 
    const double max_target_;
};

struct StepProfitPara
{ 
    StepProfitPara():profit_target_0(2.0),back_rate_0(0.5)
        ,profit_target_1(4.0),back_rate_1(0.3)
        ,profit_target_2(8.0),back_rate_2(0.25)
        ,profit_target_3(14.0),back_rate_3(0.2){}
    
    double profit_target_0;
    double back_rate_0;
    double profit_target_1;
    double back_rate_1;
    double profit_target_2;
    double back_rate_2;
    double profit_target_3;
    double back_rate_3;
};
class StepStopProfit : public FloatStopProfit
{
public:
    StepStopProfit(PositionAtom &postion_atom, const StepProfitPara &para);
    ~StepStopProfit(){}

    // ------------------------------------------------------
    //return : is to stop profit or loss;
    // ------------------------------------------------------
    virtual bool UpdateAndJudgeStop(double price) override;
    virtual std::string String(double price) override;

private:
    StepProfitPara  para_;
};
#endif // FLOAT_STOP_PROFIT_H_