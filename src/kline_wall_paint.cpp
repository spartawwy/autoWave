
#include "kline_wall.h"

#include <cassert>
#include <tuple>
#include <array>
#include <QPainter>
#include <qevent.h> 
#include <qdebug.h>
#include <qdatetime.h>
#include <QtWidgets/QMessageBox>
#include <QMenu>
#include <QAction>

#include <TLib/core/tsystem_utility_functions.h>

#include "config_man.h"
#include "mainwindow.h"
#include "tool_bar.h"
#include "stkfo_common.h"
#include "futures_forecast_app.h"
#include "exchange_calendar.h"
#include "strategy_man.h"  
#include "wave.h"

static const int cst_default_year = 2017;
static const Qt::CursorShape cst_cur_del_forcst_line = Qt::ClosedHandCursor;
const double KLineWall::cst_k_mm_enlarge_times = 1.02; 
const double KLineWall::cst_k_mm_narrow_times = 0.98; 

using namespace TSystem;
KLineWall::KLineWall(FuturesForecastApp *app, QWidget *parent, int index, TypePeriod k_type) 
    : QWidget(parent) 
    , app_(app)
    , main_win_((MainWindow*)parent)
    , wall_index_(index)
    , head_h_percent_(0.03)
    , bottom_h_percent_(0.04)
    , empty_right_w_(30)
    , right_w_(30)
    , pre_mm_w_(-1)
    , pre_mm_h_(-1)
    , h_axis_trans_in_paint_k_(0)
    , stock_code_()
    , is_index_(false)
    , p_hisdata_container_(nullptr)
    , lowestMinPrice_(99.9)
    , highestMaxPrice_(0.0)
    , lowest_price_date_(0)
    , lowest_price_hhmm_(0)
    , highest_price_date_(0)
    , highest_price_hhmm_(0)
    , show_cross_line_(false)
    , k_num_(WOKRPLACE_DEFUALT_K_NUM)
    , k_rend_index_(0)  
    , pre_k_rend_index_(0)
    , k_rend_index_for_train_(0)
    , k_cur_train_date_(0)
    , k_cur_train_hhmm_(0)
    , k_type_(k_type)
    , nmarket_(MARKET_SH_FUTURES)
    , k_cycle_tag_()
    , k_cycle_year_(0)
    , date_(0)
    , k_date_time_str_() 
    , is_resetting_stock_(false)
    , draw_action_(DrawAction::NO_ACTION)
    , mm_move_flag_(false)
    , move_start_point_(0, 0)
    , area_select_flag_(false)
    , forcast_man_(app, index)
    , auto_forcast_man_(app, index)
    , cur_select_forcast_(nullptr)
    , is_draw_fengxin_(false)
    , is_draw_bi_(false)
    , is_draw_struct_line_(false)
    , is_draw_section_(false)
    , is_draw_wave_(true)
    , right_clicked_k_date_(0)
    , right_clicked_k_hhmm_(0)
    , train_step_type_(TypePeriod::PERIOD_1M)
    , train_start_date_(0)
{
    ui.setupUi(this);
    ResetDrawState(DrawAction::NO_ACTION); 
 
    // init ui -----------
    QPalette pal = this->palette();
    pal.setColor(QPalette::Background, Qt::black);
    this->setAutoFillBackground(true);
    this->setPalette(pal);
    setMouseTracking(true);
    
    zb_windows_.push_back( std::move(std::make_shared<MomentumZhibiaoWin>(this)) );
    zb_windows_.push_back( std::move(std::make_shared<VolZhibiaoWin>(this)) );
}

bool KLineWall::Init()
{
    k_cycle_tag_ = DEFAULT_CYCLE_TAG;
    k_cycle_year_ = cst_default_year;
      
    k_wall_menu_ = new QMenu(this);

    auto action_pop_statistic_dlg = new QAction(this);
    action_pop_statistic_dlg->setText(QStringLiteral("区间统计"));
    bool ret = QObject::connect(action_pop_statistic_dlg, SIGNAL(triggered(bool)), this, SLOT(slotOpenStatisticDlg(bool)));
    k_wall_menu_->addAction(action_pop_statistic_dlg);
    
    auto action_zoomin_select = new QAction(this);
    action_zoomin_select->setText(QStringLiteral("区域放大"));
    ret = QObject::connect(action_zoomin_select, SIGNAL(triggered(bool)), this, SLOT(slotZoominSelect(bool)));
    k_wall_menu_->addAction(action_zoomin_select);

    k_wall_menu_sub_ = new QMenu(this);
    auto action_pop_related_kwall = new QAction(this);
    action_pop_related_kwall->setText(QStringLiteral("联动时段"));
    ret = QObject::connect(action_pop_related_kwall, SIGNAL(triggered(bool)), this, SLOT(slotOpenRelatedSubKwall(bool)));
    k_wall_menu_sub_->addAction(action_pop_related_kwall);

    ret = QObject::connect(this, SIGNAL(sigUpdateKwall()), this, SLOT(slotUpdateKwall()));

    ave_lines_.push_back(cst_ave_line_0_t);
    ave_lines_.push_back(cst_ave_line_1_t);
    ave_lines_pos_.insert(std::make_pair(cst_ave_line_0_t, std::unordered_map<unsigned int, double>()));
    ave_lines_pos_.insert(std::make_pair(cst_ave_line_1_t, std::unordered_map<unsigned int, double>()));
    app_->stock_data_man().AddAveLineT(cst_ave_line_0_t);
    app_->stock_data_man().AddAveLineT(cst_ave_line_1_t);
    //return ResetStock(DEFAULT_CODE, k_type_, false, MARKET_SH_FUTURES); // 600196  000301

#if 0 
    int start_index = 100 + 35 + 165 + 28;
    int len = 300;
    return LoadBaseStock(DEFAULT_CODE, k_type_, false, MARKET_SH_FUTURES, start_index, len);
#elif 0
    int eldest_date = 20180911;
    int eldest_hhmm = 901; 
    int latest_date = 20180912;  //20180913; //20180918; //20180921;
    int latest_hhmm = 200;      //1115;     //915;  //2225;   //200; //120; //ps: consider different period type's start hhmm
    return LoadBaseStock(DEFAULT_CODE, k_type_, false, MARKET_SH_FUTURES
                        , std::make_tuple(eldest_date, eldest_hhmm, latest_date, latest_hhmm));
#else

    auto config_info = app_->config_man().contract_info();
    int eldest_date = config_info.eldest_date; //20201001;
    int eldest_hhmm = config_info.eldest_hhmm; // 0901
    // 20190218 loss
    int latest_date = config_info.latest_date; //20190225; //20190130; //20190103; // order 20190118; //20190117; //20201030;  //20180913; //20180918; //20180921;
    int latest_hhmm = config_info.latest_hhmm; //910;      //1115;     //915;  //2225;   //200; //120; //ps: consider different period type's start hhmm
    
    ret = LoadBaseStock(config_info.code.c_str(), k_type_, false, MARKET_SH_FUTURES
                , std::make_tuple(eldest_date, eldest_hhmm, latest_date, latest_hhmm));
    if( !ret ) 
        QMessageBox::information(nullptr, QString::fromLocal8Bit("提示"), QString::fromLocal8Bit("database no related k date!"));
    return ret;
#endif
}
 

void KLineWall::Draw2pDownForcast(QPainter &painter, const int mm_h, double item_w, ForcastMan &forcast_man)
{
    auto p_data_vector = forcast_man.Find2pForcastVector(stock_code_, k_type_, true);

    if( p_data_vector && !p_data_vector->empty() )
    { 
        QPen pen;  
        //pen.setStyle(Qt::DotLine);
        pen.setColor(Qt::magenta);
        pen.setWidth(2);
       
        const auto font_size = painter.font().pointSizeF();
        std::vector<T_TuplePointStr>  fronts_to_draw;
        
        //for( unsigned int i = 0; i < p_data_vector->size(); ++i )
        for( auto iter = p_data_vector->begin(); iter != p_data_vector->end(); ++iter )
        {
            T_Data2pForcast &data_2pforcastdown = *iter;
            auto item_a = GetKLineDataItemByDate(data_2pforcastdown.date_a, data_2pforcastdown.hhmm_a);
            auto item_b = GetKLineDataItemByDate(data_2pforcastdown.date_b, data_2pforcastdown.hhmm_b);
            if( item_a && item_a->kline_posdata(wall_index_).date != 0 && item_b && item_b->kline_posdata(wall_index_).date != 0 )
            {
                //if( abs(item_a->kline_posdata(wall_index_).top.y()) > abs(item_b->kline_posdata(wall_index_).top.y()) )  // y is negative
                //{  
                    pen.setStyle(Qt::SolidLine); 
                    painter.setPen(pen);  
                    data_2pforcastdown.point_a = item_a->kline_posdata(wall_index_).top;
                    data_2pforcastdown.point_b = item_b->kline_posdata(wall_index_).bottom;

                    painter.drawLine(data_2pforcastdown.point_a, data_2pforcastdown.point_b);
fronts_to_draw.push_back(std::make_tuple(QPointF(data_2pforcastdown.point_a.x()-item_w/2, data_2pforcastdown.point_a.y()), "A"));
fronts_to_draw.push_back(std::make_tuple(QPointF(data_2pforcastdown.point_b.x()-item_w/2, data_2pforcastdown.point_b.y() + painter.font().pointSizeF()), "B"));
                    double y1 = get_price_y(data_2pforcastdown.c1, mm_h);
                    double y2 = get_price_y(data_2pforcastdown.c2, mm_h);
                    double y3 = get_price_y(data_2pforcastdown.c3, mm_h);
                    double x_b = item_b->kline_posdata(wall_index_).bottom.x();
                    pen.setStyle(Qt::DotLine); 
                    painter.setPen(pen);  
                    // vertical line ----
                    painter.drawLine(item_b->kline_posdata(wall_index_).bottom, QPointF(x_b, y1));
                    painter.drawLine(QPointF(x_b, y1), QPointF(x_b, y2));
                    painter.drawLine(QPointF(x_b, y2), QPointF(x_b, y3));
                    // horzon forcast line -----------
                    double h_line_left = item_b->kline_posdata(wall_index_).bottom.x() - item_w;
                    painter.drawLine(QPointF(h_line_left, y1), QPointF(x_b + 5*item_w, y1));
fronts_to_draw.push_back(std::make_tuple(QPointF(h_line_left - font_size*6, y1), utility::FormatStr("C1: %.2f", data_2pforcastdown.c1))); 

                    painter.drawLine(QPointF(h_line_left, y2), QPointF(x_b + 10*item_w, y2));
fronts_to_draw.push_back(std::make_tuple(QPointF(h_line_left - font_size*6, y2), utility::FormatStr("C2: %.2f", data_2pforcastdown.c2))); 

                    painter.drawLine(QPointF(h_line_left, y3), QPointF(x_b + 20*item_w, y3));
fronts_to_draw.push_back(std::make_tuple(QPointF(h_line_left - font_size*6, y3), utility::FormatStr("C3: %.2f", data_2pforcastdown.c3)));            
                //}else 
                //{
                //    painter.drawLine(item_a->kline_posdata(wall_index_).bottom, item_b->kline_posdata(wall_index_).top);

                //}
            }
        }  

        pen.setColor(Qt::white);
        pen.setStyle(Qt::SolidLine); 
        painter.setPen(pen);
        //char buf[32] = {0};
        std::for_each( std::begin(fronts_to_draw), std::end(fronts_to_draw), [&painter, /*&buf, */this](T_TuplePointStr& in)
        {
            //sprintf_s(buf, sizeof(buf), "%.2f\0", std::get<1>(in));
            painter.drawText(std::get<0>(in), std::get<1>(in).c_str());
        });
  }
  // end of paint 3pdates -------------------------

}

void KLineWall::Draw2pUpForcast(QPainter &painter, const int mm_h, double item_w, ForcastMan &forcast_man)
{
    auto *p_data_vector = forcast_man.Find2pForcastVector(stock_code_, k_type_, false);
    if( !p_data_vector || p_data_vector->empty() )
        return;

    QPen pen;   
    pen.setColor(Qt::darkGreen);
    pen.setWidth(2);
    //painter.setPen(pen); 
    const auto font_size = painter.font().pointSizeF();
    std::vector<T_TuplePointStr> fronts_to_draw;
        
    //for( unsigned int i = 0; i < p_data_vector->size(); ++i )
    for( auto iter = p_data_vector->begin(); iter != p_data_vector->end(); ++iter )
    {
        T_Data2pForcast &data_2pforcast = *iter;
        auto item_a = GetKLineDataItemByDate(data_2pforcast.date_a, data_2pforcast.hhmm_a);
        auto item_b = GetKLineDataItemByDate(data_2pforcast.date_b, data_2pforcast.hhmm_b);
        if( item_a && item_a->kline_posdata(wall_index_).date != 0 && item_b && item_b->kline_posdata(wall_index_).date != 0 )
        {
            //if( abs(item_a->kline_posdata(wall_index_).top.y()) < abs(item_b->kline_posdata(wall_index_).top.y()) )  // y is negative
            //{  
                pen.setStyle(Qt::SolidLine); 
                painter.setPen(pen);  
                data_2pforcast.point_a = item_a->kline_posdata(wall_index_).bottom;
                data_2pforcast.point_b = item_b->kline_posdata(wall_index_).top;

                painter.drawLine(data_2pforcast.point_a, data_2pforcast.point_b);
 fronts_to_draw.push_back(std::make_tuple(QPointF(data_2pforcast.point_a.x()-item_w/2, data_2pforcast.point_a.y()), "A"));
 fronts_to_draw.push_back(std::make_tuple(QPointF(data_2pforcast.point_b.x()-item_w/2, data_2pforcast.point_b.y() + painter.font().pointSizeF()), "B"));
                double y1 = get_price_y(data_2pforcast.c1, mm_h);
                double y2 = get_price_y(data_2pforcast.c2, mm_h);
                double y3 = get_price_y(data_2pforcast.c3, mm_h);
                double x_b = item_b->kline_posdata(wall_index_).bottom.x();
                pen.setStyle(Qt::DotLine); 
                painter.setPen(pen);  
                // vertical line ----
                painter.drawLine(item_b->kline_posdata(wall_index_).top, QPointF(x_b, y3));
                // horzon forcast line -----------
                double h_line_left = item_b->kline_posdata(wall_index_).bottom.x() - item_w;
                painter.drawLine(QPointF(h_line_left, y1), QPointF(x_b + 5*item_w, y1));
fronts_to_draw.push_back(std::make_tuple(QPointF(h_line_left - font_size*6, y1), utility::FormatStr("C1: %.2f", data_2pforcast.c1)));

                painter.drawLine(QPointF(h_line_left, y2), QPointF(x_b + 10*item_w, y2));
fronts_to_draw.push_back(std::make_tuple(QPointF(h_line_left - font_size*6, y2), utility::FormatStr("C2: %.2f", data_2pforcast.c2)));

                painter.drawLine(QPointF(h_line_left, y3), QPointF(x_b + 20*item_w, y3));
fronts_to_draw.push_back(std::make_tuple(QPointF(h_line_left - font_size*6, y3),  utility::FormatStr("C3: %.2f", data_2pforcast.c3)));           
            //}else 
            //{
            //    painter.drawLine(item_a->kline_posdata(wall_index_).top, item_b->kline_posdata(wall_index_).bottom);
            //}
        }
    }

    pen.setColor(Qt::white);
    pen.setStyle(Qt::SolidLine); 
    painter.setPen(pen); 
    std::for_each( std::begin(fronts_to_draw), std::end(fronts_to_draw), [&painter, this](T_TuplePointStr& in)
    {
        painter.drawText(std::get<0>(in), std::get<1>(in).c_str());
    });
}

