#include "aprscontroller.h"

const qreal PI = 3.14159265358979323846;

AprsController::AprsController(Settings *settings, Ui::MainWindow *wui, QObject *parent) : QObject(parent)
{
    ui = wui;
    _settings = settings;
    _aprs = new Aprs(settings);
    connectToAPRS();
    _zoom_aprs_filter_distance = true;
    QWidget *widget = reinterpret_cast<QWidget*>(parent);
    _map_scene = new MapGraphicsScene(widget);
    _map_view = new MapGraphicsView(_map_scene, widget);
    ui->mapFrame->layout()->addWidget(_map_view);

    QSharedPointer<OSMTileSource> osmTiles(new OSMTileSource(OSMTileSource::OSMTiles), &QObject::deleteLater);
    //QSharedPointer<OSMTileSource> aerialTiles(new OSMTileSource(OSMTileSource::GoogleSatTiles), &QObject::deleteLater);
    QSharedPointer<GridTileSource> gridTiles(new GridTileSource(), &QObject::deleteLater);
    QSharedPointer<CompositeTileSource> composite(new CompositeTileSource(), &QObject::deleteLater);
    composite->addSourceBottom(osmTiles);
    //composite->addSourceBottom(aerialTiles);
    composite->addSourceTop(gridTiles);
    _map_view->setTileSource(composite);


    CompositeTileSourceConfigurationWidget * tileConfigWidget = new CompositeTileSourceConfigurationWidget(composite.toWeakRef(),
                                                                                         ui->dockFrame);
    ui->dockFrame->layout()->addWidget(tileConfigWidget);


    //this->ui->menuWindow->addAction(this->ui->dockWidget->toggleViewAction());

    //this->ui->menuWindow->addAction(this->ui->dockWidget3->toggleViewAction());
    //this->ui->dockWidget->toggleViewAction()->setText("&Layers");
    //this->ui->dockWidget->toggleViewAction()->setText("&Data");
    //this->ui->dockWidget3->toggleViewAction()->setText("&Toolbox");

    //QObject::connect(_map_view,SIGNAL(map_clicked(QPointF)),this,SLOT(mapClick(QPointF)));
    //QObject::connect(_map_view,SIGNAL(mouse_moved(QPointF)),this,SLOT(getMouseCoord(QPointF)));
    QObject::connect(_map_view,SIGNAL(zoomLevelChanged(quint8)),this,SLOT(setMapItems(quint8)));
    QObject::connect(_map_view,SIGNAL(zoomLevelChanged(quint8)),this,SLOT(newAPRSquery(quint8)));
}

AprsController::~AprsController()
{
    delete _aprs;
    for(int i=0;i<_aprs_lines.size();i++)
    {
        delete _aprs_lines.at(i);
    }
    _aprs_lines.clear();
    for(int i=0;i<_aprs_stations.size();i++)
    {
        delete _aprs_stations.at(i);
    }
    _aprs_stations.clear();
}

void AprsController::connectToAPRS()
{
    QObject::connect(_aprs,SIGNAL(aprsData(AprsStation*)),this,SLOT(processAPRSData(AprsStation*)));
    QObject::connect(_aprs,SIGNAL(rawAprsData(QString)),this,SLOT(processRawAPRSData(QString)));
}

void AprsController::activateAPRS(bool active)
{
    if(!active)
    {
        changeAPRSTimeFilter(0);
        if(_aprs)
            _aprs->disconnectAPRS();
    }
    else
    {
        changeAPRSTimeFilter(3600);
        if(_aprs)
            _aprs->connectToAPRS();
        else
            this->connectToAPRS();
    }
}


