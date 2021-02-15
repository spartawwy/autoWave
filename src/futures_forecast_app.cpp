#include "futures_forecast_app.h"

#include <thread>

#include <qmessageBox>
#include <QDateTime>
#include <qdebug.h>

#include <Tlib/core/tsystem_core_common.h>

#include "config_man.h"
#include "exchange_calendar.h"
#include "database.h"
#include "stock_man.h"
#include "stock_data_man.h"

#include "mainwindow.h"
#include "capital_curve.h"

#include "wave.h"

#include "strategy_man.h" 

#include "winner_quotation_api.h"
#pragma comment(lib, "quotation_api.lib")

static const bool cst_is_use_fenbi = false;

FuturesForecastApp::FuturesForecastApp(int argc, char* argv[])
    : QApplication(argc, argv)
    , ServerClientAppBase("client", "autoforcast", "0.1")
    , data_base_(nullptr)
    //, stock_man_(nullptr)
    , stock_data_man_(nullptr)
    , wave_man_(nullptr)
    , strategy_man_(nullptr)
    , main_window_(nullptr)
    , exit_flag_(false)
    , forecast_id_(0)
    , is_use_fenbi_(cst_is_use_fenbi)
{
}

FuturesForecastApp::~FuturesForecastApp()
{
}

bool FuturesForecastApp::Init()
{
    option_dir_type(AppBase::DirType::STAND_ALONE_APP);
    option_validate_app(false);

    std::string cur_dir(".//");
    work_dir(cur_dir);
    local_logger_.SetDir(cur_dir);
    //---------------
    config_man_ = std::make_shared<ConfigMan>();
    if( !config_man_->LoadConfig("./config.ini") )
    {
        QMessageBox::information(nullptr, QString::fromLocal8Bit("警告"), QString::fromLocal8Bit("读取配置文件config.ini失败!"));
        return false;
    } 

    data_base_ = std::make_shared<DataBase>(this);
    if( !data_base_->Initialize() )
    {
        QMessageBox::information(nullptr, QString::fromLocal8Bit("警告"), QString::fromLocal8Bit("数据库初始化失败!"));
        return false;
    }
    //stock_man_ = std::make_shared<StockMan>();
    //data_base_->LoadAllStockBaseInfo(stock_man_);

    exchange_calendar_ = std::make_shared<ExchangeCalendar>();
    data_base_->LoadTradeDate(exchange_calendar_.get());

    stock_data_man_ = std::make_shared<StockDataMan>(exchange_calendar_.get(), local_logger(), *data_base_);
    if( !stock_data_man_->Init() )
    {
        QMessageBox::information(nullptr, QString::fromLocal8Bit("警告"), QString::fromLocal8Bit("stock_data_man构件初始化失败!"));
        return false;
    }
    /*if( !stock_man_->Initialize() )
    {
        QMessageBox::information(nullptr, QString::fromLocal8Bit("警告"), QString::fromLocal8Bit("stock_man构件初始化失败!"));
        return false;
    }*/
    if( is_use_fenbi_ )
    {
        char error[256] = {'\0'};
        int ret = WinnerQuotation_Init(exchange_calendar_.get(), error);
        if( ret < 1 )
        {
            QMessageBox::information(nullptr, QString::fromLocal8Bit("警告"), QString::fromLocal8Bit("WinnerQuotation_Init 初始化失败!"));
            return false;
        }
    }

    //T_HisDataItemContainer &kdata_container = stock_data_man_->GetHisDataContainer(DEFAULT_MAINKWALL_TYPE_PERIOD, config_man_->contract_info().code);
    wave_man_ = std::make_shared<WaveMan>(*this);

    main_window_ = std::make_shared<MainWindow>(this);
    if( !main_window_->Initialize() )
        return false;  
    main_window_->show();

    strategy_man_ = std::make_shared<StrategyMan>(*this);
    if( !strategy_man_->Init() )
    {
        QMessageBox::information(nullptr, QString::fromLocal8Bit("警告"), QString::fromLocal8Bit("策略工厂 初始化失败!"));
        return false;
    }

    //---------------
    this->task_pool().PostTask([this]()
    {
        int count = 0;
        int milli_second = 300;  //500
        while( !exit_flag_ )
        {
            Delay(milli_second);
            if( exit_flag_ )
                break;
 
            if( !is_use_fenbi_ && this->main_window_->is_auto_next_k() )  
            {
                if( ++count % 2 == 0 ) // 6 4
                    this->UpdateStockData();
            }
 
#if 0
            if( !this->main_window_->is_train_mode() )  
            {
                if( ++count % 4 == 0 ) // 
                    this->UpdateStockData();
                this->UpdateStockQuoteOfTrainMode();
            }
#endif 
        }
    });

    
    return true;
}

void FuturesForecastApp::Stop()
{
    exit_flag_ = true;
    if( is_use_fenbi_ )
        WinnerQuotation_UnInit();
    Shutdown();
    this->quit();
}

