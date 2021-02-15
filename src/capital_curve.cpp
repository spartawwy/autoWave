#include "capital_curve.h"

#include <cassert>

#include <QDateTime>
#include <QPainter>
#include <qevent.h> 
#include <qdebug.h>
#include "sys_common.h"

CapitalCurve::CapitalCurve(QWidget *parent)
    : QWidget(parent)
    , head_h_percent_(0.1)
    , bottom_h_percent_(0.1)
    , right_w_(40)
    , pre_mm_w_(-1)
    , pre_mm_h_(-1)
    , highestMaxCapital_(250000.0)
    , lowestMinCapital_(50000.0)
    , cp_num_(10)
    , cp_rend_index_(0)
    , mm_move_flag_(false)
    , move_start_point_(0,0)
    , pre_cp_rend_index_(-1)
    , cp_move_temp_index_(-1)
    , area_select_flag_(false)
    , show_cross_line_(false)
    , old_cursor_(this->cursor())
    , span_num_(1)
    , pre_keydown_ms_since_ep_(0)
{
    ui.setupUi(this);
      
    setMouseTracking(true);
    //setFocusPolicy(Qt::StrongFocus);

    QPainter painter(this); 
    old_font_ = painter.font();
}

CapitalCurve::~CapitalCurve()
{

}

void CapitalCurve::Append(CapitalData &data)
{
    char buf[1024] = {'\0'};
    sprintf_s(buf, sizeof(buf), "%08d%04d\0", data.date, data.hhmm);
    auto iter = datetime_map_index_.find(buf);
    if( iter != datetime_map_index_.end() )
    {
        capital_datas_[iter->second] = data;  //update
        return;
    }
    capital_datas_.push_back(data);
    datetime_map_index_[buf] = capital_datas_.size() - 1;

    if( data.capital > highestMaxCapital_ )
        highestMaxCapital_ = data.capital;
    if( data.capital < lowestMinCapital_ )
        lowestMinCapital_ = data.capital;

    UpdatePosDatas();
    update();
}
//
//bool CapitalCurve::eventFilter(QObject *o, QEvent *e)
//{
//    qDebug() << "CapitalCurve::eventFilter \n";
//    return true;
//}

