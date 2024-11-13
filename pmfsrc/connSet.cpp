//
//  This file is part of PMF (Poor Man's Flight), see README.PMF for details
//  Author: Gregor Leipelt (gregor@leipelt.de)  (C) G. Leipelt 2000
//

#include "connSet.h"
#include "pmfdefines.h"
#include <gfile.hpp>

#include <QGridLayout>
#include <QHeaderView>
#include <QDir>
#include <QCheckBox>


#include <QGroupBox>
#include <QObject>

#include "gstuff.hpp"
#include "helper.h"
#include "pmfdefines.h"
#include "editConn.h"
#include "gxml.hpp"

#define ConnSetVer 7

static int m_iConnSetInstanceCounter = 0;

#define ColCount 11
enum ColNum
{
    CN_TYPE = 0,
    CN_DB,
    CN_UID,
    CN_PWD,
    CN_HOST,
    CN_PORT,
    CN_PWDCMD,
    CN_RECONN,
    CN_DEF,
    CN_COLOR,
    CN_OPTIONS
};

ConnSet::ConnSet(GDebug *pGDeb, QWidget *parent)
  :QDialog(parent)
{
    m_pGDeb = pGDeb;
    m_pMainWdgt = parent;
    m_iChanged = 0;
}

ConnSet::~ConnSet()
{
    deb("connSet: destroying instance "+GString(m_iMyInstanceCounter));
    clearSeq(&m_seqFileCons);
    clearSeq(&m_seqCatalogCons);
    deb("connSet: destroying instance "+GString(m_iMyInstanceCounter)+" done.");
    m_iConnSetInstanceCounter--;
}

void ConnSet::initConnSet(int mode)
{
    m_iMode = mode;
    m_twMain = NULL;
    m_strErrorMessages = "";

/* Either this...
    moveToXmlFormat();
    loadFromXmlFile();
*/
//...or this
    loadFromFile();


    deb("connSet start");
    //loadFromFile();
//    GSeq <PLUGIN_DATA*> list;
//    DSQLPlugin::DBTypeNames(&list);

    //This will add all DBs from node directory. We don't want that anymore.
    //    GSeq <PLUGIN_DATA*> list;
    //    DSQLPlugin::DBTypeNames(&list);
    //    for( unsigned long i = 1; i <= list.numberOfElements(); ++i)
    //    {
    //        deb("connSet, loading databases for "+list.elementAtPosition(i));
    //        findDatabases(list.elementAtPosition(i));
    //    }
    m_iChanged = 0;

    m_iConnSetInstanceCounter++;
    m_iMyInstanceCounter = m_iConnSetInstanceCounter;
    deb("connSet: Created instance "+GString(m_iMyInstanceCounter));

    migrateData();
    //This class may be called w/o parent to retrieve connection settings.
    if( !m_pMainWdgt ) return;

    QGridLayout * pMainGrid = new QGridLayout(this);

    this->resize(750, 400);
    m_twMain = new QTableWidget(this);
    m_twMain->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_twMain->setSortingEnabled(true);
    m_twMain->setColumnCount(ColCount);
    pMainGrid->addWidget(m_twMain, 0 , 0, 1, 3);

    if( m_iMode == CONNSET_MODE_NORM)
    {
        ok   = new QPushButton("Save", this);
        connect(ok,   SIGNAL(clicked()), SLOT(save()));
        esc  = new QPushButton("Cancel", this);
        connect(esc,  SIGNAL(clicked()), SLOT(cancel()));
    }
    edit  = new QPushButton("Edit", this);
    edit->setMaximumWidth(200);
    rem  = new QPushButton("Delete", this);
    rem->setMaximumWidth(200);
    newEntry = new QPushButton("New/Clone", this);
    newEntry->setMaximumWidth(200);
    connect(edit,  SIGNAL(clicked()), SLOT(editEntry()));
    connect(rem,  SIGNAL(clicked()), SLOT(delEntry()));
    connect(newEntry, SIGNAL(clicked()), SLOT(NewEntryClicked()));

    QGroupBox * lowerBox = new QGroupBox();
    QHBoxLayout *lowerLayout = new QHBoxLayout;
    lowerLayout->addWidget(newEntry);
    lowerLayout->addWidget(edit);
    lowerLayout->addWidget(rem);

    if( m_iMode == CONNSET_MODE_NORM)
    {
        lowerLayout->addWidget(ok);
        lowerLayout->addWidget(esc);
    }
    lowerBox->setLayout(lowerLayout);
    pMainGrid->addWidget(lowerBox, 5 , 0, 1, 3);

    QTableWidgetItem * pItem1 = new QTableWidgetItem("Type");
    m_twMain->setHorizontalHeaderItem(CN_TYPE, pItem1);
    QTableWidgetItem * pItem2 = new QTableWidgetItem("Database");
    m_twMain->setHorizontalHeaderItem(CN_DB, pItem2);
    QTableWidgetItem * pItem3 = new QTableWidgetItem("Username");
    m_twMain->setHorizontalHeaderItem(CN_UID, pItem3);
    QTableWidgetItem * pItem4 = new QTableWidgetItem("Password");
    m_twMain->setHorizontalHeaderItem(CN_PWD, pItem4);
    QTableWidgetItem * pItem5 = new QTableWidgetItem("Host");
    m_twMain->setHorizontalHeaderItem(CN_HOST, pItem5);
    QTableWidgetItem * pItem6 = new QTableWidgetItem("Port");
    m_twMain->setHorizontalHeaderItem(CN_PORT, pItem6);
    QTableWidgetItem * pItem7 = new QTableWidgetItem("Pwd Cmd");
    m_twMain->setHorizontalHeaderItem(CN_PWDCMD, pItem7);
    QTableWidgetItem * pItem8 = new QTableWidgetItem("Default");
    m_twMain->setHorizontalHeaderItem(CN_DEF, pItem8);
    QTableWidgetItem * pItem9 = new QTableWidgetItem("Color");
    m_twMain->setHorizontalHeaderItem(CN_COLOR, pItem9);
    QTableWidgetItem * pItem10 = new QTableWidgetItem("Options");
    m_twMain->setHorizontalHeaderItem(CN_OPTIONS, pItem10);

    QTableWidgetItem * pItem11 = new QTableWidgetItem("Timeout");
    m_twMain->setHorizontalHeaderItem(CN_RECONN, pItem11);

    //m_twMain->setColumnHidden(8, true);
    m_twMain->setColumnHidden(9, true);
    m_twMain->setColumnHidden(10, true);

    QLabel * warn = new QLabel(this);
    warn->setText("WARNING: Passwords will be stored weakly encrypted in '"+_CONNSET_FILE+"' in your home directory");
    pMainGrid->addWidget(warn, 6, 0);


    //Read CFG and display data
    //loadAll() was called right at the start
    createRows();
    connect((QWidget*)m_twMain->horizontalHeader(), SIGNAL(sectionClicked(int)), this, SLOT(sortClicked(int)));
    connect(m_twMain, SIGNAL(itemDoubleClicked(QTableWidgetItem*)), this, SLOT(listDoubleClicked(QTableWidgetItem*)));
    Helper::setVHeader(m_twMain);
}

