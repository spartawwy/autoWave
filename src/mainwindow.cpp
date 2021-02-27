
#include <windowsx.h>
#include <qt_windows.h> 


#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <iostream>
#include <string.h>

#include <QString>
#include <QDateTime>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QComboBox>
#include <QMessageBox>
#include <QTimer>
#include <qdesktopwidget.h>

#include <qdebug.h>
#include "config_man.h"
#include "futures_forecast_app.h"
#include "kline_wall.h"
#include "title_bar.h"
#include "tool_bar.h"
#include "code_list_wall.h"
#include "train_dlg.h"
#include "mock_trade_dlg.h"
#include "capital_curve.h"

#include "strategy_man.h"
#include "position_account.h"

#include "winner_quotation_api.h"

static const int cst_win_width = 1000;
static const int cst_win_height = 500;
static const int cst_update_kwall_inter = 10;

void QuoteCallBackFunc(T_QuoteData *quote_data, void *para, char *ErrInfo);

using namespace std;

MainWindow::MainWindow(FuturesForecastApp *app, QWidget *parent) :
    QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , app_(app)
    , tool_bar_(nullptr)
    , title_(nullptr)
    , kline_wall_ori_step_(nullptr)
    , kline_wall_main_(nullptr)
    , kline_wall_sub_(nullptr)
    , code_list_wall_(nullptr)
    , cur_kline_index_(WallIndex::MAIN)
    , stock_input_dlg_(this, app->data_base())
    , timer_update_kwall_inter_(0)
    , train_dlg_(nullptr)
    , is_train_mode_(false)
    , is_mock_trade_(false)
    , mock_trade_dlg_(nullptr)
    , show_sig_(false)
    , cur_quote_price_(0.0)
    , is_show_autoforcast_(false)
    , is_auto_next_k_(false)
    , is_run_backtest_(false)
{
    ui->setupUi(this); 
}

MainWindow::~MainWindow()
{
//#ifdef USE_STATUS_BAR
//    delete timer;
//#endif
//    delete ui;
}


