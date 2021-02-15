#ifndef  FORCAST_MAN_SDF23SDFS_H
#define  FORCAST_MAN_SDF23SDFS_H

#include <cassert>
#include <string>
#include <unordered_map>
#include <tuple>

#include "stkfo_common.h"


#define TAG_FORCAST_LOG "Forcast"
#define TAG_FORCAST_BOUNCE_UP_LOG "ForcastBounceUp"
#define TAG_FORCAST_BOUNCE_DOWN_LOG "ForcastBounceDown"
#define TAG_FORCAST_TREND_DOWN_LOG  "ForcastTrendDown"
#define TAG_FORCAST_TREND_UP_LOG  "ForcastTrendUp"

class FuturesForecastApp;
class T_DataForcast
{
public:
    T_DataForcast(unsigned int id_para, bool down) : id(id_para),stock_code(), date_a(0), hhmm_a(0), index_a(-1), date_b(0), hhmm_b(0), index_b(-1), point_a(0.0,0.0), point_b(0.0,0.0), is_down(down),price_spread_type(PriceSpreadType::UNKNOW)
    , price_a(0.0), price_b(0.0), breaked_a(false), breaked_b(false){}
    
    T_DataForcast(const T_DataForcast & lh) : id(lh.id), stock_code(lh.stock_code)
        ,date_a(lh.date_a), hhmm_a(lh.hhmm_a), index_a(lh.index_a), date_b(lh.date_b), hhmm_b(lh.hhmm_b), index_b(lh.index_b), point_a(lh.point_a), point_b(lh.point_b), is_down(lh.is_down), price_spread_type(lh.price_spread_type) 
        , price_a(lh.price_a), price_b(lh.price_b), breaked_a(lh.breaked_a), breaked_b(lh.breaked_b){}
    T_DataForcast& operator =(const T_DataForcast & lh) 
    {
        if( this == &lh )
            return *this;
       SetValue(lh);
       return *this;
    }

    T_DataForcast(T_DataForcast && lh) : id(lh.id), stock_code(std::move(lh.stock_code))
        ,date_a(lh.date_a), hhmm_a(lh.hhmm_a), index_a(lh.index_a), date_b(lh.date_b), hhmm_b(lh.hhmm_b), index_b(lh.index_b), point_a(lh.point_a), point_b(lh.point_b), is_down(lh.is_down), price_spread_type(lh.price_spread_type)
       , price_a(lh.price_a), price_b(lh.price_b),breaked_a(lh.breaked_a),breaked_b(lh.breaked_b) {}
     
    virtual void Clear(){ __Clear(); }
    virtual std::string String(){ return ""; }
    virtual ~T_DataForcast(){}

    unsigned int id;
    std::string  stock_code;
    int date_a;
    int hhmm_a;
    int index_a;
    int date_b;
    int hhmm_b;
    int index_b;

    QPointF point_a;
    QPointF point_b;

    bool is_down;// it means if ab down

    PriceSpreadType price_spread_type;
    double price_a;
    double price_b;
    bool breaked_a;
    bool breaked_b;

protected:
    void SetValue(const T_DataForcast & lh)
    {
        id = lh.id; stock_code = lh.stock_code;
        date_a = lh.date_a; hhmm_a=lh.hhmm_a; index_a=lh.index_a; 
        date_b=lh.date_b; hhmm_b=lh.hhmm_b; index_b=lh.index_b;
        point_a=lh.point_a; point_b=lh.point_b; is_down=lh.is_down; price_spread_type=lh.price_spread_type; 
        price_a=lh.price_a; price_b=lh.price_b; breaked_a = lh.breaked_a; breaked_b = lh.breaked_b;
    }
    T_DataForcast(){}
    void __Clear()
    { 
        id = -1; stock_code.clear(); date_a = hhmm_a = date_b = hhmm_b = 0; is_down = false; 
        index_a = index_b = -1;
        point_a = QPointF(0.0,0.0); point_b = QPointF(0.0,0.0);
        price_spread_type=PriceSpreadType::UNKNOW; 
        price_a = 0.0;
        price_b = 0.0;
        breaked_a = false;
        breaked_b = false;
    }
};

