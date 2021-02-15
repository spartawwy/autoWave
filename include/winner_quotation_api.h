#ifndef WINNER_QUOTATION_API_DSFKJDSFDS_H_
#define WINNER_QUOTATION_API_DSFKJDSFDS_H_


#ifdef  QUOTATION_API_EXPORTS  
#define DLLIMEXPORT __declspec(dllexport) 
#else
#define DLLIMEXPORT __declspec(dllimport)
#endif

#include "sys_common.h"
#include "exchange_calendar.h"


/*
   返回值 若 <=0 一律作为失败 
*/
extern "C" DLLIMEXPORT int __cdecl WinnerQuotation_Init(ExchangeCalendar *exchange_calendar, char *ErrInfo);
extern "C" DLLIMEXPORT void __cdecl WinnerQuotation_UnInit();

//ps: call_back_data will cover old
extern "C" DLLIMEXPORT int __cdecl WinnerQuotation_Reg(QuoteCallBackData *call_back_data, char *ErrInfo);
extern "C" DLLIMEXPORT int __cdecl WinnerQuotation_UnReg(/*QuoteCallBackData *call_back_data*/);

extern "C" DLLIMEXPORT int __cdecl WinnerQuotation_Start();

extern "C" DLLIMEXPORT int __cdecl WinnerQuotation_Stop();

extern "C" DLLIMEXPORT int __cdecl WinnerQuotation_Pause();

#endif // WINNER_QUOTATION_API_DSFKJDSFDS_H_