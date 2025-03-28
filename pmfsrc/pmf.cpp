#include "pmf.h"

#if QT_VERSION >= 0x050000
#else
#include <QtGui/QLabel>
#include <QtGui/QMenu>
#include <QtGui/QMenuBar>
#include <QtGui/QAction>
#include <QMenuItem>
#endif

#include <QGridLayout>
#include <QDesktopServices>


#include <QColorDialog>
#include <QTabWidget>
#include <QFileDialog>
#include <QFontDialog>
#include <QSettings>
#include <QShortcut>
#include <QTextBrowser>
#include <QStyleFactory>
#include  <QKeyEvent>
#include <QProcess>
#include <QToolButton>
#include <QDesktopServices>


#include <sys/stat.h>

 
#include <gstring.hpp>
#include <gsocket.hpp>
#include <gstuff.hpp>
#include "loginbox.h"
#include "exportBox.h"
#include "importBox.h"
#include "simpleShow.h"
#include "deltab.h"
#include "allTabDDL.h"
#include "finddbl.h"
#include "pmfdefines.h"
#include "tabSpace.h"
#include "tbSize.h"
#include "getSnap.h"
#include "pmfCfg.h"
#include "getclp.h"
#include "querydb.h"
#include "bookmark.h"
#include "editBm.h"
#include "connSet.h"
#include "gfile.hpp"
#include "helper.h"
#include "downloader.h"
#include "catalogInfo.h"
#include "catalogDB.h"
#include "pmfTable.h"
#include "selectEncoding.h"
#include "newConn.h"
#include "selectEncoding.h"


#include "QResource"

#include "reorgAll.h"

#include "db2menu.h"
#include <qvarlengtharray.h>


//#include <string>
//using namespace std;

#ifdef MAKE_VC
#include <QWindow>
//#include <qpa/qplatformnativeinterface.h>
#endif

#ifdef MSVC_STATIC_BUILD
extern "C" int _except_handler4_common() {
    return 0; // whatever, I don't know what this is
}
#endif


Pmf::Pmf(GDebug *pGDeb, int threaded)
{	
	//QStyleHints styleHints = this->styleHints();
	
    aThread = NULL;
    m_pGDeb = pGDeb;
    m_iThreaded = threaded;
    m_iForceClose = 0;
	deb("ctor start");
	
	m_iTabIDCounter = 0;
	m_iShowing = 0;
    m_pDownloader = 0;
    m_iReconnectWaitTime = 0;
    m_gstrPwdCmd = "";
    timerCount = 0;
    m_iColorScheme = PmfColorScheme::None;

	createGUI();
    m_tabWdgt = new QTabWidget;
    m_tabWdgt->setMovable(true);
    m_tabWdgt->setTabsClosable(true);
    connect(m_tabWdgt, SIGNAL(tabCloseRequested(int)), this, SLOT(closeTabClicked(int)));


    m_mnuMainMenu = new QMenu("Menu", this);
    m_mnuAdmMenu = new QMenu("Administration", this);
    m_mnuTableMenu = new QMenu("Table", this);
    m_mnuBookmarkMenu = new QMenu("Bookmarks", this);
    m_mnuSettingsMenu = new QMenu("Settings", this);
    m_mnuMelpMenu = new QMenu("Help", this);

    //m_pDB2Menu = NULL;
    m_pIDSQL = NULL;
    connect( m_tabWdgt, SIGNAL( currentChanged ( int ) ), this, SLOT( curTabChg(int) ) );

	m_iCurrentTab = -1;
	    

    QVBoxLayout *mainVBox = new QVBoxLayout;
    createInfoArea();
    mainVBox->addWidget(reconnectInfoBox);
    mainVBox->addWidget(m_tabWdgt);
    QWidget *widget = new QWidget();
    widget->setLayout(mainVBox);
    setCentralWidget(widget);

    deb("ctor: Disabling WinErrMsgBox on LoadLibrary failure. Set once in the application.");
    #ifdef MAKE_VC
    //LPDWORD lpOldMode;
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX);
    //SetThreadErrorMode(SEM_FAILCRITICALERRORS, lpOldMode);
    #endif


    QToolButton *addTabButton = new QToolButton(this);
    m_tabWdgt->setCornerWidget(addTabButton, Qt::TopRightCorner);
//    addTabButton->setCursor(Qt::ArrowCursor);
//    addTabButton->setAutoRaise(true);
    QIcon icoB;
    icoB.addPixmap(QPixmap(":addTab.png"), QIcon::Normal, QIcon::On);
    addTabButton->setIcon(icoB);
    connect(addTabButton, SIGNAL(clicked()), SLOT(addNewTab()));
    addTabButton->setToolTip(tr("Add new tab"));    
    reconnectTimer = new QTimer( this );
    connect( reconnectTimer, SIGNAL(timeout()), this, SLOT(reconnectTimerEvent()) );
//    this->setMinimumSize(1024, 768);
//    this->showFullScreen();
    //connect(m_tabWdgt, SIGNAL(tabOrderWasChanged()), SLOT(pmfTabOrderWasChanged()));
}

Pmf::~Pmf()
{
    deb("closing ::pmf...");
    if( m_pIDSQL )
    {
        delete m_pIDSQL;
        m_pIDSQL = NULL;
    }
    QDir tmpDir(Helper::tempPath());
    QStringList filters;
    filters << "PMF6_LOB*.*";
    tmpDir.setNameFilters(filters);
    QStringList filesList = tmpDir.entryList(filters);
    for(int i = 0; i < filesList.count(); ++i)remove(Helper::tempPath()+ GString(filesList.at(i)));

    CHECKBOX_ACTION * pCbAction;
    while( m_cbMenuActionSeq.numberOfElements() )
    {
        pCbAction = m_cbMenuActionSeq.firstElement();
        delete pCbAction;
        m_cbMenuActionSeq.removeFirst();
    }
    deb("closing ::pmf done.");
}

void Pmf::createInfoArea()
{
    reconnectInfoBox = new QGroupBox();
    QGridLayout *reconnectInfoLayout = new QGridLayout(this);
    QSpacerItem * spacer = new QSpacerItem(10, 10);

    reconnectInfoBox->setLayout(reconnectInfoLayout);
    reconnectInfoLE = new QLineEdit(this);
    reconnectInfoLE->setReadOnly(true   );
    reconnectInfoLE->setMinimumWidth(500);
    printf("createInfoArea, check dark\n");
    if( Helper::isSystemDarkPalette() ) printf("pmf::createInfoArea: is Dark\n");
    else printf("pmf::createInfoArea: is Light\n");
    if( Helper::isSystemDarkPalette()) reconnectInfoLE->setStyleSheet("background:#55AA7F;");
    else reconnectInfoLE->setStyleSheet("background:#F6FA82;");

    reconnectNowBt = new QPushButton("Reconnect now");
    //reconnectNowBt ->setMaximumWidth(200);
    connect(reconnectNowBt, SIGNAL(clicked()), SLOT(reconnectNowClicked()));
    reconnectInfoLayout->addWidget(reconnectInfoLE, 0, 0);
    reconnectInfoLayout->addWidget(reconnectNowBt, 0, 1);
    reconnectInfoLayout->addItem(spacer, 0, 2);
    reconnectInfoLayout->setColumnStretch(2, 3);

    //reconnectNowBt->setEnabled(false);
    reconnectInfoBox->setLayout(reconnectInfoLayout);
    reconnectInfoBox->hide();
/*
    downloadInfoBox = new QGroupBox();
    QGridLayout *downloadInfoLayout = new QGridLayout(this);
    QSpacerItem * spacer = new QSpacerItem(10, 10);

    //downloadInfoLayout->setColumnStretch(0, 1);
    //downloadInfoLayout->setColumnStretch(1, 1);
    downloadInfoBox->setLayout(downloadInfoLayout);
    downloadInfoLE = new QLineEdit(this);
    downloadInfoLE->setReadOnly(true);
    downloadInfoLE->setMinimumWidth(500);
    downloadInfoLE->setStyleSheet("background:#F6FA82;");

    downloadCancelButton = new QPushButton("Cancel");
    //downloadCancelButton ->setMaximumWidth(200);
    connect(downloadCancelButton, SIGNAL(clicked()), SLOT(downloadCancelled()));
    downloadInfoLayout->addWidget(downloadInfoLE, 0, 0);
    downloadInfoLayout->addWidget(downloadCancelButton, 0, 1);
    downloadInfoLayout->addItem(spacer, 0, 2);
    downloadInfoLayout->setColumnStretch(2, 3);

    downloadCancelButton->setEnabled(false);
    downloadInfoBox->setLayout(downloadInfoLayout);
//    downloadInfoBox->hide();
*/
}

void Pmf::reconnectNowClicked()
{
    reconnectInfoLE->setText("Reconnecting....");
    reconnectTimer->stop();
    timerCount = 60*m_iReconnectWaitTime;
    reconnectTimerEvent();
    reconnectTimer->start();
    reconnectInfoLE->setText("Reconnecting....Done.");
}




void Pmf::reconnectTimerEvent()
{
    timerCount++;
    deb("timerCount: "+GString(timerCount)+", m_iReconnectWaitTime: "+GString(m_iReconnectWaitTime));
    printf("timer: %i, timeout: %i\n", timerCount, m_iReconnectWaitTime);
    int countDown = 60*m_iReconnectWaitTime - timerCount;
    GString info = "Reconnecting in "+GString(countDown)+" seconds...";

    for( int i = 0; i < m_tabWdgt->count(); ++i )
    {
        QWidget* pWdgt = m_tabWdgt->widget(i);
        TabEdit * pTE = (TabEdit*) pWdgt;
        pTE->setReconnInfo(info);
    }
    reconnectInfoLE->setText(info);
    if( timerCount >= 60*m_iReconnectWaitTime)
    {
        deb("Init reconnect...");
        timerCount = 0;
        printf("Reconnecting...");
        CON_SET conSet;
        GString pwd;
        GString err = getPwdFromCmd(m_gstrPwdCmd, &pwd);
        deb("err from getPwd: "+err);
        if( err.length() )
        {
            reconnectTimer->stop();
            return;
        }
        m_pIDSQL->currentConnectionValues(&conSet);
        conSet.PWD = pwd;
        for( int i = 0; i < m_tabWdgt->count(); ++i )
        {
            QWidget* pWdgt = m_tabWdgt->widget(i);
            TabEdit * pTE = (TabEdit*) pWdgt;
            deb("reconnecting tab #"+GString(i));
            err = pTE->reconnect(&conSet);
            if( err.length() )
            {
                reconnectTimer->stop();
                msg(err+"\n\nNote: You have set '"+m_gstrPwdCmd+"' for this connection.\nPlease check this script for errors.");
                return;
            }
            //pTE->okClick();
        }
    }
}

