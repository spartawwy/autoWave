/********************************************************************************
** Form generated from reading UI file 'cfgtrain.ui'
**
** Created by: Qt User Interface Compiler version 5.2.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_CFGTRAIN_H
#define UI_CFGTRAIN_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_CfgTrainForm
{
public:
    QLabel *label;
    QDoubleSpinBox *dbspbBegCapital;
    QDoubleSpinBox *dbspbFeeOpen;
    QLabel *label_9;
    QLabel *label_3;
    QDoubleSpinBox *dbspbFeeOpen_2;
    QGroupBox *groupBox;
    QCheckBox *chbox_auto_stop_loss;
    QSpinBox *spb_stop_loss;
    QCheckBox *chbox_auto_stop_profit;
    QSpinBox *spb_stop_profit;
    QPushButton *pbtn_save;
    QPushButton *pbtn_cancel;

    void setupUi(QWidget *CfgTrainForm)
    {
        if (CfgTrainForm->objectName().isEmpty())
            CfgTrainForm->setObjectName(QStringLiteral("CfgTrainForm"));
        CfgTrainForm->resize(396, 329);
        label = new QLabel(CfgTrainForm);
        label->setObjectName(QStringLiteral("label"));
        label->setGeometry(QRect(30, 10, 61, 21));
        QFont font;
        font.setPointSize(10);
        label->setFont(font);
        dbspbBegCapital = new QDoubleSpinBox(CfgTrainForm);
        dbspbBegCapital->setObjectName(QStringLiteral("dbspbBegCapital"));
        dbspbBegCapital->setGeometry(QRect(100, 10, 111, 31));
        dbspbBegCapital->setMinimum(40000);
        dbspbBegCapital->setMaximum(1e+07);
        dbspbBegCapital->setValue(50000);
        dbspbFeeOpen = new QDoubleSpinBox(CfgTrainForm);
        dbspbFeeOpen->setObjectName(QStringLiteral("dbspbFeeOpen"));
        dbspbFeeOpen->setEnabled(false);
        dbspbFeeOpen->setGeometry(QRect(100, 60, 81, 31));
        dbspbFeeOpen->setDecimals(2);
        dbspbFeeOpen->setMaximum(500);
        dbspbFeeOpen->setSingleStep(1);
        dbspbFeeOpen->setValue(25);
        label_9 = new QLabel(CfgTrainForm);
        label_9->setObjectName(QStringLiteral("label_9"));
        label_9->setGeometry(QRect(210, 60, 71, 21));
        label_9->setFont(font);
        label_3 = new QLabel(CfgTrainForm);
        label_3->setObjectName(QStringLiteral("label_3"));
        label_3->setGeometry(QRect(20, 60, 71, 21));
        label_3->setFont(font);
        dbspbFeeOpen_2 = new QDoubleSpinBox(CfgTrainForm);
        dbspbFeeOpen_2->setObjectName(QStringLiteral("dbspbFeeOpen_2"));
        dbspbFeeOpen_2->setEnabled(false);
        dbspbFeeOpen_2->setGeometry(QRect(290, 60, 81, 31));
        dbspbFeeOpen_2->setDecimals(2);
        dbspbFeeOpen_2->setMaximum(500);
        dbspbFeeOpen_2->setSingleStep(1);
        dbspbFeeOpen_2->setValue(0);
        groupBox = new QGroupBox(CfgTrainForm);
        groupBox->setObjectName(QStringLiteral("groupBox"));
        groupBox->setGeometry(QRect(10, 170, 371, 101));
        chbox_auto_stop_loss = new QCheckBox(groupBox);
        chbox_auto_stop_loss->setObjectName(QStringLiteral("chbox_auto_stop_loss"));
        chbox_auto_stop_loss->setGeometry(QRect(20, 20, 71, 18));
        spb_stop_loss = new QSpinBox(groupBox);
        spb_stop_loss->setObjectName(QStringLiteral("spb_stop_loss"));
        spb_stop_loss->setGeometry(QRect(100, 20, 42, 22));
        spb_stop_loss->setMinimum(1);
        spb_stop_loss->setValue(8);
        chbox_auto_stop_profit = new QCheckBox(groupBox);
        chbox_auto_stop_profit->setObjectName(QStringLiteral("chbox_auto_stop_profit"));
        chbox_auto_stop_profit->setGeometry(QRect(20, 60, 71, 18));
        spb_stop_profit = new QSpinBox(groupBox);
        spb_stop_profit->setObjectName(QStringLiteral("spb_stop_profit"));
        spb_stop_profit->setGeometry(QRect(100, 60, 42, 22));
        spb_stop_profit->setMinimum(1);
        spb_stop_profit->setValue(8);
        pbtn_save = new QPushButton(CfgTrainForm);
        pbtn_save->setObjectName(QStringLiteral("pbtn_save"));
        pbtn_save->setGeometry(QRect(210, 290, 75, 23));
        pbtn_cancel = new QPushButton(CfgTrainForm);
        pbtn_cancel->setObjectName(QStringLiteral("pbtn_cancel"));
        pbtn_cancel->setGeometry(QRect(300, 290, 75, 23));
        label->raise();
        dbspbFeeOpen->raise();
        label_9->raise();
        label_3->raise();
        dbspbFeeOpen_2->raise();
        groupBox->raise();
        pbtn_cancel->raise();
        pbtn_save->raise();
        dbspbBegCapital->raise();

        retranslateUi(CfgTrainForm);

        QMetaObject::connectSlotsByName(CfgTrainForm);
    } // setupUi

    void retranslateUi(QWidget *CfgTrainForm)
    {
        CfgTrainForm->setWindowTitle(QApplication::translate("CfgTrainForm", "Form", 0));
        label->setText(QApplication::translate("CfgTrainForm", "\345\210\235\345\247\213\350\265\204\351\207\221:", 0));
        label_9->setText(QApplication::translate("CfgTrainForm", "\345\271\263\344\273\223\346\211\213\347\273\255\350\264\271:", 0));
        label_3->setText(QApplication::translate("CfgTrainForm", "\345\274\200\344\273\223\346\211\213\347\273\255\350\264\271:", 0));
        groupBox->setTitle(QString());
        chbox_auto_stop_loss->setText(QApplication::translate("CfgTrainForm", "\350\207\252\345\212\250\346\255\242\346\215\237", 0));
        chbox_auto_stop_profit->setText(QApplication::translate("CfgTrainForm", "\350\207\252\345\212\250\346\255\242\347\233\210", 0));
        pbtn_save->setText(QApplication::translate("CfgTrainForm", "\344\277\235\345\255\230", 0));
        pbtn_cancel->setText(QApplication::translate("CfgTrainForm", "\345\217\226\346\266\210", 0));
    } // retranslateUi

};

namespace Ui {
    class CfgTrainForm: public Ui_CfgTrainForm {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_CFGTRAIN_H
