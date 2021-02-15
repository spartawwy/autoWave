#ifndef POSITION_ACCOUNT_DSFSDF_H_
#define POSITION_ACCOUNT_DSFSDF_H_

#include <cassert>
#include <vector>
#include <tuple>
#include <atomic>
#include <mutex>
#include <unordered_map>
#include <QString>

#include "stkfo_common.h"
#include "float_stop_profit.h"

#define POSITION_STATUS_FROZEN  0
#define POSITION_STATUS_AVAILABLE  1
#define POSITION_STATUS_ALL  2

static const int cst_position_target_status_available = 1;
static const double cst_per_tick = 0.1; // pow(0.1, DEFAULT_DECIMAL)
static const double cst_per_tick_capital = 100.00;
static const double cst_margin_capital = 40000.00;
static const double cst_default_ori_capital = 200000.00;
//static const double cst_default_fee_rate_percent = 0.025;

static const double cst_per_hand_open_fee = 25.0;
static const double cst_per_hand_close_fee = 0.0;

struct AccountAtom
{ 
    double  avaliable;
    double  frozen;
    double  float_profit;
    AccountAtom() : avaliable(0.0), frozen(0.0), float_profit(0.0){}
    double total() { return avaliable + frozen; }
};

class TradeRecordAtom
{ 
public:
    int trade_id;
    int date;
    int hhmm;
    OrderAction action;
    PositionType pos_type;
    int quantity;
    double price;
    double profit;
    double fee;
    double price_stop_profit;
    double price_stop_loss;
    explicit TradeRecordAtom() : trade_id(-1), date(0), hhmm(0), action(OrderAction::OPEN), pos_type(PositionType::POS_LONG), quantity(0), price(0.0), profit(0.0), fee(0.0)
        , price_stop_profit(MAGIC_STOP_PRICE), price_stop_loss(MAGIC_STOP_PRICE) { }
    TradeRecordAtom(const TradeRecordAtom &lh) : trade_id(lh.trade_id), date(lh.date), hhmm(lh.hhmm), action(lh.action), pos_type(lh.pos_type), quantity(lh.quantity), price(lh.price), profit(lh.profit), fee(lh.fee)
        , price_stop_profit(lh.price_stop_profit), price_stop_loss(lh.price_stop_loss) { }
    TradeRecordAtom & operator = (const TradeRecordAtom &lh)
    {
        if( &lh == this ) 
            return *this;
        trade_id = lh.trade_id;
        date = lh.date; hhmm = lh.hhmm;
        action = lh.action; pos_type = lh.pos_type;
        quantity = lh.quantity; price = lh.price; profit = lh.profit; fee = lh.fee;
        price_stop_profit = lh.price_stop_profit; price_stop_loss = lh.price_stop_loss;
        return *this;
    }

    QString ToQStr();
};


class TradeRecordSimple
{ 
public:
    int trade_id;
    int date;
    int hhmm;
    int index;
    OrderAction action;
    PositionType pos_type;
    int quantity;
    double price; 
    explicit TradeRecordSimple() : trade_id(-1), date(0), hhmm(0), index(-1), action(OrderAction::OPEN), pos_type(PositionType::POS_LONG), quantity(0), price(0.0) 
         { }
    TradeRecordSimple(const TradeRecordSimple &lh) : trade_id(lh.trade_id), date(lh.date),index(lh.index), hhmm(lh.hhmm), action(lh.action), pos_type(lh.pos_type), quantity(lh.quantity), price(lh.price) 
         { }
    TradeRecordSimple & operator = (const TradeRecordSimple &lh)
    {
        if( &lh == this ) 
            return *this;
        trade_id = lh.trade_id;
        date = lh.date; hhmm = lh.hhmm;
        index = lh.index;
        action = lh.action; pos_type = lh.pos_type;
        quantity = lh.quantity; price = lh.price;  
        return *this;
    }