GString Pmf::getPwdFromCmd(GString cmd, GString *pwd)
{
    if( cmd.strip().length() == 0 )return "";
    GString res, err;
    Helper::runCommandInProcess(cmd, res, err);

    *pwd = res.stripTrailing('\r').stripTrailing('\n').stripTrailing('\r');
    if( err.length() )
    {
        msg("Password cmd "+cmd+" gave error: \n"+err+"\nCannot reconnect.");
        return "";
    }
    return err;
}

void Pmf::versionCheckTimerEvent()
{
    if( !aThread ) return;

    if( !aThread->isAlive())
    {
        versionCheckTimer->stop();
    }
    if( m_gstrCurrentVersion.length() )
    {
        versionCheckTimer->stop();
        QSettings settings(_CFG_DIR, "pmf6");
        GString ver = m_gstrCurrentVersion.removeAll('.');
        settings.setValue("LastCheckedVer", QString((char*)ver));
        return;
    }


    /***************** DEAD CODE AHEAD *******************************/
    if( m_gstrCurrentVersion.length() )
    {
        versionCheckTimer->stop();
        if( m_gstrCurrentVersion == "?" )
        {
            QSettings dateSettings(_CFG_DIR, "pmf6");
            int reply = QMessageBox::question(this, "Versioncheck", "Do you want to allow PMF to check for updates?\n"
                                              "(to change this later, go to menu->Settings)",  QMessageBox::Yes|QMessageBox::No);
            if( reply == QMessageBox::No)
            {
                dateSettings.setValue("checksEnabled", "N");
                m_actVersionCheck->setChecked(false);
                return;
            }
            else dateSettings.setValue("checksEnabled", "Y");
            return;
        }
        QMessageBox msgBox(this);
        msgBox.setWindowTitle("New version");
//#ifdef MAKE_VC
//        msgBox.setInformativeText("There is a new version ("+m_gstrCurrentVersion+") available. \nDo you want to install it? ");
//        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
//        msgBox.setDefaultButton(QMessageBox::Yes);
//        int ret = msgBox.exec();
//        if( ret == QMessageBox::Yes)
//        {
//            getNewVersion();
//        }
//#else
        msgBox.setTextFormat(Qt::RichText);   //this is what makes the links clickable
        msgBox.setText("Hint: A new version ("+m_gstrCurrentVersion+") is available at <a href='http://leipelt.org/downld.html'>www.leipelt.org</a>");
        msgBox.exec();
//#endif

    }
}

void Pmf::MyThread::run()
{
    GStuff::dormez(5000);
    myPmf->m_gstrCurrentVersion = myPmf->checkForUpdate(1);
}

void Pmf::setConnectionColor()
{
    QColor res = QColorDialog::getColor();
    if( !res.isValid() ) return;
    QString col = "background-color: ";
    m_gstrConnectionColor = res.name();
    col += (char*) m_gstrConnectionColor;
    menuBar()->setStyleSheet(col);

    CON_SET curSet;
    m_pIDSQL->currentConnectionValues(&curSet);
    ConnSet cs;
    cs.initConnSet();
    cs.setConnectionColor(curSet.Type, curSet.DB, curSet.Host, curSet.Port, curSet.UID, m_gstrConnectionColor);
    cs.save();

    for( int i = 0; i < m_tabWdgt->count(); ++i )
    {
        QWidget * pWdgt = m_tabWdgt->widget(i);
        TabEdit* pTE = (TabEdit*) pWdgt;
        pTE->fillDBNameLE(m_gstrDBName, m_gstrConnectionColor);
    }

}

QMenu * Pmf::createStyleMenu()
{
	//qputenv("QT_QPA_PLATFORM", "windows:darkmode=0");
    printf("createStyleMenu start\n");
    QStringList styles;
    styles = QStyleFactory::keys();
    m_stylesMenu = new QMenu("Style", this);
    QAction *pLightAction, *pDarkAction;
    QSettings settings(_CFG_DIR, "pmf6");
    GString prevStyle = Helper::getSensibleStyle();
	printf("Pmf::createStyleMenu(): Got %s as sensible style\n", (char*) prevStyle);
    //settings.setValue("style", QString((char*)prevStyle));
    GString styleName;
    for (int i = 0; i < styles.size(); ++i)
    {
        styleName = (styles.at(i).toLocal8Bit()).data();
#ifdef MAKE_VC
        if( GString(styleName).lowerCase() == "windowsvista" )
        {
//            continue;
        }
#endif
        pLightAction = new QAction(styleName, this);
        pLightAction->setCheckable(true);
        pLightAction->setChecked(false);
		printf("Checking if prevStyle %s matches available style %s...\n", (char*) prevStyle, (char*)styleName);
        if( GString(prevStyle) == styleName)
        {
            pLightAction->setChecked(true);
            QApplication::setStyle(QStyleFactory::create(prevStyle));
			printf("Checking if prevStyle %s matches available style %s: have match.\n", (char*) prevStyle, (char*)styleName);
            //qApp->setStyleSheet("QTabBar::close-button { image: url(:/pmf6.1/pmfsrc/icons/addTab.png) subcontrol-position: left; }");
            qApp->setStyleSheet("QTabBar::close-button { image: url(:closeTab.png); subcontrol-position: right;} QTabBar::close-button:hover {image: url(:closeTab_hover.png)}");
        }
        m_stylesMenu->addAction(pLightAction);
        connect(pLightAction, SIGNAL(triggered()), this, SLOT(setStyle()));
    }
    printf("createStyleMenu pos1\n");
#if QT_VERSION >= 0x060500
    //Also add "FUSION - DARK" for Windows if Windows is set to DarkMode
    m_stylesMenu->addSeparator();
    for (int i = 0; i < styles.size(); ++i)
    {
        styleName = (styles.at(i).toLocal8Bit()).data();
        //if( GString(styleName).lowerCase() != "fusion"  && GString(styleName).lowerCase() != "windows" ) continue;

        pDarkAction = new QAction(styleName + _DARK_THEME, this);
        pDarkAction->setCheckable(true);
        pDarkAction->setChecked(false);
        if( GString(prevStyle) == styleName + _DARK_THEME )
        {
            pDarkAction->setChecked(true);
            //QApplication::setStyle(QStyleFactory::create(prevStyle));
        }
        m_stylesMenu->addAction(pDarkAction);
        connect(pDarkAction, SIGNAL(triggered()), this, SLOT(setStyle()));
    }
	
//    pDarkAction = new QAction("Fusion" + _DARK_THEME, this);
//    pDarkAction->setCheckable(true);
//    pDarkAction->setChecked(false);
//    printf("DarkMode: Checking if prevStyle %s matches available style %s...\n", (char*) prevStyle, (char*)styleName);
//    if( GString(prevStyle) == "Fusion" + _DARK_THEME )
//    {
//		printf("DarMode: Checking if prevStyle %s matches available style %s: Have match\n", (char*) prevStyle, (char*)styleName);
//        pDarkAction->setChecked(true);
//		//Dark: Setting This is necessary on Win2019 Server.
//        //QApplication::setStyle(QStyleFactory::create("Fusion"));
//    }
//    m_stylesMenu->addAction(pDarkAction);
//    connect(pDarkAction, SIGNAL(triggered()), this, SLOT(setStyle()));
#endif
    return m_stylesMenu;
}


//QMenu * Pmf::createStyleMenu()
//{
//    printf("createStyleMenu start\n");
//    QStringList styles;
//    styles = QStyleFactory::keys();
//    m_stylesMenu = new QMenu("Style", this);
//    QAction *pLightAction, *pDarkAction;
//    QSettings settings(_CFG_DIR, "pmf6");
//    QString prevStyle;
//#ifndef MAKE_VC
//    prevStyle = settings.value("style", "Fusion").toString();
//#else
//    prevStyle = settings.value("style", "Fusion").toString();
//#endif
//    GString styleName;
//    //Reset: Used to be DarkMode, but OS is now set to lightMode.
//    if( GString(prevStyle).occurrencesOf(_DARK_THEME) && !Helper::isSystemDarkPalette())
//    {
//        printf("Prev. was dark, but OS mode changed to light, defaulting to 'Fusion'\n");
//        prevStyle = "Fusion";
//        settings.setValue("style", prevStyle);
//    }
//    else if( !GString(prevStyle).occurrencesOf(_DARK_THEME) && Helper::isSystemDarkPalette())
//    {
//        printf("Prev. was light or none, but OS mode is dark, defaulting to 'Fusion (dark)'\n");
//        GString tmp = "Fusion" + _DARK_THEME;
//        prevStyle = (char*)tmp;
//        settings.setValue("style", prevStyle);
//    }

//    for (int i = 0; i < styles.size(); ++i)
//    {
//        styleName = (styles.at(i).toLocal8Bit()).data();
//#ifdef MAKE_VC
//        if( GString(styleName).lowerCase() == "windowsvista" )
//        {
//            continue;
//        }
//#endif
//        pLightAction = new QAction(styleName, this);
//        pLightAction->setCheckable(true);
//        pLightAction->setChecked(false);
//        if( GString(prevStyle) == styleName)
//		{
//            pLightAction->setChecked(true);
//            QApplication::setStyle(QStyleFactory::create(prevStyle));
//            //qApp->setStyleSheet("QTabBar::close-button { image: url(:/pmf6.1/pmfsrc/icons/addTab.png) subcontrol-position: left; }");
//            qApp->setStyleSheet("QTabBar::close-button { image: url(:closeTab.png); subcontrol-position: right;} QTabBar::close-button:hover {image: url(:closeTab_hover.png)}");
//		}
//        m_stylesMenu->addAction(pLightAction);
//        connect(pLightAction, SIGNAL(triggered()), this, SLOT(setStyle()));
//    }
//    printf("createStyleMenu pos1\n");
//#if QT_VERSION >= 0x060500
//    //Also add "FUSION - DARK" for Windows if Windows is set to DarkMode
//    m_stylesMenu->addSeparator();
//    pDarkAction = new QAction("Fusion" + _DARK_THEME, this);
//    pDarkAction->setCheckable(true);
//    pDarkAction->setChecked(false);
//    printf("From QSettings: GString(prevStyle): %s\n", (char*) GString(prevStyle));
//    /*
//    if( !Helper::isSystemDarkPalette() )
//    {
//        printf("Not SystemDark, disabling darkMode\n");
//        pDarkAction->setEnabled(false);
//    }
//    */
//    if( GString(prevStyle) == "Fusion" + _DARK_THEME )
//    {
//        pDarkAction->setChecked(true);
//        QApplication::setStyle(QStyleFactory::create(prevStyle));
//    }
//    m_stylesMenu->addAction(pDarkAction);
//    connect(pDarkAction, SIGNAL(triggered()), this, SLOT(setStyle()));

