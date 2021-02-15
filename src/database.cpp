#include "database.h"

#include <boost/lexical_cast.hpp>
#include <cctype>
#include <memory>

#include <SQLite/sqlite_connection.h>
#include <TLib/core/tsystem_core_common.h>
#include <TLib/core/tsystem_sqlite_functions.h>
#include <TLib/core/tsystem_utility_functions.h>
#include <Tlib/core/tsystem_time.h>

#include "exchange_calendar.h"
#include "futures_forecast_app.h"
#include "stock_man.h"

int DayEndHHmm(TypePeriod kbar_type);

using namespace  TSystem;
DataBase::DataBase(FuturesForecastApp *app)
    : db_conn_(nullptr)
    , hishq_db_conn_(nullptr)
    , strand_(std::make_shared<TSystem::TaskStrand>(app->task_pool()))
{

}

DataBase::~DataBase()
{

}

bool DataBase::Initialize()
{
    Open(db_conn_);
    OpenHisHqDb(hishq_db_conn_);

    return db_conn_ != nullptr && hishq_db_conn_ != nullptr;
}

void DataBase::Open(std::shared_ptr<SQLite::SQLiteConnection>& db_conn/*, const std::string db_file*/)
{
    db_conn = std::make_shared<SQLite::SQLiteConnection>();

    std::string db_file = "./exchbase.kd";

    if( db_conn->Open(db_file.c_str(), SQLite::SQLiteConnection::OpenMode::READ_WRITE) != SQLite::SQLiteCode::OK )
        ThrowTException( CoreErrorCategory::ErrorCode::BAD_CONTENT
        , "DataBase::Open"
        , "can't open database: " + db_file);

}

void DataBase::OpenHisHqDb(std::shared_ptr<SQLite::SQLiteConnection>& hishq_db_conn)
{
    hishq_db_conn = std::make_shared<SQLite::SQLiteConnection>();

    std::string db_file = "./hqhis.kd";

    if( hishq_db_conn->Open(db_file.c_str(), SQLite::SQLiteConnection::OpenMode::READ_WRITE) != SQLite::SQLiteCode::OK )
        ThrowTException( CoreErrorCategory::ErrorCode::BAD_CONTENT
        , "DataBase::Open"
        , "can't open database: " + db_file);
}

void DataBase::LoadAllStockBaseInfo(std::shared_ptr<StockMan> &stock_man)
{
    assert(db_conn_);
    assert(stock_man->code_stock_baseinfo_item_map_.empty());
    assert(stock_man->pinyin_stock_baseinfo_item_map_.empty());

    if( !utility::ExistTable("stock", *db_conn_) )
        ThrowTException( CoreErrorCategory::ErrorCode::BAD_CONTENT
        , "DataBase::LoadAllStockBaseInfo"
        , "can't find table stock: ");

    std::string sql = utility::FormatStr("SELECT code, type, name, pinyin, timeToMarket, industry, area, remark FROM stock ORDER BY code");
    db_conn_->ExecuteSQL(sql.c_str(), [this, &stock_man](int /*num_cols*/, char** vals, char** /*names*/)->int
    {
        auto item = std::make_shared<T_StockBaseInfoItem>();
        try
        {
            item->code = *vals;
            item->type = boost::lexical_cast<int>(*(vals + 1));
            if( *(vals + 2) )
                item->name = *(vals + 2);
            if( *(vals + 3) )
                item->pinyin = *(vals + 3);
            item->time_to_market = boost::lexical_cast<int>(*(vals + 4));
            if( *(vals + 5) )
                item->industry = *(vals + 5);
            if( *(vals + 6) )
                item->area = *(vals + 6);
            if( *(vals + 7) )
                item->remark = *(vals + 7);
        }catch(boost::exception & )
        { 
            return 0;
        }

        stock_man->pinyin_stock_baseinfo_item_map_.insert( std::make_pair(item->pinyin, item) );
        stock_man->code_stock_baseinfo_item_map_.insert( std::make_pair(item->code, std::move(item)) );
        return 0;
    });
}


