#include "wave.h"
#include "stock_data_man.h"
#include "futures_forecast_app.h"

void FindDownTowardLeftEnd(std::deque<std::shared_ptr<T_KlineDataItem> > &kline_data_items, bool is_src_k_same, double lowest_price, int start, INOUT int &end); 
void FindUpTowardLeftEnd(std::deque<std::shared_ptr<T_KlineDataItem> > &kline_data_items, bool is_src_k_same, double highest_price, int start, INOUT int &end);


bool Traverse_GetWaves(IN T_StructLineContainer &container, IN T_HisDataItemContainer & kline_data_items, int r_start_index, int backward_size, OUT Wave &wave)
{
    return true;
}


// return index 
// ps : towards left items' right end related r_start_index == 0  
int WaveMan::Traverse_GetWaveLevel1(const std::string &code, TypePeriod type_period, int r_start_index, int backward_size/*, OUT std::deque<std::shared_ptr<Wave> > &container*/)
{  
    auto find_next_up_struct_line_end_hight_p = [](T_HisDataItemContainer &kline_data_items, int index, int backward_size, double pre_btm_price)->double
    { 
        if( kline_data_items.size() < 1 )
            return -1 * MAX_PRICE;
        int temp_index = index;
        int num = backward_size > 0 ? backward_size : kline_data_items.size();
        double temp_pre_btm_price = pre_btm_price;
        while( --temp_index > 0 && num-- > 0 )
        {
            if( IsTopFractal(kline_data_items[temp_index]->type) )
            {
                int temp_end = temp_index;
                double temp_b_price = MAX_PRICE;
                int temp_b_index = find_next_btm_fractal(kline_data_items, temp_index);
                if( temp_b_index != -1 )
                {
                    temp_b_price = kline_data_items[temp_b_index]->stk_item.low_price;
                    temp_end = temp_b_index;
                } 
                FindDownTowardLeftEnd(kline_data_items, false, temp_b_price, temp_index, temp_end);
                if( temp_index != temp_end )
                {
                    if( kline_data_items[temp_end]->stk_item.low_price < temp_pre_btm_price )
                    {  
                        return kline_data_items[temp_index]->stk_item.high_price;
                    }
                } 
            }else if( IsBtmFractal(kline_data_items[temp_index]->type) )
                temp_pre_btm_price = MAX_PRICE;
        } // while( --temp_index > 0 )
        return -1 * MAX_PRICE;
    };

    //towards left
    auto find_next_down_struct_line_end_low_p = [](T_HisDataItemContainer &kline_data_items, int index, int backward_size, double pre_top_price)->double
    { 
        if( kline_data_items.size() < 1 )
            return -1 * MAX_PRICE;
        int temp_index = index;
        int num = backward_size > 0 ? backward_size : kline_data_items.size();
        double temp_pre_top_price = pre_top_price;
        while( --temp_index > 0 && num-- > 0 )
        {
            if( IsBtmFractal(kline_data_items[temp_index]->type) )
            {
                int temp_end = temp_index;
                double temp_b_price = MIN_PRICE;
                int temp_b_index = find_next_top_fractal(kline_data_items, temp_index);
                if( temp_b_index != -1 )
                {
                    temp_b_price = kline_data_items[temp_b_index]->stk_item.high_price;
                    temp_end = temp_b_index;
                } 
                FindUpTowardLeftEnd(kline_data_items, false, temp_b_price, temp_index, temp_end);
                if( temp_index != temp_end )
                {
                    if( kline_data_items[temp_end]->stk_item.high_price > temp_pre_top_price )
                    {  
                        return kline_data_items[temp_index]->stk_item.low_price;
                    }
                } 
            }else if( IsTopFractal(kline_data_items[temp_index]->type) )
                temp_pre_top_price = MIN_PRICE;
        } // while( --temp_index > 0 )
        return -1 * MAX_PRICE;
    };

    unsigned int level = 1;
    T_HisDataItemContainer & kline_data_items = app_.stock_data_man().GetHisDataContainer(type_period, code);
    if( kline_data_items.empty() )
        return 0;
    assert(r_start_index >= 0);
    assert(r_start_index < kline_data_items.size());
     
    unsigned int index = kline_data_items.size() - r_start_index;
    int num = backward_size > 0 ? backward_size : kline_data_items.size();

    double pre_btm_price = MAX_PRICE;
    double pre_top_price = MIN_PRICE;
    bool is_pre_add_up_line = false;
    bool is_pre_add_down_line = false;
    
    bool is_index_both_btm_top = false; 
    std::deque<std::shared_ptr<Wave> > temp_waves;
    while( --index > 0 && num-- > 0 )// towards left 
    { 
        int ck_index_date = kline_data_items[index]->stk_item.date;
        if( IsTopFractal(kline_data_items[index]->type) )
        { 
            pre_top_price = MIN_PRICE;

            int end = index; 
            double b_price = MAX_PRICE;
            int b_index = -1;
            if( is_index_both_btm_top )
                b_index = find_next_btm_fractal(kline_data_items, index - 1);
            else
                b_index = find_next_btm_fractal(kline_data_items, index);

            if( b_index != -1 )
            {
                b_price = kline_data_items[b_index]->stk_item.low_price;
                end = b_index; 
            }

            FindDownTowardLeftEnd(kline_data_items, is_index_both_btm_top, b_price, index, end);
            is_index_both_btm_top = false;
            if( index != end )
            {
                bool is_to_add_line = false;
                if( kline_data_items[end]->stk_item.low_price < pre_btm_price )
                {
                    is_to_add_line = true;
                }else
                { 
                    double next_up_end_h_p = find_next_up_struct_line_end_hight_p(kline_data_items, end, backward_size, kline_data_items[end]->stk_item.low_price);
                    if( next_up_end_h_p > 0.0 && next_up_end_h_p < kline_data_items[index]->stk_item.high_price )
                        is_to_add_line = true;
                }
                if( is_to_add_line )
                {
                    pre_btm_price = kline_data_items[end]->stk_item.low_price;

                    auto line = std::make_shared<Wave>(level, WaveType::UP, end, index);
                    MakeSubWave(code, type_period, *line, 0);
                    temp_waves.push_front(std::move(line));
                    
                    if( IsTopFractal(kline_data_items[end]->type) )
                        is_index_both_btm_top = true;
                    else
                        is_index_both_btm_top = false;
                }

                index = end + 1;
            } 

        }else if( IsBtmFractal(kline_data_items[index]->type) )
        {
            pre_btm_price = MAX_PRICE; 

            int end = index; 
            double b_price = MIN_PRICE;
            int b_index = -1;
            if( is_index_both_btm_top )
                b_index = find_next_top_fractal(kline_data_items, index - 1);
            else
                b_index = find_next_top_fractal(kline_data_items, index);
            if( b_index != -1 )
            {
                b_price = kline_data_items[b_index]->stk_item.high_price;
                end = b_index;
            } 
            FindUpTowardLeftEnd(kline_data_items, is_index_both_btm_top, b_price, index, end);
            is_index_both_btm_top = false;
            if( index != end )
            {
                bool is_to_add_down_line = false;
                if( kline_data_items[end]->stk_item.high_price > pre_top_price )
                {
                    is_to_add_down_line = true;
                }else
                { 
                    double next_down_end_l_p = find_next_down_struct_line_end_low_p(kline_data_items, end, backward_size, kline_data_items[end]->stk_item.high_price);
                    if( next_down_end_l_p > 0.0 && next_down_end_l_p > kline_data_items[index]->stk_item.low_price )
                        is_to_add_down_line = true;
                }
                if( is_to_add_down_line )
                {
                    pre_top_price = kline_data_items[end]->stk_item.high_price;

                    auto line = std::make_shared<Wave>(level, WaveType::DOWN, end, index);
                    MakeSubWave(code, type_period, *line, 0);
                    temp_waves.push_front(std::move(line));

                    if( IsBtmFractal(kline_data_items[end]->type) )
                        is_index_both_btm_top = true;
                    else
                        is_index_both_btm_top = false;
                }
                index = end + 1;
            } 
        }//end if( IsBtmFractal(kline_data_items[index]->type) )

    } // while 
    auto & waves = GetWaveContainer(code, type_period);
    if( !temp_waves.empty() )
    {
        int ret_index = 0; 
        while( !waves.empty() && waves.back()->end > temp_waves.front()->beg )
        {
            waves.pop_back();
        }
        if( !waves.empty() )
            ret_index = waves.back()->beg;
        waves.insert(waves.end(), temp_waves.begin(), temp_waves.end());
        return ret_index;
    }else
        return waves.empty() ? 0 : waves.back()->end;
}

