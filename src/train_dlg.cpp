#include "train_dlg.h"

#include <QtWidgets/QComboBox>
#include <QMessageBox>
#include <QStatusBar>
#include <QTimer>
#include <QDebug>

#include <TLib/core/tsystem_utility_functions.h>

#include "config_man.h"
#include "database.h"
#include "exchange_calendar.h"

#include "futures_forecast_app.h"
#include "mainwindow.h"
#include "tool_bar.h"
#include "kline_wall.h"
#include "cfg_stop_profitloss_dlg.h"
#include "cfg_train_dlg.h"

#include "position_account.h"

static const int cst_detail_page_positions = 0;
static const int cst_detail_page_trades = 1;
static const int cst_detail_page_conditions = 2;

static const int cst_min_step_delay_ms = 500;
static const int cst_max_step_delay_ms = 10000;
static const int cst_default_step_delay_ms = 5000;
 
static const int cst_small_width = 60;

static const int cst_tbview_position_id = 0;
static const int cst_tbview_position_average_price = 1;
static const int cst_tbview_position_float_profit = 2;
static const int cst_tbview_position_bs = 3;
static const int cst_tbview_position_size = 4;
static const int cst_tbview_position_avaliable = 5;
static const int cst_tbview_position_filled_time = 6;
static const int cst_tbview_position_col_count = 7;

static const int cst_tbview_hangonorder_id = 0;
static const int cst_tbview_hangonorder_bs = 1;
static const int cst_tbview_hangonorder_oc = 2;
static const int cst_tbview_hangonorder_price = 3;
static const int cst_tbview_hangonorder_qty = 4;
static const int cst_tbview_hangonorder_col_count = 5;

static const int cst_tbview_trades_id = 0;
static const int cst_tbview_trades_bs = 1;
static const int cst_tbview_trades_oc = 2;
static const int cst_tbview_trades_price = 3;
static const int cst_tbview_trades_qty = 4;
static const int cst_tbview_trades_profit = 5;
static const int cst_tbview_trades_fee = 6;
static const int cst_tbview_trades_time = 7;
static const int cst_tbview_trades_col_count = 8;

static const int cst_tbview_condition_id = 0;
static const int cst_tbview_condition_bs = 1;
static const int cst_tbview_condition_compare_type = 2;
static const int cst_tbview_condition_price = 3;
static const int cst_tbview_condition_qty = 4;
static const int cst_tbview_condition_stop_profit = 5;
static const int cst_tbview_condition_stop_loss = 6;
static const int cst_tbview_condition_col_count = 7;

static int find_model_first_fit_index(QStandardItemModel& model, int col_index, bool is_long_pos);
static std::tuple<double, unsigned int> get_total_amount_qty(PositionInfo &position, QVector<int> &ids);

static bool erase_rel_item_by_pos_ids(std::list<OrderInfo> &order_info, std::list<OrderInfo>::iterator &order_item, std::vector<int> &target_del_ids );
static bool erase_rel_item_by_fake_ids(std::list<OrderInfo> &order_info, std::list<OrderInfo>::iterator &order_item, std::vector<int> &target_del_ids );
static void remove_container_items_by_pos_ids(std::list<OrderInfo> &container, std::vector<int> *long_deleted_ids, std::vector<int> *short_deleted_ids);