void DataBase::LoadTradeDate(void *exchange_calendar)
{
    assert(db_conn_);
    if( !db_conn_ )
        Open(db_conn_);

    if( !utility::ExistTable("ExchangeDate", *db_conn_) )
        throw "DBMoudle::LoadTradeDate can't find table ExchangeDate"; 

    int end_date = MAX_TRADE_DATE; //ExchangeCalendar::TodayAddDays(7); 
    std::string sql = utility::FormatStr("SELECT date, is_tradeday FROM ExchangeDate WHERE date <= %d ORDER BY date DESC", end_date);
    int num = 0;
    ((ExchangeCalendar*)exchange_calendar)->max_trade_date_ = 0;
    db_conn_->ExecuteSQL(sql.c_str(), [&num, &exchange_calendar, this](int /*num_cols*/, char** vals, char** /*names*/)->int
    { 
        try
        { 
            ++num;
            int date =  boost::lexical_cast<int>(*(vals)); 
            bool is_trade_date = boost::lexical_cast<bool>(*(vals + 1)); 
            ((ExchangeCalendar*)exchange_calendar)->trade_dates_->insert(std::make_pair(date, is_trade_date));

            if( ((ExchangeCalendar*)exchange_calendar)->max_trade_date_ == 0 )
                ((ExchangeCalendar*)exchange_calendar)->max_trade_date_ = date;

            ((ExchangeCalendar*)exchange_calendar)->min_trade_date_ = date;
        }catch(boost::exception& )
        {
            return 0;
        } 
        return 0;
    }); 
}


void DataBase::GetStockCode(const std::string &code, std::vector<T_StockCodeName>& ret)
{
    ret.clear();

    if( !db_conn_ )
    {
        Open(db_conn_);
    }

    std::string sql;
    if( IsStrAlpha(code) )
    {
        sql = utility::FormatStr("SELECT code, name, type, nmarket FROM stock WHERE pinyin like '%s%%' ORDER BY code LIMIT 5", code.c_str());
    }else if( IsStrNum(code) )
    {
        sql = utility::FormatStr("SELECT code, name, type, nmarket FROM stock WHERE code like '%s%%' ORDER BY code LIMIT 5", code.c_str());
    }else
    {
        sql = utility::FormatStr("SELECT code, name, type, nmarket FROM stock WHERE name like '%s%%' ORDER BY code LIMIT 5", code.c_str());
    }

    if( !utility::ExistTable("stock", *db_conn_) )
        return;

    db_conn_->ExecuteSQL(sql.c_str(),[&ret, this](int /*num_cols*/, char** vals, char** /*names*/)->int
    { /*
      T_StockCodeName code_name;
      code_name.code = *vals;
      code_name.name = *(vals + 1);*/
        std::string name = *(vals + 1);
        int type = boost::lexical_cast<int>(*(vals + 2));
        int nmarket = boost::lexical_cast<int>(*(vals + 3));
        utf8ToGbk(name);
        ret.emplace_back(*vals, std::move(name), type, nmarket);
        return 0;
    });
    return;
}

std::string MakeTableName(const std::string &code, bool is_index, /*int nmarket,*/ TypePeriod kbar_type)
{
    char  table_name[128] = {"\0"};
    switch(kbar_type)
    {
        case TypePeriod::PERIOD_1M: 
            sprintf_s(table_name, "%s_1M", code.c_str()); break;
        case TypePeriod::PERIOD_5M: 
            sprintf_s(table_name, "%s_5M", code.c_str()); break;
        case TypePeriod::PERIOD_15M: 
            sprintf_s(table_name, "%s_15M", code.c_str()); break;
        case TypePeriod::PERIOD_30M: 
            sprintf_s(table_name, "%s_30M", code.c_str()); break;
        case TypePeriod::PERIOD_HOUR: 
            sprintf_s(table_name, "%s_HOUR", code.c_str()); break;
        case TypePeriod::PERIOD_DAY: 
            sprintf_s(table_name, "%s_DAY", code.c_str()); break;
        case TypePeriod::PERIOD_WEEK: 
            sprintf_s(table_name, "%s_WEEK", code.c_str()); break;
        case TypePeriod::PERIOD_MON: 
            sprintf_s(table_name, "%s_MON", code.c_str()); break;
        default:  assert(false); break;
    }
    return table_name;
}

int DayEndHHmm(TypePeriod kbar_type)
{ 
    switch(kbar_type)
    {
    case TypePeriod::PERIOD_1M: 
        return 2359; break;
    case TypePeriod::PERIOD_5M: 
        return 2355; break;
    case TypePeriod::PERIOD_15M: 
        return 2345; break;
    case TypePeriod::PERIOD_30M: 
        return 2330; break;
    case TypePeriod::PERIOD_HOUR: 
        return 2300; break;
    case TypePeriod::PERIOD_DAY:  
    case TypePeriod::PERIOD_WEEK:  
    case TypePeriod::PERIOD_MON: 
        return 0; break;
    default:  assert(false); break;
    }
    return 0;
}