void CapitalCurve::paintEvent(QPaintEvent*)
{
    /*
window left top 
  (0,0)-------------->x
    |
    |
    |
   \|/
    y
    */
    const int mm_h = this->height();
    const int mm_w = this->width();

    auto pos_from_global = mapFromGlobal(QCursor::pos());
    qDebug() << "paintEvent " << pos_from_global.x() << " " << pos_from_global.y() << "\n";    
    QPainter painter(this); 
 
#if 1 
    static auto IsAreaShapeChange = [](CapitalCurve *kwall, int w, int h)->bool
    {
        return w != kwall->pre_mm_w_ || h!= kwall->pre_mm_h_;
    }; 
    const bool is_area_shape_change = IsAreaShapeChange(this, this->width(), this->height());
    if( is_area_shape_change )
    {
        UpdatePosDatas(); 
    }

    
    QPen black_pen; black_pen.setColor(Qt::black); black_pen.setStyle(Qt::SolidLine); black_pen.setWidth(1);
    QPen red_pen; red_pen.setColor(Qt::red); red_pen.setStyle(Qt::SolidLine); red_pen.setWidth(1);
    QPen blue_pen; blue_pen.setColor(Qt::blue); blue_pen.setStyle(Qt::SolidLine); blue_pen.setWidth(1);
    QPen cp_pen; cp_pen.setColor(Qt::red); cp_pen.setStyle(Qt::SolidLine); cp_pen.setWidth(2);
    QPen line_pen; line_pen.setColor(Qt::red); line_pen.setStyle(Qt::SolidLine); line_pen.setWidth(1);
     
    QPen border_pen(blue_pen); border_pen.setWidth(2);

   
    const int head_height = HeadHeight();
    const int bottom_height = BottomHeight();
    const int main_h = mm_h - head_height - bottom_height;

    int trans_x_totoal = 0;
    int trans_y_totoal = 0;
    h_axis_trans_in_paint_cp_ = main_h + head_height; 

    painter.translate(0, h_axis_trans_in_paint_cp_);
    trans_y_totoal += h_axis_trans_in_paint_cp_;
     

    // draw bottom line ------------------
    //painter.translate(0, bttome_height);
    painter.setPen(border_pen);
    painter.drawLine(0, 0, mm_w, 0); 

    // right vertical line | -----------
    const int price_scale_num = 8;
    const int scale_part_h = main_h / price_scale_num; 
    painter.setPen(border_pen);
    painter.drawLine(mm_w - right_w_, 0, mm_w - right_w_, -1 * this->height());

    // vertical' price scale ------------
#if 1
    QPen pen; 
    pen.setColor(Qt::black);
    pen.setStyle(Qt::DotLine); // ............
    painter.setPen(pen); 
    const float price_per_len = (highestMaxCapital_ - lowestMinCapital_) / float(main_h);
    for(int i = 0; i <= price_scale_num; i++)
    {
        int pos_y = (-1) * scale_part_h * i; 
        painter.drawText(mm_w - right_w_, pos_y, QString("%1").arg(lowestMinCapital_ + (price_per_len * scale_part_h * i) ));
        ///painter.drawLine(0, pos_y, mm_w - right_w_, pos_y);
    }
#endif
    int cur_cp_num = MIN_VAL(MAX_VAL(capital_datas_.size() - 1 - cp_rend_index_, 0), cp_num_);
    const int rx = cur_cp_num > 10 ? 1 : 3;
    const int ry = rx; 

    auto old_font_size = old_font_.pointSizeF();
    //auto ck = painter.fontInfo().pointSizeF();

    double font_size = cur_cp_num > 30 ? old_font_size/3 : (cur_cp_num > 20 ? old_font_size/2 : old_font_size); 
    auto new_font = old_font_;
    new_font.setPointSizeF(font_size);
    painter.setFont(new_font);
    //---draw cp--------------------------------
    auto font_metrics = painter.fontMetrics();
    
    int j = cp_num_; 
    for( int i = capital_datas_.size() - 1 - cp_rend_index_; i >= 0 && j > 0; --i, --j )
    { 
        painter.setPen(cp_pen);
        painter.drawEllipse(rel_points_[i], rx, ry);
        char buf[1024] = {'\0'};
#if 0
        sprintf_s(buf, sizeof(buf), "%.1f\0", capital_datas_[i].capital);
        QString str_capital = buf;
        sprintf_s(buf, sizeof(buf), "(%08d\0", capital_datas_[i].date);
        QString str_date = buf;
        sprintf_s(buf, sizeof(buf), "%02d:%02d:00)\0", capital_datas_[i].hhmm/100, capital_datas_[i].hhmm%100);
        QString str_time = buf;
        auto tag = QString::fromLocal8Bit(buf);
#else
        //sprintf_s(buf, sizeof(buf), "%.0f (%08d %02d:%02d:00)\0", capital_datas_[i].capital, capital_datas_[i].date, capital_datas_[i].hhmm/100, capital_datas_[i].hhmm%100);
        sprintf_s(buf, sizeof(buf), "%.0f\0", capital_datas_[i].capital);
        QString str_capital = buf;
        sprintf_s(buf, sizeof(buf), "(%08d %02d:%02d:00)\0", capital_datas_[i].date, capital_datas_[i].hhmm/100, capital_datas_[i].hhmm%100);
        QString str_datetime = buf;
        //auto tag = buf;
#endif
        
#if 0
        painter.drawText(rel_points_[i].rx() - font_metrics.width(str_capital) * 1/2, rel_points_[i].ry() + ry
            , font_metrics.width(str_capital)
            , font_metrics.height()
            , Qt::AlignCenter | Qt::TextSingleLine | Qt::TextWordWrap
            , str_capital);
        painter.drawText(rel_points_[i].rx() - font_metrics.width(str_date) * 1/2, rel_points_[i].ry() + ry + 2*font_metrics.height()
            , font_metrics.width(str_date)
            , font_metrics.height()
            , Qt::AlignCenter | Qt::TextSingleLine | Qt::TextWordWrap
            , str_date);
        painter.drawText(rel_points_[i].rx() - font_metrics.width(str_time) * 1/2, rel_points_[i].ry() + ry + 3*font_metrics.height()
            , font_metrics.width(str_time)
            , font_metrics.height()
            , Qt::AlignCenter | Qt::TextSingleLine | Qt::TextWordWrap
            , str_time);
#else
        
#if 0
        painter.drawLine(QPoint(0,0), QPoint(rel_points_[i].rx(),0));
        painter.drawText(rel_points_[i].rx(), 0, "X");
        painter.drawLine(QPoint(0,0), QPoint(0, rel_points_[i].ry()));
        painter.drawText(0, rel_points_[i].ry(), "Y");
#endif
        if( cur_cp_num < 30 )
        {
            painter.save();

            QTransform trans;
            trans.translate(trans_x_totoal , trans_y_totoal); // fit to paint exists axis 
            trans.translate(rel_points_[i].rx(), rel_points_[i].ry());
            trans.rotate(90); 
            painter.setWorldTransform(trans);
#if 0
            painter.drawLine(QPoint(0,0), QPoint(100,0));
            painter.drawText(100, 0, "X");
            painter.drawLine(QPoint(0,0), QPoint(0,200));
            painter.drawText(0, 200, "Y");
#endif
            painter.setPen(red_pen);
            painter.drawText(6, 0, str_capital);
            painter.setPen(blue_pen);
            painter.drawText(6 + 2*font_metrics.width(str_capital), 0, str_datetime);

            painter.restore();
        }

        //painter.drawText(rel_points_[i].rx(), 3, "date");
#endif
    }

    //---draw cp line --------------------------------
    QString cur_cp_capital_str;
    QString cur_cp_date_str;
    painter.setPen(line_pen);
    j = cp_num_; 
    for( int i = capital_datas_.size() - 1 - cp_rend_index_; i >= 1 && j > 0; --i, --j )
    { 
        if( j == 1 )
            painter.drawLine(rel_points_[i], QPoint(0, rel_points_[i].y()));  // CP line
        else
            painter.drawLine(rel_points_[i], rel_points_[i-1]);  // CP line

        if( pos_from_global.x() > (int)rel_points_[i-1].rx() + rx 
            && pos_from_global.x() <= (int)rel_points_[i].rx() + rx)
        {
            char temp_str[1024];
            sprintf_s(temp_str, "%.0f\0", capital_datas_[i-1].capital);
            cur_cp_capital_str = temp_str;
            sprintf_s(temp_str, "%08d %02d:%02d:00\0", capital_datas_[i-1].date, capital_datas_[i-1].hhmm/100, capital_datas_[i-1].hhmm%100);
            cur_cp_date_str = temp_str;
        }
    }
    if( cur_cp_capital_str.isEmpty() && rel_points_.size() > 0 )
    {
        if( pos_from_global.x() >= (int)rel_points_.back().x() )
        {
            char temp_str[1024];
            sprintf_s(temp_str, "%.0f\0", capital_datas_.back().capital);
            cur_cp_capital_str = temp_str;
            sprintf_s(temp_str, "%08d %02d:%02d:00\0", capital_datas_.back().date, capital_datas_.back().hhmm/100, capital_datas_.back().hhmm%100);
            cur_cp_date_str = temp_str;
        }
    }
    // --------------------------------------
    painter.translate(0, -1 * trans_y_totoal); // translate axis back
    trans_y_totoal = 0; 
    
    // draw cross line --------------------
    if( show_cross_line_ )
    {
        painter.setFont(old_font_);
        //qDebug() << " show_cross_line_ pos x: " << mm_w-right_w_ << " y: " << (float)pos_from_global.y() << "\n";
        painter.setPen(black_pen);   
        // horizontal line 
        painter.drawLine(0, pos_from_global.y(), mm_w-right_w_, pos_from_global.y());
        // vertical line  
        painter.drawLine(pos_from_global.x(), 0, pos_from_global.x(), this->height()); 
        // scale capital 
        painter.drawText( mm_w-right_w_, pos_from_global.y(), QString("%1").arg(lowestMinCapital_ + price_per_len * (h_axis_trans_in_paint_cp_ - pos_from_global.y()) ) );
        // date
        painter.drawText(pos_from_global.x()-k_date_time_str_.size(), h_axis_trans_in_paint_cp_ + font_metrics.height(), cur_cp_date_str);
        
        // cur capital 
        painter.setPen(red_pen);   
        painter.drawText(pos_from_global.x(), pos_from_global.y(), cur_cp_capital_str);
    }
     
    if( area_select_flag_ )
    {
        QRect rect(move_start_point_, pos_from_global);
        painter.fillRect(rect, QBrush(QColor(128, 128, 255, 128))); 
    }

    this->pre_mm_w_ = this->width();
    this->pre_mm_h_ = this->height(); 
#endif
     
}