//////////////////////////////////////////
TrainDlg::TrainDlg(KLineWall *parent,  MainWindow *main_win)
    : QWidget(nullptr)
    , parent_(parent)
    , main_win_(main_win)
    , cfg_stop_profitloss_dlg_(nullptr)
    , cfg_train_dlg_(nullptr)
    , account_info_()
    , ori_capital_(cst_default_ori_capital)
    , force_close_low_(MAX_PRICE)
    , force_close_high_(MIN_PRICE) 
    , scroll_bar_date_(0)
    , auto_stop_profit_(true)
    , auto_stop_loss_(true)
    , auto_stop_profit_ticks_(10)
    , auto_stop_loss_ticks_(10)
    , condition_model_(nullptr)
    , max_hangon_order_id_(0)
    , max_condition_order_id_(0)
    , step_timer_(nullptr)
    , step_delay_ms_(cst_default_step_delay_ms)
    , is_started_(false)
    , is_running_(false)
{
    ui.setupUi(this);

    cfg_stop_profitloss_dlg_ = new CfgStopProfitLossDlg(this);
    cfg_stop_profitloss_dlg_->setWindowFlags(cfg_stop_profitloss_dlg_->windowFlags() | Qt::WindowStaysOnTopHint/*Qt::Dialog*/ );
    cfg_stop_profitloss_dlg_->hide();
     
    cfg_train_dlg_ = new CfgTrainDlg(this);
    cfg_train_dlg_->setWindowFlags(cfg_stop_profitloss_dlg_->windowFlags() | Qt::WindowStaysOnTopHint/*Qt::Dialog*/ );
    cfg_train_dlg_->hide();

    bool ret = false;
    ret = connect(ui.pbtnStart, SIGNAL(clicked()), this, SLOT(OnStartTrain()));
    assert(ret);
    ret = connect(ui.pbtnRandomStart, SIGNAL(clicked()), this, SLOT(OnRandomStartTrain()));
    assert(ret);
    ret = connect(ui.pbtnStop, SIGNAL(clicked()), this, SLOT(OnStopTrain()));
    assert(ret);
    ret = connect(ui.pbtnControl, SIGNAL(clicked()), this, SLOT(OnControl()));
    assert(ret);
    ret = connect(ui.pbtn_buy, SIGNAL(clicked()), this, SLOT(OnBuy()));
    ret = connect(ui.pbtn_sell, SIGNAL(clicked()), this, SLOT(OnSell()));
    assert(ret);
    ret = connect(ui.pbtn_config,  SIGNAL(clicked()), this, SLOT(OnShowCfg()));
    assert(ret);
    // set his k date range info----------------
    T_DateRange  date_rage_5m;
    bool result = main_win_->app_->data_base()->GetHisKBarDateRange(DEFAULT_CODE, false, TypePeriod::PERIOD_5M, date_rage_5m);
    if( !result )
        QMessageBox::information(nullptr, "erro", "there is no history k date of 5m");
    int eldest_date_5m = std::get<0>(date_rage_5m);
    int eldest_time_5m = std::get<1>(date_rage_5m);
    int latest_date_5m = std::get<2>(date_rage_5m);
    int latest_time_5m = std::get<3>(date_rage_5m);
    T_DateRange  date_rage_1m;
    result = main_win_->app_->data_base()->GetHisKBarDateRange(DEFAULT_CODE, false, TypePeriod::PERIOD_1M, date_rage_1m);
    if( !result )
        QMessageBox::information(nullptr, "erro", "there is no history k date of 1m");
    int eldest_date_1m = std::get<0>(date_rage_1m);
    int eldest_time_1m = std::get<1>(date_rage_1m);
    int latest_date_1m = std::get<2>(date_rage_1m);
    int latest_time_1m = std::get<3>(date_rage_1m);

    int eldest_date = std::max(eldest_date_5m, eldest_date_1m);
    int eldest_time = 0; 
    if( eldest_date_5m == eldest_date_1m )
        eldest_time = std::max(eldest_time_5m, eldest_time_1m);
    else 
        eldest_time = (eldest_date == eldest_date_5m ? eldest_time_5m : eldest_time_1m);

    int latest_date = std::min(latest_date_5m, latest_date_1m);
    int latest_time = 0;
    if( latest_date_5m == latest_date_1m )
        latest_time = std::min(latest_time_5m, latest_time_1m);
    else
        latest_time = (latest_date == latest_date_5m ? latest_time_5m : latest_time_1m);

    hisk_date_range_ = std::make_tuple(eldest_date, eldest_time, latest_date, latest_time);
    
    //----------------------------------------
    ui.hScrollBar_TrainTimeRange->setMinimum(eldest_date);
    ui.hScrollBar_TrainTimeRange->setMaximum(latest_date);
    ui.hScrollBar_TrainTimeRange->setValue(latest_date);
    int distan_days = main_win_->app_->exchange_calendar()->DateTradingSpan(eldest_date, latest_date);
    ui.hScrollBar_TrainTimeRange->setSingleStep((latest_date-eldest_date)/distan_days);
#if 1  // temp for debug
    ui.lab_start_date->setText(QString::number(20200305));//ndedt
    ui.hScrollBar_TrainTimeRange->setValue(20200305);
#endif
    scroll_bar_date_ = eldest_date;
    ret = connect(ui.hScrollBar_TrainTimeRange, SIGNAL(sliderMoved(int)), this, SLOT(OnScrollTrainTimeMoved(int)));
    assert(ret);
    //----------------------------------------- 
    ui.vslider_step_speed->setMinimum(cst_min_step_delay_ms);
    ui.vslider_step_speed->setMaximum(cst_max_step_delay_ms);
    assert(cst_max_step_delay_ms > cst_default_step_delay_ms);
    ui.vslider_step_speed->setValue(cst_max_step_delay_ms - cst_default_step_delay_ms); 
    ret = connect(ui.vslider_step_speed, SIGNAL(sliderReleased()), this, SLOT(OnSliderStepSpeedChanged()));
    assert(ret);
    //-----------------------------------------
    ret = connect(ui.pbtn_market_price_c, SIGNAL(clicked()), this, SLOT(OnCloseAllUnfrozenPos()));
    assert(ret);
    //------------------table position
    ui.table_view_position->setEditTriggers(QAbstractItemView::NoEditTriggers);
    
    QStandardItemModel * model = new QStandardItemModel(0, cst_tbview_position_col_count, this);
    model->setHorizontalHeaderItem(cst_tbview_position_id, new QStandardItem(QString::fromLocal8Bit("ID")));
    model->horizontalHeaderItem(cst_tbview_position_id)->setTextAlignment(Qt::AlignCenter);
    model->setHorizontalHeaderItem(cst_tbview_position_bs, new QStandardItem(QString::fromLocal8Bit("买卖")));
    model->horizontalHeaderItem(cst_tbview_position_bs)->setTextAlignment(Qt::AlignCenter);
    model->setHorizontalHeaderItem(cst_tbview_position_size, new QStandardItem(QString::fromLocal8Bit("持仓")));
    model->horizontalHeaderItem(cst_tbview_position_size)->setTextAlignment(Qt::AlignCenter);
    model->setHorizontalHeaderItem(cst_tbview_position_avaliable, new QStandardItem(QString::fromLocal8Bit("可用")));
    model->horizontalHeaderItem(cst_tbview_position_avaliable)->setTextAlignment(Qt::AlignCenter);
    model->setHorizontalHeaderItem(cst_tbview_position_average_price, new QStandardItem(QString::fromLocal8Bit("均价")));
    model->horizontalHeaderItem(cst_tbview_position_average_price)->setTextAlignment(Qt::AlignCenter);
    model->setHorizontalHeaderItem(cst_tbview_position_float_profit, new QStandardItem(QString::fromLocal8Bit("浮赢")));
    model->horizontalHeaderItem(cst_tbview_position_float_profit)->setTextAlignment(Qt::AlignCenter);
    model->setHorizontalHeaderItem(cst_tbview_position_filled_time, new QStandardItem(QString::fromLocal8Bit("时间")));
    model->horizontalHeaderItem(cst_tbview_position_filled_time)->setTextAlignment(Qt::AlignCenter);
    ui.table_view_position->setModel(model);
    ui.table_view_position->setColumnWidth(cst_tbview_position_id, cst_small_width);
    ui.table_view_position->setColumnWidth(cst_tbview_position_average_price, cst_small_width);
    ui.table_view_position->setColumnWidth(cst_tbview_position_float_profit, cst_small_width);
    ui.table_view_position->setColumnWidth(cst_tbview_position_bs, cst_small_width/2);
    ui.table_view_position->setColumnWidth(cst_tbview_position_size, cst_small_width);
    ui.table_view_position->setColumnWidth(cst_tbview_position_avaliable, cst_small_width);
    ret = connect(ui.table_view_position, SIGNAL(doubleClicked(const QModelIndex&)), this, SLOT(DoTblPosDbClick(const QModelIndex&)));
    assert(ret);
    //--------------------table hangon order------ 
     
    ui.table_view_order_hangon->setEditTriggers(QAbstractItemView::NoEditTriggers);
    
    model = new QStandardItemModel(0, cst_tbview_hangonorder_col_count, this);
    model->setHorizontalHeaderItem(cst_tbview_hangonorder_id, new QStandardItem(QString::fromLocal8Bit("ID")));
    model->horizontalHeaderItem(cst_tbview_hangonorder_id)->setTextAlignment(Qt::AlignCenter);
    model->setHorizontalHeaderItem(cst_tbview_hangonorder_bs, new QStandardItem(QString::fromLocal8Bit("买卖")));
    model->horizontalHeaderItem(cst_tbview_hangonorder_bs)->setTextAlignment(Qt::AlignCenter);
    model->setHorizontalHeaderItem(cst_tbview_hangonorder_oc, new QStandardItem(QString::fromLocal8Bit("开平")));
    model->horizontalHeaderItem(cst_tbview_hangonorder_oc)->setTextAlignment(Qt::AlignCenter);
    model->setHorizontalHeaderItem(cst_tbview_hangonorder_price, new QStandardItem(QString::fromLocal8Bit("价格")));
    model->horizontalHeaderItem(cst_tbview_hangonorder_price)->setTextAlignment(Qt::AlignCenter);
    model->setHorizontalHeaderItem(cst_tbview_hangonorder_qty, new QStandardItem(QString::fromLocal8Bit("数量")));
    model->horizontalHeaderItem(cst_tbview_hangonorder_qty)->setTextAlignment(Qt::AlignCenter);
    ui.table_view_order_hangon->setModel(model);
    ui.table_view_order_hangon->setColumnWidth(cst_tbview_hangonorder_id, cst_small_width/2);
    ui.table_view_order_hangon->setColumnWidth(cst_tbview_hangonorder_bs, cst_small_width/2);
    ui.table_view_order_hangon->setColumnWidth(cst_tbview_hangonorder_oc, cst_small_width/2);
    ui.table_view_order_hangon->setColumnWidth(cst_tbview_hangonorder_qty, cst_small_width);
    ui.table_view_order_hangon->setColumnWidth(cst_tbview_hangonorder_price, cst_small_width);
    ret = connect(ui.table_view_order_hangon, SIGNAL(doubleClicked(const QModelIndex&)), this, SLOT(OnTblHangonOrdersRowDoubleClicked(const QModelIndex&)));
    assert(ret);
    //------------------table trades----------
    ui.table_view_trades->setEditTriggers(QAbstractItemView::NoEditTriggers);
    model = new QStandardItemModel(0, cst_tbview_trades_col_count, this);
    model->setHorizontalHeaderItem(cst_tbview_trades_id, new QStandardItem(QString::fromLocal8Bit("ID")));
    model->horizontalHeaderItem(cst_tbview_trades_id)->setTextAlignment(Qt::AlignCenter);
    model->setHorizontalHeaderItem(cst_tbview_trades_bs, new QStandardItem(QString::fromLocal8Bit("买卖")));
    model->horizontalHeaderItem(cst_tbview_trades_bs)->setTextAlignment(Qt::AlignCenter);
    model->setHorizontalHeaderItem(cst_tbview_trades_oc, new QStandardItem(QString::fromLocal8Bit("开平")));
    model->horizontalHeaderItem(cst_tbview_trades_oc)->setTextAlignment(Qt::AlignCenter);
    model->setHorizontalHeaderItem(cst_tbview_trades_price, new QStandardItem(QString::fromLocal8Bit("价格")));
    model->horizontalHeaderItem(cst_tbview_trades_price)->setTextAlignment(Qt::AlignCenter);
    model->setHorizontalHeaderItem(cst_tbview_trades_qty, new QStandardItem(QString::fromLocal8Bit("数量")));
    model->horizontalHeaderItem(cst_tbview_trades_qty)->setTextAlignment(Qt::AlignCenter);
    model->setHorizontalHeaderItem(cst_tbview_trades_profit, new QStandardItem(QString::fromLocal8Bit("盈亏")));
    model->horizontalHeaderItem(cst_tbview_trades_profit)->setTextAlignment(Qt::AlignCenter);
    model->setHorizontalHeaderItem(cst_tbview_trades_fee, new QStandardItem(QString::fromLocal8Bit("手续费")));
    model->horizontalHeaderItem(cst_tbview_trades_fee)->setTextAlignment(Qt::AlignCenter);
    model->setHorizontalHeaderItem(cst_tbview_trades_time, new QStandardItem(QString::fromLocal8Bit("时间")));
    model->horizontalHeaderItem(cst_tbview_trades_time)->setTextAlignment(Qt::AlignCenter);
    ui.table_view_trades->setModel(model);
    ui.table_view_trades->setColumnWidth(cst_tbview_trades_id, cst_small_width/2);
    ui.table_view_trades->setColumnWidth(cst_tbview_trades_bs, cst_small_width/2);
    ui.table_view_trades->setColumnWidth(cst_tbview_trades_oc, cst_small_width/2);
    ui.table_view_trades->setColumnWidth(cst_tbview_trades_price, cst_small_width);
    ui.table_view_trades->setColumnWidth(cst_tbview_trades_qty, cst_small_width);
    ui.table_view_trades->setColumnWidth(cst_tbview_trades_profit, cst_small_width);
    ui.table_view_trades->setColumnWidth(cst_tbview_trades_fee, cst_small_width);
    ui.table_view_trades->setColumnWidth(cst_tbview_trades_time, cst_small_width);

    //------------------condition orders ----------
    ui.table_view_condition->setEditTriggers(QAbstractItemView::NoEditTriggers);
    model = new QStandardItemModel(0, cst_tbview_condition_col_count, this);
    model->setHorizontalHeaderItem(cst_tbview_condition_id, new QStandardItem(QString::fromLocal8Bit("ID")));
    model->horizontalHeaderItem(cst_tbview_condition_id)->setTextAlignment(Qt::AlignCenter);

    model->setHorizontalHeaderItem(cst_tbview_condition_bs, new QStandardItem(QString::fromLocal8Bit("买卖")));
    model->horizontalHeaderItem(cst_tbview_condition_bs)->setTextAlignment(Qt::AlignCenter);
      
    model->setHorizontalHeaderItem(cst_tbview_condition_qty, new QStandardItem(QString::fromLocal8Bit("数量")));
    model->horizontalHeaderItem(cst_tbview_condition_qty)->setTextAlignment(Qt::AlignCenter);

    model->setHorizontalHeaderItem(cst_tbview_condition_compare_type, new QStandardItem(QString::fromLocal8Bit("条件")));
    model->horizontalHeaderItem(cst_tbview_condition_compare_type)->setTextAlignment(Qt::AlignCenter);

    model->setHorizontalHeaderItem(cst_tbview_condition_price, new QStandardItem(QString::fromLocal8Bit("价格")));
    model->horizontalHeaderItem(cst_tbview_condition_price)->setTextAlignment(Qt::AlignCenter);

    model->setHorizontalHeaderItem(cst_tbview_condition_stop_profit, new QStandardItem(QString::fromLocal8Bit("止赢")));
    model->horizontalHeaderItem(cst_tbview_condition_stop_profit)->setTextAlignment(Qt::AlignCenter);

    model->setHorizontalHeaderItem(cst_tbview_condition_stop_loss, new QStandardItem(QString::fromLocal8Bit("止损")));
    model->horizontalHeaderItem(cst_tbview_condition_stop_loss)->setTextAlignment(Qt::AlignCenter);
    ui.table_view_condition->setModel(model);
    ui.table_view_condition->setColumnWidth(cst_tbview_condition_id, cst_small_width/2);
    ui.table_view_condition->setColumnWidth(cst_tbview_condition_bs, cst_small_width/2);
    ui.table_view_condition->setColumnWidth(cst_tbview_condition_qty, cst_small_width/2);
    ui.table_view_condition->setColumnWidth(cst_tbview_condition_compare_type, cst_small_width/2);
    ui.table_view_condition->setColumnWidth(cst_tbview_condition_price, cst_small_width);
    ui.table_view_condition->setColumnWidth(cst_tbview_condition_stop_profit, cst_small_width/2);
    ui.table_view_condition->setColumnWidth(cst_tbview_condition_stop_loss, cst_small_width/2); 
     
    condition_model_ = model;
    ret = connect(ui.table_view_condition, SIGNAL(doubleClicked(const QModelIndex&)), this, SLOT(OnTblConditionsRowDoubleClicked(const QModelIndex&)));
    assert(ret);
    ret = connect(ui.pbtn_add_condition, SIGNAL(clicked()), this, SLOT(OnAddConditionOrder()));
    assert(ret);

    ui.cmb_conditioin_compare_char->addItem(">=");
    ui.cmb_conditioin_compare_char->addItem("<="); 
    ui.cmb_condition_bs->addItem(QString::fromLocal8Bit("买"), QVariant(true));
    ui.cmb_condition_bs->addItem(QString::fromLocal8Bit("卖"), QVariant(false));
    // timer -----
    step_timer_ = new QTimer(this); 
    ret = connect(step_timer_, SIGNAL(timeout()), this, SLOT(OnStepTimer()));
    assert(ret);

    OnStopTrain();

    account_info_.capital.avaliable = ori_capital_;
    account_info_.capital.frozen = 0.0;
    account_info_.capital.float_profit = 0.0;
    RefreshCapitalUi();
    SetStatusBar("");
}