void KLineWall::Draw3pDownForcast(QPainter &painter, const int mm_h, double item_w, ForcastMan &forcast_man)
{
    _Draw3pForcast(painter, mm_h, item_w, true, forcast_man);
}

void KLineWall::Draw3pUpForcast(QPainter &painter, const int mm_h, double item_w, ForcastMan &forcast_man)
{
    _Draw3pForcast(painter, mm_h, item_w, false, forcast_man);
}

void KLineWall::_Draw3pForcast(QPainter &painter, const int mm_h, double item_w, bool is_down_forward, ForcastMan &forcast_man)
{
    Data3pForcastInnerContainer *p_data_vector = forcast_man.Find3pForcastVector(stock_code_, k_type_, is_down_forward);
    if( !p_data_vector || p_data_vector->empty() )
        return;

    QPen pen;  
    if( is_down_forward )
        pen.setColor(Qt::darkGreen);
    else
        pen.setColor(Qt::magenta);
    pen.setWidth(2);
    const auto font_size = painter.font().pointSizeF();
    std::vector<T_TuplePointStr>  fronts_to_draw;
    double lowest_price = MAX_PRICE;
    double highest_price = MIN_PRICE;
    bool has_set_price = false;
    //for( unsigned int i = 0; i < p_data_vector->size(); ++i )
    for( auto iter = p_data_vector->begin(); iter != p_data_vector->end(); ++iter )
    {
        T_Data3pForcast &data_3p_forcast = **iter;
        auto item_a = GetKLineDataItemByDate(data_3p_forcast.date_a, data_3p_forcast.hhmm_a);
        auto item_b = GetKLineDataItemByDate(data_3p_forcast.date_b, data_3p_forcast.hhmm_b);
        auto item_c = GetKLineDataItemByDate(data_3p_forcast.date_c, data_3p_forcast.hhmm_c);
        //QPointF point_a, point_b, point_c;
        double price_a = 0.0, price_b = 0.0, price_c = 0.0;
        if( item_a && item_a->kline_posdata(wall_index_).date != 0 && item_b && item_b->kline_posdata(wall_index_).date != 0 )
        {
            if( is_down_forward )
            {
#if 0
                if( !(abs(item_a->kline_posdata(wall_index_).top.y()) > abs(item_b->kline_posdata(wall_index_).bottom.y())) )  // y is negative
                    continue;
#endif
                data_3p_forcast.point_a = item_a->kline_posdata(wall_index_).top;
                data_3p_forcast.point_b = item_b->kline_posdata(wall_index_).bottom; 
                data_3p_forcast.point_c = item_c ? item_c->kline_posdata(wall_index_).top : QPointF(0.0, 0.0);
                price_a = item_a->stk_item.high_price;
                price_b = item_b->stk_item.low_price;
                price_c = item_c ? item_c->stk_item.high_price : 0.0;
            }else
            {
#if 0
                if( !(abs(item_a->kline_posdata(wall_index_).bottom.y()) < abs(item_b->kline_posdata(wall_index_).top.y())) )  // y is negative
                    continue;
#endif
                data_3p_forcast.point_a = item_a->kline_posdata(wall_index_).bottom;
                data_3p_forcast.point_b = item_b->kline_posdata(wall_index_).top; 
                data_3p_forcast.point_c = item_c ? item_c->kline_posdata(wall_index_).bottom : QPointF(0.0, 0.0);
                price_a = item_a->stk_item.low_price;
                price_b = item_b->stk_item.high_price;
                price_c = item_c ? item_c->stk_item.low_price : 0.0;
            }
            pen.setStyle(Qt::SolidLine);
            painter.setPen(pen); 
            painter.drawLine(data_3p_forcast.point_a, data_3p_forcast.point_b);
            fronts_to_draw.push_back(std::make_tuple(QPointF(data_3p_forcast.point_a.x()-item_w/2, data_3p_forcast.point_a.y()), "A"));
            if( item_c )
            {
                painter.drawLine(data_3p_forcast.point_b, data_3p_forcast.point_c);
                fronts_to_draw.push_back(std::make_tuple(QPointF(data_3p_forcast.point_b.x()-item_w/2, data_3p_forcast.point_b.y()), "B"));
                fronts_to_draw.push_back(std::make_tuple(QPointF(data_3p_forcast.point_c.x()-item_w/2, data_3p_forcast.point_c.y()), "C"));
                double y1 = get_price_y(data_3p_forcast.d1, mm_h);
                double y2 = get_price_y(data_3p_forcast.d2, mm_h);
                double y3 = get_price_y(data_3p_forcast.d3, mm_h);
                double x_c = data_3p_forcast.point_c.x();
                pen.setStyle(Qt::DotLine);
                painter.setPen(pen); 
                // vertical line ----
                painter.drawLine(data_3p_forcast.point_c, QPointF(x_c, y3));
                // horzon forcast line -----------
                double h_line_left = data_3p_forcast.point_c.x() - item_w;
                painter.drawLine(QPointF(h_line_left, y1), QPointF(x_c + 5*item_w, y1));
                fronts_to_draw.push_back(std::make_tuple(QPointF(h_line_left - font_size*6, y1), utility::FormatStr("D1: %.2f", data_3p_forcast.d1)));

                painter.drawLine(QPointF(h_line_left, y2), QPointF(x_c + 10*item_w, y2));
                fronts_to_draw.push_back(std::make_tuple(QPointF(h_line_left - font_size*6, y2), utility::FormatStr("D2: %.2f", data_3p_forcast.d2)));

                painter.drawLine(QPointF(h_line_left, y3), QPointF(x_c + 20*item_w, y3));
                fronts_to_draw.push_back(std::make_tuple(QPointF(h_line_left - font_size*6, y3), utility::FormatStr("D3: %.2f", data_3p_forcast.d3)));

                lowest_price = std::min(lowest_price, data_3p_forcast.d1);
                lowest_price = std::min(lowest_price, data_3p_forcast.d3);
                highest_price = std::max(highest_price, data_3p_forcast.d1);
                highest_price = std::max(highest_price, data_3p_forcast.d3);
                has_set_price = true;
            }
        }

    }// for
    pen.setColor(Qt::white);
    pen.setStyle(Qt::SolidLine); 
    painter.setPen(pen); 
    std::for_each( std::begin(fronts_to_draw), std::end(fronts_to_draw), [&painter, this](T_TuplePointStr& in)
    { 
        painter.drawText(std::get<0>(in), std::get<1>(in).c_str());
    });
}

void KLineWall::DrawBi(QPainter &painter, const int /*mm_h*/)
{
    T_BiContainer &container = app_->stock_data_man().GetBiContainer(PeriodType(k_type_), stock_code_);
    if( container.empty() )
        return;

    QPen pen;  
    //pen.setStyle(Qt::DotLine);
    pen.setColor(Qt::yellow);
    pen.setWidth(2);
    for( unsigned int i = 0; i < container.size(); ++i )
    { 
        auto item_start = GetKLineDataItemByDate(container[i]->start.date, container[i]->start.hhmm);
        auto item_end = GetKLineDataItemByDate(container[i]->end.date, container[i]->end.hhmm);
        if( item_start && item_start->kline_posdata(wall_index_).date != 0 
            && item_end && item_end->kline_posdata(wall_index_).date != 0 )
        {
            if( container[i]->type == BiType::UP )
                painter.drawLine(item_start->kline_posdata(wall_index_).bottom, item_end->kline_posdata(wall_index_).top);
            else
                painter.drawLine(item_start->kline_posdata(wall_index_).top, item_end->kline_posdata(wall_index_).bottom);
        }
    }
}

void KLineWall::DrawStructLine(QPainter &painter, const int /*mm_h*/)
{
    T_StructLineContainer &container = app_->stock_data_man().GetStructLineContainer(PeriodType(k_type_), stock_code_);
    if( container.empty() )
        return;
    QPen old_pen = painter.pen();
    QPen pen;  
    //pen.setStyle(Qt::DotLine);
    pen.setColor(Qt::darkCyan);
    pen.setWidth(2);
    painter.setPen(pen);
    
    T_HisDataItemContainer & his_data = *p_hisdata_container_;
    
    const int end_index = his_data.size() - k_rend_index_ - 1;
    if( end_index < 0 )
        return;
    int start_index = his_data.size() - k_rend_index_  - k_num_;
    if( start_index < 0 )
        start_index = 0;
    for( unsigned int i = 0; i < container.size(); ++i )
    { 
        if( container[i]->beg_index < start_index || container[i]->end_index > end_index )
            continue;
        if( container[i]->type == LineType::UP )
        {
            painter.drawLine(his_data[container[i]->beg_index]->kline_posdata(wall_index_).bottom
                            , his_data[container[i]->end_index]->kline_posdata(wall_index_).top);
        }else
        {
            painter.drawLine(his_data[container[i]->beg_index]->kline_posdata(wall_index_).top
                , his_data[container[i]->end_index]->kline_posdata(wall_index_).bottom);
        }
    }
    painter.setPen(old_pen);
}


void KLineWall::DrawWave(QPainter &painter, const int /*mm_h*/)
{
    //T_StructLineContainer &container = app_->stock_data_man().GetStructLineContainer(PeriodType(k_type_), stock_code_);
    auto & container = app_->wave_man().GetWaveContainer(stock_code_, k_type_);
    if( container.empty() )
        return;
    QPen old_pen = painter.pen();

    QPen wave_lev_0_pen;
    wave_lev_0_pen.setStyle(Qt::SolidLine);
    wave_lev_0_pen.setColor(QColor(0,0,200));
    wave_lev_0_pen.setWidth(1);
    
    static auto draw_leve0_waves = [](T_HisDataItemContainer &his_data, std::deque<std::shared_ptr<Wave> > &sub_waves, QPainter &painter, int wall_index)
    { 
        for( unsigned int i = 0; i < sub_waves.size(); ++i )
        { 
            if( sub_waves[i]->type == WaveType::UP )
            {
                painter.drawLine(his_data[sub_waves[i]->beg]->kline_posdata(wall_index).bottom
                    , his_data[sub_waves[i]->end]->kline_posdata(wall_index).top);

            }else
            {
                painter.drawLine(his_data[sub_waves[i]->beg]->kline_posdata(wall_index).top
                    , his_data[sub_waves[i]->end]->kline_posdata(wall_index).bottom);
            }
        }
    };
    QPen wave_lev_1_pen;  
    wave_lev_1_pen.setStyle(Qt::SolidLine);
    wave_lev_1_pen.setColor(Qt::darkCyan);
    wave_lev_1_pen.setWidth(1);
    painter.setPen(wave_lev_1_pen);

    T_HisDataItemContainer & his_data = *p_hisdata_container_;

    const int end_index = his_data.size() - k_rend_index_ - 1;
    if( end_index < 0 )
        return;
    int start_index = his_data.size() - k_rend_index_  - k_num_;
    if( start_index < 0 )
        start_index = 0;
    for( unsigned int i = 0; i < container.size(); ++i )
    { 
        if( container[i]->beg < start_index || container[i]->end > end_index )
            continue;
        if( container[i]->type == WaveType::UP )
        {
            painter.drawLine(his_data[container[i]->beg]->kline_posdata(wall_index_).bottom
                , his_data[container[i]->end]->kline_posdata(wall_index_).top); 
        }else
        {
            painter.drawLine(his_data[container[i]->beg]->kline_posdata(wall_index_).top
                , his_data[container[i]->end]->kline_posdata(wall_index_).bottom);
        }
        if( container[i]->sub_waves.size() > 1 )
        {
            painter.setPen(wave_lev_0_pen);
            draw_leve0_waves(his_data, container[i]->sub_waves, painter, wall_index_);
            painter.setPen(wave_lev_1_pen);
        }
    }

    QPen wave_lev_2_up_pen;  
    wave_lev_2_up_pen.setStyle(Qt::SolidLine);
    wave_lev_2_up_pen.setColor(Qt::red);
    wave_lev_2_up_pen.setWidth(1);

    QPen wave_lev_2_down_pen;  
    wave_lev_2_down_pen.setStyle(Qt::SolidLine);
    wave_lev_2_down_pen.setColor(Qt::green);
    wave_lev_2_down_pen.setWidth(1);
    //painter.setPen(wave_lev_2_pen);
    for( unsigned int i = 0; i < container.size(); ++i )
    {
        if( container[i]->beg < start_index || container[i]->end > end_index )
            continue;
        auto wave_trend_type = container[i]->trend;
        if( wave_trend_type == WaveTrendType::UP || wave_trend_type == WaveTrendType::DOWN )
        {
            const int cur_trend_index = container[i]->trend_index;
            // towards right, find max trend_sub_index----------
            unsigned int target_i = i;
            while( target_i + 1 < container.size() 
                && container[target_i + 1]->trend == wave_trend_type && container[target_i + 1]->trend_index == container[i]->trend_index )
                ++target_i;
            if( target_i == container.size() || container[target_i]->end > end_index )
                continue;
            int right_end_k_index = container[target_i]->end;
              
            // towards left, find min trend_sub_index == 1----------
            target_i = i;
            while( target_i - 1 >= 0 && container[target_i - 1]->trend == wave_trend_type && container[target_i - 1]->trend_index == container[i]->trend_index
                && container[target_i]->trend_sub_index > 1 )
                --target_i;
            if( target_i < 0 || container[target_i]->beg < start_index )
                continue;
            int left_beg_k_index = container[target_i]->beg;

            painter.setPen(wave_trend_type == WaveTrendType::UP ? wave_lev_2_up_pen : wave_lev_2_down_pen);
            painter.drawLine(wave_trend_type == WaveTrendType::UP ? his_data[right_end_k_index]->kline_posdata(wall_index_).top : his_data[right_end_k_index]->kline_posdata(wall_index_).bottom
                , wave_trend_type == WaveTrendType::UP ? his_data[left_beg_k_index]->kline_posdata(wall_index_).bottom : his_data[left_beg_k_index]->kline_posdata(wall_index_).top);
            //debug --------------
            if( left_beg_k_index == 1030 )
                left_beg_k_index = left_beg_k_index;
        }
    }

    painter.setPen(old_pen);
}

