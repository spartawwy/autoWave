#ifndef  STKFO_COMMON_SDF3DSF_H_
#define  STKFO_COMMON_SDF3DSF_H_

#include <string>
#include <memory>
#include <unordered_map>
#include <deque>
#include <vector>
#include <QString>

#include <QtCore/QPoint>
 
#include "stk_quoter_api.h"

#include "sys_common.h"

#pragma comment(lib, "sys_common.lib")
//#define USE_STK_QUOTER  // get k line info from stk quoter
//#define USE_WINNER_API  // get k line info from winnersystem 
#define USE_TDXHQ

//#define MAKE_SUB_WALL 

enum class FractalType : int
{
 UNKNOW_FRACTAL  = 0,
 BTM_AXIS_T_3    = 0x00000001,
 BTM_AXIS_T_5    = 0x00000002,
 BTM_AXIS_T_7    = 0x00000004,
 BTM_AXIS_T_9    = 0x00000008,
 BTM_AXIS_T_11   = 0x00000010,
 BTM_FAKE        = 0x00000020,

 TOP_AXIS_T_3    = 0x00010000,
 TOP_AXIS_T_5    = 0x00020000,
 TOP_AXIS_T_7    = 0x00040000,
 TOP_AXIS_T_9    = 0x00080000,
 TOP_AXIS_T_11   = 0x00100000,
 TOP_FAKE        = 0x40000000,
};
 
enum class FractalGeneralType : int
{
    TOP_FRACTAL  = 0,
    BTM_FRACTAL,
    UNKNOW
};
enum class PriceSpreadType : unsigned char
{
    MINI = 0, MICRO, SMALL, MID, BIG, SUPPER, UNKNOW,
};

enum class TagType : int
{
    UNKNOW_TAG      = 0,
    SELL            = 0x00000001,
    STRONG_SELL     = 0x00000002,
    BUY             = 0x00000004,
    STRONG_BUY      = 0x00000008,
};

enum class ForcastSiteType: unsigned char
{
    C1 = 0, C2, C3, D1, D2, D2p5, D3
};

enum class ForcastType: unsigned char
{
    BOUNCE_UP = 0,
    BOUNCE_DOWN,
    TREND_UP,
    TREND_DOWN,
    UNKOWN,
};

#define UPWARD_FRACTAL   0x10000000
#define DOWNWARD_FRACTAL 0x20000000
#define INSUFFIC_FRACTAL 0x40000000  
 
#define CST_MAGIC_POINT QPointF(-1, -1)
#define MAX_PRICE 100000000.0f
#define MIN_PRICE EPSINON
#define MAGIC_STOP_PRICE (-1.0f)

#define MARKET_TYPE_SH  1
#define MARKET_TYPE_SZ  0


enum PositionType : unsigned char
{
    POS_LONG  = 0, // ��ͷ
    POS_SHORT = 1, // ��ͷ
};
enum OrderAction : unsigned char
{
    OPEN = 0,
    CLOSE,
    //UNFREEZE, // only related to stock, this action is in the front of current day's other action
};
enum OrderType : unsigned char
{
    HANGON = 0,
    STOPPROFIT,
    STOPLOSS,
    CONDITION,
    //UNFREEZE, // only related to stock, this action is in the front of current day's other action
};
enum CompareType : unsigned char
{
    BIGEQUAL = 0,// >=
    SMALLEQUAL,  // <=
    EQUAL,       // ==
    BIG,         // >
    SMALL        // <
};
 
//there is 3 order types : 1. hang on orders 2. auto stop profit/loss orders 3.condition orders
struct OrderInfo
{
    OrderType  type;
    OrderAction  action;
    PositionType position_type; // target position type
    double price;
    unsigned int qty;
    int fake_id;
    int rel_position_id;    // when auto stop profit/loss type(action is close); -1 means no relate  
    //(position atom id, frozend size)
    std::unordered_map<int, unsigned int> help_contain;
    // condition order related  -----------
    CompareType compare_type;
    unsigned int profit_stop_ticks;
    unsigned int loss_stop_ticks;
    // ------------------------------------

    OrderInfo(OrderType para_type) : type(para_type), action(OrderAction::OPEN), position_type(PositionType::POS_LONG), price(MAGIC_STOP_PRICE), qty(0)
        , fake_id(-1), compare_type(CompareType::BIGEQUAL), rel_position_id(-1), profit_stop_ticks(0), loss_stop_ticks(0){}
    OrderInfo() : type(OrderType::HANGON), action(OrderAction::OPEN), position_type(PositionType::POS_LONG), price(MAGIC_STOP_PRICE), qty(0)
        , fake_id(-1), compare_type(CompareType::BIGEQUAL), rel_position_id(-1), profit_stop_ticks(0), loss_stop_ticks(0){}
    OrderInfo(const OrderInfo &lh) : type(lh.type), action(lh.action), position_type(lh.position_type), price(lh.price), qty(lh.qty)
        , fake_id(lh.fake_id), rel_position_id(lh.rel_position_id), compare_type(lh.compare_type), help_contain(lh.help_contain), profit_stop_ticks(lh.profit_stop_ticks), loss_stop_ticks(lh.loss_stop_ticks){}
};
 