class T_Data2pForcast : public T_DataForcast
{
public:
    // down: is AB down
    T_Data2pForcast(unsigned int id_para, bool down=false) : T_DataForcast(id_para, down), c1(0.0), c2(0.0), c3(0.0)
    , opened_short_c1(false), opened_short_c2(false), opened_short_c3(false)
    , opened_long_c1(false), opened_long_c2(false), opened_long_c3(0){}

    T_Data2pForcast(const T_Data2pForcast & lh) : T_DataForcast(lh), c1(lh.c1), c2(lh.c2), c3(lh.c3)
        ,opened_short_c1(lh.opened_short_c1),opened_short_c2(lh.opened_short_c2),opened_short_c3(lh.opened_short_c3)
        ,opened_long_c1(lh.opened_long_c1),opened_long_c2(lh.opened_long_c2),opened_long_c3(lh.opened_long_c3){}

    T_Data2pForcast & operator =(const T_Data2pForcast & lh)
    { 
        if( this == &lh )
            return *this;
        SetValue(lh);
        c1 = lh.c1; c2 = lh.c2; c3 = lh.c3;
        opened_short_c1 = lh.opened_short_c1;opened_short_c2 = lh.opened_short_c2;opened_short_c3 = lh.opened_short_c3;
        opened_long_c1 = lh.opened_long_c1; opened_long_c2 = lh.opened_long_c2; opened_long_c3 = lh.opened_long_c3; 
        return *this;
    }
    
    T_Data2pForcast(T_Data2pForcast && lh) : T_DataForcast(std::move(lh)), c1(lh.c1), c2(lh.c2), c3(lh.c3)
        ,opened_short_c1(lh.opened_short_c1),opened_short_c2(lh.opened_short_c2),opened_short_c3(lh.opened_short_c3)
        ,opened_long_c1(lh.opened_long_c1),opened_long_c2(lh.opened_long_c2),opened_long_c3(lh.opened_long_c3) 
    {}

    virtual std::string String() override;

    virtual void Clear() override 
    { 
        __Clear(); c1 = c2 = c3 = 0.0; 
        opened_short_c1 = false;opened_short_c2 = false;opened_short_c3 = false;
        opened_long_c1 = false; opened_long_c2 = false; opened_long_c3 = 0;
    } 
public:
    double  c1;
    double  c2;
    double  c3; 

    bool opened_short_c1;
    bool opened_short_c2;
    bool opened_short_c3;
    bool opened_long_c1;
    bool opened_long_c2;
    unsigned int opened_long_c3;
};


class T_Data3pForcast : public T_DataForcast
{
public:
    explicit T_Data3pForcast(unsigned int id_para, bool down=false) : T_DataForcast(id_para, down), date_c(0), hhmm_c(0), index_c(-1), index_break(-1)
        , d1(0.0), d2(0.0), d3(0.0), point_c(0.0,0.0) 
        , opened_short_d1(false), opened_short_d2(false), opened_short_d3(false)
        , opened_long_d1(false), opened_long_d2(false), opened_long_d3(false)
        ,opened_short_d2p5(false), opened_long_d2p5(false), break_d3_n(0), break_d3_index(-1), bounce_over_d3(false)
    {}