void KLineWall::DrawSection(QPainter &painter, const int mm_h)
{
    T_HisDataItemContainer & his_data = *p_hisdata_container_;
    const int start_index = his_data.size() - k_rend_index_  - k_num_;
    const int end_index = his_data.size() - k_rend_index_ - 1;
    if( start_index < 0 || end_index < 0 )
        return;

    T_SectionContainer &container = app_->stock_data_man().GetStructSectionContainer(PeriodType(k_type_), stock_code_);
    if( container.empty() )
        return;
    QPen old_pen = painter.pen();
    QBrush old_brush = painter.brush();
    QPen pen;  
    
    pen.setStyle(Qt::DashLine);
    pen.setColor(Qt::white);
    pen.setWidth(2);
    painter.setPen(pen);
    QBrush brush(Qt::transparent);  
    painter.setBrush(brush);

    for( int i = 0; i < container.size(); ++i )
    {
        if( container[i].top_left_index < start_index || container[i].btm_right_index > end_index )
            continue;
        painter.drawRect(container[i].top_left.x(), container[i].top_left.y(), container[i].btm_right.x() - container[i].top_left.x()
            , container[i].btm_right.y() - container[i].top_left.y());
    };
    painter.setPen(old_pen);
    painter.setBrush(old_brush);
}

void KLineWall::UpdatePosDatas()
{
    assert(p_hisdata_container_);
    if( p_hisdata_container_->empty() || k_num_ <= 0 )
        return;

    //std::lock_guard<std::mutex> locker(painting_mutex_);
    if( !painting_mutex_.try_lock() )
        return;
    // before update get pre item which drawing point in -------
    T_KlineDataItem *item_a = nullptr;
    T_KlineDataItem *item_b = nullptr;
    T_KlineDataItem *item_c = nullptr;
    if( drawing_line_A_ != CST_MAGIC_POINT )
        item_a = GetKLineDataItemByXpos(drawing_line_A_.x());
    if( drawing_line_B_ != CST_MAGIC_POINT )
        item_b = GetKLineDataItemByXpos(drawing_line_B_.x());
    if( drawing_line_C_ != CST_MAGIC_POINT )
        item_c = GetKLineDataItemByXpos(drawing_line_C_.x());

    // update ---------------------------------------------- 
    const int k_mm_h = Calculate_k_mm_h();
     
    const int mm_w = this->width();
    double item_w = double(mm_w - empty_right_w_ - right_w_) / double(k_num_ + 1) ;
    //  space between k is: item_w / 4;
    double k_bar_w = item_w * 3 / 4;
    // clear position data
    std::for_each( std::begin(*p_hisdata_container_), std::end(*p_hisdata_container_), [this](T_HisDataItemContainer::reference entry)
    {
        entry->kline_posdata(wall_index_).Clear();
    });

#if 1
    // update position data --------------------------------
    int j = k_num_;
    auto right_end = double(mm_w - empty_right_w_ - right_w_) - k_bar_w;

    assert( p_hisdata_container_->size() > k_rend_index_ );
     
    for( auto iter = p_hisdata_container_->rbegin() + k_rend_index_;
        iter != p_hisdata_container_->rend() && j > 0; 
        ++iter, --j)
    { 
        T_KlinePosData &pos_data = iter->get()->kline_posdata(wall_index_); 
        pos_data.date = iter->get()->stk_item.date;
        pos_data.hhmm = iter->get()->stk_item.hhmmss;
        auto openPrice = (*iter)->stk_item.open_price;
        auto closePrice = (*iter)->stk_item.close_price;
        auto minPrice = (*iter)->stk_item.low_price;
        auto maxPrice = (*iter)->stk_item.high_price;

        //pos_data.x_left = j * item_w + 1;
        pos_data.x_right = right_end - item_w * (k_num_ - j);
        pos_data.x_left = pos_data.x_right - k_bar_w;
        assert(pos_data.x_right - iter->get()->kline_posdata(wall_index_).x_right < 0.0001 );
        //auto pos_y = -1 * (openPrice - lowestMinPrice_)/(highestMaxPrice_ - lowestMinPrice_) * k_mm_h;
        auto pos_y = get_price_y(openPrice, k_mm_h);
        pos_data.height = -1 * (closePrice - openPrice)/(highestMaxPrice_ - lowestMinPrice_) * k_mm_h;
        pos_data.columnar_top_left = QPointF(pos_data.x_left, pos_y);

        pos_data.top = QPointF(pos_data.x_left + k_bar_w / 2, get_price_y(maxPrice, k_mm_h));
        pos_data.bottom = QPointF(pos_data.x_left + k_bar_w / 2, get_price_y(minPrice, k_mm_h));
        // update ave line point -------------------------
        const int data_index = p_hisdata_container_->size() - 1 - k_rend_index_ - (k_num_ - j); 
        for( auto line_iter = ave_lines_pos_.begin(); line_iter != ave_lines_pos_.end(); ++line_iter )
        {
            unsigned int line_t = line_iter->first; 
            double val = app_->stock_data_man().GetAveLineZhibiao(ToPeriodType(k_type_), stock_code_, line_t, data_index); 
            line_iter->second[data_index] = get_price_y(val, k_mm_h);
        }
        // update trades pos ---------------------------
        if( app_->strategy_man() && std::try_lock(app_->strategy_man()->account_info().trade_info_mutex) )
        {
            auto trades_iter = app_->strategy_man()->account_info().trade_info.find(data_index);
            if( trades_iter != app_->strategy_man()->account_info().trade_info.end() )
            {
                std::vector<TradeRecordKwallPosInfo> pos_infos;
                for( auto iter = trades_iter->second.cbegin(); iter != trades_iter->second.cend(); ++iter )
                { 
                    TradeRecordKwallPosInfo pos_info;
                    pos_info.record = (*iter);
                    pos_info.pos = get_price_y((*iter)->price, k_mm_h);
                    //(*iter)->trade_id;
                    pos_infos.push_back(std::move(pos_info));
                }
                trades_pos_[data_index] = pos_infos;
            }
            app_->strategy_man()->account_info().trade_info_mutex.unlock();
        }
        
        // update drawing line point ---------------------
        if( item_a && item_a->stk_item.date == iter->get()->stk_item.date && item_a->stk_item.hhmmss == iter->get()->stk_item.hhmmss)
        {
            switch(draw_action_)
            {
            case DrawAction::DRAWING_FOR_2PDOWN_C:
            case DrawAction::DRAWING_FOR_3PDOWN_D:
                drawing_line_A_ = pos_data.top;
                break;
            case DrawAction::DRAWING_FOR_2PUP_C:
            case DrawAction::DRAWING_FOR_3PUP_D:
                drawing_line_A_ = pos_data.bottom;
                break;
            }
        }else if( item_b && item_b->stk_item.date == iter->get()->stk_item.date && item_b->stk_item.hhmmss == iter->get()->stk_item.hhmmss )
        {
            switch(draw_action_)
            {
            case DrawAction::DRAWING_FOR_2PDOWN_C:
            case DrawAction::DRAWING_FOR_3PDOWN_D:
                drawing_line_B_ = pos_data.bottom;
                break;
            case DrawAction::DRAWING_FOR_2PUP_C:
            case DrawAction::DRAWING_FOR_3PUP_D:
                drawing_line_B_ = pos_data.top;
                break;
            }
        }else if( item_c && item_c->stk_item.date == iter->get()->stk_item.date && item_c->stk_item.hhmmss == iter->get()->stk_item.hhmmss )
        {
            switch(draw_action_)
            { 
            case DrawAction::DRAWING_FOR_3PDOWN_D:
                drawing_line_C_ = pos_data.top;
                break; 
            case DrawAction::DRAWING_FOR_3PUP_D:
                drawing_line_C_ = pos_data.bottom;
                break;
            }
        } 
    } // for 

    // draw section ---------------------
    T_HisDataItemContainer &his_data_items = *p_hisdata_container_;
    const int start_index = his_data_items.size() - k_rend_index_  - k_num_;
    const int end_index = his_data_items.size() - k_rend_index_ - 1;
    if( start_index < 0 || end_index < 0 )
        goto END_PROC;
    T_SectionContainer &container = app_->stock_data_man().GetStructSectionContainer(ToPeriodType(k_type_), stock_code_); 
    std::for_each(std::begin(container), std::end(container), [&, this](T_SectionContainer::reference entry)
    {
        if( entry.top_left_index < start_index || entry.btm_right_index > end_index )
            return;

        entry.top_left.setX(his_data_items[entry.top_left_index]->kline_posdata(wall_index_).x_left); 
        entry.top_left.setY(this->get_price_y(entry.top_left_price, k_mm_h));

        entry.btm_right.setX(his_data_items[entry.btm_right_index]->kline_posdata(wall_index_).x_right);
        entry.btm_right.setY(this->get_price_y(entry.btm_right_price, k_mm_h));
    });
#endif

END_PROC:
    painting_mutex_.unlock();
}

void KLineWall::mousePressEvent(QMouseEvent * event )
{ 
    static auto append_3pforcast_data = [](KLineWall *kwall, bool is_down, T_KlineDataItem &item_a, T_KlineDataItem &item_b)
    {
        auto id = kwall->app_->GenerateForecastId();
        T_Data3pForcast  data_3p(id);
        data_3p.stock_code = kwall->stock_code_;
        data_3p.is_down = is_down;
        data_3p.date_a = item_a.stk_item.date; 
        data_3p.hhmm_a = item_a.stk_item.hhmmss; 
        data_3p.date_b = item_b.stk_item.date; 
        data_3p.hhmm_b = item_b.stk_item.hhmmss; 
        kwall->forcast_man_.Append(kwall->k_type_, kwall->stock_code_, is_down, data_3p);
    };
    //qDebug() << "paintEvent QCursor::pos  x:" << QCursor::pos().x() << " y: "<< QCursor::pos().y() << "\n"; 
#ifdef STK_INPUT_KWALL
	if( stock_input_dlg_.isVisible() )
		stock_input_dlg_.hide();
#endif
    if( draw_action_ == DrawAction::NO_ACTION )
    {
        if( event->buttons() & Qt::LeftButton )
        {
            mm_move_flag_ = true;
            move_start_point_ = event->pos();
            pre_k_rend_index_ = k_rend_index_;
        }else if( (event->buttons() & Qt::RightButton) && !cur_select_forcast_ )
        {
            area_select_flag_ = true;
            move_start_point_ = event->pos();
        }
        return;
    }else if( event->buttons() & Qt::RightButton )
    {
        if( draw_action_ == DrawAction::DRAWING_FOR_3PDOWN_D || draw_action_ == DrawAction::DRAWING_FOR_3PUP_D )
        { 
           if( !(drawing_line_A_ == CST_MAGIC_POINT || drawing_line_B_ == CST_MAGIC_POINT) )
           {
               auto item_a = GetKLineDataItemByXpos(drawing_line_A_.x());
               auto item_b = GetKLineDataItemByXpos(drawing_line_B_.x());
               forcast_man_.Remove3pForcastItem(stock_code_, k_type_, draw_action_ == DrawAction::DRAWING_FOR_3PDOWN_D, *item_a, *item_b);
           }
        }        
        return ResetDrawState(draw_action_); 
    }

    if( drawing_line_A_ == CST_MAGIC_POINT )
    {
        drawing_line_A_ = QPointF( event->pos().x(), event->pos().y() - h_axis_trans_in_paint_k_);
        return;
    }

    auto item_a = GetKLineDataItemByXpos(drawing_line_A_.x());
    if( !item_a )
    { // todo: show warning msg
        return ResetDrawState(DrawAction::NO_ACTION); 
    }

    if( drawing_line_B_ == CST_MAGIC_POINT ) // if to generate point B
    {
        auto item_b = GetKLineDataItemByXpos(event->pos().x());
        if( !item_b )
            return;
        if( item_b->stk_item.date < item_a->stk_item.date
            || item_b->stk_item.date == item_a->stk_item.date && item_b->stk_item.hhmmss <= item_a->stk_item.hhmmss )
        {   // todo: show warning msg
            return;
        }
        if( draw_action_ == DrawAction::DRAWING_FOR_2PDOWN_C )
        { 
            bool is_down = true;
            if( forcast_man_.Find2pForcast(stock_code_, k_type_, is_down, *item_a, *item_b) )
                return ResetDrawState(draw_action_);  
            if( item_a->stk_item.high_price > item_b->stk_item.low_price )
            {
                auto id = app_->GenerateForecastId();
                T_Data2pForcast data_2pdown_fcst(id, is_down);
                data_2pdown_fcst.stock_code = stock_code_;
                data_2pdown_fcst.date_a = item_a->stk_item.date; 
                data_2pdown_fcst.hhmm_a = item_a->stk_item.hhmmss;
                data_2pdown_fcst.date_b = item_b->stk_item.date; 
                data_2pdown_fcst.hhmm_b = item_b->stk_item.hhmmss;

                auto c1_c2_c3 = ForcastC_ABDown(item_a->stk_item.high_price, item_b->stk_item.low_price);
                data_2pdown_fcst.c1 = std::get<0>(c1_c2_c3);
                data_2pdown_fcst.c2 = std::get<1>(c1_c2_c3);
                data_2pdown_fcst.c3 = std::get<2>(c1_c2_c3);
                forcast_man_.Append(k_type_, stock_code_, is_down, data_2pdown_fcst);
            }else
            {
                //    QMessageBox::information(nullptr, QString::fromLocal8Bit("提示"), QString::fromLocal8Bit("下降反弹预测,B点最高价不能大于等于A点最高价!")); 
            }
            return ResetDrawState(draw_action_); 

        }else if( draw_action_ == DrawAction::DRAWING_FOR_2PUP_C ) 
        {
            bool is_down = false;
            if( forcast_man_.Find2pForcast(stock_code_, k_type_, is_down, *item_a, *item_b) )
                return ResetDrawState(draw_action_);  
            if( item_a->stk_item.low_price < item_b->stk_item.high_price )
            {
                auto id = app_->GenerateForecastId();
                T_Data2pForcast data_2pup_fcst(id, is_down);
                data_2pup_fcst.stock_code = stock_code_; 
                data_2pup_fcst.date_a = item_a->stk_item.date; 
                data_2pup_fcst.hhmm_a = item_a->stk_item.hhmmss; 
                data_2pup_fcst.date_b = item_b->stk_item.date; 
                data_2pup_fcst.hhmm_b = item_b->stk_item.hhmmss; 

                auto c1_c2_c3 = ForcastC_ABUp(item_a->stk_item.low_price, item_b->stk_item.high_price);
                data_2pup_fcst.c1 = std::get<0>(c1_c2_c3);
                data_2pup_fcst.c2 = std::get<1>(c1_c2_c3);
                data_2pup_fcst.c3 = std::get<2>(c1_c2_c3);
                forcast_man_.Append(k_type_, stock_code_, is_down, data_2pup_fcst);
            }else
            {
                //    QMessageBox::information(nullptr, QString::fromLocal8Bit("提示"), QString::fromLocal8Bit("上升反弹预测,B点最高价不能小于等于A点最高价!")); 
            }
            return ResetDrawState(draw_action_);  

        }else if( draw_action_ == DrawAction::DRAWING_FOR_3PDOWN_D )
        { 
            if( forcast_man_.Find3pForcast(stock_code_, k_type_, true, *item_a, *item_b) ) // already exist
                return ResetDrawState(draw_action_);  
            if( !(item_a->stk_item.high_price < item_b->stk_item.high_price) )
            {
                drawing_line_B_ = item_b->kline_posdata(wall_index_).bottom;
                append_3pforcast_data(this, true, *item_a, *item_b);
                return;
            }else
            {
                //QMessageBox::information(nullptr, QString::fromLocal8Bit("提示"), QString::fromLocal8Bit("下降3点预测,B点最高价不能大于等于A点最高价!")); 
                return ResetDrawState(draw_action_);
            }

        }else if( draw_action_ == DrawAction::DRAWING_FOR_3PUP_D )
        {
            if( forcast_man_.Find3pForcast(stock_code_, k_type_, false, *item_a, *item_b) ) // already exist
                return ResetDrawState(draw_action_);  

            if( !(item_a->stk_item.low_price > item_b->stk_item.low_price) )
            { 
                drawing_line_B_ = item_b->kline_posdata(wall_index_).top; 
                append_3pforcast_data(this, false, *item_a, *item_b);
                return;
            }else
            {
                return ResetDrawState(draw_action_);
                //QMessageBox::information(nullptr, QString::fromLocal8Bit("提示"), QString::fromLocal8Bit("上升3点预测,B点最高价不能小于等于A点最高价!")); 

            }
        }
    } // if( drawing_line_B_ == CST_MAGIC_POINT )
    else if( drawing_line_C_ == CST_MAGIC_POINT ) // if to generate point C
    {
        // judge date
        auto item_b = GetKLineDataItemByXpos(drawing_line_B_.x());
        if( !item_b )
        {   // todo: show warning msg
            return ResetDrawState(DrawAction::NO_ACTION); 
        }
        auto item_c = GetKLineDataItemByXpos(event->pos().x());
        if( !item_c || item_c->stk_item.date < item_b->stk_item.date 
            || (item_c->stk_item.date == item_b->stk_item.date && item_c->stk_item.hhmmss < item_b->stk_item.hhmmss) )
        {   // todo: show warning msg
            return;
        }

        if( draw_action_ == DrawAction::DRAWING_FOR_3PDOWN_D )
        {
            auto p_data_3pfcst = forcast_man_.Find3pForcast(stock_code_, k_type_, true, *item_a, *item_b);
            assert(p_data_3pfcst);
            // set to point C-----
            p_data_3pfcst->date_c = item_c->stk_item.date;
            p_data_3pfcst->hhmm_c = item_c->stk_item.hhmmss;
            //drawing_line_C_ = item_c->kline_posdata.top;
            auto d1_d2_d3 = ForcastD_ABC_Down(item_a->stk_item.high_price, item_b->stk_item.low_price);
            p_data_3pfcst->d1 = std::get<0>(d1_d2_d3);
            p_data_3pfcst->d2 = std::get<1>(d1_d2_d3);
            p_data_3pfcst->d3 = std::get<2>(d1_d2_d3);

            double ori_price = lowestMinPrice_;
            lowestMinPrice_ = std::min(lowestMinPrice_, p_data_3pfcst->d1);
            lowestMinPrice_ = std::min(lowestMinPrice_, p_data_3pfcst->d3);
            if( lowestMinPrice_ < ori_price )
            {
                lowestMinPrice_ *= 0.98;
                UpdatePosDatas();
            }
            //update();
            return ResetDrawState(draw_action_);  

        }else if( draw_action_ == DrawAction::DRAWING_FOR_3PUP_D )
        {
            auto p_data_3pfcst = forcast_man_.Find3pForcast(stock_code_, k_type_, false, *item_a, *item_b);
            assert(p_data_3pfcst);
            // set to point C-----
            p_data_3pfcst->date_c = item_c->stk_item.date;
            p_data_3pfcst->hhmm_c = item_c->stk_item.hhmmss;
            //drawing_line_C_ = item_b->kline_posdata.bottom;
            auto d1_d2_d3 = ForcastD_ABC_Up(item_a->stk_item.low_price, item_b->stk_item.high_price);
            p_data_3pfcst->d1 = std::get<0>(d1_d2_d3);
            p_data_3pfcst->d2 = std::get<1>(d1_d2_d3);
            p_data_3pfcst->d3 = std::get<2>(d1_d2_d3);
            double ori_price = highestMaxPrice_;
            highestMaxPrice_ = std::max(highestMaxPrice_, p_data_3pfcst->d1);
            highestMaxPrice_ = std::max(highestMaxPrice_, p_data_3pfcst->d3);
            if( highestMaxPrice_ > ori_price )
            {
                highestMaxPrice_ *= 1.02;
                UpdatePosDatas();
            }
            return ResetDrawState(draw_action_);  
        }
    }        
}

