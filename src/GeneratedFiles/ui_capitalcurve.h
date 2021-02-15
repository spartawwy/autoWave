/********************************************************************************
** Form generated from reading UI file 'capitalcurve.ui'
**
** Created by: Qt User Interface Compiler version 5.2.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_CAPITALCURVE_H
#define UI_CAPITALCURVE_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_CapitalCurveClass
{
public:

    void setupUi(QWidget *CapitalCurveClass)
    {
        if (CapitalCurveClass->objectName().isEmpty())
            CapitalCurveClass->setObjectName(QStringLiteral("CapitalCurveClass"));
        CapitalCurveClass->resize(600, 400);

        retranslateUi(CapitalCurveClass);

        QMetaObject::connectSlotsByName(CapitalCurveClass);
    } // setupUi

    void retranslateUi(QWidget *CapitalCurveClass)
    {
        CapitalCurveClass->setWindowTitle(QApplication::translate("CapitalCurveClass", "\350\265\204\351\207\221\346\233\262\347\272\277", 0));
    } // retranslateUi

};

namespace Ui {
    class CapitalCurveClass: public Ui_CapitalCurveClass {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_CAPITALCURVE_H