//    /*
//    for (int i = 0; i < styles.size(); ++i)
//    {
//        styleName = (styles.at(i).toLocal8Bit()).data();
//        if( GString(styleName).lowerCase() != "fusion" ) continue;

//        pDarkAction = new QAction(styleName + _DARK_THEME, this);
//        pDarkAction->setCheckable(true);
//        pDarkAction->setChecked(false);
//        if( GString(prevStyle) == styleName + _DARK_THEME )
//        {
//            pDarkAction->setChecked(true);
//            QApplication::setStyle(QStyleFactory::create(prevStyle));
//        }
//        m_stylesMenu->addAction(pDarkAction);
//        connect(pDarkAction, SIGNAL(triggered()), this, SLOT(setStyle()));
//    }
//    */
//#endif
//    return m_stylesMenu;
//}

void Pmf::createMenu(GString dbTypeName)
{
	deb("createMenu start");
    //if( !m_pIDSQL ) return;
	int dbt;
    if( dbTypeName == "DB2" ) dbt = 1;
	else dbt = 2;
	

    menuBar()->clear();
    m_mnuMainMenu->clear();
    m_mnuAdmMenu->clear();
    m_mnuTableMenu->clear();
    m_mnuBookmarkMenu->clear();
    m_mnuSettingsMenu->clear();
    m_mnuMelpMenu->clear();

    m_cbMenuActionSeq.deleteAll();

    //Settings
    m_mnuSettingsMenu->addAction("Configuration", this, SLOT(setConfig()));
    m_mnuSettingsMenu->addSeparator();
    m_mnuSettingsMenu->addAction("Set Font", this, SLOT(setPmfFont()));
    m_mnuSettingsMenu->addAction("Reset Font to default", this, SLOT(resetPmfFont()));
    m_mnuSettingsMenu->addAction("Set Menu Color", this, SLOT(setConnectionColor()));
    m_mnuSettingsMenu->addMenu(createStyleMenu());
    m_mnuSettingsMenu->addSeparator();
    m_mnuSettingsMenu->addAction("Connection profiles", this, SLOT(setConnections()));
    if( m_pIDSQL != NULL && m_pIDSQL->getDBType() == POSTGRES )
    {
        //m_mnuSettingsMenu->addMenu(createEncodingMenu());
        m_mnuSettingsMenu->addAction("Encoding", this, SLOT(setEncoding()));
    }

    // MainMenu
    deb("createMenu (1)");
    m_mnuMainMenu->addAction("Connect",this,SLOT(loginClicked()));
    deb("createMenu (2)");
    if( !dbTypeName.length() )
    {        
        if( m_pIDSQL == NULL )
        {
            DBAPIPlugin * pApi = new DBAPIPlugin(_DB2);
            if( pApi->isValid() )m_mnuMainMenu->addAction("Catalog DBs and Nodes", this, SLOT(catalogDBs()));
            delete pApi;
        }
        m_mnuMainMenu->addAction("Exit",this,SLOT(quitPMF()));
        menuBar()->addMenu(m_mnuMainMenu);
        menuBar()->addMenu(m_mnuSettingsMenu);
        return;
    }
    deb("createMenu (3)");
    m_mnuMainMenu->addSeparator();
    m_mnuMainMenu->addAction("Next Tab", this, SLOT(nextTab()), QKeySequence(QKeySequence::NextChild));
    m_mnuMainMenu->addAction("Previous Tab", this, SLOT(prevTab()), QKeySequence(QKeySequence::PreviousChild));

    //QShortcut* scPrev = new QShortcut(QKeySequence::PreviousChild, this);
    //connect(scPrev, SIGNAL(activated()), this, SLOT(prevTab()));
    deb("createMenu (4)");
    m_mnuMainMenu->addAction("New Tab",this,SLOT(createNewTab()), QKeySequence(Qt::CTRL + Qt::Key_T) );
    m_mnuMainMenu->addAction("Close Tab", this, SLOT(closeCurrentTab()), QKeySequence(Qt::CTRL + Qt::Key_W));
    m_mnuMainMenu->addSeparator();
    if( dbt == 1 ) m_mnuMainMenu->addAction("Catalog DBs and Nodes", this, SLOT(catalogDBs()));
    m_mnuMainMenu->addAction("Query DB", this, SLOT(queryDB()));
    m_mnuMainMenu->addAction("Drop tables", this, SLOT(deleteTable()));
    m_mnuMainMenu->addAction("Create DDLs", this, SLOT(createDDLs()));
    m_mnuMainMenu->addAction("Find identical rows", this, SLOT(findIdenticals()));
    deb("createMenu (5)");
    if( dbt == 1 ) m_mnuMainMenu->addAction("Table spaces", this, SLOT(getTabSpace()));
    if( dbt == 1 ) m_mnuMainMenu->addAction("Table sizes", this, SLOT(tableSizes()));
    if( dbt == 1 ) m_mnuMainMenu->addAction("Database info", this, SLOT(showDatabaseInfo()));
    deb("createMenu (6)");
    m_mnuMainMenu->addSeparator();
    m_mnuMainMenu->addAction("Exit",this,SLOT(quitPMF()));
    deb("createMenu (7)");
    //Administration
    const char * nought = ""; //resolution probs on Manjaro
    if( dbt == 1 ) m_mnuAdmMenu->addAction("Snapshot", this, SLOT(snapShot()));
    else m_mnuAdmMenu->addAction("<DB2 only>", this, nought);

    //Table stuff
    deb("createMenu (8)");
    m_mnuTableMenu->addAction("Export", this, SLOT(exportData()));
    if( dbt == 1 || m_pIDSQL->getDBType() == MARIADB ) m_mnuTableMenu->addAction("Import and Load", this, SLOT(importData()));
    else if( m_pIDSQL->getDBType() == POSTGRES ) m_mnuTableMenu->addAction("Import", this, SLOT(importData()));
    m_mnuTableMenu->addSeparator();
    if( dbt == 1 )
    {
        m_mnuTableMenu->addAction("Delete table contents", this, SLOT(deleteTableContents()));
        m_mnuTableMenu->addSeparator();
    }
    deb("createMenu (9)");
    //m_mnuTableMenu->addAction("Create DDL", this, SLOT(createDDL()));
    m_mnuTableMenu->addAction("Table properties", this, SLOT(runstats()));

    //Bookmarks
    m_mnuBookmarkMenu->addAction("Add bookmark", this, SLOT(addBookmark()));
    m_mnuBookmarkMenu->addAction("Edit bookmarks", this, SLOT(editBookmark()));
    m_mnuBookmarkMenu->addSeparator();

    deb("createMenu (10)");
    //m_mnuSettingsMenu->addMenu(createStyleMenu());
    if( dbt == 1 ) m_mnuSettingsMenu->addMenu(createCharForBitMenu());
    m_mnuSettingsMenu->addMenu(createRestoreMenu());

    createCheckBoxActions();



    //Help
    m_mnuMelpMenu->addAction("Index", this, SLOT(showHelp()));
    m_mnuMelpMenu->addAction("Check Updates", this, SLOT(checkForUpdate()));
	m_mnuMelpMenu->addAction("Show Debug", this, SLOT(showDebug()));
    m_mnuMelpMenu->addAction("Info", this, SLOT(showInfo()));


    menuBar()->addMenu(m_mnuMainMenu);
    menuBar()->addMenu(m_mnuTableMenu);
    menuBar()->addMenu(m_mnuAdmMenu);
    menuBar()->addMenu(m_mnuSettingsMenu);
    menuBar()->addMenu(m_mnuBookmarkMenu);
    connect(m_mnuBookmarkMenu, SIGNAL(aboutToShow()), this, SLOT(bookmarkMenuClicked()))  ;



    menuBar()->addMenu(m_mnuMelpMenu);

//    if( m_actShowCloseOnTabs->isChecked() )
//    {
//        m_tabWdgt->setTabsClosable(true);
//    }
//    else m_tabWdgt->setTabsClosable(false);
    getBookmarks();
	deb("createMenu done");
//    menuBar()->setFont(this->font());
//    m_mnuMainMenu->setFont(this->font());
//    m_mnuAdmMenu->setFont(this->font());
//    m_mnuTableMenu->setFont(this->font());
//    m_mnuBookmarkMenu->setFont(this->font());
//    m_mnuSettingsMenu->setFont(this->font());
//    m_mnuMelpMenu->setFont(this->font());

    deb("createMenu done");

}
	

void Pmf::setStyle()
{  
	QList<QAction*> actionList = m_stylesMenu->actions();
	if (actionList.empty()) return;
	//Deselect all 
	for( int i = 0; i < actionList.count(); ++i ) 
    {
	    actionList.at(i)->setChecked(false);
	}

	QAction *action = qobject_cast<QAction *>(sender());
	if( !action ) return;	
	action->setChecked(true);
	//Changing style works flawless under Linux,
	//under Windows PMF gets resized and manual resizing is disabled...strange
	//NOTE: The above Windows problem occurs only when a tabEdit object exists.
    #if defined(MAKE_VC) || defined (__MINGW32__)
	#else    
//    qApp->setStyle(action->text());
//    qApp->setStyle(QStyleFactory::create(action->text()));
    #endif
    QSettings settings(_CFG_DIR, "pmf6");
	printf("pmf::setStyle: setting %s\n", (char*) GString(action->text()));
	settings.setValue("style", action->text());	

    QList<QWidget*> widgets = this->findChildren<QWidget*>();
    foreach (QWidget* w, widgets)
    {
		
        //!DARK w->setPalette(m_qPalette);
    }
    if( GString(action->text()).occurrencesOf(_DARK_THEME))msg("Note: DarkMode will only work when DarkMode is enabled system-wide.\nChanges take effect after restart.");
    else msg("Changes take effect after restart.");
}