void ConnSet::clearSeq(GSeq <CON_SET*> *pSeq)
{
    CON_SET* pCS;
    deb("connSet: clearSeq, elmts: "+GString(pSeq->numberOfElements()));
    while( !pSeq->isEmpty() )
    {
        pCS = pSeq->firstElement();
        delete pCS;
        pSeq->removeFirst();
    }
    deb("connSet: clearSeq ok");
}

int ConnSet::saveAsXml()
{
    m_strErrorMessages = "";
    if( m_pMainWdgt)
    {
        writeToSeq();
    }
    GString out;
    CON_SET *pCS;
    GSeq <GString> seqAll;
    out = "<?xml version=\"1.0\" encoding=\"utf-8\"?>";
    seqAll.add(out);
    out = "<Connections>";
    seqAll.add(out);

    for(int i = 1; i <= (int)m_seqFileCons.numberOfElements(); ++ i)
    {
        pCS = m_seqFileCons.elementAtPosition(i);
        pCS->CSVer = ConnSetVer;
        out = "<Connection ";
        out += "isDefault=\""+GString(pCS->DefDB)+"\" ";
        out += "type=\""+pCS->Type+"\" ";
        out += "db=\""+pCS->DB+"\" ";
        out += "uid=\""+pCS->UID+"\" ";
        out += "pwd=\""+GStuff::encryptToBase64(pCS->PWD)+"\" ";
        out += "host=\""+pCS->Host+"\" ";
        out += "port=\""+pCS->Port+"\" ";
        out += "pwdCmd=\""+pCS->PwdCmd+"\" ";
        out += "color=\""+pCS->Color+"\" ";
        out += "options=\""+pCS->Options+"\" ";
        out += "reconnTimeout=\""+GString(pCS->ReconnTimeout)+"\" ";
        out += "version=\""+GString(pCS->CSVer)+"\" />";
        seqAll.add(out);
    }
    out = "</Connections>";
    seqAll.add(out);

    deb("connSet: cfgFileName: "+ cfgFileNameXml());
    Helper::createFileIfNotExist(cfgFileNameXml());
    GFile f( cfgFileNameXml(), GF_OVERWRITE );
    if( f.initOK() )    {
        f.addLine("-- DO NOT CHANGE --");
        f.addLine("Version 7");
        f.addLine("-- END --");

        f.overwrite(&seqAll);
    }
    else deb("connSet: cfgFileName: "+ cfgFileName()+" FAILED");
    m_iChanged = 0;
    msg("Please connect/reconnect for changes to take effect.");
    if( m_iMode == CONNSET_MODE_NORM ) close();
    return 0;

}

int ConnSet::save()
{
    m_strErrorMessages = "";
    if( m_pMainWdgt)
    {
        writeToSeq();
    }
    GString out;
    CON_SET *pCS;
    GSeq <GString> seqAll;
    for(int i = 1; i <= (int)m_seqFileCons.numberOfElements(); ++ i)
    {
        pCS = m_seqFileCons.elementAtPosition(i);
        pCS->CSVer = ConnSetVer;
        out = "";
        out += GString(pCS->DefDB)+_PMF_PASTE_SEP;
        out += pCS->Type+_PMF_PASTE_SEP;
        out += pCS->DB+_PMF_PASTE_SEP;
        out += pCS->UID+_PMF_PASTE_SEP;
        out += GStuff::encryptToBase64(pCS->PWD)+_PMF_PASTE_SEP;
        out += pCS->Host+_PMF_PASTE_SEP;
        out += pCS->Port+_PMF_PASTE_SEP;
        out += pCS->PwdCmd+_PMF_PASTE_SEP;
        out += pCS->Color+_PMF_PASTE_SEP;
        out += pCS->Options+_PMF_PASTE_SEP;
        out += GString(pCS->ReconnTimeout)+_PMF_PASTE_SEP;
        out += pCS->CSVer;
		
		seqAll.add(out);
	}
    deb("connSet: cfgFileName: "+ cfgFileName());
    Helper::createFileIfNotExist(cfgFileName());
	GFile f( cfgFileName(), GF_OVERWRITE );
    if( f.initOK() )    {
        f.addLine("-- DO NOT CHANGE --");
        f.addLine("Version 4");
        f.addLine("-- END --");

        f.overwrite(&seqAll);
    }
    else deb("connSet: cfgFileName: "+ cfgFileName()+" FAILED");
	m_iChanged = 0;
    msg("Please connect/reconnect for changes to take effect.");
    if( m_iMode == CONNSET_MODE_NORM ) close();
    return 0;
}	

void ConnSet::writeToSeq()
{

    QCheckBox * pChkBx;
    QLineEdit * pLE;
    CON_SET * pConSet;

    clearSeq(&m_seqFileCons);
    for(int i = 0; i < m_twMain->rowCount(); ++i )
    {
        pConSet = new CON_SET;
        pConSet->init();

        pChkBx = qobject_cast<QCheckBox*>(m_twMain->cellWidget(i, CN_DEF));
        if( pChkBx->checkState() == Qt::Checked ) pConSet->DefDB = 1;
        else pConSet->DefDB = 0;	//Default 0/1

        pConSet->Type = m_twMain->item(i, CN_TYPE)->text();
        pConSet->DB   = m_twMain->item(i, CN_DB)->text();
        pConSet->UID = m_twMain->item(i, CN_UID)->text();

        pLE = qobject_cast<QLineEdit*>(m_twMain->cellWidget(i, CN_PWD));
        pConSet->PWD = pLE->text();//password

        pConSet->Host = m_twMain->item(i, CN_HOST)->text();
        pConSet->Port = m_twMain->item(i, CN_PORT)->text();
        pConSet->PwdCmd = m_twMain->item(i, CN_PWDCMD)->text();
        pConSet->Color = m_twMain->item(i, CN_COLOR)->text();
        pConSet->Options = m_twMain->item(i, CN_OPTIONS)->text();
        pConSet->ReconnTimeout = GString(m_twMain->item(i, CN_RECONN)->text()).asInt();
        pConSet->CSVer = ConnSetVer;

        addToSeq(&m_seqFileCons, pConSet);
    }
}

void ConnSet::createRows()
{
    for(int i = 1; i <= (int)m_seqFileCons.numberOfElements(); ++i) createRow(m_seqFileCons.elementAtPosition(i));
}