const T_StockHisDataItem & TrainDlg::CurHisStockDataItem()
{
    return parent_->CurTrainStockDataItem();
}
               
void TrainDlg::OnStepTimer()
{
    if( std::try_lock(stepping_mutex_) )
        OnNextStep();
    stepping_mutex_.unlock();
}

void TrainDlg::DoTblPosDbClick(const QModelIndex &index)
{
    assert(cfg_stop_profitloss_dlg_);

    QStandardItemModel * model = static_cast<QStandardItemModel *>(ui.table_view_position->model());

    std::vector<std::shared_ptr<PositionAtom>> rel_pos_atoms;  
    QVector<int> pos_ids = model->item(index.row(), cst_tbview_position_id)->data().value<QVector<int>>();
    for( int i = 0; i < pos_ids.size(); ++i )
    {
        std::shared_ptr<PositionAtom> pos_atom = account_info_.position.FindPositionAtomSharedPointer(pos_ids[i]);
        if( !pos_atom || pos_atom->qty_available == 0 )
            continue;
        rel_pos_atoms.push_back(pos_atom);
    }
    cfg_stop_profitloss_dlg_->SetContent(rel_pos_atoms);
    cfg_stop_profitloss_dlg_->showNormal();
}

void TrainDlg::OnTblHangonOrdersRowDoubleClicked(const QModelIndex &index)
{
    QStandardItemModel * tbl_view_pos_model = static_cast<QStandardItemModel *>(ui.table_view_position->model());
    auto model = (QStandardItemModel*)ui.table_view_order_hangon->model();
    auto temp_order_id = model->item(index.row(), cst_tbview_hangonorder_id)->text().toInt();
    auto price = model->item(index.row(), cst_tbview_hangonorder_price)->text().toDouble();
    // cancel related order
    for( auto iter = hangon_order_infos_.begin(); iter != hangon_order_infos_.end(); )
    {
        if( iter->fake_id == temp_order_id )
        {
            if( iter->action == OrderAction::OPEN )
            {
                auto capital = CaculateOpenPositionFreezeCapital(price, iter->qty);
                assert(!(account_info_.capital.frozen < capital));
                account_info_.capital.frozen -= capital;
                account_info_.capital.avaliable += capital;
                RefreshCapitalUi(); 
            }else  // close
            {
                // unfroze related positions 
                for(auto item = iter->help_contain.begin(); item != iter->help_contain.end(); ++item )
                { 
                    auto pos_item = account_info_.position.FindPositionAtom(item->first);
                    if( pos_item )
                    {
                        auto iter_frozen_party = pos_item->qty_frozens.find(temp_order_id);
                        if( iter_frozen_party != pos_item->qty_frozens.end() )
                        {
                            pos_item->qty_available += iter_frozen_party->second;
                            pos_item->qty_frozens.erase(iter_frozen_party);
                        }
                    }
                }
                auto row_index = find_model_first_fit_index(*tbl_view_pos_model, cst_tbview_position_bs, iter->position_type == PositionType::POS_LONG);
                RecaculatePosTableViewItem(row_index);
            }
            hangon_order_infos_.erase(iter++);
            break;
        }
        ++iter;
    }
    model->removeRow(index.row());
     
    UpdateOrders2KlineWalls(ORDER_TYPE_HANGON);
}

void TrainDlg::OnTblConditionsRowDoubleClicked(const QModelIndex&index)
{ 
    auto temp_order_id = condition_model_->item(index.row(), cst_tbview_condition_id)->text().toInt();
    for( auto iter = condition_order_infos_.begin(); iter != condition_order_infos_.end(); )
    {
        if( iter->fake_id == temp_order_id )
        { 
            condition_order_infos_.erase(iter++);
            break;
        }
        ++iter;
    }
    condition_model_->removeRow(index.row());

    UpdateOrders2KlineWalls(ORDER_TYPE_CONDITION);
}

void TrainDlg::OnScrollTrainTimeMoved(int val)
{
    int distance = val - ui.hScrollBar_TrainTimeRange->minimum();
    int dis_trade_days = distance / ui.hScrollBar_TrainTimeRange->singleStep(); 
    scroll_bar_date_ = main_win_->app_->exchange_calendar()->NextTradeDate(ui.hScrollBar_TrainTimeRange->minimum(), dis_trade_days);
    ui.lab_start_date->setText(QString::number(scroll_bar_date_));
}

void TrainDlg::OnSliderStepSpeedChanged()
{
    step_timer_->stop();
    step_delay_ms_ = cst_max_step_delay_ms - ui.vslider_step_speed->value();
    step_timer_->start(step_delay_ms_);
}

void TrainDlg::closeEvent(QCloseEvent * event)
{
    this->hide();
    auto ret = QMessageBox::information(nullptr, QString::fromLocal8Bit("提示"), QString::fromLocal8Bit("是否退出训练?"), QMessageBox::Yes, QMessageBox::No); 
    if( ret == QMessageBox::No )
    { 
        event->ignore();
        this->showNormal();
        return;
    }
    main_win_->is_train_mode(false);
    parent_->k_rend_index_for_train(0);
    if( main_win_->SubKlineWall() )
        main_win_->SubKlineWall()->k_rend_index_for_train(0);
    main_win_->tool_bar()->main_cycle_comb()->setEnabled(true);

    is_started_ = false;

    OnStopTrain();
}
 
void TrainDlg::OnRandomStartTrain()
{
    _OnStartTrain(true);
}

void TrainDlg::OnStartTrain()
{
    _OnStartTrain(false);
}

void TrainDlg::_OnStartTrain(bool is_random_start)
{ 
    is_started_ = true;
    ui.pbtnStart->setEnabled(false);
    ui.pbtnRandomStart->setEnabled(false);
    ui.pbtnStop->setEnabled(true);
     
    ui.pbtnStart->setText(QString::fromLocal8Bit("数据加载中..."));
#if 1 
    trade_records_.clear();
    auto trades_model = (QStandardItemModel*)ui.table_view_trades->model();
    trades_model->setRowCount(0); 
    cfg_train_dlg_->EnableBegCapitalCfg(false);
    //ori_capital_ = cfg_train_dlg_->ori_capital_value();
    account_info_.capital.avaliable = ori_capital_;
    account_info_.capital.frozen = 0.0;
    account_info_.capital.float_profit = 0.0;
    RefreshCapitalUi();

    ui.le_cur_capital->setText(ToQString(account_info_.capital.avaliable + account_info_.capital.frozen));
    ui.lab_assets->setText(ToQString(account_info_.capital.avaliable + account_info_.capital.frozen));
    
    ui.plain_te_record->clear();
     
    /*if( is_started_ )
    {   
         this->hide();
         auto ret = QMessageBox::information(nullptr, QString::fromLocal8Bit("提示"), QString::fromLocal8Bit("是否重新开始训练?"), QMessageBox::Yes, QMessageBox::No); 
         if( ret == QMessageBox::No )
         {
             this->showNormal();
             return;
         }
         this->showNormal();
    }*/
#endif 
    assert(main_win_->OriStepKlineWall());
    assert(main_win_->SubKlineWall());
    
    main_win_->app_->ClearStockHisDatas(parent_->stock_code());
    main_win_->OriStepKlineWall()->Reset_Stock_Train();
    parent_->Reset_Stock_Train();
    main_win_->SubKlineWall()->Reset_Stock_Train();

    const int max_end_date = std::get<2>(hisk_date_range_);
    int start_date = ui.lab_start_date->text().toInt(); 
    
    if( is_random_start )
    {
        std::srand(time(nullptr));
        int day_span = main_win_->app_->exchange_calendar()->DateTradingSpan(std::get<0>(hisk_date_range_), std::get<2>(hisk_date_range_));
        start_date = main_win_->app_->exchange_calendar()->NextTradeDate(std::get<0>(hisk_date_range_), std::rand() % day_span);
    }
    //start_date = 20180817; // for debug
    int start_time = 905; //2340; //
    int end_date = main_win_->app_->exchange_calendar()->NextTradeDate(start_date, DEFAULT_TRAIN_DAYS);
    assert(end_date > MIN_TRADE_DATE);
    if( end_date > max_end_date )
        end_date = max_end_date;
    int end_time = DEFAULT_TRAIN_END_HHMM;
    // ori k wall ----------------
    auto p_ori_wall_item = main_win_->OriStepKlineWall()->SetTrainStartEnd(DEFAULT_ORI_STEP_TYPE_PERIOD, start_date, start_time, end_date, end_time);
    //main_win_->OriStepKlineWall()->SetTrainEndDateTime(DEFAULT_ORI_STEP_TYPE_PERIOD, end_date, end_time);
    // main k wall ----------------
    TypePeriod main_type = TypePeriod(main_win_->tool_bar_->main_cycle_comb()->currentData().toInt());
    T_StockHisDataItem *start_item = parent_->SetTrainStartEnd(main_type, start_date, start_time, end_date, end_time);
    if( start_item && main_type > DEFAULT_ORI_STEP_TYPE_PERIOD )
    {
        T_StockHisDataItem* p_pre_item = parent_->TrainStockDataItem(parent_->k_rend_index_for_train() + 1);
        if( start_item && p_pre_item )
        {
            std::tuple<double, double> high_low;
            bool ret = main_win_->OriStepKlineWall()->CaculateHighLowPriceForHighPeriod(main_type, start_item->date, start_item->hhmmss, p_pre_item->date, p_pre_item->hhmmss
                            , main_win_->OriStepKlineWall()->k_rend_index_for_train(), high_low);
            if( ret )
            {
            start_item->high_price = std::get<0>(high_low);
            start_item->low_price = std::get<1>(high_low);
            }
        }
    } 

    // sub k wall ----------------
    TypePeriod sub_type = TypePeriod(main_win_->tool_bar_->sub_cycle_comb()->currentData().toInt());
    main_win_->SubKlineWall()->SetTrainStartEnd(sub_type, start_date, start_time, end_date, end_time);

    main_win_->SubKlineWall()->setVisible(true);
    main_win_->tool_bar()->SetShowSubKwallBtn(true);

    if( p_ori_wall_item )
        SetMainWinStatusBar(*p_ori_wall_item);

    ui.pbtnStart->setText(QString::fromLocal8Bit("开始"));
    ui.pbtnControl->setText(QString::fromLocal8Bit("运行"));
    ui.pbtnControl->setEnabled(true);
}

