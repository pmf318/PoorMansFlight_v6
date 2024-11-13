//
//  This file is part of PMF (Poor Man's Flight), see README.PMF for details
//  Author: Gregor Leipelt (gregor@leipelt.de)  (C) G. Leipelt 2000
//

#include <qapplication.h>
#include <qpushbutton.h>
#include <qmainwindow.h>
#include <qlineedit.h>
#include <qdialog.h>
#include <qlabel.h>
#include <qradiobutton.h>
#include <qbuttongroup.h>
#include <qlistwidget.h>

#include <QTableWidget>
#include <QGridLayout>
#include <gstring.hpp>
#include <qfiledialog.h>
#include <qstring.h>
#include <qfile.h>
#include <qtextstream.h>
#include <qtabwidget.h>
#include <qcheckbox.h>
#include <qcombobox.h>
#include <dsqlplugin.hpp>
#include <idsql.hpp>


#include <gthread.hpp>
#include "threadBox.h"
#include "connSet.h"
#include "catalogDB.h"
#include "catalogInfo.h"
#include "addDatabaseHost.h"


#ifndef _OPTIONSTAB_
#include "optionsTab.h"
#endif

#ifndef _NEWCONN_
#define _NEWCONN_



class NewConn : public QDialog
{
private:
   Q_OBJECT

public:
    NewConn(QWidget *parent, GDebug * pDeb = NULL);
    ~NewConn();
    void msg(QString message){QMessageBox::information(this, "PMF", message);}
    GString lastSelectedDbType();

public slots:
    void saveClicked();

private slots:
    void endIt();
    void toggleTab();
    void actionSelected();
    void setSecondTab(GString txt);
    void tabChanged(const int &pIndex);
    void showHelp();


private:
	void deb(GString msg);

    void addFirstTab();
    void addSecondTab();
    void clean();
    GSeq <CON_SET*> m_pConnSetList;
    QTabWidget * m_tabWdgt;
    QWidget * m_pParent;
    DSQLPlugin* m_pDSQL;
    QRadioButton * _newDbRB;
    QWidget * m_pActionWdgt;
    QPushButton * m_nextBT, *m_saveBT;
    GDebug * m_pGDeb;
    GString m_strLastSelDbType;

    ConnSet * m_pConnSet;
    CatalogDB *m_pCatalogDB;
    CatalogInfo * m_pCatalogInfo;
    AddDatabaseHost * m_pAddDatabaseHost;

};

#endif
