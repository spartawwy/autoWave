#ifndef TREND_LINE_H_DSFDSKJK_
#define TREND_LINE_H_DSFDSKJK_

#include <deque>
#include "stock_data_man.h"

struct T_BreakAtomInfo
{
    T_BreakAtomInfo():k_index(-1)
        //, distance_price(-1.0)
    {}
    explicit T_BreakAtomInfo(int i/*, double dis_price*/):k_index(i)
        //, distance_price(dis_price)
    {}
    T_BreakAtomInfo(const T_BreakAtomInfo &lh):k_index(lh.k_index)
        //, distance_price(lh.distance_price)
    {}
    T_BreakAtomInfo& operator = (const T_BreakAtomInfo &lh)
    {
        if( this == &lh )
            return *this;
        k_index = lh.k_index;
        //distance_price = lh.distance_price;
        return *this;
    }
    int k_index;
    //double distance_price;
};

enum class TrendLineType{ UP, DOWN};
class  TrendLine
{
public:
    TrendLine(TrendLineType type) : id_(0), type_(type), beg_(-1), end_(-1), slope_(0.0)
        , h_price_index_(-1), l_price_index_(-1), is_alive_(true), is_double_top_(false), is_last_(true)
    {
        is_below_price_ = type == TrendLineType::UP ? true : false;
    }
    TrendLine(TrendLineType type, int beg, int end, double slope, int h_p_index, int l_p_index) : id_(0), type_(type), beg_(beg), end_(end), slope_(slope)
        , h_price_index_(h_p_index), l_price_index_(l_p_index), is_alive_(true), is_double_top_(false), is_last_(true)
    {
        is_below_price_ = type == TrendLineType::UP ? true : false;
    }
    TrendLine(const TrendLine& lh) : id_(lh.id_), type_(lh.type_), beg_(lh.beg_), end_(lh.end_), slope_(lh.slope_)
        , breakdown_infos_(lh.breakdown_infos_),breakup_infos_(lh.breakup_infos_)
        , h_price_index_(lh.h_price_index_), l_price_index_(lh.l_price_index_), is_below_price_(lh.is_below_price_)
        , is_alive_(lh.is_alive_), is_double_top_(lh.is_double_top_), is_last_(lh.is_last_)
    {
    }
    TrendLine & operator = (const TrendLine& lh)
    {
        if( this == &lh )
            return *this;
        id_ = lh.id_;
        type_ = lh.type_;
        beg_ = lh.beg_;
        end_ = lh.end_;
        slope_ = lh.slope_;
        breakdown_infos_ = lh.breakdown_infos_;
        breakup_infos_ = lh.breakup_infos_;
        h_price_index_ = lh.h_price_index_;
        l_price_index_ = lh.l_price_index_;
        is_below_price_ = lh.is_below_price_;
        is_alive_ = lh.is_alive_;
        is_double_top_ = lh.is_double_top_;
        is_last_ = lh.is_last_;
        return *this;
    }
    T_BreakAtomInfo * FindBreakInfo(bool is_break_up, int k_index)
    {
        std::deque<T_BreakAtomInfo> *p_infos = is_break_up ? &breakup_infos_ : &breakdown_infos_;
        for( unsigned int i = 0; i < p_infos->size(); ++i )
        {
            if( p_infos->at(i).k_index == k_index )
                return std::addressof(p_infos->at(i));
        }
        return nullptr;
    }
    void SetDead();

    unsigned int id_;
    TrendLineType  type_;
    int beg_;
    int end_;
    double slope_;
    int h_price_index_;
    int l_price_index_;
    bool is_below_price_;
    bool is_alive_;
    bool is_double_top_;
    bool is_last_;
    std::deque<T_BreakAtomInfo> breakdown_infos_;
    std::deque<T_BreakAtomInfo> breakup_infos_;
    
};

