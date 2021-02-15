#include "winner_quotation_api.h"

#include <thread>
#include <windows.h>

#include "winner_quotation.h"

static WinnerQuotation* GetInstance(bool is_del=false);

extern "C" DLLIMEXPORT int __cdecl WinnerQuotation_Init(ExchangeCalendar *exchange_calendar, char *ErrInfo)
{
    return GetInstance()->Initiate(exchange_calendar, ErrInfo);
}

extern "C" DLLIMEXPORT void __cdecl WinnerQuotation_UnInit()
{
    GetInstance(true);
}

extern "C" DLLIMEXPORT int __cdecl WinnerQuotation_Reg(QuoteCallBackData *call_back_data, char *ErrInfo)
{
    /*if( !GetInstance()->is_connected() )
        return -1;*/
    //auto val = GetInstance()->RequestKData(Zqdm, date_begin, date_end, call_back_para, is_index, ErrInfo);
    if( !GetInstance()->IsInitiated() )
    {
        if( ErrInfo )
            strcpy(ErrInfo, "un initiated!");
        return -1;
    }
    if( !call_back_data )
    {
        if( ErrInfo )
            strcpy(ErrInfo, "call_back_data can't be null !");
        return -2;
    }
    auto val = GetInstance()->Register(call_back_data, ErrInfo);
    return val ? 1 : -2;
}

extern "C" DLLIMEXPORT int __cdecl WinnerQuotation_UnReg(/*QuoteCallBackData *call_back_data*/)
{
    if( !GetInstance()->UnRegister() )
        return 1;

}

extern "C" DLLIMEXPORT int __cdecl WinnerQuotation_Start()
{
    if( !GetInstance()->IsInitiated() )
        return -1;
    GetInstance()->Start();
    return 1;
}

extern "C" DLLIMEXPORT int __cdecl WinnerQuotation_Stop()
{
    if( !GetInstance()->IsInitiated() )
        return -1;
    GetInstance()->Stop();
    return 1;
}

extern "C" DLLIMEXPORT int __cdecl WinnerQuotation_Pause() 
{
    if( !GetInstance()->IsInitiated() )
        return -1;
    GetInstance()->Pause();
    return 1;
}

static WinnerQuotation* GetInstance(bool is_del)
{
    //static std::string pro_tag = TSystem::utility::ProjectTag("wzf");
    static WinnerQuotation * winn_quotation_obj = nullptr; 

    if( is_del )
    {
        if( winn_quotation_obj ) 
        {
            //winn_client_obj->FireShutdown(); 
            winn_quotation_obj->Shutdown(); 
        }
    }else if( !winn_quotation_obj )
    {
        std::thread man_thread([]()
        {
            winn_quotation_obj = new WinnerQuotation;
            winn_quotation_obj->MainLoop();

            delete winn_quotation_obj; 
            winn_quotation_obj = nullptr;
        });
        man_thread.detach();
    }

    // wait for winn_quotation_obj created 
    while( !is_del && !winn_quotation_obj )
        Sleep(10);
    return winn_quotation_obj;
}

