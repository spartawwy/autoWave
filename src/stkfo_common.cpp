#include "stkfo_common.h"

#include <boost/lexical_cast.hpp>

#include <algorithm>

#include <qtextcodec.h>
#include <qdebug.h>
#include <cmath>
#include <cassert>


bool TransIndexPinYin2CodeName(const std::string &pinyin, std::string &code, std::string &name)
{ 
    if( pinyin == "SZZS" ) //上证指数
    {
        code = "000001";
        name = "上证指数";
    }
    else if( pinyin == "SZCZ" ) // 深圳成指
    {
        code = "399001"; 
        name = "深圳成指";
    }
    else if( pinyin == "ZXBZ" ) //中小板指数
    { 
        code = "399005"; 
        name = "中小板指";
    }
    else if( pinyin == "CYBZ" ) //创业板指数
    {
        code = "399006";
        name = "创业板指";
    }
    else if( pinyin == "SZ50" ) // 上证50
    {
        code = "000016"; 
        name = "上证50";
    }
    else if( pinyin == "HS300" ) // 沪深300 
    {
        code = "000300"; 
        name = "沪深300";
    }
    else 
        return false;
    return true;
}


bool TransIndexCode2Name(const std::string &code, std::string &name)
{ 
    if( code == "999999" ) //上证指数 000001
    { 
        name = "上证指数";
    }
    else if( code == "399001" ) // 深圳成指
    { 
        name = "深圳成指";
    }
    else if( code == "399005" ) //中小板指数
    {  
        name = "中小板指";
    }
    else if( code == "399006" ) //创业板指数
    { 
        name = "创业板指";
    }
    else if( code == "000016" ) // 上证50
    { 
        name = "上证50";
    }
    else if( code == "000300" ) // 沪深300 
    { 
        name = "沪深300";
    }
    else 
        return false;
    return true;
}

std::string TransIndex2TusharedCode(const std::string &code)
{
    if( code == "999999" || code == "SZZS" ) //上证指数
        return "000001";
    else if( code == "399001" || code == "SZCZ" ) // 深圳成指
        return "399001"; 
    else if( code == "399005" || code == "ZXBZ" ) //中小板指数
        return "399005"; 
    else if( code == "399006" || code == "CYBZ" ) //创业板指数
        return "399006";
    else if( code == "000016" || code == "SZ50" ) // 上证50
        return "000016"; 
    else if( code == "000300" || code == "HS300" ) // 沪深300 
        return "000300"; 
    return code;
}

FractalType  MaxFractalType(int val)
{
    if( (val & int(FractalType::TOP_FAKE)) == int(FractalType::TOP_FAKE) )
        return FractalType::TOP_FAKE;
    else if( (val & int(FractalType::TOP_AXIS_T_11)) == int(FractalType::TOP_AXIS_T_11) )
        return FractalType::TOP_AXIS_T_11;
    else if( (val & int(FractalType::TOP_AXIS_T_9)) == int(FractalType::TOP_AXIS_T_9) )
        return FractalType::TOP_AXIS_T_9;
    else if( (val & int(FractalType::TOP_AXIS_T_7)) == int(FractalType::TOP_AXIS_T_7) )
        return FractalType::TOP_AXIS_T_7;
    else if( (val & int(FractalType::TOP_AXIS_T_5)) == int(FractalType::TOP_AXIS_T_5) )
        return FractalType::TOP_AXIS_T_5;
    else if( (val & int(FractalType::TOP_AXIS_T_3)) == int(FractalType::TOP_AXIS_T_3) )
        return FractalType::TOP_AXIS_T_3;

    else if( (val & int(FractalType::BTM_AXIS_T_11)) == int(FractalType::BTM_AXIS_T_11) )
        return FractalType::BTM_AXIS_T_11;
    else if( (val & int(FractalType::BTM_AXIS_T_9)) == int(FractalType::BTM_AXIS_T_9) )
        return FractalType::BTM_AXIS_T_9;
    else if( (val & int(FractalType::BTM_AXIS_T_7)) == int(FractalType::BTM_AXIS_T_7) )
        return FractalType::BTM_AXIS_T_7;
    else if( (val & int(FractalType::BTM_AXIS_T_5)) == int(FractalType::BTM_AXIS_T_5) )
        return FractalType::BTM_AXIS_T_5;
    else if( (val & int(FractalType::BTM_AXIS_T_3)) == int(FractalType::BTM_AXIS_T_3) )
        return FractalType::BTM_AXIS_T_3;
    else if( (val & int(FractalType::BTM_FAKE)) == int(FractalType::BTM_FAKE) )
        return FractalType::BTM_FAKE;
    else
        return FractalType::UNKNOW_FRACTAL;
}