void KLineWall::mouseReleaseEvent(QMouseEvent * e) 
{
    main_win_->SetCurKlineWallIndex((WallIndex)wall_index_);
    if( mm_move_flag_ )
    {
        pre_k_rend_index_ = k_rend_index_;
        mm_move_flag_ = false;
    }else if( area_select_flag_ )
    {
        area_select_flag_ = false;
        if( move_start_point_ != e->pos() )
        {  
            area_sel_mouse_release_point_ = e->pos();
            k_wall_menu_->popup(QCursor::pos());
             
        }else if( wall_index_ == (int)WallIndex::MAIN )
        {  
            const double x_pos = e->localPos().x();
            right_clicked_k_date_ = 0;
            right_clicked_k_hhmm_ = 0;
            int j = 0;
            for( auto iter = p_hisdata_container_->rbegin() + k_rend_index_;
                iter != p_hisdata_container_->rend() && j < k_num_; 
                ++iter, ++j)
            {  
                T_KlinePosData &pos_data = iter->get()->kline_posdata(wall_index_);
                if( pos_data.x_left == CST_MAGIC_POINT.x() )
                    continue;
                //if( pos_data.x_left > 0.0 )
                //    qDebug() << " " << pos_data.x_left << " " << x_pos << " " << pos_data.x_right << "\n";
                if( pos_data.x_left <= x_pos && x_pos <= pos_data.x_right )
                {
                    right_clicked_k_date_ = iter->get()->stk_item.date;
                    right_clicked_k_hhmm_ = iter->get()->stk_item.hhmmss;
                    break;
                } 
            }
            if( right_clicked_k_date_ != 0 )
                k_wall_menu_sub_->popup(QCursor::pos());
        }
        
    }else
    {
        if( cur_select_forcast_ )
        {
            if( main_win_->is_train_mode() )
                main_win_->MinimizeTrainDlg();

            /*if( main_win_->is_mock_trade() )
                main_win_->MinimizeMockTradeDlg();*/
            auto ret = QMessageBox::information(nullptr, QString::fromLocal8Bit("提示"), QString::fromLocal8Bit("是否删除该预测?"), QMessageBox::Yes, QMessageBox::No);
            if( ret == QMessageBox::Yes )
            {
                if( cur_select_forcast_ ) // mybe already del by pre infor box clicked
                    cur_select_forcast_->Clear();
                this->Set_Cursor(Qt::CrossCursor);
            }
        }
    }
}