/*
QMenu * Pmf::createEncodingMenu()
{
    QStringList encoding = availableEncodings();

    m_encodingMenu = new QMenu("Encoding", this);
    QAction * ac;
    QSettings settings(_CFG_DIR, "pmf6");

    //The 2nd param is the default value if nothing is found
    QString prev = settings.value("encoding", "Auto").toString();
    for (int i = 0; i < encoding.size(); ++i)
    {
        ac = new QAction(encoding.at(i).toLocal8Bit(), this);
        ac->setCheckable(true);
        if( prev == QString(encoding.at(i).toLocal8Bit()) )
        {
            ac->setChecked(true);
            m_pIDSQL->setEncoding(prev);
        }
        else ac->setChecked(false);
        m_encodingMenu->addAction(ac);
        connect(ac, SIGNAL(triggered()), this, SLOT(setEncoding()));
    }
    return m_encodingMenu;
}
*/
void Pmf::setEncoding()
{
    SelectEncoding  *foo = new SelectEncoding(m_pIDSQL, this);
    foo->exec();
    GString encoding = foo->encoding();
    deb("calling setEncoding: "+encoding);
    if( !encoding.length() ) return;
    delete foo;

    msg("All open tabs/views will be refreshed.");
    m_pIDSQL->setEncoding(encoding);
    for( int i = 0; i < m_tabWdgt->count(); ++i )
    {
        QWidget* pWdgt = m_tabWdgt->widget(i);
        TabEdit * pTE = (TabEdit*) pWdgt;
        pTE->setEncoding(encoding);
        pTE->okClick();
    }
    //msg("Please refresh all open tabs/views");
    //msg("text: "+GString(action->text().toStdString().c_str())+", cfB: "+GString(m_iCharForBit));
}

QMenu * Pmf::createCharForBitMenu()
{

    QStringList charForBit;
    charForBit << "Hide data" << "Show as binary" << "Show as HEX" << "Auto" ;
    m_charForBitMenu = new QMenu("CharForBit Data", this);
    QAction * ac;
    QSettings settings(_CFG_DIR, "pmf6");

    //The 2nd param is the default value if nothing is found
    QString prev = settings.value("charForBit", "Auto").toString();
    m_iCharForBit = DATA_AUTO;
    for (int i = 0; i < charForBit.size(); ++i)
    {	
        ac = new QAction(charForBit.at(i).toLocal8Bit(), this);
        ac->setCheckable(true);
        if( prev == QString(charForBit.at(i).toLocal8Bit()) )
        {
            ac->setChecked(true);
            if( prev == QString("Hide data") ) m_iCharForBit = DATA_HIDE;
            else if( prev == QString("Show as binary") ) m_iCharForBit = DATA_BIN;
            else if( prev == QString("Show as HEX") ) m_iCharForBit = DATA_HEX;
        }
        else ac->setChecked(false);
        m_charForBitMenu->addAction(ac);
        connect(ac, SIGNAL(triggered()), this, SLOT(setCharForBit()));
    }
    return m_charForBitMenu;
}

void Pmf::setCharForBit()
{  
	QList<QAction*> actionList = m_charForBitMenu->actions();
	if (actionList.empty()) return;
	for( int i = 0; i < actionList.count(); ++i ) actionList.at(i)->setChecked(false);

	QAction *action = qobject_cast<QAction *>(sender());
	if( !action ) return;	
	action->setChecked(true);
    QSettings settings(_CFG_DIR, "pmf6");
	settings.setValue("charForBit", action->text());	
    if( action->text() == "Hide data" ) m_iCharForBit = DATA_HIDE ;
    else if( action->text() == "Show as binary" ) m_iCharForBit = DATA_BIN;
    else if( action->text() == "Show as HEX" ) m_iCharForBit = DATA_HEX;
    else m_iCharForBit = DATA_AUTO;
	msg("Click 'Open' or 'Run Cmd' to refresh the view.");
	//msg("text: "+GString(action->text().toStdString().c_str())+", cfB: "+GString(m_iCharForBit));
}
QMenu * Pmf::createRestoreMenu()
{
    QStringList restore;
    restore << "Ask" << "Restore" << "Never" ;
    m_restoreMenu = new QMenu("Restore prev. session");
    QAction * ac;
    QSettings settings(_CFG_DIR, "pmf6");
    QString prev = settings.value("restore", "Ask").toString();

    m_iRestore = 1;
    for (int i = 0; i < restore.size(); ++i)
    {
        ac = new QAction(restore.at(i).toLocal8Bit(), this);
        ac->setCheckable(true);
        if( prev == QString(restore.at(i).toLocal8Bit()) )
        {
            ac->setChecked(true);            
            if( prev == QString("Restore") ) m_iRestore = 2;
            else if( prev == QString("Never") ) m_iRestore = 3;
        }
        else ac->setChecked(false);
        m_restoreMenu->addAction(ac);
        connect(ac, SIGNAL(triggered()), this, SLOT(setRestore()));
    }
    return m_restoreMenu;
}
void Pmf::setRestore()
{
    QList<QAction*> actionList = m_restoreMenu->actions();
    if (actionList.empty()) return;
    for( int i = 0; i < actionList.count(); ++i ) actionList.at(i)->setChecked(false);

    QAction *action = qobject_cast<QAction *>(sender());
    if( !action ) return;
    action->setChecked(true);
    QSettings settings(_CFG_DIR, "pmf6");
    settings.setValue("restore", action->text());
    if( action->text() == "Restore" ) m_iRestore = 2;
    else if( action->text() == "Never" ) m_iRestore = 3;
    else m_iRestore = 1;
}

void Pmf::curTabChg(int index)
{
    PMF_UNUSED(index);

	//This tab just got clicked.
	//Notify previously selected tab that is has lost focus.
	if( m_iCurrentTab >= 0 )
	{
		QWidget* pWdgt = m_tabWdgt->widget(m_iCurrentTab);
		if( pWdgt ) 
		{
			TabEdit * pTE = (TabEdit*) pWdgt;
			if( pTE ) pTE->lostFocus();	
		}
	}
	//Store the newly selected tab's index
	m_iCurrentTab = m_tabWdgt->currentIndex();
	if( m_iCurrentTab < 0 ) return;
	
	//Notify the tab that it has the focus now
	QWidget* pWdgt = m_tabWdgt->widget(m_iCurrentTab);
	if( pWdgt ) 
	{
		TabEdit * pTE = (TabEdit*) pWdgt;
		if( pTE ) pTE->gotFocus();	
	}
}
void Pmf::prevTab()
{
	if( !m_tabWdgt->count() ) return;
	if( m_tabWdgt->currentIndex() == 0 ) m_tabWdgt->setCurrentIndex(m_tabWdgt->count()-1);
	else m_tabWdgt->setCurrentIndex(m_iCurrentTab-1);
}
void Pmf::nextTab()
{
	if( !m_tabWdgt->count() ) return;
	if( m_iCurrentTab >= m_tabWdgt->count()-1 ) m_tabWdgt->setCurrentIndex(0);
	else m_tabWdgt->setCurrentIndex(m_iCurrentTab+1);

}
void Pmf::getGeometry(int *x, int *y, int *w, int *h )
{
	*x = this->x();
	*y = this->y();
	*w = this->width();
	*h = this->height();
}

void Pmf::addNewTab()
{
    createNewTab("", 0);
}

void Pmf::setColorScheme(PmfColorScheme scheme, QPalette palette)
{
    m_iColorScheme = scheme;
    m_qPalette = palette;
}
PmfColorScheme Pmf::getColorScheme()
{	
return PmfColorScheme::Standard;
    return m_iColorScheme;
}

void Pmf::closeTabClicked(int index)
{
    closeTab(index);
}

void Pmf::createNewTab(GString cmd, int asNext)
{

    deb("::createNewTab start ");
    deb("::createNewTab, asNext: "+GString(asNext)+", cmd: "+cmd);
    m_iTabIDCounter++;
    TabEdit * pTE = new TabEdit(this, m_tabWdgt, m_iTabIDCounter, m_pGDeb, m_iThreaded);
    pTE->setGDebug(m_pGDeb);
    pTE->setCheckBoxValues( &m_cbMenuActionSeq );    

    if( asNext )
    {
        if( m_iCurrentTab >= 0 )
        {
            TabEdit * pCurTE =  (TabEdit*) m_tabWdgt->widget(m_iCurrentTab);
            m_strLastSelectedContext = pCurTE->currentContext();
            m_strLastSelectedSchema = pCurTE->currentSchema();
        }

        m_tabWdgt->insertTab(m_iCurrentTab+1, pTE, _newTabString);
        m_tabWdgt->setCurrentIndex(m_iCurrentTab+1);
    }
    else
    {
        m_tabWdgt->addTab(pTE, _newTabString);
        m_tabWdgt->setCurrentIndex(m_tabWdgt->count()-1);
    }
    pTE->setFocus();
    m_iCurrentTab = m_tabWdgt->currentIndex();

    pTE->fillDBNameLE(m_gstrDBName, m_gstrConnectionColor);

    pTE->fillSchemaCB( m_strLastSelectedContext, m_strLastSelectedSchema);

    pTE->setHistTableName(m_strHistTableName);
    QString col = "background-color: ";
    col += (char*) m_gstrConnectionColor;
    menuBar()->setStyleSheet(col);


    //connect( m_tabWdgt, SIGNAL( currentChanged ( int ) ), this, SLOT( curTabChg(int) ) );


    //cmd is set when we restore a previous session.
    //Do NOT run cmd if it's UPDATE, DELETE, INSERT, ....
    //...and do not set cmd to uppercase...
    if( cmd.strip().length() && GString(cmd).upperCase().subString(1, 6) == "SELECT")
    {
        pTE->setCmdLine(cmd);
        //pTE->okClick();
        pTE->reload(cmd, pTE->isChecked(_AUTOLOADTABLE) ? -1 : 0);
        pTE->setTableNameFromStmt(cmd);
        pTE->fillSelectCBs();
    }
    pTE->loadCmdHist();
    deb("::createNewTab, installing evtFilters...");
    pTE->installEventFilter(this);
    if( !cmd.length() ) pTE->popupTableCB();



//    QWidget * tabCloseButton;
//    for(int i = 0; i < m_tabWdgt->count(); ++i)
//    {
//        tabCloseButton = m_tabWdgt->tabBar()->tabButton(i, QTabBar::RightSide);
//        if( tabCloseButton != 0 )
//        {
//            tabCloseButton->resize(15, 15);
//        }
//    }
}