void ConnSet::keyPressEvent(QKeyEvent * key)
{
//    if( m_iMode != CONNSET_MODE_NORM)
//    {
//        QWidget::keyPressEvent(key);
//        this->close();
//        return;
//    }
//    if( key->key() == Qt::Key_Escape )	close();

    if( key->key() == Qt::Key_Escape )
    {
        if( hasUnsavedChanges() ) return;
    }
    QWidget::keyPressEvent(key);
}

int ConnSet::hasUnsavedChanges()
{
    if( m_iChanged )
    {
        if( QMessageBox::question(this, "PMF", "Quit without saving?", QMessageBox::Yes, QMessageBox::No) == QMessageBox::No )
        {
            return 1;
        }
    }
    return 0;
}

void ConnSet::closeEvent(QCloseEvent * event)
{
    if( hasUnsavedChanges() ) event->ignore();
//    if( m_iMode != CONNSET_MODE_NORM) return;
}

void ConnSet::cancel()
{
    this->close();
}

int ConnSet::removeHostFromList(GString dbType, GString hostName)
{
    CON_SET *cs;
    for(int i = 1; i <= (int)m_seqFileCons.numberOfElements(); ++i)
    {
        cs = m_seqFileCons.elementAtPosition(i);
        if( cs->Host == hostName && cs->Type == dbType )
        {
            m_seqFileCons.removeAt(i);
            return i;
        }
    }
    return -1;
}

int ConnSet::removeFromList(GString dbType, GString dbName)
{
    CON_SET cs;
    cs.Type = dbType;
    cs.DB = dbName;
    return removeFromList(&cs);
}


int ConnSet::removeFromList(CON_SET *pNewCS)
{
    int pos = findElementPos(pNewCS);
    if( pos < 0 ) return 1;
    m_seqFileCons.removeAt(pos);
    return 0;
}

int ConnSet::replace(CON_SET *pOldCS, CON_SET *pNewCS)
{
    CON_SET *cs;
    for(int i = 1; i <= (int)m_seqFileCons.numberOfElements(); ++i)
    {
        cs = m_seqFileCons.elementAtPosition(i);
        if( pOldCS->Host == cs->Host && pOldCS->Port == cs->Port &&
             pOldCS->UID == cs->UID && pOldCS->PWD == cs->PWD &&
             pOldCS->PwdCmd == cs->PwdCmd && pOldCS->Color == cs->Color
        )
        {
            m_seqFileCons.replaceAt(i, pNewCS);
            return 0;
        }
    }
    return 1;
}

int ConnSet::findElementPos(CON_SET *pNewCS)
{
    CON_SET *cs;
    for(int i = 1; i <= (int)m_seqFileCons.numberOfElements(); ++i)
    {
        cs = m_seqFileCons.elementAtPosition(i);
        if( cs->DB == pNewCS->DB && cs->Type == pNewCS->Type )
        {
            cs->Host    = pNewCS->Host;
            cs->Port    = pNewCS->Port;
            cs->UID     = pNewCS->UID;
            cs->PWD     = pNewCS->PWD;
            cs->PwdCmd  = pNewCS->PwdCmd;
            return i;
        }
    }
    return -1;
}

int ConnSet::updSeq(CON_SET *pNewCS)
{
    CON_SET *cs;
    int pos = findElementPos(pNewCS);
    if( pos < 0 ) return 1;
    cs = m_seqFileCons.elementAtPosition(pos);
    cs->Host = pNewCS->Host;
    cs->Port = pNewCS->Port;
    cs->UID  = pNewCS->UID;
    cs->PWD  = pNewCS->PWD;
    cs->PwdCmd  = pNewCS->PwdCmd;
    cs->Color  = pNewCS->Color;
    cs->CSVer  = ConnSetVer;
    return 0;
}

void ConnSet::addToList( GString dbType, GString dbName, GString hostName, GString userName, GString password, GString port, GString pwdCmd,  GString options, GString color, int defaultDb)
{
    CON_SET * pCS = new CON_SET;
    pCS->init();

    pCS->DefDB = defaultDb;
    pCS->Type = dbType;
    pCS->DB = dbName;
    pCS->Host = hostName;
    pCS->UID = userName;
    pCS->Port = port;
    pCS->PWD = password;
    pCS->PwdCmd = pwdCmd;
    pCS->Color = color;
    pCS->Options = options;
    addToList(pCS);
}

void ConnSet::addToList( CON_SET* pCS)
{
    addToSeq(&m_seqFileCons, pCS);
}

void ConnSet::addToSeq(GSeq <CON_SET* > *pSeqConSet, CON_SET* pCS)
{
    setDefaultVals(pCS);
    if(!isInSeq(pSeqConSet, pCS))
    {
        deb("ConnSet, addingToList ");
        //showConnSet(pCS);
        pSeqConSet->add(pCS);
    }
}

void ConnSet::getStoredCons(GString dbType, GSeq <CON_SET*> * pSeq)
{
    deb("getStoredCons, start");
    clearSeq(pSeq);
    clearSeq(&m_seqCatalogCons);

    CON_SET * pCS;
    for(int i = 1; i <= (int)m_seqFileCons.numberOfElements(); ++i)
    {
        if( m_seqFileCons.elementAtPosition(i)->Type != dbType) continue;

        pCS = new CON_SET;
        pCS->init();
        pCS->DB    = m_seqFileCons.elementAtPosition(i)->DB;
        pCS->DefDB = m_seqFileCons.elementAtPosition(i)->DefDB;
        pCS->Host  = m_seqFileCons.elementAtPosition(i)->Host;
        pCS->Port  = m_seqFileCons.elementAtPosition(i)->Port;
        pCS->PWD   = m_seqFileCons.elementAtPosition(i)->PWD;
        pCS->Type  = m_seqFileCons.elementAtPosition(i)->Type;
        pCS->UID   = m_seqFileCons.elementAtPosition(i)->UID;
        pCS->PwdCmd= m_seqFileCons.elementAtPosition(i)->PwdCmd;
        pCS->Color= m_seqFileCons.elementAtPosition(i)->Color;
        pCS->ReconnTimeout = m_seqFileCons.elementAtPosition(i)->ReconnTimeout;
        pCS->CSVer = m_seqFileCons.elementAtPosition(i)->CSVer;
        addToSeq(pSeq, pCS);
    }
    deb("getStoredCons, calling findDatabases for type "+dbType);
    findDatabases(dbType);
    deb("getStoredCons, calling findDatabases for type "+dbType+", done.");
    for(int i = 1; i <= (int)m_seqCatalogCons.numberOfElements(); ++i)
    {
        if( m_seqCatalogCons.elementAtPosition(i)->Type != dbType) continue;

        pCS = new CON_SET;
        pCS->init();
        pCS->DB    = m_seqCatalogCons.elementAtPosition(i)->DB;
        pCS->DefDB = m_seqCatalogCons.elementAtPosition(i)->DefDB;
        pCS->Host  = m_seqCatalogCons.elementAtPosition(i)->Host;
        pCS->Port  = m_seqCatalogCons.elementAtPosition(i)->Port;
        pCS->PWD   = m_seqCatalogCons.elementAtPosition(i)->PWD;
        pCS->Type  = m_seqCatalogCons.elementAtPosition(i)->Type;
        pCS->UID   = m_seqCatalogCons.elementAtPosition(i)->UID;
        pCS->PwdCmd= m_seqCatalogCons.elementAtPosition(i)->PwdCmd;
        pCS->ReconnTimeout = m_seqCatalogCons.elementAtPosition(i)->ReconnTimeout;
        pCS->Color= m_seqCatalogCons.elementAtPosition(i)->Color;
        pCS->CSVer = m_seqCatalogCons.elementAtPosition(i)->CSVer;
        addToSeq(pSeq, pCS);
    }
}

