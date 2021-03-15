# coding=utf-8
###############################
#
#   edit 20201202 : fix bug of lost last month end night trading data
#   edit 20201210 : fix bug add condition: next_trd_date_mon == mon
#   edit 20210314 : fix bug in PreTradeDate; add func PreDate
###############################
import sys
import os
import time
import re
import string
import sqlite3   
import datetime as dt  
from datetime import timedelta
from datetime import datetime
from pypinyin import pinyin, lazy_pinyin
import tushare as ts 
   
g_token = '93c7d3d19917581306c31b400c570ae3059a16c4e60c35db275d5882'  
ROOT_DIR = './' 
DB_FILE_PATH = './hqhis.kd'
DATA_FILE_PATH = 'F:/StockHisdata/SCL8/201912'
#ATA_FILE_PATH = 'C:\\StockHisdata\\SCL8\\201902'
CODE = "SCL8" 
REG_STR = "^(\\d{8}),(\\S+),(\\S+),(\\s+),(\\d+\\.\\d+|\\d+),(\\d+\\.\\d+|\\d+),(\\d+\\.\\d+|\\d+),(\\d+),(\\d+\\.\\d+|\\d+),(\\d+\\.\\d+|\\d+),(\\d+\\.\\d+|\\d+),(\\d+),(\\d+),(\\d+),(\\d+\\.\\d+|\\d+),(\\d+\\.\\d+|\\d+),(\\d+\\.\\d+|\\d+),(\\d+\\.\\d+|\\d+),(\\d+),(\\d+),(\\d{2}):(\\d{2}):(\\d{2}),(\\d+),(\\d+\\.\\d+|\\d+),(\\d+),(\\d+\\.\\d+|\\d+),(\\d+),(\\d+\\.\\d+|\\d+),(\\d+),(\\d+\\.\\d+|\\d+),(\\d+),(.*)$"

def getTradeCal():
    global g_token
    ts.set_token(g_token)
    pro = ts.pro_api()
    try:
        #df = ts.trade_cal()
        df = pro.query('trade_cal')
    except Exception as err:  
        print("getTradeCal pro.query fail:%s" % err) 
        os._exit(0)
    finally: 
        if df.empty:
            print("getTradeCal ret empty")  
            os._exit(0)
        else:  
            #df['change'] = df['change'].replace('--', '')   
            #print(df) 
            pass
    return df    
    
def TradeWindowStartHhmm(index):
    window_start_hhmms = (900, 1030, 1330, 2100)
    if index >= 0 and index <= 3:
        return window_start_hhmms[index]
    return -1
        
def TradeWindowEndHhmm(index):
    window_end_hhmms = (1015, 1130, 1500, 230)
    if index >= 0 and index <= 3:
        return window_end_hhmms[index]
    return -1
    
def IsTradeWindowStartHhmm(hhmm):
    window_start_hhmms = (900, 1030, 1330, 2100)
    for key in window_start_hhmms:
        if hhmm == key:
            return True
    return False
    
def IsTradeWindowEndHhmm(hhmm):
    window_end_hhmms = (1015, 1130, 1500, 230)
    for key in window_end_hhmms:
        if hhmm == key:
            return True
    return False
    
