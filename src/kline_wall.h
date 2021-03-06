#ifndef K_LINE_WALL_SDF32DSF_
#define K_LINE_WALL_SDF32DSF_

#include <mutex>
#include <QtWidgets/QWidget>

#include "stock_data_man.h"
#ifdef STK_INPUT_KWALL
#include "stockinput_dlg.h"
#endif
#include "statistic_dlg.h"

#include "forcast_man.h"

#include "zhibiao_window.h"
#include "position_account.h"

//#define DRAW_FROM_LEFT
#ifndef DRAW_FROM_LEFT 
#define DRAW_FROM_RIGHT
#endif

#define  WOKRPLACE_DEFUALT_K_NUM  (4*20 + 100)
#define  DEFAULT_CYCLE_TAG  "����"
 
typedef std::tuple<QPointF, std::string> T_TuplePointStr;

struct  TradeRecordKwallPosInfo
{
    TradeRecordKwallPosInfo():record(nullptr), pos(0.0){}
    TradeRecordKwallPosInfo(const TradeRecordKwallPosInfo&lh):record(lh.record), pos(lh.pos){}
    TradeRecordKwallPosInfo(TradeRecordKwallPosInfo &&lh):record(std::move(lh.record)), pos(lh.pos){}
    TradeRecordKwallPosInfo & operator = (const TradeRecordKwallPosInfo&lh)
    {
        if(this == &lh )return *this;
        record = lh.record; pos = lh.pos;
        return  *this;
    }
    std::shared_ptr<TradeRecordSimple> record;
    double pos;
};

class Ui_KLineWallForm;
class ExchangeCalendar;
class FuturesForecastApp;
class MainWindow;
class DataBase;
class StockMan;
class KLineWall : public QWidget
{
    Q_OBJECT

public:
     
    static const double cst_k_mm_enlarge_times; 
    static const double cst_k_mm_narrow_times; 

    KLineWall(FuturesForecastApp *app, QWidget *parent, int index, TypePeriod k_type);
	~KLineWall() { }
	 
    bool Init(); 

    //void SetCursorShape(Qt::CursorShape& cursor_shapre);
    void draw_action(DrawAction action) {  draw_action_ = action; }
    DrawAction draw_action(){ return draw_action_; }

    void ResetDrawState(DrawAction draw_action);
    void ClearForcastData();
    void SetShowStructLine(bool val);
    void SetShowSection(bool val);

    std::string stock_code() { return stock_code_; }

    void ResetTypePeriod(TypePeriod  type);

    PeriodType ToPeriodType(TypePeriod src);
    
    TypePeriod k_type() { return k_type_; }
    int nmarket() { return nmarket_; }

    bool ResetStock(const QString& code, TypePeriod type_period, bool is_index, int nmarket);
    bool ResetStock(const QString& code, const QString& code_name, TypePeriod type_period, bool is_index, int nmarket);
    bool ResetStock(const QString& code, const QString& code_name, bool is_index, int nmarket)
    {
        return ResetStock(code, code_name, k_type_, is_index, nmarket);
    }

    bool LoadBaseStock(const QString& code, TypePeriod type_period, bool is_index, int nmarket, int start_index, int len);
    bool LoadBaseStock(const QString& code, TypePeriod type_period, bool is_index, int nmarket, T_DateRange &range);

    double GetCurWinKLargetstVol();

    int HeadHeight() { return int(height() * head_h_percent_); }
    int BottomHeight() { return int(height() * bottom_h_percent_); }
     
    void DoIfForcastLineNearbyCursor(QMouseEvent &e);

    void ShowDurationKlines(int date, int hhmm);

    T_StockHisDataItem * UpdateIfNecessary(int target_date, int target_hhmm, T_StockHisDataItem *cur_mainkwall_item=nullptr);
    void UpdateIfNecessary(const T_QuoteData &quote_data);

    const T_StockHisDataItem & CurStockDataItem();