#define TOUCH_WAVES_END  0
#define OCCURE_TREND_UP  1
#define OCCURE_TREND_DOWN 2
#define HL_BREAK_THREHOLD 0.4

int TrendDownDoAjustUp(T_HisDataItemContainer &k_datas, std::deque<std::shared_ptr<Wave> > &waves, INOUT int &w_i
                       , double trend_down_wave_high, double trend_down_wave_low, INOUT int &cur_ajust_index, INOUT int &highest_rel_w_i)
{ 
    if( ++w_i >= waves.size() )
        return TOUCH_WAVES_END;
    if( waves[w_i]->type == WaveType::UP )
    {
        //assert(waves[w_i]->type == WaveType::UP);
        double wave_i_low = KRef(k_datas, waves[w_i]->beg, KAttributeType::LOW);
        double wave_i_high = KRef(k_datas, waves[w_i]->end, KAttributeType::HIGH);
        double wave_i_close = KRef(k_datas, waves[w_i]->end, KAttributeType::CLOSE);
        
        if( wave_i_high > trend_down_wave_high + HL_BREAK_THREHOLD 
            || (wave_i_high > trend_down_wave_high && wave_i_close > trend_down_wave_high - EPSINON) )
        {
            waves[w_i]->trend = WaveTrendType::UP;
            waves[w_i]->trend_index = 1;
            return OCCURE_TREND_UP;
         }else 
         {
             if( highest_rel_w_i < 0 || wave_i_high > KRef(k_datas, waves[highest_rel_w_i]->end, KAttributeType::HIGH) )
                 highest_rel_w_i = w_i;
             waves[w_i]->trend = WaveTrendType::AJUST_TO_UP;
             waves[w_i]->trend_index = ++cur_ajust_index;
             return TrendDownDoAjustUp(k_datas, waves, w_i, trend_down_wave_high, trend_down_wave_low, cur_ajust_index, highest_rel_w_i);
         } 
    }else
    {
        assert(waves[w_i]->type == WaveType::DOWN);
        double wave_i_low = KRef(k_datas, waves[w_i]->end, KAttributeType::LOW);
        double wave_i_high = KRef(k_datas, waves[w_i]->beg, KAttributeType::HIGH);
        double wave_i_close = KRef(k_datas, waves[w_i]->end, KAttributeType::CLOSE);
        if( wave_i_low < trend_down_wave_low - HL_BREAK_THREHOLD
            || (wave_i_low < trend_down_wave_low && wave_i_close < trend_down_wave_low + EPSINON) )
        {
            waves[w_i]->trend = WaveTrendType::DOWN;
            //waves[w_i]->trend_index = 1;
            return OCCURE_TREND_DOWN;
        }else //if( wave_i_low > trend_down_wave_low - EPSINON )
        {
            waves[w_i]->trend = WaveTrendType::AJUST_TO_UP;
            waves[w_i]->trend_index = ++cur_ajust_index;
            return TrendDownDoAjustUp(k_datas, waves, w_i, trend_down_wave_high, trend_down_wave_low, cur_ajust_index, highest_rel_w_i);
        } 
    }
}