class PyDataMan;
class T_KlinePosData
{
public:
    T_KlinePosData() : date(0), hhmm(0), x_left(0.0), x_right(0.0), height(0.0), columnar_top_left(CST_MAGIC_POINT), top(CST_MAGIC_POINT), bottom(CST_MAGIC_POINT) {}
    T_KlinePosData(const T_KlinePosData &lh)
        : date(lh.date), hhmm(lh.hhmm), x_left(lh.x_left), x_right(lh.x_right), height(lh.height), columnar_top_left(lh.columnar_top_left), top(lh.top), bottom(lh.bottom) {}
    T_KlinePosData(T_KlinePosData &&lh)
        : date(lh.date), hhmm(lh.hhmm), x_left(lh.x_left), x_right(lh.x_right), height(lh.height), columnar_top_left(lh.columnar_top_left), top(lh.top), bottom(lh.bottom) {}

    T_KlinePosData & operator = (const T_KlinePosData &lh)
    {
        if( this == &lh ) return *this;
        date = lh.date;
        hhmm = lh.hhmm;
        x_left = lh.x_left;
        x_right = lh.x_right;
        height = lh.height;
        columnar_top_left = lh.columnar_top_left;
        top = lh.top;
        bottom = lh.bottom;
    }
    void Clear(){date = 0; hhmm = 0; x_left = 0.0; x_right = 0.0; height = 0.0; columnar_top_left = CST_MAGIC_POINT;top = CST_MAGIC_POINT; bottom = CST_MAGIC_POINT;}
    
    int  date; 
    int  hhmm;
    double x_left;
    double x_right;
    double height;
    QPointF columnar_top_left;
    QPointF top;
    QPointF bottom;

};

enum class ZhibiaoType: unsigned char
{
    VOL = 0,
    MOMENTUM 
};
class ZhiBiaoAtom
{
public:
    ZhiBiaoAtom(){}
    virtual ~ZhiBiaoAtom() {} 

    virtual void val0( double ){}
    virtual double val0(){ return 0.0;}
    virtual void val1( double ){}
    virtual double val1(){ return 0.0;}
    virtual void val2( double ){}
    virtual double val2(){ return 0.0;}
    virtual void val3( double ){}
    virtual double val3(){ return 0.0;}
};

class T_KlineDataItem //_t_kline_dataitem
{
public:
    T_StockHisDataItem  stk_item;
    int  type;
    int  tag;
    std::vector<std::shared_ptr<ZhiBiaoAtom> > zhibiao_atoms;

