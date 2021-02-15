#ifndef CFG_STOP_PROFITLOSS_SDFDSF_H_
#define CFG_STOP_PROFITLOSS_SDFDSF_H_

//#include <unordered_map>
#include <cassert>
#include <vector>
//#include <list>

//#include <QVector>
#include <QItemDelegate>
#include <QtWidgets/QWidget>
#include <QStandardItemModel>
#include "ui_cfgstopprofitdlg.h"

#include "stkfo_common.h" 
  
class PositionAtom;
class TrainDlg;
class CfgStopProfitLossDlg : public QWidget
{
    Q_OBJECT

public:

    CfgStopProfitLossDlg(TrainDlg *parent);

    void SetContent(std::vector<std::shared_ptr<PositionAtom>> &pos_atoms);

private slots:

    void OnSave();
    void OnClose();

private:

    void SetStatusBar(const QString &content);

    Ui::StopProfitLossForm  ui;
    TrainDlg *parent_;

    QStandardItemModel *data_model_;
};

class ReadOnlyDelegate: public QItemDelegate
{
public:
    ReadOnlyDelegate(QWidget *parent = NULL):QItemDelegate(parent)
    {}

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override //final
    {
        Q_UNUSED(parent)
        Q_UNUSED(option)
        Q_UNUSED(index)
        return NULL;
    }
};
#endif // CFG_STOP_PROFITLOSS_SDFDSF_H_