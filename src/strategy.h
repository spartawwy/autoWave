#ifndef STRATEGY_SDFKJKKWERER_H_
#define STRATEGY_SDFKJKKWERER_H_

#include "sys_common.h"
#include "forcast_man.h"
#include "position_account.h"

enum class PriceNearType : unsigned char{ UNTOUCH = 0, FIT, BREAK, OTHER};

struct AccountInfo;
class FuturesForecastApp;
//class ForcastMan;
class Strategy
{
public:
    Strategy(FuturesForecastApp &app, AccountInfo &account_info);
    virtual ~Strategy();

    virtual void Initiate(){}
    virtual void Handle(const T_QuoteData &quote) = 0;

protected:

    //void JudgeOpenLong_bounce_down(const T_QuoteData &quote, unsigned int short_pos_qty, INOUT unsigned int &long_pos_qty);
    void JudgeTrend(const T_QuoteData &quote);

    unsigned int NetPosition();
    void ClosePositionAtom(const T_QuoteData &quote, int trade_id, PositionAtom *pos_atom, unsigned int short_pos_qty, unsigned int &long_pos_qty);
     
    virtual bool ProdIfNearTradingEndTime(const T_QuoteData &quote);
    virtual void Strategy_Log(const std::string &content, const T_QuoteData *quote=nullptr);

protected:

    FuturesForecastApp &app_;
    AccountInfo &account_info_;
    ForcastMan *main_forcast_;
    ForcastMan *sub_forcast_;

    T_HisDataItemContainer *main_hisdatas_;
    T_HisDataItemContainer *sub_hisdatas_;
    std::string code_;
    TypePeriod main_k_type_;
    TypePeriod sub_k_type_;
     
    double pre_price_;

    Data2pForcastInnerContainer *main_bounce_ups_ ;
    Data2pForcastInnerContainer *main_bounce_downs_;
    ;
    Data2pForcastInnerContainer *sub_bounce_ups_ ;
    Data2pForcastInnerContainer *sub_bounce_downs_;
    ;
    Data3pForcastInnerContainer *main_trends_ups_ ;
    Data3pForcastInnerContainer *main_trends_downs_;
    ;
    Data3pForcastInnerContainer *sub_trends_ups_ ;
    Data3pForcastInnerContainer *sub_trends_downs_;

};
 
//static const double cst_ori_capital = 200000.0;
bool IsPriceNear(double price, double tag_price, const PositionAtom &pos_atom, TypePeriod type_period);
bool IsPriceRoseNear(double price, double tag_price);
bool IsPriceFallNear(double price, double tag_price);
std::string GetStamp(const T_QuoteData &quote);
bool compare_sort_desc(const RelForcastInfo &lh, const RelForcastInfo& rh);
bool compare_sort_asc(const RelForcastInfo &lh, const RelForcastInfo& rh);
bool IsNightTradingBegTime(const T_QuoteData &quote);
bool IsNearDayTradeTimeEnd(const T_QuoteData &quote);
bool IsNearNightTradeTimeEnd(const T_QuoteData &quote);

static const unsigned int cst_position_max = 3;
static const double cst_tolerance_min = 0.1;
static const double cst_tolerance_max = 0.3;
static const double cst_tolerance_equal = 0.0001;
static const double cst_default_stop_price_distance = 0.6;

#endif // STRATEGY_SDFKJKKWERER_H_