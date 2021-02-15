
#ifndef STK_QUOTER_H_23DSFSF_
#define STK_QUOTER_H_23DSFSF_

#include <cstring>

#ifdef STKQUOTER_EXPORTS 
#define STKQUOTER_IMEXPORT   __declspec(dllexport)
#else
#define STKQUOTER_IMEXPORT   __declspec(dllimport)
#endif

struct T_StockPriceInfo
{
    char code[16];
    double open_price;
    double pre_close_price;
    double cur_price;
    T_StockPriceInfo() : open_price(0.0), pre_close_price(0.0), cur_price(0.0) { code[0] = '\0'; }
    T_StockPriceInfo(const T_StockPriceInfo &rh) : open_price(rh.open_price), pre_close_price(rh.pre_close_price), cur_price(rh.cur_price)
    {
        strcpy_s(code, rh.code);
    }
    T_StockPriceInfo& operator = (const T_StockPriceInfo &rh)
    {
        strcpy_s(code, rh.code);
        open_price = rh.open_price;
        pre_close_price = rh.pre_close_price;
        cur_price = rh.cur_price;
        return *this;
    }
};

struct T_StockHisDataItem
{
    int  date; 
    int  hhmmss;
    double close_price;
    double high_price;
    double low_price;
    double open_price;
    //double pre_close_price;
    double vol;
    double capital;
    T_StockHisDataItem() : date(0), hhmmss(0), close_price(0.0), high_price(.0),low_price(0.0),open_price(0.0),vol(0.0),capital(0.0){}
    T_StockHisDataItem(int date_p, int hhmmss_p
                       , double close_price_p, double high_price_p
                        , double low_price_p, double open_price_p
                        /*, double pre_close_price_p*/, double vol_p):date(date_p), hhmmss(hhmmss_p)
                        , close_price(close_price_p), high_price(high_price_p), low_price(low_price_p), open_price(open_price_p)
                        , vol(vol_p) {}
    T_StockHisDataItem(const T_StockHisDataItem &lh):date(lh.date), hhmmss(lh.hhmmss)
                        , close_price(lh.close_price), high_price(lh.high_price), low_price(lh.low_price), open_price(lh.open_price)
                        , /*pre_close_price(lh.pre_close_price),*/ vol(lh.vol), capital(lh.capital){}
    T_StockHisDataItem & operator = (const T_StockHisDataItem&lh)
    {
        date = lh.date;hhmmss = lh.hhmmss; 
        close_price = lh.close_price; high_price = lh.high_price;
        low_price = lh.low_price; open_price = lh.open_price;
        vol = lh.vol; capital = lh.capital;
        return *this;
    }
};

extern "C" int STKQUOTER_IMEXPORT StkQuoteGetQuote(char stocks[][16], const unsigned int size, T_StockPriceInfo price_info[]/*, unsigned int *ret_size*/);

typedef int (__stdcall *StkQuoteGetQuoteDelegate)(char stocks[][16], const unsigned int size, T_StockPriceInfo price_info[]);

/***********************************************************************
T_StockHisDataItem is from big date to small date ;
 ps: max T_StockHisDataItem is 265, so make sure max days between is 265 * 3/2 =  396  
***********************************************************************/
extern "C" int STKQUOTER_IMEXPORT  StkHisData(char stocks[16], int start_date, int end_date, T_StockHisDataItem **price_info);

typedef int (__stdcall *StkHisDataDelegate)(char stocks[16], int start_date, int end_date, T_StockHisDataItem **pp_items);

extern "C" void STKQUOTER_IMEXPORT StkRelHisData(T_StockHisDataItem* p_items);

typedef void (__stdcall *StkRelHisDataDelegate)(T_StockHisDataItem* p_items);

#endif