    T_KlineDataItem() : type(int(FractalType::UNKNOW_FRACTAL)), tag(int(TagType::UNKNOW_TAG)), kline_posdata_1(), kline_posdata_0()
    {
        memset(&stk_item, 0, sizeof(stk_item));
    }
    explicit T_KlineDataItem(const T_KlineDataItem & lh)
    {
        if( this == &lh ) 
            return;
        CreateHelper(lh);
    }
    explicit T_KlineDataItem(T_KlineDataItem && lh): stk_item(std::move(lh.stk_item)), type(lh.type), tag(lh.tag), kline_posdata_1(std::move(lh.kline_posdata_1)), kline_posdata_0(std::move(lh.kline_posdata_0))
        , zhibiao_atoms(std::move(lh.zhibiao_atoms))
    {  
    }
    T_KlineDataItem & operator = (const T_KlineDataItem & lh)
    {
        if( this == &lh )
            return *this;
        CreateHelper(lh);
        return *this;
    }
    explicit T_KlineDataItem(const T_StockHisDataItem & stock_his_data_item): type(int(FractalType::UNKNOW_FRACTAL)), tag(int(TagType::UNKNOW_TAG)), kline_posdata_0(), kline_posdata_1()
    {
        memcpy(&stk_item, &stock_his_data_item, sizeof(stock_his_data_item)); 
    }
    T_KlinePosData & kline_posdata(int index = 0)
    {
        if( index == 0 ) return kline_posdata_0;
        else return kline_posdata_1;
    }
    bool operator ==(const T_KlineDataItem &b) const
    {
        return stk_item.date == b.stk_item.date && stk_item.hhmmss == b.stk_item.hhmmss;
    }
    bool operator !=(const T_KlineDataItem &b) const
    {
        return stk_item.date != b.stk_item.date || stk_item.hhmmss != b.stk_item.hhmmss;
    }
    bool operator <=(const T_KlineDataItem &b) const
    {
       if( stk_item.date < b.stk_item.date ) return true;
       else if( stk_item.date == b.stk_item.date ) return stk_item.hhmmss <= b.stk_item.hhmmss;
       else return false;
    }
    bool operator <(const T_KlineDataItem &b) const
    {
        if( stk_item.date < b.stk_item.date ) return true;
        else if( stk_item.date == b.stk_item.date ) return stk_item.hhmmss < b.stk_item.hhmmss;
        else return false;
    }
    bool operator >=(const T_KlineDataItem &b) const
    {
        if( stk_item.date > b.stk_item.date ) return true;
        else if( stk_item.date == b.stk_item.date ) return stk_item.hhmmss >= b.stk_item.hhmmss;
        else return false;
    }
    bool operator >(const T_KlineDataItem &b) const
    {
        if( stk_item.date > b.stk_item.date ) return true;
        else if( stk_item.date == b.stk_item.date ) return stk_item.hhmmss > b.stk_item.hhmmss;
        else return false;
    }
private:
    void CreateHelper(const T_KlineDataItem & lh)
    {
        memcpy(&stk_item, &lh.stk_item, sizeof(lh.stk_item));
        this->type = lh.type;
        this->tag = lh.tag;
        this->zhibiao_atoms = lh.zhibiao_atoms;
        this->kline_posdata_0 = lh.kline_posdata_0;
        this->kline_posdata_1 = lh.kline_posdata_1;
    }
    T_KlinePosData  kline_posdata_0;
    T_KlinePosData  kline_posdata_1;
};

class T_StockBaseInfoItem
{ 
public:
    T_StockBaseInfoItem() : type(0), time_to_market(0){}
    std::string code;
    int type;  // 0--normal stock 1--index code
    std::string pinyin;
    std::string name;
    int  time_to_market;
    std::string industry;
    std::string area;
    std::string remark;
};

enum class DrawAction : unsigned char 
{ 
    DRAWING_FOR_2PDOWN_C = 1, 
    DRAWING_FOR_2PUP_C, 
    DRAWING_FOR_3PDOWN_D, 
    DRAWING_FOR_3PUP_D, 
    NO_ACTION = 255
};

//bool IsNumber(const std::string& str);

bool TransIndexPinYin2CodeName(const std::string &pinyin, std::string &code, std::string &name);
bool TransIndexCode2Name(const std::string &code, std::string &name);
std::string TransIndex2TusharedCode(const std::string &code);

FractalType  MaxFractalType(int val);
FractalType  BtmestFractalType(int val);

bool IsTopFake(int val);
bool IsBtmFake(int val);

bool IsTopFractal(int type);
bool IsBtmFractal(int type);

enum class KGreenRedType : unsigned char
{
    UNKNOW_TYPE = 0,
    TEN_CROSS,

    SMALL_RED,
    MID_RED,
    STRONG_RED,

    SMALL_GREEN,
    MID_GREEN,
    STRONG_GREEN,
};

KGreenRedType KGGetGreenRedType(const T_StockHisDataItem &item, TypePeriod type_period);

class T_BiPoint
{
public:
    int index;
    int date;
    int hhmm;
    FractalType frac_type;
    T_BiPoint():index(-1), date(0), hhmm(0), frac_type(FractalType::UNKNOW_FRACTAL){}
    T_BiPoint(const T_BiPoint & lh) : index(lh.index), date(lh.date), hhmm(lh.hhmm), frac_type(lh.frac_type){}
    T_BiPoint & operator = (const T_BiPoint &lh)
    { 
        if(this == &lh){return *this;} 
        index = lh.index; date = lh.date; hhmm = lh.hhmm; frac_type = lh.frac_type;
        return *this;
    }
};

enum class BiType : unsigned char { UP = 0, DOWN};
class T_Bi 
{
public:
    BiType  type;
    T_BiPoint start;
    T_BiPoint end;
    T_Bi(BiType para_t, const T_BiPoint &para_start, const T_BiPoint &para_end) { type = para_t; start = para_start; end = para_end; }
    T_Bi(const T_Bi& bi) : type(bi.type), start(bi.start), end(bi.end){}
    T_Bi & operator = (const T_Bi& bi) {if( this == &bi){ return *this;} type = bi.type; start = bi.start; end = bi.end; }
};

#ifndef T_K_Data
typedef struct _t_k_data
{
    int yyyymmdd;
    int hhmmdd;
    double open;
    double close;
    double high;
    double low;
    int vol;
}T_K_Data;
#define T_K_Data  T_K_Data
#endif

