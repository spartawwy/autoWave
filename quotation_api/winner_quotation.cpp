#include "winner_quotation.h"

#include <io.h>
#include <iostream>
#include <fstream>
#include <regex>
#include <thread>
#include <chrono>
#include <cassert>

#include <windows.h>

#include "winner_quotation_api.h"
#include "file_mapping.h"
#include "sys_common.h"

#pragma comment(lib, "sys_common.lib")

std::tuple<unsigned int, unsigned int>  LocateStartPos(FileMapping &file_map, QuoteCallBackData &quote_call_back_data, int target_nature_date_begin);

//bool IsLegalTradeHHmm(int hhmm);
static WinnerQuotation* GetInstance(bool is_del = false);

WinnerQuotation::WinnerQuotation()
    : exchange_calendar_(nullptr)
    , is_inited_(false)
    , exit_flag_(false)
    , handle_pause_flag_(false)
    , handle_exit_flag_(false)
    , is_handle_finish_(false)
{
    memset(&quote_call_back_data_, 0, sizeof(quote_call_back_data_));
}

WinnerQuotation::~WinnerQuotation()
{
    exit_flag_ = true;
    is_inited_ = false;
}

int WinnerQuotation::Initiate(ExchangeCalendar *exchange_calendar, char *ErrInfo)
{
    if( !exchange_calendar )
    {
        if( ErrInfo )
            strcpy(ErrInfo, "ExchangeCalendar can't be null!");
        return -1;
    }
    exchange_calendar_ = exchange_calendar;
    exit_flag_ = false;
    handle_pause_flag_ = false;
    handle_exit_flag_ = false;
    is_handle_finish_ = false;
    is_inited_ = true;
    //char data_path[1024] = {'\0'};
    char *p_data_env = getenv("FUTURE_DATA_PATH");
    if( !p_data_env )
    {
        if( ErrInfo )
            strcpy(ErrInfo, "env FUTURE_DATA_PATH hasn't been set!");
        return -1;
    }
    data_path_ = p_data_env;
    return 1;
}

int WinnerQuotation::Register(QuoteCallBackData *call_back_data, char *ErrInfo)
{
    if( !is_inited_ )
    {
        if( ErrInfo )
            strcpy(ErrInfo, "quotation api hasn't been Initiated!");
        return -1;
    }
    assert(exchange_calendar_);
    if( !call_back_data || !call_back_data->quote_call_back)
    {
        if( ErrInfo )
            strcpy(ErrInfo, "call_back_data and call_back_data.quote_call_back can't be null!");
        return -1;
    }
    if( !exchange_calendar_->IsTradeDate(call_back_data->date_begin) )
    {
        if( ErrInfo )
            strcpy(ErrInfo, "call_back_data date_begin is not trade date!");
        return -1;
    }
    if( !exchange_calendar_->IsTradeDate(call_back_data->date_end) )
    {
        if( ErrInfo )
            strcpy(ErrInfo, "call_back_data date_end is not trade date!");
        return -1;
    }
    if( call_back_data->date_begin > call_back_data->date_end )
    {
        if( ErrInfo )
            strcpy(ErrInfo, "call_back_data date_begin bigger than date_end");
        return -1;
    }

    if( !IsLegalTradeHHmm(call_back_data->hhmm_begin) || !IsLegalTradeHHmm(call_back_data->hhmm_end) )
    {
        if( ErrInfo )
            strcpy(ErrInfo, "hhmm_begin or hhmm_end is illegal trade time");
        return -1;
    }
    Stop();

    quote_call_back_data_.quote_call_back = call_back_data->quote_call_back;
    quote_call_back_data_.para = call_back_data->para;
    quote_call_back_data_.each_delay_ms = call_back_data->each_delay_ms;
    strcpy_s(quote_call_back_data_.code, sizeof(quote_call_back_data_.code), call_back_data->code);
    quote_call_back_data_.date_begin = call_back_data->date_begin;
    quote_call_back_data_.hhmm_begin = call_back_data->hhmm_begin;
    quote_call_back_data_.date_end = call_back_data->date_end;
    quote_call_back_data_.hhmm_end = call_back_data->hhmm_end;
     
    Start();
    return 1;
}


int WinnerQuotation::UnRegister()
{
    if( !is_inited_ )
    { 
        return 1;
    }
    Stop();
    while( !is_handle_finish_ )
        Delay(50);
    //quote_call_back_data_.quote_call_back = nullptr;
    memset(&quote_call_back_data_, 0, sizeof(quote_call_back_data_));
    return 1;
}