bool MainWindow::Initialize()
{  
    // https://blog.csdn.net/qq_28093585/article/details/78517358
    this->setWindowFlags(Qt::FramelessWindowHint);  
    //this->setGeometry(100, 100, cst_win_width, cst_win_height);

    auto desktop_wid = QApplication::desktop();
    //获取设备屏幕大小
    QRect screenRect = desktop_wid->screenGeometry();

    this->setGeometry(0, 0, screenRect.width() * 0.8, screenRect.height() * 0.8);

#ifndef _DEBUG
    auto cur_date = QDate::currentDate().year() * 10000 + QDate::currentDate().month() * 100 + QDate::currentDate().day();
    if( cur_date > 20191225 )
    {
        QMessageBox::information(nullptr, QString::fromLocal8Bit("提示"), QString::fromLocal8Bit("软件已经到期!请联系249564063@qq.com")); 
        return false;
    }
#endif
    QWidget *wd = new QWidget(this);  
    QVBoxLayout *layout_all = new QVBoxLayout;  
    layout_all->setContentsMargins(0,0,0,0);  
    layout_all->setSpacing(0);  
    // title ---------
    title_ = new TitleBar(this);
    layout_all->addWidget(title_);  

    // tool bar ---------
    tool_bar_ = new ToolBar(this);
    tool_bar_->SetMainKwallCurCycleType(DEFAULT_MAINKWALL_TYPE_PERIOD);
    bool ret = connect(tool_bar_->main_cycle_comb(), SIGNAL(currentIndexChanged(int)), this, SLOT(onMainKwallCycleChange(int)));
    
    tool_bar_->SetSubKwallCurCycleType(DEFAULT_SUBKWALL_TYPE_PERIOD);
    ret = connect(tool_bar_->sub_cycle_comb(), SIGNAL(currentIndexChanged(int)), this, SLOT(onSubKwallCycleChange(int)));
    ret = ret;
    /*
    kline_wall_ori_step_ = new KLineWall(app_, this, (int)WallIndex::ORISTEP, DEFAULT_ORI_STEP_TYPE_PERIOD);
    if( !kline_wall_ori_step_->Init() )
        return false;
    kline_wall_ori_step_->setMouseTracking(true); 
    kline_wall_ori_step_->setFocusPolicy(Qt::StrongFocus);
    //view_layout->addWidget(kline_wall_sub_);
    kline_wall_ori_step_->setVisible(false);
    */
    // view area ----------
    QHBoxLayout * view_layout = new QHBoxLayout;
    view_layout->setContentsMargins(0,0,0,0);  
    view_layout->setSpacing(1);  

    kline_wall_main_ = new KLineWall(app_, this, (int)WallIndex::MAIN, DEFAULT_MAINKWALL_TYPE_PERIOD);
#if 1
    if( !kline_wall_main_->Init() )
        return false;  
#endif
    kline_wall_main_->setMouseTracking(true);
    //kline_wall_main_->ResetTypePeriod(DEFAULT_MAINKWALL_TYPE_PERIOD);
    kline_wall_main_->setFocusPolicy(Qt::StrongFocus);
    view_layout->addWidget(kline_wall_main_);
    
#ifdef MAKE_SUB_WALL
    kline_wall_sub_ = new KLineWall(app_, this, (int)WallIndex::SUB, DEFAULT_SUBKWALL_TYPE_PERIOD);
    if( !kline_wall_sub_->Init() )
        return false;
    kline_wall_sub_->setMouseTracking(true);
    //kline_wall_sub_->ResetTypePeriod(DEFAULT_SUBKWALL_TYPE_PERIOD);
    kline_wall_sub_->setFocusPolicy(Qt::StrongFocus);
    view_layout->addWidget(kline_wall_sub_);

    kline_wall_sub_->setVisible(false);
#endif
    // end of view area-------
     
    code_list_wall_ = new CodeListWall(app_, this);
    code_list_wall_->Init();

    layout_all->addWidget(tool_bar_);  
    layout_all->addLayout(view_layout);  
    layout_all->addWidget(code_list_wall_);  

    code_list_wall_->hide();

    wd->setLayout(layout_all);  
    this->setCentralWidget(wd);  

    capital_curve_ = std::make_shared<CapitalCurve>(nullptr);
    capital_curve_->setWindowFlags(capital_curve_->windowFlags() | Qt::WindowStaysOnTopHint/*Qt::Dialog*/ );
    capital_curve_->hide();

    train_dlg_ = new TrainDlg(kline_wall_main_, this);
    train_dlg_->setWindowFlags(train_dlg_->windowFlags() | Qt::WindowStaysOnTopHint/*Qt::Dialog*/ );
    train_dlg_->hide();

    //mock_trade_dlg_ = new MockTradeDlg(this);
    //mock_trade_dlg_->setWindowFlags(train_dlg_->windowFlags() | Qt::WindowStaysOnTopHint/*Qt::Dialog*/ );
    //mock_trade_dlg_->hide();

    bool result = connect(this, SIGNAL(sigQuoteData(double, double, double, int, int, int)), mock_trade_dlg_, SLOT(slotHandleQuote(double, double, double, int, int, int)));

    //-------------------------

    timer = new QTimer(this);
    timer->setInterval(1000);
    connect(timer, SIGNAL(timeout()), this, SLOT(onTimer()));
    //timer->start();
       
#ifdef USE_STATUS_BAR
    
    ui->labelCurrentTime->setText(
            QDateTime::currentDateTime().toString("yyyy-MM-dd HH:MM:ss"));
     
    ui->statusBar->showMessage("hello",2000); 
    ui->statusBar->addPermanentWidget(ui->labelCurrentTime);
#endif
      
#if 0 
    auto ck_val = kline_wall_main_->height();
    if( !kline_wall_main_->Init() )
        return false; 
    auto ck_val1 = kline_wall_main_->height();
#endif
    /*g_nActScreenX = screenRect.width();
    g_nActScreenY = screenRect.height();*/

    return true;
}

void MainWindow::SetCurKlineWallIndex(WallIndex index)
{
    if( cur_kline_index_ != index )
    { 
        CurKlineWall()->ResetDrawState(DrawAction::NO_ACTION);
    } 
    
    cur_kline_index_ = index; 
}