void KLineWall::paintEvent(QPaintEvent*)
{
    static auto IsAreaShapeChange = [](KLineWall *kwall, int w, int h)->bool
    {
        return w != kwall->pre_mm_w_ || h!= kwall->pre_mm_h_;
    }; 
    /* window lay:********************
      |      | header
      | k mm | body 
      | zhi_biao windows 
      | bottom (show date)
    *********************************/
    std::lock_guard<std::mutex> locker(painting_mutex_);

    QPainter painter(this); 
    auto old_font = painter.font();
    QPen red_pen; red_pen.setColor(Qt::red); red_pen.setStyle(Qt::SolidLine); red_pen.setWidth(1);
    QPen green_pen; green_pen.setColor(Qt::green); green_pen.setStyle(Qt::SolidLine); green_pen.setWidth(1);
    QBrush red_brush(Qt::red);  
    QBrush green_brush(Qt::green);  
    QPen border_pen(red_pen); border_pen.setWidth(2);
    QPen lit_border_pen(red_pen); lit_border_pen.setWidth(1);
    //qDebug() << "paintEvent QCursor::pos  x:" << QCursor::pos().x() << " y: "<< QCursor::pos().y() << "\n";
    auto pos_from_global = mapFromGlobal(QCursor::pos());
    //qDebug() << "paintEvent x:" << pos_from_global.x() << " y: "<< pos_from_global.y() << "\n";
      
    const int k_mm_h = Calculate_k_mm_h();
    const int mm_w = this->width();
     
    const bool is_area_shape_change = IsAreaShapeChange(this, this->width(), this->height());
    if( is_area_shape_change )
    {
        UpdatePosDatas(); 
    }
    QString k_detail_str;
     /*
     ------------>
     |
     |
    \|/
    纵坐标(各行价格值)：((HighestMaxPrice - lowestMinPrice_) * i)/7  + lowestMinPrice_
    横坐标(日期)：分成X等份， 每天的横坐标 
    QFont font;  
    font.setPointSize(Y_SCALE_FONT_SIZE);      font.setFamily("Microsoft YaHei");  
    font.setLetterSpacing(QFont::AbsoluteSpacing,0);      painter.setFont(font);  
    //获取这种字体情况下，文本的长度和高度  
    QFontMetricsF fontMetrics(font);  
    qreal wid = fontMetrics.width(scale_val_str);      qreal height = fontMetrics.height();  
    */ 
    //painter.translate(30, 400);  //坐标 向下 向右 移 
    
    const int price_scale_num = 8;
    const int scale_part_h = k_mm_h / price_scale_num; 
    int trans_y_totoal = 0;
     
    h_axis_trans_in_paint_k_ = k_mm_h + HeadHeight(); 
    painter.translate(0, h_axis_trans_in_paint_k_); // translate frame of axes to k mm bottom
    trans_y_totoal += h_axis_trans_in_paint_k_;

    QPointF cur_mous_point_trans(cur_mouse_point_.x(), cur_mouse_point_.y() - h_axis_trans_in_paint_k_);
    do
    {   
        if( draw_action_ == DrawAction::NO_ACTION ) 
            break;
        if( drawing_line_A_ == CST_MAGIC_POINT ) 
            break; 
        QPen pen;
        if( draw_action_ == DrawAction::DRAWING_FOR_2PDOWN_C || draw_action_ == DrawAction::DRAWING_FOR_3PUP_D )
            pen.setColor(Qt::magenta);
        else
            pen.setColor(Qt::darkGreen);
        pen.setWidth(2);
        painter.setPen(pen); 
        if( drawing_line_B_ == CST_MAGIC_POINT ) // draw line AB
        {   
            painter.drawLine(drawing_line_A_.x(), drawing_line_A_.y(), cur_mous_point_trans.x(), cur_mous_point_trans.y() );
            break;
        }else if( draw_action_ == DrawAction::DRAWING_FOR_3PDOWN_D || draw_action_ == DrawAction::DRAWING_FOR_3PUP_D )
        {   // draw line BC
            painter.drawLine(drawing_line_B_.x(), drawing_line_B_.y(), cur_mous_point_trans.x(), cur_mous_point_trans.y() );
            break;
        }
        //qDebug() << " mouseMoveEvent DRAWING_FOR_2PDOWN_C " << drawing_line_A_.x() << " " << drawing_line_A_.y() << " " << cur_mouse_point_.x() << " " << cur_mouse_point_.y() << "\n";
    }while(0);

    auto ck_h = this->height();

    // draw stock code     -------------
    QPen pen; 
    pen.setColor(Qt::white);
    painter.setPen(pen);

    QPen pen_notice;
    pen_notice.setWidth(0.1); 
    pen_notice.setColor(Qt::red);
    pen_notice.setStyle(Qt::DotLine); // ............ 

    QFont font;  
    font.setPointSize(mm_w / 45); 
    painter.setFont(font);

    QString code_name = QString::fromLocal8Bit((stock_code_ + stock_name_).c_str()); 
    painter.drawText(mm_w - right_w_ - code_name.size() * font.pointSize(), -1 * (h_axis_trans_in_paint_k_ - font.pointSize()*1.2), code_name);
     
    painter.setFont(old_font); 
    
    // right vertical line | -----------
    painter.setPen(border_pen);
    painter.drawLine(mm_w - right_w_, this->height() - h_axis_trans_in_paint_k_, mm_w - right_w_, -1 * this->height());
     
    // vertical' price scale ------------
    int old_pen_width = pen.width();
    //pen.setColor(Qt::red);
    pen.setColor(Qt::gray);
    pen.setStyle(Qt::DotLine); // ............
    painter.setPen(pen); 
    const float price_per_len = (highestMaxPrice_ - lowestMinPrice_) / float(k_mm_h);
    for(int i = 0; i < price_scale_num; i++)
    {
        int pos_y = (-1) * scale_part_h * i; 
        painter.drawText(mm_w - right_w_, pos_y, QString("%1").arg(lowestMinPrice_ + (price_per_len * scale_part_h * i) ));
        ///painter.drawLine(0, pos_y, mm_w - right_w_, pos_y);
    }
#if 0         
    //-----orders line -------0330
    pen.setColor(Qt::lightGray);
    pen.setWidth(1);
    painter.setPen(pen);  
    std::list<OrderInfo> *order_cotainer_array[] = {&hangon_order_infos_, &stop_profit_order_infos_, &stop_loss_order_infos_, &condition_order_infos_};
    for( int i = 0; i < sizeof(order_cotainer_array)/sizeof(order_cotainer_array[0]); ++i )
    {
        std::for_each(std::begin(*order_cotainer_array[i]), std::end(*order_cotainer_array[i]), [&painter, &pen, this, mm_w, k_mm_h, price_per_len](OrderInfo &entry)
        {
            QString info_tag;
            if( entry.type == OrderType::STOPPROFIT || entry.type == OrderType::STOPLOSS )
            {
                assert( entry.action == OrderAction::CLOSE );
                info_tag = QString::fromLocal8Bit(entry.position_type == PositionType::POS_LONG ? "卖" : "买");
            }else
            {
                if( entry.action == OrderAction::CLOSE )
                    info_tag = QString::fromLocal8Bit(entry.position_type == PositionType::POS_LONG ? "卖" : "买");
                else
                    info_tag = QString::fromLocal8Bit(entry.position_type == PositionType::POS_LONG ? "买" : "卖");
            }
            info_tag += QString::fromLocal8Bit(entry.action == OrderAction::OPEN ? "开" : "平");
            double pos_y = get_price_y(entry.price, k_mm_h);
            painter.drawLine(0.0, pos_y, double(mm_w - 2*right_w_), pos_y);
            painter.drawText((mm_w - 2*right_w_)/2, pos_y, info_tag);
        });
    }
#endif
    //-------------------------
    pen.setWidth(old_pen_width);

    //draw k_num_ k line -------------------------------------
	if( p_hisdata_container_ && !p_hisdata_container_->empty() )
	{   
        char temp_str[1024];
        auto iter = p_hisdata_container_->rbegin() + k_rend_index_;
        sprintf_s(temp_str, sizeof(temp_str), "%s 开:%.2f 收:%.2f   高:%.2f 低:%.2f ", stock_code_.c_str(), (*iter)->stk_item.open_price
            , (*iter)->stk_item.close_price, (*iter)->stk_item.high_price, (*iter)->stk_item.low_price);
        k_detail_str = QString::fromLocal8Bit(temp_str);

        double item_w = double(mm_w - empty_right_w_ - right_w_) / double(k_num_ + 1) ;
        
#if 1
        // for fengxin line and every k line--------------
        bool is_first_fenxin_in_r_side = true;
        assert( p_hisdata_container_->size() > k_rend_index_ );
        int j = k_num_;
        for( auto iter = p_hisdata_container_->rbegin() + k_rend_index_;
            iter != p_hisdata_container_->rend() && j > 0; 
            ++iter, --j)
        { 
#endif
        T_KlinePosData &pos_data = iter->get()->kline_posdata(wall_index_);
         
        //draw every k bar---------------
        QBrush brush(QColor(255,0,0));  
        pen.setStyle(Qt::SolidLine);
        if( (*iter)->stk_item.open_price <= (*iter)->stk_item.close_price )
        { 
            pen.setColor(QColor(255,0,0));
            brush.setColor(QColor(255,0,0));
        }else
        { 
            pen.setColor(Qt::darkGreen); //QColor(0,255,0)
            brush.setColor(Qt::darkGreen); //QColor(0,255,0)
        }
        painter.setPen(pen);  
        painter.setBrush(brush);   
        painter.drawRect(pos_data.columnar_top_left.x(), pos_data.columnar_top_left.y(), pos_data.x_right - pos_data.x_left, pos_data.height);
        painter.drawLine(pos_data.top.x(), pos_data.top.y(), pos_data.bottom.x(), pos_data.bottom.y());
        //------over trading time vertical line----------
        if( (*iter)->stk_item.hhmmss == 1500 || (*iter)->stk_item.hhmmss == 230 )
        {
            painter.setPen(pen_notice); 
            painter.drawLine(pos_data.top.x(), this->height() - h_axis_trans_in_paint_k_, pos_data.top.x(), -1 * this->height());
        }
        //----------------------------------------------
        painter.setPen(pen); 
        if( iter->get()->stk_item.date == highest_price_date_ && iter->get()->stk_item.hhmmss == highest_price_hhmm_ )
        {
            char buf[32] = {'\0'};
            sprintf_s(buf, "%.2f\0", iter->get()->stk_item.high_price);
            painter.drawText(pos_data.top.x()-item_w/3, pos_data.top.y(), buf);
        }
        if( iter->get()->stk_item.date == lowest_price_date_ && iter->get()->stk_item.hhmmss == lowest_price_hhmm_)
        {
            char buf[32] = {'\0'};
            sprintf_s(buf, "%.2f\0", iter->get()->stk_item.low_price);
            painter.drawText(pos_data.bottom.x()-item_w/3, pos_data.bottom.y()+old_font.pointSizeF(), buf);
        }

        T_KlinePosData *pre_pos_data = (j != k_num_ ? std::addressof((iter - 1)->get()->kline_posdata(wall_index_)) : nullptr);
        if( pos_from_global.x() >= pos_data.x_left 
            && (pos_from_global.x() <= pos_data.x_right || (pre_pos_data && pos_from_global.x() < pre_pos_data->x_left)) )
        {
            char temp_str[1024];
            if( k_type_ < TypePeriod::PERIOD_DAY )
                sprintf_s(temp_str, "%08d:%04d\0", (*iter)->stk_item.date, (*iter)->stk_item.hhmmss);
            else
                sprintf_s(temp_str, "%08d\0", (*iter)->stk_item.date);
            k_date_time_str_ = temp_str;
            //for debug show---
            k_date_time_str_ += utility::FormatStr("(%d)", p_hisdata_container_->size() - k_rend_index_ - 1 - (k_num_ - j) );
            
            //-----------------
            sprintf_s(temp_str, sizeof(temp_str), "%s 开:%.2f 收:%.2f   高:%.2f 低:%.2f \0", stock_code_.c_str(), (*iter)->stk_item.open_price
                                                  , (*iter)->stk_item.close_price, (*iter)->stk_item.high_price, (*iter)->stk_item.low_price);
            k_detail_str = QString::fromLocal8Bit(temp_str);
            // ave line cur index value
            for( auto line_iter = ave_lines_pos_.begin(); line_iter != ave_lines_pos_.end(); ++line_iter )
            {
                unsigned int line_t = line_iter->first; 
                const int data_index = p_hisdata_container_->size() - 1 - k_rend_index_ - (k_num_ - j); 
                double val = app_->stock_data_man().GetAveLineZhibiao(ToPeriodType(k_type_), stock_code_, line_t, data_index); 
                sprintf_s(temp_str, sizeof(temp_str), " %dT:%.2f\0", line_t, val);
                k_detail_str += temp_str;
            }
        }
        // debug -----------
        /*int cckk_date = iter->get()->stk_item.date;
        if( pos_data.date != iter->get()->stk_item.date )
        printf("error");*/
        // end -----------
        const int data_index = p_hisdata_container_->size() - 1 - k_rend_index_ - (k_num_ - j); 
        //--------------AVE LINE----------
#if 0 
        if( (iter + 1) != p_hisdata_container_->rend() && j - 1 > 0)
        {
            auto next_iter = iter + 1;
            T_KlinePosData &next_pos_data = next_iter->get()->kline_posdata(wall_index_);
            const int next_data_index = data_index - 1;
            int color = Qt::red;
            for( auto line_iter = ave_lines_pos_.begin(); line_iter != ave_lines_pos_.end(); ++line_iter )
            {
                //unsigned int line_t = line_iter->first;  
                auto cur_item_y = line_iter->second[data_index];
                auto next_item_y = line_iter->second[next_data_index];
                QPen ave_line_pen; 
                ave_line_pen.setColor(Qt::GlobalColor(color));
                ave_line_pen.setStyle(Qt::SolidLine);
                painter.setPen(ave_line_pen);
                painter.drawLine(next_pos_data.top.x(), next_item_y, pos_data.top.x(), cur_item_y);
                color += 1;
            }
        }
#endif
        //----------------trades pos -----------------
#if 0 
        auto trades_iter = trades_pos_.find(data_index);
        if( trades_iter != trades_pos_.end() )
        {  
            //option.setWrapMode(QTextOption::WordWrap); // ps: is no effort, why ?
            pen.setColor(Qt::darkYellow); 
            painter.setPen(pen);
            QFontMetrics fm(painter.font());
            std::string info_str;
            double beg_pos = -1.0;
            int i = 0;
            for( auto iter = trades_iter->second.begin(); iter != trades_iter->second.end(); ++iter, ++i )
            {
                beg_pos = trades_iter->second.rbegin()->pos;
                info_str = iter->record->action == OrderAction::OPEN ? " K" : " P";
                info_str += iter->record->pos_type == PositionType::POS_LONG ? "l" : "s";
                char buf[16];
                sprintf_s(buf, sizeof(buf), " %.1f \n \0", iter->record->price); // ps: '\n' is no effort
                info_str += buf;
                painter.drawText(pos_data.bottom.x() - item_w/3, beg_pos - i*fm.height(), QString::fromLocal8Bit(info_str.c_str()));
            }
        }
#endif
        // ------------fengxin relate -------------------  
        if( is_draw_fengxin_ && (*iter)->type != (int)FractalType::UNKNOW_FRACTAL )
        {   
            do
            { 
                if( main_win_->is_train_mode() && is_first_fenxin_in_r_side ) 
                {
                    is_first_fenxin_in_r_side = false;
                    break;
                }
            QPen fractal_pen;
            fractal_pen.setStyle(Qt::SolidLine);
            fractal_pen.setColor(QColor(0,0,255));
            painter.setPen(fractal_pen);

            T_KlinePosData *left_pos_data = nullptr;
            if( IsBtmFractal((*iter)->type) ) // bottom fractal 
            {   //painter.drawText(pos_data.bottom.x(), pos_data.bottom.y(), "BTM");
                int top_item_index = FindTopItem_TowardLeft(*p_hisdata_container_, iter, j, left_pos_data);
                if( top_item_index > 0 )
                {
                    if( top_item_index == j )
                    {
                        painter.drawLine(pos_data.bottom.x(), pos_data.bottom.y(), left_pos_data->top.x(), left_pos_data->top.y());
                        if( FindBtmItem_TowardLeft(*p_hisdata_container_, iter, j, left_pos_data) )
                            painter.drawLine(pos_data.top.x(), pos_data.top.y(), left_pos_data->bottom.x(), left_pos_data->bottom.y());
                    }else
                        painter.drawLine(pos_data.bottom.x(), pos_data.bottom.y(), left_pos_data->top.x(), left_pos_data->top.y());
                }
            }else // top fractal 
            {   //painter.drawText(pos_data.top.x(), pos_data.top.y(), "TOP");
                if( FindBtmItem_TowardLeft(*p_hisdata_container_, iter, j, left_pos_data) > 0 )
                    painter.drawLine(pos_data.top.x(), pos_data.top.y(), left_pos_data->bottom.x(), left_pos_data->bottom.y());
            }

            }while(0);
        }// if fenxin
        //----------------draw signal -----------------------

        if( main_win_->show_sig() && (*iter)->tag != (int)TagType::UNKNOW_TAG )
        {
            QPen old_pen = painter.pen();
            if( ((*iter)->tag & (int)TagType::BUY) == (int)TagType::BUY )
            {
                pen.setColor(Qt::red); 
                const auto font_size = painter.font().pointSizeF();
                painter.setPen(pen);
                painter.drawText(pos_data.bottom.x() - item_w/3, pos_data.bottom.y() + item_w, "B");
                qDebug() << "draw signal " <<  (*iter)->stk_item.hhmmss << " B";
            }
            if( ((*iter)->tag & (int)TagType::SELL) == (int)TagType::SELL )
            {
                pen.setColor(Qt::green); 
                const auto font_size = painter.font().pointSizeF();
                painter.setPen(pen);
                painter.drawText(pos_data.top.x() - item_w/3, pos_data.top.y() - item_w/2, "S");
                qDebug() << "draw signal " <<  (*iter)->stk_item.hhmmss << " S";
            }
            painter.setPen(old_pen);
        }
 
      }  // for all k bar 

        if( is_draw_bi_ )
            DrawBi(painter, k_mm_h);
        if( is_draw_struct_line_ )
            DrawStructLine(painter, k_mm_h);
        if( is_draw_section_ )
            DrawSection(painter, k_mm_h);
        if( is_draw_wave_ )
            DrawWave(painter, k_mm_h);

        // paint handle forecast ----------------------
        Draw2pDownForcast(painter, k_mm_h, item_w, forcast_man_);
        Draw2pUpForcast(painter, k_mm_h, item_w, forcast_man_);  

        Draw3pDownForcast(painter, k_mm_h, item_w, forcast_man_);
        Draw3pUpForcast(painter, k_mm_h, item_w, forcast_man_);
        if( main_win_->is_show_autoforcast() )
        {
        Draw2pDownForcast(painter, k_mm_h, item_w, auto_forcast_man_);
        Draw2pUpForcast(painter, k_mm_h, item_w, auto_forcast_man_);  

        Draw3pDownForcast(painter, k_mm_h, item_w, auto_forcast_man_);
        Draw3pUpForcast(painter, k_mm_h, item_w, auto_forcast_man_);
        }
    } //if( p_hisdata_container_ )
   
    //k line view bottom border horizontal line (----------)
    painter.setPen(lit_border_pen); 
    ///painter.drawLine(0, 0, mm_w, 0); //tmp nouse it
 
    // draw zibiao----------------------- 
    //const double item_w = double(mm_w - empty_right_w_ - right_w_) / double(k_num_ + 1) ;
    for( unsigned int i = 0 ; i < zb_windows_.size(); ++i )
    {
        if( zb_windows_[i] )
        {
            const int zb_h = zb_windows_[i]->Height();
            painter.translate(0, zb_h + lit_border_pen.width());
            trans_y_totoal += zb_h + lit_border_pen.width();
            // zibiao horizontal line -----
            painter.setPen(lit_border_pen);
            painter.drawLine(0, 0, mm_w, 0); // zibiao bottom line 
            painter.drawLine(0, -1*zb_h, mm_w, -1*zb_h); // zibiao head line 
            zb_windows_[i]->DrawWindow(painter, mm_w);
        }
    }
    // end of draw zibiao-----------------------
 
    // bottom calender right tag ------------------------------------
    const int bttome_height = BottomHeight();
    painter.translate(0, bttome_height);
    trans_y_totoal += bttome_height;
    painter.setPen(border_pen);
    painter.drawLine(0, 0, mm_w, 0); // draw bottom line
     
    // draw left top k bar detail ------
    painter.translate(0, -1 * trans_y_totoal); // translate axis back
    trans_y_totoal = 0;
    pen.setColor(Qt::white);
    pen.setStyle(Qt::SolidLine); 
    painter.setPen(pen);  
    font.setPointSizeF(mm_w / 70);  // font.setPointSizeF(mm_w / 50);
    painter.setFont(font);
    painter.drawText(0, font.pointSize() * 1.2, k_detail_str);
    painter.setFont(old_font);
    // draw cross line --------------------
    if( show_cross_line_ )
    { 
        // horizontal line 
        painter.drawLine(0, pos_from_global.y(), mm_w-right_w_, pos_from_global.y());
        // vertical line 
        painter.drawLine(pos_from_global.x(), HeadHeight(), pos_from_global.x(), this->height()); 
        painter.drawText( mm_w-right_w_, pos_from_global.y(), QString("%1").arg(lowestMinPrice_ + price_per_len * (h_axis_trans_in_paint_k_ - pos_from_global.y()) ) );
    }
    if( area_select_flag_ )
    {
        QRect rect(move_start_point_, pos_from_global);
        painter.fillRect(rect, QBrush(QColor(128, 128, 255, 128))); 
    }
    // draw date
    painter.drawText(pos_from_global.x()-k_date_time_str_.size(), this->height()-1, k_date_time_str_.c_str());
   
    this->pre_mm_w_ = this->width();
    this->pre_mm_h_ = this->height(); 
}


void KLineWall::mouseDoubleClickEvent(QMouseEvent*)
{
    //qDebug() << " x:" << e->pos().x() <<" y:" << e->pos().y() << "\n";
    show_cross_line_ = !show_cross_line_;
    //qDebug() << "show_cross_line:" << show_cross_line_ << "\n";
    //setMouseTracking(show_cross_line_);
    update();
}

// this->mapFromGlobal(this->cursor().pos()) == event.pos()
void KLineWall::mouseMoveEvent(QMouseEvent *e)
{ 
    auto pos = e->pos();
    //qDebug() << " mouseMoveEvent " << "\n";
 
    if( mm_move_flag_ )
    {
        if( k_num_ > 0 )
        {
            const int atom_k_width = this->width() / k_num_;
            if( atom_k_width == 0 )
                return;

            const int distance = e->pos().x() - move_start_point_.x();
            if( distance > 0 ) // drag to right 
            { 
                const int tmp_k_rend_index = pre_k_rend_index_ + abs(distance) / atom_k_width;

                if(p_hisdata_container_->size() < tmp_k_rend_index + k_num_ ) // for autoforcast not to append predata
                    return;
                else
                { 
                    int tmp_val = p_hisdata_container_->size() > 0 ? p_hisdata_container_->size() - 1 : 0;
                    k_rend_index_ = std::min(tmp_k_rend_index, tmp_val);
                }

            }else // drag to left
            {
                const int num = pre_k_rend_index_ - abs(distance) / atom_k_width;
                int temp_val = (num > 0 ? num : 0);
                if( main_win_->is_train_mode() )
                {
                    k_rend_index_ = temp_val < k_rend_index_for_train_ ? k_rend_index_for_train_ : temp_val;
                }else
                    k_rend_index_ = temp_val;
                
            }
            if( k_rend_index_ != k_move_temp_index_ )
            {
                UpdateKwallMinMaxPrice();
                UpdatePosDatas(); 
                k_move_temp_index_ = k_rend_index_;
            }
        }
    } // if( mm_move_flag_ )
    else if( draw_action_ != DrawAction::NO_ACTION )
    {
        //qDebug() << " mouseMoveEvent DRAWING_FOR_2PDOWN_C " << "\n";
        cur_mouse_point_ = e->pos();
    }else if( area_select_flag_ )
    {
         
    }else if( draw_action_ == DrawAction::NO_ACTION )
    {
        DoIfForcastLineNearbyCursor(*e);
    }
    update();
}