FractalType  BtmestFractalType(int val)
{
    if( (val & int(FractalType::BTM_AXIS_T_11)) == int(FractalType::BTM_AXIS_T_11) )
        return FractalType::BTM_AXIS_T_11;
    else if( (val & int(FractalType::BTM_AXIS_T_9)) == int(FractalType::BTM_AXIS_T_9) )
        return FractalType::BTM_AXIS_T_9;
    else if( (val & int(FractalType::BTM_AXIS_T_7)) == int(FractalType::BTM_AXIS_T_7) )
        return FractalType::BTM_AXIS_T_7;
    else if( (val & int(FractalType::BTM_AXIS_T_5)) == int(FractalType::BTM_AXIS_T_5) )
        return FractalType::BTM_AXIS_T_5;
    else if( (val & int(FractalType::BTM_AXIS_T_3)) == int(FractalType::BTM_AXIS_T_3) )
        return FractalType::BTM_AXIS_T_3;
    else if( (val & int(FractalType::BTM_FAKE)) == int(FractalType::BTM_FAKE) )
        return FractalType::BTM_FAKE;
    else
        return FractalType::UNKNOW_FRACTAL;
}

bool IsTopFake(int val)
{
    return  (val & int(FractalType::TOP_FAKE)) == int(FractalType::TOP_FAKE);
}

bool IsBtmFake(int val)
{
    return  (val & int(FractalType::BTM_FAKE)) == int(FractalType::BTM_FAKE);
}

bool IsTopFractal(int type)
{
    if( MaxFractalType(type) >= FractalType::TOP_AXIS_T_3 )
        return true;
    else
        return false;
}

bool IsBtmFractal(int type)
{
    return BtmestFractalType(type) != FractalType::UNKNOW_FRACTAL;
}


 /*
    spread: micro <1.51;  1.5 < small < 3.01;  3.0 < mid < 5.01; 5.0 < big < 10.01;  10.0 < supper 
    
    enum class PriceSpreadType : unsigned char
    {
        MICRO=0, SMALL, MID, BIG, SUPPER,
    };*/

PriceSpreadType JudgePriceSpreadType(TypePeriod type_period, double spread)
{
    spread = fabs(spread);
    switch (type_period)
    {
     case TypePeriod::PERIOD_5M:
     {
             if( spread < 0.5 )
                 return PriceSpreadType::MINI;
             else if( spread < 1.51 )
                 return PriceSpreadType::MICRO;
             else if( spread < 3.01 )
                 return PriceSpreadType::SMALL;
             else if( spread < 5.01 )
                 return PriceSpreadType::MID;
             else if( spread < 10.01 )
                 return PriceSpreadType::BIG;
             else
                 return PriceSpreadType::SUPPER;
     }break;
     case TypePeriod::PERIOD_30M:
     {
        if( spread < 1.0 )
            return PriceSpreadType::MINI;
        else if( spread < 1.51 )
          return PriceSpreadType::MICRO;
        else if( spread < 3.01 )
            return PriceSpreadType::SMALL;
        else if( spread < 5.01 )
            return PriceSpreadType::MID;
        else if( spread < 10.01 )
            return PriceSpreadType::BIG;
        else
            return PriceSpreadType::SUPPER;
     }break;
     default: assert(false); break;
    }
    return PriceSpreadType::SMALL;
}

