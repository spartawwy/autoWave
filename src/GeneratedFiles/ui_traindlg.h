/********************************************************************************
** Form generated from reading UI file 'traindlg.ui'
**
** Created by: Qt User Interface Compiler version 5.2.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_TRAINDLG_H
#define UI_TRAINDLG_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPlainTextEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QScrollBar>
#include <QtWidgets/QSlider>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QTableView>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_TrainDlgForm
{
public:
    QLabel *label_2;
    QTableView *table_view_record;
    QLabel *label_5;
    QWidget *layoutWidget;
    QHBoxLayout *horizontalLayout;
    QPushButton *pbtnStop;
    QPushButton *pbtnStart;
    QPlainTextEdit *plain_te_record;
    QLabel *lab_status;
    QScrollBar *hScrollBar_TrainTimeRange;
    QLabel *lab_start_date;
    QPushButton *pbtnControl;
    QTabWidget *tabwid_main;
    QWidget *tab_trade;
    QLabel *lab_price;
    QLabel *lab_v;
    QDoubleSpinBox *dbspb_price;
    QSpinBox *spb_order_num;
    QLabel *lab_h_line;
    QRadioButton *radio_postion_o;
    QRadioButton *radio_position_c;
    QPushButton *pbtn_buy;
    QPushButton *pbtn_sell;
    QTabWidget *tab_detail;
    QWidget *tab_position;
    QTableView *table_view_position;
    QPushButton *pbtn_market_price_c;
    QPushButton *pbtn_clear_close;
    QTableView *table_view_order_hangon;
    QWidget *tab_fill;
    QTableView *table_view_trades;
    QWidget *tab_condition_order;
    QTableView *table_view_condition;
    QPushButton *pbtn_add_condition;
    QComboBox *cmb_condition_bs;
    QDoubleSpinBox *dbspb_condition_price;
    QLabel *lab_price_2;
    QLabel *lab_price_3;
    QLabel *lab_price_4;
    QComboBox *cmb_conditioin_compare_char;
    QLabel *lab_price_5;
    QSpinBox *spb_condition_qty;
    QSpinBox *spb_cond_stop_profit_tick;
    QSpinBox *spb_cond_stop_loss_tick;
    QCheckBox *checkb_follow_market;
    QWidget *tab_account;
    QLabel *label_8;
    QLineEdit *le_cur_capital;
    QLabel *label_6;
    QLabel *lab_assets;
    QLabel *lab_quote_title;
    QLabel *lab_quote;
    QGroupBox *group_box_capital;
    QLabel *label_4;
    QLabel *label_capital_available;
    QLabel *label_capital;
    QLabel *label_11;
    QLabel *label_12;
    QLabel *label_float_profit;
    QLabel *label_close_profit;
    QLabel *label_13;
    QPushButton *pbtn_config;
    QSlider *vslider_step_speed;
    QPushButton *pbtnRandomStart;

    void setupUi(QWidget *TrainDlgForm)
    {
        if (TrainDlgForm->objectName().isEmpty())
            TrainDlgForm->setObjectName(QStringLiteral("TrainDlgForm"));
        TrainDlgForm->resize(621, 452);
        label_2 = new QLabel(TrainDlgForm);
        label_2->setObjectName(QStringLiteral("label_2"));
        label_2->setGeometry(QRect(0, 30, 61, 21));
        QFont font;
        font.setPointSize(10);
        label_2->setFont(font);
        table_view_record = new QTableView(TrainDlgForm);
        table_view_record->setObjectName(QStringLiteral("table_view_record"));
        table_view_record->setGeometry(QRect(20, 500, 601, 21));
        label_5 = new QLabel(TrainDlgForm);
        label_5->setObjectName(QStringLiteral("label_5"));
        label_5->setGeometry(QRect(10, 451, 61, 21));
        label_5->setFont(font);
        layoutWidget = new QWidget(TrainDlgForm);
        layoutWidget->setObjectName(QStringLiteral("layoutWidget"));
        layoutWidget->setGeometry(QRect(370, 30, 158, 28));
        horizontalLayout = new QHBoxLayout(layoutWidget);
        horizontalLayout->setObjectName(QStringLiteral("horizontalLayout"));
        horizontalLayout->setContentsMargins(0, 0, 0, 0);
        pbtnStop = new QPushButton(layoutWidget);
        pbtnStop->setObjectName(QStringLiteral("pbtnStop"));
        pbtnStop->setFont(font);

        horizontalLayout->addWidget(pbtnStop);

        pbtnStart = new QPushButton(layoutWidget);
        pbtnStart->setObjectName(QStringLiteral("pbtnStart"));
        pbtnStart->setFont(font);

        horizontalLayout->addWidget(pbtnStart);

        plain_te_record = new QPlainTextEdit(TrainDlgForm);
        plain_te_record->setObjectName(QStringLiteral("plain_te_record"));
        plain_te_record->setGeometry(QRect(10, 481, 601, 16));
        lab_status = new QLabel(TrainDlgForm);
        lab_status->setObjectName(QStringLiteral("lab_status"));
        lab_status->setGeometry(QRect(0, 430, 601, 20));
        lab_status->setFont(font);
        hScrollBar_TrainTimeRange = new QScrollBar(TrainDlgForm);
        hScrollBar_TrainTimeRange->setObjectName(QStringLiteral("hScrollBar_TrainTimeRange"));
        hScrollBar_TrainTimeRange->setGeometry(QRect(60, 30, 231, 21));
        hScrollBar_TrainTimeRange->setOrientation(Qt::Horizontal);
        lab_start_date = new QLabel(TrainDlgForm);
        lab_start_date->setObjectName(QStringLiteral("lab_start_date"));
        lab_start_date->setGeometry(QRect(290, 30, 71, 21));
        QFont font1;
        font1.setPointSize(12);
        lab_start_date->setFont(font1);
        lab_start_date->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);
        pbtnControl = new QPushButton(TrainDlgForm);
        pbtnControl->setObjectName(QStringLiteral("pbtnControl"));
        pbtnControl->setGeometry(QRect(450, 0, 75, 23));
        pbtnControl->setFont(font);
        tabwid_main = new QTabWidget(TrainDlgForm);
        tabwid_main->setObjectName(QStringLiteral("tabwid_main"));
        tabwid_main->setGeometry(QRect(0, 100, 621, 321));
        tabwid_main->setTabPosition(QTabWidget::West);
        tab_trade = new QWidget();
        tab_trade->setObjectName(QStringLiteral("tab_trade"));
        lab_price = new QLabel(tab_trade);
        lab_price->setObjectName(QStringLiteral("lab_price"));
        lab_price->setGeometry(QRect(0, 20, 41, 16));
        lab_v = new QLabel(tab_trade);
        lab_v->setObjectName(QStringLiteral("lab_v"));
        lab_v->setGeometry(QRect(0, 50, 41, 16));
        dbspb_price = new QDoubleSpinBox(tab_trade);
        dbspb_price->setObjectName(QStringLiteral("dbspb_price"));
        dbspb_price->setGeometry(QRect(40, 20, 71, 21));
        dbspb_price->setFont(font1);
        dbspb_price->setMinimum(0.1);
        dbspb_price->setMaximum(999.99);
        spb_order_num = new QSpinBox(tab_trade);
        spb_order_num->setObjectName(QStringLiteral("spb_order_num"));
        spb_order_num->setGeometry(QRect(40, 50, 71, 22));
        spb_order_num->setFont(font1);
        spb_order_num->setMinimum(1);
        spb_order_num->setMaximum(10000);
        lab_h_line = new QLabel(tab_trade);
        lab_h_line->setObjectName(QStringLiteral("lab_h_line"));
        lab_h_line->setGeometry(QRect(0, 90, 171, 16));
        radio_postion_o = new QRadioButton(tab_trade);
        radio_postion_o->setObjectName(QStringLiteral("radio_postion_o"));
        radio_postion_o->setGeometry(QRect(0, 110, 41, 16));
        radio_postion_o->setChecked(true);
        radio_position_c = new QRadioButton(tab_trade);
        radio_position_c->setObjectName(QStringLiteral("radio_position_c"));
        radio_position_c->setGeometry(QRect(0, 130, 41, 16));
        pbtn_buy = new QPushButton(tab_trade);
        pbtn_buy->setObjectName(QStringLiteral("pbtn_buy"));
        pbtn_buy->setGeometry(QRect(50, 110, 51, 23));
        pbtn_buy->setFont(font);
        pbtn_sell = new QPushButton(tab_trade);
        pbtn_sell->setObjectName(QStringLiteral("pbtn_sell"));
        pbtn_sell->setGeometry(QRect(110, 110, 51, 23));
        pbtn_sell->setFont(font);
        tab_detail = new QTabWidget(tab_trade);
        tab_detail->setObjectName(QStringLiteral("tab_detail"));
        tab_detail->setGeometry(QRect(170, 0, 421, 311));
        tab_detail->setTabPosition(QTabWidget::South);
        tab_position = new QWidget();
        tab_position->setObjectName(QStringLiteral("tab_position"));
        table_view_position = new QTableView(tab_position);
        table_view_position->setObjectName(QStringLiteral("table_view_position"));
        table_view_position->setGeometry(QRect(0, 30, 401, 121));
        pbtn_market_price_c = new QPushButton(tab_position);
        pbtn_market_price_c->setObjectName(QStringLiteral("pbtn_market_price_c"));
        pbtn_market_price_c->setGeometry(QRect(10, 0, 75, 23));
        pbtn_market_price_c->setFont(font);
        pbtn_clear_close = new QPushButton(tab_position);
        pbtn_clear_close->setObjectName(QStringLiteral("pbtn_clear_close"));
        pbtn_clear_close->setGeometry(QRect(90, 0, 75, 23));
        pbtn_clear_close->setFont(font);
        table_view_order_hangon = new QTableView(tab_position);
        table_view_order_hangon->setObjectName(QStringLiteral("table_view_order_hangon"));
        table_view_order_hangon->setGeometry(QRect(0, 160, 401, 121));
        tab_detail->addTab(tab_position, QString());
        tab_fill = new QWidget();
        tab_fill->setObjectName(QStringLiteral("tab_fill"));
        table_view_trades = new QTableView(tab_fill);
        table_view_trades->setObjectName(QStringLiteral("table_view_trades"));
        table_view_trades->setGeometry(QRect(0, 0, 421, 291));
        tab_detail->addTab(tab_fill, QString());
        tab_condition_order = new QWidget();
        tab_condition_order->setObjectName(QStringLiteral("tab_condition_order"));
        table_view_condition = new QTableView(tab_condition_order);
        table_view_condition->setObjectName(QStringLiteral("table_view_condition"));
        table_view_condition->setGeometry(QRect(0, 10, 401, 151));
        pbtn_add_condition = new QPushButton(tab_condition_order);
        pbtn_add_condition->setObjectName(QStringLiteral("pbtn_add_condition"));
        pbtn_add_condition->setGeometry(QRect(330, 250, 51, 23));
        pbtn_add_condition->setFont(font);
        cmb_condition_bs = new QComboBox(tab_condition_order);
        cmb_condition_bs->setObjectName(QStringLiteral("cmb_condition_bs"));
        cmb_condition_bs->setGeometry(QRect(150, 180, 51, 31));
        dbspb_condition_price = new QDoubleSpinBox(tab_condition_order);
        dbspb_condition_price->setObjectName(QStringLiteral("dbspb_condition_price"));
        dbspb_condition_price->setGeometry(QRect(80, 180, 71, 31));
        dbspb_condition_price->setFont(font1);
        dbspb_condition_price->setMinimum(0.1);
        dbspb_condition_price->setMaximum(999.99);
        lab_price_2 = new QLabel(tab_condition_order);
        lab_price_2->setObjectName(QStringLiteral("lab_price_2"));
        lab_price_2->setGeometry(QRect(0, 180, 31, 31));
        lab_price_3 = new QLabel(tab_condition_order);
        lab_price_3->setObjectName(QStringLiteral("lab_price_3"));
        lab_price_3->setGeometry(QRect(210, 180, 41, 31));
        lab_price_4 = new QLabel(tab_condition_order);
        lab_price_4->setObjectName(QStringLiteral("lab_price_4"));
        lab_price_4->setGeometry(QRect(310, 180, 41, 31));
        cmb_conditioin_compare_char = new QComboBox(tab_condition_order);
        cmb_conditioin_compare_char->setObjectName(QStringLiteral("cmb_conditioin_compare_char"));
        cmb_conditioin_compare_char->setGeometry(QRect(30, 180, 41, 31));
        lab_price_5 = new QLabel(tab_condition_order);
        lab_price_5->setObjectName(QStringLiteral("lab_price_5"));
        lab_price_5->setGeometry(QRect(0, 220, 31, 31));
        spb_condition_qty = new QSpinBox(tab_condition_order);
        spb_condition_qty->setObjectName(QStringLiteral("spb_condition_qty"));
        spb_condition_qty->setGeometry(QRect(40, 220, 71, 31));
        spb_condition_qty->setFont(font1);
        spb_condition_qty->setMinimum(1);
        spb_condition_qty->setMaximum(10000);
        spb_cond_stop_profit_tick = new QSpinBox(tab_condition_order);
        spb_cond_stop_profit_tick->setObjectName(QStringLiteral("spb_cond_stop_profit_tick"));
        spb_cond_stop_profit_tick->setGeometry(QRect(250, 180, 51, 31));
        spb_cond_stop_profit_tick->setFont(font1);
        spb_cond_stop_profit_tick->setMinimum(0);
        spb_cond_stop_profit_tick->setMaximum(10000);
        spb_cond_stop_profit_tick->setValue(0);
        spb_cond_stop_loss_tick = new QSpinBox(tab_condition_order);
        spb_cond_stop_loss_tick->setObjectName(QStringLiteral("spb_cond_stop_loss_tick"));
        spb_cond_stop_loss_tick->setGeometry(QRect(350, 180, 51, 31));
        spb_cond_stop_loss_tick->setFont(font1);
        spb_cond_stop_loss_tick->setMinimum(0);
        spb_cond_stop_loss_tick->setMaximum(10000);
        spb_cond_stop_loss_tick->setValue(0);
        tab_detail->addTab(tab_condition_order, QString());
        checkb_follow_market = new QCheckBox(tab_trade);
        checkb_follow_market->setObjectName(QStringLiteral("checkb_follow_market"));
        checkb_follow_market->setGeometry(QRect(120, 20, 51, 18));
        checkb_follow_market->setChecked(true);
        tabwid_main->addTab(tab_trade, QString());
        tab_account = new QWidget();
        tab_account->setObjectName(QStringLiteral("tab_account"));
        label_8 = new QLabel(tab_account);
        label_8->setObjectName(QStringLiteral("label_8"));
        label_8->setGeometry(QRect(300, 40, 41, 21));
        label_8->setFont(font);
        le_cur_capital = new QLineEdit(tab_account);
        le_cur_capital->setObjectName(QStringLiteral("le_cur_capital"));
        le_cur_capital->setGeometry(QRect(130, 40, 111, 31));
        label_6 = new QLabel(tab_account);
        label_6->setObjectName(QStringLiteral("label_6"));
        label_6->setGeometry(QRect(60, 40, 61, 21));
        label_6->setFont(font);
        lab_assets = new QLabel(tab_account);
        lab_assets->setObjectName(QStringLiteral("lab_assets"));
        lab_assets->setGeometry(QRect(340, 40, 101, 21));
        lab_assets->setFont(font);
        tabwid_main->addTab(tab_account, QString());
        lab_quote_title = new QLabel(TrainDlgForm);
        lab_quote_title->setObjectName(QStringLiteral("lab_quote_title"));
        lab_quote_title->setGeometry(QRect(120, 10, 41, 16));
        lab_quote = new QLabel(TrainDlgForm);
        lab_quote->setObjectName(QStringLiteral("lab_quote"));
        lab_quote->setGeometry(QRect(160, 10, 51, 16));
        QFont font2;
        font2.setBold(true);
        font2.setWeight(75);
        lab_quote->setFont(font2);
        group_box_capital = new QGroupBox(TrainDlgForm);
        group_box_capital->setObjectName(QStringLiteral("group_box_capital"));
        group_box_capital->setGeometry(QRect(10, 60, 581, 41));
        label_4 = new QLabel(group_box_capital);
        label_4->setObjectName(QStringLiteral("label_4"));
        label_4->setGeometry(QRect(10, 10, 51, 16));
        label_capital_available = new QLabel(group_box_capital);
        label_capital_available->setObjectName(QStringLiteral("label_capital_available"));
        label_capital_available->setGeometry(QRect(70, 10, 61, 16));
        label_capital = new QLabel(group_box_capital);
        label_capital->setObjectName(QStringLiteral("label_capital"));
        label_capital->setGeometry(QRect(180, 10, 61, 16));
        label_11 = new QLabel(group_box_capital);
        label_11->setObjectName(QStringLiteral("label_11"));
        label_11->setGeometry(QRect(120, 10, 51, 16));
        label_12 = new QLabel(group_box_capital);
        label_12->setObjectName(QStringLiteral("label_12"));
        label_12->setGeometry(QRect(390, 10, 51, 16));
        label_float_profit = new QLabel(group_box_capital);
        label_float_profit->setObjectName(QStringLiteral("label_float_profit"));
        label_float_profit->setGeometry(QRect(450, 10, 61, 16));
        label_close_profit = new QLabel(group_box_capital);
        label_close_profit->setObjectName(QStringLiteral("label_close_profit"));
        label_close_profit->setGeometry(QRect(310, 10, 61, 16));
        label_13 = new QLabel(group_box_capital);
        label_13->setObjectName(QStringLiteral("label_13"));
        label_13->setGeometry(QRect(250, 10, 51, 16));
        pbtn_config = new QPushButton(TrainDlgForm);
        pbtn_config->setObjectName(QStringLiteral("pbtn_config"));
        pbtn_config->setGeometry(QRect(0, 0, 75, 23));
        pbtn_config->setFont(font);
        vslider_step_speed = new QSlider(TrainDlgForm);
        vslider_step_speed->setObjectName(QStringLiteral("vslider_step_speed"));
        vslider_step_speed->setGeometry(QRect(570, 10, 20, 51));
        vslider_step_speed->setPageStep(100);
        vslider_step_speed->setOrientation(Qt::Vertical);
        vslider_step_speed->setTickInterval(100);
        pbtnRandomStart = new QPushButton(TrainDlgForm);
        pbtnRandomStart->setObjectName(QStringLiteral("pbtnRandomStart"));
        pbtnRandomStart->setGeometry(QRect(370, 0, 75, 23));
        pbtnRandomStart->setFont(font);

        retranslateUi(TrainDlgForm);

        tabwid_main->setCurrentIndex(0);
        tab_detail->setCurrentIndex(2);


        QMetaObject::connectSlotsByName(TrainDlgForm);
    } // setupUi

    void retranslateUi(QWidget *TrainDlgForm)
    {
        TrainDlgForm->setWindowTitle(QApplication::translate("TrainDlgForm", "\350\256\255\347\273\203\346\250\241\345\274\217", 0));
        label_2->setText(QApplication::translate("TrainDlgForm", "\345\274\200\345\247\213\346\227\266\351\227\264:", 0));
        label_5->setText(QApplication::translate("TrainDlgForm", "\350\256\260\345\275\225", 0));
        pbtnStop->setText(QApplication::translate("TrainDlgForm", "\347\273\223\346\235\237", 0));
        pbtnStart->setText(QApplication::translate("TrainDlgForm", "\345\274\200\345\247\213", 0));
        lab_status->setText(QApplication::translate("TrainDlgForm", "status", 0));
        lab_start_date->setText(QApplication::translate("TrainDlgForm", "0", 0));
        pbtnControl->setText(QApplication::translate("TrainDlgForm", "\350\277\220\350\241\214", 0));
        lab_price->setText(QApplication::translate("TrainDlgForm", "\344\273\267\346\240\274:", 0));
        lab_v->setText(QApplication::translate("TrainDlgForm", "\346\225\260\351\207\217:", 0));
        lab_h_line->setText(QApplication::translate("TrainDlgForm", "----------------------------------", 0));
        radio_postion_o->setText(QApplication::translate("TrainDlgForm", "\345\274\200\344\273\223", 0));
        radio_position_c->setText(QApplication::translate("TrainDlgForm", "\345\271\263\344\273\223", 0));
        pbtn_buy->setText(QApplication::translate("TrainDlgForm", "\344\271\260", 0));
        pbtn_sell->setText(QApplication::translate("TrainDlgForm", "\345\215\226", 0));
        pbtn_market_price_c->setText(QApplication::translate("TrainDlgForm", "\345\270\202\344\273\267\345\271\263\344\273\223", 0));
        pbtn_clear_close->setText(QApplication::translate("TrainDlgForm", "\345\205\250\351\203\250\345\271\263\344\273\223", 0));
        tab_detail->setTabText(tab_detail->indexOf(tab_position), QApplication::translate("TrainDlgForm", "\346\214\201\344\273\223/\346\214\202\345\215\225", 0));
        tab_detail->setTabText(tab_detail->indexOf(tab_fill), QApplication::translate("TrainDlgForm", "\346\210\220\344\272\244\350\256\260\345\275\225", 0));
        pbtn_add_condition->setText(QApplication::translate("TrainDlgForm", "\346\267\273\345\212\240", 0));
        lab_price_2->setText(QApplication::translate("TrainDlgForm", "\344\273\267\346\240\274:", 0));
        lab_price_3->setText(QApplication::translate("TrainDlgForm", "\346\255\242\350\265\242\347\202\271:", 0));
        lab_price_4->setText(QApplication::translate("TrainDlgForm", "\346\255\242\346\215\237\347\202\271:", 0));
        lab_price_5->setText(QApplication::translate("TrainDlgForm", "\346\225\260\351\207\217:", 0));
        tab_detail->setTabText(tab_detail->indexOf(tab_condition_order), QApplication::translate("TrainDlgForm", "\346\235\241\344\273\266\345\215\225", 0));
        checkb_follow_market->setText(QApplication::translate("TrainDlgForm", "\350\267\237\345\270\202", 0));
        tabwid_main->setTabText(tabwid_main->indexOf(tab_trade), QApplication::translate("TrainDlgForm", "\344\272\244\346\230\223", 0));
        label_8->setText(QApplication::translate("TrainDlgForm", "\346\235\203\347\233\212:", 0));
        label_6->setText(QApplication::translate("TrainDlgForm", "\345\217\257\347\224\250\350\265\204\351\207\221:", 0));
        lab_assets->setText(QApplication::translate("TrainDlgForm", "12000", 0));
        tabwid_main->setTabText(tabwid_main->indexOf(tab_account), QApplication::translate("TrainDlgForm", "\350\264\246\346\210\267", 0));
        lab_quote_title->setText(QApplication::translate("TrainDlgForm", "\344\273\267\346\240\274:", 0));
        lab_quote->setText(QApplication::translate("TrainDlgForm", "0.0", 0));
        group_box_capital->setTitle(QString());
        label_4->setText(QApplication::translate("TrainDlgForm", "\345\217\257\347\224\250\350\265\204\351\207\221:", 0));
        label_capital_available->setText(QApplication::translate("TrainDlgForm", "0", 0));
        label_capital->setText(QApplication::translate("TrainDlgForm", "0", 0));
        label_11->setText(QApplication::translate("TrainDlgForm", "\345\212\250\346\200\201\346\235\203\347\233\212:", 0));
        label_12->setText(QApplication::translate("TrainDlgForm", "\346\265\256\345\212\250\347\233\210\344\272\217:", 0));
        label_float_profit->setText(QApplication::translate("TrainDlgForm", "0", 0));
        label_close_profit->setText(QApplication::translate("TrainDlgForm", "0", 0));
        label_13->setText(QApplication::translate("TrainDlgForm", "\347\233\210\344\272\217:", 0));
        pbtn_config->setText(QApplication::translate("TrainDlgForm", "\350\256\276\347\275\256", 0));
        pbtnRandomStart->setText(QApplication::translate("TrainDlgForm", "\351\232\217\346\234\272\345\274\200\345\247\213", 0));
    } // retranslateUi

};

namespace Ui {
    class TrainDlgForm: public Ui_TrainDlgForm {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_TRAINDLG_H
