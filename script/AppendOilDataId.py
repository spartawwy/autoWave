# coding=utf-8
 
import sys
import os
import time
import re
import string
import sqlite3  
import tushare as ts  
import datetime as dt  
from datetime import timedelta
from datetime import datetime
from pypinyin import pinyin, lazy_pinyin
  
ROOT_DIR = './' 
DB_FILE_PATH = '../build/Win32/Debug/hqhis.kd'
#DATA_FILE_0 = 'D:/ProgramFilesBase/StockData/FutureData/SCL9/2018/SQSC13.csv'
DATA_FILE_1 = 'D:/ProgramFilesBase/StockData/FutureData/SCL9/2019/SQSC13.csv'
    
class FUTUREBASIC:  
    def __init__(self):  
        self.cal_dates = ts.trade_cal() #return calendar of exchange center, DataFrame, calendarDate,isOpen  
        #self.data_dir = "C:/"
        #if "STK_DATA_DIR" in os.environ:
        #    self.data_dir = os.environ["STK_DATA_DIR"] 
        self.data_dir = "."
        self.file_ok_ext = ".ok"    
        log_dir = self.data_dir + "\\log\\"
        if not os.path.exists(log_dir):
            os.makedirs(log_dir) 
        self.log_file_path = log_dir + "get_oildata_log_" + time.strftime("%Y%m%d", time.localtime()) + ".txt"   
        self.log_file_handle = open(self.log_file_path, 'a')
        self.g_db_conn = False
        self.g_db_conn = self.open_db()
        self.record_id = 0
        
    def __del__(self):
        print("del self")
        self.close_db()
        if self.log_file_handle:
            self.log_file_handle.close()
        
    def open_db(self):
        ''' open data base 保存数据库'''
        global DB_FILE_PATH 
        if not os.access(DB_FILE_PATH, os.F_OK):
            #print ("%s not exist\n" % DB_FILE_PATH)
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
        
    def get_stk_baseinfo(self):
        count = 0
        self.cur = self.g_db_conn.cursor()
        for row in self.cur:
            count = row[0]
            break
        stock_info = ts.get_stock_basics() 
        self.cur.execute("SELECT count(code) FROM stock")  
        type = 0; # 0--stock 1--index 
        num = 0
        for i in stock_info.index:
            py_str = ''.join(self.getpinyinhead(stock_info.ix[i]['name'])) 
            sql = "INSERT OR REPLACE INTO stock VALUES(?, ?, ?, ?, ?, ?, ?, '')"
            self.cur.execute(sql, (i, type, stock_info.ix[i]['name'], py_str, str(stock_info.ix[i]['timeToMarket']), stock_info.ix[i]['industry'], stock_info.ix[i]['area'])) 
            num += 1 
        type = 1; # 0--stock 1--index 
        index_array = [('999999', '上证指数', 'SZZS') 
                       ]
        for i in range(0, len(index_array)):
            sql = "INSERT OR REPLACE INTO stock VALUES(?, ?, ?, ?, 0, '', '', '')"
            self.cur.execute(sql, (index_array[i][0], type, index_array[i][1], index_array[i][2]))
            print("insert {} {} {} {}".format(index_array[i][0], type, index_array[i][1], index_array[i][2]))
            num += 1
            
        self.g_db_conn.commit()
        print("has insert or replace {} records".format(num))
        return "ok"
        
    def Check1mDataFromFile(self, file_full_path):
        print("Check1mDataFromFile {0}".format(file_full_path))
        self.write_log("Check1mDataFromFile {0}".format(file_full_path))
        reobj = re.compile("^(\\d{4})-(\\d{1,2})-(\\d{1,2}),(\\d{2}):(\\d{2}),(\\d+\\.\\d+|\\d+),(\\d+\\.\\d+|\\d+),(\\d+\\.\\d+|\\d+),(\\d+\\.\\d+|\\d+),(\\d+),(\\d+)(.*)$")
        re.compile(reobj)
        fh = open(file_full_path)
        is_firt_line = True
        is_last_line = False
        #pre_hhmm = 0
        line_count = 0
        lack_continued = 0
        pre_record = (0, 0, 0, 0, 0, 0, 0)
        for line in fh:
            try:
                #print(line)
                line_count += 1
                match1 = re.search(reobj, line) 
                year = int(match1.group(1)) 
                month = int(match1.group(2))
                day = int(match1.group(3)) 
                longdate = int(year)*10000 + int(month)*100 + int(day)
                hour = int(match1.group(4))
                minute = int(match1.group(5))
                #print("{0} hour:{1} minute:{2}".format(sys._getframe().f_lineno, hour, minute))
                hhmm = int(hour)*100 + int(minute) 
                price_o = match1.group(6) 
                price_h = match1.group(7) 
                price_l = match1.group(8) 
                price_c = match1.group(9) 
                #vol = match1.group(10)
                #hold = match1.group(11)
                if is_firt_line:
                    is_firt_line = False
                    next_date_t = GetNextDateTime(int(year), int(month), int(day), int(hour), int(minute))
                    next_hhmm = next_date_t[1]
                else:
                    if int(hour)*100 + int(minute) != next_hhmm: 
                        cur_line_dt = datetime(year, month, day, hour, minute) 
                        d = datetime(next_date_t[0]//10000, next_date_t[0]%10000//100, next_date_t[0]%100, next_hhmm//100, next_hhmm%100)
                        time_distance = cur_line_dt - d
                        if abs(time_distance.seconds) > 29*60:
                            temp_str = "file:{0} from line {1} {2}-{3}-{4},{5}:{6} too much lack ".format(file_full_path, line_count, year, month, day, hour, minute)
                            print(temp_str)
                            self.write_log(temp_str)
                            return False
                    lack_continued = 0 
                    next_date_t = GetNextDateTime(int(year), int(month), int(day), int(hour), int(minute))
                    next_hhmm = next_date_t[1] 
            except Exception as e:
                temp_str = "{0} line {1} esception {2}".format(file_full_path, line_count, e)
                print(temp_str)
                self.write_log(temp_str)
                return False  
        print("check file {0} ret Ok!".format(file_full_path))
        return True
        
    def Save1mDataFromFile(self, file_full_path):
        print("Check1mDataFromFile {0}".format(file_full_path))
        self.write_log("Check1mDataFromFile {0}".format(file_full_path))
        sql = '''INSERT OR REPLACE INTO SCL9_1M (longdate, time, open, close, high, low, vol) values (?,?,?,?,?,?,?)'''
        reobj = re.compile("^(\\d{4})-(\\d{1,2})-(\\d{1,2}),(\\d{2}):(\\d{2}),(\\d+\\.\\d+|\\d+),(\\d+\\.\\d+|\\d+),(\\d+\\.\\d+|\\d+),(\\d+\\.\\d+|\\d+),(\\d+),(\\d+)(.*)$")
        re.compile(reobj)
        fh = open(file_full_path)
        is_firt_line = True
        is_last_line = False
        #pre_hhmm = 0
        line_count = 0
        lack_continued = 0
        pre_record = (0, 0, 0, 0, 0, 0, 0)
        for line in fh:
            try:
                #print(line)
                line_count += 1
                match1 = re.search(reobj, line)
                year = match1.group(1)
                month = match1.group(2)
                day = match1.group(3)
                longdate = int(year)*10000 + int(month)*100 + int(day);
                hour = match1.group(4)
                minute = match1.group(5)
                hhmm = int(hour)*100 + int(minute)
                price_o = match1.group(6)
                price_h = match1.group(7)
                price_l = match1.group(8)
                price_c = match1.group(9)
                vol = match1.group(10)
                hold = match1.group(11)
                if is_firt_line:
                    is_firt_line = False
                    self.cur.execute(sql, (longdate, hhmm, price_o, price_c, price_h, price_l, vol))
                    pre_record = (longdate, hhmm, price_o, price_c, price_h, price_l, 0)
                    next_date_t = GetNextDateTime(int(year), int(month), int(day), int(hour), int(minute))
                    next_hhmm = next_date_t[1]
                else:
                    next_hhmm = next_date_t[1]
                    if next_hhmm != int(hour)*100 + int(minute):
                        #fake all lost records and insert to db------------
                        while True:
                            lack_continued += 1
                            print("line {0} {1} lack to fake {2}-{3}-{4},{5}:{6}".format(line_count, int(hour)*100+int(minute), int(year), int(month), int(day), int(int(next_hhmm)/100), int(next_hhmm)%100))
                            self.write_log("line {0} {1} lack to fake {2}-{3}-{4},{5}:{6}".format(line_count, int(hour)*100+int(minute), int(year), int(month), int(day), int(int(next_hhmm)/100), int(next_hhmm)%100))
                            via = list(pre_record)
                            via[0]= next_date_t[0] #edit longdate
                            via[1]= next_date_t[1] #edit hhmm
                            pre_record = tuple(via)#edit fake data
                            self.cur.execute(sql, pre_record) #insert fake record
                            next_date_t = GetNextDateTime(int(year), int(month), int(day), int(int(next_hhmm)/100), int(next_hhmm)%100)
                            next_hhmm = next_date_t[1]
                            if next_date_t[0] == longdate and next_hhmm == int(hour)*100 + int(minute):
                                break 
                    lack_continued = 0
                    self.cur.execute(sql, (longdate, hhmm, price_o, price_c, price_h, price_l, vol)) 
                    next_date_t = GetNextDateTime(int(year), int(month), int(day), int(hour), int(minute))
                    #print(sys._getframe().f_lineno)
                    pre_record = (longdate, hhmm, price_o, price_c, price_h, price_l, 0) 
            except Exception as e:
                print(e)
                return
        self.g_db_conn.commit()
        temp_str = "Save1mDataFromFile {0} ok!".format(file_full_path)
        print(temp_str)
        self.write_log(temp_str)
        print("out proc")   
    
    def CheckBeforSave5mDataFrom1mData(self, start_date, start_hhmm, end_date, end_hhmm):
        sql = "SELECT longdate, time, open, close, high, low, vol FROM SCL9_1M WHERE longdate >= {0} AND longdate <= {1} ORDER BY longdate, time".format(start_date, end_date)
        cursor = self.cur.execute(sql)
        count = 0 
        is_any_bug = False
        for it in cursor:
            count = count + 1
            it_date = it[0]
            it_time = it[1]
            if it_date == 20191225 and it_time == 1500:
                if count % 5 != 0:
                    is_any_bug = True
                    break
                count = 0
            if count % 5 == 1:
                if int(it_time) % 5 != 1:
                    is_any_bug = True
                    break
            elif count % 5 != 0:
                if int(it_time) % 5 == 0:
                    is_any_bug = True
                    break
            else:
                if int(it_time) % 5 != 0:
                    is_any_bug = True
                    break
        if is_any_bug:
            bug_str = "lack data:{0} {1}".format(it_date, it_time)
            print(bug_str)
            self.write_log(bug_str)
            return False 
        return True
    
    def Del5mDataBeforeDate(self, end_date):
        sql = "Delete FROM SCL9_5M WHERE longdate <= {0}".format(end_date)
        self.g_db_conn.execute(sql)
        print(sql)
        write_log(sql)
        self.g_db_conn.commit()
        
    def Save5mDataFrom1mData(self, start_date, start_hhmm, end_date, end_hhmm):
        sql = "SELECT longdate, time, open, close, high, low, vol FROM SCL9_1M WHERE longdate >= {0} AND longdate <= {1} ORDER BY longdate, time".format(start_date, end_date)
        cursor = self.cur.execute(sql)
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
            else:
                close_5m = it_close
                if it_high > high_5m:
                    high_5m = it_high
                if it_low < low_5m:
                    low_5m = it_low
                vol = vol + int(it_vol) 
                sql = "INSERT OR REPLACE INTO SCL9_5M VALUES({0}, {1}, {2}, {3}, {4}, {5}, {6})".format(it_date, it_time, open_5m, close_5m, high_5m, low_5m, vol)
                self.g_db_conn.execute(sql)# not use cursor,cause cursor is using by for
                self.write_log(sql)
        self.g_db_conn.commit()
        print("Save5mDataFrom1mData {0}, {1} ok!".format(start_date, end_date))
        return True
        
    def CheckBeforeSave15mDataFrom5mData(self, start_date, start_hhmm, end_date, end_hhmm):    
        sql = "SELECT longdate, time, open, close, high, low, vol FROM SCL9_5M WHERE longdate >= {0} AND longdate <= {1} ORDER BY longdate, time".format(start_date, end_date) 
        cursor = self.cur.execute(sql)
        count = 0   
        for it in cursor:
            count = count + 1 
            it_date = it[0]
            it_time = it[1] 
            #print("count {0} {1}".format(count, count % 3))
            if count != 1:
                if next_stamp != it_time:
                    erro_str = "CheckBeforeSave15mDataFrom5mData: error VALUES count:{0} {1} hhmm {2} != next_stamp:{3}".format(count, it_date, it_time, next_stamp)
                    print(erro_str)
                    self.write_log(erro_str) 
                    return False
            if it_time == 1500 and count % 3 != 0:
                erro_str = "CheckBeforeSave15mDataFrom5mData:error VALUES count:{0} {1} hhmm {2} != next_stamp:{3}".format(count, it_date, it_time, next_stamp)
                print(erro_str)
                self.write_log(erro_str) 
                return False
            next_stamp = Next15m(it_date, it_time)
            #self.write_log("Next15m {0} {1} is {2}".format(it_date, it_time, next_stamp))  
        self.write_log("CheckBeforeSave15mDataFrom5mData {0}, {1} ok!".format(start_date, end_date)) 
        return True
        
    def Save15mDataFrom5mData(self, start_date, start_hhmm, end_date, end_hhmm):
        sql = "SELECT longdate, time, open, close, high, low, vol FROM SCL9_5M WHERE longdate >= {0} AND longdate <= {1} ORDER BY longdate, time".format(start_date, end_date)
        cursor = self.cur.execute(sql)
        count = 0
        max_price = 999.9
        min_price = 0.0
        high = min_price
        low = max_price
        open_15m = 0.0
        close_15m = 0.0
        high_15m = min_price
        low_15m = max_price
        vol = 0
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
            #print("count {0} {1}".format(count, count % 3))
            if count % 3 == 1:
                open_15m = it_open 
                close_15m = it_close
                high_15m = it_high
                low_15m = it_low
                vol = int(it_vol)
            elif count % 3 != 0:
                if it_high > high_15m:
                    high_15m = it_high
                if it_low < low_15m:
                    low_15m = it_low
                vol = vol + int(it_vol)
            else:
                close_15m = it_close
                if it_high > high_15m:
                    high_15m = it_high
                if it_low < low_15m:
                    low_15m = it_low
                vol = vol + int(it_vol) 
                sql = "INSERT OR REPLACE INTO SCL9_15M VALUES({0}, {1}, {2}, {3}, {4}, {5}, {6})".format(it_date, it_time, open_15m, close_15m, high_15m, low_15m, vol)
                self.g_db_conn.execute(sql)# not use cursor,cause cursor is using by for
                self.write_log(sql)
        self.g_db_conn.commit()
        print("Save15mDataFrom5mData {0}, {1} ok!".format(start_date, end_date)) 
        self.write_log("Save15mDataFrom5mData {0}, {1} ok!".format(start_date, end_date)) 
        
    def Save30mDataFrom5mData(self, start_date, start_hhmm, end_date, end_hhmm):
        sql = "SELECT longdate, time, open, close, high, low, vol FROM SCL9_5M WHERE longdate >= {0} AND longdate <= {1} ORDER BY longdate, time".format(start_date, end_date)
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
            if it_time == 1500:
                close_30m = it_close
                if it_high > high_30m:
                    high_30m = it_high
                if it_low < low_30m:
                    low_30m = it_low
                vol = vol + int(it_vol) 
                sql = "INSERT OR REPLACE INTO SCL9_30M VALUES({0}, {1}, {2}, {3}, {4}, {5}, {6})".format(it_date, it_time, open_30m, close_30m, high_30m, low_30m, vol)
                self.g_db_conn.execute(sql)# not use cursor,cause cursor is using by for
                self.write_log(sql)
                vol = 0
                count = 0
            elif count % 6 == 1:
                open_30m = it_open 
                close_30m = it_close
                high_30m = it_high
                low_30m = it_low
                vol = int(it_vol)
            elif count % 6 != 0:
                if it_high > high_30m:
                    high_30m = it_high
                if it_low < low_30m:
                    low_30m = it_low
                vol = vol + int(it_vol)
            else:
                close_30m = it_close
                if it_high > high_30m:
                    high_30m = it_high
                if it_low < low_30m:
                    low_30m = it_low
                vol = vol + int(it_vol) 
                sql = "INSERT OR REPLACE INTO SCL9_30M VALUES({0}, {1}, {2}, {3}, {4}, {5}, {6})".format(it_date, it_time, open_30m, close_30m, high_30m, low_30m, vol)
                self.g_db_conn.execute(sql)# not use cursor,cause cursor is using by for
                self.write_log(sql)
        self.g_db_conn.commit()
        print("Save30mDataFrom5mData {0}, {1} ok!".format(start_date, end_date)) 
        
#def sprintf(s, fs, *args):
#    global s
#    s = fs % args

def Next15m(longdate, hhmm):
        hour = hhmm // 100
        minute = hhmm % 100
        special_dates = [20180404, 20180427, 20180615, 20180921, 20180928, 20181228, 20190201, 20190404, 20190430, 20190606, 20190628, 20190912, 20190930, 20191130]
        next_dates    = [20180409, 20180502, 20180619, 20180925, 20181008, 20190102, 20190211, 20190408, 20190506, 20190610, 20190701, 20190916, 20191008, 20191202]
        if longdate in special_dates and hour == 15 and minute == 0:
            return 905
        if longdate == 20191225 and hour == 15 and minute == 0:
            return 2235
        if hhmm == 1015:
            return 1035
        elif hhmm == 1500:
            return 2105
        elif hhmm == 1130:
            return 1335
        elif hhmm == 230:
            return 905
        else:
            d = datetime(longdate//10000, longdate%10000//100, longdate%100, hhmm//100, hhmm%100)
            e = d + timedelta(minutes=5)
            return e.hour * 100 + e.minute
            
def GetNextDateTime(year, month, day, hour, minute):
    d = datetime(year,month,day, hour,minute)
    e = d
    date_v = year*10000 + month*100 + day
    hm = hour*100 + minute
    next_hm = 0
    special_dates = [20180404, 20180427, 20180615, 20180921, 20180928, 20181228, 20190201, 20190404, 20190430, 20190606, 20190628, 20190912, 20190930, 20191130]
    next_dates    = [20180409, 20180502, 20180619, 20180925, 20181008, 20190102, 20190211, 20190408, 20190506, 20190610, 20190701, 20190916, 20191008, 20191202]
    if date_v in special_dates and hour == 15 and minute == 0:
        return (next_dates[special_dates.index(date_v)], 901)
    if date_v == 20191225 and hour == 15 and minute == 0:
        return (20191225, 2231)
    if hm < 229:
        e = d + timedelta(minutes=1)
        next_hm = e.hour * 100 + e.minute
    elif hm == 230:
        next_hm = 901
    elif hm < 1015:
        e = d + timedelta(minutes=1)
        next_hm = e.hour * 100 + e.minute
    elif hm == 1015:
        next_hm = 1031
    elif hm < 1130:
        e = d + timedelta(minutes=1)
        next_hm = e.hour * 100 + e.minute
    elif hm == 1130:
        next_hm = 1331
    elif hm < 1500:
        e = d + timedelta(minutes=1)
        next_hm = e.hour * 100 + e.minute
    elif hm == 1500:
        next_hm = 2101
    elif hm < 2359:
        e = d + timedelta(minutes=1)
        next_hm = e.hour * 100 + e.minute
    elif hm == 2359:
        next_hm = 0
    return (e.year*10000+e.month*100+e.day, next_hm)
    

    
if __name__ == "__main__":  
    print("main")
    if "PYTHONPATH" in os.environ:
        mystr = os.environ["PYTHONPATH"] 
        print(mystr)
    #obj = STOCKBASIC()
    #if 1:
    #    obj.get_stk_baseinfo()
    #time0 = timedelta(days=0, hours=11, minutes=1)
    #year = 2018
    #month = 2
    #day = 3
    #hour = 2
    #minute = 59
    #next_hm = GetNextDateTime(year, month, day, hour, minute)
    #print(next_hm)
    obj = FUTUREBASIC() 
    if 0:
        ret0 = obj.Check1mDataFromFile(DATA_FILE_0)
        ret1 = obj.Check1mDataFromFile(DATA_FILE_1)
        if not ret0 or not ret1:
            print("stop")
            sys.exit()
        if ret0:
            obj.Save1mDataFromFile(DATA_FILE_0)
        if ret1:
            obj.Save1mDataFromFile(DATA_FILE_1)  
    start_date = 20180301
    end_date = 20191231
    stat_time = 900
    end_time = 1500
    if 0:
        ret2 = obj.CheckBeforSave5mDataFrom1mData(start_date, stat_time, end_date, end_time)
        if ret2:
            obj.Del5mDataBeforeDate(end_date)
            obj.Save5mDataFrom1mData(start_date, stat_time, end_date, end_time)
    if 0:
        ret3 = obj.CheckBeforeSave15mDataFrom5mData(start_date, stat_time, end_date, end_time)
        if ret3:
            ret3 = obj.Save15mDataFrom5mData(start_date, stat_time, end_date, end_time)
    if 1:
        obj.Save30mDataFrom5mData(start_date, stat_time, end_date, end_time)       
            