//  spread is a b price spread
PriceSpreadType JudgeTrendPriceSpreadType(TypePeriod type_period, double spread)
{
    spread = fabs(spread);
    switch (type_period)
    {
    case TypePeriod::PERIOD_5M:
        {
            if( spread < 1.51 )
                return PriceSpreadType::MINI;
            else if( spread < 2.0 ) //1.5
                return PriceSpreadType::MICRO;
            else if( spread < 5.0 )
                return PriceSpreadType::SMALL;
            else if( spread < 8.0 )
                return PriceSpreadType::MID;
            else if( spread < 12.01 )
                return PriceSpreadType::BIG;
            else
                return PriceSpreadType::SUPPER;
        }break;
    case TypePeriod::PERIOD_30M:
        {
            if( spread < 1.0 )
                return PriceSpreadType::MINI;
            else if( spread < 3.0 )
                return PriceSpreadType::MICRO;
            else if( spread < 5.0 )
                return PriceSpreadType::SMALL;
            else if( spread < 8.0 )
                return PriceSpreadType::MID;
            else if( spread < 12.01 )
                return PriceSpreadType::BIG;
            else
                return PriceSpreadType::SUPPER;
        }break;
    default: assert(false); break;
    }
    return PriceSpreadType::SMALL;
}

KGreenRedType KGGetGreenRedType(const T_StockHisDataItem &item, TypePeriod type_period)
{
    KGreenRedType  gr_type = KGreenRedType::UNKNOW_TYPE;
    double small = 0.0;
    double mid = 0.0;
    double big = 0.0;
    switch(type_period)
    {
    case TypePeriod::PERIOD_1M: small = 0.1; mid = 0.5; big = 1.0; break;
    case TypePeriod::PERIOD_5M: 
        small = 0.2; mid = 0.5; big = 1.5; break;
    case TypePeriod::PERIOD_15M: small = 0.5; mid = 1.5; big = 3.5;break;
    case TypePeriod::PERIOD_30M: small = 0.5; mid = 4.0; big = 8.0;break;
    case TypePeriod::PERIOD_HOUR: small = 1.0; mid = 4.0; big = 8.0;break;
    case TypePeriod::PERIOD_DAY: small = 2.5; mid = 7.0; big = 9.0;break;
    case TypePeriod::PERIOD_WEEK: small = 7.0; mid = 10.0; big = 15.0;break;
    default: assert(false);
    }
    double body_high = 0.0; 
    if( item.close_price > item.open_price + EPSINON )
    {
        body_high = item.close_price - item.open_price;
        if( body_high < small )
            gr_type = KGreenRedType::TEN_CROSS;
        else if( body_high < mid )
            gr_type = KGreenRedType::SMALL_RED;
        else if( body_high < big )
            gr_type = KGreenRedType::MID_RED;
        else
            gr_type = KGreenRedType::STRONG_RED;
    }else if( item.close_price < item.open_price - EPSINON )
    {
        body_high = item.open_price - item.close_price;
        if( body_high < small )
            gr_type = KGreenRedType::TEN_CROSS;
        else if( body_high < mid )
            gr_type = KGreenRedType::SMALL_GREEN;
        else if( body_high < big )
            gr_type = KGreenRedType::MID_GREEN;
        else
            gr_type = KGreenRedType::STRONG_GREEN;
    }else
        gr_type = KGreenRedType::TEN_CROSS;

    return gr_type;
}

void SoundFilled(bool is_o_filled)
{
    if( is_o_filled )
        _beep(SOUND_DO, ONE_BEAT/2);
    else
        _beep(SOUND_MI1, ONE_BEAT/2);
}