// return <ret_value, trend_end_index, highest_rel_w_i>
std::tuple<int, int, int> TraversSetTrendDownData(T_HisDataItemContainer &k_datas, std::deque<std::shared_ptr<Wave> > &waves, INOUT int &w_i
                            , double trend_down_wave_high, double trend_down_wave_low, INOUT int &trend_index)
{ 
    assert(waves[w_i]->type == WaveType::DOWN);
    double last_trend_down_wave_high = trend_down_wave_high;
    double last_trend_down_wave_low = trend_down_wave_low;

    const int old_w_i = w_i;
    int cur_ajust_index = 0;
    int highest_rel_w_i = -1;
    int ret = TrendDownDoAjustUp(k_datas, waves, w_i, last_trend_down_wave_high, last_trend_down_wave_low, cur_ajust_index, highest_rel_w_i);
    if( ret == OCCURE_TREND_UP ) // trend up
    {
        return std::make_tuple(OCCURE_TREND_UP, old_w_i, highest_rel_w_i);
    }else if( ret == OCCURE_TREND_DOWN ) // continue trend down
    {
        waves[w_i]->trend = WaveTrendType::DOWN;
        waves[w_i]->trend_index = ++trend_index;
        last_trend_down_wave_low = KRef(k_datas, waves[w_i]->end, KAttributeType::LOW);
         
        if( highest_rel_w_i > -1 )
        {
            last_trend_down_wave_high = KRef(k_datas, waves[highest_rel_w_i]->end, KAttributeType::HIGH);
            int j = 1;
            for( int i = highest_rel_w_i + 1; i <= w_i; ++i, ++j )
            {
                waves[i]->trend = WaveTrendType::DOWN;
                waves[i]->trend_index = trend_index;
                waves[i]->trend_sub_index = j;
            }
        }else
            last_trend_down_wave_high = KRef(k_datas, waves[w_i]->beg, KAttributeType::HIGH);

        return TraversSetTrendDownData(k_datas, waves, w_i, last_trend_down_wave_high, last_trend_down_wave_low, trend_index);
    }else
        return std::make_tuple(TOUCH_WAVES_END, -1, -1);
}