    // for train mode ----------------------
    TypePeriod train_step_type(){ return train_step_type_; } 
    int train_start_date(){ return train_start_date_;}
    int train_end_date(){ return train_end_date_; }

    void ResetTypePeriodTrain(TypePeriod  type, int start_date, int end_date);
    
    T_StockHisDataItem* SetTrainStartDateTime(TypePeriod tp_period, int date, int hhmm);
    T_StockHisDataItem* SetTrainStartEnd(TypePeriod tp_period, int start_date, int star_hhmm, int end_date, int end_hhmm);
    T_StockHisDataItem* SetTrainEndDateTime(TypePeriod tp_period, int date, int hhmm);

    T_StockHisDataItem* SetTrainByDateTime(int date, int hhmm);
    T_StockHisDataItem* SetTrainByRendIndex(int rend_index);
   
    T_StockHisDataItem Train_NextStep();
    void Train_NextStep(T_StockHisDataItem & input_item);
     
    const T_StockHisDataItem & CurTrainStockDataItem();
    T_StockHisDataItem* TrainStockDataItem(int r_index);

    int k_cur_train_date() { return k_cur_train_date_; }
    int k_cur_train_hhmm() { return k_cur_train_hhmm_; }
    int  k_rend_index_for_train() { return  k_rend_index_for_train_; }
    void  k_rend_index_for_train(int val) 
    { 
        k_rend_index_for_train_ = val; 
    }
    //----------------------------------

    void right_clicked_k_date(int date) { right_clicked_k_date_ = date; }
    void right_clicked_k_hhmm(int hhmm){ right_clicked_k_hhmm_ = hhmm; }

    bool CaculateHighLowPriceForHighPeriod(TypePeriod high_type, int k_date, int hhmm, int pre_k_date, int pre_k_hhmm, int r_start, std::tuple<double, double> &re_high_low);

    //------------------
    void Set_Cursor(Qt::CursorShape sp);

    //void IncreaseRendIndex() { ++k_rend_index_; }

    void Emit_UpdateKwall() { emit sigUpdateKwall(); }

    //void LockPaintingMutex() { std::lock locker(painting_mutex_); }

    void UpdateStockQuoteOfTrainMode();
    std::tuple<int, int> NextKTradeDateHHmm();

    void HandleAutoForcast_large();
    void BounceUpAutoForcast(TypePeriod type_period, T_HisDataItemContainer &k_datas, int left_end_index, int right_end_index);
    void BounceDownAutoForcast(TypePeriod type_period, T_HisDataItemContainer &k_datas, int left_end_index, int right_end_index);
    void TrendUpAutoForcast(TypePeriod type_period, T_HisDataItemContainer &k_datas, int left_end_index, int right_end_index);
    void TrendDownAutoForcast(TypePeriod type_period, T_HisDataItemContainer &k_datas, int left_end_index, int right_end_index);
     
    //std::tuple<bool ,int> ProcOldBounceAutoForcast(TypePeriod type_period, T_HisDataItemContainer &k_datas, int left_end_index, int right_end_index, bool is_ab_down);

    ForcastMan*  auto_forcast_man() { return &auto_forcast_man_;}

    std::list<unsigned int> & ave_lines() { return ave_lines_; }

signals:
    void sigUpdateKwall();

public slots:
    void slotOpenRelatedSubKwall(bool);
    void slotUpdateKwall();

protected:

    void paintEvent(QPaintEvent*) override;
    void resizeEvent(QResizeEvent * event) override;
	void mousePressEvent( QMouseEvent * event ) override;
    void mouseReleaseEvent(QMouseEvent * e) override;

    void mouseDoubleClickEvent(QMouseEvent * e)  override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void keyPressEvent(QKeyEvent *e) override;

    void enterEvent(QEvent *e) override;
    void leaveEvent(QEvent *e) override;

private slots:

    void slotOpenStatisticDlg(bool);
    void slotZoominSelect(bool);
     
private: 
      
    bool Reset_Stock(const QString& stock, TypePeriod type_period, bool is_index, int nmarket, int oldest_date);
    
