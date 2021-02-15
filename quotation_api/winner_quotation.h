#ifndef WINNER_QUOTATION_DSFDSh_H_
#define WINNER_QUOTATION_DSFDSh_H_

//#include "winner_quotation_api.h"
#include <string>

#include "winner_quotation_api.h"

#define DATE_BEGIN_MIN  1981
#define API_NAME  "quotation_api"

class ExchangeCalendar;
class WinnerQuotation
{
public:
    WinnerQuotation();
    ~WinnerQuotation();

    int Initiate(ExchangeCalendar *exchange_calendar, char *ErrInfo);
    bool IsInitiated() { return is_inited_; }
    void MainLoop();

    void Shutdown();

    int Register(QuoteCallBackData *call_back_data, char *ErrInfo);
    int UnRegister();

    void Start();
    void Stop();
    void Pause();

private:
    void Handle();

private:
    ExchangeCalendar *exchange_calendar_;
    bool is_inited_;
    bool exit_flag_;
    bool handle_pause_flag_;
    bool handle_exit_flag_;
    bool is_handle_finish_;

    QuoteCallBackData quote_call_back_data_;
    /*void *quote_call_back_;
    unsigned int each_call_back_delay_;*/

   /* std::string code_;
    int start_date_;
    int start_hhmm_;*/

    std::string data_path_;
};

void Delay(unsigned short mseconds);

#endif