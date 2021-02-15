#ifndef STOCK_DATA_MAN_H
#define STOCK_DATA_MAN_H

//#include <list>
//#include <iostream>
#include <memory>
#include <unordered_map>
#include <vector>
 
#include "stkfo_common.h"
#include "zhibiao.h"

#include "stockdayinfo.h"

#include "stk_quoter_api.h"
 
#include "winner_hq_api.h"
  
#include "tdx_exhq_wrapper.h"
 
#include "kline_wall.h"

//<code, T_StockHisDataItem)
typedef std::unordered_map<std::string, T_StockHisDataItem>  T_CodeMapAutomDataItemContainer;
typedef std::unordered_map<PeriodType, T_CodeMapAutomDataItemContainer>  T_CodeMapStkDataItemContainer;

//<T, IndependentZhibiao>
typedef std::unordered_map<unsigned int, std::shared_ptr<IndependentZhibiao> > T_CycleTMapIndepentZhibiaoAtom;
typedef std::unordered_map<PeriodType, T_CycleTMapIndepentZhibiaoAtom> T_TypeMapIndepentZhibiao;
typedef std::unordered_map<std::string, T_TypeMapIndepentZhibiao> T_CodeMapIndepentZhibiao;

TypePeriod ToTypePeriod(PeriodType src);
PeriodType ToPeriodType(TypePeriod src);

namespace  TSystem
{
    class LocalLogger;
}
class KLineWall;
class ExchangeCalendar;
class DataBase;
class StockDataMan
{
public:

    StockDataMan(/*KLineWall *p_kwall, */ExchangeCalendar *p_exchange_calendar, TSystem::LocalLogger &local_logger, DataBase &data_base);
    ~StockDataMan();
    bool Init();

    //std::vector<std::shared_ptr<T_KlineDataItem> > &day_kline_data_container() { return day_kline_data_container_; }
    ExchangeCalendar * exchange_calendar() { return p_exchange_calendar_;}

public:

    //从fileName指定的磁盘路径中将数据一行一行读取出来，每一行初始化一个StockDayInfo对象
    //void LoadDataFromFile(std::string &fileName);

    T_HisDataItemContainer* FindStockData(PeriodType period_type, const std::string &stk_code, int start_date, int end_date, int cur_hhmm, bool is_index=false);
    T_HisDataItemContainer* FindStockData(PeriodType period_type, const std::string &stk_code, int start_date, int end_date, bool is_index=false);
    T_HisDataItemContainer* AppendStockData(PeriodType period_type, int nmarket, const std::string &stk_code, int start_date, int end_date, bool is_index=false);
	     
    void TraverseSetFeatureData(const std::string &stk_code, PeriodType period_type, bool is_index, int r_start_index, unsigned int max_left_len=0);

    int UpdateOrAppendLatestItemStockData(PeriodType period_type, int nmarket, const std::string &stk_code, OUT double &quote_price, bool is_index=false);
    int AppendLatestItemStockData(PeriodType period_type, int nmarket, const std::string &stk_code,  bool is_index=false);
    void TraverseGetBi(PeriodType period_type, const std::string &code, std::deque<std::shared_ptr<T_KlineDataItem> > &kline_data_items);

    void TraverseGetStuctLines(PeriodType period_type, const std::string &code, int r_start_index, std::deque<std::shared_ptr<T_KlineDataItem> > &kline_data_items);
    void AppendStructLinesIfNeccary(PeriodType period_type, const std::string &code, const T_StructLineContainer &lines);

    void TraversGetSections(PeriodType period_type, const std::string &code, std::deque<std::shared_ptr<T_KlineDataItem> > &kline_data_items);
     
    bool GetInstrumentQuote(const std::string &code, int nmarket, T_Quote_Data &ret_quote_data);

    T_HisDataItemContainer* LoadPartStockData(PeriodType period_type, int nmarket, const std::string &stk_code, int start_index, int len, bool is_index);
    T_HisDataItemContainer* LoadPartStockData(PeriodType period_type, int nmarket, const std::string &stk_code, const T_DateRange &range, bool is_index);

    std::tuple<int, int> AppendPartStockData(PeriodType period_type, int nmarket, const std::string &stk_code, const T_DateRange &range, bool is_index=false);

    void StoreEndStockDateItem(PeriodType period_type, const std::string &stk_code, const T_StockHisDataItem &item);
    T_StockHisDataItem * FindEndStockDateItemFromEndContain(PeriodType period_type, const std::string &stk_code);

