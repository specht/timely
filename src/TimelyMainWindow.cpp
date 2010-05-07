#include "TimelyMainWindow.h"
#include "Tango.h"


k_TimelyMainWindow::k_TimelyMainWindow()
    : QMainWindow()
{
    resize(800, 500);
    QWidget* lk_Widget_ = new QWidget(this);
    setCentralWidget(lk_Widget_);
    QBoxLayout* lk_HLayout_ = new QHBoxLayout();
    mk_SearchLineEdit_ = new QLineEdit(this);
    mk_SearchButton_ = new QPushButton("Search", this);
    lk_HLayout_->addWidget(mk_SearchLineEdit_);
    lk_HLayout_->addWidget(mk_SearchButton_);
    QBoxLayout* lk_VLayout_ = new QVBoxLayout(lk_Widget_);
    lk_VLayout_->addLayout(lk_HLayout_);
    lk_VLayout_->addWidget(&mk_TimeLine);
    connect(mk_SearchButton_, SIGNAL(clicked()), this, SLOT(searchButtonClicked()));
    connect(mk_SearchLineEdit_, SIGNAL(returnPressed()), this, SLOT(searchButtonClicked()));
}


k_TimelyMainWindow::~k_TimelyMainWindow()
{
}


void k_TimelyMainWindow::searchButtonClicked()
{
    mk_TimeLine.searchFor(mk_SearchLineEdit_->text());
}