def NextHhmm(date, hhmm):
    t_obj = dt.datetime(date//10000, date%10000//100, date%100, hhmm//100, hhmm%100)
    t_obj += dt.timedelta(minutes=1)
    return t_obj.hour*100 + t_obj.minute
    
def TimeInWhichTradeWindow(hhmm):
    #periods = ((900, 1015), (1030, 1130), (1330, 1500), (2100, 2359), (0, 230))
    #for i in range(0, len(periods)):
    #    if hhmm >= periods[i][0] and hhmm <= periods[i][1]:
    #        return i
    #return -1
    if hhmm >= 900 and hhmm <= 1015:
        return 0
    if hhmm >= 1030 and hhmm <= 1130:
        return 1
    if hhmm >= 1330 and hhmm <= 1500:
        return 2
    if hhmm >= 2100 and hhmm <= 2359 or hhmm >= 0 and hhmm <= 230:
        return 3
    return -1
    
def IsLegalTradeTime(hhmm):
    return TimeInWhichTradeWindow(hhmm) > -1
    
def IsInSameTradeWindow(hhmm0, hhmm1):
    win0 = TimeInWhichTradeWindow(hhmm0)
    win1 = TimeInWhichTradeWindow(hhmm1)
    return win0 != -1 and win1 != -1 and win0 == win1

class FUTUREBASIC:  
    def __init__(self):  
        print("FUTUREBASIC init")
        #self.cal_dates = ts.trade_cal() #return calendar of exchange center, DataFrame, calendarDate,isOpen  
        self.cal_dates = getTradeCal()
        #self.data_dir = "C:/"
        #if "STK_DATA_DIR" in os.environ:
        #    self.data_dir = os.environ["STK_DATA_DIR"] 
        self.data_dir = "."
        self.file_ok_ext = ".ok"    
        log_dir = self.data_dir + "\\log\\"
        if not os.path.exists(log_dir):
            os.makedirs(log_dir) 
        self.log_file_path = log_dir + "trans_fenbi_log_" + time.strftime("%Y%m%d", time.localtime()) + ".txt"   
        self.log_file_handle = open(self.log_file_path, 'a')
        self.g_db_conn = False
        self.g_db_conn = self.open_db()
        self.exists_pre_mon_end_night_date = False
    def __del__(self):
        print("del self")
        self.close_db()
        if self.log_file_handle:
            self.log_file_handle.close()
        
    def open_db(self):
        ''' open data base 保存数据库'''
        global DB_FILE_PATH 
        if not os.access(DB_FILE_PATH, os.F_OK):
            print ("%s not exist\n" % DB_FILE_PATH)
            os._exit(0)
        self.g_db_conn = sqlite3.connect(DB_FILE_PATH, detect_types=sqlite3.PARSE_COLNAMES)
        if self.g_db_conn is not None:
            print ("opened {0} successful!".format(DB_FILE_PATH))
            pass
        else:
            #print ("opened database fail!")
            os._exit(0)
        self.g_db_conn.text_factory = lambda x : str(x, 'utf-8', 'ignore')
        self.cur = self.g_db_conn.cursor()
        return self.g_db_conn
        
    def close_db(self): 
        print("db commit")
        self.g_db_conn.commit()
        self.g_db_conn.close() 
    
    def getTargetKDataDir(self, code):
        target_path = self.data_dir + "/" + code + "/kline"
        #print("saveCodeTick2File : %s %s" %(code, target_path) )
        if not os.path.isdir(target_path):
            os.makedirs(target_path)
        return target_path
        
    def write_log(self, content):
        if self.log_file_handle:
            self.log_file_handle.write(content + "\n")
            self.log_file_handle.flush()
            
    def is_ascii(self, s):
        return all(ord(c) < 128 for c in s)
        
    def IsTradeDate(self, date):
        df = self.cal_dates[self.cal_dates.is_open==1] 
        result = df.loc[(df.cal_date==str(date))&(df.is_open==1)]  
        if result.empty:
            return False
        else:
            return True
    def PreDate(self, date): 
        df = self.cal_dates 
        result = df.loc[(df.cal_date==str(date))]  
        if result.empty:
            print("PreDate fail cause date:{0} is not calendar date".format(date))
            return 0
        d_i = result.index.tolist()[0] 
        #print("date:{0} index:{1} {2}".format(date, d_i, result.loc[d_i]['cal_date']))
        temp_i = df.index.tolist().index(d_i) - 1 
        pre_date_i = df.index.tolist()[temp_i]
        pre_date = df.loc[pre_date_i]['cal_date']
        return int(pre_date)
        
    def PreTradeDate(self, date): 
        df = self.cal_dates[self.cal_dates.is_open==1] 
        #df = self.cal_dates[(self.cal_dates.is_open==1) & (self.cal_dates.cal_date=='20201221')]      
        #print(df.iloc[0,0]) 根据行
        #print(df.loc[0,0])  根据索引 
        #query_str = "cal_date.str.contains('{0}')".format(date)
        #result = df.query(query_str, engine='python') #索引不会自己创建 得到的仍旧是df的 
        temp_date = date
        result = df.loc[(df.cal_date==str(temp_date))&(df.is_open==1)]  
        count = 0
        while result.empty and count < 60:
            print("date:{0} is not trade date".format(temp_date))
            temp_date = self.PreDate(temp_date)
            result = df.loc[(df.cal_date==str(temp_date))&(df.is_open==1)]  
            count = count + 1
        if result.empty:
            print("PreTradeDate fail cause date:{0} is not trade date return 0".format(temp_date))
            return 0
        d_i = result.index.tolist()[0] 
        #print("date:{0} index:{1} {2}".format(date, d_i, result.loc[d_i]['cal_date']))
        temp_i = 0
        if count > 0:
            temp_i = df.index.tolist().index(d_i)
        else:
            temp_i = df.index.tolist().index(d_i) - 1 
        pre_trd_date_i = df.index.tolist()[temp_i]
        pre_trd_date = df.loc[pre_trd_date_i]['cal_date']
        return int(pre_trd_date)
    
    def NextTradeDate(self, date): 
        df = self.cal_dates[self.cal_dates.is_open==1] 
        #df = self.cal_dates[(self.cal_dates.is_open==1) & (self.cal_dates.cal_date=='20201221')]      
        #print(df.iloc[0,0]) 根据行
        #print(df.loc[0,0])  根据索引 
        #query_str = "cal_date.str.contains('{0}')".format(date)
        #result = df.query(query_str, engine='python') #索引不会自己创建 得到的仍旧是df的 
        
        result = df.loc[(df.cal_date==str(date))&(df.is_open==1)]  
        if result.empty:
            print("fail cause date:{0} is not trade date".format(date))
            return 0
        d_i = result.index.tolist()[0] 
        #print("date:{0} index:{1} {2}".format(date, d_i, result.loc[d_i]['cal_date']))
        temp_i = df.index.tolist().index(d_i) + 1 
        next_trd_date_i = df.index.tolist()[temp_i]
        next_trd_date_i = df.loc[next_trd_date_i]['cal_date']
        return int(next_trd_date_i)    
        
    def RealTradeDate(self, date, hhmm):
        if hhmm >= 2100:
            real_trd_date = self.PreTradeDate(date)
            #print("{0} {1} realTrade {2}".format(date, hhmm, real_trd_date))
            return real_trd_date
        else:
            return date
            
    def ClearRelDbData(self, code, date_para):  
        sql = ""
        try:
            tb_endians = ('1M', '5M', '15M', '30M', 'HOUR') 
            for edian in tb_endians:
                cursor = self.cur.execute("SELECT count(*) FROM sqlite_master where type='table' and name='{0}_{1}'".format(code, edian))
                is_table_exist = False
                for it in cursor:
                    if it[0] > 0:
                        is_table_exist = True
                        break
                if is_table_exist:
                    sql = "Delete FROM {0}_{1} WHERE longdate={2}".format(code, edian, date_para)
                    print(sql)
                    self.g_db_conn.execute(sql)
                    self.g_db_conn.commit()
        except Exception as e:      
            temp_str = "{0} excetion:{1}".format(sql, e)
            print(temp_str)
            self.write_log(temp_str)
    
    def Save30mDataFrom5mData(self, code, date):
        sql = "CREATE TABLE IF NOT EXISTS {0}_30M( longdate INTEGER, time INTEGER, open DOUBLE, close DOUBLE, high DOUBLE, low DOUBLE, vol DOUBLE, PRIMARY KEY(longdate, time) )".format(code)
        self.cur.execute(sql)
        tp_tuple = (30, 100, 130, 200, 230, 930, 1000, 1045, 1115, 1345, 1415, 1445, 1500, 2130, 2200, 2230, 2300, 2330)
        sql = "SELECT longdate, time, open, close, high, low, vol FROM {0}_5M WHERE time > 0 AND longdate = {1} ORDER BY longdate, time".format(code, date)
        cursor = self.cur.execute(sql)
        count = 0
        max_price = 999.9
        min_price = 0.0
        high = min_price
        low = max_price
        open_30m = 0.0
        close_30m = 0.0
        high_30m = min_price
        low_30m = max_price
        vol = 0
        is_exists_night_trd_time = False
        flag = True
        for it in cursor: 
            #self.write_log(str(it)) 
            it_date = it[0]
            it_time = it[1]
            it_open = it[2]
            it_close = it[3]
            it_high = it[4]
            it_low = it[5]
            it_vol = it[6]
            if it_time > 2100:
                is_exists_night_trd_time = True
            #if not it_time in tp_tuple: 
            if flag:
                flag = False
                open_30m = it_open 
                close_30m = it_close
                high_30m = it_high
                low_30m = it_low
                vol = int(it_vol)
            else:    
                close_30m = it_close
                if it_high > high_30m:
                    high_30m = it_high
                if it_low < low_30m:
                    low_30m = it_low
                vol = vol + int(it_vol) 
            if it_time in tp_tuple:    
                sql = "INSERT OR REPLACE INTO {0}_30M VALUES({1}, {2}, {3}, {4}, {5}, {6}, {7})".format(code, it_date, it_time, open_30m, close_30m, high_30m, low_30m, vol)
                self.g_db_conn.execute(sql)# not use cursor,cause cursor is using by for
                self.write_log(sql)
                vol = 0 
                flag = True 
            count = count + 1    
        #for time 0
        if is_exists_night_trd_time:
            #print("save 30m for time 0")
            next_trade_date = self.NextTradeDate(date) #0 分钟 归入下一交易日
            sql = "SELECT longdate, time, open, close, high, low, vol FROM {0}_5M WHERE time = 0 AND longdate = {1} ORDER BY longdate, time".format(code, next_trade_date)
            cursor = self.cur.execute(sql)
            for it in cursor: 
                print(sql)
                it_open = it[2]
                it_close = it[3]
                it_high = it[4]
                it_low = it[5]
                it_vol = it[6]
                if not flag: #exists 2335 2340 ...
                    if it_high > high_30m:
                        high_30m = it_high
                    if it_low < low_30m:
                        low_30m = it_low
                    close_30m = it_close
                    vol = vol + int(it_vol)  
                else:
                    high_30m = it_high
                    low_30m = it_low
                    open_30m = it_open 
                    close_30m = it_close
                    vol = int(it_vol)
                sql = "INSERT OR REPLACE INTO {0}_30M VALUES({1}, {2}, {3}, {4}, {5}, {6}, {7})".format(code, next_trade_date, 0, open_30m, close_30m, high_30m, low_30m, vol)
                #print(sql)
                self.g_db_conn.execute(sql)# not use cursor,cause cursor is using by for
                self.write_log(sql)    
                break
        self.g_db_conn.execute(sql)
        self.g_db_conn.commit()
        print("Save30mDataFrom5mData {0}, {1} ok!".format(code, date)) 
        return True
        
    def Save5mDataFrom1mData_sub(self, code, date_para, start_hhmm, end_hhmm):      
        time_condition = " time >= {0} and time <= {1} ".format(start_hhmm, end_hhmm)
        try:
            sql = "CREATE TABLE IF NOT EXISTS {0}_5M( longdate INTEGER, time INTEGER, open DOUBLE, close DOUBLE, high DOUBLE, low DOUBLE, vol DOUBLE, PRIMARY KEY(longdate, time) )".format(code)
            self.g_db_conn.execute(sql)
            count = 0
            max_price = 999.9
            min_price = 0.0
            high = min_price
            low = max_price
            open_5m = 0.0
            close_5m = 0.0
            high_5m = min_price
            low_5m = max_price
            vol = 0
            sql = "SELECT longdate, time, open, close, high, low, vol FROM {0}_1M WHERE longdate={1} and {2} ORDER BY longdate, time".format(code, date_para, time_condition)
            print(sql)
            self.write_log(sql)
            cursor = self.cur.execute(sql) 
            for it in cursor:
                count = count + 1
                #self.write_log(str(it)) 
                it_date = it[0]
                it_time = it[1]
                it_open = it[2]
                it_close = it[3]
                it_high = it[4]
                it_low = it[5]
                it_vol = it[6]
                if count % 5 == 1:
                    self.write_log("then set open_5m")
                    open_5m = it_open 
                    close_5m = it_close
                    high_5m = it_high
                    low_5m = it_low
                    vol = int(it_vol)
                elif count % 5 != 0:
                    if it_high > high_5m:
                        high_5m = it_high
                    if it_low < low_5m:
                        low_5m = it_low
                    vol = vol + int(it_vol)
                    close_5m = it_close
                else:
                    if it_time % 5 != 0:
                        temp_str = "Save5mDataFrom1mData exception {0} hhmm:{1} % 5 != 0 {2}".format(it_date, it_time, time_condition)
                        print(temp_str)
                        self.write_log(temp_str)
                        return False      
                    close_5m = it_close
                    if it_high > high_5m:
                        high_5m = it_high
                    if it_low < low_5m:
                        low_5m = it_low
                    vol = vol + int(it_vol) 
                    if open_5m == 0.0:
                        print("374 fail open_5m = 0.0 it_date{0} it_time{1} open_5m:{2} close_5m:{3}".format(next_trade_date, 0, open_5m, close_5m))
                        self.write_log("374 fail open_5m = 0.0 it_date{0} it_time{1} open_5m:{2} close_5m:{3}".format(next_trade_date, 0, open_5m, close_5m))
                        return False
                    sql = "INSERT OR REPLACE INTO {0}_5M VALUES({1}, {2}, {3}, {4}, {5}, {6}, {7})".format(code, it_date, it_time, open_5m, close_5m, high_5m, low_5m, vol)
                    #print(sql)
                    self.g_db_conn.execute(sql)# not use cursor,cause cursor is using by for
                    self.write_log(sql)
            #end of for
            if end_hhmm == 2359: #insert k of time 0
                next_trade_date = self.NextTradeDate(date_para) #0 分钟 归入下一交易日
                next_trd_date_mon = next_trade_date % 10000 //100
                mon = date_para % 10000 //100
                if next_trd_date_mon == mon: 
                    sql = "SELECT longdate, time, open, close, high, low, vol FROM {0}_1M WHERE longdate={1} and time=0 ".format(code, next_trade_date)
                    cursor = self.cur.execute(sql) 
                    for it in cursor:  
                        it_close = it[3]
                        it_high = it[4]
                        it_low = it[5]
                        it_vol = it[6] 
                        if it_high > high_5m:
                            high_5m = it_high
                        if it_low < low_5m:
                            low_5m = it_low
                        vol = vol + int(it_vol)
                        close_5m = it_close
                        if open_5m == 0.0:
                            print("401 fail open_5m = 0.0 it_date:{0} it_time:{1} open_5m:{2} close_5m:{3}".format(next_trade_date, 0, open_5m, close_5m))
                            self.write_log("401 fail open_5m = 0.0 it_date:{0} it_time:{1} open_5m:{2} close_5m:{3}".format(next_trade_date, 0, open_5m, close_5m))
                            return False
                        sql = "INSERT OR REPLACE INTO {0}_5M VALUES({1}, {2}, {3}, {4}, {5}, {6}, {7})".format(code, next_trade_date, 0, open_5m, close_5m, high_5m, low_5m, vol)
                        #print(sql)
                        self.g_db_conn.execute(sql) 
                        self.write_log(sql)
                        break #only once
                
        except Exception as e:
            temp_str = "Save5mDataFrom1mData select exception{0}".format(e)
            print(temp_str)
            self.write_log(temp_str)
            return False      
        self.g_db_conn.commit()
        print("Save5mDataFrom1mData {0} count:{1} ok!".format(code, count))
        return True
        
    def ChecFenbiDataFromFile(self, file_full_path, code):
        print("Check1mDataFromFile {0}".format(file_full_path))
        self.write_log("Check1mDataFromFile {0}".format(file_full_path))
        reobj = re.compile(REG_STR)
        re.compile(reobj)
        fh = open(file_full_path)  
        is_last_line = False 
        line_count = 0
        lack_continued = 0 
        pre_date = 0;
        is_first_cmp_date = True
        for line in fh:
            if line is None:
               continue
            if line_count == 0:
               line_count += 1
               continue
            try:  
                match1 = re.search(reobj, line) 
                if not match1:
                    continue
                if line_count == 1:
                    pass
                    #sc_code = match1.group(2)  
                    #if sc_code.lower() != code.lower():
                    #    temp_str = "ChecFenbiDataFromFile code {0} not fit content code{1}".format(code, sc_code)
                    #    print(temp_str)
                    #    self.write_log(temp_str)
                    #    return False 
                line_count += 1
                date = int(match1.group(1))  
                hour = int(match1.group(21)) 
                minute = int(match1.group(22))
                second = int(match1.group(23))
                ms = int(match1.group(24)) 
                #print("{0} hour:{1} minute:{2}".format(sys._getframe().f_lineno, hour, minute))
                buy1_p = match1.group(25)
                buy1_v = match1.group(26)
                sell1_p = match1.group(27)
                sell1_v = match1.group(28)
                if not is_first_cmp_date:
                    if pre_date != date:
                        temp_str = "ChecFenbiDataFromFile code {0} occure date {1} not same".format(code, date)
                        print(temp_str)
                        self.write_log(temp_str)
                        return False 
                is_first_cmp_date = False
                pre_date = date
                    
                #print("{0} {1}:{2}:{3} {4} b_p:{5} b_v:{6} s_p:{7} s_v:{8}".format(date, hour,minute,second,ms, buy1_p,buy1_v,sell1_p,sell1_v))
            except Exception as e:
                temp_str = "ChecFenbiDataFromFile {0} line {1} esception {2}".format(file_full_path, line_count, e)
                print(temp_str)
                self.write_log(temp_str)
                return False  
        
        if line_count < 2:
            print("check file {0} line too little!".format(file_full_path))
            self.write_log("check file {0} line too little!".format(file_full_path))
            return False
        print("check file {0} ret Ok!".format(file_full_path))
        return True
        
    def Save1mDataFromFile(self, file_full_path, code):
        print("Save1mDataFromFile {0}".format(file_full_path))
        sql = "CREATE TABLE IF NOT EXISTS {0}_1M( longdate INTEGER, time INTEGER, open DOUBLE, close DOUBLE, high DOUBLE, low DOUBLE, vol DOUBLE, PRIMARY KEY(longdate, time) )".format(code)
        self.cur.execute(sql)
        reobj = re.compile(REG_STR)
        re.compile(reobj)
        fh = open(file_full_path)   
        line_count = 0
        lack_continued = 0
        #pre_record = (0, 0, 0, 0, 0, 0, 0) 
        pre_mon_end_trade_date = 0
        pre_line_nature_date = 0
        pre_hhmmss = 0
        pre_hhmm = 0
        pre_total_vol = 0
        pre_vol = 0
        vol = 0
        k_open = 0.0
        k_close = 0.0
        k_low = 0.0
        k_high = 0.0
        cur_price = 0.0
        is_k_first = True
        for line in fh: 
            if not line or len(line) < 1:
                    continue
            if line_count == 0:
                line_count += 1
                continue
            try:     
                cont_list = line.split(',')
                if len(cont_list) < 2:
                    line_count += 1
                    continue
                nature_date = int(cont_list[44-1]) 
                date = int(cont_list[1-1]) 
                if pre_mon_end_trade_date == 0: 
                    year = date // 10000
                    mon = (date % 10000) //100
                    day = date % 100
                    temp_date = year * 10000 + mon *100 + 1
                    i = 1
                    while not self.IsTradeDate(temp_date) and i < 28:
                        temp_date = temp_date + i
                    pre_mon_end_trade_date = self.PreTradeDate(temp_date)
                cur_price = float(cont_list[5-1]) 
                total_vol = int(cont_list[11]) 
                hhmmss_str = cont_list[21-1]
                hhmmss_list = hhmmss_str.split(':')
                hour = int(hhmmss_list[0]) 
                minute = int(hhmmss_list[1]) 
                second = int(hhmmss_list[2])  
                hhmm = hour*100 + minute
                hhmmss = hour*10000 + minute*100 + second 
                ms = int(cont_list[22-1]) 
                if (hhmm == 859 or hhmm == 2059): #front line filter unnecessary data
                    pre_total_vol = total_vol
                    line_count += 1
                    continue
                #print("{0} hour:{1} minute:{2}".format(sys._getframe().f_lineno, hour, minute))
                
                buy1_p = float(cont_list[23-1])
                buy1_v = int(cont_list[24-1])
                sell1_p = float(cont_list[25-1])
                sell1_v = int(cont_list[26-1])
                #print("{0} {1}:{2}:{3} {4} b_p:{5} b_v:{6} s_p:{7} s_v:{8} vol:{9}".format(date, hour,minute,second,ms, buy1_p,buy1_v,sell1_p,sell1_v, total_vol))
                if is_k_first:
                    is_k_first = False
                    k_open = k_low = k_high = k_close = cur_price 
                    pre_hhmmss = hhmmss
                    pre_hhmm = hhmm 
                    pre_line_nature_date = nature_date
                if hhmm == pre_hhmm:
                    k_close = cur_price
                    #print("before cur_price:{0} k_low:{1} k_high:{2}".format(cur_price, k_low, k_high))
                    if cur_price < k_low:
                        #print("update k_low")
                        k_low = cur_price
                    elif cur_price > k_high:
                        k_high = cur_price
                        #print("update k_high")
                    vol += total_vol-pre_total_vol #累积同一分钟的成交量
                    #print("after cur_price:{0} k_low:{1} k_high:{2}".format(cur_price, k_low, k_high))
                else: #分钟切换
                    if not IsLegalTradeTime(hhmm): #有时文件末尾出现 15:17:27 > 15:00:00的数据
                        continue
                    sql = "INSERT OR REPLACE INTO {0}_1M (longdate, time, open, close, high, low, vol) values (?,?,?,?,?,?,?)".format(code)
                    if not IsTradeWindowStartHhmm(hhmm): # 当前非时段起点
                        # print("line 396 hhmm:{0} pre_hhmm:{1}".format(hhmm, pre_hhmm))
                        temp_str = "line {0} pre_hhmm:{1}, hhmm:{2} not IsTradeWindowStartHhmm".format(line_count+1, pre_hhmm, hhmm)
                        self.write_log(temp_str)
                        #print(temp_str)
                        if not IsInSameTradeWindow(hhmm, pre_hhmm): #跨时段,异常
                            temp_str = "Save1mDataFromFile {0} switch trade win but lack too much data from {1} to {2} ".format(date, pre_hhmm, hhmm)
                            print(temp_str)
                            self.write_log(temp_str) 
                            return False
                        else: #与前数据 是相同时段
                            self.write_log("pre_line_nature_date:{0} pre_hhmm:{1} date:{2} hhmm:{3}".format(pre_line_nature_date, pre_hhmm, date, hhmm))
                            t_obj = dt.datetime(pre_line_nature_date//10000, pre_line_nature_date%10000//100, pre_line_nature_date%100, pre_hhmm//100, pre_hhmm%100)
                            t_obj1 = dt.datetime(nature_date//10000, nature_date%10000//100, nature_date%100, hhmm//100, hhmm%100)
                            distance = t_obj1 - t_obj
                            is_first_fake = True
                            self.write_log("distance.seconds {0} {1}".format(distance.seconds, (t_obj < t_obj1)) )
                            if distance.seconds > 60: #中间缺少数据 
                                #fake all lack k and insert 
                                while t_obj < t_obj1: 
                                    #print(int(t_obj.date().strftime("%Y%m%d"))) 
                                    tag_hhmm = NextHhmm(pre_line_nature_date, t_obj.hour*100 + t_obj.minute) 
                                    real_trd_date = self.RealTradeDate(date, tag_hhmm) 
                                    self.g_db_conn.execute(sql, (real_trd_date, tag_hhmm, k_open, k_close, k_high, k_low, 0)) #insert 
                                    if pre_mon_end_trade_date != 0 and real_trd_date == pre_mon_end_trade_date:
                                        self.exists_pre_mon_end_night_date = True
                                        #self.write_log("exists_pre_mon_end_night_date real_trd_date:{0}".format(real_trd_date))
                                    temp_vol = 0
                                    if is_first_fake:
                                        is_first_fake = False
                                        temp_vol = vol
                                    temp_str = "Save1mDataFromFile same win fake k {0} {1} o:{2} c:{3} l:{4} h:{5} v:{6}".format(real_trd_date, tag_hhmm, k_open, k_close, k_low, k_high, temp_vol)
                                    print(temp_str)
                                    self.write_log(temp_str) 
                                    t_obj += dt.timedelta(minutes=1)
                            else: #不缺数据, 则生成前时间段的1分钟数据: sample:前一分钟若是900,则生成901 k
                                #print("line 423")
                                tag_hhmm = NextHhmm(date, pre_hhmm)
                                real_trd_date = self.RealTradeDate(date,tag_hhmm)
                                self.g_db_conn.execute(sql, (real_trd_date, tag_hhmm, k_open, k_close, k_high, k_low, vol)) #insert 
                                if pre_mon_end_trade_date != 0 and real_trd_date == pre_mon_end_trade_date:
                                    self.exists_pre_mon_end_night_date = True
                                    #self.write_log("exists_pre_mon_end_night_date real_trd_date:{0} ".format(real_trd_date))
                                temp_str = "Save1mDataFromFile generat k {0} {1} o:{2} c:{3} l:{4} h:{5} v:{6}".format(real_trd_date, tag_hhmm, k_open, k_close, k_low, k_high, vol)
                                #print(temp_str)
                                self.write_log(temp_str)
                    else: # 当前是时段起点
                        if not IsTradeWindowEndHhmm(pre_hhmm): #前面不是时段终点,则缺少了数据 
                            #print("line 430")
                            w_i = TimeInWhichTradeWindow(pre_hhmm)
                            if w_i >= 0: #if  < 0, may be time 859
                                temp_hhmm = TradeWindowEndHhmm(w_i) 
                                self.write_log("当前是时段起点 前面不是时段终点 pre_line_nature_date:{0} pre_hhmm:{1} date:{2} hhmm:{3}".format(pre_line_nature_date, pre_hhmm, date, hhmm))
                                t_obj = dt.datetime(pre_line_nature_date//10000, pre_line_nature_date%10000//100, pre_line_nature_date%100, pre_hhmm//100, pre_hhmm%100)
                                t_obj1 = dt.datetime(nature_date//10000, nature_date%10000//100, nature_date%100, temp_hhmm//100, temp_hhmm%100)
                                is_first_fake = True
                                while t_obj < t_obj1: #补充数据 fake 
                                    #print(int(t_obj.date().strftime("%Y%m%d")))
                                    tag_hhmm = NextHhmm(pre_line_nature_date, t_obj.hour*100 + t_obj.minute)
                                    real_trd_date = self.RealTradeDate(date, tag_hhmm) 
                                    temp_vol = 0
                                    if is_first_fake:
                                        is_first_fake = False
                                        temp_vol = vol
                                    self.g_db_conn.execute(sql, (real_trd_date, tag_hhmm, k_open, k_close, k_high, k_low, temp_vol)) #insert 
                                    if pre_mon_end_trade_date != 0 and real_trd_date == pre_mon_end_trade_date:
                                        self.exists_pre_mon_end_night_date = True
                                        #self.write_log("exists_pre_mon_end_night_date real_trd_date:{0} ".format(real_trd_date))
                                    temp_str = "Save1mDataFromFile diff win fake k {0} {1} o:{2} c:{3} l:{4} h:{5} v:{6}".format(real_trd_date, tag_hhmm, k_open, k_close, k_low, k_high, temp_vol)
                                    #print(temp_str) 
                                    self.write_log(temp_str) 
                                    t_obj += dt.timedelta(minutes=1)
                        else: #前面是时段终点, 则生成对应的1分钟数据
                            #print("line 450") 
                            real_trd_date = self.RealTradeDate(date,pre_hhmm)
                            self.g_db_conn.execute(sql, (real_trd_date, pre_hhmm, k_open, k_close, k_high, k_low, vol)) #insert 
                            if pre_mon_end_trade_date != 0 and real_trd_date == pre_mon_end_trade_date:
                                self.exists_pre_mon_end_night_date = True
                            temp_str = "Save1mDataFromFile generat k {0} {1} o:{2} c:{3} l:{4} h:{5} v:{6}".format(real_trd_date, pre_hhmm, k_open, k_close, k_low, k_high, vol)
                            #print(temp_str)
                            self.write_log(temp_str) 
                    k_open = k_low = k_high = k_close = cur_price
                    pre_vol = vol
                    vol = total_vol-pre_total_vol  
                    #end of 分钟切换
                    
                pre_total_vol = total_vol
                pre_hhmmss = hhmmss    
                pre_hhmm = hhmm 
                pre_line_nature_date = nature_date
            except Exception as e:
                temp_str = "Save1mDataFromFile {0} line {1} exception {2}".format(file_full_path, line_count+1, e)
                print(temp_str)
                self.write_log(temp_str)
                return False  
            line_count += 1
            
        if line_count < 3:
            temp_str = "Save1mDataFromFile check file {0} line too little!".format(file_full_path)
            print(temp_str)
            self.write_log(temp_str)
            return False
        #insert last k
        t_obj = dt.datetime(date//10000, date%10000//100, date%100, pre_hhmmss//10000, pre_hhmmss%10000//100, pre_hhmmss%100)
        if t_obj.second > 0 and t_obj.second <= 59:
            t_obj += dt.timedelta(minutes=1)
        tag_hhmm = t_obj.hour*100 + t_obj.minute 
        sql = "INSERT OR REPLACE INTO {0}_1M (longdate, time, open, close, high, low, vol) values (?,?,?,?,?,?,?)".format(code)
        if vol != 0:
            vol += pre_vol
        else:
            vol = pre_vol
        real_trd_date = self.RealTradeDate(date,tag_hhmm)
        self.g_db_conn.execute(sql, (real_trd_date, tag_hhmm, k_open, k_close, k_high, k_low, vol)) #insert 
        self.g_db_conn.commit()
        temp_str = "Save1mDataFromFile end k generate k {0} {1} o:{2} c:{3} l:{4} h:{5} v:{6}".format(real_trd_date, tag_hhmm, k_open, k_close, k_low, k_high, vol)
        #print(temp_str)
        self.write_log(temp_str)
        
        print("Save1mDataFromFile check file {0} ret Ok!".format(file_full_path))
        return True
    
    def Save5mDataFrom1mData(self, code, date_para):   
        ret = self.Save5mDataFrom1mData_sub(code, date_para, 901, 1500)
        if ret:
            ret = self.Save5mDataFrom1mData_sub(code, date_para, 2101, 2359)
        if ret:
            ret = self.Save5mDataFrom1mData_sub(code, date_para, 1, 230)
        return ret
        
      
def main():
    global CODE, DATA_FILE_PATH
    print("main")
    if "PYTHONPATH" in os.environ:
        mystr = os.environ["PYTHONPATH"] 
        print(mystr)
    obj = FUTUREBASIC() 
    #print("before check")
    #index = obj.TimeInWhichTradeWindow(2100)
    #print("index:{0}".format(index))
    #index = obj.TimeInWhichTradeWindow(0)
    #print("index:{0}".format(index))
    #index = obj.TimeInWhichTradeWindow(1016)
    #print("index:{0}".format(index))
    
    #help(ts.trade_cal)
    #df = getTradeCal()
    
    date = 0  #20201012  #20201009
    #pre_trd_date = obj.PreTradeDate(date)
    #if pre_trd_date:
    #    print(pre_trd_date)
    #else:
    #    print("fail!")
    #return
     
    #202010  -------------    
    #DATA_FILE_PATH = 'F:/StockHisdata/SCL8/202010'
    #code = "SC2012"
    
    #201901 -------------
    #DATA_FILE_PATH = 'F:/StockHisdata/SCL8/201901'
    #code = "SC1903"
    
    #201902 -------20190118主力切换为sc1904------
    #DATA_FILE_PATH = 'F:/StockHisdata/SCL8/201901'
    #DATA_FILE_PATH = 'C:\\StockHisdata\\SCL8\\201902'
    #code = "SC1904"
    
    #date = 20201013  # 20201014  #  20201012 #20201009
    #file_full_path = "{0}/{1}/{2}_{3}.csv".format(DATA_FILE_PATH, date, code, date) 
     
    if 0:
        for file in os.listdir(DATA_FILE_PATH):  
            if not file.isdigit():
                continue
            date = int(file)
            path = os.path.join(DATA_FILE_PATH, file)  
            if os.path.isdir(path):   
                obj.ClearRelDbData(code, date)
        return
    if 0:   
        #DATA_FILE_PATH = 'C:\\StockHisdata\\SCL8\\201903'
        date = 20190320
        code = "SC1904"
        file_full_path = "C:\\StockHisdata\\SCL8\\201903\\{0}\\{1}_{2}.csv".format(date, code, date)
        ret = obj.ChecFenbiDataFromFile(file_full_path, code)
        if ret:
            obj.ClearRelDbData(code, date)
            ret = obj.Save1mDataFromFile(file_full_path, code) 
        #return
    if 0:
        ret = obj.Save5mDataFrom1mData(code, date)
        return
        
    if 0:  
        for file in os.listdir(DATA_FILE_PATH):  
            if not file.isdigit():
                continue
            path = os.path.join(DATA_FILE_PATH, file)  
            if os.path.isdir(path):   
                try:
                    date = int(file)
                    file_full_path = "{0}/{1}_{2}.csv".format(path, code, date) 
                    ret = obj.Save30mDataFrom5mData(code, date)
                    if not ret:
                        break
                except Exception as e:
                    print(e)
                    ret = False 
                    
        if ret:                    
            print("trans has Finish!")
        else:
            print("trans Failed!")
        #ret = obj.Save30mDataFrom5mData(code, 20201021) 
        return
        
    #check file -----------------------------
    date = 0
    ret = False
    for file in os.listdir(DATA_FILE_PATH):  
        if not file.isdigit():
            continue
        date = int(file)
        path = os.path.join(DATA_FILE_PATH, file)  
        if os.path.isdir(path):  
            file_full_path = "{0}\\{1}_{2}.csv".format(path, CODE, date) 
            #print("ChecFenbiDataFromFile {0}".format(file_full_path))
            ret = obj.ChecFenbiDataFromFile(file_full_path, CODE)
            if not ret:
                return
                
    #del related date in db------------------
    for file in os.listdir(DATA_FILE_PATH):  
        if not file.isdigit():
            continue
        date = int(file)
        path = os.path.join(DATA_FILE_PATH, file)  
        if os.path.isdir(path):   
            obj.ClearRelDbData(CODE, date)
            
    #create 1m k and save to db---------------         
    for file in os.listdir(DATA_FILE_PATH):  
        if not file.isdigit():
            continue
        path = os.path.join(DATA_FILE_PATH, file)  
        if os.path.isdir(path):   
            try:
                date = int(file)
                file_full_path = "{0}/{1}_{2}.csv".format(path, CODE, date) 
                ret = obj.Save1mDataFromFile(file_full_path, CODE) 
                if not ret:
                    break
            except Exception as e:
                print(e)
                ret = False 
                
    if not ret:    
        print("trans Failed!")      
        return
    #处理上月月末夜盘数据    -----------------------------
    if obj.exists_pre_mon_end_night_date:
        year = date // 10000
        mon = (date % 10000) //100
        day = date % 100
        temp_date = year * 10000 + mon *100 + 1
        pre_mon_end_trade_date = obj.PreTradeDate(temp_date)
        obj.write_log("exists_pre_mon_end_night_date Save5mDataFrom1mData pre_mon_end_trade_date:{0} temp_date:{1} ".format(pre_mon_end_trade_date, temp_date))
        ret = obj.Save5mDataFrom1mData(CODE, pre_mon_end_trade_date) 
        
    #create 5m k and save to db------因为1m钟有日期改动 所以5m 必须独立,以等1m全部完成-----------                    
    for file in os.listdir(DATA_FILE_PATH):  
        if not file.isdigit():
            continue
        path = os.path.join(DATA_FILE_PATH, file)  
        if os.path.isdir(path):   
            try:
                date = int(file)
                file_full_path = "{0}/{1}_{2}.csv".format(path, CODE, date) 
                ret = obj.Save5mDataFrom1mData(CODE, date) 
                if not ret:
                    break
            except Exception as e:
                print(e)
                ret = False 
                
    if not ret:    
        print("trans Failed!")      
        return
    #处理上月月末夜盘数据    -----------------------------
    if obj.exists_pre_mon_end_night_date:
        year = date // 10000
        mon = (date % 10000) //100
        day = date % 100
        temp_date = year * 10000 + mon *100 + 1
        pre_mon_end_trade_date = obj.PreTradeDate(temp_date)
        ret = obj.Save30mDataFrom5mData(CODE, pre_mon_end_trade_date) 
        
    #create 30m k and save to db------因为 0 分钟建立在  5m的下一交易日上 必须独立-----------                    
    for file in os.listdir(DATA_FILE_PATH):  
        if not file.isdigit():
            continue
        path = os.path.join(DATA_FILE_PATH, file)  
        if os.path.isdir(path):   
            try:
                date = int(file)
                file_full_path = "{0}/{1}_{2}.csv".format(path, CODE, date) 
                ret = obj.Save30mDataFrom5mData(CODE, date)
                if not ret:
                    break
            except Exception as e:
                print(e)
                ret = False 
                
    if ret:                    
        print("trans has Finish!")
    else:
        print("trans Failed!")
    #datetime.datetime (year, month, day[ , hour[ , minute[ , second[ , microsecond[ , tzinfo] ] ] ] ] )
    #t_obj = dt.datetime(2020, 10, 9, 22, 29, 12)
    #t_obj1 = t_obj + dt.timedelta(minutes=1)
    #print(t_obj1.minute) 
     
main()    
    
    