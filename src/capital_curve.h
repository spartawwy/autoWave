#ifndef CAPITAL_CURVE_H
#define CAPITAL_CURVE_H

#include <vector>
#include <mutex>
#include <unordered_map>

#include <QtWidgets/QWidget>
#include "ui_capitalcurve.h"

struct CapitalData
{
    explicit CapitalData(double capital_p, int date_p, int hhmm_p) : capital(capital_p), date(date_p), hhmm(hhmm_p){}
    CapitalData() : capital(0.0), date(0), hhmm(0){}
    CapitalData(const CapitalData &lh) : capital(lh.capital), date(lh.date), hhmm(lh.hhmm){}
    CapitalData& operator = (const CapitalData &lh)
    {
        if( this == &lh )
            return *this;
        capital = lh.capital; date = lh.date; hhmm = lh.hhmm;
        return *this;
    }
    double capital;
    int date;
    int hhmm;
};


class CapitalCurve : public QWidget
{
    Q_OBJECT

public:
    CapitalCurve(QWidget *parent = 0);
    ~CapitalCurve();

    void Append(CapitalData &data);

protected:
    //virtual bool eventFilter(QObject *o, QEvent *e) override;

    virtual void paintEvent(QPaintEvent*) override;
    virtual void mouseDoubleClickEvent(QMouseEvent*) override;
    virtual void mousePressEvent(QMouseEvent * event) override;
    virtual void mouseReleaseEvent(QMouseEvent * e) override;
     
    virtual void mouseMoveEvent(QMouseEvent *e) override;
    virtual void keyPressEvent(QKeyEvent *e) override;
    virtual void keyReleaseEvent(QKeyEvent *e) override;

    void UpdatePosDatas();

    int HeadHeight() { return int(height() * head_h_percent_); }
    int BottomHeight() { return int(height() * bottom_h_percent_); }

    double get_capital_y(double price, int mm_h)
    {  
        return -1 * (price - lowestMinCapital_)/(highestMaxCapital_ - lowestMinCapital_) * mm_h;
    }
    void UpdateCapitalwallMinMaxPrice();

private:
    Ui::CapitalCurveClass ui;

    double head_h_percent_;
    double bottom_h_percent_;
    int right_w_;

    int pre_mm_w_;
    int pre_mm_h_;
    int h_axis_trans_in_paint_cp_;

    double highestMaxCapital_;
    double lowestMinCapital_;

    int cp_num_;
    int cp_rend_index_;

    bool mm_move_flag_;
    QPoint move_start_point_;
    int pre_cp_rend_index_;
    int cp_move_temp_index_;
    bool area_select_flag_;

    bool show_cross_line_;

    std::mutex  painting_mutex_;

    std::unordered_map<std::string, int> datetime_map_index_;
    std::vector<CapitalData>  capital_datas_;
    std::vector<QPoint>       rel_points_;

    std::string k_date_time_str_;
    QCursor old_cursor_;
    QFont old_font_;

    //--------------for keypress zoom in out-----
    int span_num_;
    __int64 pre_keydown_ms_since_ep_;
};

#endif // CAPITAL_CURVE_H