void FuturesForecastApp::UpdateStockData()
{ 
#if 0 
    int cur_date = QDate::currentDate().year() * 10000 + QDate::currentDate().month() * 100 + QDate::currentDate().day();
    int cur_hhmm = QTime::currentTime().hour() * 100 + QTime::currentTime().minute();
    int target_date = 0;

    int pre_date = exchange_calendar()->PreTradeDate(cur_date, 1);
    bool is_trade_time = false;
    if( exchange_calendar()->IsTradeDate(cur_date) && exchange_calendar()->IsTradeTime(cur_hhmm) )
    {
        is_trade_time = true;
        target_date = cur_date;
    }else if( exchange_calendar()->IsTradeDate(pre_date) && exchange_calendar()->IsMidNightTradeTime(cur_hhmm) )
    {
        is_trade_time = true;
        target_date = exchange_calendar()->NextTradeDate(cur_date, 1);
    }
    if( !is_trade_time )
        return;
#endif

     //main_window()->UpdateStockData(target_date, cur_hhmm);// update main and sub window stock data
     main_window()->UpdateStockData(0, 0);
     ////////////////////////////////////
#if 0    
     TypePeriod  type_periods_to_judge[] = {TypePeriod::PERIOD_1M, TypePeriod::PERIOD_5M, TypePeriod::PERIOD_15M, TypePeriod::PERIOD_30M, TypePeriod::PERIOD_HOUR
         , TypePeriod::PERIOD_DAY, TypePeriod::PERIOD_WEEK}; 

     if( !main_window()->MainKlineWall() )
         return;

     std::vector<TypePeriod> target_periods;

     // update other k period stock data which has been opened----------------
                   
     for( int i = 0; i < sizeof(type_periods_to_judge)/sizeof(type_periods_to_judge[0]); ++i )
     {
         if( main_window()->MainKlineWall()->k_type() == type_periods_to_judge[i] )
             continue;
         if( main_window()->SubKlineWall() && main_window()->SubKlineWall()->k_type() == type_periods_to_judge[i] )
             continue; 

         T_HisDataItemContainer &container = stock_data_man_->GetHisDataContainer(type_periods_to_judge[i], main_window()->MainKlineWall()->stock_code());
         if( container.empty() ) // has not opened
             continue;

         target_periods.push_back(type_periods_to_judge[i]);
     }

     std::string code = main_window()->MainKlineWall()->stock_code();
     int nmarket = main_window()->MainKlineWall()->nmarket();
     
     for( int j = 0; j < target_periods.size(); ++j )
     {
         UpdateStockData(target_date, cur_hhmm, code, target_periods[j], nmarket);
     }
#endif
}

//ps: for other period data which is not current kwall(main, sub)
void FuturesForecastApp::UpdateStockData(int target_date, int cur_hhmm, const std::string &code, TypePeriod type_period, int nmarket)
{
    //bool is_need_updated = false;
    int hhmm = GetKDataTargetStartTime(type_period, cur_hhmm);
    T_HisDataItemContainer &container = stock_data_man().GetHisDataContainer(ToPeriodType(type_period), code);
    if( !container.empty() )
    {
        auto p_contain = stock_data_man().FindStockData(ToPeriodType(type_period), code, target_date, target_date, hhmm/*, bool is_index*/);
        if( p_contain ) // current time k data exists
        { 
            //local_logger().LogLocal(TSystem::utility::FormatStr("UpdateStockData k ex %d %04d tp:%d", target_date, cur_hhmm, type_period));
            double quote_price = 0.0;
            int ret = stock_data_man().UpdateOrAppendLatestItemStockData(ToPeriodType(type_period), nmarket, code, quote_price, false);
            if( ret == 1 ) // if updated last item
                TraverSetSignale(type_period, container, 0, DEFAULT_TRAVERSE_LEFT_K_NUM);
            else if( ret == 2 ) // if appended an item
            {
                TraverSetSignale(type_period, container);
                stock_data_man().TraverseGetStuctLines(ToPeriodType(type_period), code, 0, container);
            }
            
        }else
        {
            int start_index = 100;
            int len = 50;
            stock_data_man().LoadPartStockData(ToPeriodType(type_period), nmarket, code, start_index, len, false); 
#if 0
            int pre_k_num = container.size();
            auto date_time = GetKDataTargetDateTime(*exchange_calendar(), type_period, target_date, cur_hhmm, WOKRPLACE_DEFUALT_K_NUM);
            auto p_cur_time_contain = stock_data_man().AppendStockData(ToPeriodType(type_period), nmarket, code, std::get<0>(date_time), target_date, false);
            local_logger().LogLocal(TSystem::utility::FormatStr("UpdateStockData k app <%d %d %d> tp:%d %d %d", std::get<0>(date_time), std::get<1>(date_time), target_date, type_period, pre_k_num, p_cur_time_contain->size()));
#endif
            TraverSetSignale(type_period, container);
            stock_data_man().TraverseGetStuctLines(ToPeriodType(type_period), code, 0, container);
        }
    }
}

void FuturesForecastApp::ClearStockHisDatas(const std::string &code)
{
    PeriodType  type_array[] = { PeriodType::PERIOD_1M, PeriodType::PERIOD_5M, PeriodType::PERIOD_15M, PeriodType::PERIOD_30M, PeriodType::PERIOD_HOUR, PeriodType::PERIOD_DAY, PeriodType::PERIOD_WEEK, PeriodType::PERIOD_MON };
    for( unsigned int i = 0; i < sizeof(type_array)/sizeof(type_array[0]); ++i )
    {
        T_HisDataItemContainer * p_container = stock_data_man().FindHisDataContainer(type_array[i], code);
        if( p_container )
            p_container->clear();
    }
}

void FuturesForecastApp::ClearStockHisDatas(const std::string &code, TypePeriod type_period)
{
    T_HisDataItemContainer * p_container = stock_data_man().FindHisDataContainer(ToPeriodType(type_period), code);
    if( p_container )
        p_container->clear();
}

void FuturesForecastApp::UpdateStockQuoteOfTrainMode()
{
    main_window()->UpdateStockQuoteOfTrainMode();
}

CapitalCurve& FuturesForecastApp::capital_curve()
{
    return main_window_->capital_curve();
}

void Delay(__int64 mseconds)
{ 
    std::this_thread::sleep_for(std::chrono::system_clock::duration(std::chrono::milliseconds(mseconds)));
}