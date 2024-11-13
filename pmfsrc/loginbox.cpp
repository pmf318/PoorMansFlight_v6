//
//  This file is part of PMF (Poor Man's Flight), see README.PMF for details
//  Author: Gregor Leipelt (gregor@leipelt.de)  (C) G. Leipelt 2000
//

#include "loginbox.h"
#include "catalogDB.h"

#include <QLabel>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QKeyEvent>
#include <QCloseEvent>
#include <QSortFilterProxyModel>
#include <QSettings>
#include <QTextBrowser>
#include <QProcess>
#include <QDomComment>

#include <qlayout.h>
#include "pmfdefines.h"
#include <gstuff.hpp>
#include <qfont.h>
#include <gfile.hpp>
#include "helper.h"
#include "gxml.hpp"
#include "addDatabaseHost.h"
#include "selectEncoding.h"
#include "newConn.h"
#include "gkeyval.hpp"

#include <dsqlplugin.hpp>
#include <dbapiplugin.hpp>

#if defined(MAKE_VC) || defined (__MINGW32__)
#include <io.h>
#else
#include <unistd.h>
#endif



LoginBox::LoginBox( GDebug *pGDeb, QWidget* parent ): QDialog(parent)
{

    m_pGDeb = pGDeb;
    this->setWindowTitle("Connect");
	QVBoxLayout *topLayout = new QVBoxLayout( );
	
    m_pMainGrid = new QGridLayout(this);
    topLayout->addLayout( m_pMainGrid, 10 );
	
	QLabel* tmpQLabel;

    int startRow = 0;
    tmpQLabel = new QLabel( this);
    tmpQLabel->setText( "Type" );
    tmpQLabel->setFixedHeight( tmpQLabel->sizeHint().height() );
    m_pMainGrid->addWidget(tmpQLabel, startRow+0, 0);

	tmpQLabel = new QLabel( this);
    tmpQLabel->setText( "Host:Port" );
	tmpQLabel->setFixedHeight( tmpQLabel->sizeHint().height() );
    m_pMainGrid->addWidget(tmpQLabel, startRow+1, 0);
	
	tmpQLabel = new QLabel( this );
    tmpQLabel->setText( "Database" );
	tmpQLabel->setFixedHeight( tmpQLabel->sizeHint().height() );
    m_pMainGrid->addWidget(tmpQLabel, startRow+2, 0);
	
	tmpQLabel = new QLabel( this );
    tmpQLabel->setText( "User" );
	tmpQLabel->setFixedHeight( tmpQLabel->sizeHint().height() );
    m_pMainGrid->addWidget(tmpQLabel, startRow+3, 0);

    tmpQLabel = new QLabel( this );
    tmpQLabel->setText( "Password" );
    tmpQLabel->setFixedHeight( tmpQLabel->sizeHint().height() );
    m_pMainGrid->addWidget(tmpQLabel, startRow+4, 0);


//    tmpQLabel = new QLabel( this );
//    tmpQLabel->setText( "Port" );
//    tmpQLabel->setFixedHeight( tmpQLabel->sizeHint().height() );
//    m_pMainGrid->addWidget(tmpQLabel, startRow+5, 0);



    dbTypeCB = new QComboBox( );
    dbTypeCB->setFixedHeight( dbTypeCB->sizeHint().height() );
    m_pMainGrid->addWidget(dbTypeCB, startRow+0, 1, 1, 2);


    hostNameCB = new QComboBox();
    hostNameCB->setFixedHeight( hostNameCB->sizeHint().height() );
    m_pMainGrid->addWidget(hostNameCB, startRow+1, 1, 1, 2);
    hostNameCB->setInsertPolicy(QComboBox::InsertAlphabetically);


    dbNameCB = new QComboBox( );
	dbNameCB->setFixedHeight( dbNameCB->sizeHint().height() );
    dbNameCB->setEditable(true);
    m_pMainGrid->addWidget(dbNameCB, startRow+2, 1, 1, 2);
    dbNameCB->setInsertPolicy(QComboBox::InsertAlphabetically);



	
    userNameCB = new QComboBox(  );
    userNameCB->setFixedHeight( userNameCB->sizeHint().height() );
    userNameCB->setEditable(true);
    m_pMainGrid->addWidget(userNameCB, startRow+3, 1, 1, 2);
	
	
	passWordLE = new QLineEdit();
	passWordLE->setText( "" );
	passWordLE->setEchoMode(QLineEdit::Password);
	passWordLE->setFixedHeight( passWordLE->sizeHint().height() );
    m_pMainGrid->addWidget(passWordLE, startRow+4, 1, 1, 1);

    viewPwdB = new QPushButton(this);
    QPixmap pixmap(":eye3.png.piko");
    QIcon ButtonIcon(pixmap);
    viewPwdB->setIcon(ButtonIcon);
    viewPwdB->setIconSize(QSize(16,16));
    m_pMainGrid->addWidget(viewPwdB, startRow+4, 2);
    connect( viewPwdB, SIGNAL(clicked()), SLOT(togglePwdClicked()) );

    //CheckBox widget
    QHBoxLayout *chckBxLayout = new QHBoxLayout;
    QWidget * chckBxWidget = new QWidget();
    chckBxWidget->setLayout(chckBxLayout);
    /*
    tmpQLabel = new QLabel("Set as default", this);
    tmpQLabel->setFixedHeight( tmpQLabel->sizeHint().height() );
    m_pMainGrid->addWidget(tmpQLabel, startRow+6, 0);
    */
    setDefaultConnChkBx = new QCheckBox("Set as default connection", this);
    hideSysDbsChkBx = new QCheckBox("Hide system databases", this);
    hideSysDbsChkBx->setChecked(true);
    m_pMainGrid->addWidget(setDefaultConnChkBx, startRow+6, 1, 1, 2);
    chckBxLayout->addWidget(setDefaultConnChkBx);
    chckBxLayout->addWidget(hideSysDbsChkBx);
    m_pMainGrid->addWidget(chckBxWidget, startRow+6, 0, 1, 2);
    connect( hideSysDbsChkBx, SIGNAL(clicked()), SLOT(toggleSysDbs()) );




//    portLE = new QLineEdit();
//    portLE->setText( "" );
//    portLE->setFixedHeight( portLE->sizeHint().height() );
//    m_pMainGrid->addWidget(portLE, startRow+5, 1, 1, 2);


    //Button widget
    QHBoxLayout *buttonLayout = new QHBoxLayout;
    QWidget * buttonWdiget = new QWidget();
    buttonWdiget->setLayout(buttonLayout);
	okB = new QPushButton();
	connect( okB, SIGNAL(clicked()), SLOT(okClicked()) );
	okB->setText( "OK" );
    okB->setAutoRepeat( false );
    okB->setDefault(true);
	okB->setFixedHeight( okB->sizeHint().height() );
    buttonLayout->addWidget(okB);

	cancelB = new QPushButton();
	connect( cancelB, SIGNAL(clicked()), SLOT(cancelClicked()) );
	cancelB->setText( "Cancel" );
    cancelB->setAutoRepeat( false );
	cancelB->setFixedHeight( cancelB->sizeHint().height() );
    buttonLayout->addWidget(cancelB);

    helpB = new QPushButton();
    connect( helpB, SIGNAL(clicked()), SLOT(helpClicked() ) );
    helpB->setText( "New/Edit" );
    helpB->setAutoRepeat( false );
    helpB->setFixedHeight( helpB->sizeHint().height() );
    buttonLayout->addWidget(helpB);
    m_pMainGrid->addWidget(buttonWdiget, startRow+7, 0, 1, 3);


    runPluginCheck();
    createPluginInfo();


    deb("Creating m_pConnSet");
    m_pConnSet = new ConnSet(m_pGDeb);
    m_pConnSet->initConnSet();
    m_pIDSQL = NULL;
    initBox();
    resize( 270, 180 );
    QWidget * host = parent; //this->parentWidget();
    QRect hostRect = host->geometry();
    //this->move(hostRect.center() - this->rect().center());
}