void ConnSet::setDefaultConnection(CON_SET *pCS)
{
    int pos = 0;
    CON_SET * cs;
    for(int i = 1; i <= (int)m_seqFileCons.numberOfElements(); ++i)
    {
        cs = m_seqFileCons.elementAtPosition(i);
        if( cs->DB == pCS->DB && cs->Type == pCS->Type && cs->Host == pCS->Host && cs->Port == pCS->Port)
        {
            pos = i;
            break;
        }
    }
    if( pos == 0 ) return;
    for(int i = 1; i <= (int)m_seqFileCons.numberOfElements(); ++i)
    {
        m_seqFileCons.elementAtPosition(i)->DefDB = 0;
        if( i == pos ) m_seqFileCons.elementAtPosition(i)->DefDB = 1;
    }
}

void ConnSet::setDefaultVals(CON_SET* pCS)
{
    if( pCS->Host.strip().length() == 0 ) pCS->Host = _HOST_DEFAULT;
    else if( pCS->Host == "[local]" )     pCS->Host = _HOST_DEFAULT;

    if( pCS->Port.strip().length() == 0 || pCS->Port == "[default]" )
    {
        if( pCS->Type == _DB2 || pCS->Type == _DB2ODBC ) pCS->Port = _PORT_DB2;
        else if( pCS->Type == _POSTGRES ) pCS->Port = _PORT_PGSQL;
        else if( pCS->Type == _MARIADB ) pCS->Port = _PORT_MARIADB;
        else if( pCS->Type == _SQLSRV ) pCS->Port = _PORT_SQLSRV;
    }
}

int ConnSet::isInList(CON_SET *pCS)
{
    return isInSeq(&m_seqFileCons, pCS);
}



int ConnSet::isInSeq(GSeq <CON_SET* > *pSeqConSet, CON_SET *pCS)
{
    deb("connSet isInSeq start, elmts: "+GString(m_seqFileCons.numberOfElements()));
    CON_SET *cs;
    setDefaultVals(pCS);
    for(int i = 1; i <= (int)pSeqConSet->numberOfElements(); ++i)
    {
        cs = pSeqConSet->elementAtPosition(i);
        if( cs->DB == pCS->DB && cs->Type == pCS->Type &&
            cs->Host == pCS->Host && cs->UID == pCS->UID &&
            cs->PWD == pCS->PWD && cs->Port == pCS->Port && cs->PwdCmd == pCS->PwdCmd )
        {
            deb("connSet isInSeq: found elmt (1).");
            return 1;
        }
    }
    //In Seq, with different UID/PWD
    for(int i = 1; i <= (int)pSeqConSet->numberOfElements(); ++i)
    {
        cs = pSeqConSet->elementAtPosition(i);
        if( cs->DB == pCS->DB && cs->Type == pCS->Type && cs->Host == pCS->Host && cs->Port == pCS->Port && cs->UID == pCS->UID )
        {
            deb("connSet isInSeq: found elmt (2).");
            return 2;
        }
    }
    deb("connSet isInSeq: elmt not found.");
    return 0;
}

void ConnSet::fillSeq(CON_SET* pCS, QTableWidgetItem * pItem)
{
    if( !pItem ) return;
    QLineEdit * pLE = qobject_cast<QLineEdit*>(m_twMain->cellWidget(pItem->row(), CN_PWD));
    pCS->Type = m_twMain->item(pItem->row(), CN_TYPE)->text();
    pCS->DB = m_twMain->item(pItem->row(), CN_DB)->text();
    pCS->UID = m_twMain->item(pItem->row(), CN_UID)->text();
    pCS->PwdCmd = m_twMain->item(pItem->row(), CN_PWDCMD)->text();
    pCS->PWD = pLE->text();
    pCS->Port = m_twMain->item(pItem->row(), CN_PORT)->text();
    pCS->Host = m_twMain->item(pItem->row(), CN_HOST)->text();
    pCS->Color = m_twMain->item(pItem->row(), CN_COLOR)->text();
    pCS->Options = m_twMain->item(pItem->row(), CN_OPTIONS)->text();
    pCS->ReconnTimeout = GString(m_twMain->item(pItem->row(), CN_RECONN)->text()).asInt();

}

void ConnSet::showEditConn(QTableWidgetItem * pItem)
{
    //Get content of column 0 -> position in seq
//    int idx = GString(m_twMain->item(pItem->row(), 0)->text()).asInt();
//    if( idx > m_seqFileCons.numberOfElements() ) return;

    CON_SET oldCS;
    QLineEdit * pLE = qobject_cast<QLineEdit*>(m_twMain->cellWidget(pItem->row(), CN_PWD));
    fillSeq(&oldCS, pItem);

    CON_SET newCS;
    EditConn * foo = new EditConn(&oldCS, &newCS, this);
    foo->CreateDisplay(1);
    foo->exec();
    m_iChanged = 0;
    if( foo->hasChanged() )
    {
        m_twMain->item(pItem->row(), CN_TYPE)->setText(newCS.Type);
        m_twMain->item(pItem->row(), CN_DB)->setText(newCS.DB);
        m_twMain->item(pItem->row(), CN_UID)->setText(newCS.UID);
        pLE->setText(newCS.PWD);
        m_twMain->item(pItem->row(), CN_PWDCMD)->setText(newCS.PwdCmd);
        m_twMain->item(pItem->row(), CN_PORT)->setText(newCS.Port);
        m_twMain->item(pItem->row(), CN_HOST)->setText(newCS.Host);
        m_twMain->item(pItem->row(), CN_COLOR)->setText(newCS.Color);
        m_twMain->item(pItem->row(), CN_OPTIONS)->setText(newCS.Options);
        m_twMain->item(pItem->row(), CN_RECONN)->setText(GString(newCS.ReconnTimeout));
        setRowBackGroundColor(pItem->row(), newCS.Color);
        m_iChanged = 1;
    }
    delete foo;
    m_twMain->clearSelection();
    //this->save();
}

