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


class ConnSet : public QDialog
{
   Q_OBJECT
public:
    ConnSet(GDebug *pGDeb = NULL, QWidget *parent = 0);
    ~ConnSet();
	GString createCfgFile();
	GString getValue(GString key);
    int setConnectionSettingsFromFile(CON_SET *pConSet);
    void getStoredCons(GString dbType, GSeq <CON_SET*> * pSeq);
    CON_SET* getDefaultCon();
    CON_SET* getConSet(GString dbType, GString dbName);

    int isInList(CON_SET * pCS);
    void addToList(CON_SET* pCS);
    void addToList( GString dbType, GString dbName, GString hostName, GString userName, GString password, GString port, int defaultDb = 0);

    int updSeq(CON_SET* pNewCS);
    void closeEvent(QCloseEvent * event);

    int removeFromList(CON_SET *pNewCS);
    int removeFromList(GString type, GString dbName);
    GString errorMessages(){ return m_strErrorMessages; }
    void setDefaultVals(CON_SET* pCS);
    void setDefaultConnection(CON_SET* pCS);

public slots:
    int save();
	void keyPressEvent(QKeyEvent *event);	
    void sortClicked(int);

private slots:
	void cancel();
	void newEntry();
	void delEntry();
    void testEntry();
    void defaultSelected(int);
	GString cfgFileName();
    void getAllDatabases(int pos);
    void showConnSet(CON_SET *pCS);

private:
	QLineEdit *dbLE, *uidLE, *pwdLE;
    QPushButton * ok, *esc, *add, *rem, *test;
	QTableWidget * m_twMain;
    int createRow(CON_SET * pConSet);
    QWidget * m_pMainWdgt;

    void addToSeq(GSeq <CON_SET* > *pSeqConSet, CON_SET* pCS);
    void clearSeq(GSeq <CON_SET*> *pSeq);
    int isInSeq(GSeq <CON_SET* > *pSeqConSet, CON_SET * pCS);
	void tm(QString message){QMessageBox::information(this, "PMF Connections ", message);}
    void deb(GString msg);
    void migrateData();
    int loadFromFile();
    int findDatabases(GString dbType);
    void writeToSeq();
    void createRows();
    int m_iMyInstanceCounter;
	int m_iChanged;
    GDebug * m_pGDeb;
    int findElementPos(CON_SET *pNewCS);
    GSeq <CON_SET* > m_seqFileCons;
    GSeq <CON_SET* > m_seqCatalogCons;
    GString m_strErrorMessages;

};

#endif