LoginBox::~LoginBox()
{
    dbName = _NO_DB;
}

void LoginBox::runPluginCheck()
{
    QSettings settings(_CFG_DIR, "pmf6");
    GString pluginPath = "./plugins/";
    pluginPath = QCoreApplication::applicationDirPath()+"/plugins/";
    if( checkPlugins(pluginPath) == 0 )
    {
        DSQLPlugin::setPluginPath(pluginPath);
        DBAPIPlugin::setPluginPath(pluginPath);
        settings.setValue("PluginPath", QString((char*)DSQLPlugin::pluginPath()));
        return;
    }

    if( settings.value("PluginPath", -1).toInt() >= 0 )
    {
        pluginPath = settings.value("PluginPath").toString();
    }
    while( checkPlugins(pluginPath) )
    {
        GString msg = "No plugins found in "+pluginPath+",\nplease navigate to the directory where the plugins are located.";
        #if defined(MAKE_VC) || defined (__MINGW32__)
        msg += "\n\nValid plugins are db2dsql.dll, odbcdsql.dll, db2dcli.dll, ...";
        #else
        msg += "\nValid plugins are:\n\n libdb2dsql.so\n libodbcdsql.so\n libdb2dcli.so, ...\n";
        #endif
        QMessageBox::information(this, "Missing plugins", msg);
        QString path = QFileDialog::getExistingDirectory (this, "Plugin path");
        if ( path.isNull()  ) return;
        pluginPath = path;        
    }
    settings.setValue("PluginPath", QString((char*)pluginPath));
}

void LoginBox::initBox()
{
    disconnect(userNameCB, SIGNAL(activated(int)));
    disconnect(dbNameCB, SIGNAL(activated(int)));
    disconnect(hostNameCB, SIGNAL(activated(int)));
    disconnect(dbTypeCB, SIGNAL(activated(int)));
    hostNameCB->clear();
    dbNameCB->clear();

    if(dbTypeCB->count() > 1 )
    {        
        if( GString(dbTypeCB->itemText(0)) != _selStringCB) dbTypeCB->insertItem(0, _selStringCB);
        dbTypeCB->setCurrentIndex(0);
    }
    else if(dbTypeCB->count() == 1 )
    {
        deb("Only one dbType");
        getAllHosts();
        dbTypeCB->setCurrentIndex(0);
        dbNameCB->setFocus();
    }
    dbTypeCB->setFocus();

    connect(userNameCB, SIGNAL(activated(int)), SLOT(setUidAndPwd(int)));
    connect(dbNameCB, SIGNAL(activated(int)), SLOT(getConnDataSlot(int)));
    connect(hostNameCB, SIGNAL(activated(int)), SLOT(getAllDatabases(int)));
    connect(dbTypeCB, SIGNAL(activated(int)), SLOT(getAllHosts()));
    setDefaultCon();
}