int Pmf::closeTab(int index)
{
    if( index < 0 ) return 0;
    QWidget* pWdgt = m_tabWdgt->widget(index);

    TabEdit * pTE = (TabEdit*) pWdgt;
    if( !pTE->canClose() ) return 1;

    //pWdgt->close();
    m_tabWdgt->removeTab(index);
    delete pWdgt;
    pWdgt = NULL;
    return 0;
}

int Pmf::closeCurrentTab()
{
    //msg("Current: "+GString(m_tabWdgt->currentIndex()));
    int index = m_tabWdgt->currentIndex();
    return closeTab(index);
}

int Pmf::closeDBConn()
{
    if( !m_gstrDBName.length() || m_gstrDBName == _NO_DB ) return 0;
		
	if( QMessageBox::question(this, "PMF", "Close this connection?", QMessageBox::Yes, QMessageBox::No) != QMessageBox::Yes ) return 1;

    setWindowTitle( Helper::pmfNameAndVersion() );
    disconnect( m_tabWdgt, SIGNAL( currentChanged ( int ) ), this, SLOT( curTabChg(int) ) );
    savePrevious();
	while(m_tabWdgt->count())
    {	        
        if( closeCurrentTab() )
        {
            connect( m_tabWdgt, SIGNAL( currentChanged ( int ) ), this, SLOT( curTabChg(int) ) );
            return 1;
        }
    }
    if( m_pIDSQL )
    {
        m_pIDSQL->disconnect();
        delete m_pIDSQL;
        m_pIDSQL = NULL;
    }
    m_iCurrentTab = -1;
    connect( m_tabWdgt, SIGNAL( currentChanged ( int ) ), this, SLOT( curTabChg(int) ) );
    return 0;
}

int Pmf::loginClicked()
{
	
    deb("loginClicked start");
    checkMigration();
    reconnectTimer->stop();
    reconnectInfoBox->hide();
    if( closeDBConn() > 0 ) return 0;

    int x, y, w, h;
    this->getGeometry(&x, &y, &w, &h);
    printf("in pmf, calling LoginBox, pmfGeometry: x: %i, y: %i, w: %i, h: %i\n", x, y, w, h);

    LoginBox * lb = new LoginBox(m_pGDeb, this);
    lb->exec();
    m_gstrDBType = lb->DBType();
	m_gstrDBName = lb->DBName();
    m_gstrUID =  lb->UserName();
    m_gstrPWD =  lb->PassWord();
    m_gstrNODE = lb->HostName();
    m_gstrPort = lb->Port();

    deb("loginClicked, box done");
    m_pIDSQL = lb->getConnection();
    deb("loginClicked, creating menu...");
    if( !m_pIDSQL )
    {
        createMenu("");
        deb("loginClicked: m_pIDSQL is NULL.");
        return 1;
    }
    else createMenu(m_pIDSQL->getDBTypeName());
    deb("loginClicked, check for _NO_DB...");
	
    if( m_gstrDBName == _NO_DB ) return 1;

    CON_SET curSet;
    m_pIDSQL->currentConnectionValues(&curSet);
    GString conInf = curSet.DB+" ["+curSet.Type+" on "+curSet.Host+":"+curSet.Port+"] - "+Helper::pmfNameAndVersion();
    ConnSet cs;
    cs.initConnSet();
    m_gstrConnectionColor = cs.getConnectionColor(curSet.Type, curSet.DB, curSet.Host, curSet.Port, curSet.UID);
    m_iReconnectWaitTime  = cs.getReconnectTimeout(curSet.Type, curSet.DB, curSet.Host, curSet.Port, curSet.UID);
    m_gstrPwdCmd          = cs.getPwdCmd(curSet.Type, curSet.DB, curSet.Host, curSet.Port, curSet.UID);
    deb("m_iReconnectWaitTime: "+GString(m_iReconnectWaitTime)+", m_gstrPwdCmd: "+m_gstrPwdCmd);

    setWindowTitle(conInf);
    m_strHistTableName = histTableName();
	deb("loginClicked, restoring...");
    setFontFromSettings();

    if( !restorePrevious() )
    {
        if( m_tabWdgt->count() == 0 )createNewTab();

		deb("loginClicked, restoring "+GString(m_tabWdgt->count())+" tabs...");
        for( int i = 0; i < m_tabWdgt->count(); ++i )
        {
            QWidget* pWdgt = m_tabWdgt->widget(i);
            TabEdit * pTE = (TabEdit*) pWdgt;
            pTE->fillDBNameLE(m_gstrDBName, m_gstrConnectionColor);
            pTE->fillSchemaCB(m_actHideSysTabs->isChecked() ? 1 : 0 );
            pTE->loadCmdHist();
        }
    }
	deb("loginClicked, got connInfo.");
    lb->close();
    delete lb;
	


    if( m_iReconnectWaitTime > 0 && m_gstrPwdCmd.length() )
    {
        timerCount = 0;
        reconnectTimer->start( 1000 );
        reconnectInfoBox->show();
    }
    //VersionCheck
    versionCheckTimer = new QTimer( this );
    connect( versionCheckTimer, SIGNAL(timeout()), this, SLOT(versionCheckTimerEvent()) );
    versionCheckTimer->start( 1000 );


    aThread = new MyThread;
    aThread->setOwner( this );
    aThread->start();

    //checkForUpdate(1);
//	Helper::showHintMessage(this, 1002);
//    Helper::showHintMessage(this, 1003);
	deb("loginClicked OK");
    return 0;
}

void Pmf::createGUI()
{

}

void Pmf::msg(GString txt)
{
	QMessageBox::information(this, "pmf", txt);
}

GString Pmf::currentSchema()
{
	if( m_tabWdgt->currentIndex() < 0 ) return "";
	GString s = m_tabWdgt->tabText(m_tabWdgt->currentIndex());
    if( s == _newTabString )
    {
         return "";
    }
    return s.subString(1, s.lastIndexOf(".")-1);
}
GString Pmf::currentTable()
{
	if( m_tabWdgt->currentIndex() < 0 ) return "";
	
	QWidget* pWdgt = m_tabWdgt->widget(m_tabWdgt->currentIndex());
	TabEdit * pTE = (TabEdit*) pWdgt;
	
	return pTE->currentTable(1);	
}
void Pmf::setLastSelectedSchema(GString context, GString schema)
{
	m_strLastSelectedContext = context;
    m_strLastSelectedSchema = schema;
}

/***********************************************************************
*
* SLOTS
*
***********************************************************************/
void Pmf::setConfig()
{
    PmfCfg * foo = new PmfCfg(m_pGDeb, this);
	foo->exec();
    PmfCfg aCFG(m_pGDeb);
	GString mr = aCFG.getValue("MaxRows");
	for( int i = 0; i < m_tabWdgt->count(); ++i )
	{
		QWidget* pWdgt = m_tabWdgt->widget(i);
		TabEdit * pTE = (TabEdit*) pWdgt;        
		pTE->setMaxRows(mr);
	}
}


void Pmf::setConnections()
{

    NewConn * foo = new NewConn(this, m_pGDeb);
    //foo->initConnSet();
	foo->exec();
    delete foo;
}

void Pmf::getNewVersion()
{
    if( m_pDownloader ) return;

    m_pDownloader = new Downloader(m_pGDeb);

    connect(m_pDownloader, SIGNAL (downloadCompleted()), SLOT (handleDownloadResult()));
    //connect(m_pDownloader, SIGNAL (downloadFailed()), SLOT (downloadFailed));
    disconnect( versionCheckTimer, SIGNAL(timeout()), this, SLOT(versionCheckTimerEvent()) );
    versionCheckTimer->start(500);
    connect( versionCheckTimer, SIGNAL(timeout()), this, SLOT(checkDownloadSize()) );
    downloadInfoBox->show();
    downloadInfoLE->setText("Dowloading");
    remove( newVersionFileLocation() );
    m_pDownloader->getPmfSetup();
}

void Pmf::downloadCancelled()
{
    deb("pmf::cancelClicked\n");
    if( m_pDownloader )
    {
        m_pDownloader->cancelDownload();
    }    
}

void Pmf::checkDownloadSize()
{
    deb("In chckDlD size\n");
    int size = m_pDownloader->downloadedSize();    
    int perc = size / 120000;
    if( size > 0 )
    {
        downloadCancelButton->setEnabled(true);
        downloadInfoLE->setText("Downloading: "+GString(perc)+"% ");
    }
    else downloadInfoLE->setText("Connecting (this may take a while)...");
}

void Pmf::handleDownloadResult()
{
    versionCheckTimer->stop();
    downloadInfoBox->hide();
    if( m_pDownloader )
    {
        if( m_pDownloader->downloadedSize() < 0 )
        {
            msg("Download failed.");
            delete m_pDownloader;
            m_pDownloader = 0;
            return;
        }
    }
    downloadInfoBox->hide();
    if( m_pDownloader ) delete m_pDownloader;
    m_pDownloader = 0;

#if defined(MAKE_VC)

    GString fileName = newVersionFileLocation();
    int rc = (int)ShellExecute(0, 0, fileName, 0, 0 , SW_SHOW );
    if( rc < 32 )
    {
        QProcess * qp = new QProcess(this);
        QString prog = "RUNDLL32.EXE SHELL32.DLL,OpenAs_RunDLL "+fileName;
        qp->start(prog);
    }
#endif
}