void TrainDlg::OnStopTrain()
{
    assert(step_timer_);
    is_started_ = false;

    cfg_train_dlg_->EnableBegCapitalCfg(true);

    step_timer_->stop();
    is_running_ = false;
    ui.pbtnControl->setText(QString::fromLocal8Bit("运行"));
    ui.pbtnControl->setEnabled(false);

    ui.pbtnStart->setText(QString::fromLocal8Bit("开始"));
    ui.pbtnStart->setEnabled(true);
    ui.pbtnRandomStart->setEnabled(true);
    ui.pbtnStop->setEnabled(false);

    force_close_low_ = MAX_PRICE;
    force_close_high_ = MIN_PRICE;

    ui.le_cur_capital->setText(ToQString(account_info_.capital.avaliable));
    ui.lab_assets->setText(ToQString(account_info_.capital.avaliable));
}
 
void TrainDlg::OnCloseAllUnfrozenPos()
{
    int total_closed = 0;
    QString ret_info;
    T_StockHisDataItem fake_item = cur_kdata_item_;
    const unsigned int long_avaliable = account_info_.position.LongPosQty(POSITION_STATUS_AVAILABLE);
    const unsigned int tblview_long_avaliable = GetTableViewPositionAvailable(true);
    if( long_avaliable != tblview_long_avaliable )
    {
        SetStatusBar(QString::fromLocal8Bit("多头可用仓位异常:视图仓位与后台仓位不符!"));
        return;
    }
    const unsigned int short_avaliable = account_info_.position.ShortPosQty(POSITION_STATUS_AVAILABLE);
    const unsigned int tblview_short_avaliable = GetTableViewPositionAvailable(false);
    if( short_avaliable != tblview_short_avaliable )
    {
        SetStatusBar(QString::fromLocal8Bit("空头可用仓位异常:视图仓位与后台仓位不符!"));
        return;
    }
    int ret = ClosePosition(cur_quote(), long_avaliable, true, fake_item, &ret_info);
    if( !ret_info.isEmpty() )
        SetStatusBar(ret_info);
    if( ret > 0 )
        total_closed += long_avaliable;
    
    int ret1 = ClosePosition(cur_quote(), short_avaliable, false, fake_item, &ret_info);
    if( !ret_info.isEmpty() )
        SetStatusBar(ret_info);
    if( ret1 > 0 )
        total_closed += short_avaliable;
    if( total_closed > 0 )
    {
        ui.tab_detail->setCurrentIndex(cst_detail_page_trades);
        RefreshCapitalUi();
    }
}

void TrainDlg::OnShowCfg()
{
    cfg_train_dlg_->showNormal();
}

//ps: auto ajust account_info_.capital
std::vector<TradeRecordAtom> TrainDlg::DoIfStopProfitLoss(const T_StockHisDataItem &k_item, std::vector<int> &ret_pos_ids, double &ret_profit)
{
    std::vector<TradeRecordAtom>  records;

    if( account_info_.position.TotalPosition() == 0 )
        return records;
    int date = 0;
    int time = 0;
    double capital_ret_stop_profit_long  = 0.0;
    double capital_ret_stop_loss_short   = 0.0;
    double capital_ret_stop_profit_short = 0.0;
    double capital_ret_stop_loss_long    = 0.0;
    if( fabs(k_item.close_price - k_item.high_price) > fabs(k_item.close_price - k_item.low_price) ) 
    {
        // close price is nearby low price: first high price then low price
        double profit_long_pos = 0.0;
        std::vector<int> stop_profit_long_ids;
        std::vector<TradeRecordAtom> trades_stop_profit_long = account_info_.position.DoIfStopProfitLongPos(k_item, capital_ret_stop_profit_long, stop_profit_long_ids, nullptr, &profit_long_pos);
        records.insert(records.end(), trades_stop_profit_long.begin(), trades_stop_profit_long.end());
        ret_pos_ids.insert(ret_pos_ids.end(), stop_profit_long_ids.begin(), stop_profit_long_ids.end());

        double loss_short_pos = 0.0;
        std::vector<int> stop_loss_short_ids;
        std::vector<TradeRecordAtom> trades_stop_loss_short = account_info_.position.DoIfStopLossShortPos(k_item, capital_ret_stop_loss_short, stop_loss_short_ids, nullptr, &loss_short_pos);
        records.insert(records.end(), trades_stop_loss_short.begin(), trades_stop_loss_short.end());
        ret_pos_ids.insert(ret_pos_ids.end(), stop_loss_short_ids.begin(), stop_loss_short_ids.end());

        double profit_short_pos = 0.0;
        std::vector<int> stop_profit_short_ids;
        std::vector<TradeRecordAtom> trades_stop_profit_short = account_info_.position.DoIfStopProfitShortPos(k_item, capital_ret_stop_profit_short, stop_profit_short_ids, nullptr, &profit_short_pos);
        records.insert(records.end(), trades_stop_profit_short.begin(), trades_stop_profit_short.end());
        ret_pos_ids.insert(ret_pos_ids.end(), stop_profit_short_ids.begin(), stop_profit_short_ids.end());

        double loss_long_pos = 0.0;
        std::vector<int> stop_loss_long_ids;
        std::vector<TradeRecordAtom> trades_stop_loss_long = account_info_.position.DoIfStopLossLongPos(k_item, capital_ret_stop_loss_long, stop_loss_long_ids, nullptr, &loss_long_pos);
        records.insert(records.end(), trades_stop_loss_long.begin(), trades_stop_loss_long.end());
        ret_pos_ids.insert(ret_pos_ids.end(), stop_loss_long_ids.begin(), stop_loss_long_ids.end());

        ret_profit = profit_long_pos + loss_short_pos + profit_short_pos + loss_long_pos;
    }else
    { // close price is nearby high price: first low price then high price
        double profit_short_pos = 0.0;
        std::vector<int> stop_profit_short_ids;
        std::vector<TradeRecordAtom> trades_stop_profit_short = account_info_.position.DoIfStopProfitShortPos(k_item, capital_ret_stop_profit_short, stop_profit_short_ids, nullptr, &profit_short_pos);
        records.insert(records.end(), trades_stop_profit_short.begin(), trades_stop_profit_short.end());
        ret_pos_ids.insert(ret_pos_ids.end(), stop_profit_short_ids.begin(), stop_profit_short_ids.end());

        double loss_long_pos = 0.0;
        std::vector<int> stop_loss_long_ids;
        std::vector<TradeRecordAtom> trades_stop_loss_long = account_info_.position.DoIfStopLossLongPos(k_item, capital_ret_stop_loss_long, stop_loss_long_ids, nullptr, &loss_long_pos);
        records.insert(records.end(), trades_stop_loss_long.begin(), trades_stop_loss_long.end());
        ret_pos_ids.insert(ret_pos_ids.end(), stop_loss_long_ids.begin(), stop_loss_long_ids.end());

        double profit_long_pos = 0.0;
        std::vector<int> stop_profit_long_ids;
        std::vector<TradeRecordAtom> trades_stop_profit_long = account_info_.position.DoIfStopProfitLongPos(k_item, capital_ret_stop_profit_long, stop_profit_long_ids, nullptr, &profit_long_pos);
        records.insert(records.end(), trades_stop_profit_long.begin(), trades_stop_profit_long.end());
        ret_pos_ids.insert(ret_pos_ids.end(), stop_profit_long_ids.begin(), stop_profit_long_ids.end());

        double loss_short_pos = 0.0;
        std::vector<int> stop_loss_short_ids;
        std::vector<TradeRecordAtom> trades_stop_loss_short = account_info_.position.DoIfStopLossShortPos(k_item, capital_ret_stop_loss_short, stop_loss_short_ids, nullptr, &loss_short_pos);
        records.insert(records.end(), trades_stop_loss_short.begin(), trades_stop_loss_short.end());
        ret_pos_ids.insert(ret_pos_ids.end(), stop_loss_short_ids.begin(), stop_loss_short_ids.end());

        ret_profit = profit_long_pos + loss_short_pos + profit_short_pos + loss_long_pos;
    }
    for( unsigned int i = 0; i < records.size(); ++i )
    {
        account_info_.capital.frozen -= cst_margin_capital * records.at(i).quantity;
        account_info_.capital.avaliable += cst_margin_capital * records.at(i).quantity;
    }
    account_info_.capital.avaliable += ret_profit;
    return records;
}

void TrainDlg::OnControl()
{
    if( !is_running_ )
    {
        is_running_ = true;
        ui.pbtnControl->setText(QString::fromLocal8Bit("暂停"));
        step_timer_->start(step_delay_ms_);//step_timer_->start(500);
    }else
    {
        is_running_ = false;
        ui.pbtnControl->setText(QString::fromLocal8Bit("运行"));
        step_timer_->stop();
    }
}

