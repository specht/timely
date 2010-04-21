#include "TimeLine.h"
#include "Tango.h"


typedef struct r_TextBoxInfo
{
	QString ms_Text;
	int mi_BoxSpanId;
	int mi_BoxSpanOffset;
	double md_Position;
    double md_RealPosition;
	QChar mc_Type;
};


k_TimeLine::k_TimeLine()
	: md_CenterJd((double)QDate::currentDate().toJulianDay())
	, md_PixelTime(0.03)
	, mb_Moving(false)
	, mb_Steering(false)
	, mi_EventCount(0)
	, mi_TextBoxWidth(300)
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
	double ld_CenterX = (double)width() * 0.5;
	double ld_LeftJd = md_CenterJd - ld_CenterX * md_PixelTime;
	double ld_RightJd = md_CenterJd + ld_CenterX * md_PixelTime;
	
	QDate lk_LeftDate = QDate::fromJulianDay((int)floor(ld_LeftJd));
	QDate lk_RightDate = QDate::fromJulianDay((int)floor(ld_RightJd));
	
	QPainter lk_Painter(this);
/*	lk_Painter.setRenderHint(QPainter::Antialiasing, true);
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
/*	if (1.0 / 24.0 / 60.0 / md_PixelTime >= 3)
	{
		li_DrawMinuteLabels = 1;
		if (1.0 / 24.0 / 60.0 / md_PixelTime >= ld_MaxTimePartLabelWidth + ld_FontHPadding * 2.0)
			li_DrawMinuteLabels = 2;
	}*/
	int li_DrawHourLabels = 0;