class FuturesForecastApp;
class TrendLineMan
{
public:
    TrendLineMan(FuturesForecastApp &app) 
        : app_(app)/*, down_line_(TrendLine(TrendLineType::DOWN)), up_line_(TrendLine(TrendLineType::UP))*/
        , last_date_(0)
        , last_trend_up_line_(nullptr)
    {
        line_id_ = 0;
    }
    
    
    TrendLine *  CreateTrendUpLine(const std::string &code, TypePeriod type_period, TrendLine *p_last_trend_up_line=nullptr);
    TrendLine *  CreateTrendUpLineByDoubleTop(const std::string &code, TypePeriod type_period, int left_index, int right_index);
    TrendLine *  __CreateTrendUpLine(const std::string &code, TypePeriod type_period, int left_index, int right_index, bool db_top, TrendLine *p_last_trend_up_line=nullptr);

    void  Update(const std::string &code, TypePeriod type_period, int k_index, const T_StockHisDataItem &quote_k);

    int GetLeftEndIndex(const std::string &code, TypePeriod type_period);
    double CaculateMidAxisSlop(const std::string &code, TypePeriod type_period);

    std::deque<std::shared_ptr<TrendLine> > * FindTrendLineContainer(const std::string &code, TypePeriod type_period, TrendLineType trend_line_type);
    TrendLine * FindLastTrendLine(const std::string &code, TypePeriod type_period, TrendLineType trend_line_type);
    TrendLine & GetLastTrendLine(const std::string &code, TypePeriod type_period, TrendLineType trend_line_type);
    TrendLine * FindTrendLine(const std::string &code, TypePeriod type_period, TrendLineType trend_line_type, int line_beg, int line_end);
    
    std::shared_ptr<TrendLine> FindTrendLineById(unsigned int id);

    TrendLine * AppendTrendLine(const std::string &code, TypePeriod type_period, const std::shared_ptr<TrendLine> &trend_line, bool is_use_second_lh = false);

    unsigned int GenerateLineId(){ return ++line_id_; }

    //bool IsDoubleTopShape();
    std::shared_ptr<TrendLine> last_trend_up_line() { return last_trend_up_line_;}
    void last_trend_up_line(const std::shared_ptr<TrendLine> &line)
    {
        if( !line )
        {
            if( last_trend_up_line_ )
            {
                last_trend_up_line_->is_last_ = false;
                last_trend_up_line_ = nullptr;
            }
        }else
        {
            if( last_trend_up_line_ && last_trend_up_line_->id_ != line->id_ )
                last_trend_up_line_->is_last_ = false;
            last_trend_up_line_ = line; 
            last_trend_up_line_->is_last_ = true;
        }
    }
private:
    int SolveTrendUpLineEndIndex(T_HisDataItemContainer &k_datas, int lowest_index, int highest_index, OUT double &rel_slop);

private:
    FuturesForecastApp &app_;
    //TrendLine down_line_;
    //TrendLine up_line_;
    int last_date_;
    std::atomic<unsigned int> line_id_;
    std::shared_ptr<TrendLine> last_trend_up_line_;
    //<code, ...>
    std::unordered_map<std::string, std::unordered_map<TypePeriod, std::unordered_map<TrendLineType, std::deque<std::shared_ptr<TrendLine> > > > > code_type_trend_lines_;
    std::unordered_map<unsigned int, std::shared_ptr<TrendLine> > id_trend_lines_;
};

double TrendLineValue(IN T_HisDataItemContainer &his_data, IN TrendLine &trend_line, int k_index);
bool IsBreakTrendLine(IN T_HisDataItemContainer &his_data, IN TrendLine &trend_line, int k_index, bool is_judge_break_up, double price);

void SortTrendLines(IN T_HisDataItemContainer &his_data, int k_index, INOUT std::deque<std::shared_ptr<TrendLine> > &lines, bool is_asc, INOUT std::vector<double> *p_line_values=nullptr);

#endif // TREND_LINE_H_DSFDSKJK_