void KLineWall::DoIfForcastLineNearbyCursor(QMouseEvent &e)
{
    static auto is_nearby_line = [](QPointF &beg_point, QPointF &end_point, QPointF &point) -> bool
    { 
        double k = (end_point.y() - beg_point.y()) / (end_point.x() - beg_point.x());
        double y_on_line = k * (point.x() - beg_point.x()) + beg_point.y();
        return fabs(point.y() - y_on_line) < 4;
    };

    static auto get_target_forcast_from_3p = [](KLineWall *k_wall, Data3pForcastInnerContainer *p_data_vector, QPointF cur_mous_point_trans)->bool
    {
        assert( p_data_vector);
        //for( unsigned int i = 0; i < p_data_vector->size(); ++i )
        for( auto iter = p_data_vector->begin(); iter != p_data_vector->end(); ++iter )
        {
            T_Data3pForcast &data_3p_forcast = **iter;
            if( cur_mous_point_trans.x() > data_3p_forcast.point_a.x() - 0.0001 && cur_mous_point_trans.x() < data_3p_forcast.point_b.x() + 0.0001 )
            {
                if( is_nearby_line(data_3p_forcast.point_a, data_3p_forcast.point_b, cur_mous_point_trans) )
                {
                    k_wall->cur_select_forcast_ = &data_3p_forcast; //  (T_DataForcast*)
                    return true;
                }
            }
            if( cur_mous_point_trans.x() > data_3p_forcast.point_b.x() - 0.0001 && cur_mous_point_trans.x() < data_3p_forcast.point_c.x() + 0.0001 )
            {
                if( is_nearby_line(data_3p_forcast.point_b, data_3p_forcast.point_c, cur_mous_point_trans) )
                {
                    k_wall->cur_select_forcast_ = &data_3p_forcast; //  (T_DataForcast*)
                    return true;
                }
            }
        }
        return false;
    };

    static auto get_target_forcast_from_2p = [](KLineWall *k_wall, Data2pForcastInnerContainer *p_data_vector, QPointF cur_mous_point_trans)->bool
    {
        assert( p_data_vector);
        //for( unsigned int i = 0; i < p_data_vector->size(); ++i )
        for( auto iter = p_data_vector->begin(); iter != p_data_vector->end(); ++iter )
        {
            T_Data2pForcast &data_2p_forcast = *iter;
            if( cur_mous_point_trans.x() > data_2p_forcast.point_a.x() - 0.0001 && cur_mous_point_trans.x() < data_2p_forcast.point_b.x() + 0.0001 )
            {
                if( is_nearby_line(data_2p_forcast.point_a, data_2p_forcast.point_b, cur_mous_point_trans) )
                {
                    k_wall->cur_select_forcast_ = &data_2p_forcast; //  (T_DataForcast*)
                    return true;
                }
            }
            
        }
        return false;
    };

    cur_select_forcast_ = nullptr;
    QPointF cur_mous_point_trans(e.x(), e.y() - h_axis_trans_in_paint_k_);
    bool ret = false;
    auto p_data_vector = forcast_man_.Find3pForcastVector(stock_code_, k_type_, /*is_down_forward*/true);
    if( p_data_vector )
        ret = get_target_forcast_from_3p(this, p_data_vector, cur_mous_point_trans); 
    if( !ret )
    {
        p_data_vector = forcast_man_.Find3pForcastVector(stock_code_, k_type_, /*is_down_forward*/false);
        if( p_data_vector )
            ret = get_target_forcast_from_3p(this, p_data_vector, cur_mous_point_trans);
    }
    if( !ret )
    {
        auto p_2p_data_vector = forcast_man_.Find2pForcastVector(stock_code_, k_type_, /*is_down_forward*/true);
        if( p_2p_data_vector )
            ret = get_target_forcast_from_2p(this, p_2p_data_vector, cur_mous_point_trans);
        if( !ret )
        {
            p_2p_data_vector = forcast_man_.Find2pForcastVector(stock_code_, k_type_, /*is_down_forward*/false);
            if( p_2p_data_vector )
                ret = get_target_forcast_from_2p(this, p_2p_data_vector, cur_mous_point_trans);
        }
    }
    if( ret )
        this->Set_Cursor(cst_cur_del_forcst_line);
    else
        this->Set_Cursor(Qt::CrossCursor);

}

void KLineWall::keyPressEvent(QKeyEvent *e)
{
    assert(p_hisdata_container_);
    //assert(p_hisdata_container_->size() >  k_rend_index_ );
    auto key_val = e->key();
    if( (e->modifiers() & Qt::ControlModifier) )
    {
        if( key_val == Qt::Key_Z )
        {
            // : add to code list
            main_win_->AddCode2CodeList(stock_code_.c_str(), QString::fromLocal8Bit(stock_name_.c_str()), is_index_, nmarket_);
            e->ignore();
            return;
        }
    }

    switch( key_val )
    {
    case Qt::Key_Left:
        {
            if( !show_cross_line_ )
                return;
            auto pos_from_global = mapFromGlobal(QCursor::pos());
            if( pos_from_global.x() > 0 )
            {
                double item_w = double(this->width() - empty_right_w_ - right_w_) / double(k_num_ + 1) ;

                QPoint curs_new_point(QCursor::pos().x() - item_w, QCursor::pos().y());
                QCursor::setPos(curs_new_point);

            }else
            {
                if( k_rend_index_< p_hisdata_container_->size() - 1 )
                {
                    ++k_rend_index_;
                    UpdateKwallMinMaxPrice();
                    UpdatePosDatas();
                    update();
                }
            }
            pos_from_global = mapFromGlobal(QCursor::pos());
            //qDebug() << "Key_Left x:" << pos_from_global.x();
            break;
        }
    case Qt::Key_Right:
        {
            if( !show_cross_line_ )
                return;
            auto pos_from_global = mapFromGlobal(QCursor::pos());
            if( pos_from_global.x() < this->width() )
            {
                double item_w = double(this->width() - empty_right_w_ - right_w_) / double(k_num_ + 1) ;

                QPoint curs_new_point(QCursor::pos().x() + item_w, QCursor::pos().y());
                QCursor::setPos(curs_new_point);
            }else if( !main_win_->is_train_mode() )
            {
                if( k_rend_index_ > 0 )
                {
                    --k_rend_index_;
                    UpdateKwallMinMaxPrice();
                    UpdatePosDatas();
                    update();
                }
            }
            pos_from_global = mapFromGlobal(QCursor::pos());
            //qDebug() << "Key_Right x:" << pos_from_global.x();
            break;
        }
    case Qt::Key_Up:  //zoom out 
        {
            if( p_hisdata_container_->empty() )
                return;
            if( k_num_ > 1 )
            {
                k_num_ --;
                UpdateKwallMinMaxPrice();
                UpdatePosDatas();
                update();
            }
            break;
        }
    case Qt::Key_Down: //zoom in 
        {  
            if( p_hisdata_container_->empty() )
                return;

            // for autoforcast 
            if( k_num_ + 1 > p_hisdata_container_->size() - k_rend_index_ )
            {
                if( k_rend_index_ > 0 )
                {
                    --k_rend_index_;
                    k_num_ ++;
                }
                else
                    return;
            }
            else
                k_num_ ++;
            /*
            k_num_ ++;
            if( k_num_ + 20 > p_hisdata_container_->size() - k_rend_index_ )
                AppendData();*/

            UpdateKwallMinMaxPrice();
            UpdatePosDatas();
            update();
            break;
        } 
#ifdef STK_INPUT_KWALL
    case Qt::Key_F3:
    {
        ResetStock("999999", QString::fromLocal8Bit("上证指数"), true);
        break;
    }
    case Qt::Key_0: case Qt::Key_1: case Qt::Key_2: case Qt::Key_3: case Qt::Key_4:  
    case Qt::Key_5: case Qt::Key_6: case Qt::Key_7: case Qt::Key_8: case Qt::Key_9: 
    case Qt::Key_A: case Qt::Key_B: case Qt::Key_C: case Qt::Key_D: case Qt::Key_E:
    case Qt::Key_F: case Qt::Key_G: case Qt::Key_H: case Qt::Key_I: case Qt::Key_J:
    case Qt::Key_K: case Qt::Key_L: case Qt::Key_M: case Qt::Key_N: case Qt::Key_O:
    case Qt::Key_P: case Qt::Key_Q: case Qt::Key_R: case Qt::Key_S: case Qt::Key_T:
    case Qt::Key_U: case Qt::Key_V: case Qt::Key_W: case Qt::Key_X: case Qt::Key_Y:
    case Qt::Key_Z:
		{
			//qDebug() << __FUNCDNAME__ << "\n"; 
            stock_input_dlg_.ui.stock_input->clear();
            char tmpbuf[8] = {0};
            if( key_val >= Qt::Key_0 && key_val <= Qt::Key_9 )
                sprintf_s(tmpbuf, sizeof(tmpbuf), "%c", (char)key_val);
            else
                sprintf_s(tmpbuf, sizeof(tmpbuf), "%c", char(key_val+32));
            stock_input_dlg_.ui.stock_input->setText(tmpbuf);
			stock_input_dlg_.show();

		}
		break;
#endif
    default:
        break;
    }
     
    e->ignore();
}

void KLineWall::enterEvent(QEvent *)
{
    //qDebug() << __FUNCTION__ << "\n";
}

void KLineWall::leaveEvent(QEvent *)
{
    //qDebug() << __FUNCTION__ << "\n";
}

// 
void KLineWall::slotOpenStatisticDlg(bool)
{
    double left_x = std::min((double)move_start_point_.x(),  area_sel_mouse_release_point_.x());
    double right_x = std::max((double)move_start_point_.x(),  area_sel_mouse_release_point_.x());

    assert( p_hisdata_container_->size() > k_rend_index_ );
     
    int k_num_in_area = 0;
    int start_date = 0;
    int end_date = 0;
    double min_price = MAX_PRICE;
    double max_price = MIN_PRICE;
    double end_price = 0.0;
    double begin_price = 0.0;
    int j = 0;
    for( auto iter = p_hisdata_container_->rbegin() + k_rend_index_;
        iter != p_hisdata_container_->rend() && j < k_num_; 
        ++iter, ++j)
    {  
        T_KlinePosData &pos_data = iter->get()->kline_posdata(wall_index_);
        if( pos_data.x_left == CST_MAGIC_POINT.x() )
            continue;
        if( pos_data.x_right >= left_x &&  pos_data.x_left <= right_x )
        {
            ++k_num_in_area;
            if( k_num_in_area == 1 )
            {
                end_price = iter->get()->stk_item.close_price;
                end_date = iter->get()->stk_item.date;
            }
            if( iter->get()->stk_item.high_price > max_price ) 
            {
                max_price = iter->get()->stk_item.high_price; 
            }
            if( iter->get()->stk_item.low_price < min_price ) 
            {
                min_price = iter->get()->stk_item.low_price; 
            }
            begin_price = iter->get()->stk_item.open_price;
            start_date = iter->get()->stk_item.date;
        }
        if( pos_data.x_right < left_x )
            break;
    }

    statistic_dlg_.ui.lab_knum->setText(QString("%1").arg(k_num_in_area));
    char buf[32] = {0};

    sprintf_s(buf, sizeof(buf), "%d\0", end_date);
    statistic_dlg_.ui.le_end_date->setText(buf);
    sprintf_s(buf, sizeof(buf), "%d\0", start_date);
    statistic_dlg_.ui.le_start_date->setText(buf);

    if( k_num_in_area > 0 && min_price > 0.0 && begin_price > 0.0 )
    {
        double shake_scale = (max_price - min_price)* 100 / min_price;
        sprintf_s(buf, sizeof(buf), "%.2f\0", shake_scale);
        statistic_dlg_.ui.le_scale_shake->setText(buf);

        double scale = (end_price - begin_price) * 100 / begin_price;
        sprintf_s(buf, sizeof(buf), "%.2f\0", scale);
        statistic_dlg_.ui.le_scale_increase->setText(buf);
    }
    sprintf_s(buf, sizeof(buf), "%.2f\0", max_price);
    statistic_dlg_.ui.le_HighesttPrice->setText(buf);
    sprintf_s(buf, sizeof(buf), "%.2f\0", min_price);
    statistic_dlg_.ui.le_LowestPrice->setText(buf);

    statistic_dlg_.show();
}

void KLineWall::slotZoominSelect(bool)
{
    double left_x = std::min((double)move_start_point_.x(),  area_sel_mouse_release_point_.x());
    double right_x = std::max((double)move_start_point_.x(),  area_sel_mouse_release_point_.x());

    assert( p_hisdata_container_->size() > k_rend_index_ );
     
    int k_num_in_area = 0;
    
    int j = 0;
    for( auto iter = p_hisdata_container_->rbegin() + k_rend_index_;
        iter != p_hisdata_container_->rend() && j < k_num_; 
        ++iter, ++j)
    {  
        T_KlinePosData &pos_data = iter->get()->kline_posdata(wall_index_);
        if( pos_data.x_left == CST_MAGIC_POINT.x() )
            continue;
        if( pos_data.x_right >= left_x &&  pos_data.x_left <= right_x )
        {
            ++k_num_in_area;
            if( k_num_in_area == 1 )
                k_rend_index_ += j;
        }
        if( pos_data.x_right < left_x )
            break;
    }
    k_num_ = k_num_in_area;
    UpdateKwallMinMaxPrice();
    UpdatePosDatas(); 
    update();
}

void KLineWall::UpdateStockQuoteOfTrainMode()
{
    if( is_resetting_stock_ || /*draw_action_ != DrawAction::NO_ACTION || */main_win_->is_train_mode() )
        return;
    T_Quote_Data  quote_date;
    if( app_->stock_data_man().GetInstrumentQuote(stock_code_, nmarket_, quote_date) )
    {
        if( is_resetting_stock_ ||/* draw_action_ != DrawAction::NO_ACTION || */main_win_->is_train_mode() )
            return;

        main_win_->EmitSigQuoteData(quote_date.cur_price, quote_date.sell_price, quote_date.buy_price, quote_date.vol, quote_date.sell_vol, quote_date.buy_vol); // => slotHandleQuote
        main_win_->cur_quote_price(quote_date.cur_price);
    }
}