/*	if (1.0 / 24.0 / md_PixelTime >= 3)
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
	lk_Painter.fillRect(0.0, height() - ld_TextRowHeight * li_RowCount, width(), ld_TextRowHeight * li_RowCount, QBrush(TANGO_ALUMINIUM_0));
	
	double ld_BorderX = 0.0;
	QDateTime lk_BorderDate;
	
/*	if (li_DrawHourLabels > 0)
	{
		ld_BorderX = 0.0;
		lk_BorderDate = QDateTime(lk_LeftDate);
		
		while (ld_BorderX < width())
		{
			ld_BorderX = ((double)lk_BorderDate.date().toJulianDay() + (double)lk_BorderDate.time().hour() / 24.0 - md_CenterJd) / md_PixelTime + ld_CenterX;
			lk_Painter.setPen(QPen(TANGO_ALUMINIUM_2));
			lk_Painter.drawLine(ld_BorderX, height() - ld_TextRowHeight * 4.0, ld_BorderX, height() - ld_TextRowHeight * 3.0);
			QDateTime lk_NextDate = lk_BorderDate.addSecs(3600);
			if (li_DrawHourLabels > 1)
			{
				QString ls_Text = QVariant(lk_BorderDate.time().hour()).toString();
				if (ld_BorderX < 0.0)
				{
					ld_BorderX = 0.0;
					double ld_Delta = (lk_FontMetrics.width(ls_Text) + ld_FontHPadding * 2.0) - (((double)lk_NextDate.date().toJulianDay() + (double)lk_NextDate.time().hour() / 24.0 - md_CenterJd) / md_PixelTime + ld_CenterX);
					if (ld_Delta > 0.0)
						ld_BorderX -= ld_Delta;
				}
				lk_Painter.setPen(QPen(TANGO_ALUMINIUM_4));
				lk_Painter.drawText(ld_FontHPadding + ld_BorderX, height() - ld_TextRowHeight * 3.0 - ld_FontDescent, ls_Text);
			}
			lk_BorderDate = lk_NextDate;
		}
	}*/
	
	if (li_DrawDayLabels > 0)
	{
		ld_BorderX = 0.0;
		lk_BorderDate = QDateTime(lk_LeftDate);
		
		while (ld_BorderX < width())
		{
			ld_BorderX = ((double)lk_BorderDate.date().toJulianDay() - md_CenterJd) / md_PixelTime + ld_CenterX;
			if (lk_BorderDate.date().dayOfWeek() >= 6)
				lk_Painter.fillRect(ld_BorderX, height() - ld_TextRowHeight * 3.0, 1.0 / md_PixelTime + 1.0,ld_TextRowHeight, QBrush(TANGO_ALUMINIUM_1));
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
		double x = (ld_NowJd - md_CenterJd) / md_PixelTime + ld_CenterX;
		lk_Painter.drawLine(x, height(), x, height() - ld_TextRowHeight * li_RowCount);
	}
	
	if (li_DrawDayLabels > 0)
	{
		ld_BorderX = 0.0;
		lk_BorderDate = QDateTime(lk_LeftDate);
		
		while (ld_BorderX < width())
		{
			ld_BorderX = ((double)lk_BorderDate.date().toJulianDay() - md_CenterJd) / md_PixelTime + ld_CenterX;
			lk_Painter.setPen(QPen(TANGO_ALUMINIUM_2));
			lk_Painter.drawLine(ld_BorderX, height() - ld_TextRowHeight * 3.0, ld_BorderX, height() - ld_TextRowHeight * 2.0);
			QDateTime lk_NextDate = QDateTime(lk_BorderDate.date().addDays(1));
			if (li_DrawDayLabels > 1)
			{
				QString ls_Text = QVariant(lk_BorderDate.date().day()).toString();
				if (ld_BorderX < 0.0)
				{
					ld_BorderX = 0.0;
					double ld_Delta = (lk_FontMetrics.width(ls_Text) + ld_FontHPadding * 2.0) - (((double)lk_NextDate.date().toJulianDay() - md_CenterJd) / md_PixelTime + ld_CenterX);
					if (ld_Delta > 0.0)
						ld_BorderX -= ld_Delta;
				}
				lk_Painter.setPen(QPen(TANGO_ALUMINIUM_4));
				lk_Painter.drawText(ld_FontHPadding + ld_BorderX, height() - ld_TextRowHeight * 2.0 - ld_FontDescent, ls_Text);
			}
			lk_BorderDate = lk_NextDate;
		}
	}
	if (li_DrawMonthLabels > 0)
	{
		ld_BorderX = 0.0;
		lk_BorderDate.setDate(QDate(lk_LeftDate.year(), lk_LeftDate.month(), 1));
		
		while (ld_BorderX < width())
		{
			ld_BorderX = ((double)lk_BorderDate.date().toJulianDay() - md_CenterJd) / md_PixelTime + ld_CenterX;
			lk_Painter.setPen(QPen(TANGO_ALUMINIUM_2));
			lk_Painter.drawLine(ld_BorderX, height() - ld_TextRowHeight * 2.0, ld_BorderX, height() - ld_TextRowHeight);
			QDateTime lk_NextDate = QDateTime(lk_BorderDate.date().addMonths(1));
			if (li_DrawMonthLabels > 0)
			{
				QString ls_Text;
				if (li_DrawMonthLabels == 1)
					ls_Text = QDate::shortMonthName(lk_BorderDate.date().month());
				else if (li_DrawMonthLabels == 2)
					ls_Text = QDate::longMonthName(lk_BorderDate.date().month());
				if (ld_BorderX < 0.0)
				{
					ld_BorderX = 0.0;
					double ld_Delta = (lk_FontMetrics.width(ls_Text) + ld_FontHPadding * 2.0) - (((double)lk_NextDate.date().toJulianDay() - md_CenterJd) / md_PixelTime + ld_CenterX);
					if (ld_Delta > 0.0)
						ld_BorderX -= ld_Delta;
				}
				lk_Painter.setPen(QPen(TANGO_ALUMINIUM_4));
				lk_Painter.drawText(ld_FontHPadding + ld_BorderX, height() - ld_TextRowHeight - ld_FontDescent, ls_Text);
			}
			lk_BorderDate = lk_NextDate;
		}
	}
	
	int li_YearSkip = 1;
	while (366.0 * li_YearSkip / md_PixelTime < ld_MaxYearLabelWidth + ld_FontHPadding * 2.0)
		li_YearSkip *= 10;
	ld_BorderX = 0.0;
	int li_BorderYear = floor((qreal)lk_LeftDate.year() / li_YearSkip) * li_YearSkip;
	if (li_BorderYear == 0)
		li_BorderYear = 1;
	if (li_BorderYear < -4700)
		li_BorderYear = -4700;
	lk_BorderDate.setDate(QDate(li_BorderYear, 1, 1));
	
	while (ld_BorderX < width())
	{
		ld_BorderX = ((double)lk_BorderDate.date().toJulianDay() - md_CenterJd) / md_PixelTime + ld_CenterX;
		lk_Painter.setPen(QPen(TANGO_ALUMINIUM_2));
		lk_Painter.drawLine(ld_BorderX, height() - ld_TextRowHeight, ld_BorderX, height());
		QString ls_Text = QVariant(abs(lk_BorderDate.date().year())).toString();
		QDateTime lk_NextDate = lk_BorderDate;
		if (lk_BorderDate.date().year() == 1 && li_YearSkip > 1)
			lk_NextDate = QDateTime(lk_BorderDate.addYears(li_YearSkip - 1));
		else
			lk_NextDate = QDateTime(lk_BorderDate.addYears(li_YearSkip));
		if (ld_BorderX < 0.0)
		{
			ld_BorderX = 0.0;
			
			double ld_Delta = (lk_FontMetrics.width(ls_Text) + ld_FontHPadding * 2.0) - (((double)lk_NextDate.date().toJulianDay() - md_CenterJd) / md_PixelTime + ld_CenterX);
			if (ld_Delta > 0.0)
				ld_BorderX -= ld_Delta;
		}
		lk_Painter.setPen(QPen(TANGO_ALUMINIUM_4));
		if (lk_BorderDate.date().year() < 0)
			ls_Text += " BC";
		lk_Painter.drawText(ld_FontHPadding + ld_BorderX, height() - ld_FontDescent, ls_Text);
		lk_BorderDate = lk_NextDate;
	}
	
	lk_Painter.setPen(QPen(TANGO_ALUMINIUM_4));
	
// 	lk_Painter.drawText(ld_FontHPadding, ld_TextRowHeight, lk_LeftDate.toString());
	QString ls_Text = lk_RightDate.toString();
// 	lk_Painter.drawText(width() - lk_FontMetrics.width(ls_Text) - ld_FontHPadding - ld_FontDescent, ld_TextRowHeight, ls_Text);
	
	lk_Painter.setRenderHint(QPainter::Antialiasing, true);
	int li_VisibleEventCount = 0;
	int li_LeftJd = (int)floor(ld_LeftJd - md_PixelTime * (mi_TextBoxWidth + 20));
	int li_RightJd = (int)ceil(ld_RightJd + md_PixelTime * (mi_TextBoxWidth + 20));
	if (li_LeftJd < 0)
		li_LeftJd = 0;
	
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
	QMultiMap<int, r_TextBoxInfo> lk_TextBoxes;
	
    double ld_BoxTime = md_PixelTime * (mi_TextBoxWidth + mi_BoxPadding);
    int li_OldBoxSlot = -1;
	for (int i = 0; i <= mi_EventCount; ++i)
	{
		bool lb_JumpIn = false;
		if (i == mi_EventCount)
			lb_JumpIn = true;
		else
			lb_JumpIn = (mi_EventJd_[i] >= li_LeftJd && mi_EventJd_[i] <= li_RightJd);
		
		if (lb_JumpIn)
		{
			int li_Jd = -1;
			if (i < mi_EventCount)
			{
				li_Jd = mi_EventJd_[i];
				li_Jd = (li_Jd / li_SnapWidth) * li_SnapWidth;
			}
			
			if (li_Jd != li_OldJd)
			{
				int li_OldJdEndIndex = li_LastInIndex;
				int li_OldJdEventCount = li_OldJdEndIndex - li_OldJdStartIndex + 1;
				if (li_OldJdStartIndex >= 0 && li_OldJdEventCount > 0)
				{
					// draw the li_OldJd (group)
					char* lc_Color_ = TANGO_ALUMINIUM_2;
					for (int k = li_OldJdStartIndex; k <= li_OldJdEndIndex; ++k)
						if (mk_HitIndices.contains(k))
							lc_Color_ = TANGO_SKY_BLUE_1;
					lk_Painter.setPen(QPen(Qt::NoPen));
					lk_Painter.setBrush(QBrush(lc_Color_));
					double x = ((double)li_OldJd - md_CenterJd) / md_PixelTime + ld_CenterX + 3.0 + ld_FontHPadding;
					double ld_Radius = 2.0 + log((double)li_OldJdEventCount) / log(8.0);
					lk_Painter.drawEllipse(QPointF(x, height() - ((double)ld_TextRowHeight * ((double)li_RowCount + 0.5))), ld_Radius, ld_Radius);
					double y = height() - ((double)ld_TextRowHeight * ((double)li_RowCount + 1.0));
					for (int k = li_OldJdStartIndex; k <= li_OldJdEndIndex; ++k)
					{
						if (!mk_HitIndices.contains(k))
							continue;
						double x = ((floor((double)mi_EventJd_[k] / ld_BoxTime) * ld_BoxTime) - md_CenterJd) / md_PixelTime + ld_CenterX + 3.0 + ld_FontHPadding;
                        double realx = ((double)mi_EventJd_[k] - md_CenterJd) / md_PixelTime + ld_CenterX + 3.0 + ld_FontHPadding;
						int li_BoxId = mi_EventJd_[k] / li_BoxSnapWidth;
						int li_BoxOffset = mi_EventJd_[k] % li_BoxSnapWidth;
						QString ls_Text = "<p><b>" + QDate::fromJulianDay(mi_EventJd_[k]).toString() + "</b> &ndash; " + mk_EventDescriptions[k] + "</p>";
/*						QTextDocument doc;
						doc.setHtml(ls_Text);
						doc.setHtml(doc.toPlainText());
						doc.setHtml(" &ndash; " + doc.toPlainText());
						ls_Text = doc.toPlainText();*/
						if (!lk_BoxSpanHeight.contains(li_BoxId))
							lk_BoxSpanHeight[li_BoxId] = 0.0;
						r_TextBoxInfo lr_TextBoxInfo;
						lr_TextBoxInfo.ms_Text = ls_Text;
						lr_TextBoxInfo.mi_BoxSpanId = li_BoxId;
						lr_TextBoxInfo.mi_BoxSpanOffset = li_BoxOffset;
						lr_TextBoxInfo.md_Position = x;
                        lr_TextBoxInfo.md_RealPosition = realx;
						lr_TextBoxInfo.mc_Type = mk_EventTypes[k];
						lk_TextBoxes.insert(mi_EventJd_[k], lr_TextBoxInfo);
					}
				}
				if (i == mi_EventCount)
					break;
				li_OldJd = li_Jd;
				li_OldJdStartIndex = i;
				ls_LastDescription = mk_EventDescriptions[i];
			}
			
			++li_VisibleEventCount;
			li_LastInIndex = i;
/*			QTextDocument doc;
			doc.setHtml(mk_EventDescriptions[i]);*/
			//doc.drawContents(&lk_Painter, QRectF(QPointF(((double)mi_EventJd_[i] - md_CenterJd) / md_PixelTime + ld_CenterX, height() - ((double)ld_TextRowHeight * ((double)li_RowCount + 0.5))), QSizeF(1000.0, ld_TextRowHeight)));

// 			lk_Painter.drawText(((double)mi_EventJd_[i] - md_CenterJd) / md_PixelTime + ld_CenterX, height() - ((double)ld_TextRowHeight * ((double)li_RowCount + 1.5)), doc.toPlainText());
		}
	}

	QMultiMap<int, r_TextBoxInfo>::const_iterator lk_Iter;
// 	printf("\n");
	double maxheight = 0.0;
	double maxwidth = 0.0;
    double ld_LastPosition = -1.0;
	for (lk_Iter = lk_TextBoxes.constBegin(); lk_Iter != lk_TextBoxes.constEnd(); ++lk_Iter)
	{
		r_TextBoxInfo lr_Info = lk_Iter.value();
		QTextDocument doc;
		doc.setDefaultStyleSheet(QString("* { font-family: Bitstream Charter; font-size: 12px; color: #000;} a { color: %1; text-decoration: none; }").arg(TANGO_SKY_BLUE_1));
		doc.setPageSize(QSizeF(mi_TextBoxWidth, height()));
		doc.setHtml(lr_Info.ms_Text);
		double x = lr_Info.md_Position;
		if (x > ld_LastPosition)
			maxheight = 0.0;
        ld_LastPosition = x;
		if (maxheight < height())
		{
			double ld_BoxSpanHeight = maxheight;
			double y = height() - ((double)ld_TextRowHeight * ((double)li_RowCount + 1.0)) - ld_BoxSpanHeight;
	// 		printf("%1.2f, %1.2f, %d %d\n", x, y, lr_Info.mi_BoxSpanId, lr_Info.mi_BoxSpanOffset);
	// 		QRectF lk_ResultRect = lk_FontMetrics.boundingRect(0, 0, 300, 600, Qt::TextWordWrap | Qt::AlignLeft, lr_Info.ms_Text);
			QRectF lk_ResultRect = QRectF(QPointF(0.0, 0.0), QSizeF(doc.idealWidth(), doc.size().height()));
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
            lk_ResultRect.setWidth(mi_TextBoxWidth);
			lk_Painter.drawRoundedRect(QRectF(QPointF(x + mi_BoxPadding / 2, y - lk_ResultRect.height() - 0.5), lk_ResultRect.size()), 4.0, 4.0);
            lk_Painter.drawPixmap(QPointF(lr_Info.md_RealPosition - 5.0, y - 6.0), mk_Arrow);
			//lk_Painter.drawText(QRectF(QPointF(x, y - lk_ResultRect.height()), lk_ResultRect.size()), Qt::TextWordWrap | Qt::AlignBottom | Qt::AlignLeft, lr_Info.ms_Text);
			lk_Painter.setViewport(QRectF(QPointF(x + mi_BoxPadding / 2, y - lk_ResultRect.height()), size()).toRect());
			doc.drawContents(&lk_Painter);
			lk_Painter.setViewTransformEnabled(false);
			double ld_Right = x + lk_ResultRect.width();
			maxwidth = std::max<double>(ld_Right, maxwidth);
			maxheight += lk_ResultRect.height() + mi_BoxPadding;
		}
	}
	lk_Painter.setRenderHint(QPainter::Antialiasing, false);
	lk_Painter.setPen(QPen(TANGO_SKY_BLUE_2));
// 	lk_Painter.drawText(ld_FontHPadding, ld_TextRowHeight * 2.0, QVariant(li_VisibleEventCount).toString() + " events");
	
	lk_Painter.setPen(QPen(TANGO_ALUMINIUM_2));
	for (int i = 1; i < li_RowCount + 1; ++i)
		lk_Painter.drawLine(0.0, height() - ld_TextRowHeight * i, width(), height() - ld_TextRowHeight * i);

	lk_Painter.setPen(QPen(TANGO_ALUMINIUM_2));
	lk_Painter.setBrush(QBrush(Qt::NoBrush));
	lk_Painter.drawRect(0, 0, width() - 1, height() - 1);
}