int TrendUpDoAjustDown(T_HisDataItemContainer &k_datas, std::deque<std::shared_ptr<Wave> > &waves, INOUT int &w_i
                       , double trend_up_wave_high, double trend_up_wave_low, INOUT int &cur_ajust_index, INOUT int &lowest_rel_w_i)
{ 
    if( ++w_i >= waves.size() )
        return TOUCH_WAVES_END;
    if( waves[w_i]->type == WaveType::DOWN )
    {
        //assert(waves[w_i]->type == WaveType::DOWN); 
        double wave_i_low = KRef(k_datas, waves[w_i]->end, KAttributeType::LOW);
        double wave_i_high = KRef(k_datas, waves[w_i]->beg, KAttributeType::HIGH);
        double wave_i_close = KRef(k_datas, waves[w_i]->end, KAttributeType::CLOSE);
        if( wave_i_low < trend_up_wave_low - HL_BREAK_THREHOLD 
            || (wave_i_low < trend_up_wave_low && wave_i_close < trend_up_wave_low + EPSINON) )
        {
            waves[w_i]->trend = WaveTrendType::DOWN;
            waves[w_i]->trend_index = 1;
            return OCCURE_TREND_DOWN;
        }else 
        {
            if( lowest_rel_w_i < 0 || wave_i_low < KRef(k_datas, waves[lowest_rel_w_i]->end, KAttributeType::LOW) )
                lowest_rel_w_i = w_i;
            waves[w_i]->trend = WaveTrendType::AJUST_TO_DOWN;
            waves[w_i]->trend_index = ++cur_ajust_index;
            return TrendUpDoAjustDown(k_datas, waves, w_i, trend_up_wave_high, trend_up_wave_low, cur_ajust_index, lowest_rel_w_i);
        } 
    }else
    {  
        assert(waves[w_i]->type == WaveType::UP);
        double wave_i_low =  KRef(k_datas, waves[w_i]->beg, KAttributeType::LOW);
        double wave_i_high = KRef(k_datas, waves[w_i]->end, KAttributeType::HIGH);
        double wave_i_close = KRef(k_datas, waves[w_i]->end, KAttributeType::CLOSE);
        if( wave_i_high > trend_up_wave_high + HL_BREAK_THREHOLD 
            || (wave_i_high > trend_up_wave_high && wave_i_close > trend_up_wave_high - EPSINON) )
        {
            waves[w_i]->trend = WaveTrendType::UP;
            //waves[w_i]->trend_index = 1;
            return OCCURE_TREND_UP;
        }else 
        {
            waves[w_i]->trend = WaveTrendType::AJUST_TO_DOWN;
            waves[w_i]->trend_index = ++cur_ajust_index;
            return TrendUpDoAjustDown(k_datas, waves, w_i, trend_up_wave_high, trend_up_wave_low, cur_ajust_index, lowest_rel_w_i);
        } 
    }
}

