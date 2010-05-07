#include <QtGui>
#include <QtCore>
#include "TimelyMainWindow.h"


int main(int ai_ArgumentCount, char** ac_Arguments__)
{
    Q_INIT_RESOURCE(timely);
    QApplication lk_Application(ai_ArgumentCount, ac_Arguments__);
    
    k_TimelyMainWindow lk_MainWindow;
    lk_MainWindow.show();
    
    return lk_Application.exec();
}