    //QString ToQStr();
};

struct RelForcastInfo
{
    ForcastType  forcast_type;
    unsigned int forcast_id;
    ForcastSiteType site_type;
    double ab_distance;
    RelForcastInfo(ForcastType  forcast_type_p, unsigned int forcast_id_p, ForcastSiteType site_type_p, double ab_distance_p)
    {
        forcast_type = forcast_type_p; forcast_id = forcast_id_p; site_type = site_type_p; ab_distance = ab_distance_p;
    }
    RelForcastInfo():forcast_type(ForcastType::UNKOWN), forcast_id(0), site_type(ForcastSiteType::C1), ab_distance(0.0){}
    RelForcastInfo(const RelForcastInfo&lh):forcast_type(lh.forcast_type), forcast_id(lh.forcast_id), site_type(lh.site_type), ab_distance(lh.ab_distance){}
    RelForcastInfo& operator =(const RelForcastInfo&lh)
    { 
        if( this == &lh )
            return *this;
        forcast_type = lh.forcast_type; forcast_id = lh.forcast_id; site_type = lh.site_type;
        ab_distance = lh.ab_distance;
        return *this;
    }
};

struct PosAtomHelpInfo
{
    bool is_remain; // if true, not to close in trading end time(ps : if next day is not rest day)
    bool has_near_small_aim;
    bool has_near_mid_aim;
    bool has_near_big_aim;
    double small_aim;
    double mid_aim;
    double big_aim;
    
    PosAtomHelpInfo(): has_near_small_aim(false), has_near_mid_aim(false), has_near_big_aim(false), small_aim(99999.9), mid_aim(99999.9), big_aim(99999.9)
    , is_remain(false){}
    PosAtomHelpInfo(const PosAtomHelpInfo &lh): has_near_small_aim(lh.has_near_small_aim), has_near_mid_aim(lh.has_near_mid_aim), has_near_big_aim(lh.has_near_big_aim)
    , small_aim(lh.small_aim), mid_aim(lh.mid_aim), big_aim(lh.big_aim), is_remain(lh.is_remain){}
    PosAtomHelpInfo& operator = (const PosAtomHelpInfo& lh)
    { 
        if( this == &lh ) 
            return *this; 
        else 
        {
            has_near_small_aim = lh.has_near_small_aim; has_near_mid_aim = lh.has_near_mid_aim; has_near_big_aim = lh.has_near_big_aim; 
            small_aim = lh.small_aim; mid_aim = lh.mid_aim; big_aim = lh.big_aim; is_remain = lh.is_remain;
        }
        return *this;
    }
    std::string ToString();
     
};
 
class PositionAtom
{
public:
    PositionAtom() : trade_id(-1), price(0.0), is_long(false), stop_loss_price(MAGIC_STOP_PRICE), stop_profit_price(MAGIC_STOP_PRICE), qty_available(0), qty_frozens(), float_stop_profit(nullptr)
    {}
    PositionAtom(const PositionAtom& lh) : trade_id(lh.trade_id), price(lh.price), is_long(lh.is_long)
        , stop_loss_price(lh.stop_loss_price),stop_profit_price(lh.stop_profit_price), qty_available(lh.qty_available), qty_frozens(lh.qty_frozens)
    , help_info(lh.help_info), float_stop_profit(lh.float_stop_profit)
    {}
    PositionAtom& operator = (const PositionAtom& lh)
    { 
        if( this == &lh ) 
            return *this; 
        else 
        {
            trade_id = lh.trade_id;
            price = lh.price; 
            is_long = lh.is_long;
            stop_loss_price = lh.stop_loss_price;
            stop_profit_price = lh.stop_profit_price;
            qty_available = lh.qty_available;
            qty_frozens = lh.qty_frozens;
            //is_frozen = lh.is_frozen;
            help_info = lh.help_info;
            float_stop_profit = lh.float_stop_profit;
        }
        return *this; 
    }