bool DataBase::GetHisKBars(const std::string &code, bool is_index, int nmarket, TypePeriod kbar_type, int start_date, int end_date
                           , OUT std::vector<T_StockHisDataItem> &items, char *error)
{
    assert(hishq_db_conn_);
   /* if( !hishq_db_conn_ )
    {
        OpenHisHqDb(hishq_db_conn_);
    }*/
     
    std::string table_name = MakeTableName(code, is_index, kbar_type);

    // judge if table exists
    if( !utility::ExistTable(table_name, *hishq_db_conn_) )
    {
        if( error ) sprintf(error, "DBMoudle::GetHisKBars can't find table %s", table_name.c_str());
        return false;//throw "DBMoudle::LoadTradeDate can't find table ExchangeDate"; 
    }
    // select data
    std::string sql = utility::FormatStr("SELECT longdate, time, close, high, low, open, vol FROM %s WHERE longdate >= %d "
                                        " AND longdate <= %d "
                                        " ORDER BY longdate, time"
                                        , table_name.c_str(), start_date, end_date); 

     hishq_db_conn_->ExecuteSQL(sql.c_str(), [&items, this](int /*num_cols*/, char** vals, char** /*names*/)->int
    { 
        try
        {  
            T_StockHisDataItem item;
            item.date =  boost::lexical_cast<int>(*(vals)); 
            item.hhmmss = boost::lexical_cast<int>(*(vals + 1)); 
            item.close_price = boost::lexical_cast<double>(*(vals + 2)); 
            item.high_price = boost::lexical_cast<double>(*(vals + 3)); 
            item.low_price = boost::lexical_cast<double>(*(vals + 4)); 
            item.open_price = boost::lexical_cast<double>(*(vals + 5)); 
            item.vol = boost::lexical_cast<double>(*(vals + 6)); 
            items.push_back(item);
             
        }catch(boost::exception& )
        {
            return 0;
        } 
        return 0;
    }); 
    return true;
}


bool DataBase::GetHisKBars(const std::string &code, bool is_index, int nmarket, TypePeriod kbar_type, const T_DateRange &range
                           , OUT std::vector<T_StockHisDataItem> &items, char *error)
{
    assert(hishq_db_conn_); 
     
    int eldest_date = std::get<0>(range);
    int eldest_hhmm = std::get<1>(range);
    int latest_date = std::get<2>(range);
    int latest_hhmm = std::get<3>(range);
    assert(eldest_date <= latest_date);

    std::string table_name = MakeTableName(code, is_index, kbar_type);

    // judge if table exists
    if( !utility::ExistTable(table_name, *hishq_db_conn_) )
    {
        if( error ) sprintf(error, "DBMoudle::GetHisKBars can't find table %s", table_name.c_str());
        return false;//throw "DBMoudle::LoadTradeDate can't find table ExchangeDate"; 
    }
    auto lamb_execute_sql = [this](std::string &sql, OUT std::vector<T_StockHisDataItem> &items)
    {
        this->hishq_db_conn_->ExecuteSQL(sql.c_str(), [&items](int /*num_cols*/, char** vals, char** /*names*/)->int
        { 
            try
            {  
                T_StockHisDataItem item;
                item.date =  boost::lexical_cast<int>(*(vals)); 
                item.hhmmss = boost::lexical_cast<int>(*(vals + 1)); 
                item.close_price = boost::lexical_cast<double>(*(vals + 2)); 
                item.high_price = boost::lexical_cast<double>(*(vals + 3)); 
                item.low_price = boost::lexical_cast<double>(*(vals + 4)); 
                item.open_price = boost::lexical_cast<double>(*(vals + 5)); 
                item.vol = boost::lexical_cast<double>(*(vals + 6)); 
                items.push_back(item);

            }catch(boost::exception& )
            {
                return 0;
            } 
            return 0;
        }); 
    };

    // select data
    std::string sql;
    if( eldest_date == latest_date )
    {
        sql = utility::FormatStr("SELECT longdate, time, close, high, low, open, vol FROM %s WHERE longdate == %d "
                                " AND time >= %d AND time <= %d "
                                " ORDER BY longdate, time"
                                , table_name.c_str(), eldest_date, eldest_hhmm, latest_hhmm); 
        lamb_execute_sql(sql, items);
    }
    else
    {
        sql = utility::FormatStr("SELECT longdate, time, close, high, low, open, vol FROM %s WHERE longdate == %d "
                                " AND time >= %d "
                                " ORDER BY longdate, time"
                                , table_name.c_str(), eldest_date, eldest_hhmm); 
        lamb_execute_sql(sql, items);

        sql = utility::FormatStr("SELECT longdate, time, close, high, low, open, vol FROM %s WHERE longdate > %d "
            " AND longdate < %d "
            " ORDER BY longdate, time"
            , table_name.c_str(), eldest_date, latest_date); 
        lamb_execute_sql(sql, items);
        sql = utility::FormatStr("SELECT longdate, time, close, high, low, open, vol FROM %s WHERE longdate == %d "
                                    " AND time <= %d "
                                    " ORDER BY longdate, time"
                                    , table_name.c_str(), latest_date, latest_hhmm); 
        lamb_execute_sql(sql, items);
    }
    return true;
}