// return <ret_value, trend_end_index, lowest_rel_w_i>
std::tuple<int, int, int> TraversSetTrendUpData(T_HisDataItemContainer &k_datas, std::deque<std::shared_ptr<Wave> > &waves, INOUT int &w_i
                            , double trend_up_wave_high, double trend_up_wave_low, INOUT int &trend_index)
{ 
    assert(waves[w_i]->type == WaveType::UP);
    double last_trend_up_wave_high = trend_up_wave_high;
    double last_trend_up_wave_low = trend_up_wave_low;

    const int old_w_i = w_i;
    int cur_ajust_index = 0;
    int lowest_rel_w_i = -1;
    int ret = TrendUpDoAjustDown(k_datas, waves, w_i, last_trend_up_wave_high, last_trend_up_wave_low, cur_ajust_index, lowest_rel_w_i);
    if( ret == OCCURE_TREND_DOWN ) // trend down
    {
        return std::make_tuple(OCCURE_TREND_DOWN, old_w_i, lowest_rel_w_i);
    }else if( ret == OCCURE_TREND_UP ) // trend up
    {
        waves[w_i]->trend = WaveTrendType::UP;
        waves[w_i]->trend_index = ++trend_index;
        last_trend_up_wave_high = KRef(k_datas, waves[w_i]->end, KAttributeType::HIGH);
        if( lowest_rel_w_i > -1 )
        {
            last_trend_up_wave_low = KRef(k_datas, waves[lowest_rel_w_i]->end, KAttributeType::LOW);
            /*for( int i = w_i - 1; i > lowest_rel_w_i; --i )
            {
                waves[w_i]->sub_waves.insert(waves[w_i]->sub_waves.begin(), waves[i]->sub_waves.begin(), waves[i]->sub_waves.end());
            }*/
            int j = 1;
            for( int i = lowest_rel_w_i + 1; i <= w_i; ++i, ++j )
            {
                waves[i]->trend = WaveTrendType::UP;
                waves[i]->trend_index = trend_index;
                waves[i]->trend_sub_index = j;
            }
        }else
            last_trend_up_wave_low = KRef(k_datas, waves[w_i]->beg, KAttributeType::LOW);
        return TraversSetTrendUpData(k_datas, waves, w_i, last_trend_up_wave_high, last_trend_up_wave_low, trend_index);
    }else
        return std::make_tuple(TOUCH_WAVES_END, -1, -1);
}