void MainWindow::SetMainView(WallType wall_type)
{
    kline_wall_main_->hide();
    switch(wall_type)
    {    
    case WallType::KLINE: 
        code_list_wall_->hide();
        if( kline_wall_sub_ )
            kline_wall_sub_->hide();
        kline_wall_main_->show(); 
        break;
    case WallType::CODE_LIST: 
        kline_wall_main_->hide();
        code_list_wall_->show(); 
        break;
    default:break;
    }
}

void MainWindow::ResetKLineWallCode(const QString &code, const QString &cn_name, bool is_index, int nmarket)
{
    kline_wall_main_->ResetStock(code, cn_name, is_index, nmarket);
}
 
void MainWindow::closeEvent(QCloseEvent * event)
{
    if( train_dlg_ )
        train_dlg_->hide();
    if( capital_curve_ )
        capital_curve_->hide();
    auto ret_button = QMessageBox::question(nullptr, QString::fromLocal8Bit("提示"), QString::fromLocal8Bit("确定退出系统?"),
        QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Cancel);
    if( ret_button == QMessageBox::Cancel )
        event->ignore();
    else
        app_->Stop();
}

void MainWindow::UncheckBtnABDownPen()
{
    if( tool_bar_ ) 
        tool_bar_->UncheckBtnABDownPen(); 
}

void MainWindow::UncheckBtnABUpPen()
{
    if( tool_bar_ ) 
        tool_bar_->UncheckBtnABUpPen(); 
}

void MainWindow::UncheckBtnABCDownPen()
{
    if( tool_bar_ ) 
        tool_bar_->UncheckBtnABCDownPen(); 
}

void MainWindow::UncheckBtnABCUpPen()
{
    if( tool_bar_ ) 
        tool_bar_->UncheckBtnABCUpPen(); 
}

void MainWindow::AddCode2CodeList(const QString &code, const QString &cn_name, bool is_index, int nmarket)
{
    code_list_wall_->AddCode(code , cn_name, is_index, nmarket);
}


void MainWindow::StockInputDlgRet()
{
    bool is_index = false;

    QString::SectionFlag flag = QString::SectionSkipEmpty;
    QString tgt_tag = stock_input_dlg_.ui.stock_input->text().section('/', 0, 0, flag);
    QString stock_code = tgt_tag.toLocal8Bit().data();
    /*if( !IsStrNum(stock_code) || stock_code.size() != 6 )
    return;*/
    QString stock_name = stock_input_dlg_.ui.stock_input->text().section('/', 1, 1, flag);

    QString type = stock_input_dlg_.ui.stock_input->text().section('/', 2, 2, flag);
    is_index = type == "1";
    QString stock_code_changed;
	stock_input_dlg_.ui.stock_input->text().clear();

	auto first_code = stock_code.toLocal8Bit().data();
	/*if( *first_code != '\0' && *first_code != '\2' && *first_code == '\3' && *first_code == '\6' )
		return;
	if( stock_code.length() != 6 || !IsNumber(stock_code.toLocal8Bit().data()) )
		return;*/
    QString maket = stock_input_dlg_.ui.stock_input->text().section('/', 3, 3, flag);
    int nmakert = maket.toInt();
    stock_code_changed = stock_code.toLocal8Bit().data();

    kline_wall_main_->ResetStock(stock_code_changed, stock_name, is_index, nmakert);
    if( kline_wall_sub_ )
        kline_wall_sub_->ResetStock(stock_code_changed, stock_name, is_index, nmakert);
}
 
void MainWindow::UpdateStockData(int cur_date, int cur_hhmm)
{
    int target_date = 0;
    int target_hhmm = 0;
    if( kline_wall_main_ )
    {
        // fake. ndedt:
        auto temp = kline_wall_main_->NextKTradeDateHHmm();
        target_date = std::get<0>(temp);
        target_hhmm = std::get<1>(temp);

        auto p_item = kline_wall_main_->UpdateIfNecessary(target_date, target_hhmm);
        if( kline_wall_sub_ && p_item )
            kline_wall_sub_->UpdateIfNecessary(target_date, target_hhmm, p_item);
    }
    
}