void KLineWall::slotOpenRelatedSubKwall(bool)
{
    if( main_win_->SubKlineWall() )
    {
        main_win_->SubKlineWall()->ShowDurationKlines(right_clicked_k_date_, right_clicked_k_hhmm_);
        main_win_->SubKlineWall()->setVisible(true);
    }
    main_win_->tool_bar()->SetShowSubKwallBtn(true);
}

void KLineWall::slotUpdateKwall()
{
    UpdateKwallMinMaxPrice();
    UpdatePosDatas();
    update();
}

bool KLineWall::LoadBaseStock(const QString& code, TypePeriod type_period, bool is_index, int nmarket, int start_index, int len)
{
    assert(!main_win_->is_train_mode());
    T_HisDataItemContainer & items_in_container = app_->stock_data_man().GetHisDataContainer(ToPeriodType(k_type_), code.toLocal8Bit().data());
     
    is_resetting_stock_ = true;

    k_rend_index_ = 0;
    pre_k_rend_index_ = 0;
    k_move_temp_index_ = 0;

    stock_code_ = code.toLocal8Bit().data(); 
    k_type_ = type_period;
    nmarket_ = nmarket;
     
    bool ret = false;
    do
    {  
        p_hisdata_container_ = app_->stock_data_man().LoadPartStockData(ToPeriodType(k_type_), nmarket_, stock_code_, start_index, len, is_index);

        if( !p_hisdata_container_ )
        {
            ret = false;
            break;
        }
        app_->stock_data_man().TraverseSetFeatureData(stock_code_, ToPeriodType(k_type_), is_index_, 0);
        app_->wave_man().Traverse_GetWaveLevel1(stock_code_, k_type_, 0, 0);
        app_->wave_man().TraverseSetTrendDataTowardRight(stock_code_, k_type_, 0);

        this->is_index_ = is_index;
        if( !p_hisdata_container_->empty() )
        {
            if( p_hisdata_container_->size() > 720 )
                k_num_ = p_hisdata_container_->size() / 10;
            if( p_hisdata_container_->size() > 360 )
                k_num_ = p_hisdata_container_->size() / 6;
            else if( p_hisdata_container_->size() > 180 )
                k_num_ = p_hisdata_container_->size() / 3;
            else if( p_hisdata_container_->size() > 60 )
                k_num_ = p_hisdata_container_->size() / 2;
            else 
                k_num_ = p_hisdata_container_->size();

            UpdateKwallMinMaxPrice();

        }else
        {
            this->highestMaxPrice_ = 20.0;
            this->lowestMinPrice_ = 0.0;
        }

        UpdatePosDatas();
        ret = true;

    }while(0);

    is_resetting_stock_ = false;
    return ret;
}

bool KLineWall::LoadBaseStock(const QString& code, TypePeriod type_period, bool is_index, int nmarket, T_DateRange &range)
{
    assert(!main_win_->is_train_mode());
    T_HisDataItemContainer & items_in_container = app_->stock_data_man().GetHisDataContainer(ToPeriodType(k_type_), code.toLocal8Bit().data());

    is_resetting_stock_ = true;

    k_rend_index_ = 0;
    pre_k_rend_index_ = 0;
    k_move_temp_index_ = 0;

    stock_code_ = code.toLocal8Bit().data(); 
    k_type_ = type_period;
    nmarket_ = nmarket;

    bool ret = false;
    do
    {  
        char error[64] = {'\0'};
        p_hisdata_container_ = app_->stock_data_man().LoadPartStockData(ToPeriodType(k_type_), nmarket_, stock_code_, range, is_index);
        if( !p_hisdata_container_ )
        {
            ret = false;
            break;
        }
        app_->stock_data_man().TraverseSetFeatureData(stock_code_, ToPeriodType(k_type_), is_index_, 0);
        app_->wave_man().Traverse_GetWaveLevel1(stock_code_, k_type_, 0, 0);
        app_->wave_man().TraverseSetTrendDataTowardRight(stock_code_, k_type_, 0);

        this->is_index_ = is_index;
        if( !p_hisdata_container_->empty() )
        {
            if( p_hisdata_container_->size() > 720 )
                k_num_ = p_hisdata_container_->size() / 10;
            if( p_hisdata_container_->size() > 360 )
                k_num_ = p_hisdata_container_->size() / 6;
            else if( p_hisdata_container_->size() > 180 )
                k_num_ = p_hisdata_container_->size() / 3;
            else if( p_hisdata_container_->size() > 60 )
                k_num_ = p_hisdata_container_->size() / 2;
            else 
                k_num_ = p_hisdata_container_->size();
            UpdateKwallMinMaxPrice();

        }else
        {
            this->highestMaxPrice_ = 20.0;
            this->lowestMinPrice_ = 0.0;
        }
        UpdatePosDatas();
        ret = true;
    }while(0);

    is_resetting_stock_ = false;
    return ret;
}

bool KLineWall::ResetStock(const QString& code, TypePeriod type_period, bool is_index, int nmarket)
{  
    auto date_time = GetKDataTargetDateTime(*app_->exchange_calendar(), type_period
        , QDate::currentDate().toString("yyyyMMdd").toInt(), QTime::currentTime().hour() * 100 + QTime::currentTime().minute(), WOKRPLACE_DEFUALT_K_NUM);
    return Reset_Stock(code, type_period, is_index, nmarket, std::get<0>(date_time));
}

bool KLineWall::ResetStock(const QString& code, const QString& code_name, TypePeriod type_period, bool is_index, int nmarket)
{
   if( ResetStock(code, type_period, is_index, nmarket) )
   {
       stock_name_ = code_name.toLocal8Bit().data(); 
       return true;
   }else
       return false;
}

// ps : it may update k data's current data;  make sure not invoker by train mode
bool KLineWall::Reset_Stock(const QString& stock, TypePeriod type_period, bool is_index, int nmarket, int start_date)
{   
    assert(!main_win_->is_train_mode());
    T_HisDataItemContainer & items_in_container = app_->stock_data_man().GetHisDataContainer(ToPeriodType(k_type_), stock.toLocal8Bit().data());

    if( k_type_ == type_period && stock_code_ ==  stock.toLocal8Bit().data() && !items_in_container.empty() )
        return true;

    is_resetting_stock_ = true;
    
    k_rend_index_ = 0;
    pre_k_rend_index_ = 0;
    k_move_temp_index_ = 0;

    stock_code_ = stock.toLocal8Bit().data(); 
    k_type_ = type_period;
    nmarket_ = nmarket;

    int cur_date = QDate::currentDate().year() * 10000 + QDate::currentDate().month() * 100 + QDate::currentDate().day();
    int cur_hhmm = QTime::currentTime().hour() * 100 + QTime::currentTime().minute();
    if( cur_hhmm > 2100 )
    {
        cur_date = QDate::currentDate().addDays(1).toString("yyyyMMdd").toInt();
    }
 
    bool ret = false;
    do
    {
    // temp debug -------------
    //start_date = 20190524;
    //cur_date = 20190527;
    int hhmm = GetKDataTargetStartTime(type_period, cur_hhmm);
    // find his k data which till cur hhmm --------------
    p_hisdata_container_ = app_->stock_data_man().FindStockData(ToPeriodType(k_type_), stock_code_, start_date, cur_date, hhmm, is_index);
    if( !p_hisdata_container_ )
    {
        //p_hisdata_container_ = app_->stock_data_man().AppendStockData(stock_code_, 20171216, 20180108); 
        p_hisdata_container_ = app_->stock_data_man().AppendStockData(ToPeriodType(k_type_), nmarket_, stock_code_, start_date, cur_date, is_index);
    }else
    {
        int a_pre_date = app_->exchange_calendar()->PreTradeDate(cur_date, 1);
        p_hisdata_container_ = app_->stock_data_man().AppendStockData(ToPeriodType(k_type_), nmarket_, stock_code_, a_pre_date, cur_date, is_index);
    }
	
    if( !p_hisdata_container_ )
    {
        ret = false;
        break;
    }
    app_->stock_data_man().TraverseSetFeatureData(stock_code_, ToPeriodType(k_type_), is_index_, 0);

    this->is_index_ = is_index;
    if( !p_hisdata_container_->empty() )
    {
        if( p_hisdata_container_->size() > 720 )
            k_num_ = p_hisdata_container_->size() / 10;
        if( p_hisdata_container_->size() > 360 )
            k_num_ = p_hisdata_container_->size() / 6;
        else if( p_hisdata_container_->size() > 180 )
            k_num_ = p_hisdata_container_->size() / 3;
        else if( p_hisdata_container_->size() > 60 )
            k_num_ = p_hisdata_container_->size() / 2;
        else 
            k_num_ = p_hisdata_container_->size();
 
        UpdateKwallMinMaxPrice();
 
    }else
    {
        this->highestMaxPrice_ = 20.0;
        this->lowestMinPrice_ = 0.0;
    }
    
    UpdatePosDatas();
 	ret = true;

    }while(0);

    is_resetting_stock_ = false;
    return ret;
}

bool KLineWall::Reset_Stock_Train(const QString& stock, TypePeriod type_period, bool is_index, int nmarket, int start_date, int end_date)
{
    assert(start_date <= end_date);
    T_HisDataItemContainer & items_in_container = app_->stock_data_man().GetHisDataContainer(ToPeriodType(k_type_), stock.toLocal8Bit().data());
    if( k_type_ == type_period && stock_code_ ==  stock.toLocal8Bit().data() && !items_in_container.empty() )
        return true;

    is_resetting_stock_ = true;

    k_rend_index_ = 0;
    k_rend_index_for_train_ = 0;
    pre_k_rend_index_ = 0;
    k_move_temp_index_ = 0;

    stock_code_ = stock.toLocal8Bit().data(); 
    k_type_ = type_period;
    nmarket_ = nmarket;
     
    bool ret = false;
    do
    { 
        int hhmm = GetKDataTargetStartTime(type_period, end_date);
        // find his k data which till cur hhmm --------------
        p_hisdata_container_ = app_->stock_data_man().FindStockData(ToPeriodType(k_type_), stock_code_, start_date, end_date, hhmm, is_index);
        if( !p_hisdata_container_ )
        {  
            int beg_date = start_date;
            int pre_span = 30;
            switch(k_type_)
            {
            case TypePeriod::PERIOD_HOUR: pre_span = 100 / 4 + 4;break;
            case TypePeriod::PERIOD_30M: pre_span = 100 / 8 + 3;break;
            case TypePeriod::PERIOD_15M: pre_span = 100 / 16 + 2;break;
            case TypePeriod::PERIOD_5M: pre_span = 100 / 48 + 1;break;
            case TypePeriod::PERIOD_1M: pre_span = 1;break;
            default: break;
            }
            beg_date = app_->exchange_calendar()->PreTradeDate(start_date, pre_span);
            p_hisdata_container_ = app_->stock_data_man().AppendStockData(ToPeriodType(k_type_), nmarket_, stock_code_, beg_date, end_date, is_index);
            auto p_item = SetTrainByDateTime(k_cur_train_date_, k_cur_train_hhmm_);
            if( !p_item )
            {
                // log error
            }
            app_->stock_data_man().TraverseSetFeatureData(stock_code_, ToPeriodType(k_type_), is_index_, k_rend_index_for_train_);
            //HandleAutoForcast();
        } 

        if( !p_hisdata_container_ )
        {
            ret = false;
            break;
        }
        
        this->is_index_ = is_index;
        if( !p_hisdata_container_->empty() )
        { 
            int target_k_num = 0;
            if( p_hisdata_container_->size() > 720 )
                target_k_num = p_hisdata_container_->size() / 10;
            if( p_hisdata_container_->size() > 360 )
                target_k_num = p_hisdata_container_->size() / 6;
            else if( p_hisdata_container_->size() > 180 )
                target_k_num = p_hisdata_container_->size() / 3;
            else if( p_hisdata_container_->size() > 60 )
                target_k_num = p_hisdata_container_->size() / 2;
            else 
                target_k_num = p_hisdata_container_->size();
            if( p_hisdata_container_->size() < k_num_ )
                k_num_ = p_hisdata_container_->size();
            else if( k_num_ < 30 && target_k_num > k_num_ )
                k_num_ = target_k_num;

            UpdateKwallMinMaxPrice();

        }else
        {
            this->highestMaxPrice_ = 20.0;
            this->lowestMinPrice_ = 0.0;
        }

        UpdatePosDatas();
        ret = true;

    }while(0);

    is_resetting_stock_ = false;
    return ret;
}

void KLineWall::Reset_Stock_Train()
{
    app_->ClearStockHisDatas(stock_code_, k_type_);
    is_resetting_stock_ = true;

    k_rend_index_ = 0;
    k_rend_index_for_train_ = 0;
    pre_k_rend_index_ = 0;
    k_move_temp_index_ = 0;

    cur_select_forcast_ = nullptr; 
    auto_forcast_man_.RemoveForcastItems(stock_code_, k_type_);
    forcast_man_.RemoveForcastItems(stock_code_, k_type_);
    ResetDrawState(DrawAction::NO_ACTION);
        
    this->highestMaxPrice_ = 99.9;
    this->lowestMinPrice_ = 0.0;
             
    lowest_price_date_ = 0;
    lowest_price_hhmm_ = 0;
    highest_price_date_ = 0;
    highest_price_hhmm_ = 0;

    is_resetting_stock_ = false; 
}

// ps: for sub kline wall
void KLineWall::ShowDurationKlines(int date, int hhmm)
{
    if( p_hisdata_container_->empty() )
        return;

    if( date < p_hisdata_container_->begin()->get()->stk_item.date )
    {
        int beg_date = date;
        int pre_span = 5;
        switch(k_type_)
        {
        case TypePeriod::PERIOD_HOUR: pre_span = 100 / 4 + 4;break;
        case TypePeriod::PERIOD_30M: pre_span = 100 / 8 + 3;break;
        case TypePeriod::PERIOD_15M: pre_span = 100 / 16 + 2;break;
        case TypePeriod::PERIOD_5M: pre_span = 100 / 48 + 1;break;
        default:break;
        }
        beg_date = app_->exchange_calendar()->PreTradeDate(date, pre_span);
        AppendPreData(beg_date, hhmm); 
        if( main_win_->is_train_mode() )
            SetTrainByDateTime(date, hhmm); 
        app_->stock_data_man().TraverseSetFeatureData(stock_code_, ToPeriodType(k_type_), is_index_, k_rend_index_for_train_);
    }
    k_rend_index_ = 0;
    int index = FindKRendIndex(p_hisdata_container_, date, hhmm);
    if( index > -1 )
    { 
        k_rend_index_ = index;
        k_num_ = 60;
    }else
        k_num_ = 0;

    UpdateKwallMinMaxPrice();
    UpdatePosDatas();
    update();
}