void TrainDlg::OnNextStep()
{  
    static auto remove_pos_tableview_closed_atoms = [this](const std::vector<int> &pos_del_ids, bool is_long)
    {
        if( pos_del_ids.empty() )
            return;
        QTableView &tbv = *ui.table_view_position;
        QStandardItemModel * model = static_cast<QStandardItemModel *>(tbv.model());
        auto row_index = find_model_first_fit_index(*model, cst_tbview_position_bs, is_long);
        assert(row_index > -1); 
        QVector<int> ids = model->item(row_index, cst_tbview_position_id)->data().value<QVector<int>>();
        QVector<int> ids_after;
        for( int i = 0; i < ids.size(); ++i )
        {
            auto ret = std::find_if(pos_del_ids.cbegin(), pos_del_ids.cend(),[&ids, i](int id){ return id == ids[i]; });
            if( ret == std::end(pos_del_ids) )
                ids_after.push_back(ids[i]);
        }
        if( ids_after.empty() )
            model->removeRow(row_index);
        else
            RecaculatePosTableViewItem(ids_after, row_index);
    };
    
    //double close_price = MAGIC_STOP_PRICE;
    T_StockHisDataItem ori_cur_item = main_win_->OriStepKlineWall()->Train_NextStep();
    parent_->Train_NextStep(ori_cur_item);
    main_win_->SubKlineWall()->Train_NextStep(ori_cur_item);

    const T_StockHisDataItem & stock_item = CurHisStockDataItem();
    if( stock_item.date == 0 )
    {
        SetStatusBar(QString::fromLocal8Bit("日期为0, 数据异常!"));
        return;
    }
    cur_kdata_item_ = ori_cur_item;
    ui.lab_quote->setText(QString::number(ori_cur_item.close_price));
    if( ui.checkb_follow_market->isChecked() )
        ui.dbspb_price->setValue(ori_cur_item.close_price);
      
     

     
}

void TrainDlg::SetMainWinStatusBar(const T_StockHisDataItem &k_item)
{
    int hh = k_item.hhmmss/100;
    int mm = k_item.hhmmss%100;
    int ss = 0;
    int min_5_amain_second = (5 - (hh * 60 + mm) % 5) * 60 - ss; 
    int min_15_amain_second = (15 - (hh * 60 + mm) % 15) * 60 - ss; 
    QString content = QString("%1                                         %2                 5M: %3     \t\t     15M: %4")
        .arg(QString("%1:%2:00").arg(hh,2,10,QLatin1Char('0')).arg(mm,2,10,QLatin1Char('0')))
        .arg(QString::number(k_item.close_price, 'f', 1))
        .arg(min_5_amain_second)
        .arg(min_15_amain_second);
    main_win_->statusBar()->showMessage(content);
}

void TrainDlg::RemoveInPositionTableView(int position_id, PositionType position_type)
{
    QTableView &tbv = *ui.table_view_position;
    QStandardItemModel * model = static_cast<QStandardItemModel *>(tbv.model());
    auto row_index = find_model_first_fit_index(*model, cst_tbview_position_bs, position_type == PositionType::POS_LONG);
    QVector<int> ids = model->item(row_index, cst_tbview_position_id)->data().value<QVector<int>>();
    auto target_item = std::find_if(std::begin(ids), std::end(ids),[position_id](int id){ return id == position_id; });
    //assert( target_item != std::end(ids) );
    ids.erase(target_item);
    if( ids.empty() )
        model->removeRow(row_index);
    else
    {
        RecaculatePosTableViewItem(ids, row_index);
    }
}

void TrainDlg::RecaculatePosTableViewItem(QVector<int> &ids, int row_index)
{
    QTableView &tbv = *ui.table_view_position;
    QStandardItemModel * model = static_cast<QStandardItemModel *>(tbv.model());
    QVariant var_data;
    var_data.setValue(ids);
    model->item(row_index, cst_tbview_position_id)->setData(var_data);
    //recaculate average open price-------
    double total_amount = 0.0;
    unsigned int total_qty = 0;
    unsigned int avaliable_qty = 0;
    for( int i = 0; i < ids.size(); ++i )
    {
        auto pos_item = account_info_.position.FindPositionAtom(ids[i]);
        if( pos_item )
        {
            total_amount += pos_item->qty_all() * pos_item->price;
            total_qty += pos_item->qty_all();
            avaliable_qty += pos_item->qty_available;
        }
    }
    assert(total_qty > 0);
    double after_average_price = ProcDecimal(total_amount/total_qty, DEFAULT_DECIMAL + 1);

    model->item(row_index, cst_tbview_position_average_price)->setText(QString::number(after_average_price));
    model->item(row_index, cst_tbview_position_size)->setText(QString::number(total_qty));
    model->item(row_index, cst_tbview_position_avaliable)->setText(QString::number(avaliable_qty));
}

void TrainDlg::RecaculatePosTableViewItem(int row_index)
{
    QTableView &tbv = *ui.table_view_position;
    QStandardItemModel * model = static_cast<QStandardItemModel *>(tbv.model());
     
    QVector<int> ids = model->item(row_index, cst_tbview_position_id)->data().value<QVector<int>>();
    RecaculatePosTableViewItem(ids, row_index);
}

// ps: auto set  profit ui item
double TrainDlg::RecaculatePosTableViewFloatProfit(double cur_price)
{
    double total_profit = 0.0;
    QTableView &tbv = *ui.table_view_position;
    QStandardItemModel * model = static_cast<QStandardItemModel *>(tbv.model());
    for( int i = 0; i < model->rowCount(); ++i )
    {
        bool is_long_pos = model->item(i, cst_tbview_position_bs)->data().toBool();
        auto position_size = model->item(i, cst_tbview_position_size)->text().toInt();
        auto average_price = model->item(i, cst_tbview_position_average_price)->text().toDouble();

        double profit = 0.0;
        if( is_long_pos )
            profit = (cur_price - average_price) / cst_per_tick * cst_per_tick_capital * position_size;
        else
            profit = (average_price - cur_price) / cst_per_tick * cst_per_tick_capital * position_size;

        profit = ProcDecimal(profit, 0);
        total_profit += profit;
        //ProcDecimal(allow_highest_price, DEFAULT_DECIMAL);
        auto profit_item = model->item(i, cst_tbview_position_float_profit);
        profit_item->setText(QString::number(profit));
    }
    return total_profit;
}

void TrainDlg::UpdateOrders2KlineWalls(int type)
{
    if( type == ORDER_TYPE_ALL )
    { 
        parent_->hangon_order_infos_ = hangon_order_infos_;
        parent_->stop_profit_order_infos_ = stop_profit_order_infos_;
        parent_->stop_loss_order_infos_ = stop_loss_order_infos_;
        parent_->condition_order_infos_ = condition_order_infos_;
        if( main_win_->SubKlineWall() )
        {
            main_win_->SubKlineWall()->hangon_order_infos_ = hangon_order_infos_;
            main_win_->SubKlineWall()->stop_profit_order_infos_ = stop_profit_order_infos_;
            main_win_->SubKlineWall()->stop_loss_order_infos_ = stop_loss_order_infos_;
            main_win_->SubKlineWall()->condition_order_infos_ = condition_order_infos_;
        }
    }else if( type == ORDER_TYPE_HANGON )
    {
        parent_->hangon_order_infos_ = hangon_order_infos_; 
        if( main_win_->SubKlineWall() )
        {
            main_win_->SubKlineWall()->hangon_order_infos_ = hangon_order_infos_; 
        }
    }else if( type == ORDER_TYPE_STOPPROFIT )
    { 
        parent_->stop_profit_order_infos_ = stop_profit_order_infos_; 
        if( main_win_->SubKlineWall() )
        {
            main_win_->SubKlineWall()->stop_profit_order_infos_ = stop_profit_order_infos_; 
        }
    }else if( type == ORDER_TYPE_STOPLOSS )
    { 
        parent_->stop_loss_order_infos_ = stop_loss_order_infos_; 
        if( main_win_->SubKlineWall() )
        {
            main_win_->SubKlineWall()->stop_loss_order_infos_ = stop_loss_order_infos_; 
        }
    }else if( type == ORDER_TYPE_CONDITION )
    { 
        parent_->condition_order_infos_ = condition_order_infos_;
        if( main_win_->SubKlineWall() )
        {
            main_win_->SubKlineWall()->condition_order_infos_ = condition_order_infos_;
        }
    }

    parent_->update();
    if( main_win_->SubKlineWall() )
        main_win_->SubKlineWall()->update();
}

int TrainDlg::TblHangonOrdersRowCount()
{
    auto model = (QStandardItemModel*)ui.table_view_order_hangon->model();
    return model->rowCount();
}

void TrainDlg::Append2TblHangonOrders(OrderInfo &order_info)
{
    auto model = (QStandardItemModel*)ui.table_view_order_hangon->model();
    model->insertRow(model->rowCount());
    int row_index = model->rowCount() - 1;
    auto item = new QStandardItem(QString::number(order_info.fake_id));
    model->setItem(row_index, cst_tbview_hangonorder_id, item);

    QString bs_str;
    if( order_info.action == OrderAction::OPEN )
        bs_str = QString::fromLocal8Bit(order_info.position_type == PositionType::POS_LONG ? "买" : "卖");
    else
        bs_str = QString::fromLocal8Bit(order_info.position_type == PositionType::POS_LONG ? "卖" : "买");
    item = new QStandardItem(bs_str);
    model->setItem(row_index, cst_tbview_hangonorder_bs, item);

    item = new QStandardItem(QString::fromLocal8Bit(order_info.action == OrderAction::CLOSE ? "平" : "开"));
    model->setItem(row_index, cst_tbview_hangonorder_oc, item);

    item = new QStandardItem(QString::number(order_info.price));
    model->setItem(row_index, cst_tbview_hangonorder_price, item);

    item = new QStandardItem(QString::number(order_info.qty));
    model->setItem(row_index, cst_tbview_hangonorder_qty, item);
}

void TrainDlg::RemoveFromTblHangonOrderByFakeId(int fake_id)
{
    auto model = (QStandardItemModel*)ui.table_view_order_hangon->model();
    for( int i = 0; i < model->rowCount(); ++i )
    {
        if( model->item(i, cst_tbview_hangonorder_id)->text().toInt() == fake_id )
        {
            model->removeRow(i);
            break;
        }
    }
}