void CapitalCurve::mouseDoubleClickEvent(QMouseEvent*)
{
    //qDebug() << " x:" << e->pos().x() <<" y:" << e->pos().y() << "\n";
    show_cross_line_ = !show_cross_line_;
    //qDebug() << "show_cross_line:" << show_cross_line_ << "\n";
    //setMouseTracking(show_cross_line_);
    if( show_cross_line_ )
    {
        old_cursor_ = this->cursor();
        this->setCursor(Qt::CrossCursor);
    }
    update();
}

void CapitalCurve::mousePressEvent(QMouseEvent * event)
{
    if( event->buttons() & Qt::LeftButton )
    {
        mm_move_flag_ = true;
        move_start_point_ = event->pos();
        pre_cp_rend_index_ = cp_rend_index_;
        //old_cursor_ = this->cursor();
        //this->setCursor(Qt::OpenHandCursor);
    }else if( event->buttons() & Qt::RightButton )
    {
        mm_move_flag_ = false;
        //this->setCursor(old_cursor_);
        area_select_flag_ = true;
        move_start_point_ = event->pos();
    }
}

void CapitalCurve::mouseReleaseEvent(QMouseEvent * e)
{
    if( mm_move_flag_ )
    {
        mm_move_flag_ = false;
        pre_cp_rend_index_ = cp_rend_index_;
        //this->setCursor(old_cursor_);
    } else if( area_select_flag_ )
    {
        area_select_flag_ = false;
        if( move_start_point_ != e->pos() )
        {  
            // todo: zoom out------------------------------
            double left_x = std::min(move_start_point_.x(),  e->pos().x());
            double right_x = std::max(move_start_point_.x(),  e->pos().x());

            assert( rel_points_.size() > cp_rend_index_ );

            int cp_num_in_area = 0;
            int j = cp_num_; 
             
            int target_cp_rend_index = cp_rend_index_;
            for( int i = rel_points_.size() - 1 - cp_rend_index_; i >= 0 && j >= 0; --i, --j )
            {  
                 
                if( rel_points_[i].x() >= left_x &&  rel_points_[i].x() <= right_x )
                {
                    ++cp_num_in_area;
                    if( cp_num_in_area == 1 )
                    {
                       target_cp_rend_index = rel_points_.size() - 1 - i;
                    } 
                }
                if( rel_points_[i].x() < left_x )
                    break;
            }
            if( cp_rend_index_ != target_cp_rend_index || cp_num_ != cp_num_in_area )
            {
                cp_rend_index_ = target_cp_rend_index;
                cp_num_ = cp_num_in_area;
                UpdateCapitalwallMinMaxPrice();
                UpdatePosDatas(); 
            }
            update();
        }
    }// if( area_select_flag_ )
} 

