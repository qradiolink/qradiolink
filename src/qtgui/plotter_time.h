/* -*- c++ -*- */
#ifndef PLOTTER_TIME_H
#define PLOTTER_TIME_H

#include <QtGui>
#include <QFont>
#include <QFrame>
#include <QImage>
#include <vector>
#include <QMap>

#define HORZ_DIVS_MAX_T 20    //50
#define VERT_DIVS_MIN_T 20
#define MAX_SCREENSIZE_T 16384

#define PEAK_CLICK_MAX_H_DISTANCE_T 10 //Maximum horizontal distance of clicked point from peak
#define PEAK_CLICK_MAX_V_DISTANCE_T 20 //Maximum vertical distance of clicked point from peak
#define PEAK_H_TOLERANCE_T 2


class CTimePlotter : public QFrame
{
    Q_OBJECT

public:
    explicit CTimePlotter(QWidget *parent = 0);
    ~CTimePlotter();

    QSize minimumSizeHint() const;
    QSize sizeHint() const;

    //void SetSdrInterface(CSdrInterface* ptr){m_pSdrInterface = ptr;}
    void draw();		//call to draw new fft data onto screen plot
    void setRunningState(bool running) { m_Running = running; }
    void setClickResolution(qint64 clickres) { m_ClickResolution = clickres; }
    void setFilterClickResolution(int clickres) { m_FilterClickResolution = clickres; }
    void setFilterBoxEnabled(bool enabled) { m_FilterBoxEnabled = enabled; }
    void setCenterLineEnabled(bool enabled) { m_CenterLineEnabled = enabled; }
    void setTooltipsEnabled(bool enabled) { m_TooltipsEnabled = enabled; }
    void setBookmarksEnabled(bool enabled) { m_BookmarksEnabled = enabled; }

    void setNewFftData(float *fftData, int size);
    void setNewFftData(float *fftData, float *wfData, int size);

    void setCenterFreq(quint64 f);
    void setFreqUnits(float unit) { m_FreqUnits = unit; }

    void setDemodCenterFreq(quint64 f) { m_DemodCenterFreq = f; }

    /*! \brief Move the filter to freq_hz from center. */
    void setFilterOffset(qint64 freq_hz)
    {
        m_DemodCenterFreq = m_CenterFreq + freq_hz;
        drawOverlay();
    }
    qint64 getFilterOffset(void)
    {
        return m_DemodCenterFreq - m_CenterFreq;
    }

    int getFilterBw()
    {
        return m_DemodHiCutFreq - m_DemodLowCutFreq;
    }

    void setHiLowCutFrequencies(qint64 LowCut, qint64 HiCut)
    {
        m_DemodLowCutFreq = LowCut;
        m_DemodHiCutFreq = HiCut;
        drawOverlay();
    }

    void getHiLowCutFrequencies(qint64 *LowCut, qint64 *HiCut)
    {
        *LowCut = m_DemodLowCutFreq;
        *HiCut = m_DemodHiCutFreq;
    }

    void setDemodRanges(int FLowCmin, int FLowCmax, int FHiCmin, int FHiCmax, bool symetric);

    /* Shown bandwidth around SetCenterFreq() */
    void setSpanFreq(quint32 s)
    {
        if (s > 0 && s < INT_MAX) {
            m_Span = (qint32)s;
            setFftCenterFreq(m_FftCenter);
        }
        drawOverlay();
    }

    void setHdivDelta(qint64 delta) { m_HdivDelta = delta; }
    void setVdivDelta(qint64 delta) { m_VdivDelta = delta; }

    void setFreqDigits(int digits) { m_FreqDigits = digits>=0 ? digits : 0; }

    /* Determines full bandwidth. */
    void setSampleRate(float rate)
    {
        if (rate > 0.0)
        {
            m_SampleFreq = rate;
            drawOverlay();
        }
    }