void ConnSet::NewEntryClicked()
{
    CON_SET newCS;
    CON_SET oldCS;
    QItemSelectionModel* selectionModel = m_twMain->selectionModel();
    QModelIndexList selected = Helper::getSelectedRows(selectionModel);
    if( selected.count() > 0 )
    {
        QModelIndex index = selected.at(0);
        QTableWidgetItem* pItem = m_twMain->item(index.row(), 0);
        fillSeq(&oldCS, pItem);
    }
    EditConn * foo = new EditConn(&oldCS, &newCS, this);
    foo->CreateDisplay();
    foo->exec();
    m_iChanged = 0;
    if( foo->hasChanged() )
    {
        if( isInList(&newCS) )
        {
            tm("This connection already exists.");
            delete foo;
            return;
        }
        newCS.DefDB = 0;
        createRow(&newCS);
        Helper::setVHeader(m_twMain);
        //m_twMain->selectRow(rows);
        m_iChanged = 1;
    }
}


void ConnSet::listDoubleClicked(QTableWidgetItem * pItem)
{
    showEditConn(pItem);
}

void ConnSet::editEntry()
{
    QItemSelectionModel* selectionModel = m_twMain->selectionModel();
    QModelIndexList selected = Helper::getSelectedRows(selectionModel);
    if( selected.count() == 0 )
    {
        tm("Nothing selected.\nTo select a row, click on the vertical header.");
        return;
    }
    QModelIndex index = selected.at(0);
    showEditConn(m_twMain->item(index.row(), 0));
}

void ConnSet::delEntry()
{
	QItemSelectionModel* selectionModel = m_twMain->selectionModel();
    QModelIndexList selected = Helper::getSelectedRows(selectionModel);
	if( selected.count() == 0 )
	{
        tm("Nothing selected.\nTo select a row, click on the vertical header.");
		return;
    }
    int count = selected.count();
    if( QMessageBox::question(this, "PMF", "Removing "+GString(count)+" entries, continue?", "Yes", "No", 0, 1) == 1 ) return;

	for(int i = count-1; i >= 0; i--)
	{
	 	QModelIndex index = selected.at(i);
        m_twMain->removeRow(index.row());
	}
	m_iChanged = 1;
}

CON_SET* ConnSet::getConSet(GString dbType, GString dbName)
{
    for(int i = 1; i <= (int)m_seqFileCons.numberOfElements(); ++i)
    {
        if( m_seqFileCons.elementAtPosition(i)->Type == dbType && m_seqFileCons.elementAtPosition(i)->DB == dbName ) return m_seqFileCons.elementAtPosition(i);
    }
    //Nothing found in file, let's return Node-info
    for(int i = 1; i <= (int)m_seqCatalogCons.numberOfElements(); ++i)
    {
        if( m_seqCatalogCons.elementAtPosition(i)->Type == dbType && m_seqCatalogCons.elementAtPosition(i)->DB == dbName ) return m_seqCatalogCons.elementAtPosition(i);
    }
    return NULL;
}

GString ConnSet::getConnectionColor(GString type, GString db, GString host, GString port, GString uid)
{
    CON_SET * pCS = findConnSet(type, db, host, port, uid);
    if( pCS != NULL ) return pCS->Color;
    return "";
}

int ConnSet::setConnectionColor(GString type, GString db, GString host, GString port, GString uid, GString color)
{
    CON_SET * pCS = findConnSet(type, db, host, port, uid);
    if( pCS != NULL )
    {
        pCS->Color = color;
        return 0;
    }
    return 1;
}

GString ConnSet::getPwdCmd(GString type, GString db, GString host, GString port, GString uid)
{    
    CON_SET * pCS = findConnSet(type, db, host, port, uid);
    if( pCS != NULL ) return pCS->PwdCmd;
    return "";
}

int ConnSet::getReconnectTimeout(GString type, GString db, GString host, GString port, GString uid)
{
    CON_SET * pCS = findConnSet(type, db, host, port, uid);
    if( pCS != NULL ) return pCS->ReconnTimeout;
    return 0;
}

CON_SET* ConnSet::findConnSet(GString type, GString db, GString host, GString port, GString uid)
{
    CON_SET * pCS;
    for(int i = 1; i<= m_seqFileCons.numberOfElements(); ++i )
    {
        pCS = m_seqFileCons.elementAtPosition(i);
        if( pCS->Type == type && pCS->DB == db && pCS->Host == host && pCS->Port == port && pCS->UID == uid ) return pCS;
    }
    for(int i = 1; i<= m_seqFileCons.numberOfElements(); ++i )
    {
        pCS = m_seqFileCons.elementAtPosition(i);
        if( pCS->Type == type && pCS->DB == db && pCS->Host == host && pCS->UID == uid ) return pCS;
    }
    for(int i = 1; i<= m_seqFileCons.numberOfElements(); ++i )
    {
        pCS = m_seqFileCons.elementAtPosition(i);
        if( pCS->Type == type && pCS->DB == db && pCS->UID == uid ) return pCS;
    }

    return NULL;
}


CON_SET* ConnSet::getDefaultCon()
{
    for(int i = 1; i <= (int)m_seqFileCons.numberOfElements(); ++i)
    {
        if( m_seqFileCons.elementAtPosition(i)->DefDB ) return m_seqFileCons.elementAtPosition(i);
    }
    return NULL;
}

void ConnSet::testEntry()
{
    QItemSelectionModel* selectionModel = m_twMain->selectionModel();
    QModelIndexList selected = Helper::getSelectedRows(selectionModel);

    if( selected.count() == 0 )
    {
        tm("Nothing selected.\nTo select a row, click on the vertical header.");
        return;
    }
    QModelIndex index = selected.at(0);

    GString type = m_twMain->item(index.row(), CN_TYPE)->text();
    GString db   = m_twMain->item(index.row(), CN_DB)->text();
    GString uid  = m_twMain->item(index.row(), CN_UID)->text();
    GString pwd  = m_twMain->item(index.row(), CN_PWD)->text();
    GString host = m_twMain->item(index.row(), CN_HOST)->text();
    GString port = m_twMain->item(index.row(), CN_PORT)->text();
    DSQLPlugin * plg = new DSQLPlugin(type);
    int erc = plg->connect(db, uid, pwd, host, port);
    if( erc )
    {
        tm("Error: "+GString(plg->sqlCode())+" - "+plg->sqlError());
    }
    else tm("Connection successful");
    plg->disconnect();
}