void AprsController::filterPrefixAPRS()
{

    QString prefix = "";//ui->callsignFilterEdit->text();
    _zoom_aprs_filter_distance = false;
    this->clearAPRS();
    if(_aprs!=NULL)
        _aprs->filterPrefix(prefix);
    QVector<AprsStation *> aprs_stations = _aprs_stations;//_db->filter_aprs_station(prefix);
    for (int i=0;i<aprs_stations.size();++i)
    {
        AprsStation *st = aprs_stations.at(i);
        QString callsign_text;
        bool mobile = false;
        QRegExp re(";([^*]+)\\*");
        if(re.indexIn(st->payload)!=-1)
        {
            callsign_text = re.cap(1);
        }
        else
        {
            callsign_text = st->callsign;
        }
        if(st->payload.startsWith('=') || st->payload.startsWith('/')
                || st->payload.startsWith('@') || st->payload.startsWith('!'))
            mobile= true;
        QString filename = ":/aprs/res/aprs_icons/slice_";
        QString icon;
        QPointF pos = QPointF(st->longitude,st->latitude);
        int zoom = _map_view->zoomLevel();
        QPointF xypos = convertToXY(pos, zoom);

        QVector<AprsStation *> related_stations = _aprs_stations;//_db->similar_stations(st->callsign, st->time_seen);
        if(related_stations.size()>1 && mobile)
        {
            icon = "15_0";

            AprsStation *next = related_stations[1];
            QPointF next_pos = QPointF(next->longitude,next->latitude);

            QPointF next_xypos = convertToXY(next_pos, zoom);
            QLineF progress_line(next_xypos,xypos);

            QColor colour(30,169,255,254);
            QBrush brush(colour);

            QPen pen(brush, 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
            QGraphicsLineItem *line1 = _map_view->_childView->scene()->addLine(progress_line,pen);
            _aprs_lines.push_back(line1);
            draw_lines lines;
            lines.push_back(pos);
            lines.push_back(next_pos);
            _moving_stations.insertMulti(st->callsign,lines);
        }
        else
        {
            icon = st->getImage();
        }
        //_aprs_lines.insert(st->callsign,lines);
        //icon = st->getImage();
        filename.append(icon).append(".png");
        QPixmap pixmap(filename);
        pixmap = pixmap.scaled(16,16);
        AprsPixmapItem *pic = new AprsPixmapItem(pixmap);
        pic->setAcceptHoverEvents(true);

        _map_view->_childView->scene()->addItem(pic);

        pic->setMessage(st->callsign,st->via,st->message);
        pic->setPosition(xypos);
        pic->setOffset(xypos - QPoint(7,25));
        AprsIcon ic;
        ic.position = pos;
        ic.icon = icon;
        _map_aprs.insert(pic, ic);

        if(!(related_stations.size()>1) || !mobile)
        {
            QGraphicsTextItem * callsign = new QGraphicsTextItem;
            callsign->setPos(xypos - QPoint(0,16));
            callsign->setPlainText(callsign_text);
            _map_view->_childView->scene()->addItem(callsign);
            _map_aprs_text.insert(callsign,pos);
        }

        delete st;
    }
    aprs_stations.clear();
}

void AprsController::clearFilterPrefixAPRS()
{
    //ui->callsignFilterEdit->clear();
    _zoom_aprs_filter_distance = true;
    this->clearAPRS();
    this->restoreMapState();
}

void AprsController::clearAPRS()
{
    QMapIterator<AprsPixmapItem *, AprsIcon> i(_map_aprs);
    while(i.hasNext())
    {
        i.next();
        delete i.key();
    }
    _map_aprs.clear();
    QMapIterator<QGraphicsTextItem *, QPointF> j(_map_aprs_text);
    while(j.hasNext())
    {
        j.next();
        delete j.key();
    }
    _map_aprs_text.clear();

    for (int i=0;i<_aprs_lines.size();++i)
    {

        _map_view->_childView->scene()->removeItem(_aprs_lines.at(i));
        delete _aprs_lines.at(i);
    }
    _aprs_lines.clear();
}

void AprsController::changeAPRSTimeFilter(int hours)
{
    this->clearAPRS();

    int time_now = QDateTime::currentDateTime().toTime_t();

    QVector<AprsStation *> filtered_stations = _aprs_stations;//_db->filter_aprs_stations(time_now - (hours*3600));
    for(int k=0;k<filtered_stations.size();++k)
    {
        AprsStation *st = filtered_stations[k];
        bool replace_icon = false;
        bool mobile = false;
        if(st->payload.startsWith('=') || st->payload.startsWith('/'))
            mobile= true;
        QVector<AprsStation *> related_stations = _aprs_stations;//_db->similar_stations(st->callsign, st->time_seen);
        if(related_stations.size()>1 && mobile)
        {
            replace_icon = true;
        }

        QPointF pos(st->longitude,st->latitude);
        double zoom = _map_view->zoomLevel();
        QPointF xypos = convertToXY(pos, zoom);
        QString filename = ":/aprs/res/aprs_icons/slice_";
        AprsIcon ic;
        QString icon;
        if(replace_icon)
        {
            icon = "15_0";

            AprsStation *next = related_stations[1];
            QPointF next_pos = QPointF(next->longitude,next->latitude);

            QPointF next_xypos = convertToXY(next_pos, zoom);
            QLineF progress_line(next_xypos,xypos);

            QColor colour(30,169,255,254);
            QBrush brush(colour);

            QPen pen(brush, 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
            QGraphicsLineItem *line1 = _map_view->_childView->scene()->addLine(progress_line,pen);
            _aprs_lines.push_back(line1);
            draw_lines lines;
            lines.push_back(pos);
            lines.push_back(next_pos);
            _moving_stations.insertMulti(st->callsign,lines);
        }
        else
        {
            icon = st->getImage();
        }


        ic.icon = icon;
        ic.position = pos;
        filename.append(icon).append(".png");
        QPixmap pixmap(filename);
        pixmap = pixmap.scaled(16,16);
        AprsPixmapItem *img = new AprsPixmapItem(pixmap);
        img->setAcceptHoverEvents(true);

        _map_view->_childView->scene()->addItem(img);

        img->setMessage(st->callsign,st->via,st->message);
        img->setPosition(xypos);
        img->setOffset(xypos - QPoint(8,8));
        _map_aprs.insert(img, ic);

        QString callsign_text;
        QRegExp re(";([^*]+)\\*");
        //QRegularExpressionMatch match = re.match(st->payload);
        if(re.indexIn(st->payload)!=-1)
        {
            callsign_text = re.cap(1);
        }
        else
        {
            callsign_text = st->callsign;
        }
        if(!replace_icon)
        {
            QGraphicsTextItem * callsign = new QGraphicsTextItem;
            callsign->setPos(xypos - QPoint(0,16));
            callsign->setPlainText(callsign_text);

            _map_view->_childView->scene()->addItem(callsign);
            _map_aprs_text.insert(callsign,pos);
        }
        delete st;
    }
    filtered_stations.clear();

}

void AprsController::newAPRSquery(quint8 zoom)
{
    if(!_aprs || !_zoom_aprs_filter_distance)
        return;
    QPointF cursor_pos = _map_view->_childView->mapToScene(_map_view->_childView->mapFromGlobal(QCursor::pos()));
    QPointF pos = convertToLL(cursor_pos, zoom);
    int range = _settings->aprs_filter_range;
    _aprs->setFilter(pos, range);
}

void AprsController::processRawAPRSData(QString data)
{
    ui->aprsLogTextEdit->append(data);
}

void AprsController::processAPRSData(AprsStation *st)
{
    double zoom = _map_view->zoomLevel();

    QPointF pos(st->longitude,st->latitude);
    QPointF xypos = convertToXY(pos, zoom);

    bool replace_icon = false;
    bool mobile = false;
    if(st->payload.startsWith('=') || st->payload.startsWith('/')
            || st->payload.startsWith('@') || st->payload.startsWith('!'))
        mobile= true;
    QVector<AprsStation *> older_pos = _aprs_stations;//_db->older_positions(st->callsign, st->time_seen);
    if(older_pos.size()>0 && mobile)
    {
        replace_icon = true;
        AprsStation *oldst = older_pos.at(0);
        QPointF oldpos(oldst->longitude,oldst->latitude);
        QPointF oldxypos = convertToXY(oldpos, zoom);
        QLineF progress_line(xypos,oldxypos);

        QColor colour(30,169,255,254);
        QBrush brush(colour);

        QPen pen(brush, 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
        QGraphicsLineItem *line1 = _map_view->_childView->scene()->addLine(progress_line,pen);
        _aprs_lines.push_back(line1);
        draw_lines lines;
        lines.push_back(oldpos);
        lines.push_back(pos);
        _moving_stations.insertMulti(st->callsign,lines);

    }
    QMapIterator<AprsPixmapItem*, AprsIcon> i(_map_aprs);
    while(i.hasNext())
    {
        i.next();
        AprsIcon oldic = i.value();
        QPointF oldpos= oldic.position;
        if( (fabs(oldpos.rx() - st->longitude) <= 0.001) && (fabs(oldpos.ry() - st->latitude) <=0.001 )
                && (oldic.icon==st->getImage()) )
        {
            return;
        }
        AprsPixmapItem *pm = i.key();
        if(replace_icon && (pm->_callsign == st->callsign) && (oldic.icon!="15_0"))
        {
            QString filename1 = ":/aprs/res/aprs_icons/slice_";
            _map_view->_childView->scene()->removeItem(i.key());
            AprsIcon newic;
            QString newicon = "15_0";
            newic.icon = newicon;
            newic.position = oldpos;
            newic.callsign = st->callsign;
            newic.time_seen = st->time_seen;
            filename1.append(newicon).append(".png");
            QPixmap newpixmap(filename1);
            newpixmap = newpixmap.scaled(16,16);
            AprsPixmapItem *newimg = new AprsPixmapItem(newpixmap);
            newimg->setAcceptHoverEvents(true);

            _map_view->_childView->scene()->addItem(newimg);
            QPointF oldxypos = convertToXY(oldpos, zoom);
            newimg->setMessage(st->callsign,st->via,st->message);
            newimg->setPosition(oldxypos);
            newimg->setOffset(oldxypos - QPoint(8,8));
            delete i.key();
            _map_aprs.remove(i.key());
            _map_aprs.insert(newimg, newic);

        }

    }

    QString filename = ":/aprs/res/aprs_icons/slice_";
    AprsIcon ic;
    QString icon = st->getImage();
    ic.icon = icon;
    ic.position = pos;
    ic.callsign = st->callsign;
    ic.time_seen = st->time_seen;
    filename.append(icon).append(".png");
    QPixmap pixmap(filename);
    pixmap = pixmap.scaled(16,16);
    AprsPixmapItem *img = new AprsPixmapItem(pixmap);
    img->setAcceptHoverEvents(true);

    _map_view->_childView->scene()->addItem(img);

    img->setMessage(st->callsign,st->via,st->message);
    img->setPosition(xypos);
    img->setOffset(xypos - QPoint(8,8));
    _map_aprs.insert(img, ic);

    QString callsign_text;
    QRegExp re(";([^*]+)\\*");
    if(re.indexIn(st->payload)!=-1)
    {
        callsign_text = re.cap(1);
    }
    else
    {
        callsign_text = st->callsign;
    }
    QMapIterator<QGraphicsTextItem*, QPointF> j(_map_aprs_text);
    while(j.hasNext())
    {
        j.next();
        QGraphicsTextItem *c = j.key();
        if((c->toPlainText()==callsign_text) && replace_icon)
        {
            _map_view->_childView->scene()->removeItem(c);
            delete c;
            _map_aprs_text.remove(c);
        }
    }
    QGraphicsTextItem * callsign = new QGraphicsTextItem;
    callsign->setPos(xypos - QPoint(0,16));
    callsign->setPlainText(callsign_text);

    _map_view->_childView->scene()->addItem(callsign);
    _map_aprs_text.insert(callsign,pos);
    _aprs_stations.append(st);
    setMapItems(_map_view->zoomLevel());
    //_db->update_aprs_stations(st);
    //delete st;
}

QPointF AprsController::convertToLL(QPointF pos, double zoom)
{

    const qreal tilesOnOneEdge = pow(2.0,zoom);
    const quint16 tileSize = 256;
    qreal longitude = (pos.rx()*(360/(tilesOnOneEdge*tileSize)))-180;
    qreal latitude = (180/PI)*(atan(sinh((1-pos.ry()*(2/(tilesOnOneEdge*tileSize)))*PI)));
    return QPointF(longitude,latitude);
}


QPointF AprsController::convertToXY(QPointF ll, double zoom)
{
    const qreal tilesOnOneEdge = pow(2.0,zoom);
    const quint16 tileSize = 256;
    qreal x = (ll.x()+180) * (tilesOnOneEdge*tileSize)/360; // coord to pixel!
    qreal y = (1-(log(tan(PI/4+(ll.y()*PI/180)/2)) /PI)) /2  * (tilesOnOneEdge*tileSize);

    return QPoint(int(x), int(y));
}

void AprsController::restoreMapState()
{
    // restore aprs stations from previous sessions
    QVector<AprsStation *> aprs_stations = _aprs_stations;//_db->select_aprs_stations();
    for (int i=0;i<aprs_stations.size();++i)
    {
        AprsStation *st = aprs_stations.at(i);
        QString callsign_text;
        bool mobile = false;
        QRegExp re(";([^*]+)\\*");
        //QRegularExpressionMatch match = re.match(st->payload);
        if(re.indexIn(st->payload)!=-1)
        {
            callsign_text = re.cap(1);
        }
        else
        {
            callsign_text = st->callsign;
        }
        if(st->payload.startsWith('=') || st->payload.startsWith('/')
                || st->payload.startsWith('@') || st->payload.startsWith('!'))
            mobile= true;
        QString filename = ":/aprs/res/aprs_icons/slice_";
        QString icon;
        QPointF pos = QPointF(st->longitude,st->latitude);
        int zoom = _map_view->zoomLevel();
        QPointF xypos = convertToXY(pos, zoom);

        QVector<AprsStation *> related_stations = _aprs_stations;//_db->similar_stations(st->callsign, st->time_seen);
        if(related_stations.size()>1 && mobile)
        {
            icon = "15_0";

            AprsStation *next = related_stations[1];
            QPointF next_pos = QPointF(next->longitude,next->latitude);

            QPointF next_xypos = convertToXY(next_pos, zoom);
            QLineF progress_line(next_xypos,xypos);

            QColor colour(30,169,255,254);
            QBrush brush(colour);

            QPen pen(brush, 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
            QGraphicsLineItem *line1 = _map_view->_childView->scene()->addLine(progress_line,pen);
            _aprs_lines.push_back(line1);
            draw_lines lines;
            lines.push_back(pos);
            lines.push_back(next_pos);
            _moving_stations.insertMulti(st->callsign,lines);
        }
        else
        {
            icon = st->getImage();
        }
        //_aprs_lines.insert(st->callsign,lines);
        //icon = st->getImage();
        filename.append(icon).append(".png");
        QPixmap pixmap(filename);
        pixmap = pixmap.scaled(16,16);
        AprsPixmapItem *pic = new AprsPixmapItem(pixmap);
        pic->setAcceptHoverEvents(true);

        _map_view->_childView->scene()->addItem(pic);

        pic->setMessage(st->callsign,st->via,st->message);
        pic->setPosition(xypos);
        pic->setOffset(xypos - QPoint(7,25));
        AprsIcon ic;
        ic.position = pos;
        ic.icon = icon;
        _map_aprs.insert(pic, ic);

        if(!(related_stations.size()>1) || !mobile)
        {
            QGraphicsTextItem * callsign = new QGraphicsTextItem;
            callsign->setPos(xypos - QPoint(0,16));
            callsign->setPlainText(callsign_text);
            _map_view->_childView->scene()->addItem(callsign);
            _map_aprs_text.insert(callsign,pos);
        }

        delete st;
    }
    aprs_stations.clear();
}

void AprsController::setMapItems(quint8 zoom)
{
    {
        QMapIterator<AprsPixmapItem *, AprsIcon> i(_map_aprs);
        while (i.hasNext()) {
            i.next();
            AprsIcon ic = i.value();
            QPointF pos = ic.position;
            QPointF xypos = convertToXY(pos, zoom);
            AprsPixmapItem * img = i.key();
            img->setOffset(xypos - QPoint(8,8));
            img->setPosition(xypos);

        }
    }

    {
        QMapIterator<QGraphicsTextItem *, QPointF> i(_map_aprs_text);
        while (i.hasNext()) {
            i.next();
            QPointF pos = i.value();
            QPointF xypos = convertToXY(pos, zoom);
            QGraphicsTextItem * callsign = i.key();
            callsign->setPos(xypos - QPoint(0,16));

        }
    }

    {

        for (int i=0;i<_aprs_lines.size();++i)
        {

            _map_view->_childView->scene()->removeItem(_aprs_lines.at(i));
            delete _aprs_lines.at(i);
        }
        _aprs_lines.clear();


        QMapIterator<QString,draw_lines> it(_moving_stations);
        while(it.hasNext())
        {
            it.next();
            QPointF pos = it.value().at(0);
            QPointF next_pos = it.value().at(1);
            QPointF xypos = convertToXY(pos, zoom);
            QPointF next_xypos = convertToXY(next_pos, zoom);
            QLineF progress_line(next_xypos,xypos);

            QColor colour(30,169,255,254);
            QBrush brush(colour);

            QPen pen(brush, 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
            QGraphicsLineItem *line1 = _map_view->_childView->scene()->addLine(progress_line,pen);
            _aprs_lines.push_back(line1);
        }

    }
}