//void ClearTopFractal(int &val)
void ClearTopFractal(T_KlineDataItem &k_data_item)
{
    int vals[] = {(int)FractalType::TOP_AXIS_T_3, (int)FractalType::TOP_AXIS_T_5, (int)FractalType::TOP_AXIS_T_7
        , (int)FractalType::TOP_AXIS_T_9, (int)FractalType::TOP_AXIS_T_11, (int)FractalType::TOP_FAKE};
    for( int i = 0; i < sizeof(vals) / sizeof(vals[0]); ++i )
    {
        int tmp_val = int(vals[i]);
        tmp_val ^= 0xffffffff;
        k_data_item.type &= tmp_val;
    }
    qDebug() << __FUNCTION__ << "  " << k_data_item.stk_item.date << ":" << k_data_item.stk_item.hhmmss << "\n";
}

//void ClearBtmFractal(int &val)
void ClearBtmFractal(T_KlineDataItem &k_data_item)
{
    int vals[] = {(int)FractalType::BTM_AXIS_T_3, (int)FractalType::BTM_AXIS_T_5, (int)FractalType::BTM_AXIS_T_7
        , (int)FractalType::BTM_AXIS_T_9, (int)FractalType::BTM_AXIS_T_11, (int)FractalType::BTM_FAKE};
    for( int i = 0; i < sizeof(vals) / sizeof(vals[0]); ++i )
    {
        int tmp_val = int(vals[i]);
        tmp_val ^= 0xffffffff;
        k_data_item.type &= tmp_val;
    }
    qDebug() << __FUNCTION__ << "  " << k_data_item.stk_item.date << ":" << k_data_item.stk_item.hhmmss << "\n";
}

 
int cur_hhmm()
{
    time_t rawtime;
    struct tm timeinfo;
    time( &rawtime );
    localtime_s(&timeinfo, &rawtime); 
    return timeinfo.tm_hour * 100 + timeinfo.tm_min;
}

bool IsNearCloseKTime(int hhmmss, TypePeriod type_period)
{
    const int minute = hhmmss % 10000 / 100;
    const int second = hhmmss % 100; 
    // time near close k(5 minute k)
    switch (type_period)
    {
    case TypePeriod::PERIOD_5M: 
        return (minute % 5 == 0 && second == 0) || (minute % 5 == 4 && second > 56);
    default:
        return false;
    }
}

void utf8ToGbk(std::string& strUtf8)
{
    QTextCodec* utf8Codec = QTextCodec::codecForName("utf-8");
    QTextCodec* gbkCodec = QTextCodec::codecForName("gbk");

    QString strUnicode = utf8Codec->toUnicode(strUtf8.c_str());
    QByteArray ByteGbk = gbkCodec->fromUnicode(strUnicode);

    strUtf8 = ByteGbk.data();
}

void gbkToUtf8(std::string& strGbk)
{
    QTextCodec* utf8Codec = QTextCodec::codecForName("utf-8");
    QTextCodec* gbkCodec = QTextCodec::codecForName("gbk");

    QString strUnicode = gbkCodec->toUnicode(strGbk.c_str());
    QByteArray ByteUtf8 = utf8Codec->fromUnicode(strUnicode);
    strGbk = ByteUtf8.data();
}

QString ToQString(double val)
{ 
    return QString::number(val, 'f', DEFAULT_DECIMAL);
}

QString ToQString(int val)
{ 
   //return ToQString(double(val));
    return QString::number(val);
}