void WaveMan::TraverseSetTrendDataTowardRight(const std::string &code, TypePeriod type_period, unsigned int start_index)
{
    T_HisDataItemContainer & k_datas = app_.stock_data_man().GetHisDataContainer(type_period, code);
    auto & waves = GetWaveContainer(code, type_period);
    if( waves.empty() )
        return;
    // find start_index's related wave index ----------
    int wave_index = 0;
    int i = 0;
    for( ; i < waves.size(); ++i )
    {
        if( start_index < waves[i]->beg )
            break;
    }
    if( i == waves.size() )
    {
        if( start_index > waves[i-1]->end )
            return; 
        wave_index = i - 1;
    }else
        wave_index = i - 1 < 0 ? 0 : i - 1;
    //----------------------------------------------------
    const double rebounce_threhold = 1/2; // 2/3
    double last_trend_up_wave_low = MAX_PRICE;
    double last_trend_up_wave_high = MIN_PRICE;
    double last_trend_down_wave_high = MIN_PRICE;
    double last_trend_down_wave_low = MAX_PRICE;
            
    while( wave_index > 0 && waves[wave_index]->trend != WaveTrendType::DOWN && waves[wave_index]->trend != WaveTrendType::UP )
        --wave_index;
    if( wave_index > 0 )
    {
        WaveTrendType temp_type = waves[wave_index]->trend;
        while( wave_index > 0 && waves[wave_index]->trend == temp_type && waves[wave_index]->trend_sub_index > 1 )
            --wave_index;
    }
    //if( wave_index == 0 )
    {
        std::tuple<int, int, int> ret;
        if( waves[wave_index]->type == WaveType::UP )
        { 
            last_trend_up_wave_low = KRef(k_datas, waves[wave_index]->beg, KAttributeType::LOW);
            last_trend_up_wave_high = KRef(k_datas, waves[wave_index]->end, KAttributeType::HIGH);
            int trend_index = 1;
            ret = TraversSetTrendUpData(k_datas, waves, wave_index, last_trend_up_wave_high, last_trend_up_wave_low, trend_index);
        }else // waves[wave_index]->type == WaveType::DOWN
        {
            last_trend_down_wave_high = KRef(k_datas, waves[wave_index]->beg, KAttributeType::HIGH);
            last_trend_down_wave_low = KRef(k_datas, waves[wave_index]->end, KAttributeType::LOW);
            int trend_index = 1;
            ret = TraversSetTrendDownData(k_datas, waves, wave_index, last_trend_down_wave_high, last_trend_down_wave_low, trend_index);
        }
        while( std::get<0>(ret) != TOUCH_WAVES_END )
        {
            if( std::get<0>(ret) == OCCURE_TREND_DOWN )
            {
                const int last_trend_up_wave_index = std::get<1>(ret);
                assert(waves[wave_index]->type == WaveType::DOWN);
                last_trend_up_wave_low = KRef(k_datas, waves[last_trend_up_wave_index]->beg, KAttributeType::LOW); 
                last_trend_up_wave_high = KRef(k_datas, waves[last_trend_up_wave_index]->end, KAttributeType::HIGH); 
                    
                bool has_trend_index_generated = false;
                if( std::get<2>(ret) > -1 )
                {
                    double rebouce_down = KRef(k_datas, waves[std::get<2>(ret)]->end, KAttributeType::LOW);
                    if( (rebouce_down - last_trend_up_wave_high) / (last_trend_up_wave_low - last_trend_up_wave_high) > rebounce_threhold )
                    { 
                        int j = 1;
                        for( int i = last_trend_up_wave_index + 1; i <= std::get<2>(ret); ++i, ++j )
                        {
                            waves[i]->trend = WaveTrendType::DOWN;
                            waves[i]->trend_index = 1;
                            waves[i]->trend_sub_index = j;
                            has_trend_index_generated = true;
                        }
                    }
                }
                double cur_trend_down_wave_high = last_trend_up_wave_high;
                double cur_trend_down_wave_low = KRef(k_datas, waves[wave_index]->end, KAttributeType::LOW); 
                int trend_index = has_trend_index_generated ? 2 : 1;
                ret = TraversSetTrendDownData(k_datas, waves, wave_index, cur_trend_down_wave_high, cur_trend_down_wave_low, trend_index);
                         
            }else if( std::get<0>(ret) == OCCURE_TREND_UP )
            {
                const int last_trend_down_wave_index = std::get<1>(ret);
                assert(waves[last_trend_down_wave_index]->type == WaveType::DOWN);
                last_trend_down_wave_high = KRef(k_datas, waves[last_trend_down_wave_index]->beg, KAttributeType::HIGH);
                last_trend_down_wave_low = KRef(k_datas, waves[last_trend_down_wave_index]->end, KAttributeType::LOW);
                bool has_trend_index_generated = false;
                if( std::get<2>(ret) > -1 )
                {
                    double rebouce_up = KRef(k_datas, waves[std::get<2>(ret)]->end, KAttributeType::HIGH);
                    if( (rebouce_up - last_trend_down_wave_low) / (last_trend_down_wave_high - last_trend_down_wave_low) > rebounce_threhold )
                    { 
                        int j = 1;
                        for( int i = last_trend_down_wave_index + 1; i <= std::get<2>(ret); ++i, ++j )
                        {
                            waves[i]->trend = WaveTrendType::UP;
                            waves[i]->trend_index = 1;
                            waves[i]->trend_sub_index = j;
                            has_trend_index_generated = true;
                        }
                    }
                }
                double cur_trend_up_wave_low = last_trend_down_wave_low;
                double cur_trend_up_wave_high = KRef(k_datas, waves[wave_index]->end, KAttributeType::HIGH);
                int trend_index = has_trend_index_generated ? 2 : 1;
                ret = TraversSetTrendUpData(k_datas, waves, wave_index, cur_trend_up_wave_high, cur_trend_up_wave_low, trend_index);
            }
        }// while
        
    }//if( wave_index == 0 )
            
    //------------------------------

}

