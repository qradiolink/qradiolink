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

#ifndef SKINNEDDIAL_H
#define SKINNEDDIAL_H

#include <QDial>
#include <QPixmap>
#include <QPainter>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QTransform>


/** Custom-Skinned QDial

  This class allows one to create a custom-skinned QDial by supplying your own image
  for the background and needle, both through Qt Style Sheets and through normal C++
  usage.

  Example Usage:
  @code
  #include "skinneddial.h"

  ...

  SkinnedDial* myDial = new SkinnedDial();
  @endcode

  All look and feel is provided through images, you cannot have hash marks drawn programmatically, if you
  want them, put them in your image.

  @section dialvalues Acceptable QDial Limits

  The SkinnedDial class makes an assumption that the center of the dial is at the value of 0, and
  that negative values are on the left side of the dial, and the positive values are on the right
  side.  SkinnedDial requires that the minimum be < 0 and the maximum be > 0 and that there be
  an equivalent distance on both side of the dial. For example:

  @code
  myDial->setMaximum(100);
  myDial->setMinimum(-100);
  @endcode

  You must also specify where the maximum travel of the dial is, in degrees.
  See the \ref qsscustom "Customing the look and feel via QSS" section for more information.

  @section qsscustom Customizing the look and feel via QSS

  You can control the look and feel of the SkinnedDial via Qt Stylesheets by setting values for the custom
  properties associated with the skinned dial.

  It is expected that the backgroundImage and needleImage are of the exact same dimensions, and the needleImage
  is transparent everywhere the needle isn't.

  The maxAngle property determines the extent that the needle may travel on either side of the circle.  140' would
  imply that the needle may not move more than 140' either clockwise or counter-clockwise.

  @code
  SkinnedDial {
    qproperty-backgroundImage: url(:dial-back.png);
    qproperty-needleImage: url(:dial-needle.png);
    qproperty-maxAngle: 140;
   }
   @endcode

   @section scaling Scaling of Images

   Your images are automatically scaled relative to the container, up and to the full size of the input
   image.  The input image will not be scaled larger than its original size.


   @author
   C. A. Church

   Copyright &copy; 2012 Dynamic Perception LLC
   */

class SkinnedDial : public QDial
{
    Q_OBJECT
        /** Background Image Property

          Sets the background image for the dial (the dial its self, and any surrounding decoration).
          */
    Q_PROPERTY(QPixmap backgroundImage READ backgroundImage WRITE setBackgroundImage DESIGNABLE true)
        /** Needle Image Property

          Sets the needle (pointer) image, to be drawn over the dial. Must be the same width and height
          as the background image, and transparent everywhere the needle isn't.
          */
    Q_PROPERTY(QPixmap needleImage READ needleImage WRITE setNeedleImage DESIGNABLE true)
        /** Maximum Rotation Angle Property

          Sets the maximum angle at which the needle may be rotated on either side of the dial
          */
    Q_PROPERTY(float maxAngle READ maxAngle WRITE setMaxAngle DESIGNABLE true)

public:
    explicit SkinnedDial(QWidget *parent = 0);
    SkinnedDial(QPixmap* c_back, QPixmap* c_needle, float c_angle, QWidget *parent = 0);

    ~SkinnedDial();

    QPixmap backgroundImage();
    void setBackgroundImage(QPixmap p_img);

    QPixmap needleImage();
    void setNeedleImage(QPixmap p_img);

    float maxAngle();
    void setMaxAngle(float p_angle);


protected:

    void paintEvent(QPaintEvent *pe);

signals:
    
    void mouseReleased();

public slots:
    
    void mouseReleaseEvent(QMouseEvent *p_me);
    void resizeEvent(QResizeEvent *re);

private:

    QPixmap* m_background;
    QPixmap* m_needle;
    QPainter::RenderHint m_paintFlags;
    float m_maxDeg;

    QPixmap* m_cacheBackground;
    QPixmap* m_cacheNeedle;
    QPixmap* m_cacheNeedleRot;


    int m_cacheVal;
    int m_cacheWidth;
    int m_cacheHeight;

    QPixmap _rotatePix(QPixmap* p_pix, float p_deg, bool p_dir);

};

#endif // SKINNEDDIAL_H
