//
//  This file is part of PMF (Poor Man's Flight), see README.PMF for details
//  Author: Gregor Leipelt (gregor@leipelt.de)  (C) G. Leipelt 2000
//

#include "addDatabaseHost.h"

#include <qlayout.h>
//Added by qt3to4:
#include <QVBoxLayout>
#include <QGridLayout>
#include <QMessageBox>
#include <QTextBrowser>
#include "helper.h"
#include "connSet.h"
#include "clickLabel.h"

#if QT_VERSION >= 0x060000
#include <QRegularExpression>
#endif


AddDatabaseHost::AddDatabaseHost(DSQLPlugin* pDSQL, GSeq <CON_SET*> *conSetList, QWidget *parent )
  :QDialog(parent) //, f("Charter", 48, QFont::Bold)
{
    m_pDSQL = pDSQL;
    m_pConSetList = conSetList;
    m_pParent = parent;
}

AddDatabaseHost::~AddDatabaseHost()
{
}

void AddDatabaseHost::CreateDisplay(int mode)
{
    m_iMode = mode;

    this->resize(290, 200);
    this->setWindowTitle("Add Host");
    QBoxLayout *topLayout = new QVBoxLayout(this);
    QGridLayout * grid = new QGridLayout();
    topLayout->addLayout(grid);

    if( m_iMode == ADD_PGSQL_MODE_NORM )
    {
        saveBt = new QPushButton("Save", this);
        saveBt->setDefault(true);
        saveBt->setFixedSize(130, saveBt->sizeHint().height());
        connect(saveBt, SIGNAL(clicked()), SLOT(OKClicked()));
        cancel = new QPushButton("Cancel", this);
        cancel->setFixedHeight( cancel->sizeHint().height());
        connect(cancel, SIGNAL(clicked()), SLOT(CancelClicked()));
    }


    m_pHostLE = new QLineEdit(this);
    m_pPortLE = new QLineEdit(this);

    if( m_pDSQL->getDBTypeName() == _POSTGRES ) m_pPortLE->setText("5432");
    else if( m_pDSQL->getDBTypeName() == _MARIADB ) m_pPortLE->setText("3306");

    m_pUidLE = new QLineEdit(this);
    m_pPwdLE = new QLineEdit(this);
    m_pPwdCmdLE = new QLineEdit(this);
    m_pPwdLE->setEchoMode(QLineEdit::Password);
    m_pReconnectTimeLE = new QLineEdit(this);
    int row = 0;

//    grid->addWidget(new QLabel("You can also set a command to generate a Password", this), row, 0, 1, 3);
//    row++;
    grid->addWidget(new QLabel("Host: ", this), row, 0);
    grid->addWidget(m_pHostLE, row, 1, 1, 4);

    row++;
    grid->addWidget(new QLabel("Port: ", this), row, 0);
    grid->addWidget(m_pPortLE, row, 1, 1, 4);
    row++;
    grid->addWidget(new QLabel("User: ", this), row, 0);
    grid->addWidget(m_pUidLE, row, 1, 1, 4);
    row++;
    grid->addWidget(new QLabel("Password: ", this), row, 0);
    grid->addWidget(m_pPwdLE, row, 1, 1, 3);

    viewPwdB = new QPushButton(this);
    testConnBt = new QPushButton("Test", this);
    connect(testConnBt, SIGNAL(clicked()), SLOT(testConnClicked()));
    testConnBt->hide();
    QPixmap pixmap(":eye3.png.piko");
    QIcon ButtonIcon(pixmap);
    viewPwdB->setIcon(ButtonIcon);
    viewPwdB->setIconSize(QSize(16,16));
    grid->addWidget(viewPwdB, row, 4);
    connect( viewPwdB, SIGNAL(clicked()), SLOT(togglePwdClicked()) );


    if( m_pDSQL->getDBTypeName() == _POSTGRES)
    {
        row++;
        QButtonGroup* selectSSH = new QButtonGroup(this);
        _noSSLRB = new QRadioButton("Disable  (do not use SSL)", this);
        _allowSSLRB = new QRadioButton("Allow (try non-SSL first, fallback to SSL)", this);
        _preferSSLRB = new QRadioButton("Prefer [default] (try SSL first, fallback to non-SSL)", this);
        _requireSSLRB = new QRadioButton("Require (do not connect if SSL fails)", this);
        selectSSH->addButton(_noSSLRB);
        selectSSH->addButton(_allowSSLRB);
        selectSSH->addButton(_preferSSLRB);
        selectSSH->addButton(_requireSSLRB);
        _preferSSLRB->setChecked(true);

        row++;
        grid->addWidget(new QLabel("", this), row, 0);

        row++;
        grid->addWidget(new QLabel("Connection security", this), row, 0, 1, 5);
        row++;

        grid->addWidget(_noSSLRB, row, 1);row++;
        grid->addWidget(_allowSSLRB, row, 1);row++;
        grid->addWidget(_preferSSLRB, row, 1);row++;
        grid->addWidget(_requireSSLRB, row, 1);row++;

        row++;
        grid->addWidget(new QLabel("", this), row, 0);

        //row++;
        /*
        ClickLabel *helpLink = new ClickLabel("Examples");
        connect(helpLink, SIGNAL(clicked()), SLOT(helpClicked()));
        helpLink->setMaximumWidth(70);
        */

        QLabel *helpLink = new QLabel("<a href='http://leipelt.org/pmfhowto.html#pwdcmd'>Examples</a>");
        helpLink->setOpenExternalLinks(true);
        row++;
        grid->addWidget(new QLabel("You can also set a command to generate or fetch a password:", this), row, 0, 1, 2);
        grid->addWidget(helpLink, row, 4, 1, 1);

        row++;
        grid->addWidget(new QLabel("PwdCmd: ", this), row, 0);
        grid->addWidget(m_pPwdCmdLE, row, 1, 1, 3);
        testConnBt->show();
        grid->addWidget(testConnBt, row, 4);
#ifdef MAKE_VC
        m_pPwdCmdLE->setPlaceholderText("[Path to]\some.[EXE, CMD, BAT, PS1,...] myParam1 myParam2 ...");
#else
        m_pPwdCmdLE->setPlaceholderText("[Path to]/myCommand myParam1 myParam2 ...");
#endif
        row++;
        grid->addWidget(new QLabel("Timeout: ", this), row, 0);
        grid->addWidget(m_pReconnectTimeLE, row, 1, 1, 4);
        m_pReconnectTimeLE->setPlaceholderText("Automatic reconnect after n minutes. 0: No reconnect");
        //grid->addWidget(new QLabel("Reconnect after n seconds ", this), row, 2, 1, 1);
    }
    else
    {
        m_pPwdCmdLE->hide();
        m_pReconnectTimeLE->hide();
    }

    m_infoLBL = new QLabel("", this);
    row++;
    grid->addWidget(m_infoLBL, row, 1);


    m_excludeCkB = new QCheckBox("Exclude 'template' databases");
    //grid->addWidget(m_excludeCkB, 4, 1);

    row++;
    QHBoxLayout *buttonLayout = new QHBoxLayout;
    QWidget * buttonWdiget = new QWidget();
    buttonWdiget->setLayout(buttonLayout);    
    if(m_iMode == ADD_PGSQL_MODE_NORM )
    {
        buttonLayout->addWidget(cancel);
        buttonLayout->addWidget(saveBt);
        grid->addWidget(buttonWdiget, row, 1, 1, 1);
    }
    grid->setRowStretch(grid->rowCount(), 1);
}