double WaveMan::MeasureHight(const std::string &code, TypePeriod type_period, const Wave &wave) const
{
    T_HisDataItemContainer & k_datas = app_.stock_data_man().GetHisDataContainer(type_period, code);

    double h = 0.0;
    double l = 0.0;
    if( wave.type == WaveType::UP )
    {
        h = KRef(k_datas, wave.end, KAttributeType::HIGH);
        l = KRef(k_datas, wave.beg, KAttributeType::LOW);
    }else
    {
        h = KRef(k_datas, wave.beg, KAttributeType::HIGH);
        l = KRef(k_datas, wave.end, KAttributeType::LOW);
    }
    return h-l;
}

void WaveMan::MakeSubWave(const std::string &code, TypePeriod type_period, INOUT Wave &wave, unsigned int sub_level)
{
    T_HisDataItemContainer & k_datas = app_.stock_data_man().GetHisDataContainer(type_period, code);
    std::deque<std::shared_ptr<Wave> > sub_waves;
    if( wave.type == WaveType::UP )
    { 
        int start = wave.end;
        int index = find_next_btm_fractal(k_datas, start);
        if( index == -1 || index == wave.beg )
            return;
        auto sub_wave = std::make_shared<Wave>(sub_level, WaveType::UP, index, start);
        sub_waves.push_front(sub_wave);

        start = index == start ? index - 1 : index;
        int i = 0;
        while( start >= wave.beg )
        {
            if( i % 2 == 0 )
            {
                int index = find_next_top_fractal(k_datas, start);
                if( index == -1 || index < wave.beg )
                {
                    auto sub_wave = std::make_shared<Wave>(sub_level, WaveType::DOWN, wave.beg, sub_waves.front()->beg);
                    sub_waves.push_front(sub_wave);
                    break;
                }else
                {
                    auto sub_wave = std::make_shared<Wave>(sub_level, WaveType::DOWN, index, sub_waves.front()->beg);
                    sub_waves.push_front(sub_wave);
                    start = index == start ? index - 1 : index;
                }
            }else
            {
                int index = find_next_btm_fractal(k_datas, start);
                if( index == -1 || index <= wave.beg )
                {
                    auto sub_wave = std::make_shared<Wave>(sub_level, WaveType::UP, wave.beg, sub_waves.front()->beg);
                    sub_waves.push_front(sub_wave);
                    break;
                }else
                {
                    auto sub_wave = std::make_shared<Wave>(sub_level, WaveType::UP, index, sub_waves.front()->beg);
                    sub_waves.push_front(sub_wave);
                    start = index == start ? index - 1 : index;
                }
            }
            ++i; 
        }// while  
        wave.sub_waves = sub_waves;
    }else // down
    {
        int start = wave.end;
        int index = find_next_top_fractal(k_datas, start);
        if( index == -1 || index == wave.beg )
            return;
        auto sub_wave = std::make_shared<Wave>(sub_level, WaveType::DOWN, index, start);
        sub_waves.push_front(sub_wave);

        start = index == start ? index - 1 : index;
        int i = 0;
        while( start >= wave.beg )
        {
            if( i % 2 == 0 )
            {
                int index = find_next_btm_fractal(k_datas, start);
                if( index == -1 || index < wave.beg )
                {
                    auto sub_wave = std::make_shared<Wave>(sub_level, WaveType::UP, wave.beg, sub_waves.front()->beg);
                    sub_waves.push_front(sub_wave);
                    break;
                }else
                {
                    auto sub_wave = std::make_shared<Wave>(sub_level, WaveType::UP, index, sub_waves.front()->beg);
                    sub_waves.push_front(sub_wave);
                    start = index == start ? index - 1 : index;
                }
            }else
            {
                int index = find_next_top_fractal(k_datas, start);
                if( index == -1 || index <= wave.beg )
                {
                    auto sub_wave = std::make_shared<Wave>(sub_level, WaveType::DOWN, wave.beg, sub_waves.front()->beg);
                    sub_waves.push_front(sub_wave);
                    break;
                }else
                {
                    auto sub_wave = std::make_shared<Wave>(sub_level, WaveType::DOWN, index, sub_waves.front()->beg);
                    sub_waves.push_front(sub_wave);
                    start = index == start ? index - 1 : index;
                }
            }
            ++i; 
        }// while  
        wave.sub_waves = sub_waves;
    }
}