void TrainDlg::RemoveFromTblConditionOrderByFakeId(int fake_id)
{
    for( int i = 0; i < condition_model_->rowCount(); ++i )
    {
        if( condition_model_->item(i, cst_tbview_condition_id)->text().toInt() == fake_id )
        {
            condition_model_->removeRow(i);
            break;
        }
    }
}

void TrainDlg::Append2TblTrades(TradeRecordAtom &trade)
{
    SoundFilled(trade.action == OrderAction::OPEN);
    auto model = (QStandardItemModel*)ui.table_view_trades->model();
    model->insertRow(0);
    unsigned int row_index = 0;
    auto item = new QStandardItem(QString::number(trade.trade_id));
    model->setItem(row_index, cst_tbview_trades_id, item);

    QString bs_tag;
    if( trade.action == OrderAction::OPEN )
        bs_tag = trade.pos_type == PositionType::POS_LONG ? QString::fromLocal8Bit("买") : QString::fromLocal8Bit("卖");
    else
        bs_tag = trade.pos_type == PositionType::POS_LONG ? QString::fromLocal8Bit("卖") : QString::fromLocal8Bit("买");
    item = new QStandardItem(bs_tag);
    model->setItem(row_index, cst_tbview_trades_bs, item);

    item = new QStandardItem(trade.action == OrderAction::OPEN ? QString::fromLocal8Bit("开") : QString::fromLocal8Bit("平"));
    model->setItem(row_index, cst_tbview_trades_oc, item);

    item = new QStandardItem(QString::number(trade.price));
    model->setItem(row_index, cst_tbview_trades_price, item);

    item = new QStandardItem(QString::number(trade.quantity));
    model->setItem(row_index, cst_tbview_trades_qty, item);

    item = new QStandardItem(QString::number(trade.fee));
    model->setItem(row_index, cst_tbview_trades_fee, item);

    item = new QStandardItem(QString::number(trade.profit));
    model->setItem(row_index, cst_tbview_trades_profit, item);

    item = new QStandardItem(QString::number(trade.hhmm));
    model->setItem(row_index, cst_tbview_trades_time, item);
}

void TrainDlg::RefreshCapitalUi()
{
    ui.label_capital->setText(QString::number(account_info_.capital.avaliable + account_info_.capital.frozen + account_info_.capital.float_profit));
    ui.label_capital_available->setText(QString::number(account_info_.capital.avaliable));

    ui.label_close_profit->setText(QString::number(account_info_.capital.avaliable + account_info_.capital.frozen - ori_capital_));
    ui.label_float_profit->setText(QString::number(account_info_.capital.float_profit));
}  

void TrainDlg::OnBuy()
{
    SetStatusBar("");
    const double market_price = ui.lab_quote->text().toDouble();
    const double order_price = ui.dbspb_price->text().toDouble();
    if( market_price < EPSINON )
    {
        SetStatusBar(QString::fromLocal8Bit("市价异常!"));
        return;
    } 
    if( ui.radio_postion_o->isChecked() ) // buy to open long position 
    {
        if( !(order_price < market_price) )
            OpenPosition(market_price, ui.spb_order_num->value(), true);
        else
            AddOpenOrder(ui.dbspb_price->value(), ui.spb_order_num->value(), true);
    }else // buy to close short position
    {
        if( !(order_price < market_price) )
            CloseInputSizePosition(market_price, false, cur_kdata_item_);
        else
            AddCloseOrder(ui.dbspb_price->value(), ui.spb_order_num->value(), false);
    }
    ui.tab_detail->setCurrentIndex(cst_detail_page_positions);
    RefreshCapitalUi();
}

void TrainDlg::OnSell()
{
    SetStatusBar("");
    const double market_price = ui.lab_quote->text().toDouble();
    const double order_price = ui.dbspb_price->text().toDouble();
    if( market_price < EPSINON )
    {
        SetStatusBar(QString::fromLocal8Bit("市价异常!"));
        return;
    }
    if( ui.radio_postion_o->isChecked() ) // sell to open short position 
    {
        if( !(order_price > market_price) )
            OpenPosition(market_price, ui.spb_order_num->value(), false);
        else
            AddOpenOrder(ui.dbspb_price->value(), ui.spb_order_num->value(), false);
    }else // sell to close long position
    {
        if( !(order_price > market_price) )
            CloseInputSizePosition(market_price, true, cur_kdata_item_);
        else
            AddCloseOrder(ui.dbspb_price->value(), ui.spb_order_num->value(), true);
    }
    ui.tab_detail->setCurrentIndex(cst_detail_page_positions);
    RefreshCapitalUi();
}

void TrainDlg::SaveStopProfitLoss(std::vector<PositionAtom> &pos_atoms)
{
    auto append_stop_order = [this](PositionAtom &pos_atom, bool is_stop_loss)
    {
        OrderInfo order(is_stop_loss ? OrderType::STOPLOSS : OrderType::STOPPROFIT);
        order.rel_position_id = pos_atom.trade_id;
        order.action = OrderAction::CLOSE;
        // target position type
        order.position_type = pos_atom.is_long ? PositionType::POS_LONG : PositionType::POS_SHORT;
        order.qty = pos_atom.qty_available;
        if( is_stop_loss )
        {
            order.price = pos_atom.stop_loss_price; 
            stop_loss_order_infos_.push_back(order);
        }else // stop profit
        {
            order.price = pos_atom.stop_profit_price; 
            stop_profit_order_infos_.push_back(order);
        } 
    };

    assert(!pos_atoms.empty());
    for( unsigned int i = 0; i < pos_atoms.size(); ++i )
    {
       auto pos_item = account_info_.position.FindPositionAtomSharedPointer(pos_atoms[i].trade_id);
       if( !pos_item || pos_item->qty_available == 0 )
           continue;
       pos_item->stop_profit_price = pos_atoms[i].stop_profit_price;
       pos_item->stop_loss_price = pos_atoms[i].stop_loss_price;
       
       auto target_order_item = std::find_if(stop_profit_order_infos_.begin(), stop_profit_order_infos_.end(), [&](OrderInfo &order_info){ return order_info.rel_position_id == pos_atoms[i].trade_id; });
       if( target_order_item != stop_profit_order_infos_.end() )
           target_order_item->price = pos_atoms[i].stop_profit_price;
       else
           append_stop_order(pos_atoms[i], false);

       target_order_item = std::find_if(stop_loss_order_infos_.begin(), stop_loss_order_infos_.end(), [&](OrderInfo &order_info){ return order_info.rel_position_id == pos_atoms[i].trade_id; });
       if( target_order_item != stop_loss_order_infos_.end() )
           target_order_item->price = pos_atoms[i].stop_loss_price;
       else
           append_stop_order(pos_atoms[i], true);
    }
    UpdateOrders2KlineWalls(ORDER_TYPE_STOPPROFIT);
    UpdateOrders2KlineWalls(ORDER_TYPE_STOPLOSS);
}

void TrainDlg::OnAddConditionOrder()
{
    condition_model_->insertRow(condition_model_->rowCount());
    const int row_index = condition_model_->rowCount() - 1;
    const int condition_order_id = GenerateConditionOrderId();
    auto item = new QStandardItem(QString::number(condition_order_id));
    condition_model_->setItem(row_index, cst_tbview_condition_id, item);
    
    const CompareType comp_type = ui.cmb_conditioin_compare_char->currentText() == ">=" ? CompareType::BIGEQUAL : CompareType::SMALLEQUAL;
    item = new QStandardItem(ui.cmb_condition_bs->currentData().toBool() ? QString::fromLocal8Bit("买") : QString::fromLocal8Bit("卖"));
    item->setData(ui.cmb_condition_bs->currentData());
    condition_model_->setItem(row_index, cst_tbview_condition_bs, item);

    item = new QStandardItem(QString::number(ui.spb_condition_qty->value()));
    condition_model_->setItem(row_index, cst_tbview_condition_qty, item);

    item = new QStandardItem( ui.cmb_conditioin_compare_char->currentText() );
    //item->setData(QVariant(int(comp_type)));
    condition_model_->setItem(row_index, cst_tbview_condition_compare_type, item);

    const double condition_price = ui.dbspb_condition_price->value();
    item = new QStandardItem(QString::number(condition_price));
    condition_model_->setItem(row_index, cst_tbview_condition_price, item);

    item = new QStandardItem(QString::number(ui.spb_cond_stop_profit_tick->value()));
    condition_model_->setItem(row_index, cst_tbview_condition_stop_profit, item);

    item = new QStandardItem(QString::number(ui.spb_cond_stop_loss_tick->value()));
    condition_model_->setItem(row_index, cst_tbview_condition_stop_loss, item);

    OrderInfo  order_info;
    order_info.type = OrderType::CONDITION;
    order_info.fake_id = condition_order_id;
    order_info.price = condition_price;
    order_info.compare_type = comp_type;
    order_info.action = OrderAction::OPEN;
    order_info.position_type = ui.cmb_condition_bs->currentData().toBool() ? PositionType::POS_LONG : PositionType::POS_SHORT;
    order_info.qty = ui.spb_condition_qty->value();
    order_info.profit_stop_ticks = ui.spb_cond_stop_profit_tick->value();
    order_info.loss_stop_ticks = ui.spb_cond_stop_loss_tick->value();
    condition_order_infos_.push_back(order_info);
    UpdateOrders2KlineWalls(ORDER_TYPE_CONDITION);
}

