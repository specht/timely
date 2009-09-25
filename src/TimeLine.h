#pragma once

#include <QtGui>


class k_TimeLine: public QWidget
{
	Q_OBJECT
public:
	k_TimeLine();
	virtual ~k_TimeLine();
	
public slots:
	virtual void searchFor(QString as_Text);
	
protected slots:
	virtual void steerTimeout();
	
protected:
	virtual void loadData();
	
	virtual void paintEvent(QPaintEvent* ak_Event_);
	virtual void mouseMoveEvent(QMouseEvent* ak_Event_);
	virtual void mousePressEvent(QMouseEvent* ak_Event_);
	virtual void mouseReleaseEvent(QMouseEvent* ak_Event_);
	virtual void wheelEvent(QWheelEvent* ak_Event_);
	virtual void keyPressEvent(QKeyEvent* ak_Event_);
	
	virtual void fixView();
	virtual void setAnimationInterval();
	
	QFont mk_Font;
	double md_CenterJd;
	double md_PixelTime;
	
	bool mb_Moving;
	bool mb_Steering;
	QPoint mk_MoveStartPoint;
	double md_MoveStartCenterJd;
	
	QTimer mk_AnimationTimer;
	QTimer mk_SteeringTimer;
	QTime mk_SteeringLastTime;
	QPoint mk_MouseMovePosition;
	
	int mi_EventCount;
	QMultiMap<int, QString> mk_Events;
	int* mi_EventJd_;
	QList<QString> mk_EventDescriptions;
	QList<QChar> mk_EventTypes;
	QSet<int> mk_HitIndices;
	
	int mi_TextBoxWidth;
};