//void find_down_towardleft_end(std::deque<std::shared_ptr<T_KlineDataItem> > &kline_data_items, bool is_src_k_same, double lowest_price, int start, int &end) 
void FindDownTowardLeftEnd(std::deque<std::shared_ptr<T_KlineDataItem> > &kline_data_items, bool is_src_k_same, double lowest_price, int start, OUT int &end) 
{
    if( start == 0 )
        return;
    // start is top fractal. A
    int a_date = kline_data_items[start]->stk_item.date;
    double price_a = kline_data_items[start]->stk_item.high_price;
    // 1.find next btm fractal B 
    int btm_index_b = -1;
    if( !is_src_k_same && IsBtmFractal(kline_data_items[start]->type) ) // start is both top and btm fractal line 
        btm_index_b = start;
    else
        btm_index_b = find_next_btm_fractal(kline_data_items, start - 1);
    if( btm_index_b == -1 )
        return; 
    const double price_b = kline_data_items[btm_index_b]->stk_item.low_price;
    //assert( price_b < price_a );
    // find next top fractal C
    int top_index_c = -1;
    if( btm_index_b != start && IsTopFractal(kline_data_items[btm_index_b]->type) ) // start is both top and btm fractal line 
        top_index_c = btm_index_b;
    else
        top_index_c = find_next_top_fractal(kline_data_items, btm_index_b - 1);

    if( top_index_c == -1 )
    {
        end = btm_index_b;
        return;
    }

    //assert( IsTopFractal(kline_data_items[top_index_c]->type) ); 
    double price_c = kline_data_items[top_index_c]->stk_item.high_price;
    ///assert( !(price_c < price_b ) );  //occure cash

    if( price_c < price_a )// judge if C is lower than A
    { 
        if( !(price_b > lowest_price) ) // price b <= lowest_price 
        {
            lowest_price = price_b;
            end = btm_index_b;
            FindDownTowardLeftEnd(kline_data_items, btm_index_b == top_index_c, lowest_price, top_index_c, end);
        }
        else  // price_b > lowest_price
            return;
    }else // higher than A: set B as end point
    {
        if( !(price_b > lowest_price) ) // price_b <= lowest_price
        {
            end = btm_index_b;
        }
        return;
    }
} 

//void find_up_towardleft_end(std::deque<std::shared_ptr<T_KlineDataItem> > &kline_data_items, bool is_src_k_same, double highest_price, int start, int &end) 
void FindUpTowardLeftEnd(std::deque<std::shared_ptr<T_KlineDataItem> > &kline_data_items, bool is_src_k_same, double highest_price, int start, int &end) 
{
    if( start == 0 )
        return;
    // start is top fractal. A
    int a_date = kline_data_items[start]->stk_item.date;
    double price_a = kline_data_items[start]->stk_item.low_price;
    // 1.find next top fractal B 
    int top_index_b = -1;
    if( !is_src_k_same && IsTopFractal(kline_data_items[start]->type) ) // start is both top and btm fractal line 
        top_index_b = start;
    else
        top_index_b = find_next_top_fractal(kline_data_items, start - 1);
    if( top_index_b == -1 )
        return; 

    const double price_b = kline_data_items[top_index_b]->stk_item.high_price;

    // find next btm fractal C
    int btm_index_c = -1;
    if( top_index_b != start && IsBtmFractal(kline_data_items[top_index_b]->type) ) // start is both top and btm fractal line 
        btm_index_c = top_index_b;
    else
        btm_index_c = find_next_btm_fractal(kline_data_items, top_index_b - 1);

    if( btm_index_c == -1 )
    {
        end = top_index_b;
        return;
    }

    double price_c = kline_data_items[btm_index_c]->stk_item.low_price;
    ///assert( !(price_c > price_b ) );//occure cash

    if( price_c > price_a )// judge if C is higher than A
    { 
        if( !(price_b < highest_price) ) // price b >= highest_price 
        {
            highest_price = price_b;
            end = top_index_b;
            FindUpTowardLeftEnd(kline_data_items, top_index_b == btm_index_c, highest_price, btm_index_c, end);
        }
        else  // price_b < highest_price
            return;
    }else // C is lower than A: set B as end point
    {
        if( !(price_b < highest_price) ) // price_b >= highest_price
        {
            end = top_index_b;
        }
        return;
    }
} 
