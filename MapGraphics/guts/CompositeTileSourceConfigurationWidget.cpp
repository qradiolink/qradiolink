#include "CompositeTileSourceConfigurationWidget.h"
#include "ui_CompositeTileSourceConfigurationWidget.h"

#include "MapTileLayerListModel.h"
#include "MapTileSourceDelegate.h"

#include "tileSources/OSMTileSource.h"

#include <QtDebug>
#include <QMenu>

CompositeTileSourceConfigurationWidget::CompositeTileSourceConfigurationWidget(QWeakPointer<CompositeTileSource> composite,
                                                             QWidget *parent) :
    QWidget(parent),
    ui(new Ui::CompositeTileSourceConfigurationWidget),
    _composite(composite)
{
    ui->setupUi(this);

    this->init();
}

CompositeTileSourceConfigurationWidget::~CompositeTileSourceConfigurationWidget()
{
    delete ui;
}

void CompositeTileSourceConfigurationWidget::setComposite(QWeakPointer<CompositeTileSource> nComposite)
{
    _composite = nComposite;
    this->init();
}

//private slot
void CompositeTileSourceConfigurationWidget::handleCurrentSelectionChanged(QModelIndex current, QModelIndex previous)
{
    Q_UNUSED(previous)
    bool enableGUI = current.isValid();

    this->ui->removeSourceButton->setEnabled(enableGUI);
    this->ui->opacitySlider->setEnabled(enableGUI);

    QSharedPointer<CompositeTileSource> strong = _composite.toStrongRef();
    if (strong.isNull())
        return;

    this->ui->moveDownButton->setEnabled(enableGUI &&  (strong->numSources()-1) > current.row());
    this->ui->moveUpButton->setEnabled(enableGUI && 0 < current.row());

    if (enableGUI)
    {
        qreal opacityFloat = strong->getOpacity(current.row());
        this->ui->opacitySlider->setValue(opacityFloat*100);
    }
}

//private slot
void CompositeTileSourceConfigurationWidget::handleCompositeChange()
{
    QItemSelectionModel * selModel = this->ui->listView->selectionModel();
    QModelIndex index = selModel->currentIndex();

    QSharedPointer<CompositeTileSource> strong = _composite.toStrongRef();
    if (strong.isNull())
        return;

    qreal opacityFloat = strong->getOpacity(index.row());
    this->ui->opacitySlider->setValue(opacityFloat*100);
}

//private slot
void CompositeTileSourceConfigurationWidget::addOSMTileLayer()
{
    QSharedPointer<CompositeTileSource> composite = _composite.toStrongRef();
    if (composite.isNull())
        return;

    QSharedPointer<OSMTileSource> source(new OSMTileSource(OSMTileSource::OSMTiles));
    composite->addSourceTop(source);
}

//private slot
void CompositeTileSourceConfigurationWidget::on_removeSourceButton_clicked()
{
    QItemSelectionModel * selModel = this->ui->listView->selectionModel();
    QModelIndex index = selModel->currentIndex();

    QSharedPointer<CompositeTileSource> strong = _composite.toStrongRef();
    if (strong.isNull())
        return;

    strong->removeSource(index.row());

    selModel->clear();
}

//private slot
void CompositeTileSourceConfigurationWidget::on_opacitySlider_valueChanged(int value)
{
    QItemSelectionModel * selModel = this->ui->listView->selectionModel();
    QModelIndex index = selModel->currentIndex();

    if (!index.isValid())
        return;

    QSharedPointer<CompositeTileSource> strong = _composite.toStrongRef();
    if (strong.isNull())
        return;

    qreal opacityFloat = (qreal)value / 100.0;

    strong->setOpacity(index.row(),opacityFloat);
}

//private slot
void CompositeTileSourceConfigurationWidget::on_moveDownButton_clicked()
{
    QItemSelectionModel * selModel = this->ui->listView->selectionModel();
    QModelIndex index = selModel->currentIndex();

    if (!index.isValid())
        return;

    QSharedPointer<CompositeTileSource> strong = _composite.toStrongRef();
    if (strong.isNull())
        return;

    int numberOfLayers = strong->numSources();
    int currentIndex = index.row();
    int desiredIndex = qMin<int>(numberOfLayers-1,currentIndex+1);
    strong->moveSource(currentIndex,desiredIndex);
    selModel->setCurrentIndex(selModel->model()->index(desiredIndex,0),
                              QItemSelectionModel::SelectCurrent);
}

//private slot
void CompositeTileSourceConfigurationWidget::on_moveUpButton_clicked()
{
    QItemSelectionModel * selModel = this->ui->listView->selectionModel();
    QModelIndex index = selModel->currentIndex();

    if (!index.isValid())
        return;

    QSharedPointer<CompositeTileSource> strong = _composite.toStrongRef();
    if (strong.isNull())
        return;

    int currentIndex = index.row();
    int desiredIndex = qMax<int>(0,currentIndex-1);
    strong->moveSource(currentIndex,desiredIndex);
    selModel->setCurrentIndex(selModel->model()->index(desiredIndex,0),
                              QItemSelectionModel::SelectCurrent);
}

//private
void CompositeTileSourceConfigurationWidget::init()
{
    //Create a fancy delegate for custom drawing of our list items
    MapTileSourceDelegate * delegato = new MapTileSourceDelegate(_composite,this);

    //Create the model that watches the CompositeTileSource
    MapTileLayerListModel * model = new MapTileLayerListModel(_composite,this);

    //Set the QListView to watch the model and use the delegate
    QAbstractItemModel * oldModel = this->ui->listView->model();
    QAbstractItemDelegate * oldDelegate = this->ui->listView->itemDelegate();

    this->ui->listView->setModel(model);
    this->ui->listView->setItemDelegate(delegato);

    if (oldModel != 0)
        delete oldModel;
    if (oldDelegate != 0)
        delete oldDelegate;

    QItemSelectionModel * selModel = this->ui->listView->selectionModel();
    connect(selModel,
            SIGNAL(currentChanged(QModelIndex,QModelIndex)),
            this,
            SLOT(handleCurrentSelectionChanged(QModelIndex,QModelIndex)));


    //Build a menu of possible sources for the "add" button
    QMenu * menu = new QMenu(this->ui->addSourceButton);
    menu->addAction("OpenStreetMap Tiles", this, SLOT(addOSMTileLayer()));
    this->ui->addSourceButton->setMenu(menu);
}