void LoginBox::createPluginInfo()
{
    deb("createPluginInfo start");
    GSeq <PLUGIN_DATA*> list;
    GString dbType;
    int rc;
    QSettings settings(_CFG_DIR, "pmf6");

    DSQLPlugin::PluginNames(&list);


    pInfoLBL = new QLabel("Binding....please wait...", this);
    pInfoLBL->setStyleSheet("color: red;");
    pInfoLBL->setFrameStyle( QFrame::Panel | QFrame::Sunken );
    pInfoLBL->setAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
    m_pMainGrid->addWidget(pInfoLBL, m_pMainGrid->rowCount(), 0, 1, 3);
    pInfoLBL->setHidden(true);


    int lastCheckedVer = settings.value("LastCheckedVer", -1).toInt() ;
    if( lastCheckedVer >= 0 && lastCheckedVer > GString(PMF_VER).asInt())
    {
        QLabel * updLBL = new QLabel(this);
        QPalette pal = updLBL->palette();
        pal.setColor(QPalette::Window, QColor(250, 247, 212));

        updLBL->setAutoFillBackground(true);
        updLBL->setPalette(pal);
        updLBL->setTextFormat(Qt::RichText);

        //updLBL->setStyleSheet("QLabel { background-color : yellow;}");
        updLBL->setFrameStyle( QFrame::Panel | QFrame::Sunken );
        updLBL->setAlignment(Qt::AlignVCenter | Qt::AlignHCenter);

        updLBL->setText("Update available at <a href='http://leipelt.org/downld.html'>www.leipelt.org</a>");
        updLBL->setOpenExternalLinks(true);
        m_pMainGrid->addWidget(updLBL, m_pMainGrid->rowCount()+1, 0, 1, 3);
    }


    GString pluginPath = QCoreApplication::applicationDirPath() +"/plugins/";
    if( settings.value("PluginPath", -1).toInt() >= 0 )
    {
        pluginPath = settings.value("PluginPath").toString();
    }

    DSQLPlugin::setPluginPath(pluginPath);

    deb("createPluginInfo listCount: "+GString(list.numberOfElements()));
    for( int i = 1; i <= (int)list.numberOfElements(); ++i)
    {
        QLabel *pLBL;
        dbType = list.elementAtPosition(i)->Type;
        DSQLPlugin dsql(dbType);

        rc = dsql.loadError();

        deb("Loading dbType "+dbType+", rc: "+GString(rc));
        if( rc == PluginLoaded )
        {
            dbTypeCB->addItem(dbType);
            pLBL = new QLabel(dbType+": Plugin OK.", this);
        }
        else if( dbType == _PGSQLCLI ) continue;
        else if( rc == PluginMissing ) pLBL = new QLabel(dbType+": Plugin missing.", this);
        else pLBL = new QLabel(dbType+": Plugin not loadable, click New/Edit", this);

        pLBL->setTextFormat(Qt::RichText);
        pLBL->setFrameStyle( QFrame::Panel | QFrame::Sunken );
        pLBL->setAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
        //pLBL->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
        //Place info at top or bottom of GUI
        m_pMainGrid->addWidget(pLBL, m_pMainGrid->rowCount()+1, 0, 1, 3);
        //m_pMainGrid->addWidget(pLBL, i-1, 0, 1, 3);
    }
    PLUGIN_DATA* plg;
    for(int i = 1; i <= (int)list.numberOfElements(); ++i)
    {
        plg = list.elementAtPosition(i);
        delete plg;
    }

}

void LoginBox::toggleSysDbs()
{
    getAllDatabases(0);
}

void LoginBox::togglePwdClicked()
{
    QPixmap pixmap3(":eye3.png.piko");
    QPixmap pixmap2(":eye2.png.piko");

    if( passWordLE->echoMode() == QLineEdit::Password)
    {
        QIcon ButtonIcon(pixmap2);
        viewPwdB->setIcon(ButtonIcon);
        viewPwdB->setIconSize(QSize(16,16));
        passWordLE->setEchoMode(QLineEdit::Normal);
    }
    else
    {
        QIcon ButtonIcon(pixmap3);
        viewPwdB->setIcon(ButtonIcon);
        viewPwdB->setIconSize(QSize(16,16));
        passWordLE->setEchoMode(QLineEdit::Password);
    }
}

