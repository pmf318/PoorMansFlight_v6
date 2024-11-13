//
//  This file is part of PMF (Poor Man's Flight), see README.PMF for details
//  Author: Gregor Leipelt (gregor@leipelt.de)  (C) G. Leipelt 2000
//

#include "newConn.h"

#include <qcheckbox.h>
//Added by qt3to4:
#include <QLabel>
#include <QGridLayout>
#include <gseq.hpp>
#include <gfile.hpp>
#include <qlayout.h>
#include <QHeaderView>
#include <QScrollArea>
#include <QSettings>

#include <gstuff.hpp>
#include <dbapiplugin.hpp>
#include "simpleShow.h"
#include "createCheck.h"
#include "pmfdefines.h"
#include "clickLabel.h"
#include <idsql.hpp>


NewConn::NewConn( QWidget *parent, GDebug * pDeb  )  : QDialog( parent )
{

    m_pParent = parent;
    m_pGDeb = pDeb;

    m_pConnSet = NULL;
    m_pAddDatabaseHost = NULL;
    //m_pCatalogDB = NULL;
    m_pCatalogInfo = NULL;
    m_pDSQL = NULL;
    m_strLastSelDbType = "";

    m_tabWdgt = new QTabWidget(this);

    connect(m_tabWdgt, SIGNAL(currentChanged(int)), this, SLOT(tabChanged(const int &)));


    this->resize(900, 480);

    QGridLayout * mainGrid = new QGridLayout(this);
    QPushButton * exitBT = new QPushButton("Exit", this);
    m_nextBT = new QPushButton("Next", this);
    m_saveBT = new QPushButton("Save", this);
    mainGrid->addWidget(m_tabWdgt, 0, 0, 1,5);
    mainGrid->addWidget(exitBT,1, 0);
    mainGrid->addWidget(m_nextBT,1, 1);
    mainGrid->addWidget(m_saveBT,1, 4);
    connect(exitBT, SIGNAL(clicked()), SLOT(endIt()));
    connect(m_nextBT, SIGNAL(clicked()), SLOT(toggleTab()));
    connect(m_saveBT, SIGNAL(clicked()), SLOT(saveClicked()));
    //mainGrid->setColumnStretch(1, 2);
    m_saveBT->setDisabled(true);
    addFirstTab();
}


void NewConn::toggleTab()
{
    if( m_tabWdgt->count() < 2 )
    {
        m_nextBT->setText("Next");
        m_nextBT->setEnabled(false);
        m_saveBT->setDisabled(true);
        return;
    }
    if( m_tabWdgt->currentIndex() == 1 )
    {
        m_nextBT->setText("Next");
        m_tabWdgt->setCurrentIndex(0);
        m_saveBT->setDisabled(true);
    }
    else {
        m_nextBT->setText("Previous");
        m_tabWdgt->setCurrentIndex(1);
        m_saveBT->setDisabled(false);
    }
}

void NewConn::endIt()
{
    if(m_pConnSet)
    {
        if( m_pConnSet->guiHasChanges() )
        {
            if( QMessageBox::question(this, "PMF", "Quit without saving?", QMessageBox::Yes, QMessageBox::No) == QMessageBox::No )
            {
                return;
            }

        }
    }
	close();
}

NewConn::~NewConn()
{
    clean();
}

void NewConn::tabChanged(const int &pIndex)
{
    if( pIndex == 1 )
    {
        m_nextBT->setText("Previous");
        m_saveBT->setDisabled(false);
    }
    else if( pIndex == 0 )
    {
        m_nextBT->setText("Next");
        m_saveBT->setDisabled(true);
    }
}

void NewConn::saveClicked()
{
    if( m_strLastSelDbType.length() == 0 && m_pConnSet )
    {
        m_pConnSet->save();
    }
    else if( m_strLastSelDbType == _DB2 && m_pCatalogInfo )m_pCatalogInfo->OKClicked();
    else if( m_strLastSelDbType == _POSTGRES && m_pAddDatabaseHost )m_pAddDatabaseHost->OKClicked();
    else if( m_strLastSelDbType == _MARIADB && m_pAddDatabaseHost )m_pAddDatabaseHost->OKClicked();
}