void TrainDlg::OpenPosition(double para_price, unsigned int qty, bool is_long, unsigned int *p_profit_stop_ticks, unsigned int *p_loss_stop_ticks, bool is_proc_hangon_order)
{ 
    assert(qty > 0);
    double price = para_price; 
    const double margin = cst_margin_capital * qty;
    const double fee = CalculateFee(qty, price, false);
    if( is_proc_hangon_order )
    {  // margin and fee has already been frozen
        assert(!(account_info_.capital.frozen < margin + fee)); 
        account_info_.capital.frozen -= fee;
    }else 
    {   
        if( margin + fee > account_info_.capital.avaliable )
        {
            SetStatusBar(QString::fromLocal8Bit("可用资金不足!"));
            return;
        }
        account_info_.capital.avaliable -= margin + fee; 
        account_info_.capital.frozen += margin;
    }
    auto pos_atom = std::make_shared<PositionAtom>();
    pos_atom->trade_id = account_info_.position.GenerateTradeId();
    pos_atom->is_long = is_long;
    pos_atom->price = price;
    pos_atom->qty_available = qty;
    
    account_info_.position.PushBack(is_long, pos_atom);

    TradeRecordAtom  trade_item;
    trade_item.trade_id = pos_atom->trade_id;
    trade_item.date = cur_kdata_item_.date;   // ndchk
    trade_item.hhmm = cur_kdata_item_.hhmmss; // ndchk
    trade_item.action = OrderAction::OPEN;
    trade_item.pos_type = is_long ? PositionType::POS_LONG : PositionType::POS_SHORT;
    trade_item.quantity = qty;
    trade_item.price = pos_atom->price;
    trade_item.fee = fee;
    trade_records_.push_back(trade_item);
    Append2TblTrades(trade_item);

    // fill or ajust position table view -------------
    auto align_way = Qt::AlignCenter;
    QTableView &tbv = *ui.table_view_position;
    QStandardItemModel * model = static_cast<QStandardItemModel *>(tbv.model());
    QVariant var_ids;
    QVector<int> ids;
    auto row_index = find_model_first_fit_index(*model, cst_tbview_position_bs, pos_atom->is_long);
    if( row_index > -1 ) 
    {   // merge long position
        ids = model->item(row_index, cst_tbview_position_id)->data().value<QVector<int>>();
        ids.push_back(pos_atom->trade_id);
        var_ids.setValue(ids);
        model->item(row_index, cst_tbview_position_id)->setData(var_ids);
         
        unsigned int total_qty = is_long ? account_info_.position.LongPosQty(POSITION_STATUS_ALL) : account_info_.position.ShortPosQty(POSITION_STATUS_ALL);
        assert(total_qty > 0); 
        unsigned int ava_qty = is_long ? account_info_.position.LongPosQty(POSITION_STATUS_AVAILABLE) : account_info_.position.ShortPosQty(POSITION_STATUS_AVAILABLE);
        double avarege_price = is_long ? account_info_.position.LongAveragePrice() : account_info_.position.ShortAveragePirce();
        model->item(row_index, cst_tbview_position_average_price)->setText(QString::number(avarege_price));
        model->item(row_index, cst_tbview_position_size)->setText(QString::number(total_qty));
        model->item(row_index, cst_tbview_position_avaliable)->setText(QString::number(ava_qty));

    }else
    {
        model->insertRow(model->rowCount());
        row_index = model->rowCount() - 1;

        ids.push_back(pos_atom->trade_id);
        var_ids.setValue(ids);
        auto item = new QStandardItem( QString::number(pos_atom->trade_id) );
        item->setData(var_ids);
        model->setItem(row_index, cst_tbview_position_id, item);
        model->item(row_index, cst_tbview_position_id)->setTextAlignment(align_way);

        item = new QStandardItem( QString::number(pos_atom->price) );
        model->setItem(row_index, cst_tbview_position_average_price, item);
        model->item(row_index, cst_tbview_position_average_price)->setTextAlignment(align_way);

        item = new QStandardItem( QString::number(0) );
        model->setItem(row_index, cst_tbview_position_float_profit, item);
        model->item(row_index, cst_tbview_position_float_profit)->setTextAlignment(align_way);

        item = new QStandardItem( QString::fromLocal8Bit(pos_atom->is_long ? "买" : "卖") );
        item->setData(QVariant(pos_atom->is_long));
        model->setItem(row_index, cst_tbview_position_bs, item);
        model->item(row_index, cst_tbview_position_bs)->setTextAlignment(align_way);

        item = new QStandardItem( QString::number(pos_atom->qty_all()) );
        model->setItem(row_index, cst_tbview_position_size, item);
        model->item(row_index, cst_tbview_position_size)->setTextAlignment(align_way);

        item = new QStandardItem( QString::number(pos_atom->qty_available) );
        model->setItem(row_index, cst_tbview_position_avaliable, item);
        model->item(row_index, cst_tbview_position_avaliable)->setTextAlignment(align_way);
    }
    // consider stop (profit/loss) order -------------------
    // ps: set PositionAtom's stop profit/loss price which calculate from para price
    auto append_stop_order_auto_calc_price = [this](PositionAtom &pos_atom, double price, bool is_long, bool is_stop_loss)
    {
        OrderInfo order(is_stop_loss ? OrderType::STOPLOSS : OrderType::STOPPROFIT);
        order.rel_position_id = pos_atom.trade_id;
        order.action = OrderAction::CLOSE;
        // target position type
        order.position_type = is_long ? PositionType::POS_LONG : PositionType::POS_SHORT;
        order.qty = pos_atom.qty_available;
        if( is_stop_loss )
        {
            order.price = is_long ? (price - cst_per_tick * (double)auto_stop_loss_ticks_) : (price + cst_per_tick * (double)auto_stop_loss_ticks_);
            pos_atom.stop_loss_price = order.price;
            stop_loss_order_infos_.push_back(order);
        }else // stop profit
        {
            order.price = is_long ? (price + cst_per_tick * (double)auto_stop_profit_ticks_) : (price - cst_per_tick * (double)auto_stop_profit_ticks_);
            pos_atom.stop_profit_price = order.price;
            stop_profit_order_infos_.push_back(order);
        } 
    };
    // ps: set PositionAtom's stop profit/loss price  
    auto append_stop_order = [this](PositionAtom &pos_atom, double stop_price, bool is_long, bool is_stop_loss)
    {
        OrderInfo order(is_stop_loss ? OrderType::STOPLOSS : OrderType::STOPPROFIT);
        order.rel_position_id = pos_atom.trade_id;
        order.action = OrderAction::CLOSE;
        // target position type
        order.position_type = is_long ? PositionType::POS_LONG : PositionType::POS_SHORT;
        order.qty = pos_atom.qty_available;
        order.price = stop_price;
        if( is_stop_loss )
        {
            pos_atom.stop_loss_price = order.price;
            stop_loss_order_infos_.push_back(order);
        }else
        {
            pos_atom.stop_profit_price = order.price;
            stop_profit_order_infos_.push_back(order);
        }
    };
    bool is_set_hand_stop = false;
    if( p_profit_stop_ticks && *p_profit_stop_ticks != 0 )
    {
        is_set_hand_stop = true;
        double stop_profit_price = is_long ? (price + cst_per_tick * (*p_profit_stop_ticks)) : (price - cst_per_tick * (*p_profit_stop_ticks)); 
        append_stop_order(*pos_atom, stop_profit_price, is_long, false);
        UpdateOrders2KlineWalls(ORDER_TYPE_STOPPROFIT);
    }
    if( p_loss_stop_ticks && *p_loss_stop_ticks != 0 )
    {
        is_set_hand_stop = true;
        double stop_loss_price = is_long ? (price - cst_per_tick * (*p_loss_stop_ticks)) : (price + cst_per_tick * (*p_loss_stop_ticks)); 
        append_stop_order(*pos_atom, stop_loss_price, is_long, true);
        UpdateOrders2KlineWalls(ORDER_TYPE_STOPLOSS);
    }

    if( !is_set_hand_stop ) // auto set
    {
        if( auto_stop_profit_ )
        {
            append_stop_order_auto_calc_price(*pos_atom, price, is_long, false);
            UpdateOrders2KlineWalls(ORDER_TYPE_STOPPROFIT);
        }
        if( auto_stop_loss_ )
        {
            append_stop_order_auto_calc_price(*pos_atom, price, is_long, true); 
            UpdateOrders2KlineWalls(ORDER_TYPE_STOPLOSS);
        }
    }
    bool ret = CheckPosition();//debug nddel
    ret = ret;
}

int TrainDlg::ClosePosition(double para_price, unsigned int qty, bool is_long, const T_StockHisDataItem &fake_k_item, QString *p_ret_info)
{
    if( qty == 0 )
        return 0;
    //int date = 0;
    //int hhmm = 0;//ndedt
    QTableView &tbv = *ui.table_view_position;
    QStandardItemModel * model = static_cast<QStandardItemModel *>(tbv.model());
    auto row_index = find_model_first_fit_index(*model, cst_tbview_position_bs, is_long);
    if( row_index < 0 ) 
    { 
        if( p_ret_info ) *p_ret_info = (is_long ? QString::fromLocal8Bit("无多头仓位可平!") : QString::fromLocal8Bit("无空头仓位可平!"));
        return -1;
    }
    
    QVector<int> ids = model->item(row_index, cst_tbview_position_id)->data().value<QVector<int>>();
    double margin_ret = 0.0;
    double profit_close = 0.0;
    std::vector<int> position_ids;
    std::vector<TradeRecordAtom> trade_record_atoms;
    if( is_long )
    {
        trade_record_atoms = account_info_.position.CloseAvaliableLong(para_price, qty, margin_ret, &profit_close, &position_ids);
    }else
    {
        trade_record_atoms = account_info_.position.CloseAvaliableShort(para_price, qty, margin_ret, &profit_close, &position_ids);
    }
    if( trade_record_atoms.empty() )
    {
        if( p_ret_info ) *p_ret_info = QString::fromLocal8Bit("此平仓失效!");
        return -1;
    }
    remove_container_items_by_pos_ids(stop_profit_order_infos_, (is_long ? &position_ids : nullptr), (is_long ? nullptr : &position_ids));
    remove_container_items_by_pos_ids(stop_loss_order_infos_, (is_long ? &position_ids : nullptr), (is_long ? nullptr : &position_ids));

    UpdateOrders2KlineWalls(ORDER_TYPE_STOPPROFIT);
    UpdateOrders2KlineWalls(ORDER_TYPE_STOPLOSS);

    //capital------------
    account_info_.capital.frozen -= margin_ret;
    account_info_.capital.avaliable += margin_ret + profit_close;

    // reset position table view----------
    QVector<int> ids_after;
    for( int i = 0; i < ids.size(); ++i )
    {
        auto ret = std::find_if(std::begin(position_ids), std::end(position_ids),[&ids, i](int id){ return id == ids[i]; });
        if( ret == std::end(position_ids) )
            ids_after.push_back(ids[i]);
    }
    if( ids_after.empty() )
        model->removeRow(row_index);
    else
    { 
        RecaculatePosTableViewItem(ids_after, row_index); 
    }

    // recaculae float prift ------
    double cur_quote = ui.lab_quote->text().toDouble();
    double total_profit = RecaculatePosTableViewFloatProfit(cur_quote);
    account_info_.capital.float_profit = ProcDecimal(total_profit, 0);

    // ------------------
    std::for_each(std::begin(trade_record_atoms), std::end(trade_record_atoms),[this](TradeRecordAtom &trade_atom)
    {
        Append2TblTrades(trade_atom);
    });

    bool ret = CheckPosition();//debug nddel

    return trade_record_atoms.size();
}