void k_TimeLine::mouseMoveEvent(QMouseEvent* ak_Event_)
{
	mk_MouseMovePosition = ak_Event_->pos();
	if (mb_Moving)
	{
		md_CenterJd = md_MoveStartCenterJd + (double)(mk_MoveStartPoint.x() - ak_Event_->pos().x()) * md_PixelTime;
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
	md_PixelTime *= 1.0 - (double)ak_Event_->delta() * 0.001;
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
	double ld_CenterX = (double)width() * 0.5;
	double ld_LeftJd = md_CenterJd - ld_CenterX * md_PixelTime;
	double ld_RightJd = md_CenterJd + ld_CenterX * md_PixelTime;
	if (ld_LeftJd < 4748.0)
	{
		md_CenterJd += 4748.0 - ld_LeftJd;
		ld_LeftJd = md_CenterJd - ld_CenterX * md_PixelTime;
		ld_RightJd = md_CenterJd + ld_CenterX * md_PixelTime;
	}
	if (ld_RightJd > 5373468.0)
	{
		md_CenterJd += 5373468.0 - ld_RightJd;
		ld_LeftJd = md_CenterJd - ld_CenterX * md_PixelTime;
		ld_RightJd = md_CenterJd + ld_CenterX * md_PixelTime;
	}
	if (ld_LeftJd < 4748.0)
	{
		md_CenterJd = (3437698.0 + 4748.0) * 0.5;
		md_PixelTime = (md_CenterJd - 4748.0) / ld_CenterX;
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