GString ConnSet::createCfgFile()
{
	QString home = QDir::homePath ();
	if( !home.length() ) return "";
	
	QDir aDir(home);
    #if defined(MAKE_VC) || defined (__MINGW32__)
	aDir.mkdir(_CFG_DIR);

	#else
	aDir.mkdir("."+_CFG_DIR);
	#endif
	QFile cfgFile( cfgFileName() );	
	if (!cfgFile.open(QIODevice::WriteOnly | QIODevice::Text)) return "";
	return home;
}

GString ConnSet::getValue(GString key)
{
	QString home = QDir::homePath ();
	if( !home.length() ) return "";
	GFile cfgFile( cfgFileName() );
	if( !cfgFile.initOK() ) return 2;
	return cfgFile.getKey(key);
}

GString ConnSet::cfgFileNameXml()
{
	QString home = QDir::homePath ();
	if( !home.length() ) return "";
    #if defined(MAKE_VC) || defined (__MINGW32__)
    return GString(home)+"\\"+_CFG_DIR+"\\"+_CONNSET_XML;
	#else
    return GString(home)+"/."+_CFG_DIR + "/"+_CONNSET_XML;
	#endif
}

GString ConnSet::cfgFileName()
{
    QString home = QDir::homePath ();
    if( !home.length() ) return "";
    #if defined(MAKE_VC) || defined (__MINGW32__)
    return GString(home)+"\\"+_CFG_DIR+"\\"+_CONNSET_FILE;
    #else
    return GString(home)+"/."+_CFG_DIR + "/"+_CONNSET_FILE;
    #endif
}

int ConnSet::loadFromXmlFile()
{
    deb("connset, loadFromXmlFile");
    QString home = QDir::homePath ();
    if( !home.length() ) return 1;

    GXml fXml;
    int erc = fXml.readFromFile(cfgFileNameXml());
    if( erc ) return erc;

    GXml outXml = fXml.getBlocksFromXPath("/Connections/Connection");
    int count = outXml.countBlocks("Connection");

    clearSeq(&m_seqFileCons);
    CON_SET * pConSet;
    deb("connset, loadFromXmlFile, loading "+GString(count)+" items...");
    for(int i=1; i <= count; ++i )
    {
        GXml inner = outXml.getBlockAtPosition("Connection", i);
        pConSet = new CON_SET;
        pConSet->init();
        pConSet->DefDB = inner.getAttribute("isDefault").asInt();
        pConSet->Type = inner.getAttribute("type");
        pConSet->Type = inner.getAttribute("type");
        pConSet->DB = inner.getAttribute("db");
        pConSet->Host = inner.getAttribute("host");
        pConSet->Port = inner.getAttribute("port");
        pConSet->Color = inner.getAttribute("color");
        pConSet->UID = inner.getAttribute("uid");
        pConSet->PWD = GStuff::decryptFromBase64(inner.getAttribute("pwd"));
        pConSet->PwdCmd = inner.getAttribute("pwdCmd");
        pConSet->Options = inner.getAttribute("options");
        GString recTo = inner.getAttribute("reconnTimeout");
        if( !recTo.length() ) pConSet->ReconnTimeout = 0;
        else pConSet->ReconnTimeout = recTo.asInt();
        GString s = inner.getAttribute("reconnTimeout");
        pConSet->CSVer = inner.getAttribute("version").asInt();
        //deb("connset, loadFromFile, adding ToSeq...");
        addToSeq(&m_seqFileCons, pConSet);
    }
    deb("connset, loadFromXmlFile done. elmts: "+GString(m_seqFileCons.numberOfElements()));

    return 0;
}

int ConnSet::loadFromFile()
{    
    deb("connset, loadFromFile");
    QString home = QDir::homePath ();
    if( !home.length() ) return 1;
    GFile cfgFile( cfgFileName() );
    if( !cfgFile.initOK() ) return 2;

    clearSeq(&m_seqFileCons);
    CON_SET * pConSet;
    GString data;
    for( int i = 1; i <=cfgFile.lines(); ++i )
    {
        pConSet = new CON_SET;
        pConSet->init();
        data = cfgFile.getLine(i);


        GSeq <GString> csElmtSeq;
        if( data.occurrencesOf(_PMF_PASTE_SEP) == 0 ) csElmtSeq = data.split("@");
        else csElmtSeq = data.split(_PMF_PASTE_SEP);
        if( csElmtSeq.numberOfElements() < 7 ) continue;

        //Default: 0 or 1
        pConSet->DefDB = csElmtSeq.elementAtPosition(1).asInt();
        //Type: DB2, SqlServer,...
        pConSet->Type = csElmtSeq.elementAtPosition(2).strip();
        //DBName
        pConSet->DB = csElmtSeq.elementAtPosition(3).strip();
        //UserID
        pConSet->UID = csElmtSeq.elementAtPosition(4).strip();
        //PWD
        pConSet->PWD = csElmtSeq.elementAtPosition(5).strip();
        //Host
        pConSet->Host = csElmtSeq.elementAtPosition(6).strip();
        //Port
        pConSet->Port  = csElmtSeq.elementAtPosition(7).strip();
		
		pConSet->CSVer = 0;
		pConSet->PwdCmd  = "";

        if( csElmtSeq.numberOfElements() == 8 )
        {
            pConSet->PwdCmd  = "";
            pConSet->CSVer  = csElmtSeq.elementAtPosition(8).asInt();
        }
        else if( csElmtSeq.numberOfElements() == 9 )
        {
            pConSet->PwdCmd  = csElmtSeq.elementAtPosition(8).strip();
            pConSet->CSVer  = csElmtSeq.elementAtPosition(9).asInt();
        }
        else if( csElmtSeq.numberOfElements() == 10 )
        {
            pConSet->PwdCmd  = csElmtSeq.elementAtPosition(8).strip();
            pConSet->Color  = csElmtSeq.elementAtPosition(9);
            pConSet->CSVer  = csElmtSeq.elementAtPosition(10).asInt();
        }
        else if( csElmtSeq.numberOfElements() == 11 )
        {
            pConSet->PwdCmd  = csElmtSeq.elementAtPosition(8).strip();
            pConSet->Color  = csElmtSeq.elementAtPosition(9);
            pConSet->Options  = csElmtSeq.elementAtPosition(10);
            pConSet->CSVer  = csElmtSeq.elementAtPosition(11).asInt();
        }
        else if( csElmtSeq.numberOfElements() == 12 )
        {
            pConSet->PwdCmd  = csElmtSeq.elementAtPosition(8).strip();
            pConSet->Color  = csElmtSeq.elementAtPosition(9);
            pConSet->Options  = csElmtSeq.elementAtPosition(10);
            pConSet->ReconnTimeout  = csElmtSeq.elementAtPosition(11).asInt();
            pConSet->CSVer  = csElmtSeq.elementAtPosition(12).asInt();
        }

        if( pConSet->CSVer >= 2 ) pConSet->PWD = GStuff::decryptFromBase64(pConSet->PWD);
        //Activating this will show ALL db settings ever entered, even if the node is not there anymore.
        //Deactivating this will clear the list from defunct connections.
        deb("connset, loadFromFile, adding ToSeq...");
        addToSeq(&m_seqFileCons, pConSet);
    }
    deb("connset, loadFromFile done. elmts: "+GString(m_seqFileCons.numberOfElements()));
    return 0;
}