void WinnerQuotation::Start()
{
    handle_pause_flag_ = false;
    handle_exit_flag_ = false;
    is_handle_finish_ = false;
}

void WinnerQuotation::Stop()
{
    handle_pause_flag_ = false;
    handle_exit_flag_ = true;
    //is_handle_finish_ = true;
}

void WinnerQuotation::Pause()
{
    handle_pause_flag_ = true;
}

void WinnerQuotation::MainLoop()
{
    while(!exit_flag_)
    {
        Sleep(500);
        if( !is_handle_finish_ && quote_call_back_data_.quote_call_back && quote_call_back_data_.date_begin > DATE_BEGIN_MIN )
        {
            Handle();
        }
    }
}

void WinnerQuotation::Shutdown()
{
    exit_flag_ = true;
    Delay(100);
}

#if 0
void WinnerQuotation::Handle()
{
    assert(exchange_calendar_);
    assert(quote_call_back_data_.quote_call_back);

    
    // because fenbi data which after 21:00 save in next date'file, it's need + 1
    const int real_f_date_begin = quote_call_back_data_.hhmm_begin >= 2100 ? exchange_calendar_->NextTradeDate(quote_call_back_data_.date_begin, 1) : quote_call_back_data_.date_begin;
    const int real_f_date_end = quote_call_back_data_.hhmm_end >= 2100 ? exchange_calendar_->NextTradeDate(quote_call_back_data_.date_end, 1) : quote_call_back_data_.date_end;
    int cur_f_date = real_f_date_begin; 
    //std::string full_path = "F:/StockHisdata/SCL8/201901/20190102/sc1903_20190102.csv";

    //--------------------for all trade date's data -------------------------
    do 
    { 

    char full_path[1024] = {'\0'};
    int yymm = cur_f_date/100;
    sprintf_s(full_path, sizeof(full_path), "%s/%d/%d/%s_%d.csv"
        , data_path_.c_str(), yymm, cur_f_date, quote_call_back_data_.code, cur_f_date);
     
    std::fstream _file;
    if( (_access( full_path, _A_NORMAL )) == -1 )
    {
        WriteLog(API_NAME, "WinnerQuotation::Handle file:%s not exist", full_path);
        char error[256] = {'\0'};
        sprintf_s(error, sizeof(error), "open %s fail", full_path);
        quote_call_back_data_.quote_call_back(nullptr, quote_call_back_data_.para, error);
        is_handle_finish_ = true;
        return;
    } 
    //std::string id;
    int date;
    int hour;
    int minute;
    int second;
    double price;
    //int change_price;
    int cur_total_vol; 

    std::string partten_string =
    "^(\\d{8}),(\\S+),(\\S+),(\\s+),(\\d+\\.\\d+|\\d+),(\\d+\\.\\d+|\\d+),(\\d+\\.\\d+|\\d+),(\\d+),(\\d+\\.\\d+|\\d+),(\\d+\\.\\d+|\\d+),(\\d+\\.\\d+|\\d+),(\\d+),(\\d+),(\\d+),(\\d+\\.\\d+|\\d+),(\\d+\\.\\d+|\\d+),(\\d+\\.\\d+|\\d+),(\\d+\\.\\d+|\\d+),(\\d+),(\\d+),(\\d{2}):(\\d{2}):(\\d{2}),(\\d+),(\\d+\\.\\d+|\\d+),(\\d+),(\\d+\\.\\d+|\\d+),(\\d+),(\\d+\\.\\d+|\\d+),(\\d+),(\\d+\\.\\d+|\\d+),(\\d+),(.*)$";

    std::regex regex_obj(partten_string); 
     
    char buf[1024] = {0};
    std::fstream in(full_path);
    if( !in.is_open() )
    {
        char error[256] = {'\0'};
        sprintf_s(error, sizeof(error), "open %s fail", full_path);
        WriteLog(API_NAME, "WinnerQuotation::Handle %s", error);
        quote_call_back_data_.quote_call_back(nullptr, quote_call_back_data_.para, error);
        is_handle_finish_ = true;
        return;
    }
    int line = 0;
    while( !in.eof() && !exit_flag_ && !handle_exit_flag_ )
    { 
        if( line++ % 100 == 0 )
            Delay(1);

        in.getline(buf, sizeof(buf)); 
        if( line == 1 )
            continue;

        //Delay(each_call_back_delay_);
        
        int len = strlen(buf);
        if( len < 1 )
            continue;

        while( handle_pause_flag_ )
            Delay(50);

        char *p0 = buf;
        char *p1 = &buf[len-1];
        std::string src(p0, p1);

        std::smatch result; 
        if( std::regex_match(src.cbegin(), src.cend(), result, regex_obj) )
        {  //std::cout << result[1] << " " << result[2] << " " << result[3] << " " << result[4] << std::endl;
            try
            { 
                date = std::stoi(result[1]);
                std::string code = result[2];
                hour = std::stoi(result[21]);   //result[21];
                minute = std::stoi(result[22]); //result[22]; 
                second = std::stoi(result[23]); //result[23];
                int msecond = std::stoi(result[24]);  
                price = std::stod(result[5]);   //result[5];

                cur_total_vol = std::stoi(result[12]); //result[12];
                double b_1 = std::stod(result[25]);  
                int v_b_1 = std::stoi(result[26]);
                double s_1 = std::stod(result[27]); 
                int v_s_1 = std::stoi(result[28]);

                std::cout << date << " " << hour << ":" << minute << ":" << second << " " << price << " " << cur_total_vol << std::endl;
                int hhmm = hour * 100 + minute;
                if( !IsLegalTradeHHmm(hhmm) )
                    continue;
                if( hhmm >= 2100 )
                    date = exchange_calendar_->PreTradeDate(date, 1);
                if( date < quote_call_back_data_.date_begin || date == quote_call_back_data_.date_begin && hhmm < quote_call_back_data_.hhmm_begin )
                    continue;
                T_QuoteData quote_data;
                quote_data.date = date;
                strcpy(quote_data.code, code.c_str());
                quote_data.hhmmss = hour * 10000 + minute * 100 + second;
                quote_data.msecond = msecond;
                quote_data.b_1 = b_1;
                quote_data.v_b_1 = v_b_1;
                quote_data.s_1 = s_1;
                quote_data.v_s_1 = v_s_1;
                quote_data.price = price;
                quote_call_back_data_.quote_call_back(&quote_data, quote_call_back_data_.para, "");
                if( date > quote_call_back_data_.date_end
                    || date == quote_call_back_data_.date_end && hhmm >= quote_call_back_data_.hhmm_end )
                {
                    is_handle_finish_ = true;
                    return;
                }

            }catch(std::exception &e)
            {
                //printf("exception:%s", e.what());
                WriteLog(API_NAME, "WinnerQuotation::Handle %s exception:%s", full_path, e.what());
                continue;
            }catch(...)
            {
                WriteLog(API_NAME, "WinnerQuotation::Handle %s exception", full_path);
                continue;
            }
        }
        //--------------------
        if( *p1 == '\0' ) break; 
        if( int(*p1) == 0x0A ) // filter 0x0A
            ++p1;
    } // while !eof

    cur_f_date = exchange_calendar_->NextTradeDate(cur_f_date, 1);

    } while(cur_f_date <= real_f_date_end);
}