    T_Data3pForcast(const T_Data3pForcast & lh) : T_DataForcast(lh), date_c(lh.date_c), hhmm_c(lh.hhmm_c), index_c(lh.index_c), index_break(lh.index_break)
        , d1(lh.d1), d2(lh.d2), d3(lh.d3), point_c(lh.point_c)
        ,opened_short_d1(lh.opened_short_d1),opened_short_d2(lh.opened_short_d2),opened_short_d3(lh.opened_short_d3)
        ,opened_long_d1(lh.opened_long_d1),opened_long_d2(lh.opened_long_d2),opened_long_d3(lh.opened_long_d3)
        ,opened_short_d2p5(lh.opened_short_d2p5), opened_long_d2p5(lh.opened_long_d2p5), break_d3_n(lh.break_d3_n), break_d3_index(lh.break_d3_index)
        , bounce_over_d3(lh.bounce_over_d3)
        ,parents(lh.parents), childrens(lh.childrens)
    {}
    T_Data3pForcast & operator =(const T_Data3pForcast & lh)
    { 
        if( this == &lh )
            return *this;
        SetValue(lh);
        date_c = lh.date_c;  hhmm_c = lh.hhmm_c; 
        index_c = lh.index_c; index_break = lh.index_break;
        d1 = lh.d1; d2 = lh.d2; d3 = lh.d3; 
        point_c = lh.point_c;
        opened_short_d1 = lh.opened_short_d1;opened_short_d2 = lh.opened_short_d2;opened_short_d3 = lh.opened_short_d3;
        opened_long_d1 = lh.opened_long_d1; opened_long_d2 = lh.opened_long_d2; opened_long_d3 = lh.opened_long_d3;
        opened_short_d2p5 = lh.opened_short_d2p5; opened_long_d2p5 = lh.opened_long_d2p5;
        break_d3_n = lh.break_d3_n; break_d3_index = lh.break_d3_index; bounce_over_d3 = lh.bounce_over_d3;
        parents = lh.parents; childrens = lh.childrens;
        return *this;
    }
    T_Data3pForcast(T_Data3pForcast && lh) : T_DataForcast(std::move(lh)), date_c(lh.date_c), hhmm_c(lh.hhmm_c), index_c(lh.index_c), index_break(lh.index_break)
        , d1(lh.d1), d2(lh.d2), d3(lh.d3), point_c(lh.point_c)
        ,opened_short_d1(lh.opened_short_d1),opened_short_d2(lh.opened_short_d2),opened_short_d3(lh.opened_short_d3)
        ,opened_long_d1(lh.opened_long_d1),opened_long_d2(lh.opened_long_d2),opened_long_d3(lh.opened_long_d3)
        ,opened_short_d2p5(lh.opened_short_d2p5), opened_long_d2p5(lh.opened_long_d2p5), break_d3_n(lh.break_d3_n),break_d3_index(lh.break_d3_index), bounce_over_d3(lh.bounce_over_d3)
        ,parents(std::move(lh.parents)), childrens(std::move(lh.childrens))
    {} 

    virtual std::string String() override;

    virtual void Clear() override 
    { 
        __Clear(); 
        index_c = -1;
        index_break = -1;
        date_c = hhmm_c = 0; d1 = d2 = d3 = 0.0; 
        point_c = QPointF(0.0,0.0);
        opened_short_d1 = false;opened_short_d2 = false;opened_short_d3 = false;
        opened_long_d1 = false; opened_long_d2 = false; opened_long_d3 = false; 
        opened_short_d2p5 = false;
        opened_long_d2p5 = false;
        break_d3_n = 0; bounce_over_d3 = false; break_d3_index = -1;
        parents.clear(); childrens.clear(); 
    } 

    int date_c;
    int hhmm_c;
    int index_c;
    int index_break;
    double  d1;
    double  d2;
    double  d3; 
     
    QPointF point_c;
    bool opened_short_d1;
    bool opened_short_d2;
    bool opened_short_d2p5;
    bool opened_short_d3;
    bool opened_long_d1;
    bool opened_long_d2;
    bool opened_long_d2p5;
    bool opened_long_d3;
    unsigned int break_d3_n;
    int break_d3_index;
    bool bounce_over_d3;

    //<id, T_Data3pForcast>
    std::unordered_map<unsigned int, std::shared_ptr<T_Data3pForcast> > parents;
    std::unordered_map<unsigned int, std::shared_ptr<T_Data3pForcast> > childrens;
};
 
// (code, T_Data2pUpForcast)
//typedef std::unordered_map<std::string, std::vector<T_Data2pForcast> > Code2pForcastType;

#if 0
struct ForcastContainerInfo
{
    ForcastContainerInfo():min_a(0.0), max_a(0.0),min_b(0.0),max_b(0.0){}
    double min_a;
    double max_a;
    double min_b;
    double max_b;
};
struct Code2pForcastData
{
    Data2pForcastInnerContainer container;
    ForcastContainerInfo  info;
};
#endif
typedef std::list<T_Data2pForcast>  Data2pForcastInnerContainer;

typedef std::unordered_map<std::string, Data2pForcastInnerContainer > Code2pForcastType;

//typedef std::unordered_map<std::string, std::vector<T_Data3pForcast> > Code3pForcastType;
typedef std::list<std::shared_ptr<T_Data3pForcast>>  Data3pForcastInnerContainer;

typedef std::unordered_map<std::string, Data3pForcastInnerContainer > Code3pForcastType;

class ForcastMan
{
public:
    ForcastMan(FuturesForecastApp *app, int wall_index);
    ~ForcastMan(){}

