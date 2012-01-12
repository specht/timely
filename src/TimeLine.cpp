#include "TimeLine.h"
#include "Tango.h"


typedef struct r_TextBoxInfo
{
    QString ms_Text;
    double x, y;
    QChar mc_Type;
};


k_TimeLine::k_TimeLine()
    : md_CenterJd((double)QDate::currentDate().toJulianDay())
    , md_PixelTime(0.03)
    , mb_Moving(false)
    , mb_Steering(false)
    , mi_EventCount(0)
    , mi_TextBoxWidth(180)
    , mi_BoxPadding(4)
    , mk_Arrow(":icons/arrow.png")
{
    setFocusPolicy(Qt::StrongFocus);
    QDateTime lk_CurrentDateTime = QDateTime::currentDateTime();
    md_CenterJd = (double)lk_CurrentDateTime.date().toJulianDay() + 
        (double)lk_CurrentDateTime.time().hour() * 1.0 / 24.0 + 
        (double)lk_CurrentDateTime.time().minute() * 1.0 / 24.0 / 60.0 + 
        (double)lk_CurrentDateTime.time().second() * 1.0 / 24.0 / 60.0 / 60.0 + 
        (double)lk_CurrentDateTime.time().msec() * 1.0 / 24.0 / 60.0 / 60.0 / 1000.0;

    mk_AnimationTimer.setSingleShot(false);
    setAnimationInterval();
    mk_AnimationTimer.start();
    connect(&mk_AnimationTimer, SIGNAL(timeout()), this, SLOT(repaint()));
    
    connect(&mk_SteeringTimer, SIGNAL(timeout()), this, SLOT(steerTimeout()));
    mk_SteeringTimer.setSingleShot(true);
    
    loadData();
}


k_TimeLine::~k_TimeLine()
{
    delete [] mi_EventJd_;
}


void k_TimeLine::searchFor(QString as_Text)
{
    as_Text = as_Text.toLower();
    mk_HitIndices.clear();
    for (int i = 0; i < mi_EventCount; ++i)
        if (mk_EventDescriptions[i].toLower().contains(as_Text))
            mk_HitIndices.insert(i);
    update();
}


void k_TimeLine::steerTimeout()
{
    if (!mb_Steering)
        return;
    int li_Milliseconds = mk_SteeringLastTime.elapsed();
    md_CenterJd += (double)(mk_MouseMovePosition.x() - mk_MoveStartPoint.x()) * (double)li_Milliseconds * 0.001 * md_PixelTime;
    md_PixelTime *= 1.0 - (double)(mk_MouseMovePosition.y() - mk_MoveStartPoint.y()) * (double)li_Milliseconds * 0.000001;
    fixView();
    update();
    mk_SteeringLastTime.start();
    mk_SteeringTimer.start(50);
}


void k_TimeLine::loadData()
{
    QFile lk_File("timely.txt");
    lk_File.open(QIODevice::ReadOnly);
    QTextStream lk_Stream(&lk_File);
    mi_EventCount = 0;
    mk_Events.clear();
    while (!lk_Stream.atEnd())
    {
        lk_Stream.readLine();
        ++mi_EventCount;
    }
    mi_EventJd_ = new int[mi_EventCount];

    lk_Stream.seek(0);
    int i = 0;
    while (!lk_Stream.atEnd())
    {
        QString ls_Line = lk_Stream.readLine();
        int li_SpaceIndex;
        QString ls_Part;

        li_SpaceIndex = ls_Line.indexOf(QChar(' '));
        ls_Part = ls_Line.left(li_SpaceIndex);
        ls_Line = ls_Line.right(ls_Line.length() - li_SpaceIndex - 1);
        int li_Jd = QVariant(ls_Part).toInt();
        
        li_SpaceIndex = ls_Line.indexOf(QChar(' '));
        ls_Part = ls_Line.left(li_SpaceIndex);
        ls_Line = ls_Line.right(ls_Line.length() - li_SpaceIndex - 1);
        int li_DateScope = QVariant(ls_Part).toInt();

        li_SpaceIndex = ls_Line.indexOf(QChar(' '));
        ls_Part = ls_Line.left(li_SpaceIndex);
        ls_Line = ls_Line.right(ls_Line.length() - li_SpaceIndex - 1);
        QString ls_Marker = ls_Part;
        
        li_SpaceIndex = ls_Line.indexOf(QChar(' '));
        ls_Part = ls_Line.left(li_SpaceIndex);
        ls_Line = ls_Line.right(ls_Line.length() - li_SpaceIndex - 1);
        QString ls_EventType = ls_Part;
        
        mk_Events.insert(li_Jd, ls_Line);
        mi_EventJd_[i] = li_Jd;
        mk_EventTypes.append(ls_EventType.at(0));
        mk_EventDescriptions.append(ls_Line.trimmed());
        ++i;
    }

    mk_HitIndices.clear();
    lk_File.close();
}