void Pmf::showInfo()
{
    GString s = "PMF (Poor Man's Flight) "+Helper::pmfVersion();
    s += "\n(C) Gregor Leipelt \n\nSee www.leipelt.net for updates and documentation";
    s += "\nQt version: "+GString(qVersion());
	msg(s);    
}
void Pmf::exportData()
{
    QWidget* pWdgt = m_tabWdgt->widget(m_tabWdgt->currentIndex());
    TabEdit * pTE = (TabEdit*) pWdgt;
    QSettings settings(_CFG_DIR, "pmf6");
    m_gstrPrevExportPath = settings.value("exportPath", "").toString();

    if( !m_gstrPrevExportPath.length() ) m_gstrPrevExportPath = m_gstrPrevImportPath;

    ExportBox * foo = new ExportBox(m_pIDSQL, this, &m_gstrPrevExportPath);
    foo->setSelect(pTE->lastSqlSelectCmd(), pTE->currentTable() );
    foo->exec();
}


void Pmf::importData()
{
    QSettings settings(_CFG_DIR, "pmf6");
    m_gstrPrevImportPath = settings.value("importPath", "").toString();

	if( !m_gstrPrevImportPath.length() ) m_gstrPrevImportPath = m_gstrPrevExportPath;
    ImportBox * foo = new ImportBox(m_pIDSQL, this, currentSchema(), currentTable(), &m_gstrPrevImportPath);
    foo->setGDebug(m_pGDeb);
	foo->exec();
    delete foo;
}


void Pmf::deleteTableContents()
{
    TabEdit* pTE = getCurrentTabWdgt();
    if( !pTE ) return;
    pTE->slotRightClickDeleteTable();
}



void Pmf::catalogDBs()
{
    CatalogInfo *foo = new CatalogInfo(m_pIDSQL, this);
    foo->createDisplay();
    foo->exec();
//    SimpleShow * foo = new SimpleShow("DBInfo", this);
//    DBAPIPlugin* pApi = new DBAPIPlugin(m_pIDSQL->getDBTypeName());
//    if( !pApi )
//    {
//        foo->setText("Could not load required plugin, sorry.");
//        return;
//    }
//    GSeq <DB_INFO*> dataSeq;
//    dataSeq = pApi->dbInfo();
//    GString out;
//    DB_INFO * pInfo;
//    for (int i = 1; i <= (int)dataSeq.numberOfElements(); ++i )
//    {
//        pInfo = dataSeq.elementAtPosition(i);
//        out += "Name: "+pInfo->Name+"\n";
//        out += "Alias: "+pInfo->Alias+"\n";
//        out += "Drive: "+pInfo->Drive+"\n";
//        //out += "Directory: "+pInfo->Directory+"\n";
//        out += "NodeName: "+pInfo->NodeName+"\n";
//        out += "Release Level: "+pInfo->ReleaseLevel+"\n";
//        out += "Comment: "+pInfo->Comment+"\n";
//        out += "DB Type: "+pInfo->DbType+"\n";
//        out += "*************************\n";
//    }
//    dataSeq.deleteAll();
//    delete pApi;
//    foo->setText(out);
//    foo->exec();
}

void Pmf::showDatabaseInfo()
{
    SimpleShow * foo = new SimpleShow("DBInfo", this);
    DBAPIPlugin* pApi = new DBAPIPlugin(m_pIDSQL->getDBTypeName());
    if( !pApi )
    {
        foo->setText("Could not load required plugin, sorry.");
        return;
    }
    GSeq <DB_INFO*> dataSeq;
    dataSeq = pApi->dbInfo();
    GString out;
    DB_INFO * pInfo;
    for (int i = 1; i <= (int)dataSeq.numberOfElements(); ++i )
    {
        pInfo = dataSeq.elementAtPosition(i);
        //out += "Name: "+pInfo->Name+"\n";
        out += "Alias: "+pInfo->Alias+"\n";
        out += "Drive: "+pInfo->Drive+"\n";
        //out += "Directory: "+pInfo->Directory+"\n";
        out += "NodeName: "+pInfo->NodeName+"\n";
        //out += "Release Level: "+pInfo->ReleaseLevel+"\n";
        out += "Comment: "+pInfo->Comment+"\n";
        out += "DB Type: "+pInfo->DbType+"\n";
        out += "*************************\n";
    }
    dataSeq.deleteAll();

    delete pApi;
    foo->setText(out);
    foo->exec();


}

void Pmf::createDDLs()
{
    AllTabDDL * foo = new AllTabDDL(m_pIDSQL, this, currentSchema(), m_actHideSysTabs->isChecked() ? 1 : 0 );
    foo->exec();
    delete foo;
}

void Pmf::deleteTable()
{
    Deltab * foo = new Deltab(m_pIDSQL, this, currentSchema(), m_actHideSysTabs->isChecked() ? 1 : 0 );
	foo->exec();
    delete foo;
}
void Pmf::findIdenticals()
{	
    Finddbl * foo = new Finddbl(m_pGDeb, m_pIDSQL, this, currentSchema(), m_actHideSysTabs->isChecked() ? 1 : 0);
	foo->exec();
    delete foo;
}

TabEdit * Pmf::getCurrentTabWdgt()
{
    if( checkTableSet() ) return NULL;

    if( m_iCurrentTab >= 0 )
    {
        QWidget* pWdgt = m_tabWdgt->widget(m_iCurrentTab);
        if( pWdgt )
        {
            return (TabEdit*)pWdgt;
        }
    }
    return NULL;
}

void Pmf::runstats()
{
    TabEdit * pTE = getCurrentTabWdgt();
    if( !pTE ) return;
    ReorgAll * foo = new ReorgAll(m_pIDSQL, pTE, currentTable(), m_actHideSysTabs->isChecked() ? 1 : 0);
    foo->exec();
    delete foo;
}

void Pmf::tableSizes()
{
    TableSize * foo = new TableSize(m_pGDeb, m_pIDSQL, this, currentSchema(), m_actHideSysTabs->isChecked() ? 1 : 0);
	foo->exec();
    delete foo;
}
void Pmf::getTabSpace()
{
    TabSpace * foo = new TabSpace(m_pIDSQL, this);
	foo->exec();	
    delete foo;
}
void Pmf::snapShot()
{
    GetSnap * foo = new GetSnap(m_pIDSQL, this);
    //!Dark this->getColorScheme();

	foo->setDBName(m_gstrDBName);
	if( !m_gstrNODE.length() ) m_gstrNODE = "DB2";
	foo->exec();	
    delete foo;
}
void Pmf::createDDL()
{
    if( checkTableSet() ) return;

    if( m_iCurrentTab >= 0 )
    {
        QWidget* pWdgt = m_tabWdgt->widget(m_iCurrentTab);
        if( pWdgt )
        {
            TabEdit * pTE = (TabEdit*) pWdgt;
            pTE->runGetClp();
        }
    }
}
void Pmf::queryDB()
{
    Querydb * foo = new Querydb(m_pIDSQL, this,currentSchema(), m_actHideSysTabs->isChecked() ? 1 : 0);
    foo->show();
    //foo->exec();
}
void Pmf::keyPressEvent(QKeyEvent * key)
{
    //The events are currently eaten in tabEdit.
    //if( key->key() == Qt::Key_Escape ) { key->ignore();}	///this->close();
    if( key->key() == Qt::Key_Escape ) this->close();
    else QMainWindow::keyPressEvent(key);
}
void Pmf::closePMF()
{
    if( QMessageBox::question(m_tabWdgt, "PMF", "Quit PMF?", QMessageBox::Yes, QMessageBox::No) != QMessageBox::Yes )
    {
        return;
    }
    disconnect( m_tabWdgt, SIGNAL( currentChanged ( int ) ), this, SLOT( curTabChg(int) ) );
    this->close();
}

void Pmf::closeEvent(QCloseEvent * event)
{
    deb("got close evt");
    disconnect( m_tabWdgt, SIGNAL( currentChanged ( int ) ), this, SLOT( curTabChg(int) ) );
    if( m_iForceClose )
    {
        while(m_tabWdgt->count())  closeCurrentTab();
        deb("close forced");
        event->accept();
        return;
    }
    /*
    if( QMessageBox::question(m_tabWdgt, "PMF", "Quit PMF?", QMessageBox::Yes, QMessageBox::No) != QMessageBox::Yes )
    {
        event->ignore();
        return;
    }
    */


    deb("Closing...");
    savePrevious();
	while(m_tabWdgt->count())
	{	
        if( closeCurrentTab() )
		{
			event->ignore();
			return;
		}
	}

    deb("got evt");
	event->accept();
    deb("Deleting IDSQL");
}

void Pmf::quitPMF()
{
	this->close();
}

void Pmf::resetPmfFont()
{
    QSettings settings(_CFG_DIR, "pmf6");
    settings.remove("font");
    msg("Please restart PMF for changes to take effect.");
}



void Pmf::setPmfFont()
{

	bool ok;
	QFont orgFont = this->font();
	QFont font = QFontDialog::getFont(&ok, orgFont, this);
	if (ok) 
	{
        QApplication::setFont(font);
        this->setFont(font);
        this->repaint();
        QCoreApplication::processEvents();
        msg("Please restart PMF for changes to take effect.");
        for( int i = 0; i < m_tabWdgt->count(); ++i )
        {
            QWidget* pWdgt = m_tabWdgt->widget(i);
            TabEdit * pTE = (TabEdit*) pWdgt;
            //pTE->setCellFont(&font);
        }
        QSettings settings(_CFG_DIR, "pmf6");
        settings.setValue("font", font.toString());
//        m_mnuMainMenu->setFont(font);
//        m_mnuAdmMenu->setFont(font);
//        m_mnuTableMenu->setFont(font);
//        m_mnuBookmarkMenu->setFont(font);
//        m_mnuSettingsMenu->setFont(font);
//        m_mnuMelpMenu->setFont(font);
	}

}