void MainWindow::UpdateStockQuote(const T_QuoteData &quote_data)
{
    if( kline_wall_main_ )
    { 
        kline_wall_main_->UpdateIfNecessary(quote_data);
    }
    if( kline_wall_sub_ )
        kline_wall_sub_->UpdateIfNecessary(quote_data);

    app_->strategy_man()->Handle(quote_data);
}

// ps : cause 1m is updated high frequent, so only update main k line wall
void MainWindow::UpdateStockQuoteOfTrainMode()
{
    if( kline_wall_main_ )
        kline_wall_main_->UpdateStockQuoteOfTrainMode();
}

void MainWindow::PopTrainDlg(bool reset_type)
{
    assert(train_dlg_);
    if( reset_type )
    {
        if( tool_bar()->main_cycle_comb()->currentIndex() != COMBO_PERIOD_5M_INDEX )
        {
            // ps: it will trigger onMainKwallCycleChange
            tool_bar()->main_cycle_comb()->setCurrentIndex(COMBO_PERIOD_5M_INDEX); // set current to period day
        }
        if( tool_bar()->sub_cycle_comb()->currentIndex() != COMBO_PERIOD_1M_INDEX )
        {
            // ps: it will trigger onSubKwallCycleChange
            tool_bar()->sub_cycle_comb()->setCurrentIndex(COMBO_PERIOD_1M_INDEX);
        }
    }
    
    tool_bar()->main_cycle_comb()->setEnabled(false);

    is_train_mode(true);
    train_dlg_->showNormal();
}

void MainWindow::MinimizeTrainDlg()
{
    train_dlg_->showMinimized();
}

void MainWindow::PopCapitalCurve()
{
    capital_curve_->showNormal();
}

void MainWindow::PopMokeTradeDlg()
{
    mock_trade_dlg_->showNormal();
}

void MainWindow::MinimizeMockTradeDlg()
{
    mock_trade_dlg_->showMinimized();
}

