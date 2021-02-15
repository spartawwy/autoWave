/********************************************************************************
** Form generated from reading UI file 'cfgstopprofitdlg.ui'
**
** Created by: Qt User Interface Compiler version 5.2.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_CFGSTOPPROFITDLG_H
#define UI_CFGSTOPPROFITDLG_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QTableView>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_StopProfitLossForm
{
public:
    QTableView *table_v_positions;
    QPushButton *pbtn_save;
    QPushButton *pbtn_close;
    QLabel *lab_status;

    void setupUi(QWidget *StopProfitLossForm)
    {
        if (StopProfitLossForm->objectName().isEmpty())
            StopProfitLossForm->setObjectName(QStringLiteral("StopProfitLossForm"));
        StopProfitLossForm->resize(374, 220);
        table_v_positions = new QTableView(StopProfitLossForm);
        table_v_positions->setObjectName(QStringLiteral("table_v_positions"));
        table_v_positions->setGeometry(QRect(10, 20, 351, 121));
        pbtn_save = new QPushButton(StopProfitLossForm);
        pbtn_save->setObjectName(QStringLiteral("pbtn_save"));
        pbtn_save->setGeometry(QRect(210, 150, 75, 23));
        QFont font;
        font.setPointSize(10);
        pbtn_save->setFont(font);
        pbtn_close = new QPushButton(StopProfitLossForm);
        pbtn_close->setObjectName(QStringLiteral("pbtn_close"));
        pbtn_close->setGeometry(QRect(290, 150, 75, 23));
        pbtn_close->setFont(font);
        lab_status = new QLabel(StopProfitLossForm);
        lab_status->setObjectName(QStringLiteral("lab_status"));
        lab_status->setGeometry(QRect(10, 200, 351, 20));
        lab_status->setFont(font);

        retranslateUi(StopProfitLossForm);

        QMetaObject::connectSlotsByName(StopProfitLossForm);
    } // setupUi

    void retranslateUi(QWidget *StopProfitLossForm)
    {
        StopProfitLossForm->setWindowTitle(QApplication::translate("StopProfitLossForm", "\346\255\242\350\265\242\346\255\242\346\215\237\350\256\276\347\275\256", 0));
        pbtn_save->setText(QApplication::translate("StopProfitLossForm", "\344\277\235\345\255\230", 0));
        pbtn_close->setText(QApplication::translate("StopProfitLossForm", "\345\205\263\351\227\255", 0));
        lab_status->setText(QApplication::translate("StopProfitLossForm", "status", 0));
    } // retranslateUi

};

namespace Ui {
    class StopProfitLossForm: public Ui_StopProfitLossForm {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_CFGSTOPPROFITDLG_H