typedef struct _t_quote_data
{
    double cur_price;
    double vol;
    double sell_price;
    double buy_price;
    int sell_vol;
    int buy_vol;
}T_Quote_Data;

enum class KAttributeType : unsigned char
{
    OPEN = 0, CLOSE, HIGH, LOW
};
enum class TrendType: unsigned char { UP = 0, DOWN, SHOCK, NO};
enum class LineType : unsigned char { UP = 0, DOWN};
//typedef struct _t_struct_line
class T_StructLine
{
public:
    T_StructLine() : type(LineType::UP), beg_index(-1), end_index(-1) {}
    T_StructLine(LineType para_type, int beg, int end) : type(para_type), beg_index(beg), end_index(end){}
    LineType  type;
    int beg_index;
    int end_index;
};
// time is from oldest to recent
typedef std::deque<std::shared_ptr<T_KlineDataItem> >  T_HisDataItemContainer;
typedef std::unordered_map<std::string, T_HisDataItemContainer>  T_CodeMapHisDataItemContainer;

typedef std::deque<std::shared_ptr<T_Bi> >  T_BiContainer;
typedef std::unordered_map<std::string, T_BiContainer> T_CodeMapBiContainer;

typedef std::deque<std::shared_ptr<T_StructLine> >  T_StructLineContainer; 

// eldest date, eldest time, latest date, latest time
typedef std::tuple<int, int, int, int>  T_DateRange;

class T_Section
{
public:
    int  top_left_index;
    double top_left_price;
    int  btm_right_index;
    double btm_right_price;
    QPointF top_left;
    QPointF btm_right;
    T_Section() : top_left_index(-1), top_left_price(0.0), btm_right_index(-1), btm_right_price(0.0), top_left(0.0, 0.0), btm_right(0.0, 0.0) {}
};
typedef std::vector<T_Section> T_SectionContainer;

class T_StructData
{
public:
    T_StructLineContainer  struct_line_container;
    T_SectionContainer     section_container;
    
};

typedef std::unordered_map<std::string, T_StructData> T_CodeMapStructDataContainer;

void utf8ToGbk(std::string& strUtf8);
void gbkToUtf8(std::string& strGbk);

QString ToQString(double val);
QString ToQString(int val);
std::string ToString(ForcastType type);
std::string ToString(ForcastSiteType type);

void ClearTopFractal(T_KlineDataItem &k_data_item);
 
void ClearBtmFractal(T_KlineDataItem &k_data_item);

 
int cur_hhmm();
bool IsNearCloseKTime(int hhmmss, TypePeriod type_period);

KGreenRedType KGGetGreenRedType(const T_StockHisDataItem &item, TypePeriod type_period);

PriceSpreadType JudgePriceSpreadType(TypePeriod type_period, double spread);
PriceSpreadType JudgeTrendPriceSpreadType(TypePeriod type_period, double spread);

int GetRelKHhmmTag(TypePeriod type_period, int tmp_hhmm);

void SoundFilled(bool is_o_filled);

#define  EPSINON  0.0001
#define  DEFAULT_DECIMAL 1

#define  MOMENTUM_POS 0

#define  DEFAULT_MAINKWALL_TYPE_PERIOD TypePeriod::PERIOD_5M //TypePeriod::PERIOD_1M //
#define  DEFAULT_SUBKWALL_TYPE_PERIOD  TypePeriod::PERIOD_30M
// train mode related
#define  DEFAULT_ORI_STEP_TYPE_PERIOD  TypePeriod::PERIOD_1M
#define  DEFAULT_TRAIN_DAYS            30
#define  DEFAULT_TRAIN_END_HHMM        1500
//-------------------
#define COMBO_PERIOD_1M_INDEX    0
#define COMBO_PERIOD_5M_INDEX    1
#define COMBO_PERIOD_15M_INDEX   2
#define COMBO_PERIOD_30M_INDEX   3
#define COMBO_PERIOD_HOUR_INDEX  4
#define COMBO_PERIOD_DAY_INDEX   5

#define MARKET_SH_FUTURES  30


#define  MIN_TRADE_DATE 19800000
#define  MAX_TRADE_DATE 20500000

// ----------------------------
#define SOUND_QDO 262 
#define SOUND_QRE 294
#define SOUND_QMI 330
#define SOUND_QFA 349
#define SOUND_QSO 392
#define SOUND_QLA 440
#define SOUND_QSI 494
#define SOUND_DO 523

#define SOUND_MI1 1318

#define ONE_BEAT 400

#define DEFAULT_TRAVERSE_LEFT_K_NUM  50

#endif // STKFO_COMMON_SDF3DSF_H_