    int GetRelKDateTag(PeriodType type_period, int date, int tmp_hhmm);

public:
      
	double GetHisDataLowestMinPrice(PeriodType period_type, const std::string& code, int start_date, int end_date);
	double GetHisDataHighestMaxPrice(PeriodType period_type, const std::string& code, int start_date, int end_date);

public: 

    T_HisDataItemContainer &GetHisDataContainer(TypePeriod type_period, const std::string& code)
    {
        return GetHisDataContainer(ToPeriodType(type_period), code);
    }

    T_HisDataItemContainer *FindHisDataContainer(PeriodType period_type, const std::string& code);
    T_HisDataItemContainer &GetHisDataContainer(PeriodType period_type, const std::string& code);
    T_BiContainer &GetBiContainer(PeriodType period_type, const std::string& code);
    T_StructLineContainer &GetStructLineContainer(PeriodType period_type, const std::string& code);
    T_SectionContainer &GetStructSectionContainer(PeriodType period_type, const std::string& code/*, int wall_index*/);

    // (stock , data)  k data's date is from small to big
    T_CodeMapHisDataItemContainer m1_stock_his_items_;
    T_CodeMapHisDataItemContainer m5_stock_his_items_;
    T_CodeMapHisDataItemContainer m15_stock_his_items_;
    T_CodeMapHisDataItemContainer m30_stock_his_items_;
    T_CodeMapHisDataItemContainer hour_stock_his_items_;
    T_CodeMapHisDataItemContainer day_stock_his_items_;
    T_CodeMapHisDataItemContainer week_stock_his_items_;
    T_CodeMapHisDataItemContainer mon_stock_his_items_;

    T_CodeMapBiContainer m1_stock_bi_items_;
    T_CodeMapBiContainer m5_stock_bi_items_;
    T_CodeMapBiContainer m15_stock_bi_items_;
    T_CodeMapBiContainer m30_stock_bi_items_;
    T_CodeMapBiContainer hour_stock_bi_items_;
    T_CodeMapBiContainer day_stock_bi_items_;
    T_CodeMapBiContainer week_stock_bi_items_;
    T_CodeMapBiContainer mon_stock_bi_items_;

    T_CodeMapStructDataContainer m1_stock_struct_datas_;
    T_CodeMapStructDataContainer m5_stock_struct_datas_;
    T_CodeMapStructDataContainer m15_stock_struct_datas_;
    T_CodeMapStructDataContainer m30_stock_struct_datas_;
    T_CodeMapStructDataContainer hour_stock_struct_datas_;
    T_CodeMapStructDataContainer day_stock_struct_datas_;
    T_CodeMapStructDataContainer week_stock_struct_datas_;
    T_CodeMapStructDataContainer mon_stock_struct_datas_;

#ifndef USE_STK_QUOTER
    std::vector<T_StockHisDataItem> *p_stk_hisdata_item_vector_;
    bool is_fetched_stk_hisdata_;
#endif 

    T_CodeMapStkDataItemContainer  end_stk_data_items_;

    T_CodeMapIndepentZhibiao ave_lines_zhibiao_datas_;

    void ReCaculateZhibiaoItem(T_HisDataItemContainer &data_items_in_container, unsigned int item_index); 

    void AddAveLineT(unsigned int line_t);
    void DelAveLineT(unsigned int line_t);
    void ReCaculateAveLineZhibiaoItem(PeriodType period_type, const std::string& code, T_HisDataItemContainer &data_items_in_container, unsigned int item_index); 
    void ReCaculateAveLineZhibiao(PeriodType period_type, const std::string& code, T_HisDataItemContainer &data_items_in_container, unsigned int beg_index, unsigned int end_index); 
    double GetAveLineZhibiao(PeriodType period_type, const std::string& code, unsigned int line_t, unsigned int index);

private:

    std::tuple<int, int> GetDateIndexFromContainer(PeriodType period_type, const std::string& stock, int start_date, int end_date);
    void CaculateZhibiao(T_HisDataItemContainer &data_items_in_container);


private:

    //KLineWall *kwall_;
#if 1
    TdxExHqWrapper  tdx_exhq_wrapper_;
#endif
     
   std::vector<ZhibiaoType> zhibiao_types_;
   ExchangeCalendar *p_exchange_calendar_;
   TSystem::LocalLogger &local_logger_;
   DataBase  &data_base_;

