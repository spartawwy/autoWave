#ifndef SYS_COMMON_SDFKJKDF_H_
#define SYS_COMMON_SDFKJKDF_H_

#include <string>
#include <vector>

#define  OUT
#define  IN
#define  INOUT
#define  MAX_VAL(a,b) ((a)>(b) ? (a) : (b))
#define  MIN_VAL(a,b) ((a)>(b) ? (b) : (a))


enum class TypePeriod : unsigned char
{
    PERIOD_1M = 1,
    PERIOD_5M,
    PERIOD_15M,
    PERIOD_30M,
    PERIOD_HOUR,
    PERIOD_DAY,
    PERIOD_WEEK,
    PERIOD_MON,
    PERIOD_YEAR,
};


typedef struct _t_quotedata
{
    // 'code','date','time','price','change','volume','amount','type'
    char code[16];
    int  date; //yyyymmdd
    //__int64  time; //HHMMSS
    int hhmmss;
    int msecond;
    double price; // .2f
    //double price_change;
    //int  vol;
    //unsigned char bid_type; // 0: buy_pan  1 :sell_pan
    double b_1;
    int v_b_1;
    double b_2;
    double b_3;
    double b_4;
    double b_5;
    double s_1;
    int v_s_1;
    double s_2;
    double s_3;
    double s_4;
    double s_5; 
}T_QuoteData;

typedef void (*QuoteCallBack)(T_QuoteData *quote_data, void *para, char *ErrInfo);

typedef struct quote_call_back_data_
{
    QuoteCallBack quote_call_back;
    void *para;
    unsigned int  each_delay_ms;

    char code[256];
    int date_begin;
    int hhmm_begin;
    int date_end ;
    int hhmm_end;
}QuoteCallBackData;

bool IsStrAlpha(const std::string& str);
bool IsNumber(const std::string& str);
bool IsStrNum(const std::string& str);
bool IsDouble(const std::string& str);
bool TransToDouble(const std::string& str, double &ret_val);

double ProcDecimal(double val, unsigned int decimal=1); // decimal 小数位数
bool Equal(double lh, double rh);

void WriteLog(char *file_name_tag, const char *fmt, ...);

bool IsLegalTradeHHmm(int hhmm);

std::vector<std::string> StringSplit(const std::string &str, char separator);

#endif