    float getSampleRate(void)
    {
        return m_SampleFreq;
    }

    void setFftCenterFreq(qint64 f) {
        qint64 limit = ((qint64)m_SampleFreq + m_Span) / 2 - 1;
        m_FftCenter = qBound(-limit, f, limit);
    }

    int     getNearestPeak(QPoint pt);
    void    setWaterfallSpan(quint64 span_ms);
    quint64 getWfTimeRes(void);
    void    setFftRate(int rate_hz);
    void    clearWaterfall(void);
    bool    saveWaterfall(const QString & filename) const;

signals:
    void newCenterFreq(qint64 f);
    void newDemodFreq(qint64 freq, qint64 delta); /* delta is the offset from the center */
    void newLowCutFreq(qint64 f);
    void newHighCutFreq(qint64 f);
    void newFilterFreq(qint64 low, qint64 high);  /* substitute for NewLow / NewHigh */
    void pandapterRangeChanged(float min, float max);
    void newZoomLevel(float level);
    void updatePlotter();

public slots:
    // zoom functions
    void resetHorizontalZoom(void);
    void moveToCenterFreq(void);
    void moveToDemodFreq(void);
    void zoomOnXAxis(float level);

    // other FFT slots
    void setFftPlotColor(const QColor color);
    void setFftFill(bool enabled);
    void setPeakHold(bool enabled);
    void setFFTHistory(bool enabled);
    void setColourFFT(bool enabled);
    void setFftRange(float min, float max);
    void setPandapterRange(float min, float max);
    void setWaterfallRange(float min, float max);
    void setPeakDetection(bool enabled, float c);
    void updateOverlay();
    void drawSampleChannel(bool real, bool imag)
    {
        m_drawReal = real;
        m_drawImag = imag;
    }

    void setPercent2DScreen(int percent)
    {
        m_Percent2DScreen = percent;
        m_Size = QSize(0,0);
        resizeEvent(NULL);
    }

protected:
    //re-implemented widget event handlers
    void paintEvent(QPaintEvent *event);
    void resizeEvent(QResizeEvent* event);
    void mouseMoveEvent(QMouseEvent * event);
    void mousePressEvent(QMouseEvent * event);
    void mouseReleaseEvent(QMouseEvent * event);
    void wheelEvent( QWheelEvent * event );

private:
    enum eCapturetype {
        NOCAP,
        LEFT,
        CENTER,
        RIGHT,
        YAXIS,
        XAXIS,
        BOOKMARK
    };

    void        drawOverlay();
    void        makeFrequencyStrs();
    qint64 xFromFreq(qint64 freq);
    qint64      freqFromX(qint64 x);
    float dbFromY(int y);
    void        zoomStepX(float factor, int x);
    qint64      roundFreq(qint64 freq, int resolution);
    quint64     msecFromY(int y);
    void        clampDemodParameters();
    bool        isPointCloseTo(int x, int xr, int delta)
    {
        return ((x > (xr - delta)) && (x < (xr + delta)));
    }
    void getScreenIntegerFFTData(qint32 plotHeight, qint32 plotWidth,
                                 float maxdB, float mindB,
                                 qint64 startFreq, qint64 stopFreq,
                                 float *inBuf, qint32 *outBuf,
                                 qint32 *maxbin, qint32 *minbin);
    void calcDivSize (qint64 low, qint64 high, int divswanted, qint64 &adjlow, qint64 &step, int& divs);
    QBrush getFFTGradient(int w, int h);
    void setWaterfallColorScheme();

    bool        m_PeakHoldActive;
    bool        m_PeakHoldValid;
    qint32      m_fftbuf[MAX_SCREENSIZE_T];
    quint8      m_wfbuf[MAX_SCREENSIZE_T]; // used for accumulating waterfall data at high time spans
    qint32      m_fftPeakHoldBuf[MAX_SCREENSIZE_T];
    float      *m_fftData;     /*! pointer to incoming FFT data */
    float      *m_wfData;
    qint64         m_fftDataSize;

