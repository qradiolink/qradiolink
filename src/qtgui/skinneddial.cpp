/*

  Graffik Motion Control Application

  Copyright (c) 2011-2013 Dynamic Perception

 This file is part of Graffik.

    Graffik is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Graffik is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Graffik.  If not, see <http://www.gnu.org/licenses/>.

    */

#include "skinneddial.h"


 /** Constructor

   Create a new instance of SkinnedDial, with no images pre-defined.

   You must set the images and values required for display via the property setters before use.

   @param parent
   The parent widget
   */
SkinnedDial::SkinnedDial(QWidget *parent) : QDial(parent) {

    m_background      = new QPixmap(":res/dial-background.png");
    m_needle          = new QPixmap(":res/dial-needle.png");
    m_maxDeg          = 135.0;
    m_paintFlags      = QPainter::RenderHint(QPainter::Antialiasing | QPainter::SmoothPixmapTransform || QPainter::Antialiasing);
    m_cacheBackground = new QPixmap;
    m_cacheNeedle     = new QPixmap;
    m_cacheNeedleRot  = new QPixmap;
    m_cacheVal        = 0;
    m_cacheWidth      = 0;
    m_cacheHeight     = 0;

    setWrapping(false);
}

 /** Constructor

   Create a new instance of SkinnedDial, specifying all images and maximum angle.

   @param c_back
   The backgroundImage for the dial

   @param c_needle
   The needleImage for the dial

   @param c_angle
   The maxAngle for the dial

   @param parent
   The parent widget
   */
SkinnedDial::SkinnedDial(QPixmap* c_back, QPixmap* c_needle, float c_angle, QWidget *parent) : QDial(parent) {

    *m_background      = *c_back;
    *m_needle          = *c_needle;
    m_maxDeg           = c_angle;
    m_paintFlags       = QPainter::RenderHint(QPainter::Antialiasing | QPainter::SmoothPixmapTransform || QPainter::Antialiasing);
    *m_cacheBackground = *m_background;
    *m_cacheNeedle     = *m_needle;
    *m_cacheNeedleRot  = *m_needle;
    m_cacheVal         = 0;
    m_cacheWidth       = 0;
    m_cacheHeight      = 0;

}

SkinnedDial::~SkinnedDial() {
    delete m_background;
    delete m_needle;
    delete m_cacheBackground;
    delete m_cacheNeedle;
    delete m_cacheNeedleRot;
}

void SkinnedDial::setBackgroundImage(QPixmap p_img) {
    *m_background = p_img;
}

QPixmap SkinnedDial::backgroundImage() {
    return *m_background;
}

void SkinnedDial::setNeedleImage(QPixmap p_img) {
    *m_needle = p_img;
}

QPixmap SkinnedDial::needleImage() {
    return *m_needle;
}

float SkinnedDial::maxAngle() {
    return m_maxDeg;
}

void SkinnedDial::setMaxAngle(float p_angle) {
    m_maxDeg = p_angle;
}


/** Overloaded mouseReleaseEvent

  We overload the mouseReleaseEvent so that we may emit a
  signal when the mouse is released, this allows other code
  to change the behavior of the dial as needed.

  Note: the inherited QDial::mouseReleaseEvent() is called before
  emitting the signal so that it may be handled properly.

  */

void SkinnedDial::mouseReleaseEvent(QMouseEvent *p_me) {
    QDial::mouseReleaseEvent(p_me);
    emit mouseReleased();
}

/** Overloaded resizeEvent

  Causes the cache to result in a false hit whenever resized.

  */

void SkinnedDial::resizeEvent(QResizeEvent *re) {
    Q_UNUSED(re);

    QDial::resizeEvent(re);

    m_cacheHeight = 0;
    m_cacheWidth  = 0;
}

/** Overloaded paintEvent
  */

void SkinnedDial::paintEvent(QPaintEvent *pe) {

    QPainter painter(this);
    QRect eventRect = pe->rect();

    bool cacheHit = true;

        // scale pixmap so that it fits within the
        // boundaries of the event

    int height = eventRect.height();
    int  width = eventRect.width();



        // only perform scaling again if our event rectangle has changed
    if( height != m_cacheHeight || width != m_cacheWidth ) {
        if( width >= height ) {
            if( height <= m_background->height() ) {
                *m_cacheBackground = m_background->scaledToHeight(height, Qt::SmoothTransformation);
                *m_cacheNeedle     = m_needle->scaledToHeight(height, Qt::SmoothTransformation);
                *m_cacheNeedleRot  = *m_cacheNeedle;
            }
        }
        else {
            if( width <= m_background->width() ) {
                *m_cacheBackground = m_background->scaledToWidth(width, Qt::SmoothTransformation);
                *m_cacheNeedle     = m_needle->scaledToWidth(width, Qt::SmoothTransformation);
                *m_cacheNeedleRot  = *m_cacheNeedle;
            }
        }
        m_cacheHeight = height;
        m_cacheWidth  = width;
        cacheHit      = false;
    }


     // find top-left corner to start placing the pixmap for the background,
     // centering it in the total event area

    int leftPoint = (width - m_cacheBackground->width()) / 2;
    int topPoint  = (height - m_cacheBackground->height()) / 2;

    QPoint startPix(leftPoint, topPoint);

    painter.setRenderHints(m_paintFlags);
    painter.drawPixmap(startPix, *m_cacheBackground);

        // rotate the needle image and display it

    int curVal = this->value();


    QPixmap rotNeedle = *m_cacheNeedleRot;

        // only re-process the needle image if the position
        // changes, or we had a size cache miss

    if( curVal != m_cacheVal || cacheHit == false ) {

        rotNeedle = *m_cacheNeedle;

        const float midPoint = (this->maximum() - this->minimum()) * 0.5;

        if( (curVal - this->minimum()) < midPoint ) {
            float pct = (curVal - this->minimum()) / midPoint;
            rotNeedle = _rotatePix(&rotNeedle, m_maxDeg - (m_maxDeg * std::abs(pct)), false);
        }
        else if( (curVal - this->minimum()) >= midPoint ) {
            float pct = (curVal - this->minimum() - midPoint) / midPoint ;
            rotNeedle = _rotatePix(&rotNeedle, m_maxDeg * std::abs(pct), true);
        }

        m_cacheVal = curVal;
        *m_cacheNeedleRot = rotNeedle;
    }


    painter.drawPixmap(startPix, rotNeedle);

}


QPixmap SkinnedDial::_rotatePix(QPixmap *p_pix, float p_deg, bool p_dir) {

        // all rotation is CCW, so calculate for the "right" side of the knob
    if( p_dir == false )
        p_deg = 360.0 - p_deg;

        // perform rotation, transforming around the center of the
        // image

    QTransform trans;
    trans.translate(p_pix->width()/2.0 , p_pix->height()/2.0);
    trans.rotate(p_deg);
    trans.translate(-p_pix->width()/2.0 , -p_pix->height()/2.0);
    QPixmap outPix = p_pix->transformed(trans, Qt::SmoothTransformation);

        // re-crop to original size
    int xOffset = (outPix.width() - p_pix->width()) / 2;
    int yOffset = (outPix.height() - p_pix->height()) / 2;
    outPix = outPix.copy(xOffset, yOffset, p_pix->width(), p_pix->height());

    return outPix;

}
