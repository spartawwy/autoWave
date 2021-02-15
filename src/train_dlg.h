#ifndef TRAIN_DLG_SDFS23343543_H_
#define TRAIN_DLG_SDFS23343543_H_
 
#include <cassert>
#include <vector>
#include <list>

#include <QVector>
#include <QtWidgets/QWidget>
#include <QStandardItemModel>
#include "ui_traindlg.h"

#include "stkfo_common.h"
#include "position_account.h"

#define ORDER_TYPE_HANGON  1
#define ORDER_TYPE_STOPPROFIT 2
#define ORDER_TYPE_STOPLOSS 3
#define ORDER_TYPE_CONDITION  4
#define ORDER_TYPE_ALL  5
 
class CfgTrainDlg;
class CfgStopProfitLossDlg;
class KLineWall;
class MainWindow;
class QTimer;
class TrainDlg : public QWidget
{
    Q_OBJECT

public:

    TrainDlg(KLineWall *parent, MainWindow *main_win);
     
    AccountInfo & account_info() { return account_info_; }

    const T_StockHisDataItem & CurHisStockDataItem();

public slots:

    void OnStepTimer();

    void DoTblPosDbClick(const QModelIndex &);
    void OnTblHangonOrdersRowDoubleClicked(const QModelIndex &);
    void OnTblConditionsRowDoubleClicked(const QModelIndex&);
    void OnScrollTrainTimeMoved(int);
    void OnSliderStepSpeedChanged();

    void OnRandomStartTrain();
    void OnStartTrain();
    void _OnStartTrain(bool);
    void OnStopTrain();
    
    void OnControl();
    void OnNextStep();
     
    void OnBuy();
    void OnSell(); 
    void OnCloseAllUnfrozenPos();
    void OnShowCfg();

    double cur_quote(){ return cur_kdata_item_.close_price; }

    void SaveStopProfitLoss(std::vector<PositionAtom> &pos_atoms);

    void OnAddConditionOrder();

protected:

    virtual void closeEvent(QCloseEvent *) override;
    //virtual void hideEvent(QHideEvent * event) override;
private:

    int GenerateConditionOrderId(){return ++max_condition_order_id_;}
    int GenerateHangonOrderId(){ return ++max_hangon_order_id_;}
    void PrintTradeRecords();

    bool CheckPosition();
    
    //void OpenPosition(double para_price, bool is_long);
    void OpenPosition(double para_price, unsigned int qty, bool is_long, unsigned int *p_profit_stop_ticks=nullptr, unsigned int *p_loss_stop_ticks=nullptr, bool is_proc_hangon_order=false);
    int ClosePosition(double para_price, unsigned int qty, bool is_long, const T_StockHisDataItem &fake_k_item, QString *p_ret_info=nullptr);
    void CloseInputSizePosition(double para_price, bool is_long, const T_StockHisDataItem &fake_k_item);

    bool AddOpenOrder(double price, unsigned int quantity, bool is_long);
    bool AddCloseOrder(double price, unsigned int quantity, bool is_long);

    std::vector<TradeRecordAtom> DoIfStopProfitLoss(const T_StockHisDataItem &k_item, std::vector<int> &ret_pos_ids, double &ret_profit);
    unsigned int GetTableViewPositionAvailable(bool is_long);
    unsigned int GetItemPositionAllQty(QStandardItemModel& model, int row_index);

    void UpdateOrders2KlineWalls(int type);

    //int QtyInHangonOrderInfo();
    int TblHangonOrdersRowCount();
    
    // UI -----
    void SetMainWinStatusBar(const T_StockHisDataItem &k_item);
    void SetStatusBar(const QString & val)
    {
        ui.lab_status->setText(val);
    }
    void RefreshCapitalUi();
    void RemoveInPositionTableView(int position_id, PositionType position_type);
    void RecaculatePosTableViewItem(QVector<int> &ids, int row_index);
    void RecaculatePosTableViewItem(int row_index);
    double RecaculatePosTableViewFloatProfit(double cur_price);

    void Append2TblHangonOrders(OrderInfo &order_info);
    void RemoveFromTblHangonOrderByFakeId(int fake_id);
    void RemoveFromTblConditionOrderByFakeId(int fake_id);

    void Append2TblTrades(TradeRecordAtom &trade);

private:

    Ui::TrainDlgForm ui;

    KLineWall *parent_;
    MainWindow *main_win_;
    CfgStopProfitLossDlg *cfg_stop_profitloss_dlg_;
    CfgTrainDlg *cfg_train_dlg_;

    AccountInfo  account_info_;
    double ori_capital_;
    double force_close_low_;
    double force_close_high_;

    double fee_rate_;
     
    std::vector<TradeRecordAtom>  trade_records_;
    std::list<OrderInfo> hangon_order_infos_;
    std::list<OrderInfo> stop_profit_order_infos_; 
    std::list<OrderInfo> stop_loss_order_infos_; 
    std::list<OrderInfo> condition_order_infos_; 

    T_DateRange  hisk_date_range_;
    int scroll_bar_date_;

    // auto stop profit/loss related -----
    //void auto_stop_profit(bool val){ auto_stop_profit_ = val; }
    //void auto_stop_loss(bool val){ auto_stop_loss_ = val; }

    bool auto_stop_profit_;
    bool auto_stop_loss_;
    unsigned int auto_stop_profit_ticks_;
    unsigned int auto_stop_loss_ticks_;

    // condition open related -----
    QStandardItemModel *condition_model_;
    int max_hangon_order_id_;
    int max_condition_order_id_;
    //double cur_quote_;
    T_StockHisDataItem cur_kdata_item_;

    QTimer *step_timer_;
    int step_delay_ms_;
    bool is_started_;
    bool is_running_;
    std::mutex stepping_mutex_;

    friend class CfgTrainDlg;
};

#endif // TRAIN_DLG_SDFS23343543_H_