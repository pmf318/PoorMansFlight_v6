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
#include <QListWidget>
#include <QMessageBox>
#include <QGridLayout>

#include <gstring.hpp>
#include <gseq.hpp>
#include <qfiledialog.h>
#include <qstring.h>
#include <qfile.h>
#include <qtextstream.h>

#include <QTableWidget>
#include <QComboBox>

#include <dsqlplugin.hpp>

#include <gseq.hpp>


#ifndef _CONNSET_
#define _CONNSET_

#define CONNSET_MODE_NORM 0
#define CONNSET_MODE_EMBED 1

static GString _CON_NO_SSL  = " sslmode=disable";
static GString _CON_ALLOW_SSL  = " sslmode=allow";
static GString _CON_PREFER_SSL  = " sslmode=prefer";
static GString _CON_REQUIRE_SSL  = " sslmode=require";


class ConnSet : public QDialog
{
   Q_OBJECT
public:
    ConnSet(GDebug *pGDeb = NULL, QWidget *parent = 0);
    ~ConnSet();
    void initConnSet(int mode = CONNSET_MODE_NORM);
	GString createCfgFile();
	GString getValue(GString key);
    int setConnectionSettingsFromFile(CON_SET *pConSet);
    void getStoredCons(GString dbType, GSeq <CON_SET*> * pSeq);
    CON_SET* getDefaultCon();
    CON_SET* getConSet(GString dbType, GString dbName);

    int isInList(CON_SET * pCS);
    void addToList(CON_SET* pCS);
    void addToList( GString dbType, GString dbName, GString hostName, GString userName, GString password, GString port, GString pwdCmd, GString options, GString color, int defaultDb = 0);

    int updSeq(CON_SET* pNewCS);
    void closeEvent(QCloseEvent * event);

    int removeFromList(CON_SET *pNewCS);
    int removeHostFromList(GString dbType, GString hostName);
    int removeFromList(GString type, GString dbName);
    GString errorMessages(){ return m_strErrorMessages; }
    void setDefaultVals(CON_SET* pCS);
    void setDefaultConnection(CON_SET* pCS);
    GString getPwdCmd(GString type, GString db, GString host, GString port, GString uid);
    GString getConnectionColor(GString type, GString db, GString host, GString port, GString uid);
    int getReconnectTimeout(GString type, GString db, GString host, GString port, GString uid);
    int setConnectionColor(GString type, GString db, GString host, GString port, GString uid, GString color);
    int guiHasChanges();
    int replace(CON_SET *pOldCS, CON_SET *pNewCS);
    CON_SET* findConnSet(GString type, GString db, GString host, GString port, GString uid);    
    int hasUnsavedChanges();

public slots:
    int save();
	int saveAsXml();
	void keyPressEvent(QKeyEvent *event);	
    void sortClicked(int);

private slots:
	void cancel();
	void editEntry();
	void delEntry();
    void testEntry();
    void defaultSelected(int);
	GString cfgFileName();
    GString cfgFileNameXml();
    void getAllDatabases(int pos);
    void showConnSet(CON_SET *pCS);
    void listDoubleClicked(QTableWidgetItem*);
    void NewEntryClicked();

private:
	QLineEdit *dbLE, *uidLE, *pwdLE;
    QPushButton * ok, *esc, *edit, *rem, *test, *newEntry;
	QTableWidget * m_twMain;
    int createRow(CON_SET * pConSet);
    QWidget * m_pMainWdgt;
    int m_iMode;

    void addToSeq(GSeq <CON_SET* > *pSeqConSet, CON_SET* pCS);
    void clearSeq(GSeq <CON_SET*> *pSeq);
    int isInSeq(GSeq <CON_SET* > *pSeqConSet, CON_SET * pCS);
	void tm(QString message){QMessageBox::information(this, "PMF Connections ", message);}
    void msg(GString message);
    void deb(GString msg);
    void migrateData();
    void moveToXmlFormat();
    int loadFromFile();
    int loadFromXmlFile();
    int findDatabases(GString dbType);
    void writeToSeq();
    void createRows();
    void showEditConn(QTableWidgetItem * pItem);
    void fillSeq(CON_SET* pCS, QTableWidgetItem * pItem);
    void setRowBackGroundColor(int row, GString rgbColor);
    int m_iMyInstanceCounter;
	int m_iChanged;
    GDebug * m_pGDeb;
    int findElementPos(CON_SET *pNewCS);
    GSeq <CON_SET* > m_seqFileCons;
    GSeq <CON_SET* > m_seqCatalogCons;
    GString m_strErrorMessages;
    int saveAsXml2();


};

#endif