#else 

void WinnerQuotation::Handle()
{
    assert(exchange_calendar_);
    assert(quote_call_back_data_.quote_call_back);
    static auto get_para_relate_nature_date = [](ExchangeCalendar & calender, const QuoteCallBackData &quote_call_back_data, bool is_begin)->int
    { 
        int hhmm = is_begin ? quote_call_back_data.hhmm_begin : quote_call_back_data.hhmm_end;
        int date = is_begin ? quote_call_back_data.date_begin : quote_call_back_data.date_end;
        if( hhmm < 2055 )
        {
            if( hhmm < 235 )// night trading time which after midnight. para is next trade date
            { 
                int temp_date = calender.PreTradeDate(date, 1);
                return calender.DateAddDays(temp_date, 1);
            }
            else // day trading time. para is nature date
                return date;
        }else // night trading time which before midnight. para is nature date
        {
            return date;
        }
    };
    // because fenbi data which after 21:00 save in next date'file, it's need + 1
    const int real_f_date_begin = quote_call_back_data_.hhmm_begin >= 2100 ? exchange_calendar_->NextTradeDate(quote_call_back_data_.date_begin, 1) : quote_call_back_data_.date_begin;
    const int real_f_date_end = quote_call_back_data_.hhmm_end >= 2100 ? exchange_calendar_->NextTradeDate(quote_call_back_data_.date_end, 1) : quote_call_back_data_.date_end;
    int cur_f_date = real_f_date_begin; 
    //std::string full_path = "F:/StockHisdata/SCL8/201901/20190102/sc1903_20190102.csv";

    //--------------------for all trade date's data -------------------------
    // in fenbi file each line, the end is business date
    // 20190103,sc1904,INE, ,375.5,377.3,375.1,6618,375.5,375.5,375.5,10,3755000,6608,0,0,407.4,347.1,0,0,20:59:00,500,375.4,1,377.2,5,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,375500,20190102
    std::string partten_string =
        "^(\\d{8}),(\\S+),(\\S+),(\\s+),(\\d+\\.\\d+|\\d+),(\\d+\\.\\d+|\\d+),(\\d+\\.\\d+|\\d+),(\\d+),(\\d+\\.\\d+|\\d+),(\\d+\\.\\d+|\\d+),(\\d+\\.\\d+|\\d+),(\\d+),(\\d+),(\\d+),(\\d+\\.\\d+|\\d+),(\\d+\\.\\d+|\\d+),(\\d+\\.\\d+|\\d+),(\\d+\\.\\d+|\\d+),(\\d+),(\\d+),(\\d{2}):(\\d{2}):(\\d{2}),(\\d+),(\\d+\\.\\d+|\\d+),(\\d+),(\\d+\\.\\d+|\\d+),(\\d+),(\\d+\\.\\d+|\\d+),(\\d+),(\\d+\\.\\d+|\\d+),(\\d+),(.*)$";

    std::regex regex_obj(partten_string); 
    do 
    {  
        char full_path[1024] = {'\0'};
        int yymm = cur_f_date/100;
        sprintf_s(full_path, sizeof(full_path), "%s/%d/%d/%s_%d.csv"
            , data_path_.c_str(), yymm, cur_f_date, quote_call_back_data_.code, cur_f_date);

        std::fstream _file;
        if( (_access( full_path, _A_NORMAL )) == -1 )
        {
            WriteLog(API_NAME, "WinnerQuotation::Handle file:%s not exist", full_path);
            char error[256] = {'\0'};
            sprintf_s(error, sizeof(error), "open %s fail", full_path);
            quote_call_back_data_.quote_call_back(nullptr, quote_call_back_data_.para, error);
            is_handle_finish_ = true;
            return;
        } 
        FileMapping file_map;
        if( !file_map.Create(full_path) )
        {
            char error[256] = {'\0'};
            sprintf_s(error, sizeof(error), "file map %s fail", full_path);
            WriteLog(API_NAME, "WinnerQuotation::Handle %s", error);
            quote_call_back_data_.quote_call_back(nullptr, quote_call_back_data_.para, error);
            is_handle_finish_ = true;
            return;
        }
        
        const int target_nature_date_begin = get_para_relate_nature_date(*exchange_calendar_, quote_call_back_data_, true);
        const int target_nature_date_end = get_para_relate_nature_date(*exchange_calendar_, quote_call_back_data_, false);
        //std::string id;
        //int date;
        int hour;
        int minute;
        int second;
        double price;
        //int change_price;
        int cur_total_vol; 
        //-------------------------locate start pos----------------------------
        auto rel_ret = LocateStartPos(file_map, quote_call_back_data_, target_nature_date_begin);
        if( std::get<0>(rel_ret) == BAD_POS )
            continue;
        //---------------------------------------------------------------------
        std::string src; 
        //int line = 0;
        //for( unsigned long len = 0, total_len = 0; !exit_flag_ && !handle_exit_flag_ && (len = file_map.get_line(total_len, src)) > 0; total_len += len )
        int line = std::get<1>(rel_ret);
        for( unsigned long len = 0, total_len = std::get<0>(rel_ret); !exit_flag_ && !handle_exit_flag_ && (len = file_map.get_line(total_len, src)) > 0; total_len += len )
        {  
            if( line++ % 100 == 0 )
            {
                Delay(1);
            }
             
            if( line == 1 || len < 10)
            {
                continue;
            }

            while( handle_pause_flag_ )
                Delay(50);

            // for fast find begin line ----------------
            auto index = src.find_last_of(',');
            if( index == std::string::npos )
                continue;
            auto ret_vect = StringSplit(src, ',');
            if( ret_vect.size() < 44 )
                continue;
            auto nature_date_str = ret_vect[43];
            if( !IsNumber(nature_date_str) )
                continue;
            auto nature_date = std::stoi(nature_date_str);
            if( nature_date < target_nature_date_begin )
                continue;
            auto hhmmss_str = ret_vect[20];
            auto temp_vect = StringSplit(hhmmss_str, ':');
            if( temp_vect.size() < 3 )
                continue;
            int hhmm = std::stoi(temp_vect[0]) * 100 + std::stoi(temp_vect[1]);
            if( nature_date == target_nature_date_begin && hhmm < quote_call_back_data_.hhmm_begin )
                continue;
             
            try
            { 
                int trade_date = std::stoi(ret_vect[0]);
                std::string code = ret_vect[1];
                int second = std::stoi(temp_vect[2]);
                int msecond = std::stoi(ret_vect[21]);  
                price = std::stod(ret_vect[4]);   
                cur_total_vol = std::stoi(ret_vect[11]); 
                double b_1 = std::stod(ret_vect[22]);  
                int v_b_1 = std::stoi(ret_vect[23]);
                double s_1 = std::stod(ret_vect[24]); 
                int v_s_1 = std::stoi(ret_vect[25]);
             
                T_QuoteData quote_data;
                memset(&quote_data, 0, sizeof(quote_data));
                if( hhmm > 2000 )
                    quote_data.date = exchange_calendar_->PreTradeDate(trade_date, 1);
                else 
                    quote_data.date = trade_date;

                strcpy(quote_data.code, code.c_str());
                quote_data.hhmmss = hhmm * 100 + second;
                quote_data.msecond = msecond;
                quote_data.price = price;
                quote_data.b_1 = b_1;
                quote_data.v_b_1 = v_b_1;
                quote_data.s_1 = s_1;
                quote_data.v_s_1 = v_s_1;
                    
                quote_call_back_data_.quote_call_back(&quote_data, quote_call_back_data_.para, "");
                if( nature_date > target_nature_date_end
                    || nature_date == target_nature_date_end && hhmm >= quote_call_back_data_.hhmm_end )
                {
                    is_handle_finish_ = true;
                    return;
                }

            }catch(std::exception &e)
            {
                //printf("exception:%s", e.what());
                WriteLog(API_NAME, "WinnerQuotation::Handle %s exception:%s", full_path, e.what());
                continue;
            }catch(...)
            {
                WriteLog(API_NAME, "WinnerQuotation::Handle %s exception", full_path);
                continue;
            }
        } // for  each line

        cur_f_date = exchange_calendar_->NextTradeDate(cur_f_date, 1);

    } while( cur_f_date <= real_f_date_end && !exit_flag_ && !handle_exit_flag_ );
    is_handle_finish_ = true;
}
#endif