void k_TimeLine::paintEvent(QPaintEvent* ak_Event_)
{
    double ld_CenterY = (double)height() * 0.5;
    double ld_LeftJd = md_CenterJd - ld_CenterY * md_PixelTime;
    double ld_RightJd = md_CenterJd + ld_CenterY * md_PixelTime;
    
    QDate lk_LeftDate = QDate::fromJulianDay((int)floor(ld_LeftJd));
    QDate lk_RightDate = QDate::fromJulianDay((int)floor(ld_RightJd));
    
    QPainter lk_Painter(this);
/*    lk_Painter.setRenderHint(QPainter::Antialiasing, true);
    lk_Painter.setRenderHint(QPainter::TextAntialiasing, true);
    lk_Painter.setRenderHint(QPainter::SmoothPixmapTransform, true);*/
    lk_Painter.setFont(QFont("Bitstream Charter", 12));
    QFontMetrics lk_FontMetrics(lk_Painter.font());
    double ld_MaxTimePartLabelWidth = lk_FontMetrics.width("99");
    double ld_MaxDayLabelWidth = lk_FontMetrics.width("99");
    double ld_MaxMonthLabelWidth = lk_FontMetrics.width("September");
    double ld_MaxShortMonthLabelWidth = lk_FontMetrics.width("May");
    double ld_MaxYearLabelWidth = lk_FontMetrics.width("2009");
    if (lk_LeftDate.year() < 0)
        ld_MaxYearLabelWidth = lk_FontMetrics.width("2009 BC");
    
    double ld_FontHeight = (double)lk_FontMetrics.height();
    double ld_FontHPadding = ld_FontHeight * 0.2;
    double ld_TextRowHeight = ld_FontHeight;
    double ld_FontDescent = (double)lk_FontMetrics.descent();

    // determine which rows should be drawn
    int li_DrawMinuteLabels= 0;
/*    if (1.0 / 24.0 / 60.0 / md_PixelTime >= 3)
    {
        li_DrawMinuteLabels = 1;
        if (1.0 / 24.0 / 60.0 / md_PixelTime >= ld_MaxTimePartLabelWidth + ld_FontHPadding * 2.0)
            li_DrawMinuteLabels = 2;
    }*/
    int li_DrawHourLabels = 0;
/*    if (1.0 / 24.0 / md_PixelTime >= 3)
    {
        li_DrawHourLabels = 1;
        if (1.0 / 24.0 / md_PixelTime >= ld_MaxTimePartLabelWidth + ld_FontHPadding * 2.0)
            li_DrawHourLabels = 2;
    }*/
    int li_DrawDayLabels = 0;
    if (1.0 / md_PixelTime >= 3)
    {
        li_DrawDayLabels = 1;
        if (1.0 / md_PixelTime >= ld_MaxDayLabelWidth + ld_FontHPadding * 2.0)
            li_DrawDayLabels = 2;
    }
    int li_DrawMonthLabels = 0;
    if (30.0 / md_PixelTime >= ld_MaxShortMonthLabelWidth + ld_FontHPadding * 2.0)
    {
        li_DrawMonthLabels = 1;
        if (30.0 / md_PixelTime >= ld_MaxMonthLabelWidth + ld_FontHPadding * 2.0)
            li_DrawMonthLabels = 2;
    }
    
    int li_RowCount = 1;
    if (li_DrawMonthLabels > 0)
        ++li_RowCount;
    if (li_DrawDayLabels > 0)
        ++li_RowCount;
    if (li_DrawHourLabels > 0)
        ++li_RowCount;
    if (li_DrawMinuteLabels > 0)
        ++li_RowCount;
    
    lk_Painter.fillRect(0, 0, width(), height(), QBrush(Qt::white));
    lk_Painter.fillRect(0.0, 0.0, ld_TextRowHeight * li_RowCount, height(), QBrush(TANGO_ALUMINIUM_0));
    
    double ld_BorderY = 0.0;
    QDateTime lk_BorderDate;
    
/*    if (li_DrawHourLabels > 0)
    {
        ld_BorderY = 0.0;
        lk_BorderDate = QDateTime(lk_LeftDate);
        
        while (ld_BorderY < width())
        {
            ld_BorderY = ((double)lk_BorderDate.date().toJulianDay() + (double)lk_BorderDate.time().hour() / 24.0 - md_CenterJd) / md_PixelTime + ld_CenterY;
            lk_Painter.setPen(QPen(TANGO_ALUMINIUM_2));
            lk_Painter.drawLine(ld_BorderY, height() - ld_TextRowHeight * 4.0, ld_BorderY, height() - ld_TextRowHeight * 3.0);
            QDateTime lk_NextDate = lk_BorderDate.addSecs(3600);
            if (li_DrawHourLabels > 1)
            {
                QString ls_Text = QVariant(lk_BorderDate.time().hour()).toString();
                if (ld_BorderY < 0.0)
                {
                    ld_BorderY = 0.0;
                    double ld_Delta = (lk_FontMetrics.width(ls_Text) + ld_FontHPadding * 2.0) - (((double)lk_NextDate.date().toJulianDay() + (double)lk_NextDate.time().hour() / 24.0 - md_CenterJd) / md_PixelTime + ld_CenterY);
                    if (ld_Delta > 0.0)
                        ld_BorderY -= ld_Delta;
                }
                lk_Painter.setPen(QPen(TANGO_ALUMINIUM_4));
                lk_Painter.drawText(ld_FontHPadding + ld_BorderY, height() - ld_TextRowHeight * 3.0 - ld_FontDescent, ls_Text);
            }
            lk_BorderDate = lk_NextDate;
        }
    }*/
    
    if (li_DrawDayLabels > 0)
    {
        ld_BorderY = 0.0;
        lk_BorderDate = QDateTime(lk_LeftDate);
        
        while (ld_BorderY < height())
        {
            ld_BorderY = ((double)lk_BorderDate.date().toJulianDay() - md_CenterJd) / md_PixelTime + ld_CenterY;
            if (lk_BorderDate.date().dayOfWeek() >= 6)
                lk_Painter.fillRect(ld_TextRowHeight * 2.0, ld_BorderY, ld_TextRowHeight, 1.0 / md_PixelTime + 1.0, QBrush(TANGO_ALUMINIUM_1));
            QDateTime lk_NextDate = QDateTime(lk_BorderDate.date().addDays(1));
            lk_BorderDate = lk_NextDate;
        }
    }
    
    // draw 'now'
    QDateTime lk_CurrentDateTime = QDateTime::currentDateTime();
    double ld_NowJd = (double)lk_CurrentDateTime.date().toJulianDay() + 
        (double)lk_CurrentDateTime.time().hour() * 1.0 / 24.0 + 
        (double)lk_CurrentDateTime.time().minute() * 1.0 / 24.0 / 60.0 + 
        (double)lk_CurrentDateTime.time().second() * 1.0 / 24.0 / 60.0 / 60.0 + 
        (double)lk_CurrentDateTime.time().msec() * 1.0 / 24.0 / 60.0 / 60.0 / 1000.0;

    if (ld_NowJd >= ld_LeftJd && ld_NowJd <= ld_RightJd)
    {
        lk_Painter.setPen(QPen(TANGO_CHAMELEON_2));
        double y = (ld_NowJd - md_CenterJd) / md_PixelTime + ld_CenterY;
        lk_Painter.drawLine(0, y, ld_TextRowHeight * li_RowCount, y);
    }
    
    if (li_DrawDayLabels > 0)
    {
        ld_BorderY = 0.0;
        lk_BorderDate = QDateTime(lk_LeftDate);
        
        while (ld_BorderY < height())
        {
            ld_BorderY = ((double)lk_BorderDate.date().toJulianDay() - md_CenterJd) / md_PixelTime + ld_CenterY;
            lk_Painter.setPen(QPen(TANGO_ALUMINIUM_2));
            lk_Painter.drawLine(ld_TextRowHeight * 2.0, ld_BorderY, ld_TextRowHeight * 3.0, ld_BorderY);
            QDateTime lk_NextDate = QDateTime(lk_BorderDate.date().addDays(1));
            if (li_DrawDayLabels > 1)
            {
                QString ls_Text = QVariant(lk_BorderDate.date().day()).toString();
                if (ld_BorderY < 0.0)
                {
                    ld_BorderY = 0.0;
                    double ld_Delta = (lk_FontMetrics.width(ls_Text) + ld_FontHPadding * 2.0) - (((double)lk_NextDate.date().toJulianDay() - md_CenterJd) / md_PixelTime + ld_CenterY);
                    if (ld_Delta > 0.0)
                        ld_BorderY -= ld_Delta;
                }
                lk_Painter.setPen(QPen(TANGO_ALUMINIUM_4));
                lk_Painter.save();
                lk_Painter.translate(ld_TextRowHeight * 2.0 + ld_FontDescent, ld_FontHPadding + ld_BorderY);
                lk_Painter.rotate(90);
                lk_Painter.drawText(0, 0, ls_Text);
                lk_Painter.restore();
            }
            lk_BorderDate = lk_NextDate;
        }
    }
    if (li_DrawMonthLabels > 0)
    {
        ld_BorderY = 0.0;
        lk_BorderDate.setDate(QDate(lk_LeftDate.year(), lk_LeftDate.month(), 1));
        
        while (ld_BorderY < height())
        {
            ld_BorderY = ((double)lk_BorderDate.date().toJulianDay() - md_CenterJd) / md_PixelTime + ld_CenterY;
            lk_Painter.setPen(QPen(TANGO_ALUMINIUM_2));
            lk_Painter.drawLine(ld_TextRowHeight * 1.0, ld_BorderY, ld_TextRowHeight * 2.0, ld_BorderY);
            QDateTime lk_NextDate = QDateTime(lk_BorderDate.date().addMonths(1));
            if (li_DrawMonthLabels > 0)
            {
                QString ls_Text;
                if (li_DrawMonthLabels == 1)
                    ls_Text = QDate::shortMonthName(lk_BorderDate.date().month());
                else if (li_DrawMonthLabels == 2)
                    ls_Text = QDate::longMonthName(lk_BorderDate.date().month());
                if (ld_BorderY < 0.0)
                {
                    ld_BorderY = 0.0;
                    double ld_Delta = (lk_FontMetrics.width(ls_Text) + ld_FontHPadding * 2.0) - (((double)lk_NextDate.date().toJulianDay() - md_CenterJd) / md_PixelTime + ld_CenterY);
                    if (ld_Delta > 0.0)
                        ld_BorderY -= ld_Delta;
                }
                lk_Painter.setPen(QPen(TANGO_ALUMINIUM_4));
                lk_Painter.save();
                lk_Painter.translate(ld_TextRowHeight + ld_FontDescent, ld_FontHPadding + ld_BorderY);
                lk_Painter.rotate(90);
                lk_Painter.drawText(0, 0, ls_Text);
                lk_Painter.restore();
            }
            lk_BorderDate = lk_NextDate;
        }
    }
    
    int li_YearSkip = 1;
    while (366.0 * li_YearSkip / md_PixelTime < ld_MaxYearLabelWidth + ld_FontHPadding * 2.0)
        li_YearSkip *= 10;
    ld_BorderY = 0.0;
    int li_BorderYear = floor((qreal)lk_LeftDate.year() / li_YearSkip) * li_YearSkip;
    if (li_BorderYear == 0)
        li_BorderYear = 1;
    if (li_BorderYear < -4700)
        li_BorderYear = -4700;
    lk_BorderDate.setDate(QDate(li_BorderYear, 1, 1));
    
    while (ld_BorderY < height())
    {
        ld_BorderY = ((double)lk_BorderDate.date().toJulianDay() - md_CenterJd) / md_PixelTime + ld_CenterY;
        lk_Painter.setPen(QPen(TANGO_ALUMINIUM_2));
        lk_Painter.drawLine(0.0, ld_BorderY, ld_TextRowHeight, ld_BorderY);
        QString ls_Text = QVariant(abs(lk_BorderDate.date().year())).toString();
        QDateTime lk_NextDate = lk_BorderDate;
        if (lk_BorderDate.date().year() == 1 && li_YearSkip > 1)
            lk_NextDate = QDateTime(lk_BorderDate.addYears(li_YearSkip - 1));
        else
            lk_NextDate = QDateTime(lk_BorderDate.addYears(li_YearSkip));
        if (ld_BorderY < 0.0)
        {
            ld_BorderY = 0.0;
            
            double ld_Delta = (lk_FontMetrics.width(ls_Text) + ld_FontHPadding * 2.0) - (((double)lk_NextDate.date().toJulianDay() - md_CenterJd) / md_PixelTime + ld_CenterY);
            if (ld_Delta > 0.0)
                ld_BorderY -= ld_Delta;
        }
        lk_Painter.setPen(QPen(TANGO_ALUMINIUM_4));
        if (lk_BorderDate.date().year() < 0)
            ls_Text += " BC";
        lk_Painter.save();
        lk_Painter.translate(ld_FontDescent, ld_FontHPadding + ld_BorderY);
        lk_Painter.rotate(90);
        lk_Painter.drawText(0, 0, ls_Text);
        lk_Painter.restore();
        lk_BorderDate = lk_NextDate;
    }
    
    /*
    lk_Painter.setPen(QPen(TANGO_ALUMINIUM_4));
    
//     lk_Painter.drawText(ld_FontHPadding, ld_TextRowHeight, lk_LeftDate.toString());
    QString ls_Text = lk_RightDate.toString();
//     lk_Painter.drawText(width() - lk_FontMetrics.width(ls_Text) - ld_FontHPadding - ld_FontDescent, ld_TextRowHeight, ls_Text);
    
    lk_Painter.setRenderHint(QPainter::Antialiasing, true);
    int li_VisibleEventCount = 0;
    */
    int li_LeftJd = (int)floor(ld_LeftJd - md_PixelTime * 500.0);
    int li_RightJd = (int)ceil(ld_RightJd + md_PixelTime * 500.0);
    if (li_LeftJd < 0)
        li_LeftJd = 0;
    /*
    
    // determine event snapping width
    int li_SnapWidth = 1;
    while ((double)li_SnapWidth / md_PixelTime < 10.0)
        li_SnapWidth <<= 1;
    int li_BoxSnapWidth = 1;
    while ((double)li_BoxSnapWidth / md_PixelTime < mi_TextBoxWidth)
        li_BoxSnapWidth <<= 1;
    
    lk_Painter.setFont(QFont("Bitstream Charter", 9));
    lk_FontMetrics = QFontMetrics(lk_Painter.font());
    
    int li_OldJd = -1;
    int li_OldJdStartIndex = -1;
    int li_LastInIndex = -1;
    QString ls_LastDescription;
    
    QHash<int, double> lk_BoxSpanHeight;
    */
    QMultiMap<int, r_TextBoxInfo> lk_TextBoxes;
    
    for (int i = 0; i <= mi_EventCount; ++i)
    {
        bool lb_JumpIn = false;
        if (i == mi_EventCount)
            lb_JumpIn = true;
        else
            lb_JumpIn = (mi_EventJd_[i] >= li_LeftJd && mi_EventJd_[i] <= li_RightJd);
        
        if (lb_JumpIn)
        {
            int li_Jd = mi_EventJd_[i];
            if (mk_HitIndices.contains(i))
            {
                double y = ((double)mi_EventJd_[i] - md_CenterJd) / md_PixelTime + height() / 2;
                QString ls_Text = "<p><b>" + QDate::fromJulianDay(mi_EventJd_[i]).toString() + "</b> &ndash; " + mk_EventDescriptions[i] + "</p>";
    
    //                        QTextDocument doc;
    //                        doc.setHtml(ls_Text);
    //                        doc.setHtml(doc.toPlainText());
    //                        doc.setHtml(" &ndash; " + doc.toPlainText());
    //                        ls_Text = doc.toPlainText();
                r_TextBoxInfo lr_TextBoxInfo;
                lr_TextBoxInfo.ms_Text = ls_Text;
                lr_TextBoxInfo.x = li_RowCount * ld_FontHeight + 8;
                lr_TextBoxInfo.y = y;
                lr_TextBoxInfo.mc_Type = mk_EventTypes[i];
                lk_TextBoxes.insert(mi_EventJd_[i], lr_TextBoxInfo);
            }
            if (i == mi_EventCount)
                break;
            
//            QTextDocument doc;
//            doc.setHtml(mk_EventDescriptions[i]);
            //doc.drawContents(&lk_Painter, QRectF(QPointF(((double)mi_EventJd_[i] - md_CenterJd) / md_PixelTime + ld_CenterY, height() - ((double)ld_TextRowHeight * ((double)li_RowCount + 0.5))), QSizeF(1000.0, ld_TextRowHeight)));

//             lk_Painter.drawText(((double)mi_EventJd_[i] - md_CenterJd) / md_PixelTime + ld_CenterY, height() - ((double)ld_TextRowHeight * ((double)li_RowCount + 1.5)), doc.toPlainText());
        }
    }
    
    QMultiMap<int, r_TextBoxInfo>::const_iterator lk_Iter;
    QList<QRect> lk_DrawnRects;
//     printf("\n");

    int li_DrawFirstJd = lk_TextBoxes.keys().first();
    int li_DrawLastJd = lk_TextBoxes.keys().last();

    int li_DrawLastJdCopy = li_DrawLastJd;
    int li_MaxLevel = -1;
    while (li_DrawLastJdCopy > 0)
    {
        li_MaxLevel += 1;
        li_DrawLastJdCopy >>= 1;
    }
    for (int li_Level = li_MaxLevel; li_Level >= 0; --li_Level)
    {
        int li_TempStart = 1 << li_Level;
        int li_Step = li_TempStart << 1;
        int li_Mask = ~((1 << (li_Level + 1)) - 1);
        int li_Start = (li_DrawFirstJd & li_Mask) | li_TempStart;
        while (li_Start <= li_DrawLastJd)
        {
            // handle li_Start
            li_Start += li_Step;
            int li_Jd = li_Start;
            if (lk_TextBoxes.contains(li_Jd))
            {
                foreach (r_TextBoxInfo lr_Info, lk_TextBoxes.values(li_Jd))
                {
                    QTextDocument doc;
                    doc.setDefaultStyleSheet(QString("* { font-family: Bitstream Charter; font-size: 12px; color: #000;} a { color: %1; text-decoration: none; }").arg(TANGO_SKY_BLUE_1));
                    doc.setPageSize(QSizeF(mi_TextBoxWidth, height()));
                    doc.setHtml(lr_Info.ms_Text);
                    double x = lr_Info.x;
                    double y = lr_Info.y;
                    
                    QRectF lk_ResultRect = QRectF(QPointF(0.0, 0.0), QSizeF(doc.idealWidth(), doc.size().height()));
                    QRect lk_QueryRect = (lk_ResultRect.translated(QPointF(x + mi_BoxPadding / 2, y - 0.5))).toRect();
                    while (true)
                    {
                        bool lb_WasGood = true;
                        foreach (QRect lk_TestRect, lk_DrawnRects)
                        {
                            if (!(lk_TestRect.right() < lk_QueryRect.left() ||
                                lk_TestRect.left() > lk_QueryRect.right() ||
                                lk_TestRect.bottom() < lk_QueryRect.top() ||
                                lk_TestRect.top() > lk_QueryRect.bottom()))
                            {
                                x = lk_TestRect.right() + 5;
                                lk_QueryRect = (lk_ResultRect.translated(QPointF(x + mi_BoxPadding / 2, y - 0.5))).toRect();
                                lb_WasGood = false;
                                break;
                            }
                        }
                        if (lb_WasGood)
                            break;
                    }
                    
                    if (lr_Info.mc_Type == QChar('b'))
                    {
                        lk_Painter.setBrush(QBrush("#d7dee9"));
                        lk_Painter.setPen(QPen(TANGO_SKY_BLUE_2));
                    }
                    else if (lr_Info.mc_Type == QChar('d'))
                    {
                        lk_Painter.setBrush(QBrush("#efd1d1"));
                        lk_Painter.setPen(QPen(TANGO_SCARLET_RED_2));
                    }
                    else
                    {
                        lk_Painter.setBrush(QBrush(TANGO_ALUMINIUM_0));
                        lk_Painter.setPen(QPen(TANGO_ALUMINIUM_3));
                    }
                    lk_Painter.drawRoundedRect(QRectF(QPointF(x + mi_BoxPadding / 2, y - 0.5), lk_ResultRect.size()), 4.0, 4.0);
                    lk_Painter.setViewport(QRectF(QPointF(x + mi_BoxPadding / 2, y), size()).toRect());
                    doc.drawContents(&lk_Painter);
                    lk_Painter.setViewTransformEnabled(false);
                    lk_ResultRect.moveTo(x + mi_BoxPadding / 2, y - 0.5);
                    lk_DrawnRects << lk_ResultRect.toRect();
                }
            }
        }
    }
        
    lk_Painter.setPen(QPen(TANGO_ALUMINIUM_2));
    for (int i = 1; i < li_RowCount + 1; ++i)
        lk_Painter.drawLine(ld_TextRowHeight * i, 0.0, ld_TextRowHeight * i, height());

    lk_Painter.setPen(QPen(TANGO_ALUMINIUM_2));
    lk_Painter.setBrush(QBrush(Qt::NoBrush));
    lk_Painter.drawRect(0, 0, width() - 1, height() - 1);
}