void ConnSet::deb(GString msg)
{
    if( !m_pGDeb ) return;
    m_pGDeb->debugMsg("connSet", 1, msg);
}

int ConnSet::findDatabases(GString dbType)
{

    DSQLPlugin * plg = new DSQLPlugin(dbType);

    deb("loadConnnections checking plugin for "+dbType);
    if( !plg || !plg->isOK() ) return 1;
    deb("loadConnnections plugin for "+dbType+": ok");
    plg->setGDebug(m_pGDeb);

    GSeq <CON_SET*> dbSeq;
    deb("getting databases...");
    int rc = plg->getDataBases(&dbSeq);
    deb("getting databases, rc: "+GString(rc));
    if( rc )
    {
        m_strErrorMessages += "No Databases found for Type "+dbType+", sqlCode: "+GString(rc)+"\n";
        return rc;
    }

    deb("loadConnnections getDatabases: "+GString(dbSeq.numberOfElements()));
    //In the plugin, CON_SETs are created with 'new.
    //These elements are zombified when the plugin closes. So we need to copy them.
    //Simply saving the pointers to the created CON_SETs does not work under Windows!
    CON_SET * pCS;

    //Copy the CON_SETs
    for( int i = 1; i <= (int) dbSeq.numberOfElements(); ++i)
    {
        if( !dbSeq.elementAtPosition(i)->DB.length() ) continue;
        pCS = new CON_SET;
        pCS->init();
        pCS->DB    = dbSeq.elementAtPosition(i)->DB;
        pCS->DefDB = 0; //dbSeq.elementAtPosition(i)->DefDB;
        pCS->Host  = dbSeq.elementAtPosition(i)->Host;
        pCS->Port  = dbSeq.elementAtPosition(i)->Port;
        pCS->PWD   = dbSeq.elementAtPosition(i)->PWD;
        pCS->Type  = dbSeq.elementAtPosition(i)->Type;
        pCS->UID   = dbSeq.elementAtPosition(i)->UID;       
        pCS->PwdCmd   = dbSeq.elementAtPosition(i)->PwdCmd;
        pCS->ReconnTimeout = dbSeq.elementAtPosition(i)->ReconnTimeout;
        pCS->CSVer = ConnSetVer;
        //See if connection information (UID, PWD,...) was stored
        //setConnectionSettingsFromFile(pCS);
        deb("adding db "+pCS->DB);
        addToSeq(&m_seqCatalogCons, pCS);
    }
    deb("dbList elmts: "+GString(m_seqFileCons.numberOfElements()));
    //We have to clean this manually
    while( !dbSeq.isEmpty() )
    {
        pCS = dbSeq.firstElement();
        delete pCS;
        pCS  = NULL;
        dbSeq.removeFirst();
    }

    delete plg;
    return 0;
}

int ConnSet::createRow(CON_SET *pConSet)
{
    QTableWidgetItem * pItem;

    QCheckBox * pChkBx   = new QCheckBox(this);
    QLineEdit * pPWDLE   = new QLineEdit(this);
    m_twMain->setSortingEnabled(false);
    m_twMain->setUpdatesEnabled(false);


    int rows = m_twMain->rowCount();
    m_twMain->insertRow(rows);

    //Type: SqlSrv, DB2,...
    pItem = new QTableWidgetItem((char*)pConSet->Type);
    pItem->setFlags(pItem->flags() ^ Qt::ItemIsEditable);
    m_twMain->setItem(rows, CN_TYPE, pItem);

    //Database
    pItem = new QTableWidgetItem((char*)pConSet->DB);
    pItem->setFlags(pItem->flags() ^ Qt::ItemIsEditable);
    m_twMain->setItem(rows, CN_DB, pItem);

    //UserID
    pItem = new QTableWidgetItem((char*)pConSet->UID);
    pItem->setFlags(pItem->flags() ^ Qt::ItemIsEditable);
    m_twMain->setItem(rows, CN_UID, pItem);

    //PWD
    pPWDLE->setText((char*)pConSet->PWD);
    pPWDLE->setEnabled(false);
    pPWDLE->setEchoMode(QLineEdit::Password);
    m_twMain->setCellWidget(rows, CN_PWD, pPWDLE);

    GString host = pConSet->Host;
    if( !host.length() ) host = _HOST_DEFAULT;


    GString port = pConSet->Port;
    if( !port.length() ) port = _PORT_DEFAULT;

    pItem = new QTableWidgetItem((char*)host);
    m_twMain->setItem(rows, CN_HOST, pItem);
    pItem->setFlags(pItem->flags() ^ Qt::ItemIsEditable);

    pItem = new QTableWidgetItem((char*)port);
    m_twMain->setItem(rows, CN_PORT, pItem);
    pItem->setFlags(pItem->flags() ^ Qt::ItemIsEditable);

    pItem = new QTableWidgetItem((char*)pConSet->PwdCmd);
    m_twMain->setItem(rows, CN_PWDCMD, pItem);
    pItem->setFlags(pItem->flags() ^ Qt::ItemIsEditable);

    pItem = new QTableWidgetItem((char*)pConSet->Color);
    m_twMain->setItem(rows, CN_COLOR, pItem);

    pItem = new QTableWidgetItem((char*)pConSet->Options);
    m_twMain->setItem(rows, CN_OPTIONS, pItem);

    pItem = new QTableWidgetItem((char*)GString(pConSet->ReconnTimeout));
    m_twMain->setItem(rows, CN_RECONN, pItem);

    //DefaultCheckBox
    m_twMain->setCellWidget(rows, CN_DEF, pChkBx); //Default

    if( pConSet->DefDB ) pChkBx->setCheckState(Qt::Checked);
    connect(pChkBx, SIGNAL(stateChanged(int)), SLOT(defaultSelected(int)));

/*
    GSeq <GString> list;
    DSQLPlugin::DBTypeNames(&list);
    for(unsigned long i = 1; i <= list.numberOfElements(); ++i)
    {
        pTypeCB->addItem(list.elementAtPosition(i));
        if( pConSet->Type == list.elementAtPosition(i)) pTypeCB->setCurrentIndex(i-1);
    }

    //Currently,, I have no idea how to fetch database instances for non-DB2 databases.
    if( m_pIDSQL->getDBType() == IDSQL::DB2)
    {
        GSeq <CON_SET*> aSeq;
        m_pIDSQL->getDataBases(&aSeq);
        for(unsigned long i = 1; i <= aSeq.numberOfElements(); ++i)
        {
            pDBCB->addItem(aSeq.elementAtPosition(i)->DB);
            if( pConSet->DB == aSeq.elementAtPosition(i)->DB) pDBCB->setCurrentIndex(i-1);
        }
    }
    else if( pConSet->DB.length()) pDBCB->addItem(pConSet->DB);
    pDBCB->setEditable(true);
    pTypeCB->adjustSize();
    connect(pTypeCB, SIGNAL(activated(int)), SLOT(getAllDatabases(int)));

*/

    m_twMain->setRowHeight(rows, QFontMetrics( m_twMain->font()).height()+5);
    //m_twMain->selectRow(rows);
    m_twMain->setSortingEnabled(true);
    m_twMain->setUpdatesEnabled(true);
    setRowBackGroundColor(rows, pConSet->Color);

    return 0;
}