int Pmf::checkTableSet()
{
	if( !currentSchema().length() || !currentTable().length() )
	{
		msg("Please select/open a table.");
		return 1;
	}
	return 0;
}
void Pmf::addBookmark()
{
	QWidget* pWdgt = m_tabWdgt->widget(m_tabWdgt->currentIndex());
	TabEdit * pTE = (TabEdit*) pWdgt;
	if( pTE->currentTable().strip(".") == _selStringCB && 0 == pTE->getSQL().length()  )
	{
		msg("There is nothing to bookmark. At least open a table.");
		return;
	}
	
    Bookmark * foo = new Bookmark(m_pGDeb, this, pTE->getSQL(), pTE->currentTable());
	foo->exec();
	getBookmarks();
}
void Pmf::editBookmark()
{	
    EditBm * foo = new EditBm(m_pGDeb, this);
	foo->exec();
	getBookmarks();
}

void Pmf::setBookmarkData()
{
	QAction *action = qobject_cast<QAction *>(sender());
	if( !action ) return;
	int pos = action->data().toInt();
	
    BookmarkSeq bmSeq(m_pGDeb);
	QWidget* pWdgt = m_tabWdgt->widget(m_tabWdgt->currentIndex());
	TabEdit * pTE = (TabEdit*) pWdgt;

    BOOKMARK * pBm = bmSeq.getBookmark(pos);
    if( !pBm ) return;
    pTE->loadBookmark(pBm->Table, pBm->SqlCmd.change('\n', " "));
}
void Pmf::getBookmarks()
{
	deb("getBookmarks, start.");
	/***************************************************
	* This is rather nice: The menu-items are created during
	* run-time, the items are connected to the same slot.
	* See http://doc.trolltech.com/4.6/mainwindows-recentfiles-mainwindow-cpp.html
	*/
    m_mnuBookmarkMenu->clear();
    m_mnuBookmarkMenu->addAction("Add bookmark", this, SLOT(addBookmark()));
    m_mnuBookmarkMenu->addAction("Edit bookmarks", this, SLOT(editBookmark()));
    m_mnuBookmarkMenu->addSeparator();
	
	deb("calling bm ctor...");
    BookmarkSeq * bm = new BookmarkSeq();
    deb("getBookmark");
	//QAction * bmActs = new QAction[bm->count()];
	for( int i = 1; i <= bm->count() && i < MaxBookmarkActs; ++i )
	{
		bmActs[i-1] = new QAction(this);
		bmActs[i-1]->setText(bm->getBookmarkName(i));
		bmActs[i-1]->setData(i);
        m_mnuBookmarkMenu->addAction(bmActs[i-1]);
		connect(bmActs[i-1], SIGNAL(triggered()), this, SLOT(setBookmarkData()));		
	}
	deb("bookmarks done, calling dtor...");
	delete bm;
	deb("bookmarks OK.");
}
GString Pmf::restoreFileName(int checkForOldFiles )
{
    QString home = QDir::homePath ();
    if( !home.length() ) return "";
    GString path;

    path = basePath() + _RESTORE_FILE;
    /*
#if defined(MAKE_VC) || defined (__MINGW32__)
    path = GString(home)+"\\"+_CFG_DIR+"\\"+_RESTORE_FILE;
#else
    path = GString(home)+"/."+_CFG_DIR + "/"+_RESTORE_FILE;
#endif
*/

    if( checkForOldFiles == 1 )
    {
        return path + m_gstrDBName;
    }
    else if( checkForOldFiles == 2 )
    {
        return path +m_gstrDBType+"_"+ m_gstrDBName;
    }
    else
    {
        return path +m_gstrDBType+"_"+m_gstrNODE+"_"+m_gstrPort+"_"+ m_gstrDBName;
    }

}

int Pmf::restorePrevious()
{
    if( m_iRestore == 3 ) return 0;

    int i = 0;
    QString home = QDir::homePath ();
    if( !home.length() ) return 0;

    GFile f;
    f.readFile(restoreFileName());
    if( !f.initOK() ) f.readFile(restoreFileName(1));
    if( !f.initOK() ) f.readFile(restoreFileName(2));
    if( !f.initOK() ) return 0;


    if( m_iRestore == 1 &&  f.lines() > 0 )
    {
        if( QMessageBox::question(this, "PMF", "Restore previous session?\n\nTired of this? Go to Menu->Settings->Restore prev. session", QMessageBox::Yes, QMessageBox::No) != QMessageBox::Yes ) return 0;
    }
    for( i = 0; i < f.lines(); ++i )
    {
        createNewTab(f.getLine(i+1));
    }
    return i;
}
void Pmf::savePrevious()
{
    GString line;
    QWidget* pWdgt;
    TabEdit * pTE;
    int haveLines = 0;
    for( int i = 0; i < m_tabWdgt->count(); ++i )
    {
        pWdgt = m_tabWdgt->widget(i);
        pTE = (TabEdit*) pWdgt;
        line = pTE->getLastSelect();
        if( line.length() ) haveLines++;
    }
    if( !haveLines ) return;

    GFile f(restoreFileName(), GF_OVERWRITE);

    for( int i = 0; i < m_tabWdgt->count(); ++i )
    {
        pWdgt = m_tabWdgt->widget(i);
        pTE = (TabEdit*) pWdgt;
        line = pTE->getLastSelect();
        if( line.length() )f.addLine(line);
    }
}

void Pmf::createCheckBoxActions()
{
    QSettings *settings = new QSettings(_CFG_DIR, "pmf6");

    m_actTextCompleter = new QAction("Enable text completer", this);
    setAndConnectActions(settings, m_actTextCompleter, _TEXTCOMPLETER, "Y");

    m_actCountAllRows = new QAction("Count rows", this);
    setAndConnectActions(settings, m_actCountAllRows, _COUNTROWS);


    m_actReadUncommitted = new QAction("Read uncommitted rows", this);
    setAndConnectActions(settings, m_actReadUncommitted, _READUNCOMMITTED);

    m_actHideSysTabs = new QAction("Hide system tables", this);
    setAndConnectActions(settings, m_actHideSysTabs, _HIDESYSTABS);

    m_actRefreshOnFocus = new QAction("Refresh Tabs on focus", this);
    setAndConnectActions(settings, m_actRefreshOnFocus, _REFRESHONFOCUS, "N");

//    m_actShowCloseOnTabs = new QAction("Show close button on tabs", this);
//    setAndConnectActions(settings, m_actShowCloseOnTabs, _SHOWCLOSEONTABS, "Y");

    m_actConvertGuid = new QAction("Convert GUID", this);
    setAndConnectActions(settings, m_actConvertGuid, _CONVERTGUID, "Y");

    m_actAutoLoadTable = new QAction("Auto load table", this);
    setAndConnectActions(settings, m_actAutoLoadTable, _AUTOLOADTABLE, "Y");

    m_mnuSettingsMenu->addSeparator();

    m_actEnterToSave = new QAction("ENTER to Save/Update", this);
    setAndConnectActions(settings, m_actEnterToSave, _ENTERTOSAVE, "Y");

    m_actUseEscKey = new QAction("ESC closes PMF", this);
    setAndConnectActions(settings, m_actUseEscKey, _USEESCTOCLOSE);

    m_actVersionCheck = new QAction("Warn if new version is available", this);
    setAndConnectActions(settings, m_actVersionCheck, _CHECKSENABLED, "Y");

    m_mnuSettingsMenu->addSeparator();

}
void Pmf::setAndConnectActions(QSettings *settings,  QAction* action, QString name, QString defVal)
{
    QString prev;
    connect(action, SIGNAL(triggered()), SLOT(checkBoxAction()) );
    action->setCheckable(true);
    if( !defVal.length() ) defVal = name;
    prev = settings->value(name, defVal).toString();
    if( prev == "Y") action->setChecked(true);
    else action->setChecked(false);
    m_mnuSettingsMenu->addAction(action);

    CHECKBOX_ACTION * pCbAction = new CHECKBOX_ACTION;
    pCbAction->Action = action;
    pCbAction->Default = defVal;
    pCbAction->Name = name;
    m_cbMenuActionSeq.add(pCbAction);
//    if( action == m_actShowCloseOnTabs)
//    {
//        m_tabWdgt->setTabsClosable(action->isChecked() );
//    }
}

void Pmf::checkBoxAction()
{
    QAction *action = qobject_cast<QAction *>(sender());
    for( int i = 0; i < m_tabWdgt->count(); ++i )
    {        
        QWidget* pWdgt = m_tabWdgt->widget(i);
        TabEdit * pTE = (TabEdit*) pWdgt;
        if( action == m_actHideSysTabs) pTE->fillSchemaCB();
        pTE->setCheckBoxValues(&m_cbMenuActionSeq);
    }
    QSettings settings(_CFG_DIR, "pmf6");

    if( action == m_actTextCompleter) msg("Changes take effect after restart");
    else if( action == m_actCountAllRows && action->isChecked()) msg("Enabling this will do a \nSELECT COUNT(*) FROM <table>\nthe result will be displayed at the bottom in the 'info' field.\n\nBe aware that this may take a lot of time." );
    else if( action == m_actConvertGuid && action->isChecked()) msg("Enabling this will convert GUIDs between DB2, SqlServer and Postgres format.\nOnly useful when copying&pasting between those databases." );
    else if( action == m_actAutoLoadTable ) msg("- Enabled: When you select a table (from the drop-down), it will automatically load.\n- Disabled: After selecting a table, you need to click 'Open' to load it. Use this to avoid long loading times." );

    //if( action == m_actShowCloseOnTabs)m_tabWdgt->setTabsClosable(m_tabWdgt->tabsClosable() ? false : true );

    //Save in settings
    CHECKBOX_ACTION* pAction;
    for(int i = 1; i <= m_cbMenuActionSeq.numberOfElements(); ++i )
    {
        pAction = m_cbMenuActionSeq.elementAtPosition(i);
        settings.setValue( pAction->Name , pAction->Action->isChecked() ? "Y" : "N" );
    }
//    if( action == m_actShowCloseOnTabs)
//    {
//        m_tabWdgt->setTabsClosable(action->isChecked() );
//    }

}