void LoginBox::okClicked()
{
    deb("LoginBox, okClicked Start");

    if( GString(dbTypeCB->currentText()) == _selStringCB)
    {        
        dbTypeCB->setFocus();
        return;
    }
    GString pwdCmd;
    m_pIDSQL = new DSQLPlugin(dbTypeCB->currentText());
    m_pIDSQL->setGDebug(m_pGDeb);
//    if( dbNameCB->currentText() == LGNBOX_CREATE_NODE )
//    {
//        CatalogDB foo(m_pIDSQL, this);
//        foo.createDisplay();
//        foo.exec();
//        if( foo.catalogChanged() ) initBox();
//        return;
//    }
    if( dbNameCB->currentText() == LGNBOX_RUN_STARTUP )
    {
        runAutoCatalog();
        return;
    }

    if( dbNameCB->currentText() == LGNBOX_HELP) return;
//    if( hostNameCB->currentText() == LGNBOX_FIND_DATABASES )
//    {
//        findDatabases();
//        return;
//    }

    if(m_pIDSQL == NULL || !m_pIDSQL->isOK() )
    {
        tm("Could not load plugin for this database, sorry.");
        return;
    }	
    dbType   = dbTypeCB->currentText();
    dbName   = dbNameCB->currentText();
    userName = userNameCB->currentText();
    hostName = getHost(hostNameCB->currentText());
    port     = getPort(hostNameCB->currentText());
    options = "";

    CON_SET * pCS;
    pCS = m_pConnSet->findConnSet(dbType, dbName, hostName, port, userName);
    if( pCS )
    {
        pwdCmd = pCS->PwdCmd;
        options = pCS->Options;
        deb("pwdCmd: "+pwdCmd);
    }
    if( pwdCmd.strip().length() )
    {
        GString err;
        Helper::runCommandInProcess(pwdCmd, passWord, err);
        passWord = passWord.stripTrailing('\r').stripTrailing('\n').stripTrailing('\r');
        deb("from runCommandInProcess: cmd: "+pwdCmd+", err: "+err+", pwd: "+passWord);
        passWord = passWord.stripTrailing('\r').stripTrailing('\n').stripTrailing('\r');
        if( err.length() )
        {
            msg("Cmd "+pwdCmd+" gave error: \n"+err);
            return;
        }
        passWordLE->setText(passWord);
    }
    else passWord = passWordLE->text();
    if( dbName.length() == 0 && GString(dbTypeCB->currentText()) != _MARIADB  ) return;

    m_pIDSQL->disconnect();
    m_pIDSQL->setGDebug(m_pGDeb);

    if( nodeNameHasChanged(dbName, hostName) )
    {
        hostName = getHost(hostNameCB->currentText());
        return;
    }
    int erc;
    if( hostName == LGNBOX_ALL_DATABASES )
    {
        GSeq <CON_SET*> conSetList;
        m_pIDSQL->getDataBases(&conSetList);
        for(int i = 1; i <= conSetList.numberOfElements(); ++i )
        {
            if(conSetList.elementAtPosition(i)->DB == dbName) hostName = conSetList.elementAtPosition(i)->Host;
        }
    }


    erc = bindAndConnect(m_pIDSQL, dbName, userName, passWord, hostName, port, options, pwdCmd);
    if( erc  )
    {
        if( (erc == -1097 ||erc == -1027) && (m_pIDSQL->getDBType() == DB2 || m_pIDSQL->getDBType() == DB2ODBC ) )
        {
            if( QMessageBox::question(this, "PMF", "The NODE "+hostName+" appears to not exist anymore. Do you want to uncatalog this databases?", QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes )
            {
                DBAPIPlugin* pApi = new DBAPIPlugin(m_pIDSQL->getDBTypeName());
                if( pApi->isValid() )
                {
                    pApi->uncatalogDatabase(dbName);
                }
                delete pApi;
                m_pConnSet->removeFromList(_DB2ODBC, dbName);
                m_pConnSet->removeFromList(_DB2, dbName);
                m_pConnSet->save();
                initBox();
            }
        }
        if( (erc == -1013 )  && (m_pIDSQL->getDBType() == DB2 || m_pIDSQL->getDBType() == DB2ODBC ) )
        {
            if( QMessageBox::question(this, "PMF", "Do you want to remove this entry from the list of known databases?", QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes )
            {
                m_pConnSet->removeFromList(_DB2ODBC, dbName);
                m_pConnSet->removeFromList(_DB2, dbName);
                m_pConnSet->save();
                initBox();
            }
        }
        dbName = _NO_DB;
        userName = "";
        passWord = "";
    }
    else if( dbName != LGNBOX_HELP )
    {
        CON_SET * pCS = new CON_SET;
        pCS->init();
        pCS->DefDB = 0;
        pCS->Type = dbTypeCB->currentText();
        pCS->DB = dbName;
        pCS->Host = hostName;
        pCS->UID = userName;
        pCS->Port = port;
        pCS->PWD = passWord;
        pCS->PwdCmd = pwdCmd;
        //pCS->CltEnc = "";

        int rc = m_pConnSet->isInList(pCS);
        if(rc == 0 )
        {
            if( QMessageBox::question(this, "PMF", "Connected. Save connection information?", QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes )
            {
                m_pConnSet->addToList(pCS);                
                deb("LoginBox, okClicked connset saved ok");
//                for(int i = 0; i < dbNameCB->count(); ++i )
//                {
//                    //if( pCS->Type == _POSTGRES && dbNameCB->itemText(i) != LGNBOX_FIND_DATABASES)
//                    m_pConnSet->addToList(dbTypeCB->currentText(), dbNameCB->itemText(i), hostName, userName, passWord, port, "");
//                }
                m_pConnSet->save();

            }
        }
        else if(rc == 2) //Update UID, PWD
        {
            if( QMessageBox::question(this, "PMF", "Connected. Update connection information?", QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes )
            {
                m_pConnSet->updSeq(pCS);
                m_pConnSet->save();
            }
        }
        if( setDefaultConnChkBx->isChecked()) m_pConnSet->setDefaultConnection(pCS);
        m_pConnSet->save();
        deb("LoginBox, deleting connset...");
        deb("LoginBox, deleting closing...");
        close();
    }
    //if( dbName == LGNBOX_FIND_DATABASES ) close();

    GString encoding = SelectEncoding::getEncoding(m_pIDSQL);
#ifdef MAKE_VC
    if( !encoding.length() ) m_pIDSQL->setEncoding("WIN1252");
#endif
    deb("calling setEncoding: "+encoding);
    m_pIDSQL->setEncoding(encoding);
    GKeyVal gkv;
    gkv.readFromFile(lastConnDataSettings());
    gkv.addOrReplace(GString(dbTypeCB->currentText())+GString(hostNameCB->currentText()), dbName);
    gkv.toFile(lastConnDataSettings());
    deb("LoginBox, okClicked end");

}


void LoginBox::keyPressEvent(QKeyEvent *event)
{
    switch(event->key())
    {
        case Qt::Key_Escape:
            cancelClicked();
            break;
			
        case Qt::Key_Return:
            okClicked();
            break;

        default:
        event->accept();
			
    }
}

void LoginBox::closeEvent(QCloseEvent * event)
{
	event->accept();
}
void LoginBox::helpClicked()
{

    /*
    QByteArray systemRoot = qgetenv("SystemRoot");
    if (!systemRoot.isEmpty())
    {
        QString e1 = QString::fromLocal8Bit(systemRoot);
        QMessageBox::information(this, "pmf", e1);
        QProcess * qp = new QProcess(this);
        //QString prog = "%systemroot%\\SysWOW64\\odbcad32.exe";
        QString prog = e1+"\\SysWOW64\\odbcad32.exe";
        qp->start(prog);
        qp->waitForStarted();
        return;
    }
    */
//    OdbcMdf * pOdbcMdf = new OdbcMdf(m_pGDeb, this);
//    pOdbcMdf->show();
//    return;
    NewConn* foo = new NewConn(this, m_pGDeb);
    foo->exec();
    GString selDb = foo->lastSelectedDbType();
    delete foo;
    m_pConnSet->initConnSet();

    int pos = dbTypeCB->findText(selDb);
    if( pos != -1 )
    {
        dbTypeCB->setCurrentIndex(pos);
    }
    else
    {
        dbTypeCB->setCurrentIndex(0);
    }
    getAllHosts();

    dbTypeCB->setFocus();

    return;


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

void LoginBox::cancelClicked()
{
    dbName = _NO_DB;
    hostName = _NO_DB;
    if( m_pIDSQL ) delete m_pIDSQL;
    m_pIDSQL = NULL;
    this->close();
}

void LoginBox::getAllHosts()
{
    deb("getAllDatabases start");
    userNameCB->clear();
    passWordLE->setText("");
    hostNameCB->clear();
    dbNameCB->clear();
    if( GString(dbTypeCB->currentText()) == _selStringCB)
    {
        okB->setEnabled(false);
        return;
    }
    else okB->setEnabled(true);

    hostNameCB->setEnabled(true);
    hostNameCB->setEditable(true);

    if( GString(dbTypeCB->currentText()) == _DB2)
    {
        dbNameCB->setEditable(false);
    }
    else
    {
        dbNameCB->setEditable(true);
    }
    deb("getAllDatabases, calling fillDbNameCB...");
    fillHostNameCB(dbTypeCB->currentText());

}

void LoginBox::getAllDatabases(int pos)
{
    PMF_UNUSED(pos);
    deb("getAllDatabases start");
    userNameCB->clear();
    passWordLE->setText("");
    dbNameCB->clear();

    if( GString(dbTypeCB->currentText()) == _selStringCB)
    {
        okB->setEnabled(false);
        return;
    }
    else okB->setEnabled(true);

    hostNameCB->setEnabled(true);

    dbNameCB->clear();
    if( GString(dbTypeCB->currentText()) == _DB2)
    {
        dbNameCB->setEditable(false);
    }
    else
    {
		dbNameCB->setEditable(true);
    }
    deb("getAllDatabases, calling fillDbNameCB...");
    fillDbNameCB(hostNameCB->currentText());
}

void LoginBox::addDbNameToComboBox(GString dbName)
{
    if( !hideSysDbsChkBx->isChecked() )
    {
        dbNameCB->addItem(dbName);
        return;
    }
    GString dbType = dbTypeCB->currentText();
    if( dbType == _MARIADB )
    {
        if( dbName == "performance_schema" || dbName == "sys" ) return;
    }
    else if( dbType == _POSTGRES )
    {
        if( dbName == "template0" || dbName == "template1" )
        {
            return;
        }
    }
    dbNameCB->addItem(dbName);
}

void LoginBox::fillDbNameCB(GString host)
{
    deb("fillDbNameCB, start");
    dbNameCB->clear();
    GSeq <GString>sortSeq;
    GSeq <CON_SET*> seqConSet;
    m_pConnSet->getStoredCons(dbTypeCB->currentText(), &seqConSet);

    GString hostName = getHost(host);
    GString port = getPort(host);
    CON_SET * pCS;
    for( unsigned long i = 1; i <= seqConSet.numberOfElements(); ++i )
    {
        deb("In seq, host: "+seqConSet.elementAtPosition(i)->Host+", db: "+seqConSet.elementAtPosition(i)->DB+", port: "+seqConSet.elementAtPosition(i)->Port);
        pCS = seqConSet.elementAtPosition(i);
        if( (pCS->Host == hostName && pCS->Port == port) || host == LGNBOX_ALL_DATABASES)
        {
            if( !sortSeq.contains(pCS->DB)) sortSeq.add(pCS->DB);
            deb("in seq: "+pCS->DB);
        }
    }
    sortSeq.sort();
    deb("fillDbNameCB, sorting done");
    for( unsigned long i = 1; i <= sortSeq.numberOfElements(); ++i )
    {        
        addDbNameToComboBox(sortSeq.elementAtPosition(i));
        //dbNameCB->addItem(sortSeq.elementAtPosition(i));

    }
    if( seqConSet.numberOfElements() == 0 )
    {
        if( GString(dbTypeCB->currentText()) == _MARIADB )
        {
            dbNameCB->addItem(LGNBOX_MARIADB);
        }
        else if( GString(dbTypeCB->currentText()) == _POSTGRES )
        {            
            dbNameCB->clear();
            dbNameCB->addItem("");
        }
        else dbNameCB->addItem(LGNBOX_HELP);
    }
    if( GString(dbTypeCB->currentText()) == _DB2 )
    {
        DBAPIPlugin* pApi = new DBAPIPlugin(dbTypeCB->currentText());
        if( pApi->isValid() )
        {
            //dbNameCB->addItem(LGNBOX_CREATE_NODE);
            if( haveStartupFile() ) dbNameCB->addItem(LGNBOX_RUN_STARTUP);
        }
        delete pApi;
    }
    deb("fillDbNameCB, calling getConnData(0)");
    GKeyVal gkv;
    gkv.readFromFile(lastConnDataSettings());
    GString lastDB = gkv.getValForKey(GString(dbTypeCB->currentText())+GString(hostNameCB->currentText()));
    int pos = dbNameCB->findText(lastDB);
    if( pos != -1 && lastDB.length() )
    {
        dbNameCB->setCurrentIndex(pos);
    }
    getConnDataSlot(0);
}

void LoginBox::fillHostNameCB(GString dbType)
{
    deb("fillDbNameCB, start");
    dbNameCB->clear();
    GSeq <GString>sortSeq;
    GSeq <CON_SET*> seqConSet;
    deb("fillDbNameCB, getting stored conns for type "+dbType);
    m_pConnSet->getStoredCons(dbType, &seqConSet);

    for( unsigned long i = 1; i <= seqConSet.numberOfElements(); ++i )
    {
        GString host = seqConSet.elementAtPosition(i)->Host;
        GString port = seqConSet.elementAtPosition(i)->Port;
        if( host.strip().length() == 0 ) host = _HOST_DEFAULT;
        if( !sortSeq.contains(host+":"+port)) sortSeq.add(host+":"+port);
    }
    sortSeq.sort();
    deb("fillDbNameCB, sorting done");
    if( dbType == _DB2 || dbType == _DB2ODBC ) hostNameCB->addItem(LGNBOX_ALL_DATABASES);
    for( unsigned long i = 1; i <= sortSeq.numberOfElements(); ++i )
    {
        hostNameCB->addItem(sortSeq.elementAtPosition(i));
    }
    if( dbType == _DB2)
    {
        hostNameCB->setCurrentIndex(0);
    }
//    if( GString(dbTypeCB->currentText()) == _POSTGRES || GString(dbTypeCB->currentText()) == _MARIADB )
//    {
//        hostNameCB->addItem(LGNBOX_FIND_DATABASES);
//    }
    getAllDatabases(0);

}

int LoginBox::nodeNameHasChanged(GString alias, GString hostName)
{
    if( GString(dbTypeCB->currentText()) != _DB2) return 0;
    if( hostName == LGNBOX_ALL_DATABASES ) return 0;
    GSeq <CON_SET*> csSeq;
    int rc = m_pIDSQL->getDataBases(&csSeq);
    if( rc )
    {
        msg("Could not determine databases, SqlCode: "+GString(m_pIDSQL->sqlCode())+", ErrorText: "+m_pIDSQL->sqlError());
        return 1;
    }
    CON_SET* pCS;
    for( int i = 1; i <= (int) csSeq.numberOfElements(); ++i )
    {
        pCS = csSeq.elementAtPosition(i);
        if( pCS->DB == alias && pCS->Host != hostName && hostName.length() > 0 )
        {
            if( QMessageBox::question(this, "PMF", "The node for this database appears to have changed to '"+pCS->Host+"'\nSet this node?", QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes )
            {
                hostNameCB->addItem(pCS->Host);
                userNameCB->clear();
                passWordLE->setText("");
                userNameCB->setFocus();
                return 1;
            }
            break;
        }
    }
    return 0;
}


void LoginBox::setDefaultCon()
{
    deb("setDefaultCon");
    int pos;
    if( m_pConnSet->errorMessages().length() ) msg(m_pConnSet->errorMessages()+"\n\nCheck your client configuration, maybe try connecting in a shell.\nOr run PMF as Admin");

    getConnDataSlot(0);
    getAllDatabases(0);

    CON_SET* pCS = m_pConnSet->getDefaultCon();
    deb("setDefaultCon, checking pCS for NULL");
    if( pCS == NULL ) return;

    deb("setDefaultCon, pCS OK. Type: "+pCS->Type);
    if( pCS->Type.length() )
    {
        pos = dbTypeCB->findText(pCS->Type);
        deb("DBType "+pCS->Type+", found at pos "+GString(pos));
        if( pos < 0 ) pos = 0;
        dbTypeCB->setCurrentIndex(pos);
        getAllHosts();
    }
    if( pCS->Host.length() )
    {
        pos = hostNameCB->findText(pCS->Host+":"+pCS->Port);
        if( pos < 0 ) pos = 0;
        hostNameCB->setCurrentIndex(pos);
        getAllDatabases(0);
    }
    if( pCS->DB.length() )
    {
        pos = dbNameCB->findText(pCS->DB);
        deb("DBName "+pCS->DB+", found at pos "+GString(pos));
        if( pos < 0 ) pos = 0;
        dbNameCB->setCurrentIndex(pos);
    }
    if( pCS->UID.length() )
    {
        pos = userNameCB->findText(pCS->UID);
        if( pos < 0 ) userNameCB->clear();
        else userNameCB->setCurrentIndex(pos);
    }
    else userNameCB->clear();
    if( pCS->PwdCmd.length())
    {
        GString txt = pCS->PWD;
        txt += " ["+pCS->PwdCmd+"]";
        passWordLE->setText(txt);
    }
    else passWordLE->setText(pCS->PWD);
    passWordLE->setText(pCS->PWD);



    //pos = hostNameCB->findText(pCS->Host);
    //hostNameCB->setText(pCS->Host);
    //if( pos < 0 ) pos = 0;
    //hostNameCB->setCurrentIndex(pos);

    //portLE->setText(GString(pCS->Port));
    dbTypeCB->blockSignals(false);
    dbNameCB->blockSignals(false);
    hostNameCB->blockSignals(false);

}

void LoginBox::findDatabases()
{
    GString host = getHost(hostNameCB->currentText());

    //if( host != LGNBOX_FIND_DATABASES ) return;

    GSeq <CON_SET*> conSetList;
    AddDatabaseHost * pAddHost = new AddDatabaseHost(m_pIDSQL, &conSetList, this);
    pAddHost->CreateDisplay();
    pAddHost->exec();
    delete pAddHost;
    if( conSetList.numberOfElements() == 0 ) return;


    //hostNameCB->clear();
    deb("getConnData, getting databases...");
    deb("getConnData, getting databases, found "+GString(conSetList.numberOfElements())+" databases.");    
    CON_SET* pCS = conSetList.elementAtPosition(1);
    hostNameCB->addItem(pCS->Host+":"+pCS->Port);
    userNameCB->findText(pCS->UID);
    if( pCS->PwdCmd.length() )passWordLE->setText(pCS->PwdCmd);
    else passWordLE->setText(pCS->PWD);
    passWordLE->setText(pCS->PWD);


     //Helper::runStuffInProcess("C:\\Client_Tools\\Powershell\\pwsh.exe -File \"C:\\Client_Tools\\tools\\ad_connect.ps1\"");
    //hostNameCB->addItem(LGNBOX_FIND_DATABASES);

    int saveCon = 0;
    if( QMessageBox::question(this, "PMF", "Found "+GString(conSetList.numberOfElements())+" databases. Save connection information?", QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes )saveCon = 1;
    for( int i = 1; i <= conSetList.numberOfElements(); ++i )
    {
        GString db = conSetList.elementAtPosition(i)->DB.strip("\'");
        dbNameCB->addItem(db);
        if( saveCon)
        {
            pCS = conSetList.elementAtPosition(i);
            m_pConnSet->addToList(dbTypeCB->currentText(), pCS->DB.strip("\'"), pCS->Host, pCS->UID, pCS->PWD, pCS->Port, pCS->PwdCmd, "", "");
        }
    }    
    m_pConnSet->save();
    int pos = hostNameCB->findText(pCS->Host+":"+pCS->Port);
    if( pos != -1 )hostNameCB->setCurrentIndex(pos);
    dbNameCB->setCurrentIndex(0);
    dbNameCB->setFocus();


    return;
}

void LoginBox::setUidAndPwd(int pos)
{
    deb("getConnData, start");
    PMF_UNUSED(pos);

    passWordLE->setText("");
    GString dbName = dbNameCB->currentText();
    GString uid = userNameCB->currentText();
    GSeq <CON_SET*> seqConSet;
    CON_SET * pCS;
    m_pConnSet->getStoredCons(dbTypeCB->currentText(), &seqConSet);
    for( int i = 1; i <= seqConSet.numberOfElements(); ++i )
    {
        pCS = seqConSet.elementAtPosition(i);
        if( pCS->DB == dbName && pCS->UID == uid)
        {
            if( pCS->PwdCmd.length() )passWordLE->setText(pCS->PwdCmd);
            else passWordLE->setText(pCS->PWD);
            passWordLE->setText(pCS->PWD);
        }
    }
}
void LoginBox::getConnDataSlot(int pos)
{
    deb("getConnData, start");
    PMF_UNUSED(pos);

    userNameCB->clear();
    GString dbName = dbNameCB->currentText();

    GSeq <CON_SET*> seqConSet;
    CON_SET * pCS;
    m_pConnSet->getStoredCons(dbTypeCB->currentText(), &seqConSet);
    for( int i = 1; i <= seqConSet.numberOfElements(); ++i )
    {
        pCS = seqConSet.elementAtPosition(i);
        if( pCS->DB == dbName )
        {
            userNameCB->addItem(pCS->UID);
        }
    }
    setUidAndPwd(0);

//    CON_SET * pCS = m_pConnSet->getConSet(dbType, dbName);
//    if( pCS )
//    {
//        //hostNameCB->setText(pCS->Host);
//        //portLE->setText(pCS->Port);
//        userNameCB->setCurrentText(pCS->UID);
//        passWordLE->setText(pCS->PWD);
//    }
}
GString LoginBox::Port()
{
    return port;
}


GString LoginBox::HostName()
{
    return hostName;
}

GString LoginBox::DBType()
{
    return dbType;
}

GString LoginBox::DBName()
{
    if( GString(dbTypeCB->currentText()) == _MARIADB) return hostName;
	return dbName;
}
GString LoginBox::UserName()
{
	return userName;
}

GString LoginBox::PassWord()
{
	return passWord;
}
void LoginBox::deb(GString msg)
{
    m_pGDeb->debugMsg("loginBox", 1, msg);
}
int LoginBox::checkPlugins(GString pluginPath)
{
	pluginPath = pluginPath.stripTrailing("/") + "/";

    GSeq <PLUGIN_DATA*> list;
    DSQLPlugin::PluginNames(&list);
    for( int i = 1; i <= list.numberOfElements(); ++i )
    {
        GString libName = list.elementAtPosition(i)->PluginName;
        #if defined(MAKE_VC) || defined (__MINGW32__)
        if( _access(pluginPath+libName, 0) >= 0 ) return 0;
        #else
        if( access(pluginPath+libName, 0) >= 0 ) return 0;
        #endif
    }
        return 1;
}


int LoginBox::bindAndConnect(DSQLPlugin *pDSQL, GString db, GString uid, GString pwd, GString node, GString port, GString options, GString pwdCmd)
{
    if( db == LGNBOX_MARIADB ) db = "";
    deb("bindAndConnect start.");
    if( !pDSQL ) return 1;

    deb("bindAndConnect db: "+db+", uid: "+uid+", node: "+node+", port: "+port);
    CON_SET cs;
    cs.DB = db;
    cs.UID = uid;
    cs.PWD = pwd;
    cs.Host = node;
    cs.Port = port;
    cs.Options = options;
    cs.PwdCmd = pwdCmd;
    //GString err = pDSQL->connect(db, uid, pwd, node, port);
    GString err = pDSQL->connect(&cs);
    pDSQL->commit();
    if( err.length() )
    {
        if( pwdCmd.length())  msg(err+"\n\nNote: You have set '"+pwdCmd+"' for this connection.\nPlease check this script for errors.");
        else msg(err);
        return pDSQL->sqlCode() == 0 ? -1 : pDSQL->sqlCode();
    }
    deb("bindAndConnect ret on NOT DB...");
    if( pDSQL->getDBType() != DB2 ) return 0;

    deb("bindAndConnect getSysTables...");
    //Now check if BIND is necessary:
    pDSQL->getSysTables();
    if( pDSQL->numberOfRows() > 0 ) return 0;

    pInfoLBL->setHidden(false);
    pInfoLBL->repaint();
    this->repaint();
    //msg("No Tables Found...Maybe BIND will help.\nPLEASE NOTE:\nThis should take only a few seconds. If PMF appears to hang, another instance of PMF is running somewhere.\nStop this instance and restart PMF.");

    //We need to disconnect before rebind:
    pDSQL->disconnect();

    GString bndFile;
    #if defined(MAKE_VC) || defined (__MINGW32__)
        bndFile = DBAPIPlugin::pluginPath()+"PMF"+GString(PMF_VER)+"W.bnd";
        bndFile = bndFile.change("/", "\\");
    #else
         bndFile = DBAPIPlugin::pluginPath()+"PMF"+GString(PMF_VER)+"L.bnd";
    #endif
    deb("loginBox, bndFile is "+bndFile);

    bndFile = checkBindFile(bndFile);
    pInfoLBL->setHidden(true);

    if( !bndFile.length() )
    {
        msg("Cannot bind: Bindfile is missing.");
        return 1;
    }	
    int erc = pDSQL->bindIt(bndFile, db, uid, pwd, Helper::tempPath()+"BND.MSG");
    if( erc  )
    {		
#if defined(MAKE_VC) || defined (__MINGW32__)
		GString hlp = "\nThings to try:\n - Start PMF as Admin (required only once for BINDING)\n";
		hlp += " - Close alle other instances of PMF while binding\n";
		hlp += " - Check TempPath:\n    "+Helper::tempPath()+"\n  TempPath must exist and you need r/w access.\n";
        Helper::tempPath();
        msg("Could not bind, Error: "+GString(erc)+"\n"+pDSQL->sqlError()+hlp);
#else
        msg("Could not bind, Error: "+GString(erc)+"\n"+pDSQL->sqlError());
#endif
        return pDSQL->sqlCode();
    }
    //Check if BIND was successful:
    deb("Connecting ....");
    //err = pDSQL->connect(db, uid, pwd, node, port);
    err = pDSQL->connect(&cs);
    deb("getting SysTables ....");
    pDSQL->getSysTables();
    //if( pDSQL->numberOfRows() > 0 ) msg("Good news: Bind OK, Reconnect Too.");
    if( pDSQL->numberOfRows() == 0 )
    {
        msg("No luck. Possibly wrong version of BIND file (pmf6***.bnd) - OR - insufficient rights: Last errors are\n"+GString(err)+ "\nand\n"+pDSQL->sqlError());
        return 1;
    }
    deb("bindAndConnect done.");
    return 0;
}


/********************************
 *Check or get bndFile
 *******************************/
GString LoginBox::checkBindFile(GString bndFile)
{
    GFile gf(bndFile, GF_READONLY);
    if( gf.initOK() ) return bndFile;
     QString newName = "";
     msg("Could not find file '"+bndFile+"'\nThis file is needed for BINDing PMF to your database.\n"
        "Please enter the file's position in the following dialog.\n\n"
        "OR: bind "+bndFile+" manually ('db2 bind [path...]\\"+bndFile+"')");

     while( newName.indexOf(GStuff::fileFromPath(bndFile)) < 0 )
     {

            newName = QFileDialog::getOpenFileName(this, "Find: "+bndFile, ".");
            if( !newName.length() ) return "";
     }
#if defined(MAKE_VC) || defined (__MINGW32__)
     return GString(newName).translate('/', '\\');
#else
    return  GString(newName);
#endif
}

void LoginBox::msg(GString txt)
{
    QMessageBox::information(this, "pmf", txt);
}

void LoginBox::runAutoCatalog()
{
    QFile file(autoCatalogFilePath()+STARTUP_FILE_NAME);
    file.open(QFile::ReadOnly|QFile::Text);

    QDomDocument dom;
    QString error;
    int line, column;

    if(!dom.setContent(&file, &error, &line, &column))
    {
        msg("pmf_startup.xml: "+GString(error)+", line "+GString(line)+", column: "+GString(column));
        file.close();
        return;
    }
    file.close();

    GXml fXml;
    fXml.readFromFile(autoCatalogFilePath()+STARTUP_FILE_NAME);

    GXml outXml = fXml.getBlocksFromXPath("/Commands/CatalogList/Add/");
    int count = outXml.countBlocks("Add");
    for(int i=1; i <= count; ++i )
    {
        CATALOG_DB cDB;
        cDB.Alias = outXml.getBlockAtPosition("Add", i).getAttribute("dbAlias");
        cDB.Database = outXml.getBlockAtPosition("Add", i).getAttribute("dbName");
        cDB.Host = outXml.getBlockAtPosition("Add", i).getAttribute("nodeHost");
        cDB.NodeName = outXml.getBlockAtPosition("Add", i).getAttribute("nodeName");
        cDB.Port = outXml.getBlockAtPosition("Add", i).getAttribute("nodePort");
        int erc = CatalogDB::catalogDbAndNode(&cDB);
        if( erc ) msg("Error "+GString(erc)+" on catalog dbAlias "+cDB.Alias);
    }

    outXml = fXml.getBlocksFromXPath("/Commands/CatalogList/Remove/");
    count = outXml.countBlocks("Remove");
    for(int i=1; i <= count; ++i )
    {
        CATALOG_DB cDB;
        cDB.Alias = outXml.getBlockAtPosition("Remove", i).getAttribute("dbAlias");
        int erc = CatalogDB::uncatalogDb(&cDB);
        if( erc ) msg("Error "+GString(erc)+" on uncatalog dbAlias "+cDB.Alias);
    }

    msg("Please restart PMF for changes to take effect.");
    return;
}

int LoginBox::haveStartupFile()
{
    if( QDir().exists(autoCatalogFilePath()+STARTUP_FILE_NAME)) return 1;
    return 0;
}

DSQLPlugin * LoginBox::getConnection()
{
    deb("GetConnection:");
    return m_pIDSQL;
}


GString LoginBox::getPort(GString in)
{
    if( in.occurrencesOf(":") == 0 ) return "";
    return in.subString(in.indexOf(":")+1, in.length()).strip();
}

GString LoginBox::getHost(GString in)
{
    if( in.occurrencesOf(":") == 0 ) return in;
    return in.subString(1, in.indexOf(":")-1).strip();

}