// <rel_start_pos, rel_line>
std::tuple<unsigned int, unsigned int> LocateStartPos(FileMapping &file_map, QuoteCallBackData &quote_call_back_data, int target_nature_date_begin)
{
    const auto total_line = file_map.total_line();
    if( total_line < 1 )
        return std::make_tuple(BAD_POS, -1);
     
    int left = 1;
    int right = total_line - 1; 
    int middle = 0;

    int bigger_near_by = -1;
    unsigned int rel_start_pos = 0;
    while( left <= right)
    {
        middle = (left + right) / 2;
        rel_start_pos = file_map.find_line_start(middle);
        assert(rel_start_pos != BAD_POS);
        std::string src; 
        if( file_map.get_line(rel_start_pos, src) <= 0 )
            return std::make_tuple(BAD_POS, -1);
        //------------------------
        auto index = src.find_last_of(',');
        if( index == std::string::npos )
        {
            left += 1;
            continue;
        }
        auto ret_vect = StringSplit(src, ',');
        if( ret_vect.size() < 44 )
        {
            left += 1;
            continue;
        }
        auto nature_date_str = ret_vect[43];
        if( !IsNumber(nature_date_str) )
        {
            left += 1;
            continue;
        }
        auto nature_date = std::stoi(nature_date_str);
        if( nature_date < target_nature_date_begin )
        {
            left = middle + 1;
            continue;
        }else if( nature_date > target_nature_date_begin )
        {
            right = middle - 1;
            bigger_near_by = middle;
            continue;
        }
        auto hhmmss_str = ret_vect[20];
        auto temp_vect = StringSplit(hhmmss_str, ':');
        if( temp_vect.size() < 3 )
            continue;
        int hhmm = std::stoi(temp_vect[0]) * 100 + std::stoi(temp_vect[1]);

        if( hhmm < quote_call_back_data.hhmm_begin )
        {
            left = middle + 1;
            continue;
        }else if( hhmm > quote_call_back_data.hhmm_begin )
        {
            right = middle - 1;
            bigger_near_by = middle;
            continue;
        }else
            return std::make_tuple(rel_start_pos, (unsigned int)middle);  
    }// while 
    if( bigger_near_by > -1 )
        return std::make_tuple(rel_start_pos, (unsigned int)bigger_near_by);  
    else
        return std::make_tuple(BAD_POS, -1);
}

void Delay(unsigned short mseconds)
{
    std::this_thread::sleep_for(std::chrono::system_clock::duration(std::chrono::milliseconds(mseconds)));
}