void k_TimeLine::mouseMoveEvent(QMouseEvent* ak_Event_)
{
    mk_MouseMovePosition = ak_Event_->pos();
    if (mb_Moving)
    {
        md_CenterJd = md_MoveStartCenterJd + (double)(mk_MoveStartPoint.y() - ak_Event_->pos().y()) * md_PixelTime;
        fixView();
        update();
    }
}


void k_TimeLine::mousePressEvent(QMouseEvent* ak_Event_)
{
    mk_MoveStartPoint = ak_Event_->pos();
    mk_MouseMovePosition = ak_Event_->pos();
    if (ak_Event_->button() == Qt::RightButton)
    {
        mb_Steering = true;
        mk_SteeringLastTime.start();
        mk_SteeringTimer.start(50);
    }
    else
    {
        mb_Moving = true;
        md_MoveStartCenterJd = md_CenterJd;
    }
}


void k_TimeLine::mouseReleaseEvent(QMouseEvent* ak_Event_)
{
    mb_Moving = false;
    mb_Steering = false;
}


void k_TimeLine::wheelEvent(QWheelEvent* ak_Event_)
{
    int dy = ak_Event_->pos().y() - height() / 2;
    double ld_PointerTime = ((double)dy * md_PixelTime) + md_CenterJd;
    md_PixelTime *= 1.0 - (double)ak_Event_->delta() * 0.001;
    md_CenterJd = ld_PointerTime - md_PixelTime * dy;
    fixView();
    update();
    setAnimationInterval();
}


