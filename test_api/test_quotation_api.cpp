#ifdef TEST_QUOTAION_API

#include <cstdio>
#include <vector>
#include <iostream>
#include <algorithm>

#include <windows.h>

#include "..\include\winner_quotation_api.h"

#pragma comment(lib, "quotation_api.lib")

std::vector<std::string> StringSplit(const std::string &str, char separator);

//typedef void (*QuoteCallBack)(T_QuoteData *quote_data, void *para);
void QuoteCallBackFunc(T_QuoteData *quote_data, void *para)
{
    if( quote_data )
        printf("code:%s price:%f \n", quote_data->code, quote_data->price);
}

using namespace std;
int main()
{
    std::vector<double> temp_vector;
    temp_vector.push_back(2.1);
    temp_vector.push_back(1.3);
    temp_vector.push_back(1.3);
    temp_vector.push_back(66.7);
    static auto compare_price_distance = [](const double &lh, const double& rh)
    {
        //return lh.ab_distance >= rh.ab_distance;
        return lh > rh;
    };
    std::sort(temp_vector.begin(), temp_vector.end(), compare_price_distance);
    getchar();
    std::string src = "20190103,sc1904,INE, ,375.5,377.3,375.1,6618,375.5,375.5,375.5,10,3755000,6608,0,0,407.4,347.1,0,0,20:59:00,500,375.4,1,377.2,5,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,375500,20190102";
    
    auto index = src.find_last_of(',');
    if( index != std::string::npos )
    {
        std::string targ = src.substr(index + 1);
        targ = targ;
    }
    
    auto ret_vect = StringSplit(src, ',');
    auto hhmmss = ret_vect[20];
    auto temp_vect = StringSplit(hhmmss, ':');
    int hhmm = std::stoi(temp_vect[0]) * 100 + std::stoi(temp_vect[1]);
    auto business_date = ret_vect[43];
#if 0
    WinnerQuotation_Init();
    char error[256] = {'\0'};
    QuoteCallBackData call_back_data;
    call_back_data.quote_call_back = QuoteCallBackFunc;
    call_back_data.each_delay_ms = 50;
    auto ret = WinnerQuotation_Reg("sc1903", 20190101, 20190229
        , &call_back_data, false, error);
    if( ret > 0 )
        WinnerQuotation_Start();
    else
    {
        printf("fail");
        getchar();
    }
    int count = 0;
    while( true )
    {
        ++count;
        Sleep(500);
        if( count % 20 == 0 )
            WinnerQuotation_Pause();
        if( count % 30 == 0 )
            WinnerQuotation_Start();
    }

#endif
    return 0;
}
#endif


vector<string> StringSplit(const std::string &str, char separator)
{
    //string str;
    //char separator=',';
    string substr;    
    vector<string> vstr;
    int start=0;
    int index=0; 

    do {
        index = str.find_first_of(&separator,start);
        if ( index !=string::npos ) 
        {
            substr= str.substr(start,index-start);
            vstr.push_back(substr);

            start = str.find_first_not_of(&separator,index);
            if (start == string::npos)  
                break;
        }

    }while(index !=string::npos);
     
    if (start !=string::npos) 
    {
        substr = str.substr(start);
        vstr.push_back(substr);
    }

    int i = 0;
    for (vector<string>::iterator iter = vstr.begin(); iter !=vstr.end(); iter++, ++i) 
    {
        cout<< i << " " <<*iter<<endl;
    }
    return vstr;
}