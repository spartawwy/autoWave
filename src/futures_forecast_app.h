#ifndef STK_FORECAST_APP_SDF8533_H_
#define STK_FORECAST_APP_SDF8533_H_

#include <memory>

#include <QtWidgets/QApplication>
#include <QThread>

#include <TLib/tool/tsystem_server_client_appbase.h>
#include <TLib/core/tsystem_communication_common.h>
#include <TLib/core/tsystem_serialization.h>

#include "stock_data_man.h"
//#include "wave.h"

//class QTimer;
class CapitalCurve;
class ConfigMan;
class StrategyMan;
class ExchangeCalendar;
class DataBase;
class StockMan;
class MainWindow;
class KLineWall;
class WaveMan;
class FuturesForecastApp : public QApplication, public TSystem::ServerClientAppBase
{
    //Q_OBJECT

public:

    FuturesForecastApp(int argc, char* argv[]); 
    ~FuturesForecastApp();

    bool Init();
    void Stop();
    ConfigMan& config_man(){ return *config_man_;}

    std::shared_ptr<ExchangeCalendar>&  exchange_calendar() { return exchange_calendar_;}
    std::shared_ptr<DataBase>& data_base() { return data_base_; }

    StockDataMan & stock_data_man() { return *stock_data_man_; }
    WaveMan & wave_man() { return *wave_man_; }

    MainWindow * main_window() { return main_window_.get(); }
    CapitalCurve& capital_curve();

    void UpdateStockData(); 
    void UpdateStockQuoteOfTrainMode();

    void ClearStockHisDatas(const std::string &code);
    void ClearStockHisDatas(const std::string &code, TypePeriod type_period);
    // ps: ret > 0
    unsigned int GenerateForecastId(){ return ++forecast_id_; }

    bool is_use_fenbi(){ return is_use_fenbi_; }

    std::shared_ptr<StrategyMan>&  strategy_man() { return strategy_man_; }

protected:

    virtual void HandleNodeHandShake(TSystem::communication::Connection* , const TSystem::Message& ) override {};
    virtual void HandleNodeDisconnect(std::shared_ptr<TSystem::communication::Connection>& 
        , const TSystem::TError& ) override {};

private:
    void UpdateStockData(int target_date, int cur_hhmm, const std::string &code, TypePeriod  type_period, int nmarket);

private:

    std::shared_ptr<ConfigMan>  config_man_;
    std::shared_ptr<MainWindow>  main_window_;

    std::shared_ptr<DataBase>  data_base_;
    std::shared_ptr<ExchangeCalendar>  exchange_calendar_;
    //std::shared_ptr<StockMan>  stock_man_;

    std::shared_ptr<StockDataMan>  stock_data_man_;
    //-----------------------
    std::shared_ptr<WaveMan>  wave_man_;
    //-----------------------
    bool exit_flag_;
     
    std::atomic<unsigned int> forecast_id_;

    bool is_use_fenbi_;

    //std::shared_ptr<Strategy>  strategy_;
    std::shared_ptr<StrategyMan>  strategy_man_;

};


void Delay(__int64 mseconds);

#endif // STK_FORECAST_APP_SDF8533_H_