    bool Reset_Stock_Train(const QString& stock, TypePeriod type_period, bool is_index, int nmarket, int start_date, int end_date);
    void Reset_Stock_Train();

    void AppendData();
    T_HisDataItemContainer* AppendPreData(int date, int hhmm);
    T_HisDataItemContainer* AppendDataForTrain(int start_date, int end_date);
    T_HisDataItemContainer* AppendData(int date, int hhmm);
    
     
    void Draw2pDownForcast(QPainter &, const int mm_h, double item_w, ForcastMan &forcast_man);
    void Draw2pUpForcast(QPainter &, const int mm_h, double item_w, ForcastMan &forcast_man);

    void Draw3pDownForcast(QPainter &, const int mm_h, double item_w, ForcastMan &forcast_man);
    void Draw3pUpForcast(QPainter &, const int mm_h, double item_w, ForcastMan &forcast_man);
    void _Draw3pForcast(QPainter &, const int mm_h, double item_w, bool is_down_forward, ForcastMan &forcast_man);

    void DrawBi(QPainter &, const int mm_h);
    void DrawStructLine(QPainter &painter, const int mm_h);
    void DrawWave(QPainter &painter, const int mm_h);
    void DrawSection(QPainter &painter, const int mm_h);
    void DrawTrendLine(QPainter &painter, const int mm_h);

    void UpdatePosDatas();
    void UpdateKwallMinMaxPrice();

    T_KlineDataItem * GetKLineDataItemByXpos(int x);
    QPointF GetPointFromKLineDataItems(int x, bool is_get_top);
    T_KlineDataItem * GetKLineDataItemByDate(int date, int hhmm);
    T_KlinePosData * GetKLinePosDataByDate(int date, int hhmm);

    double get_price_y(double price, int mm_h)
    {  
        return -1 * (price - lowestMinPrice_)/(highestMaxPrice_ - lowestMinPrice_) * mm_h;
    }

    void SetLowestMinPrice(float val) { lowestMinPrice_ = val;}
    double GetLowestMinPrice() { return lowestMinPrice_; }
    void SetHighestMaxPrice(float val) { highestMaxPrice_ = val;}
    double GetHighestMaxPrice() { return highestMaxPrice_; }

    bool GetContainerMaxMinPrice(PeriodType period_type, const std::string& code, int k_num, std::tuple<double, double>& ret, std::tuple<int, int, int, int> &date_times);

    int FindTopItem_TowardLeft(T_HisDataItemContainer &his_data, T_HisDataItemContainer::reverse_iterator iter, int k_index, T_KlinePosData *&left_pos_data);
    int FindTopFakeItem_TowardLeft(T_HisDataItemContainer &his_data, T_HisDataItemContainer::reverse_iterator iter, int k_index, T_KlinePosData *&left_pos_data);
    int FindBtmItem_TowardLeft(T_HisDataItemContainer &his_data, T_HisDataItemContainer::reverse_iterator iter, int k_index, T_KlinePosData *&left_pos_data);
    int FindBtmFakeItem_TowardLeft(T_HisDataItemContainer &his_data, T_HisDataItemContainer::reverse_iterator iter, int k_index, T_KlinePosData *&left_pos_data);

    int Calculate_k_mm_h();
     
    //-------------------------------------------------------
    FuturesForecastApp *app_;
    MainWindow  *main_win_;
	std::shared_ptr<Ui_KLineWallForm>  ui;
    const int wall_index_;

    const double head_h_percent_;
    const double bottom_h_percent_;
     
    int empty_right_w_;
    int right_w_;
    
    int pre_mm_w_;
    int pre_mm_h_;
    int h_axis_trans_in_paint_k_;
#ifdef STK_INPUT_KWALL
	StockInputDlg  stock_input_dlg_;
#endif
    std::string    stock_code_;
    std::string    stock_name_;
    bool           is_index_;
   
	T_HisDataItemContainer *p_hisdata_container_; //point to stock_data_man_'s a stock's data
    