   std::mutex ave_zhibiao_lines_t_mutex_;
   std::unordered_map<unsigned int, bool> ave_zhibiao_lines_t_;
  /* unsigned int line_0_t_;
   unsigned int line_1_t_;*/
};
int find_next_btm_fractal(std::deque<std::shared_ptr<T_KlineDataItem> > &kline_data_items, int from_index);
int find_next_top_fractal(std::deque<std::shared_ptr<T_KlineDataItem> > &kline_data_items, int from_index);
void find_down_towardleft_end(std::deque<std::shared_ptr<T_KlineDataItem> > &kline_data_items, bool is_src_k_same, double lowest_price, int start, int &end);
void find_up_towardleft_end(std::deque<std::shared_ptr<T_KlineDataItem> > &kline_data_items, bool is_src_k_same, double highest_price, int start, int &end);
void Traverse_GetStuctLines(IN T_HisDataItemContainer & kline_data_items, int r_start_index, int l_end_index, OUT T_StructLineContainer &container);


// < 0 : meaning no related data
int FindDataIndex(T_HisDataItemContainer &data_items_in_container, int date, int cur_hhmm);
bool IsDataIn(T_HisDataItemContainer &data_items_in_container, int date);

// 下分形遍历
void TraverseSetUpwardFractal(T_HisDataItemContainer &kline_data_items, int r_start_index = 0, int backward_size = 0);
// 上分形遍历
void TraverseSetDownwardFractal(T_HisDataItemContainer &kline_data_items, int r_start_index = 0, int backward_size = 0);

void TraverseClearFractalType(T_HisDataItemContainer &kline_data_items, int r_start_index = 0, int backward_size = 0);

void TraverseAjustFractal(T_HisDataItemContainer &kline_data_items, int r_start_index = 0, int backward_size = 0);

void TraverSetSignale(TypePeriod type_period, T_HisDataItemContainer &data_items_in_container, /*bool is_only_set_tail,*/ int r_start_index = 0, int backward_size = 0);

//typedef std::function<int(double o, double c, double h, double l)>  QueryKHandlerType;
typedef std::function<int(void *para0/*, void *para1, void *para2, void *para3*/)>  QueryKHandlerType;

int KRef(const T_HisDataItemContainer & items, int start, int pre_or_back_range, QueryKHandlerType && handler = QueryKHandlerType());

T_KlineDataItem * KRef(T_HisDataItemContainer & items, int start, int distance=0);
double KRef(T_HisDataItemContainer & items, int index, KAttributeType attr);

// include start. distance < 0 : toward left; distance > 0 : toward right distance==0 : only start
//ps: start <= range <= start +distance or start + distance <= range <= start 
int Lowest(T_HisDataItemContainer & items, int start, int distance);
// include start. distance < 0 : toward left; distance > 0 : toward right distance==0 : only start
//ps: start <= range <= start +distance or start + distance <= range <= start 
int Highest(T_HisDataItemContainer & items, int start, int distance);

// find last bar(kline) from start to left which fit condition (attr compare val)
int BarLast(T_HisDataItemContainer & items, int start, KAttributeType attr, CompareType compare_type, double val);
// find first bar(kline) toward right [start, right_end] which fit condition (attr compare val)
int BarFirst(T_HisDataItemContainer & items, int start, int right_end, KAttributeType attr, CompareType compare_type, double val);

// find last bar(kline) from start(not include start) to left which is fenxin k. [left_end, start)
std::tuple<int, FractalGeneralType> FractalLast(T_HisDataItemContainer & items, int start, int left_end);

// find first special fenxin toward right [start, right_end]
int FractalFirst(T_HisDataItemContainer & items, FractalGeneralType fractal_type, int start, int right_end);
// find fenxin toward left [left_end, start)
std::vector<int> FindFractalsBetween(T_HisDataItemContainer & items, FractalGeneralType type, int start, int left_end);

TrendType JudgeTrendBaseonFxRightToLeft(T_HisDataItemContainer & items, int start, int toward_left_len
                                        , OUT int &top_index, OUT int &mid_index, OUT int &btm_index);

int PreTradeDaysOrNightsStartIndex(ExchangeCalendar &calendar, TypePeriod type_period, T_HisDataItemContainer &k_datas, int index, int t);

int Binary_Search(T_HisDataItemContainer &k_datas, int start, int end, T_KlineDataItem &item); 
int Lower_Bound(T_HisDataItemContainer &k_datas, int start, int end, T_KlineDataItem &item);

int FindDateHHmmRelIndex(T_HisDataItemContainer &k_datas, int left_end_index, int right_end_index, int date, int hhmm);

#endif // STOCK_DATA_MAN_H