void TrainDlg::CloseInputSizePosition(double para_price, bool is_long, const T_StockHisDataItem &fake_k_item)
{
    assert(para_price > 0.1);
    int close_qty = ui.spb_order_num->value();
    assert(close_qty > 0);
    QTableView &tbv = *ui.table_view_position;
    QStandardItemModel * model = static_cast<QStandardItemModel *>(tbv.model());
    auto row_index = find_model_first_fit_index(*model, cst_tbview_position_bs, is_long);
    if( row_index < 0 ) 
    { 
        return SetStatusBar(is_long ? QString::fromLocal8Bit("无多头仓位可平!") : QString::fromLocal8Bit("无空头仓位可平!"));
    }
     
    auto total_qty = is_long ? account_info_.position.LongPosQty(POSITION_STATUS_AVAILABLE) : account_info_.position.ShortPosQty(POSITION_STATUS_AVAILABLE);
    if( (unsigned int)close_qty > total_qty )
         return SetStatusBar(QString::fromLocal8Bit("平仓数量超过现有仓位!"));

    //double para_price, unsigned int qty, bool is_long, const T_StockHisDataItem &fake_k_item, QString *p_ret_info
    QString ret_info;
    ClosePosition(para_price, close_qty, is_long, fake_k_item, &ret_info);
    if( !ret_info.isEmpty() )
        SetStatusBar(ret_info);
}

bool TrainDlg::AddOpenOrder(double price, unsigned int quantity, bool is_long)
{ 
    /*double capital_buy = cst_margin_capital * quantity;
    double fee = CalculateFee(quantity, price, false);*/
    double capital_to_freeze = CaculateOpenPositionFreezeCapital(price, quantity);
    if( capital_to_freeze > account_info_.capital.avaliable )
    {
        SetStatusBar(QString::fromLocal8Bit("可用资金不足!"));
        return false;
    }
    account_info_.capital.avaliable -= capital_to_freeze;
    account_info_.capital.frozen += capital_to_freeze ;

    OrderInfo order(OrderType::HANGON);
    order.fake_id = GenerateHangonOrderId();  
    order.action = OrderAction::OPEN;
    order.position_type = is_long ? PositionType::POS_LONG : PositionType::POS_SHORT;
    order.qty = quantity;
    order.price = price;
    hangon_order_infos_.push_back(order);
    Append2TblHangonOrders(order);
    UpdateOrders2KlineWalls(ORDER_TYPE_HANGON);
    return true;
}

bool TrainDlg::AddCloseOrder(double price, unsigned int quantity, bool is_long)
{
    unsigned int rel_pos_size = is_long ? account_info_.position.LongPosQty(POSITION_STATUS_AVAILABLE) : account_info_.position.ShortPosQty(POSITION_STATUS_AVAILABLE);
    if( quantity > rel_pos_size )
    {
        SetStatusBar(is_long ? QString::fromLocal8Bit("无多头仓位可平!") : QString::fromLocal8Bit("无空头仓位可平!"));
        return false;
    }

    //---------------------------------------
    OrderInfo order(OrderType::HANGON);
    order.fake_id = GenerateHangonOrderId();
    order.action = OrderAction::CLOSE;
    order.position_type = is_long ? PositionType::POS_LONG : PositionType::POS_SHORT;
    order.qty = quantity;
    order.price = price;
    // freeze avaliable position
    std::unordered_map<int, unsigned int> rel_ids_sizes = is_long ? account_info_.position.LongPosSizeInfo(POSITION_STATUS_AVAILABLE) : account_info_.position.ShortPosSizeInfo(POSITION_STATUS_AVAILABLE);
    unsigned int remain_qty = quantity;

    for( auto iter = rel_ids_sizes.begin(); iter != rel_ids_sizes.end() && remain_qty > 0; ++iter )
    {
        PositionAtom * p_target_atom = account_info_.position.FindPositionAtom(iter->first);
        assert( p_target_atom );
        if( p_target_atom->qty_available >= remain_qty )
        {
            p_target_atom->Freeze(order.fake_id, remain_qty);
            order.help_contain.insert(std::make_pair(p_target_atom->trade_id, remain_qty));
            remain_qty = 0;
            break;
        }else
        {
            p_target_atom->Freeze(order.fake_id, p_target_atom->qty_available);
            order.help_contain.insert(std::make_pair(p_target_atom->trade_id, p_target_atom->qty_available));
            remain_qty -= (int)p_target_atom->qty_available;
        }
    }

    hangon_order_infos_.push_back(order);

    Append2TblHangonOrders(order);

    // ----------update table_view_position 
    QTableView &tbv = *ui.table_view_position;
    QStandardItemModel * model = static_cast<QStandardItemModel *>(tbv.model());
    auto row_index = find_model_first_fit_index(*model, cst_tbview_position_bs, is_long);
    if( row_index > -1 ) 
    {  
        unsigned int ava_qty = is_long ? account_info_.position.LongPosQty(POSITION_STATUS_AVAILABLE) : account_info_.position.ShortPosQty(POSITION_STATUS_AVAILABLE);
        model->item(row_index, cst_tbview_position_avaliable)->setText(QString::number(ava_qty));
    }
     
    UpdateOrders2KlineWalls(ORDER_TYPE_HANGON);
    return true;
}

unsigned int TrainDlg::GetTableViewPositionAvailable(bool is_long)
{
    unsigned int total = 0;
    QTableView &tbv = *ui.table_view_position;
    QStandardItemModel * model = static_cast<QStandardItemModel *>(tbv.model());
    for( int i = 0; i < model->rowCount(); ++i )
    {
        if( model->item(i, cst_tbview_position_bs)->data().toBool() == is_long )
        {
           total += model->item(i, cst_tbview_position_avaliable)->text().toInt();
        }
    }
    return total;
}

unsigned int TrainDlg::GetItemPositionAllQty(QStandardItemModel& model, int row_index)
{ 
    unsigned int total_qty = 0;
    QVector<int> ids = model.item(row_index, cst_tbview_position_id)->data().value<QVector<int>>();
    for( int i = 0; i < ids.size(); ++i )
    {
        PositionAtom *position_atom = account_info_.position.FindPositionAtom(ids[i]);
        assert(position_atom);
        total_qty += position_atom->qty_all();
    }
    return total_qty;
}

void TrainDlg::PrintTradeRecords()
{
    ui.plain_te_record->clear();

    QString records_str;
    for(unsigned int i = 0; i < trade_records_.size(); ++i )
    {
        records_str.append(trade_records_.at(i).ToQStr());
        records_str.append("\n");
    }
    ui.plain_te_record->setPlainText(records_str);
    //ui.plain_te_record->setText()
}

bool TrainDlg::CheckPosition()
{
    const unsigned int long_avaliable = account_info_.position.LongPosQty(POSITION_STATUS_AVAILABLE);
    const unsigned int tblview_long_avaliable = GetTableViewPositionAvailable(true);
    if( long_avaliable != tblview_long_avaliable )
    {
        SetStatusBar(QString::fromLocal8Bit("多头可用仓位异常:视图仓位与后台仓位不符!"));
        return false;
    }
    const unsigned int short_avaliable = account_info_.position.ShortPosQty(POSITION_STATUS_AVAILABLE);
    const unsigned int tblview_short_avaliable = GetTableViewPositionAvailable(false);
    if( short_avaliable != tblview_short_avaliable )
    {
        SetStatusBar(QString::fromLocal8Bit("空头可用仓位异常:视图仓位与后台仓位不符!"));
        return false;
    }
    return true;
}

bool erase_rel_item_by_pos_ids(std::list<OrderInfo> &order_info, std::list<OrderInfo>::iterator &order_item, std::vector<int> &target_del_ids )
{
    auto ret = std::find_if(target_del_ids.begin(), target_del_ids.end(), [&order_item](int id){ return id == order_item->rel_position_id; });
    if( ret != target_del_ids.end() )
    {
        order_info.erase(order_item++);
        return true;
    }else
        return false;
}

bool erase_rel_item_by_fake_ids(std::list<OrderInfo> &order_info, std::list<OrderInfo>::iterator &order_item, std::vector<int> &target_del_ids)
{
    auto ret = std::find_if(target_del_ids.begin(), target_del_ids.end(), [&order_item](int id){ return id == order_item->fake_id; });
    if( ret != target_del_ids.end() )
    {
        order_info.erase(order_item++);
        return true;
    }else
        return false;
};

void remove_container_items_by_pos_ids(std::list<OrderInfo> &container, std::vector<int> *long_deleted_ids, std::vector<int> *short_deleted_ids)
{
    for( auto order_item = container.begin(); order_item != container.end(); )
    {
        if( long_deleted_ids && erase_rel_item_by_pos_ids(container, order_item, *long_deleted_ids) )
            continue;
        if( short_deleted_ids && erase_rel_item_by_pos_ids(container, order_item, *short_deleted_ids) )
            continue;
        ++order_item;
    }
}

int find_model_first_fit_index(QStandardItemModel& model, int col_index, bool is_long_pos)
{
    for(int j = 0; j < model.rowCount(); ++j )
    {
        if( model.item(j, col_index)->data().toBool() == is_long_pos )
            return j;
    }
    return -1;
}

std::tuple<double, unsigned int> get_total_amount_qty(PositionInfo &position, QVector<int> &ids)
{
    double total_amount = 0.0;
    unsigned int total_qty = 0;
    for( int i = 0; i < ids.size(); ++i )
    {
        auto pos_item = position.FindPositionAtom(ids[i]);
        if( pos_item )
        {
            total_amount += pos_item->qty_all() * pos_item->price;
            total_qty += pos_item->qty_all();
        }
    }
    return std::make_tuple(total_amount, total_qty);
}