bool DataBase::GetHisKBarDateRange(const std::string &code, bool is_index, TypePeriod kbar_type, OUT T_DateRange &range)
{
    assert(hishq_db_conn_); 
     
    std::string table_name = MakeTableName(code, is_index, kbar_type);

    // judge if table exists
    if( !utility::ExistTable(table_name, *hishq_db_conn_) )
        return false;//throw "DBMoudle::LoadTradeDate can't find table ExchangeDate"; 

    int eldest_date = 0, eldest_time = 0;
    int latest_date = 0, latest_time = 0;
    // select data
    std::string sql = utility::FormatStr("select longdate, MIN(time) from %s where longdate = (SELECT MIN(longdate) from %s )"
                                        , table_name.c_str(), table_name.c_str()); 
    hishq_db_conn_->ExecuteSQL(sql.c_str(), [this, &eldest_date, &eldest_time](int /*num_cols*/, char** vals, char** /*names*/)->int
    {
        if( !*vals )
            return 1;
        try
        {
            eldest_date =  boost::lexical_cast<int>(*(vals)); 
            eldest_time =  boost::lexical_cast<int>(*(vals + 1)); 
        }catch(boost::exception& )
        {
            return 0;
        } 
        return 0;
    });

    sql = utility::FormatStr("select longdate, MAX(time) from %s where longdate = (SELECT MAX(longdate) from %s )"
                                        , table_name.c_str(), table_name.c_str()); 
    hishq_db_conn_->ExecuteSQL(sql.c_str(), [this, &latest_date, &latest_time](int /*num_cols*/, char** vals, char** /*names*/)->int
    {
        if( !*vals )
            return 1;
        try
        {
            latest_date =  boost::lexical_cast<int>(*(vals)); 
            latest_time =  boost::lexical_cast<int>(*(vals + 1)); 
        }catch(boost::exception& )
        {
            return 0;
        } 
        return 0;
    });

    if( eldest_date == 0 || latest_date == 0 )
        return false;
    range = std::make_tuple(eldest_date, eldest_time, latest_date, latest_time);
    return true;
}


bool DataBase::GetPartHisKBars(const std::string &code, bool is_index, int nmarket, TypePeriod kbar_type, int start_index, int length
                           , OUT std::vector<T_StockHisDataItem> &items, char *error)
{
    assert(hishq_db_conn_); 
    std::string table_name = MakeTableName(code, is_index, kbar_type);

    // judge if table exists
    if( !utility::ExistTable(table_name, *hishq_db_conn_) )
    {
        if( error ) sprintf(error, "DBMoudle::GetPartHisKBars can't find table %s", table_name.c_str());
        return false;//throw "DBMoudle::LoadTradeDate can't find table ExchangeDate"; 
    }
    // select data
    int target_date = 0;
    int target_hhmm = 0;
    std::string sql = utility::FormatStr("SELECT longdate, time FROM %s "
                                 " ORDER BY longdate, time limit %d", table_name.c_str(), start_index + 1);
    hishq_db_conn_->ExecuteSQL(sql.c_str(), [&items, &target_date, &target_hhmm, this](int /*num_cols*/, char** vals, char** /*names*/)->int
    { 
        try
        {   
            target_date =  boost::lexical_cast<int>(*(vals)); 
            target_hhmm = boost::lexical_cast<int>(*(vals + 1));  

        }catch(boost::exception& )
        {
            return 0;
        } 
        return 0;
    });
    if( target_date == 0 )
    {
        sprintf(error, "DBMoudle::GetPartHisKBars can't find start index:%d related record %s", start_index, table_name.c_str());
        return false;//throw "DBMoudle::LoadTradeDate can't find table ExchangeDate"; 
    }
    sql = utility::FormatStr("SELECT longdate, time, close, high, low, open, vol FROM %s WHERE longdate >= %d " 
                                        " ORDER BY longdate, time LIMIT %d "
                                        , table_name.c_str(), target_date, length + start_index + 1); 

    bool first_tag = true; 
    hishq_db_conn_->ExecuteSQL(sql.c_str(), [&items, &first_tag, target_hhmm, length, this](int /*num_cols*/, char** vals, char** /*names*/)->int
    { 
        if( items.size() >= length )
            return 0;
        try
        {  
            T_StockHisDataItem item;
            item.date =  boost::lexical_cast<int>(*(vals)); 
            item.hhmmss = boost::lexical_cast<int>(*(vals + 1)); 
            if( first_tag && item.hhmmss < target_hhmm )
                return 0;
            first_tag = false; 

            item.close_price = boost::lexical_cast<double>(*(vals + 2)); 
            item.high_price = boost::lexical_cast<double>(*(vals + 3)); 
            item.low_price = boost::lexical_cast<double>(*(vals + 4)); 
            item.open_price = boost::lexical_cast<double>(*(vals + 5)); 
            item.vol = boost::lexical_cast<double>(*(vals + 6)); 
            items.push_back(item);
             
        }catch(boost::exception& )
        {
            return 0;
        } 
        return 0;
    }); 
    return true;
}
 