    void Append(TypePeriod type_period, const std::string &code,  bool is_down_forward, T_Data2pForcast& ); 
    Data2pForcastInnerContainer * Find2pForcastVector(const std::string &code, TypePeriod type_period, bool is_down_forward);
    Data2pForcastInnerContainer & Get2pForcastVector(const std::string &code, TypePeriod type_period, bool is_down_forward);
     
    T_Data2pForcast * Find2pForcast(const std::string &code, TypePeriod type_period, bool is_down_forward, T_KlineDataItem &item_a, T_KlineDataItem &item_b);
    //Data2pForcastInnerContainer::iterator * Find2pForcastIter(const std::string &code, TypePeriod type_period, bool is_down_forward, unsigned int id);
    T_Data2pForcast * Find2pForcast(const std::string &code, TypePeriod type_period, bool is_down_forward, unsigned int id);
    void Erase2pForcast(const std::string &code, TypePeriod type_period, bool is_down_forward, unsigned int id);

    void Append(TypePeriod type_period, const std::string &code, bool is_down_forward, T_Data3pForcast& );
    Data3pForcastInnerContainer * Find3pForcastVector(const std::string &code, TypePeriod type_period, bool is_down_forward);
    Data3pForcastInnerContainer & Get3pForcastVector(const std::string &code, TypePeriod type_period, bool is_down_forward);
    T_Data3pForcast * Find3pForcast(const std::string &code, TypePeriod type_period, bool is_down_forward, T_KlineDataItem &item_a, T_KlineDataItem &item_b);
    T_Data3pForcast * Find3pForcast(const std::string &code, TypePeriod type_period, bool is_down_forward, unsigned int id);
    void Erase3pForcast(const std::string &code, TypePeriod type_period, bool is_down_forward, unsigned int id);
    void Remove3pForcastItem(const std::string &code, TypePeriod type_period, bool is_down_forward, T_KlineDataItem &item_a, T_KlineDataItem &item_b);
    
    void RemoveForcastItems(const std::string &code, TypePeriod type_period);
    // is_down_forward is 2p down
    void Remove2pForcastItems(const std::string &code, TypePeriod type_period, bool is_down_forward);
    // is_down_forward is 2p down
    void Remove3pForcastItems(const std::string &code, TypePeriod type_period, bool is_down_forward);

    double FindMaxForcastPrice(const std::string &code, TypePeriod type_period, int start_date, int start_hhmm, int end_date, int end_hhmm);
    double FindMinForcastPrice(const std::string &code, TypePeriod type_period, int start_date, int start_hhmm, int end_date, int end_hhmm);

    void ResetIndexs(const std::string &code, TypePeriod type_period, T_HisDataItemContainer &container);

private:
     
    template<typename T1,typename T2>
    struct is__same
    {
        operator bool(){return false;}
    };
    template<typename T1>
    struct is__same<T1,T1>
    {
        operator bool(){return true;}
    }; 


    template <typename DataForcastType>
    void _Append2pForcast(TypePeriod type_period, const std::string &code, DataForcastType &forcast_data)
    {
        Code2pDownForcastType *down_holder = nullptr;
        Code2pUpForcastType   *up_holder = nullptr;
        bool is_2pdown = false;
        if( is__same<DataForcastType, T_Data2pDownForcast>() )
            is_2pdown = true;

        if( is_2pdown )
            down_holder = &Get2pDownDataHolder(type_period);
        else
            up_holder = &Get2pUpDataHolder(type_period);
         
        if( is_2pdown )
        {
            auto vector_iter = down_holder->find(code);
            if( vector_iter == down_holder->end() )
                vector_iter = down_holder->insert(std::make_pair(code, std::vector<T_Data2pDownForcast>())).first;
            vector_iter->second.push_back( *(T_Data2pDownForcast*)(&forcast_data) );
        }else
        {
            auto vector_iter = up_holder->find(code);
            if( vector_iter == up_holder->end() )
                vector_iter = up_holder->insert(std::make_pair(code, std::vector<T_Data2pUpForcast>())).first;
            vector_iter->second.push_back( *(T_Data2pUpForcast*)(&forcast_data) );
        }
        
    }

    Code3pForcastType & Get3pDataHolder(TypePeriod type_period, bool is_down);
    Code2pForcastType & Get2pDataHolder(TypePeriod type_period, bool is_down_forward);
    Code2pForcastType & Get2pUpDataHolder(TypePeriod type_period);
    Code2pForcastType & Get2pDownDataHolder(TypePeriod type_period);