void NewConn::addFirstTab()
{
    QSettings settings(_CFG_DIR, "pmf6");
    GString pluginPath = QCoreApplication::applicationDirPath() +"/plugins/";
    if( settings.value("PluginPath", -1).toInt() >= 0 )
    {
        pluginPath = settings.value("PluginPath").toString();
    }


    QWidget * pWdgt = new QWidget();
    QGridLayout * grid = new QGridLayout(pWdgt);

    GSeq <PLUGIN_DATA*> list;
    DSQLPlugin::PluginNames(&list);


    QButtonGroup* selectAction = new QButtonGroup(this);
    _newDbRB = new QRadioButton("Edit existing connection", this);

    selectAction->addButton(_newDbRB);
    grid->addWidget(_newDbRB, 0, 0);
    connect(_newDbRB, SIGNAL(clicked()), SLOT(actionSelected()));
    _newDbRB->setChecked(true);

    grid->addWidget(new QLabel(""), 1, 0);

    ClickLabel * clickMe;

    DSQLPlugin::setPluginPath(pluginPath);
    GString txt ;
    for(int i = 1; i <= (int)list.numberOfElements(); ++i )
    {
        if( list.elementAtPosition(i)->Type == _DB2ODBC || list.elementAtPosition(i)->Type == _SQLSRV ) continue;
        _newDbRB = new QRadioButton(txt, this);
        _newDbRB->setEnabled(false);
        DSQLPlugin dsql(list.elementAtPosition(i)->Type);

        int res = dsql.loadError();
        if( res == PluginLoaded )
        {
            txt = list.elementAtPosition(i)->Type+": Create connection";
            _newDbRB->setEnabled(true);
            connect(_newDbRB, SIGNAL(clicked()), SLOT(actionSelected()));
        }
        else
        {
            if( res == PluginMissing ) txt = list.elementAtPosition(i)->Type+": Plugin missing";
            else txt = list.elementAtPosition(i)->Type+": Plugin not loadable";
            clickMe = new ClickLabel("Help...", this);
            grid->addWidget(clickMe, i+1, 1);
            connect(clickMe, SIGNAL(clicked()), SLOT(showHelp()));
        }
        _newDbRB->setText(txt);
        selectAction->addButton(_newDbRB);
        grid->addWidget(_newDbRB, i+1, 0);
    }

    grid->setColumnStretch(grid->columnCount(), 1);
    grid->setRowStretch(grid->rowCount(), 1);

    m_tabWdgt->addTab(pWdgt, "Start");
    setSecondTab("");
}



void NewConn::actionSelected()
{
    QRadioButton *sendRB = qobject_cast<QRadioButton *>(sender());
    GString txt = sendRB->text();
    if( txt.occurrencesOf(':') ) txt = txt.subString(1, txt.indexOf(':')-1);
    else txt = "";
    setSecondTab(txt);
}


void NewConn::showHelp()
{
    QDialog * helpVw = new QDialog(this);
    QTextBrowser* browser = new QTextBrowser();

    QHBoxLayout *helpLayout = new QHBoxLayout;
    helpLayout->addWidget(browser);
    helpVw->setLayout(helpLayout);


    helpVw->setFixedSize(800,500);
    helpVw->setWindowTitle("PMF Help - ESC to close");
    browser->setSource( QUrl("qrc:///pmfHelp_con.html") );
    helpVw->exec();
}

void NewConn::setSecondTab(GString txt)
{
    clean();    
    m_tabWdgt->removeTab(1);
    m_nextBT->setEnabled(false);
    m_pDSQL = new DSQLPlugin(txt);
    //m_pActionWdgt = new QWidget(pWdgt);
    QWidget * pWdgt = new QWidget();

    m_strLastSelDbType = txt;
    if(!txt.length())
    {
        m_pConnSet = new ConnSet(m_pGDeb, pWdgt);
        m_pConnSet->initConnSet(CONNSET_MODE_EMBED);
        m_tabWdgt->addTab(m_pConnSet, "Config");
        m_nextBT->setEnabled(true);
    }
    else if(txt == _DB2)
    {
        m_pCatalogInfo = new CatalogInfo(m_pDSQL, pWdgt);
        m_pCatalogInfo->createDisplay(ADD_CATINFO_MODE_EMBED);
        m_tabWdgt->addTab(m_pCatalogInfo , "Config");
        m_nextBT->setEnabled(true);
    }
    else if(txt == _POSTGRES || txt == _MARIADB )
    {
        m_pAddDatabaseHost = new AddDatabaseHost(m_pDSQL, &m_pConnSetList, pWdgt);
        m_pAddDatabaseHost->CreateDisplay(ADD_PGSQL_MODE_EMBED);
        m_tabWdgt->addTab(m_pAddDatabaseHost, "Config");
        m_nextBT->setEnabled(true);
    }

}

void NewConn::clean()
{
    if( m_pConnSet ) delete m_pConnSet;
    if( m_pAddDatabaseHost ) delete m_pAddDatabaseHost;
    if( m_pCatalogInfo ) delete m_pCatalogInfo;
    m_pConnSet = NULL;
    m_pAddDatabaseHost = NULL;
    m_pCatalogInfo = NULL;
    //if( m_pDSQL ) delete m_pDSQL;
}

GString NewConn::lastSelectedDbType()
{
    return m_strLastSelDbType;
}

void NewConn::addSecondTab()
{

}
void NewConn::deb(GString msg)
{
    printf("NewConn > %s\n", (char*) msg);
#ifdef MAKE_VC
    _flushall();
#endif
}