int GetRelKHhmmTag(TypePeriod type_period, int tmp_hhmm)
{
    static auto get_hhmm = [](int hhmm_para, int *tp_array, int num)->int
    {
        assert(num > 0);
        int i = 0;
        for( ; i < num; ++i )
        {
            if( hhmm_para <= tp_array[i] ) 
                break;
        }
        if( i < num )
            return tp_array[i] == 2359 ? 0 : tp_array[i]; 
        return tp_array[num-1];
    };
    int hhmm = 0;
    switch( type_period )
    {
    case TypePeriod::PERIOD_YEAR: 
    case TypePeriod::PERIOD_MON:
    case TypePeriod::PERIOD_DAY:
    case TypePeriod::PERIOD_WEEK:
        break;
    case TypePeriod::PERIOD_HOUR:  // ndchk 
        {
            //10:30 13:00 14:00 15:00
            int tp_array[] = {100, 200, 930, 1045, 1345, 1445, 1500, 2200, 2300, 2359};
            hhmm = get_hhmm(tmp_hhmm, tp_array, sizeof(tp_array)/sizeof(tp_array[0]));
            break;
        }
    case TypePeriod::PERIOD_30M:
        {
            int tp_array[] = {30, 100, 130, 200, 230, 930, 1000, 1045, 1115, 1345, 1415, 1445, 1500, 2130, 2200, 2230, 2300, 2330, 2359};
            hhmm = get_hhmm(tmp_hhmm, tp_array, sizeof(tp_array)/sizeof(tp_array[0]));
            break; 
        }
    case TypePeriod::PERIOD_15M:
        {
            int tp_array[] = {15,30, 45, 100, 115, 130, 145, 200,215, 230
                , 915, 930, 945, 1000, 1015, 1045, 1100, 1115, 1130, 1345, 1400, 1415, 1430, 1445, 1500
                , 2115, 2130, 2145, 2200, 2215, 2230, 2245, 2300, 2315, 2330, 2345, 2359};
            hhmm = get_hhmm(tmp_hhmm, tp_array, sizeof(tp_array)/sizeof(tp_array[0]));
            break;
        }
    case TypePeriod::PERIOD_5M:
        {
            // ndedt
            int tp_array[] = {5, 10, 15,20,25,30,35,40,45,50,55,100,105,110,115,120,125,130,135,140,145,150,155
                ,200,205,210,215,220,225,230
                ,905, 910, 915, 920, 925, 930, 935,940,945,950,955
                ,1000,1005,1010,1015,1035,1040,1045,1050,1055,1100,1105
                ,1110,1115,1120,1125,1130,1335,1340,1345,1350,1355,1400,1405
                ,1410,1415,1420,1425,1430,1435,1440,1445,1450,1455,1500
                , 2105, 2110,2115, 2120, 2125,2130,2135,2140,2145,2150,2155,2200
                , 2205, 2210,2215, 2220, 2225,2230,2235,2240,2245,2250,2255,2300
                , 2305, 2310,2315, 2320, 2325,2330,2335,2340,2345,2350,2355,2359};
            hhmm = get_hhmm(tmp_hhmm, tp_array, sizeof(tp_array)/sizeof(tp_array[0]));
            break;
        }
    case TypePeriod::PERIOD_1M:
        hhmm = tmp_hhmm;
        break;
    }
    return hhmm;
}

std::string ToString(ForcastType type)
{
    switch( type )
    {
    case ForcastType::BOUNCE_UP: return "BOUNCE_UP";
    case ForcastType::BOUNCE_DOWN: return "BOUNCE_DOWN";
    case ForcastType::TREND_UP: return "TREND_UP";
    case ForcastType::TREND_DOWN: return "TREND_DOWN";
    default: return "UNKOWN";
    }
    return "UNKOWN";
}

std::string ToString(ForcastSiteType type)
{
    switch( type )
    {
    case ForcastSiteType::C1: return "C1";
    case ForcastSiteType::C2: return "C2";
    case ForcastSiteType::C3: return "C3";
    case ForcastSiteType::D1: return "D1";
    case ForcastSiteType::D2: return "D2"; 
    case ForcastSiteType::D2p5: return "D2p5"; 
    case ForcastSiteType::D3: return "D3"; 
    default: return "UNKOWN";
    }
    return "UNKOWN";
}
 