    double lowestMinPrice_;
    double highestMaxPrice_;
    int lowest_price_date_;
    int lowest_price_hhmm_;
    int highest_price_date_;
    int highest_price_hhmm_;

    bool  show_cross_line_;

    int  k_num_;
    volatile int  k_rend_index_;
    int  pre_k_rend_index_;
    int  k_move_temp_index_;
    // train mode rel---------
    int  k_rend_index_for_train_;
    int  k_cur_train_date_;
    int  k_cur_train_hhmm_;
    //-------------------------
    TypePeriod  k_type_;
    int nmarket_;
    std::string  k_cycle_tag_;
    
    int  k_cycle_year_;
    int  date_;
    std::string k_date_time_str_;

    volatile bool is_resetting_stock_;

    volatile DrawAction draw_action_;
 
    QPointF drawing_line_A_;
    QPointF drawing_line_B_;
    QPointF drawing_line_C_;

    QPointF cur_mouse_point_;

    bool mm_move_flag_;
    QPoint move_start_point_;
     
    bool area_select_flag_;
    QPointF area_sel_mouse_release_point_;

    QMenu * k_wall_menu_;
    QMenu * k_wall_menu_sub_;

    // forcast related -----------
    ForcastMan  forcast_man_; 
    T_DataForcast *cur_select_forcast_;

    ForcastMan  auto_forcast_man_;
    
    //---------------------------
    std::vector<std::shared_ptr<ZhibiaoWindow> > zb_windows_;

    bool is_draw_fengxin_;
    bool is_draw_bi_;
    bool is_draw_struct_line_;
    bool is_draw_section_;
    bool is_draw_wave_;
    bool is_draw_trend_line_;

    std::mutex  painting_mutex_;

    int  right_clicked_k_date_; // right mouse click
    int  right_clicked_k_hhmm_; // right mouse click
     
    StatisticDlg  statistic_dlg_;
    // train related ------------
    std::list<OrderInfo> hangon_order_infos_;
    std::list<OrderInfo> stop_profit_order_infos_; 
    std::list<OrderInfo> stop_loss_order_infos_; 
    std::list<OrderInfo> condition_order_infos_; 

    TypePeriod  train_step_type_;
    int train_start_date_;
    int train_end_date_;
    //------------------------
    // <index, vector>
    std::unordered_map<int, std::vector<TradeRecordKwallPosInfo> > trades_pos_;
    //------------------------
    std::list<unsigned int> ave_lines_;
    // <line t, <item index, pos>>
    std::unordered_map<unsigned int, std::unordered_map<unsigned int, double> > ave_lines_pos_;
    friend class ZhibiaoWindow;
    friend class VolZhibiaoWin;
    friend class MomentumZhibiaoWin;
    friend class TrainDlg;
    friend class StockDataMan;
};

int CalculateSpanDays(TypePeriod type_period, int k_count);
// ret: <date, hhmm>
std::tuple<int, int> GetKDataTargetDateTime(ExchangeCalendar &exch_calender, TypePeriod type_period, int end_date, int tmp_hhmm, int max_k_count);
// ret: hhmm
int GetKDataTargetStartTime(TypePeriod type_period, int hhmm);

int FindKRendIndex(T_HisDataItemContainer *p_hisdata_container, int date_val, int hhmm);

int FindKRendIndexInHighPeriodContain(TypePeriod tp_period, T_HisDataItemContainer &p_hisdata_container, ExchangeCalendar &calender, int date_val, int hhmm);
int FindKRendIndexInHighContain_FromRStart2Right(TypePeriod tp_period, T_HisDataItemContainer &p_hisdata_container, ExchangeCalendar &calender, int date_val, int hhmm, int r_start);

int FindStartKRendIndexInLowContain(TypePeriod high_type, T_HisDataItemContainer &container
                                               , int k_date, int hhmm, int pre_k_date, int pre_k_hhmm, int r_start);

#endif // K_LINE_WALL_SDF32DSF_