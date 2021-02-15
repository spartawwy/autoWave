#ifndef ZHIBIAO_SDF3SDFSD_H_
#define ZHIBIAO_SDF3SDFSD_H_

#include "stkfo_common.h"

static const unsigned int cst_ave_line_0_t = 20;
static const unsigned int cst_ave_line_1_t = 120;

class MomentumZhibiao : public ZhiBiaoAtom
{
public:
    static void Caculate(T_HisDataItemContainer &data_items_in_container);
    static void ReCaculateZhibiao(T_HisDataItemContainer &data_items_in_container, unsigned int item_index);

    MomentumZhibiao():val0_(0.0), val1_(0.0),val2_(0.0), val3_(0.0) {}
      
    virtual void val0( double val) override { val0_ = val;} 
    virtual double val0() override { return val0_;}

    virtual void val1( double val) override { val1_ = val;} 
    virtual double val1(){ return val1_;}

    virtual void val2( double val) override { val2_ = val;} 
    virtual double val2(){ return val2_;}

    virtual void val3( double val) override { val3_ = val;} 
    virtual double val3(){ return val3_;}

private:
    double val0_; // short 
    double val1_; // long
    double val2_; // DEA
    double val3_; // MACD
};

class IndependentZhibiao
{
public:
    IndependentZhibiao(){}
    virtual ~IndependentZhibiao(){}

    virtual void Caculate(T_HisDataItemContainer &data_items_in_container, unsigned int item_index) = 0;
    virtual void Caculate(T_HisDataItemContainer &data_items_in_container, unsigned int beg_index, unsigned int end_index) = 0;
    void SetVal(unsigned int index, double val);
    double GetVal(unsigned int index);
     
    //<index, val>
    std::unordered_map<unsigned int, double> datas_;  
};

class AverageLineZhibiao : public IndependentZhibiao
{
public:
    AverageLineZhibiao(unsigned int T);

    virtual void Caculate(T_HisDataItemContainer &data_items_in_container, unsigned int item_index) override;
    virtual void Caculate(T_HisDataItemContainer &data_items_in_container, unsigned int beg_index, unsigned int end_index) override;

private:
    unsigned int t_;
};

#endif