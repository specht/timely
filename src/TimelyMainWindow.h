#pragma once

#include <QtGui>
#include "TimeLine.h"


class k_TimelyMainWindow: public QMainWindow
{
	Q_OBJECT
public:
	k_TimelyMainWindow();
	virtual ~k_TimelyMainWindow();
	
protected slots:
	void searchButtonClicked();
	
protected:
	k_TimeLine mk_TimeLine;
	QLineEdit* mk_SearchLineEdit_;
	QPushButton* mk_SearchButton_;
};