void AddDatabaseHost::helpClicked()
{
    QDialog * helpVw = new QDialog(this);

    QBoxLayout *browserLayout = new QVBoxLayout(helpVw);
    QGridLayout * grid = new QGridLayout();

    browserLayout->addLayout(grid);

    QTextBrowser* browser = new QTextBrowser(helpVw);
    grid->addWidget(browser, 0, 0);


    helpVw->resize(800,500);
    helpVw->setWindowTitle("PMF Help - ESC to close");
    browser->setSource( QUrl("qrc:///pwdcmd_help.html") );
    helpVw->exec();
}

void AddDatabaseHost::togglePwdClicked()
{
    QPixmap pixmap3(":eye3.png.piko");
    QPixmap pixmap2(":eye2.png.piko");

    if( m_pPwdLE->echoMode() == QLineEdit::Password)
    {
        QIcon ButtonIcon(pixmap2);
        viewPwdB->setIcon(ButtonIcon);
        viewPwdB->setIconSize(QSize(16,16));
        m_pPwdLE->setEchoMode(QLineEdit::Normal);
    }
    else
    {
        QIcon ButtonIcon(pixmap3);
        viewPwdB->setIcon(ButtonIcon);
        viewPwdB->setIconSize(QSize(16,16));
        m_pPwdLE->setEchoMode(QLineEdit::Password);
    }
}