void Pmf::deb(GString msg)
{    
    m_pGDeb->debugMsg("pmf", 1, msg);
}
void Pmf::showEvent( QShowEvent * evt)
{
    printf("Pmf::showEvent, start\n");
   if( m_iShowing ) return;
#ifndef MAKE_VC
        //return;
#endif
    QMainWindow::showEvent(evt);
    printf("Pmf::showEvent, calling login...\n");
    QTimer::singleShot(20, this, SLOT(callLogin()));
}
void Pmf::callLogin()
{    
    m_iShowing = 1;
    loginClicked();
    deb("callLogin done");	
}
void Pmf::showDebug()
{
    m_pGDeb->setParent(this);
    for( int i = 0; i < m_tabWdgt->count(); ++i )
    {
        QWidget* pWdgt = m_tabWdgt->widget(i);
        TabEdit * pTE = (TabEdit*) pWdgt;
        //!!pTE->setGDebug(m_pGDeb);
    }
}
void Pmf::showHelp()
{	
    bool res = QDesktopServices::openUrl(QUrl("http://leipelt.de/pmfhowto.html") );
    if( !res ) msg("Cannot open browser (?), please go to http://leipelt.de");
    return;
    /* Not working with darkMode
    QDialog * helpVw = new QDialog(this);
    QTextBrowser* browser = new QTextBrowser(helpVw);
    browser->resize(800,500);
	helpVw->setWindowTitle("PMF Help - ESC to close");
    browser->setSource( QUrl("qrc:///pmfhelp.htm") );
	helpVw->show();	
    */
			
}
DSQLPlugin* Pmf::getConnection()
{
    return m_pIDSQL;
}
GString Pmf::histTableName()
{
    if( m_pIDSQL->getDBType() != ODBCDB::DB2 && m_pIDSQL->getDBType() != ODBCDB::DB2ODBC ) return "";
    deb("histTableName, start");
    DSQLPlugin plg(*m_pIDSQL);
    plg.initAll("SELECT COUNT(*) FROM SYSCAT.TABLES WHERE TABSCHEMA='PMF' AND TABNAME='HISTORY'");
    if( plg.rowElement(1,1).asInt() == 1 ) return "PMF.HISTORY";
    deb("histTableName, done");
    return "";
}


GString Pmf::checkForUpdate(int dailyCheck)
{



    //This runs in a Thread. Do not call deb(...) here while the DebOutputWindow is open.
    deb("checkForUpdate start");
    QSettings dateSettings(_CFG_DIR, "pmf6");
    GString lastDate  = dateSettings.value("lastUpdCheck", "").toString();
    GString lastVer   = dateSettings.value("lastVerCheck", "").toString();
    GString checksEnabled = dateSettings.value("checksEnabled", "?").toString();


    //Do not check.
    deb("Enabled: "+checksEnabled+", daily: "+GString(dailyCheck));
    if( checksEnabled == "N" && dailyCheck) return "";
    if( checksEnabled == "?" )
    {
        return checksEnabled;
    }
    deb("checkForUpdate lastVer: "+lastVer+", PMF_VER: "+GString(PMF_VER));
    if(!lastVer.length())lastVer = GString(PMF_VER);

    GString today = QDate::currentDate().toString("yyyyMMdd");

	
    deb("LastCheck: "+GString(lastDate.asInt())+", today: "+GString(today.asInt()));
    if( lastDate.asInt() >= today.asInt() && dailyCheck ) return "";

    dateSettings.setValue("lastUpdCheck", QDate::currentDate().toString("yyyyMMdd"));


    GString server = "leipelt.de";
    int port = 80;
    GString verSite = "/curver6.html";

    GSocket sock;
    GString data, ver, fullVer;
    sock.set_non_blocking(true);
    GStuff::startStopwatch();
    int rc = sock.connect(server, port);
    int seconds = GStuff::getStopwatch();
    deb("Error from connect: "+GString(rc)+", waitTime: "+GString(seconds)+" seconds.");

    if( !rc )
    {
        rc = sock.sendRaw("GET "+verSite+" HTTP/1.0\r\n");
        deb("Rc send1: "+GString(rc));
        rc = sock.sendRaw("Host: "+server+":"+GString(port)+"\r\n");
        //deb("Rc send2: "+GString(rc));
        rc = sock.sendRaw("\r\n"); //<- always close with \r\n, this signals end of http request
        //deb("Rc send3: "+GString(rc));
        int lng = sock.recvRawText(&data);
        //deb("lng: "+GString(lng));
    }
    if( rc )
    {
        if( !dailyCheck ) msg("Sorry, cannot check for new version. Please go to \nwww.leipelt.org");
        return "";
    }
    seconds = GStuff::getStopwatch();
    //deb("Error from recRaw: "+GString(rc)+", total waitTime: "+GString(seconds)+" seconds, data: "+data);


    ver = fullVer = getVersionFromHTML(data);
    ver = ver.removeAll('.');

    deb("Ver: "+ver+", full: "+fullVer);
    //store lastVer: So we don't nag daily.

    if( ver.asInt() > GString(PMF_VER).asInt())
    {

        if( !dailyCheck )msg("Update available at <a href='http://leipelt.org/downld.html'>www.leipelt.org</a>");
        return fullVer;
    }
    else
    {
        if( !dailyCheck )msg("This version is up to date.\nTo enable or disable automatic check, click\nmenu->Settings ");
    }
    return "";
}
GString Pmf::getVersionFromHTML(GString data)
{
    GString ver;
    if( data.occurrencesOf("ver=")  )
    {
        ver = data.subString(data.indexOf("ver=")+4, data.length()).strip();
        if( ver.occurrencesOf("<") ) ver = ver.subString(1, ver.indexOf("<")-1);
        ver = ver.strip().stripTrailing("\n").strip();
    }
    return ver;
}

TabEdit * Pmf::currentTabEdit()
{
    if( checkTableSet() ) return NULL;
    if( m_iCurrentTab >= 0 )
    {
        QWidget* pWdgt = m_tabWdgt->widget(m_iCurrentTab);
        if( pWdgt )
        {
            return (TabEdit*) pWdgt;
        }
    }
    return NULL;
}

void Pmf::refreshTabOrder()
{
    for( int i = 0; i < m_tabWdgt->count(); ++i )
    {
        TabEdit * pTE = (TabEdit*) m_tabWdgt->widget(i);
        pTE->setNewIndex(i);
    }
}

void Pmf::checkMigration()
{
    SelectEncoding::convertToXml();

    GString home = GString(QDir::homePath());
    GString oldSettings;
    GString newSettings;
#if defined(MAKE_VC) || defined (__MINGW32__)
    oldSettings = home+"\\pmf5";
    newSettings = home+"\\"+_CFG_DIR;
#else
    oldSettings = home+"/.pmf5";
    newSettings = home+"/."+_CFG_DIR;
#endif

    if( QDir().exists(newSettings)) return;
    if( !QDir().exists(oldSettings)) return;
    GString msg = "Do you want to move your settings from\n"+oldSettings+" to\n"+newSettings+" ?\n\n";
    msg += "If you are using pmf5 along with pmf6, click No.\n";
    msg += "If in doubt, click Yes.";
    if( QMessageBox::question(this, "PMF", msg, QMessageBox::Yes, QMessageBox::No) != QMessageBox::Yes ) return;

    rename(oldSettings, newSettings);


}
void Pmf::addToHostVarSeq(GSeq <GString> * pSeq)
{
    GString elmt;
    int found = 0;
    for(int j=1; j <= (int)pSeq->numberOfElements(); ++j)
    {
        found = 0;
        elmt = pSeq->elementAtPosition(j);
        for(int i=1; i <= (int) m_hostVarSeq.numberOfElements(); ++i)
        {
            if( m_hostVarSeq.elementAtPosition(i) == elmt )
            {
                found = 1;
                break;
            }
        }
        if( !found )m_hostVarSeq.add(elmt);
    }
    m_hostVarSeq.sort();
}
GSeq <GString>* Pmf::sqlCmdSeq()
{
    if( m_sqlCmdSeq.numberOfElements() == 0 )
    {
        m_sqlCmdSeq.add("select");
        m_sqlCmdSeq.add("update");
        m_sqlCmdSeq.add("insert");
        m_sqlCmdSeq.add("delete");
        m_sqlCmdSeq.add("from");
        m_sqlCmdSeq.add("where");
        m_sqlCmdSeq.add("and");
        m_sqlCmdSeq.add("or");
        m_sqlCmdSeq.add("order");
        m_sqlCmdSeq.add("by");
        m_sqlCmdSeq.add("not");
        m_sqlCmdSeq.add("in");
        m_sqlCmdSeq.add("right");
        m_sqlCmdSeq.add("left");
        m_sqlCmdSeq.add("outer");
        m_sqlCmdSeq.add("inner");
        m_sqlCmdSeq.add("upper");
        m_sqlCmdSeq.add("lower");
        m_sqlCmdSeq.add("coalesce");
        m_sqlCmdSeq.add("like");

        m_sqlCmdSeq.add("@DATA@");
        m_sqlCmdSeq.add("@COL@");
        m_sqlCmdSeq.add("@TABLE@");
    }
    return &m_sqlCmdSeq;
}

void Pmf::setFontFromSettings()
{
    QFont f;
    QSettings settings(_CFG_DIR, "pmf6");
    if( settings.value("font", -1).toInt() >= 0 )
    {
        f.fromString(settings.value("font").toString());
        QApplication::setFont(f);
    }
}


GSeq <GString>* Pmf::hostVarSeq()
{
    return &m_hostVarSeq;
}

QStringList Pmf::completerStringList()
{
    sqlCmdSeq();
    QStringList completerStrings;
    for(int i = 1; i <= (int)m_hostVarSeq.numberOfElements(); ++i) completerStrings += m_hostVarSeq.elementAtPosition(i);
    for(int i = 1; i <= (int)m_sqlCmdSeq.numberOfElements(); ++i) completerStrings += m_sqlCmdSeq.elementAtPosition(i);
    return completerStrings;
}

void Pmf::bookmarkMenuClicked()
{
    getBookmarks();
}

/*
bool pmf::eventFilter(QObject* object, QEvent* event)
{

    if( event->type() == QEvent::WindowActivate )
    {
        for( int i = 0; i < m_tabWdgt->count(); ++i )
        {
            QWidget* pWdgt = m_tabWdgt->widget(i);
            tabEdit * pTE = (tabEdit*) pWdgt;
            if( pTE == object)
            {
                printf("Activate\n");
                if( pTE->isChecked(_REFRESHONFOCUS) ) pTE->okClick();
            }
        }
    }
    if( event->type() == QEvent::WindowDeactivate )
    {
        printf("DeActivate\n");
    }

    return false;
}
*/