    qint64         m_XAxisYCenter;
    qint64         m_YAxisWidth;

    eCapturetype    m_CursorCaptured;
    QPixmap     m_2DPixmap;
    QPixmap     m_2DPixmap_h1;
    QPixmap     m_2DPixmap_h2;
    QPixmap     m_OverlayPixmap;
    QPixmap     m_WaterfallPixmap;
    QColor      m_ColorTbl[256];
    QSize       m_Size;
    QString     m_Str;
    QString     m_HDivText[HORZ_DIVS_MAX_T+1];
    bool        m_Running;
    bool        m_DrawOverlay;
    bool        m_fftHistory;
    qint64      m_CenterFreq;       // The HW frequency
    qint64      m_FftCenter;        // Center freq in the -span ... +span range
    qint64      m_DemodCenterFreq;
    qint64      m_StartFreqAdj;
    qint64      m_FreqPerDiv;
    bool        m_CenterLineEnabled;  /*!< Distinguish center line. */
    bool        m_FilterBoxEnabled;   /*!< Draw filter box. */
    bool        m_TooltipsEnabled;     /*!< Tooltips enabled */
    bool        m_BookmarksEnabled;   /*!< Show/hide bookmarks on spectrum */
    qint64         m_DemodHiCutFreq;
    qint64         m_DemodLowCutFreq;
    qint64         m_DemodFreqX;		//screen coordinate x position
    qint64         m_DemodHiCutFreqX;	//screen coordinate x position
    qint64         m_DemodLowCutFreqX;	//screen coordinate x position
    qint64         m_CursorCaptureDelta;
    qint64         m_GrabPosition;
    int         m_Percent2DScreen;

    qint64         m_FLowCmin;
    qint64         m_FLowCmax;
    qint64         m_FHiCmin;
    qint64         m_FHiCmax;
    bool        m_symetric;

    int         m_HorDivs;   /*!< Current number of horizontal divisions. Calculated from width. */
    int         m_VerDivs;   /*!< Current number of vertical divisions. Calculated from height. */

    float       m_PandMindB;
    float       m_PandMaxdB;
    float       m_WfMindB;
    float       m_WfMaxdB;

    qint64      m_Span;
    float       m_SampleFreq;    /*!< Sample rate. */
    float      m_FreqUnits;
    qint64         m_ClickResolution;
    int         m_FilterClickResolution;

    qint64         m_Xzero;
    qint64         m_Yzero;  /*!< Used to measure mouse drag direction. */
    int         m_FreqDigits;  /*!< Number of decimal digits in frequency strings. */

    QFont       m_Font;         /*!< Font used for plotter (system font) */
    qint64         m_HdivDelta; /*!< Minimum distance in pixels between two horizontal grid lines (vertical division). */
    qint64         m_VdivDelta; /*!< Minimum distance in pixels between two vertical grid lines (horizontal division). */

    quint32     m_LastSampleRate;

    QColor      m_FftColor, m_FftFillCol, m_PeakHoldColor;
    bool        m_FftFill;

    float       m_PeakDetection;
    QMap<int,int>   m_Peaks;

    QList< QPair<QRect, qint64> >     m_BookmarkTags;

    // Waterfall averaging
    quint64     tlast_wf_ms;        // last time waterfall has been updated
    quint64     msec_per_wfline;    // milliseconds between waterfall updates
    quint64     wf_span;            // waterfall span in milliseconds (0 = auto)
    int         fft_rate;           // expected FFT rate (needed when WF span is auto)
    QPoint      LineBuf[MAX_SCREENSIZE_T];
    QBrush      m_fftBrush;
    bool        m_ColourFFT;
    QMutex      m_drawMutex;
    bool        m_drawReal;
    bool        m_drawImag;

};

#endif // PLOTTER_TIME_H
