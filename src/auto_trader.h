#ifndef AUTO_TRADE_SDFS_H_
#define AUTO_TRADE_SDFS_H_

#include "position_account.h"

class FuturesForecastApp;
class AutoTrader
{
public:
    AutoTrader(FuturesForecastApp *app);

    void HandleTickerCallBak();

private:
    FuturesForecastApp *app_;

    AccountInfo  account_info;
};
#endif