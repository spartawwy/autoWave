#ifndef TLS_CMOMMON_H_DSF23DSFDS_
#define TLS_CMOMMON_H_DSF23DSFDS_

// kbar type
#define    KTYPE_PERIOD_1M     1
#define    KTYPE_PERIOD_5M     2
#define    KTYPE_PERIOD_15M    3
#define    KTYPE_PERIOD_30M    4
#define    KTYPE_PERIOD_HOUR   5
#define    KTYPE_PERIOD_DAY    6
#define    KTYPE_PERIOD_WEEK   7
#define    KTYPE_PERIOD_MON    8
#define    KTYPE_PERIOD_YEAR   9

#define MAGIC_STOP_PRICE (-1.0f)

// equal to kbar type
enum TlsTypePeriod  
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

#define MARKET_SH_FUTURES  30

#define  MAX_K_COUNT 500 

struct T_KbarData
{
    int  date; 
    int  hhmmss;
    double open;
    double close;
    double high;
    double low;
    double vol;
    double hold;  // total hold position
    T_KbarData() : date(0), hhmmss(0), open(0.0), close(0.0), high(0.0), low(0.0), vol(0.0){}
    T_KbarData(const T_KbarData&lh) : date(lh.date), hhmmss(lh.hhmmss), open(lh.open)
        , close(lh.close), high(lh.high), low(lh.low), vol(lh.vol){}

};

typedef int T_GetDataCallBack(void *para, int k_type, T_KbarData *data_ret[], unsigned int size);



#endif //TLS_CMOMMON_H_DSF23DSFDS_