void ConnSet::setRowBackGroundColor(int row, GString rgbColor)
{    

    QColor color = QColor((char*)rgbColor);
    if( !color.isValid() )
    {
        return;
        //color = QColor("#ffffff");
    }


    for(int i = 0; i < m_twMain->columnCount(); ++i)
    {
        if( i == CN_DEF || i == CN_PWD ) continue;
#if QT_VERSION >= 0x060000
        m_twMain->item(row, i)->setBackground(color);
#else
        m_twMain->item(row, i)->setBackgroundColor(color);
#endif
    }
}


void ConnSet::defaultSelected(int)
{
	//This object was clicked:
	QCheckBox * pChkBx = qobject_cast<QCheckBox *>(sender());	
	//if already unchecked, do nothing.
	//the previously checked sender is already unchecked when this function is called.
	if( pChkBx->checkState() == Qt::Unchecked ) return;

	//uncheck all
	this->blockSignals(true);
	QCheckBox * pUnChkBx;
	for(int i = 0; i < m_twMain->rowCount(); ++i )
	{
        pUnChkBx = qobject_cast<QCheckBox*>(m_twMain->cellWidget(i, CN_DEF));
		if( pChkBx != pUnChkBx ) pUnChkBx->setCheckState(Qt::Unchecked);
	}
	//Now set sender checked
	pChkBx->setCheckState(Qt::Checked);
	this->blockSignals(false);
}

int ConnSet::setConnectionSettingsFromFile(CON_SET *pConSet)
{

    QString home = QDir::homePath ();
    if( !home.length() ) return 1;


    GFile cfgFile( cfgFileName() );
    if( !cfgFile.initOK() ) return 2;

    GString data;
    for( int i = 1; i <=cfgFile.lines(); ++i )
    {
        data = cfgFile.getLine(i);
        GSeq <GString> csElmtSeq = data.split(_PMF_PASTE_SEP);

        if( csElmtSeq.numberOfElements() < 7 ) continue;

        //wrong database: DBName or Type different
        if( pConSet->DB != csElmtSeq.elementAtPosition(3).strip() || pConSet->Type != csElmtSeq.elementAtPosition(2).strip() ) continue;

        //Default: 0 or 1
        pConSet->DefDB = csElmtSeq.elementAtPosition(1).strip().asInt();
        //DBName
        pConSet->DB = csElmtSeq.elementAtPosition(3).strip();
        //Type: DB2, SqlServer,...
        pConSet->Type = csElmtSeq.elementAtPosition(2).strip();
        //UserID
        pConSet->UID = csElmtSeq.elementAtPosition(4).strip();
        //PWD
        pConSet->PWD = csElmtSeq.elementAtPosition(5).strip();
        //Host
        pConSet->Host = csElmtSeq.elementAtPosition(6).strip();
        //Port
        pConSet->Port  = csElmtSeq.elementAtPosition(7);
        if( csElmtSeq.numberOfElements() >= 8 )pConSet->PwdCmd = csElmtSeq.elementAtPosition(8);
        if( csElmtSeq.numberOfElements() >= 9 )pConSet->Options = csElmtSeq.elementAtPosition(9);
        if( csElmtSeq.numberOfElements() >= 10 )
        {
            pConSet->CSVer = csElmtSeq.elementAtPosition(10);
        }
        else pConSet->CSVer = 0;
    }
    return 0;
}

void ConnSet::getAllDatabases(int pos)
{
    PMF_UNUSED(pos);

    //Neat: Multiple ComboBoxes can signal this slot. QObject::sender()
    //will give us the sender. Heed the warnings, though.
    //In this case we're fine.
    QComboBox *pCB = (QComboBox*)QObject::sender();
}

void ConnSet::moveToXmlFormat()
{
    deb("moveToXmlFormat start");
    GString fileName;
    QString home = QDir::homePath ();
    if( !home.length() ) return;
    #if defined(MAKE_VC) || defined (__MINGW32__)
    fileName = GString(home)+"\\"+_CFG_DIR+"\\"+_CONNSET_XML;
    #else
    fileName = GString(home)+"/."+_CFG_DIR + "/"+_CONNSET_XML;
    #endif
    GFile f(fileName);
    if(f.initOK())
    {
        deb("moveToXmlFormat, have xml, all good.");
        return;
    }

    deb("moveToXmlFormat load old file...");
    loadFromFile();
    deb("moveToXmlFormat saving in new format...");
    saveAsXml();
    #if defined(MAKE_VC) || defined (__MINGW32__)
    fileName = GString(home)+"\\"+_CFG_DIR+"\\"+_CONNSET_FILE;
    #else
    fileName = GString(home)+"/."+_CFG_DIR + "/"+_CONNSET_FILE;
    #endif
    deb("moveToXmlFormat removing old file");
    remove(fileName);
}

void ConnSet::migrateData()
{
    for(int i = 1; i <= (int)m_seqFileCons.numberOfElements(); ++ i)
    {
        if( m_seqFileCons.elementAtPosition(i)->CSVer < 7)
        {
            this->save();
            break;
        }
    }
}

int ConnSet::guiHasChanges()
{
    return m_iChanged;
}

void ConnSet::sortClicked(int)
{
    Helper::setVHeader(m_twMain);
}

void ConnSet::showConnSet(CON_SET *pCS)
{
    deb("-- ConnSet: ");
    deb("   DB:   "+pCS->DB);
    deb("   Type: "+pCS->Type);
    deb("   UID:  "+pCS->UID);
    deb("   Ver:  "+GString(pCS->CSVer));
    deb("   Host: "+pCS->Host);
    deb("   Port: "+pCS->Port);
    deb("   PwdCmd: "+pCS->PwdCmd);
    deb("   Timeout: "+GString(pCS->ReconnTimeout));
}

void ConnSet::msg(GString message)
{
    if( m_pMainWdgt ) QMessageBox::information(this, "PMF", message);
}