void CapitalCurve::mouseMoveEvent(QMouseEvent *e)
{
    qDebug() << "CapitalCurve mouseMoveEvent " << e->x() << " " << e->y() << "\n"; 
    if( mm_move_flag_ )
    {
        if( cp_num_ < 1 )
            return;
         
        int atom_cp_width = this->width() / cp_num_;
        if( cp_num_ < 3 )
            atom_cp_width = this->width() / 3;

        if( atom_cp_width == 0 )
            return;

        const int distance = e->pos().x() - move_start_point_.x();
        //qDebug() << "mouse press MoveEvent " << distance << "\n";
        if( distance > 0 ) // drag to right 
        { 
            const int tmp_cp_rend_index = pre_cp_rend_index_ + abs(distance) / atom_cp_width;
            //qDebug() << "mouse press MoveEvent distance " << distance << " pre_cp_rend_index_:" << pre_cp_rend_index_
            //    << "tmp_cp_rend_index:" << tmp_cp_rend_index << "\n";
            if((int)rel_points_.size() < tmp_cp_rend_index + cp_num_ )  
                return; 
            else
            { 
                int tmp_val = rel_points_.size() > 0 ? rel_points_.size() - 1 : 0;
                cp_rend_index_ = std::min(tmp_cp_rend_index, tmp_val);
            }

        }else // drag to left
        {
            const int num = pre_cp_rend_index_ - abs(distance) / atom_cp_width;
            int temp_val = (num > 0 ? num : 0);
            cp_rend_index_ = temp_val;
        }
        if( cp_rend_index_ != cp_move_temp_index_ )
        {
            UpdateCapitalwallMinMaxPrice();
            UpdatePosDatas(); 
            cp_move_temp_index_ = cp_rend_index_;
            update();
        }
    }else if( show_cross_line_ || area_select_flag_ )
    {
        update();
    }
}