void AddDatabaseHost::OKClicked()
{
    m_infoLBL->setText("Please wait, checking connection...");
    m_infoLBL->repaint();
    GString pwdCmd = m_pPwdCmdLE->text();
    if( pwdCmd.strip().length( ))
    {
		GString res, err;
        Helper::runCommandInProcess(pwdCmd, res, err);
        //printf(" AddDatabaseHost::OKClicked(), cmd: %s, res: %s, err: %s\n", (char*) pwdCmd, (char*) res, (char*) err);
		res = res.stripTrailing('\r').stripTrailing('\n').stripTrailing('\r');
		if( err.length() ) 
		{
            m_infoLBL->setText("");
            m_infoLBL->repaint();
            #if QT_VERSION >= 0x060000
            QRegularExpression re("\\x1B\\[([0-9]{1,2}(;[0-9]{1,2})?)?[m|K]");
            QString qErr = QString((char*)err);
            qErr.replace(re, "");
            msg(GString(qErr));
            #else
            msg(err);
            #endif
			return;
		}
		printf("Result: %s<--\n", (char*) res);
        m_pPwdLE->setText(res);
    }
    GString options;
    if( m_pDSQL->getDBTypeName() == _POSTGRES)
    {
        if( _noSSLRB->isChecked() ) options = _CON_NO_SSL;
        else if( _allowSSLRB->isChecked() ) options = _CON_ALLOW_SSL;
        else if( _preferSSLRB->isChecked() ) options = _CON_PREFER_SSL;
        else if( _requireSSLRB->isChecked() ) options = _CON_REQUIRE_SSL;
    }

	GString dbName = "";
    if( m_pDSQL->getDBTypeName() == _POSTGRES ) dbName = "template1";
    GString connectionError = m_pDSQL->connect(dbName, m_pUidLE->text(), m_pPwdLE->text(), m_pHostLE->text(), m_pPortLE->text());
    if( connectionError.length())
    {
        m_infoLBL->setText("");
        m_infoLBL->repaint();
        //msg(err);
        //return;
    }
    ConnSet * allConnSeq = new ConnSet();
    allConnSeq->initConnSet();

    CON_SET * pCS;
    while( m_pConSetList->numberOfElements() )
    {
        pCS = m_pConSetList->firstElement();
        delete pCS;
        m_pConSetList->removeFirst();
    }
    m_pDSQL->getDataBases(m_pConSetList);
    if( m_pConSetList->numberOfElements() == 0 )
    {
        GString err;
        m_infoLBL->setText("");
        m_infoLBL->repaint();
        if( connectionError.length() ) err = "Found no databases on this host: "+connectionError+"\n\nSave anyway?";
        else err = "Found no databases on this host.\n\nContinue anyway?";
        if( QMessageBox::question(this, "PMF", err, QMessageBox::Yes, QMessageBox::No) == QMessageBox::No ) return;

        allConnSeq->addToList(m_pDSQL->getDBTypeName(), "", m_pHostLE->text(), m_pUidLE->text(), m_pPwdLE->text(), m_pPortLE->text(), m_pPwdCmdLE->text(), options, "");
        allConnSeq->save();
        if( m_iMode == ADD_PGSQL_MODE_NORM ) close();
        return;
    }

    GString defaultPort;
    if( m_pDSQL->getDBTypeName() == _POSTGRES ) defaultPort = "5432";
    else if( m_pDSQL->getDBTypeName() == _MARIADB ) defaultPort = "3306";
    for(int i = 1; i <= m_pConSetList->numberOfElements(); ++i )
    {
        GString port = m_pPortLE->text();
        if(port.strip().length() == 0) port = defaultPort;
        pCS = m_pConSetList->elementAtPosition(i);
        pCS->Host = m_pHostLE->text();
        pCS->Port = port;
        pCS->UID  = m_pUidLE->text();
        pCS->PWD  = m_pPwdLE->text();
        pCS->PwdCmd = m_pPwdCmdLE->text();
    }

    if( m_iMode == ADD_PGSQL_MODE_NORM )
    {        
        close();
        return;
    }
    m_infoLBL->setText("");
    m_infoLBL->repaint();


    msg("Found "+GString(m_pConSetList->numberOfElements())+" databases");
    for( int i = 1; i <= m_pConSetList->numberOfElements(); ++i )
    {
        pCS = m_pConSetList->elementAtPosition(i);
        allConnSeq->addToList(pCS->Type, pCS->DB.strip("\'"), pCS->Host, pCS->UID, pCS->PWD, pCS->Port, pCS->PwdCmd, options, "");
    }
    allConnSeq->save();
}
void AddDatabaseHost::msg(GString message)
{
    QMessageBox::information(this, "PMF", message);
}

