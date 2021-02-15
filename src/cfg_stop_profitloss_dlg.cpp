#include "cfg_stop_profitloss_dlg.h"

#include "train_dlg.h"

static const int cst_small_width = 60;

static const int cst_tbv_pos_id = 0;
static const int cst_tbv_pos_bs = 1;
static const int cst_tbv_pos_qty = 2;
static const int cst_tbv_pos_price = 3;
static const int cst_tbv_pos_stop_profit_price = 4;
static const int cst_tbv_pos_stop_loss_price = 5;
static const int cst_tbv_pos_col_count = 6;

CfgStopProfitLossDlg::CfgStopProfitLossDlg(TrainDlg *parent)
    : parent_(parent)
    , data_model_(nullptr)
{
    ui.setupUi(this);
    bool ret = false;
    ret = connect(ui.pbtn_save, SIGNAL(clicked()), this, SLOT(OnSave()));
    assert(ret);
    ret = connect(ui.pbtn_close, SIGNAL(clicked()), this, SLOT(OnClose()));
    assert(ret);

    //ui.table_v_positions->setEditTriggers(QAbstractItemView::NoEditTriggers);
    data_model_ = new QStandardItemModel(0, cst_tbv_pos_col_count, this);
    data_model_->setHorizontalHeaderItem(cst_tbv_pos_id, new QStandardItem(QString::fromLocal8Bit("ID")));
    data_model_->horizontalHeaderItem(cst_tbv_pos_id)->setTextAlignment(Qt::AlignCenter);

    data_model_->setHorizontalHeaderItem(cst_tbv_pos_bs, new QStandardItem(QString::fromLocal8Bit("买卖")));
    data_model_->horizontalHeaderItem(cst_tbv_pos_bs)->setTextAlignment(Qt::AlignCenter);

    data_model_->setHorizontalHeaderItem(cst_tbv_pos_qty, new QStandardItem(QString::fromLocal8Bit("数量")));
    data_model_->horizontalHeaderItem(cst_tbv_pos_qty)->setTextAlignment(Qt::AlignCenter);

    data_model_->setHorizontalHeaderItem(cst_tbv_pos_price, new QStandardItem(QString::fromLocal8Bit("成交")));
    data_model_->horizontalHeaderItem(cst_tbv_pos_price)->setTextAlignment(Qt::AlignCenter);

    data_model_->setHorizontalHeaderItem(cst_tbv_pos_stop_profit_price, new QStandardItem(QString::fromLocal8Bit("止赢")));
    data_model_->horizontalHeaderItem(cst_tbv_pos_stop_profit_price)->setTextAlignment(Qt::AlignCenter);

    data_model_->setHorizontalHeaderItem(cst_tbv_pos_stop_loss_price, new QStandardItem(QString::fromLocal8Bit("止损")));
    data_model_->horizontalHeaderItem(cst_tbv_pos_stop_loss_price)->setTextAlignment(Qt::AlignCenter);

    ui.table_v_positions->setModel(data_model_);
    ui.table_v_positions->setColumnWidth(cst_tbv_pos_id, cst_small_width/2);
    ui.table_v_positions->setColumnWidth(cst_tbv_pos_bs, cst_small_width/2);
    ui.table_v_positions->setColumnWidth(cst_tbv_pos_qty, cst_small_width);
    ui.table_v_positions->setColumnWidth(cst_tbv_pos_price, cst_small_width);
    ui.table_v_positions->setColumnWidth(cst_tbv_pos_stop_profit_price, cst_small_width);
    ui.table_v_positions->setColumnWidth(cst_tbv_pos_stop_loss_price, cst_small_width);

    ReadOnlyDelegate* readOnlyDelegate = new ReadOnlyDelegate();
    ui.table_v_positions->setItemDelegateForColumn(cst_tbv_pos_id, readOnlyDelegate);
    ui.table_v_positions->setItemDelegateForColumn(cst_tbv_pos_qty, readOnlyDelegate);
    ui.table_v_positions->setItemDelegateForColumn(cst_tbv_pos_bs, readOnlyDelegate);
    ui.table_v_positions->setItemDelegateForColumn(cst_tbv_pos_price, readOnlyDelegate);
}

