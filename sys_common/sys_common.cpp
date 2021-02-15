
#include "sys_common.h"

#include <boost/lexical_cast.hpp>
#include <algorithm>
#include <string>

bool IsNumber(const std::string& str)
{
	//bool ret = true;
	//std::for_each( std::begin(str), std::end(str), [&ret](char entry)
	for( unsigned int i = 0; i < str.length(); ++i )
	{
		if( str.at(i) < '0' || str.at(i) > '9' )
		  return false;
	}
	return true;
	/*{
		if( entry < '\0' || entry > '\9' )
		{
			ret = false;
		}
	}*/
}
 
bool IsStrAlpha(const std::string& str)
{
    try
    {
        auto iter = std::find_if_not( str.begin(), str.end(), [](int val) 
        { 
            if( val < 0 || val > 99999 ) 
                return 0;
            return isalpha(val);
        });
        return iter == str.end();
    }catch(...)
    {
        return false;
    }

}

bool IsStrNum(const std::string& str)
{
    try
    {
        auto iter = std::find_if_not( str.begin(), str.end(), [](int val) 
        { 
            if( val < 0 || val > 99999 ) 
                return 0;
            return isalnum(val);
        });
        return iter == str.end();
    }catch(...)
    {
        return false;
    }

}

bool IsDouble(const std::string& str)
{
    bool ret = true;
    try
    {
        double val = boost::lexical_cast<double>(str.c_str());
    }catch (boost::exception &)
    {
        ret = false;
    }catch(...)
    {
        ret = false;
    }
    return ret;
}

bool TransToDouble(const std::string& str, double &ret_val)
{
    ret_val = 0.0;
    bool ret = true;
    try
    {
        ret_val = boost::lexical_cast<double>(str.c_str());
    }catch (boost::exception &)
    {
        ret = false;
    }catch(...)
    {
        ret = false;
    }
    return ret;
}

double ProcDecimal(double val, unsigned int decimal)
{
    int temp = pow(10, decimal);
    int64_t big_val = int64_t(fabs(val * temp) + 0.5) * (val < 0 ? - 1 : 1); //4Éá5Èë
    return double(big_val) / temp;
}

bool Equal(double lh, double rh)
{
    return fabs(lh-rh) < 0.0001;
}

bool IsLegalTradeHHmm(int hhmm)
{
    int trd_win_index = -1;
    if( hhmm >= 900 && hhmm <= 1015 )
        trd_win_index = 0;
    else if( hhmm >= 1030 && hhmm <= 1130)
        trd_win_index = 1;
    else if( hhmm >= 1330 && hhmm <= 1500)
        trd_win_index = 2;
    else if( hhmm >= 2100 && hhmm <= 2359 || hhmm >= 0 && hhmm <= 230)
        trd_win_index = 3;
    return trd_win_index > -1;
}


std::vector<std::string> StringSplit(const std::string &str, char separator)
{
    //string str;
    //char separator=',';
    std::string substr;    
    std::vector<std::string> vstr;
    int start=0;
    int index=0; 

    do {
        index = str.find_first_of(&separator,start);
        if ( index != std::string::npos ) 
        {
            substr= str.substr(start,index-start);
            vstr.push_back(substr);

            start = str.find_first_not_of(&separator,index);
            if (start == std::string::npos)  
                break;
        }

    }while(index != std::string::npos);

    if (start != std::string::npos) 
    {
        substr = str.substr(start);
        vstr.push_back(substr);
    }

   /* for (std::vector<string>::iterator iter = vstr.begin(); iter != vstr.end(); iter++) 
    {
        cout<<*iter<<endl;
    }*/
    return vstr;
}