    double FloatProfit(double cur_price);
    double PartProfit(double cur_price, unsigned int qty);

    unsigned int qty_all(){ return qty_available + qty_frozen(); }
    unsigned int qty_frozen();

    void ClearAvaliable(){ qty_available = 0; }
    void DecreaseAvaliable(unsigned int qty)
    {
        assert(qty_available >= qty);
        qty_available -= qty;
    }
    void Freeze(int fake_id, unsigned qty)
    { 
        assert(qty_available >= qty);
        qty_available -= qty;
        qty_frozens.insert(std::make_pair(fake_id, qty));
    }
    void UnFreeze(int fake_id)
    {
        auto iter = qty_frozens.find(fake_id);
        assert(iter != qty_frozens.end());
        if( iter != qty_frozens.end() )
        {
            qty_available += iter->second;
            qty_frozens.erase(iter);
        }
    }
    unsigned int DecreaseFrozen(int fake_id)
    {
        unsigned int size = 0; 
        auto iter = qty_frozens.find(fake_id);
        assert(iter != qty_frozens.end());
        if( iter != qty_frozens.end() )
        {
            size = iter->second; 
            qty_frozens.erase(iter);
        }
        return size;
    }
    std::string String();

    int trade_id;
    double price; // open price
    bool is_long;
    double stop_loss_price;   // if < 0.0 means not set
    double stop_profit_price; // if < 0.0 means not set
    unsigned int qty_available;
    //<order fake id, frozen quantity>
    std::unordered_map<int, unsigned int> qty_frozens;
     
    PosAtomHelpInfo help_info;
    RelForcastInfo  rel_forcast_info;
    std::shared_ptr<FloatStopProfit> float_stop_profit;
};
 
class PositionInfo
{  
public:
     
    //PositionInfo(double open_fee=100.0, double close_fee=300.0) : open_fee_(open_fee), close_fee_(close_fee) { max_trade_id_ = 0; }
    PositionInfo() : open_fee_(cst_per_hand_open_fee), close_fee_(cst_per_hand_close_fee) { max_trade_id_ = 0; }
     
    void open_fee(double fee) { open_fee_ = fee; }
    void close_fee(double fee) { close_fee_ = fee; }
    

    void Clear(){ long_positions_.clear(); short_positions_.clear(); position_holder_.clear(); max_trade_id_ = 0;}

    int GenerateTradeId(){ return ++max_trade_id_; }
    unsigned int TotalPosition() { return LongPosQty() + ShortPosQty(); }
     
    unsigned int LongPosQty(int target_status=POSITION_STATUS_ALL);
    // (id, special position size)
    std::unordered_map<int, unsigned int> LongPosSizeInfo(int target_status=POSITION_STATUS_ALL){ return PositionSizeInfo(PositionType::POS_LONG, target_status);}
    double LongAveragePrice();
     
    unsigned int ShortPosQty(int target_status=POSITION_STATUS_ALL);
    // (id, special position size)
    std::unordered_map<int, unsigned int> ShortPosSizeInfo(int target_status=POSITION_STATUS_ALL){ return PositionSizeInfo(PositionType::POS_SHORT, target_status);}
    double ShortAveragePirce();

    unsigned int PositionQty(PositionType type, int target_status=POSITION_STATUS_ALL);
    // (id, special status position size)
    std::unordered_map<int, unsigned int> PositionSizeInfo(PositionType type,  int target_status=POSITION_STATUS_ALL);

    double FloatProfit(double price);
    // ret <low, high>
    std::tuple<double, double> GetForceClosePrices(double capital);