void CfgStopProfitLossDlg::SetContent(std::vector<std::shared_ptr<PositionAtom>> &pos_atoms)
{
    data_model_->removeRows(0,data_model_->rowCount());
    for( unsigned int i = 0; i < pos_atoms.size(); ++i )
    {
        if( pos_atoms[i]->qty_available <= 0 )
            continue;
        data_model_->insertRow(data_model_->rowCount());
        int row_index = data_model_->rowCount() - 1;
        auto item = new QStandardItem(QString::number(pos_atoms[i]->trade_id));
        data_model_->setItem(row_index, cst_tbv_pos_id, item);

        item = new QStandardItem(QString("%1").arg(pos_atoms[i]->is_long ? QString::fromLocal8Bit("买") : QString::fromLocal8Bit("卖")));
        data_model_->setItem(row_index, cst_tbv_pos_bs, item);
        data_model_->item(row_index, cst_tbv_pos_bs)->setData(QVariant(pos_atoms[i]->is_long));

        item = new QStandardItem(QString::number(pos_atoms[i]->qty_available));
        data_model_->setItem(row_index, cst_tbv_pos_qty, item);

        item = new QStandardItem(QString::number(pos_atoms[i]->price));
        data_model_->setItem(row_index, cst_tbv_pos_price, item);

        item = new QStandardItem(QString::number(pos_atoms[i]->stop_profit_price));
        data_model_->setItem(row_index, cst_tbv_pos_stop_profit_price, item);

        item = new QStandardItem(QString::number(pos_atoms[i]->stop_loss_price));
        data_model_->setItem(row_index, cst_tbv_pos_stop_loss_price, item);
    }
}

void CfgStopProfitLossDlg::OnSave()
{ 
    static auto is_legal_stop_price = [](double quote_price, double price , bool is_stop_prifit, bool is_long)->bool
    {
        if( is_stop_prifit )
        {
            return is_long ? (price > quote_price) : (price < quote_price);
        }else
        {
            return is_long ? (price < quote_price) : (price > quote_price);
        }
    };
    auto check_contents = [](QStandardItemModel &model, int row, bool is_stop_profit, bool is_long, double cur_quote, double &ret_price, QString &ret_info)->bool
    {
        QString stop_price_str = model.item(row, is_stop_profit ? cst_tbv_pos_stop_profit_price : cst_tbv_pos_stop_loss_price)->text().trimmed();
        double stop_price = 0.0; 
        if( !TransToDouble(stop_price_str.toLocal8Bit().data(), stop_price) )
        { 
            ret_info = QString::fromLocal8Bit("价格为非法字符!");
            return false;
        }
        if( !is_legal_stop_price(cur_quote, stop_price, is_stop_profit, is_long) )
        {
            ret_info = is_stop_profit ? QString::fromLocal8Bit( "止赢价不合理!") : QString::fromLocal8Bit( "止损价不合理!");
            return false;
        } 
        ret_price = stop_price;
        return true;
    };

    double cur_quote = parent_->cur_quote();
    std::vector<PositionAtom> pos_atoms;
    for( int i = 0; i < data_model_->rowCount(); ++i )
    {
        //auto check_contents = [](QStandardItemModel &model, int row, bool is_stop_profit, bool is_long, double cur_quote, double &ret_price, QString &ret_info)->bool
        QString alarm_info;
        double stop_profit_price = 0.0;
        if( !check_contents(*data_model_, i, true, data_model_->item(i, cst_tbv_pos_bs)->data().toBool(), cur_quote, stop_profit_price, alarm_info) )
        {
            auto index = data_model_->item(i, cst_tbv_pos_stop_profit_price)->index();
            ui.table_v_positions->setCurrentIndex(index);
            SetStatusBar(alarm_info);
            return;
        }
        double stop_loss_price = 0.0;
        if( !check_contents(*data_model_, i, false, data_model_->item(i, cst_tbv_pos_bs)->data().toBool(), cur_quote, stop_loss_price, alarm_info) )
        {
            auto index = data_model_->item(i, cst_tbv_pos_stop_loss_price)->index();
            //ui.table_v_positions->indexWidget(index)->setFocus(); // cause crash becuse there is no widget
            ui.table_v_positions->setCurrentIndex(index);
            SetStatusBar(alarm_info);
            return;
        }
        
        PositionAtom atom;
        atom.trade_id = data_model_->item(i, cst_tbv_pos_id)->text().toInt(); 
        atom.qty_available = data_model_->item(i, cst_tbv_pos_qty)->text().toInt(); 
        atom.stop_profit_price = stop_profit_price;
        atom.stop_loss_price = stop_loss_price;
        pos_atoms.push_back(atom);

    }
    if( !pos_atoms.empty() )
        parent_->SaveStopProfitLoss(pos_atoms);
    SetStatusBar("");
    this->hide();
}

void CfgStopProfitLossDlg::OnClose()
{
    SetStatusBar("");
    this->hide();
}

void CfgStopProfitLossDlg::SetStatusBar(const QString &content)
{
    ui.lab_status->setText(content);
}