bool DataBase::GetNextKBar(const std::string &code, bool is_index, int nmarket, TypePeriod kbar_type, int date, int hhmm, OUT T_StockHisDataItem &item)
{
    assert(hishq_db_conn_); 
    std::string table_name = MakeTableName(code, is_index, kbar_type);

    // judge if table exists
    if( !utility::ExistTable(table_name, *hishq_db_conn_) )
    {
        //if( error ) sprintf(error, "DBMoudle::GetPartHisKBars can't find table %s", table_name.c_str());
        return false;//throw "DBMoudle::LoadTradeDate can't find table ExchangeDate"; 
    }
    // select data 
    std::string sql;
    int day_end_hhmm = DayEndHHmm(kbar_type);
    if( hhmm != day_end_hhmm )
    { 
        sql =  utility::FormatStr("SELECT longdate, time, close, high, low, open, vol FROM %s WHERE longdate == %d " 
            " AND time > %d"
            " ORDER BY longdate, time LIMIT 1 "
            , table_name.c_str(), date, hhmm); 
    }else
    {
        sql =  utility::FormatStr("SELECT longdate, time, close, high, low, open, vol FROM %s WHERE longdate > %d " 
            " ORDER BY longdate, time LIMIT 1 "
            , table_name.c_str(), date); 
    }
    bool is_exist = false;
    hishq_db_conn_->ExecuteSQL(sql.c_str(), [&item, &is_exist, this](int /*num_cols*/, char** vals, char** /*names*/)->int
    { 
        try
        {    
            item.date =  boost::lexical_cast<int>(*(vals)); 
            item.hhmmss = boost::lexical_cast<int>(*(vals + 1)); 

            item.close_price = boost::lexical_cast<double>(*(vals + 2)); 
            item.high_price = boost::lexical_cast<double>(*(vals + 3)); 
            item.low_price = boost::lexical_cast<double>(*(vals + 4)); 
            item.open_price = boost::lexical_cast<double>(*(vals + 5)); 
            item.vol = boost::lexical_cast<double>(*(vals + 6)); 
            is_exist = true;
        }catch(boost::exception& )
        {
            return 0;
        } 
        return 0;
    });
    if( is_exist )
        return true;
    if( hhmm == day_end_hhmm )
        return false;
    sql =  utility::FormatStr("SELECT longdate, time, close, high, low, open, vol FROM %s WHERE longdate > %d " 
                        " AND time > %d"
                        " ORDER BY longdate, time LIMIT 1 "
                        , table_name.c_str(), date, hhmm); 
    hishq_db_conn_->ExecuteSQL(sql.c_str(), [&item, &is_exist, this](int /*num_cols*/, char** vals, char** /*names*/)->int
    { 
        try
        {    
            item.date =  boost::lexical_cast<int>(*(vals)); 
            item.hhmmss = boost::lexical_cast<int>(*(vals + 1)); 

            item.close_price = boost::lexical_cast<double>(*(vals + 2)); 
            item.high_price = boost::lexical_cast<double>(*(vals + 3)); 
            item.low_price = boost::lexical_cast<double>(*(vals + 4)); 
            item.open_price = boost::lexical_cast<double>(*(vals + 5)); 
            item.vol = boost::lexical_cast<double>(*(vals + 6)); 
            is_exist = true;
        }catch(boost::exception& )
        {
            return 0;
        } 
        return 0;
    });

    return is_exist;
}