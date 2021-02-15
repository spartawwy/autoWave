#ifndef WAVE_HEADER_DSFKJDF_H_
#define WAVE_HEADER_DSFKJDF_H_

#include <memory>
#include <deque>
#include "stkfo_common.h"
#if 0 
class T_WavePoint
{
public:
    int index;
    int date;
    int hhmm;
    //FractalType frac_type;
    T_WavePoint():index(-1), date(0), hhmm(0)/*, frac_type(FractalType::UNKNOW_FRACTAL)*/{}
    T_WavePoint(const T_WavePoint & lh) : index(lh.index), date(lh.date), hhmm(lh.hhmm)/*, frac_type(lh.frac_type)*/{}
    T_WavePoint & operator = (const T_WavePoint &lh)
    { 
        if(this == &lh){return *this;} 
        index = lh.index; date = lh.date; hhmm = lh.hhmm;
        //frac_type = lh.frac_type;
        return *this;
    }
};
#else
typedef int T_WavePoint;
#endif
enum class WaveType : unsigned char { UP = 0, DOWN, UNKNOW=255};
enum class WaveTrendType : unsigned char { UP = 0, DOWN, AJUST_TO_DOWN, AJUST_TO_UP, UNKNOW=255};
//enum class WaveTrendIndex : unsigned char { WAVE_1 = 1, WAVE_2, WAVE_3, WAVE_4, WAVE_5, WAVE_6, WAVE_7, WAVE_8, WAVE_9, UNKNOW=255};
class Wave 
{
public:
    unsigned int level; // 2,1,0; 0 is atom wave 
    WaveType  type;
    T_WavePoint beg;
    T_WavePoint end; 
    std::deque<std::shared_ptr<Wave> >  sub_waves;
    WaveTrendType  trend;
    unsigned int trend_index; // >= 1
    unsigned int trend_sub_index; // >= 1 relate to trend_index
    Wave():level(2), type(WaveType::UNKNOW), beg(-1), end(-1), trend(WaveTrendType::UNKNOW), trend_index(-1), trend_sub_index(1)
    { 
    }
    Wave(unsigned int para_level, WaveType para_t, const T_WavePoint &para_beg, const T_WavePoint &para_end)
        : level(para_level), type(para_t), beg(para_beg), end(para_end)
        , trend(WaveTrendType::UNKNOW), trend_index(-1), trend_sub_index(1)
    {  
    }
    Wave(const Wave& wa) : level(wa.level), type(wa.type), beg(wa.beg), end(wa.end), trend(wa.trend), trend_index(wa.trend_index), trend_sub_index(wa.trend_sub_index)
    {
        sub_waves = wa.sub_waves;
    }
    Wave & operator = (const Wave& wa) 
    {
        if( this == &wa)
        { 
            return *this;
        } 
        level = wa.level;
        type = wa.type; 
        beg = wa.beg; 
        end = wa.end;
        sub_waves = wa.sub_waves;
        trend = wa.trend;
        trend_index = wa.trend_index;
        trend_sub_index = wa.trend_sub_index;
        return *this;
    }
    //double Hight();
private:
    
    
};
 
class FuturesForecastApp;
class WaveMan
{
public:
    WaveMan(FuturesForecastApp &app):app_(app)/*, k_datas_(k_datas)*/
    {
    }

    int Traverse_GetWaveLevel1(const std::string &code, TypePeriod type_period, int r_start_index, int backward_size);
    void TraverseSetTrendDataTowardRight(const std::string &code, TypePeriod type_period, unsigned int start_index);

    double MeasureHight(const std::string &code, TypePeriod type_period, const Wave &wave) const;
    void MakeSubWave(const std::string &code, TypePeriod type_period, INOUT Wave &wave, unsigned int sub_level);

    std::deque<std::shared_ptr<Wave> > &GetWaveContainer(const std::string &code, TypePeriod type_period)
    {
        auto iter = code_type_waves_.find(code);
        if( iter == code_type_waves_.end() )
            iter = code_type_waves_.insert(std::make_pair(code, std::unordered_map<TypePeriod, std::deque<std::shared_ptr<Wave> > >())).first;
        auto inner_iter = iter->second.find(type_period);
        if( inner_iter == iter->second.end() )
            inner_iter = iter->second.insert(std::make_pair(type_period, std::deque<std::shared_ptr<Wave> >())).first;
        return inner_iter->second;
    }

    
private:
    FuturesForecastApp &app_;
    //StockDataMan *stock_data;
    //T_HisDataItemContainer &k_datas_;

    std::unordered_map<std::string, std::unordered_map<TypePeriod, std::deque<std::shared_ptr<Wave> > > > code_type_waves_;
};
//bool Traverse_GetWaves(IN T_StructLineContainer &container, IN T_HisDataItemContainer & kline_data_items, int r_start_index, int backward_size, OUT Wave &wave);

#endif // WAVE_HEADER_DSFKJDF_H_