    std::vector<TradeRecordAtom> DoIfStopProfitLongPos(const T_StockHisDataItem &k_item, double &capital_ret, std::vector<int> &ret_ids, double *p_cur_price, double *p_profit);
    std::vector<TradeRecordAtom> DoIfStopProfitShortPos(const T_StockHisDataItem &k_item, double &capital_ret, std::vector<int> &ret_ids, double *p_cur_price, double *p_profit);
    std::vector<TradeRecordAtom> DoIfStopLossLongPos(const T_StockHisDataItem &k_item, double &capital_ret, std::vector<int> &ret_ids, double *p_cur_price, double *p_profit);
    std::vector<TradeRecordAtom> DoIfStopLossShortPos(const T_StockHisDataItem &k_item, double &capital_ret, std::vector<int> &ret_ids, double *p_cur_price, double *p_profit);
#if 0
    std::vector<TradeRecordAtom> DoIfStopProfit(int date, int hhmm, double h_price, double l_price, double *p_profit);
    std::vector<TradeRecordAtom> DoIfStopLoss(int date, int hhmm, double h_price, double l_price, double *p_profit);
#endif
     
    // return trades
    std::vector<TradeRecordAtom> CloseAvaliableLong(double price, unsigned int qty, double &margin_ret, double *p_profit, std::vector<int> *p_ret_close_ids=nullptr)
    {
        return CloseAvaliable(true, price, qty, margin_ret, p_profit, p_ret_close_ids);
    }
    // return trades
    std::vector<TradeRecordAtom> CloseAvaliableShort(double price, unsigned int qty, double &margin_ret, double *p_profit, std::vector<int> *p_ret_close_ids=nullptr)
    {
        return CloseAvaliable(false, price, qty, margin_ret, p_profit, p_ret_close_ids);
    }
    // return trades
    std::vector<TradeRecordAtom> CloseAvaliable(bool target_long, double price, unsigned int qty, double &margin_ret, double *p_profit, std::vector<int> *p_ret_close_ids=nullptr);
    //void CloseAvaliable(PositionAtom &pos_atom, double cur_price, double &margin_ret, double *p_profit);

    void PushBack(bool is_long, std::shared_ptr<PositionAtom> &item);

    PositionAtom * PopBack(bool is_long);

    unsigned int PositionCountRemainedForRunProfit(bool target_long);
    //return <highest price, lowest price>
    std::tuple<double, double> GetHighestLowestPrice(bool is_long);
    PositionAtom * FindPositionAtom(int id);
    PositionAtom * FindPositionAtomByRelForcast(int forcast_id, ForcastSiteType site, bool is_long);
    std::shared_ptr<PositionAtom> FindPositionAtomSharedPointer(int id);
    TradeRecordAtom  ClosePositionAtomRetCapitalProfit(int id, double price, double *capital_ret, double *p_profit=nullptr);
    TradeRecordAtom  ClosePositionAtom(int id, double price, double *margin_ret, double *p_profit=nullptr);
    void RemoveAtom(int id);

    std::mutex mutex_;

private:
     
    PositionInfo(const PositionInfo&);
    PositionInfo& operator = (const PositionInfo&);

    double open_fee_;
    double close_fee_;
    std::unordered_map<int, std::shared_ptr<PositionAtom> > position_holder_;

    typedef std::vector<PositionAtom*> T_PositionAtoms;
    T_PositionAtoms  long_positions_;
    T_PositionAtoms  short_positions_;

    std::atomic_int max_trade_id_;
    
};

struct AccountInfo
{
    AccountAtom  capital; 
    PositionInfo  position;
    // <k_index, record>
    std::unordered_map <int, std::vector<std::shared_ptr<TradeRecordSimple>> > trade_info;
    std::mutex trade_info_mutex;
};

std::string ToStr(OrderAction action);
std::string ToStr(PositionType pos_tp);

double CalculateFee(int quantity, double price, bool is_close);
double CaculateOpenPositionMargin(double price, unsigned int quantity);
double CaculateOpenPositionFreezeCapital(double price, unsigned int quantity);
int CalculateMaxQtyAllowOpen(double capital, double price);

#endif // TRAIN_TRADE_DSFSDF_H_