void MainWindow::changeEvent(QEvent *e)
{
    QMainWindow::changeEvent(e);
    switch (e->type()) 
    {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}
 
bool MainWindow::eventFilter(QObject *o, QEvent *e)
{ 
    if( o == kline_wall_main_ 
#ifdef MAKE_SUB_WALL
        || o == kline_wall_sub_ 
#endif
        )
    {
        switch ( e->type() )
        {
        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonDblClick:
        case QEvent::MouseButtonRelease:
        case QEvent::MouseMove:
            return true;
        default: return false; // ndchk
        } 
    } 
    return false; // 20120216
}

void MainWindow::keyPressEvent(QKeyEvent *e)
{
    if( is_train_mode_ )
        return;
    switch(e->key())
    {
        case Qt::Key_F3:
        { 
            /*kline_wall_main_->ResetStock("999999", QString::fromLocal8Bit("上证指数"), true);
            if( kline_wall_sub_ )
            {
                kline_wall_sub_->ResetStock("999999", QString::fromLocal8Bit("上证指数"), true);
                if( kline_wall_sub_->isVisible() )
                {
                    kline_wall_main_->update();
                    kline_wall_sub_->update();
                }
            }*/
        } break;
        case Qt::Key_F5:
        {
            kline_wall_main_->show();
            code_list_wall_->hide();
        } break;
        case Qt::Key_F6:
        {
            kline_wall_main_->hide();
            if( kline_wall_sub_ )
                kline_wall_sub_->hide();
            code_list_wall_->show();
        } break;
        case Qt::Key_0: case Qt::Key_1: case Qt::Key_2: case Qt::Key_3: case Qt::Key_4:  
        case Qt::Key_5: case Qt::Key_6: case Qt::Key_7: case Qt::Key_8: case Qt::Key_9: 
        case Qt::Key_A: case Qt::Key_B: case Qt::Key_C: case Qt::Key_D: case Qt::Key_E:
        case Qt::Key_F: case Qt::Key_G: case Qt::Key_H: case Qt::Key_I: case Qt::Key_J:
        case Qt::Key_K: case Qt::Key_L: case Qt::Key_M: case Qt::Key_N: case Qt::Key_O:
        case Qt::Key_P: case Qt::Key_Q: case Qt::Key_R: case Qt::Key_S: case Qt::Key_T:
        case Qt::Key_U: case Qt::Key_V: case Qt::Key_W: case Qt::Key_X: case Qt::Key_Y:
        case Qt::Key_Z:
		{
            if( (e->modifiers() & Qt::ControlModifier) )
                break;
            //qDebug() << "MainWindow::keyPressEvent " << e->key() << "\n";
            stock_input_dlg_.ui.stock_input->clear();
            char tmpbuf[8] = {0};
            if( e->key() >= Qt::Key_0 && e->key() <= Qt::Key_9 )
                sprintf_s(tmpbuf, sizeof(tmpbuf), "%c", (char)e->key());
            else
                sprintf_s(tmpbuf, sizeof(tmpbuf), "%c", char(e->key()+32));
            stock_input_dlg_.ui.stock_input->setText(tmpbuf);
            stock_input_dlg_.showNormal();
        } break;

        default:
            break;
    }
    e->ignore();
}

void MainWindow::onTimer()
{
    // updateDateTime();  1015-1030
    //int hhmmss = QDateTime::currentDateTime().toString("hhmmss").toInt();
    int hh = QDateTime::currentDateTime().time().hour();
    int mm = QDateTime::currentDateTime().time().minute();
    int ss = QDateTime::currentDateTime().time().second();

    int min_5_amain_second = (5 - (hh * 60 + mm) % 5) * 60 - ss; 
    int min_15_amain_second = (15 - (hh * 60 + mm) % 15) * 60 - ss; 

     
    //hhmm / 100 * 60 - 9 * 60
    QString content = QString("%1                                         %2                 5M: %3     \t\t     15M: %4")
        .arg(QDateTime::currentDateTime().toString("yyyyMMdd hh:mm:ss"))
        .arg(QString::number(cur_quote_price(), 'f', 1))
        .arg(min_5_amain_second)
        .arg(min_15_amain_second);

    statusBar()->showMessage(content);

}

void MainWindow::on_actionExit_triggered()
{
    this->close();
}

void MainWindow::updateDateTime()
{
#ifdef USE_STATUS_BAR
    ui->labelCurrentTime->setText(
            QDateTime::currentDateTime().toString("yyyy-MM-dd HH:MM:ss"));
#endif
}
  
void MainWindow::onMainKwallCycleChange(int /*index*/)
{
    assert(kline_wall_main_);
    tool_bar_->main_cycle_comb()->clearFocus();
    kline_wall_main_->ResetTypePeriod( TypePeriod(tool_bar_->main_cycle_comb()->currentData().toInt()) );
}

void MainWindow::onSubKwallCycleChange(int /*index*/)
{
    if( !kline_wall_sub_ )
        return;
    tool_bar_->sub_cycle_comb()->clearFocus();
    
    TypePeriod target_type_period = TypePeriod(tool_bar_->sub_cycle_comb()->currentData().toInt());
    
    if( is_train_mode() )
    {
        kline_wall_sub_->ResetTypePeriodTrain(target_type_period, kline_wall_ori_step_->train_start_date(), kline_wall_ori_step_->train_end_date());
        // 1) get sub wall's  pre k date time, and cur k date time
        // 2) calculate ori k  data 's high low price in  duration which step 1 get
        if( kline_wall_ori_step_->k_cur_train_date() > 0 )
        {
            if( kline_wall_sub_->k_type() == kline_wall_ori_step_->k_type() )
                kline_wall_sub_->SetTrainByRendIndex(kline_wall_ori_step_->k_rend_index_for_train());
            else
            {
                T_StockHisDataItem* p_item = kline_wall_sub_->SetTrainStartEnd(target_type_period
                    , kline_wall_ori_step_->k_cur_train_date(), kline_wall_ori_step_->k_cur_train_hhmm()
                    , kline_wall_ori_step_->train_end_date(), DEFAULT_TRAIN_END_HHMM);
                T_StockHisDataItem* p_pre_item = kline_wall_sub_->TrainStockDataItem(kline_wall_sub_->k_rend_index_for_train() + 1);
                if( p_item && p_pre_item )
                {
                    std::tuple<double, double> high_low;
                    bool ret = kline_wall_ori_step_->CaculateHighLowPriceForHighPeriod(target_type_period, p_item->date, p_item->hhmmss, p_pre_item->date, p_pre_item->hhmmss
                                    , kline_wall_ori_step_->k_rend_index_for_train(), high_low);
                    if( ret )
                    {
                    p_item->high_price = std::get<0>(high_low);
                    p_item->low_price = std::get<1>(high_low);
                    }
                } 
            }
        }

    }else // not train mode 
    {
        kline_wall_sub_->ResetTypePeriod(target_type_period);
        if( kline_wall_main_->k_cur_train_date() > 0 )
        {
            kline_wall_sub_->ShowDurationKlines(kline_wall_main_->k_cur_train_date(), kline_wall_main_->k_cur_train_hhmm());
            // ndchk:
            kline_wall_sub_->SetTrainStartDateTime(TypePeriod(tool_bar_->sub_cycle_comb()->currentData().toInt())
                , kline_wall_main_->k_cur_train_date(), kline_wall_main_->k_cur_train_hhmm());
        }
    }
        
}

void MainWindow::OnShowAutoForcast(bool val)
{
    is_show_autoforcast(val);
    if( val )
        kline_wall_main_->HandleAutoForcast_large();
}

void MainWindow::OnBackTest()
{ 
    if( !app_->is_use_fenbi() )
    {
        bool pre_state = is_auto_next_k();
        is_auto_next_k(!pre_state);

    }else 
    {
        if( !is_run_backtest_ )
        {
            //------------------------
            const T_StockHisDataItem & cur_item = kline_wall_main_->CurStockDataItem();
            assert(cur_item.date > 0);

            QuoteCallBackData  quote_call_back_data;
            memset(&quote_call_back_data, 0, sizeof(quote_call_back_data));

            quote_call_back_data.quote_call_back = QuoteCallBackFunc;
            quote_call_back_data.para = this;
            strcpy_s(quote_call_back_data.code, sizeof(quote_call_back_data.code), app_->config_man().contract_info().code.c_str()); 
            quote_call_back_data.each_delay_ms = 1;
            quote_call_back_data.date_begin = cur_item.date;
            quote_call_back_data.hhmm_begin = cur_item.hhmmss;
            //quote_call_back_data.date_end = app_->exchange_calendar()->NextTradeDate(cur_item.date, 30*12); //20190228;
            quote_call_back_data.date_end = app_->exchange_calendar()->PreTradeDate(app_->config_man().contract_info().end_date, 1); //20190228;
            quote_call_back_data.hhmm_end = 1500;

            //int reg_hhmm = (cur_item.hhmmss / 100) * 100 + 30;
            capital_curve_->Append(CapitalData(app_->strategy_man()->account_info().capital.total()
                                            , cur_item.date, cur_item.hhmmss));
            char error[256] = {'\0'};
            int ret = WinnerQuotation_Reg(&quote_call_back_data, error);
            if( ret < 1 )
            {
                QMessageBox::information(nullptr, QString::fromLocal8Bit("警告"), QString::fromLocal8Bit("WinnerQuotation_Reg 失败!"));
            }else
                is_run_backtest_ = true;
            //------------------------
        }else
        { 
            WinnerQuotation_UnReg();
            is_run_backtest_ = false;
        }
    }
}

void MainWindow::PauseBackTest(bool val)
{
    if( !app_->is_use_fenbi() )
        return;
    if( is_run_backtest_ )
    {
        if( val )
            WinnerQuotation_Pause();
        else
            WinnerQuotation_Start();
    }
}

void QuoteCallBackFunc(T_QuoteData *quote_data, void *para, char *ErrInfo)
{
    if( quote_data )
    {
        MainWindow *mainw = (MainWindow*)para;
        mainw->UpdateStockQuote(*quote_data);
    }else
    {
        if( ErrInfo )
        {

        }
    }
}