// if p_cur_price != null means invoked from main kwall to sub kwall
T_StockHisDataItem * KLineWall::UpdateIfNecessary(int target_date, int target_hhmm, T_StockHisDataItem *cur_mainkwall_item)
{
    //T_StockHisDataItem *ret_item = nullptr;
     if( is_resetting_stock_ || draw_action_ != DrawAction::NO_ACTION || main_win_->is_train_mode() 
        || mm_move_flag_ )
        return nullptr;

    bool is_need_updated = false;
     
    std::lock_guard<std::mutex> locker(painting_mutex_);
     
    //int hhmm = GetKDataTargetStartTime(k_type_, cur_hhmm);
    T_StockHisDataItem cur_k_item;
    T_HisDataItemContainer &container = app_->stock_data_man().GetHisDataContainer(ToPeriodType(k_type_), stock_code_);
    if( !container.empty() )
    {  
            int ret = 0;
            cur_k_item = container.back()->stk_item;
            if( target_date > container.back()->stk_item.date 
                || target_date == container.back()->stk_item.date && target_hhmm > container.back()->stk_item.hhmmss )
            {
                T_StockHisDataItem *ori_stk_item = app_->stock_data_man().FindEndStockDateItemFromEndContain(ToPeriodType(k_type_), stock_code_);
                if( ori_stk_item )
                    container.back()->stk_item = *ori_stk_item;//restore ori

                ret = app_->stock_data_man().AppendLatestItemStockData(ToPeriodType(k_type_), nmarket_, stock_code_, is_index_);
                
                if( ret > 0 ) // has appended to back
                { 
                    app_->stock_data_man().StoreEndStockDateItem(ToPeriodType(k_type_), stock_code_, container.back()->stk_item);
                    // set new item's high_price and low_price to close_price
                    if( cur_mainkwall_item )
                    {
                        container.back()->stk_item.close_price = cur_mainkwall_item->close_price;
                        container.back()->stk_item.low_price = cur_mainkwall_item->low_price;
                        container.back()->stk_item.high_price = cur_mainkwall_item->high_price;
                    }
                }
                
            }else // update
            {
                if( cur_mainkwall_item )
                {
                    if( !Equal(container.back()->stk_item.close_price, cur_mainkwall_item->close_price) )
                    {
                        container.back()->stk_item.close_price = cur_mainkwall_item->close_price;
                        ret = 1;
                    }
                    if( cur_mainkwall_item->low_price < container.back()->stk_item.low_price )
                    {
                        container.back()->stk_item.low_price = cur_mainkwall_item->low_price;
                        ret = 1;
                    }
                    if( cur_mainkwall_item->high_price > container.back()->stk_item.high_price )
                    {
                        container.back()->stk_item.high_price = cur_mainkwall_item->high_price;
                        ret = 1;
                    }
                }
            }
            
            if( ret > 0 )
            { 
                int backward_size = 50;
                backward_size = MIN_VAL(container.size(), backward_size); 
                TraverseClearFractalType(container, 0, backward_size);
                // ps: it's important, cause create fractal needs more indexs than clear fractal, otherwise will lost fractal
                backward_size += 5;
                backward_size = MIN_VAL(container.size(), backward_size); 
                TraverseSetUpwardFractal(container, 0, backward_size);
                TraverseSetDownwardFractal(container, 0, backward_size);
                TraverseAjustFractal(container, 0, backward_size);
                T_StructLineContainer temp_lines;
                Traverse_GetStuctLines(container, 0, backward_size, temp_lines);
                app_->stock_data_man().AppendStructLinesIfNeccary(ToPeriodType(k_type_), stock_code_, temp_lines);
                int occure_index = app_->wave_man().Traverse_GetWaveLevel1(stock_code_, k_type_, 0, backward_size);
                app_->wave_man().TraverseSetTrendDataTowardRight(stock_code_, k_type_, occure_index);
            }

            if( ret == 1 )
                TraverSetSignale(k_type_, container, 0, DEFAULT_TRAVERSE_LEFT_K_NUM);
            else if( ret == 2 )
            {
                TraverSetSignale(k_type_, container);
                app_->stock_data_man().TraverseGetStuctLines(ToPeriodType(k_type_), stock_code_, 0, container);
            }
            is_need_updated = ret > 0;
            
        }else
        {  
            auto config_info = app_->config_man().contract_info();
            int eldest_date = config_info.eldest_date;  
            int eldest_hhmm = config_info.eldest_hhmm;  
            int latest_date = config_info.latest_date; //20190225; //20190130; //20190103; // order 20190118; //20190117; //20201030;  //20180913; //20180918; //20180921;
            int latest_hhmm = config_info.latest_hhmm; //910;      //1115;     //915;  //2225;   //200; //120; //ps: consider different period type's start hhmm
            LoadBaseStock(config_info.code.c_str(), k_type_, false, MARKET_SH_FUTURES
                , std::make_tuple(eldest_date, eldest_hhmm, latest_date, latest_hhmm));
        }
       /* if( is_only_updated_from_close_price )
            Emit_UpdateKwall(); 
        else */
        if( is_need_updated )
        {
            if( main_win_->is_show_autoforcast() 
                && ( !Equal(cur_k_item.high_price, container.back()->stk_item.high_price) || !Equal(cur_k_item.low_price, container.back()->stk_item.low_price)) )
            {
                HandleAutoForcast_large();
            }
            Emit_UpdateKwall(); 
        }
    //} 
    return container.empty() ? nullptr : std::addressof(container.back()->stk_item);
}

void KLineWall::UpdateIfNecessary(const T_QuoteData &quote_data)
{  
    if( is_resetting_stock_ || draw_action_ != DrawAction::NO_ACTION || main_win_->is_train_mode() 
        || mm_move_flag_ )
        return;

    double quote_price = 0.0;
    bool is_need_updated = false;
    bool is_only_updated_from_close_price = false;
 
    std::lock_guard<std::mutex> locker(painting_mutex_);
     
    //int hhmm = GetKDataTargetStartTime(k_type_, cur_hhmm);
    T_StockHisDataItem cur_k_item;
    T_HisDataItemContainer &container = app_->stock_data_man().GetHisDataContainer(ToPeriodType(k_type_), stock_code_);
    if( !container.empty() )
    {  
            int ret = 0;
#if 1
            cur_k_item = container.back()->stk_item; 
            // ps: quote_data.hhmmss real content is hhmmss, but cur_k_item.hhmmss real content is hhmm 
            int quote_hhmm = quote_data.hhmmss/100;
            int hour = quote_hhmm / 100;
            int minute = quote_hhmm % 100;
            int second = quote_data.hhmmss%100;
            int real_quote_hhmm = quote_hhmm;
            if( second > 0 )
            { 
                if( minute < 59 )
                    minute += 1;
                else
                {
                    if( hour < 23 )
                    {   
                        minute = 0;
                        hour += 1;
                    }else
                    {
                        hour = minute = 0;
                    }
                }
                real_quote_hhmm = hour * 100 + minute;
            }
            if( quote_data.date > cur_k_item.date || quote_data.date == cur_k_item.date && real_quote_hhmm > cur_k_item.hhmmss )
            {
                T_StockHisDataItem *ori_stk_item = app_->stock_data_man().FindEndStockDateItemFromEndContain(ToPeriodType(k_type_), stock_code_);
                if( ori_stk_item )
                    container.back()->stk_item = *ori_stk_item;//restore ori

                int next_k_hhmm = GetRelKHhmmTag(k_type_, real_quote_hhmm);
                int next_k_date = app_->stock_data_man().GetRelKDateTag(ToPeriodType(k_type_), quote_data.date, real_quote_hhmm);
                T_DateRange range = std::make_tuple(cur_k_item.date, cur_k_item.hhmmss, next_k_date, next_k_hhmm);
                auto count_tuple = app_->stock_data_man().AppendPartStockData(ToPeriodType(k_type_), nmarket_, stock_code_, range, is_index_);
#if 0
                if( container.back()->stk_item.hhmmss > next_k_hhmm ) //debug
                { 
                    app_->local_logger().LogLocal(utility::FormatStr("error KLineWall::UpdateIfNecessary k_type:%d container.back()->stk_item.hhmmss:%d > next_k_hhmm:d"
                        , k_type_, container.back()->stk_item.hhmmss, next_k_hhmm));
                }
#endif
                if( std::get<1>(count_tuple) > 0 ) // has appended to back
                {
                    app_->stock_data_man().StoreEndStockDateItem(ToPeriodType(k_type_), stock_code_, container.back()->stk_item);
                    // set new item's high_price and low_price to close_price
                    container.back()->stk_item.high_price = container.back()->stk_item.low_price = container.back()->stk_item.close_price;
                }
                if( std::get<0>(count_tuple) > 0 || std::get<1>(count_tuple) > 0 )
                {
                    ret = 2;
                    if( std::get<0>(count_tuple) > 0 ) // exist insert front
                        auto_forcast_man_.ResetIndexs(stock_code_, k_type_, container);
                }
                
            }else
            {
                //  update c, h, l ----------------
                if( quote_data.price > container.back()->stk_item.high_price )
                {
                    container.back()->stk_item.high_price = quote_data.price;
                    ret = 2;
                }else if( quote_data.price < container.back()->stk_item.low_price )
                {
                    container.back()->stk_item.low_price = quote_data.price;
                    ret = 2;
                }else if( !Equal(quote_data.price, container.back()->stk_item.close_price) )
                {
                    is_only_updated_from_close_price = true;
                }
                container.back()->stk_item.close_price = quote_data.price;
            }
#else //old
            ret = app_->stock_data_man().UpdateOrAppendLatestItemStockData(ToPeriodType(k_type_), nmarket_, stock_code_, quote_price, is_index_);
#endif
            if( ret > 0 )
            { 
                int backward_size = 50;
                backward_size = MIN_VAL(container.size(), backward_size); 
                TraverseClearFractalType(container, 0, backward_size);
                // ps: it's important, cause create fractal needs more indexs than clear fractal, otherwise will lost fractal
                backward_size += 5;
                backward_size = MIN_VAL(container.size(), backward_size); 
                TraverseSetUpwardFractal(container, 0, backward_size);
                TraverseSetDownwardFractal(container, 0, backward_size);
                TraverseAjustFractal(container, 0, backward_size);
            }

            if( ret == 1 )
                TraverSetSignale(k_type_, container, 0, DEFAULT_TRAVERSE_LEFT_K_NUM);
            else if( ret == 2 )
            {
                TraverSetSignale(k_type_, container);
                app_->stock_data_man().TraverseGetStuctLines(ToPeriodType(k_type_), stock_code_, 0, container);
            }
            is_need_updated = ret > 0;
            
        }else
        { 
            auto config_info = app_->config_man().contract_info();
            int eldest_date = config_info.eldest_date;  
            int eldest_hhmm = config_info.eldest_hhmm;  
            int latest_date = config_info.latest_date; //20190225; //20190130; //20190103; // order 20190118; //20190117; //20201030;  //20180913; //20180918; //20180921;
            int latest_hhmm = config_info.latest_hhmm; //910;      //1115;     //915;  //2225;   //200; //120; //ps: consider different period type's start hhmm
            LoadBaseStock(config_info.code.c_str(), k_type_, false, MARKET_SH_FUTURES
                , std::make_tuple(eldest_date, eldest_hhmm, latest_date, latest_hhmm));
        }
        if( is_only_updated_from_close_price )
            Emit_UpdateKwall(); 
        else if( is_need_updated )
        {
            if( main_win_->is_show_autoforcast() 
                && ( !Equal(cur_k_item.high_price, container.back()->stk_item.high_price) || !Equal(cur_k_item.low_price, container.back()->stk_item.low_price)) )
            {
                HandleAutoForcast_large();
            }
            Emit_UpdateKwall(); 
        }
    //}
}
 
const T_StockHisDataItem & KLineWall::CurStockDataItem()
{
    static T_StockHisDataItem no_use_item;

    if( p_hisdata_container_->size() <= 0 || p_hisdata_container_->size() - 1 < k_rend_index_ 
        || k_rend_index_ < 0 )
        return no_use_item;

    return p_hisdata_container_->at(p_hisdata_container_->size() - 1 - k_rend_index_)->stk_item;
}

T_StockHisDataItem* KLineWall::TrainStockDataItem(int r_index)
{
    if( p_hisdata_container_->size() <= 0 || p_hisdata_container_->size() - 1 < r_index 
        || r_index < 0 )
        return nullptr;
     
    return std::addressof(p_hisdata_container_->at(p_hisdata_container_->size() - 1 - r_index)->stk_item);
}

void KLineWall::Set_Cursor(Qt::CursorShape sp)
{
    //qDebug() << " Set_Cursor " << sp << "\n";
    this->setCursor(sp);
}

T_KlineDataItem * KLineWall::GetKLineDataItemByXpos(int x)
{
    int j = k_num_;
    for( auto iter = p_hisdata_container_->rbegin() + k_rend_index_;
        iter != p_hisdata_container_->rend() && j > 0; 
        ++iter, --j)
    { 
        T_KlinePosData &pos_data = iter->get()->kline_posdata(wall_index_);
        if( pos_data.x_left == CST_MAGIC_POINT.x() )
            continue;
        if( (double)x > pos_data.x_left - 0.0001 && (double)x <= pos_data.x_right + 0.0001 )
            return iter->get();
    }
    return nullptr;
}

QPointF KLineWall::GetPointFromKLineDataItems(int x, bool is_get_top)
{
    //ps: dont't need untranslate cause x hadn't been translated : painter.translate(0, ...); //  
    auto p_item = GetKLineDataItemByXpos(x);
    if( p_item ) return is_get_top ? p_item->kline_posdata(wall_index_).top : p_item->kline_posdata(wall_index_).bottom;
    return CST_MAGIC_POINT;
}

T_KlinePosData * KLineWall::GetKLinePosDataByDate(int date, int hhmm)
{
    for( auto iter = p_hisdata_container_->rbegin();
        iter != p_hisdata_container_->rend(); 
        ++iter )
    {   
        if( iter->get()->kline_posdata(wall_index_).date == date && iter->get()->kline_posdata(wall_index_).hhmm == hhmm )
            return std::addressof(iter->get()->kline_posdata(wall_index_));
    }
    return nullptr;
}

void KLineWall::ResetDrawState(DrawAction action)
{ 
    draw_action_ = DrawAction::NO_ACTION;  
    drawing_line_A_ = CST_MAGIC_POINT;
    drawing_line_B_ = CST_MAGIC_POINT;
    drawing_line_C_ = CST_MAGIC_POINT;

    switch(action)
    {
    case DrawAction::NO_ACTION:
        main_win_->UncheckBtnABDownPen();
        main_win_->UncheckBtnABUpPen();
        main_win_->UncheckBtnABCDownPen();
        main_win_->UncheckBtnABUpPen();
        break;
    case DrawAction::DRAWING_FOR_2PDOWN_C:
        main_win_->UncheckBtnABDownPen();
        break;
    case DrawAction::DRAWING_FOR_2PUP_C:
        main_win_->UncheckBtnABUpPen();
        break;
    case DrawAction::DRAWING_FOR_3PDOWN_D:
        main_win_->UncheckBtnABCDownPen();
        break;
    case DrawAction::DRAWING_FOR_3PUP_D:
        main_win_->UncheckBtnABCUpPen();
        break;
    default: assert(false);
    } 
    
}

void KLineWall::SetShowStructLine(bool val)
{
    is_draw_struct_line_ = val;
    update();
}

void KLineWall::SetShowSection(bool val)
{
    is_draw_section_ = val;
    update();
}

//void KLineWall::SetCursorShape(Qt::CursorShape& cursor_shapre)
//{
//    setCursor(cursor_shapre);
//}
 