    Code2pForcastType  stock_2pdown_forcast_1m_; // 1 minute
    Code2pForcastType  stock_2pdown_forcast_5m_; // 5 minute
    Code2pForcastType  stock_2pdown_forcast_15m_; // 15 minute
    Code2pForcastType  stock_2pdown_forcast_30m_; // 30 minute
    Code2pForcastType  stock_2pdown_forcast_h_;
    Code2pForcastType  stock_2pdown_forcast_d_;
    Code2pForcastType  stock_2pdown_forcast_w_; // week
    Code2pForcastType  stock_2pdown_forcast_mon_; // month
    Code2pForcastType  df_no_use_;

    Code2pForcastType  stock_2pup_forcast_1m_; // 5 minute 
    Code2pForcastType  stock_2pup_forcast_5m_; // 5 minute 
    Code2pForcastType  stock_2pup_forcast_15m_; // 15 minute 
    Code2pForcastType  stock_2pup_forcast_30m_; // 30 minute 
    Code2pForcastType  stock_2pup_forcast_h_;  
    Code2pForcastType  stock_2pup_forcast_d_;  
    Code2pForcastType  stock_2pup_forcast_w_;  
    Code2pForcastType  stock_2pup_forcast_mon_;  
    Code2pForcastType  uf_no_use_;  

    Code3pForcastType    stock_3pdown_forcast_1m_;
    Code3pForcastType    stock_3pdown_forcast_5m_;
    Code3pForcastType    stock_3pdown_forcast_15m_;
    Code3pForcastType    stock_3pdown_forcast_30m_;
    Code3pForcastType    stock_3pdown_forcast_h_;
    Code3pForcastType    stock_3pdown_forcast_d_;
    Code3pForcastType    stock_3pdown_forcast_w_;
    Code3pForcastType    stock_3pdown_forcast_mon_;

    Code3pForcastType    stock_3pup_forcast_1m_;
    Code3pForcastType    stock_3pup_forcast_5m_;
    Code3pForcastType    stock_3pup_forcast_15m_;
    Code3pForcastType    stock_3pup_forcast_30m_;
    Code3pForcastType    stock_3pup_forcast_h_;
    Code3pForcastType    stock_3pup_forcast_d_;
    Code3pForcastType    stock_3pup_forcast_w_;
    Code3pForcastType    stock_3pup_forcast_mon_;
    Code3pForcastType    no_use_3p_;

    FuturesForecastApp   *app_;
    int wall_index_; 

};

// return: c1, c2, c3 ; ps: make sure a > b > 0;
std::tuple<double, double, double>  ForcastC_ABDown(double a, double b);

// return: c1, c2, c3 ; ps: make sure 0 < a < b;
std::tuple<double, double, double>  ForcastC_ABUp(double a, double b);

// return: d1, d2, d3 ;ps: make sure a > b > 0
std::tuple<double, double, double>  ForcastD_ABC_Down(double A, double B);

// return: d1, d2, d3 ; ps: make sure 0 < a < b
std::tuple<double, double, double>  ForcastD_ABC_Up(double A, double B);

struct AbcIndexType
{
    int id; // forecast id
    int index_a;
    int index_b;
    int index_c;
    int index_break;
    int date_a;
    int hhmm_a;
    int date_b;
    int hhmm_b;
    AbcIndexType():id(-1),index_a(-1),index_b(-1),index_c(-1),index_break(-1),date_a(0),hhmm_a(0),date_b(0),hhmm_b(0){}
    AbcIndexType(const AbcIndexType &lh):id(lh.id),index_a(lh.index_a),index_b(lh.index_b),index_c(lh.index_c),index_break(lh.index_break)
        ,date_a(lh.date_a),hhmm_a(lh.hhmm_a),date_b(lh.date_b),hhmm_b(lh.hhmm_b){}
    AbcIndexType & operator =(const AbcIndexType &lh)
    {
        if( this == &lh) return *this;
        id = lh.id;
        index_a = lh.index_a;index_b = lh.index_b;index_c = lh.index_c;index_break = lh.index_break;
        date_a = lh.date_a; hhmm_a = lh.hhmm_a;   
        date_b = lh.date_b; hhmm_b = lh.hhmm_b;
        return *this;
    }
};
#endif // FORCAST_MAN_SDF23SDFS_H