void AddDatabaseHost::CancelClicked()
{
    close();
}

void AddDatabaseHost::testConnClicked()
{
    if( m_pDSQL->getDBTypeName() != _POSTGRES) return;
    GString res, cmdErr;
    int erc;

    CON_SET cs;
    cs.Type = m_pDSQL->getDBTypeName();
    cs.DB   = "template1";
    cs.Host = m_pHostLE->text();
    cs.Port = m_pPortLE->text();
    cs.UID  = m_pUidLE->text();
    cs.PWD  = m_pPwdLE->text();
    cs.PwdCmd = m_pPwdCmdLE->text();

    if(cs.Port.strip().length() == 0) cs.Port = "5432";

    GString options;
    if( m_pDSQL->getDBTypeName() == _POSTGRES)
    {
        if( _noSSLRB->isChecked() ) options = _CON_NO_SSL;
        else if( _allowSSLRB->isChecked() ) options = _CON_ALLOW_SSL;
        else if( _preferSSLRB->isChecked() ) options = _CON_PREFER_SSL;
        else if( _requireSSLRB->isChecked() ) options = _CON_REQUIRE_SSL;
    }
    DSQLPlugin * pDSQL = new DSQLPlugin(cs.Type);
    if( !pDSQL )
    {
        msg("Could not load plugin for Type "+cs.Type);
        return;
    }
    if( cs.PwdCmd.length() )
    {
        erc = Helper::runCommandInProcess(cs.PwdCmd, res, cmdErr);
        if( erc )
        {
            msg("Running PwdCmd failed:\n\n"+cmdErr);
            return;
        }
        cs.PWD = res;
    }
    GString err = pDSQL->connect(&cs);
    if( !err.length() )
    {
        msg("Success! Connection established.");
        m_pPwdLE->setText(res);
    }
    else msg("Could not connect: \n"+err);
    delete pDSQL;
}

void AddDatabaseHost::keyPressEvent(QKeyEvent *event)
{
    switch(event->key())
    {
        case Qt::Key_Enter:
        case Qt::Key_Return:
            OKClicked();
            break;
        case Qt::Key_Escape:
            CancelClicked();
            break;
        default:
            QWidget::keyPressEvent(event);
    }
}