void k_TimeLine::keyPressEvent(QKeyEvent* ak_Event_)
{
    bool lb_Handled = true;
    double ld_MoveRatio = 50.0;
    if (ak_Event_->modifiers() & Qt::AltModifier)
        ld_MoveRatio = 10.0;
    if (ak_Event_->modifiers() & Qt::ControlModifier)
        ld_MoveRatio = width();
    switch (ak_Event_->key())
    {
        case Qt::Key_Left:
            md_CenterJd -= md_PixelTime * ld_MoveRatio;
            break;
        case Qt::Key_Right:
            md_CenterJd += md_PixelTime * ld_MoveRatio;
            break;
        case Qt::Key_Up:
            md_PixelTime /= 1.1;
            break;
        case Qt::Key_Down:
            md_PixelTime *= 1.1;
            break;
        default:
            lb_Handled = false;
    }
    if (lb_Handled)
    {
        fixView();
        update();
        setAnimationInterval();
    }
    else
        QWidget::keyPressEvent(ak_Event_);
}


void k_TimeLine::fixView()
{
    double ld_CenterY = (double)width() * 0.5;
    double ld_LeftJd = md_CenterJd - ld_CenterY * md_PixelTime;
    double ld_RightJd = md_CenterJd + ld_CenterY * md_PixelTime;
    if (ld_LeftJd < 4748.0)
    {
        md_CenterJd += 4748.0 - ld_LeftJd;
        ld_LeftJd = md_CenterJd - ld_CenterY * md_PixelTime;
        ld_RightJd = md_CenterJd + ld_CenterY * md_PixelTime;
    }
    if (ld_RightJd > 5373468.0)
    {
        md_CenterJd += 5373468.0 - ld_RightJd;
        ld_LeftJd = md_CenterJd - ld_CenterY * md_PixelTime;
        ld_RightJd = md_CenterJd + ld_CenterY * md_PixelTime;
    }
    if (ld_LeftJd < 4748.0)
    {
        md_CenterJd = (3437698.0 + 4748.0) * 0.5;
        md_PixelTime = (md_CenterJd - 4748.0) / ld_CenterY;
    }
}


void k_TimeLine::setAnimationInterval()
{
    double ld_Interval = md_PixelTime / (1.0 / 86400000.0);
    if (ld_Interval < 10.0)
        ld_Interval = 10.0;
    if (ld_Interval > 86400000)
        ld_Interval = 86400000;
    mk_AnimationTimer.setInterval(ld_Interval);
}