void CapitalCurve::keyPressEvent(QKeyEvent *e)
{
    auto key_val = e->key();
    if( (e->modifiers() & Qt::ControlModifier) )
    {
        if( key_val == Qt::Key_Z )
        { 
            e->ignore();
            return;
        }
    }
    switch( key_val )
    {
        case Qt::Key_Up:  //zoom out 
        {
            if( capital_datas_.empty() )
                return;
            if( cp_num_ > 1 )
            {
                cp_num_ --;
                UpdateCapitalwallMinMaxPrice();
                UpdatePosDatas();
                update();
            }
            break;
        }
        case Qt::Key_Down: //zoom in 
        {   
            if( capital_datas_.empty() )
                return;
            __int64 ms_keydown_since_ep = QDateTime::currentDateTime().toMSecsSinceEpoch();
            
            assert((int)capital_datas_.size() > cp_rend_index_);
             
            if( ms_keydown_since_ep - pre_keydown_ms_since_ep_ < 50 )
            {
                if( cp_num_ + span_num_*2 <= (int)capital_datas_.size() )
                    span_num_ *= 2;
                else if( cp_num_ + span_num_ + 3 <= (int)capital_datas_.size() )
                    span_num_ += 3;
                else if( cp_num_ + span_num_ + 1 <= (int)capital_datas_.size() )
                    ++span_num_;
            }else
                span_num_ = 1;

            if( cp_num_ + span_num_ > (int)capital_datas_.size() - cp_rend_index_ )
            {
                cp_num_ = (int)capital_datas_.size();
                cp_rend_index_ = 0;

            } else
            {
                if( cp_rend_index_ > span_num_/2 )
                {
                    if( span_num_/2 == 0 )
                        cp_rend_index_ -= 1;
                    else
                        cp_rend_index_ -= span_num_/2;
                    cp_num_ += span_num_ - span_num_/2;
                }else
                {
                    if( cp_rend_index_ > 0 )
                        --cp_rend_index_;
                    cp_num_ += span_num_;
                }
            }
            pre_keydown_ms_since_ep_ = ms_keydown_since_ep;

            UpdateCapitalwallMinMaxPrice();
            UpdatePosDatas();
            update();
            break;
        } 
    }
}

void CapitalCurve::keyReleaseEvent(QKeyEvent *e)
{
    qDebug() << "keyReleaseEvent \n";

    auto key_val = e->key(); 
    switch( key_val )
    {
    case Qt::Key_Up:  //zoom out 
        {
            break;
        }
    case Qt::Key_Down: //zoom in 
        {  
            qDebug() << "Key_Down keyReleaseEvent \n";
            break;
        } 
    }
}

void CapitalCurve::UpdatePosDatas()
{
    const int mm_h = this->height();
    const int mm_w = this->width();
    const int head_height = HeadHeight();
    const int bottom_height = BottomHeight();
    const int main_h = mm_h - head_height - bottom_height;
    
    if( capital_datas_.empty() || cp_num_ <= 0 )
        return;
    rel_points_.resize(capital_datas_.size());

    //assert(capital_datas_.size() == rel_points_.size());
    //std::lock_guard<std::mutex> locker(painting_mutex_);
    if( !painting_mutex_.try_lock() )
        return;

    // update ----------------------------------------------  
     
    int empty_right_w_ = 0;
    double item_w = double(mm_w - empty_right_w_ - right_w_) / double(cp_num_ + 1) ;
    //  space between k is: item_w / 4;
    double cp_bar_w = item_w * 1 / 2;
    auto right_end = double(mm_w - empty_right_w_ - right_w_) - cp_bar_w;

    // update position data --------------------------------
    int j = cp_num_; 
    //assert( p_hisdata_container_->size() > k_rend_index_ );
     
    for( int i = capital_datas_.size() - 1 - cp_rend_index_; i >=0 && j > 0; --i, --j )
    { 
        CapitalData &capital_data = capital_datas_[i];
         
        auto pos_x = j * item_w + 1;
        auto pos_y = get_capital_y(capital_data.capital, main_h);
        rel_points_[i] = QPoint(pos_x, pos_y);
         
    } // for 
     
    painting_mutex_.unlock();
}


void CapitalCurve::UpdateCapitalwallMinMaxPrice()
{
    if( !painting_mutex_.try_lock() )
        return;

    int j = cp_num_; 
    for( int i = capital_datas_.size() - 1 - cp_rend_index_; i >= 1 && j > 0; --i, --j )
    { 
        if( capital_datas_[i].capital > highestMaxCapital_ )
            highestMaxCapital_ = capital_datas_[i].capital;
        if( capital_datas_[i].capital < lowestMinCapital_ )
            lowestMinCapital_ = capital_datas_[i].capital;
    }
     
    painting_mutex_.unlock();
}