//
//  This file is part of PMF (Poor Man's Flight), see README.PMF for details
//  Author: Gregor Leipelt (gregor@leipelt.de)  (C) G. Leipelt 2000
//

#include "editConn.h"

#include <qlayout.h>
//Added by qt3to4:
#include <QVBoxLayout>
#include <QGridLayout>
#include <QMessageBox>
#include <QColorDialog>
#include "connSet.h"
#include "helper.h"

#if QT_VERSION >= 0x060000
#include <QRegularExpression>
#endif


EditConn::EditConn( CON_SET *conSet, CON_SET *newCS, ConnSet *parent ) :QDialog(parent)
{
    m_pConSet = conSet;
    m_pNewCS  = newCS;
    m_pParent = parent;
    m_iHasChanged = 0;    
}

EditConn::~EditConn()
{
}

void EditConn::CreateDisplay(int locked)
{
    m_iLocked = locked;
    this->resize(490, 350);
    this->setWindowTitle("Edit connection");
    QBoxLayout *topLayout = new QVBoxLayout(this);
    QGridLayout * grid = new QGridLayout();
    topLayout->addLayout(grid);

    viewPwdBt = new QPushButton(this);
    QPixmap pixmap(":eye3.png.piko");
    QIcon ButtonIcon(pixmap);
    viewPwdBt->setIcon(ButtonIcon);
    viewPwdBt->setIconSize(QSize(16,16));


    okBt = new QPushButton("OK", this);
    okBt->setDefault(true);
    okBt->setFixedHeight( okBt->sizeHint().height());
    //saveBt->setFixedSize(130, saveBt->sizeHint().height());
    connect(okBt, SIGNAL(clicked()), SLOT(OKClicked()));
    cancel = new QPushButton("Cancel", this);
    cancel->setFixedHeight( cancel->sizeHint().height());
    connect(cancel, SIGNAL(clicked()), SLOT(cancelClicked()));

    selColorBt = new QPushButton("Choose", this);
    resetColorBt = new QPushButton("Reset", this);
    testConnBt = new QPushButton("Test", this);
    testConnBt->hide();
    connect(selColorBt, SIGNAL(clicked()), SLOT(selectColorClicked()));
    connect(resetColorBt, SIGNAL(clicked()), SLOT(resetColorClicked()));
    connect(testConnBt, SIGNAL(clicked()), SLOT(testConnClicked()));

    m_pTypeCB = new QComboBox(this);
    m_pDbLE = new QLineEdit(this);
    m_pHostLE = new QLineEdit(this);
    m_pPortLE = new QLineEdit(this);
    m_pUidLE = new QLineEdit(this);
    m_pPwdLE = new QLineEdit(this);


    m_pPwdLE->setEchoMode(QLineEdit::Password);

    int row = 0;
    grid->addWidget(new QLabel("Type: ", this), row, 0);
    grid->addWidget(m_pTypeCB, row, 1, 1, 5);

    row++;
    grid->addWidget(new QLabel("Database: ", this), row, 0);
    grid->addWidget(m_pDbLE, row, 1, 1, 5);
    m_pDbLE->setText(m_pConSet->DB);

    row++;
    grid->addWidget(new QLabel("Host: ", this), row, 0);
    grid->addWidget(m_pHostLE, row, 1, 1, 5);
    m_pHostLE->setText(m_pConSet->Host);
    row++;
    grid->addWidget(new QLabel("Port: ", this), row, 0);
    grid->addWidget(m_pPortLE, row, 1, 1, 5);
    m_pPortLE->setText(m_pConSet->Port);
    row++;
    grid->addWidget(new QLabel("User: ", this), row, 0);
    grid->addWidget(m_pUidLE, row, 1, 1, 5);
    m_pUidLE->setText(m_pConSet->UID);
    row++;
    grid->addWidget(new QLabel("Password: ", this), row, 0);
    grid->addWidget(m_pPwdLE, row, 1, 1, 4);
    m_pPwdLE->setText(m_pConSet->PWD);
    grid->addWidget(viewPwdBt, row, 5);
    row++;

    if( m_pConSet->Type == _POSTGRES )
    {
        m_pPwdCmdLE = new QLineEdit(this);
        grid->addWidget(new QLabel("PwdCmd: ", this), row, 0);
        grid->addWidget(m_pPwdCmdLE, row, 1, 1, 4);
        m_pPwdCmdLE->setText(m_pConSet->PwdCmd);
        testConnBt->show();
        grid->addWidget(testConnBt, row, 5);
        row++;
        m_pReconnLE = new QLineEdit(this);
        m_pReconnLE->setPlaceholderText("0 (default): No reconnect");
        grid->addWidget(new QLabel("Timeout: ", this), row, 0);
        grid->addWidget(new QLabel("Reconnect after n minutes", this), row, 2, 1, 4);
        grid->addWidget(m_pReconnLE, row, 1, 1, 1);
        m_pReconnLE->setText(GString(m_pConSet->ReconnTimeout));

        row++;
        grid->addWidget(new QLabel("", this), row, 0, 1, 5);

        row++;
        grid->addWidget(new QLabel("Connection security settings:", this), row, 0, 1, 5);

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
        if( m_pConSet->Options == _CON_NO_SSL ) _noSSLRB->setChecked(true);
        else if( m_pConSet->Options == _CON_ALLOW_SSL ) _allowSSLRB->setChecked(true);
        else if( m_pConSet->Options == _CON_PREFER_SSL ) _preferSSLRB->setChecked(true);
        else if( m_pConSet->Options == _CON_REQUIRE_SSL ) _requireSSLRB->setChecked(true);
        else _preferSSLRB->setChecked(true);
        grid->addWidget(_noSSLRB, row, 1, 1, 5);row++;
        grid->addWidget(_allowSSLRB, row, 1, 1, 5);row++;
        grid->addWidget(_preferSSLRB, row, 1, 1, 5);row++;
        grid->addWidget(_requireSSLRB, row, 1, 1, 5);row++;
        row++;
        grid->addWidget(new QLabel("", this), row, 0, 1, 5);

    }

    row++;
    //grid->addWidget(new QLabel("Optionally, you can set a color for PMF's menu bar:", this), row, 0, 1, 5);
    row++;

    QHBoxLayout *colButtonLayout = new QHBoxLayout;
    QWidget * colButtonWdiget = new QWidget();
    colButtonWdiget->setLayout(colButtonLayout);
    colButtonLayout->addWidget(new QLabel("You can set a color for PMF's menu bar:", this));
    colButtonLayout->addWidget(selColorBt);
    colButtonLayout->addWidget(resetColorBt);
    grid->addWidget(colButtonWdiget, row, 0, 1, 6);
    /*
    grid->addWidget(new QLabel("Color: ", this), row, 0);
    grid->addWidget(selColorBt, row, 1, 1, 1);
    grid->addWidget(resetColorBt, row, 2, 1, 1);
    */

    m_pNewCS->Color = m_pConSet->Color;
    //setColorFromString(m_pConSet->Color);

    m_infoLBL = new QLabel("", this);
    row++;
    grid->addWidget(m_infoLBL, row, 1);
    row++;


    connect( viewPwdBt, SIGNAL(clicked()), SLOT(togglePwdClicked()) );

    QHBoxLayout *buttonLayout = new QHBoxLayout;
    QWidget * buttonWdiget = new QWidget();
    buttonWdiget->setLayout(buttonLayout);
    buttonLayout->addWidget(cancel);
    buttonLayout->addWidget(okBt);
    grid->addWidget(buttonWdiget, row, 0, 1, 6);
    grid->setRowStretch(grid->rowCount(), 1);

    if( m_iLocked )
    {
        m_pTypeCB->setEnabled(false);
        m_pTypeCB->addItem(m_pConSet->Type);
        m_pDbLE->setEnabled(false);
    }
    else
    {
        GSeq <PLUGIN_DATA*> list;
        DSQLPlugin::PluginNames(&list);
        GString dbType;
        for( int i = 1; i <= (int)list.numberOfElements(); ++i)
        {
            dbType = list.elementAtPosition(i)->Type;
            DSQLPlugin dsql(dbType);
            if( dsql.loadError() == PluginLoaded ) m_pTypeCB->addItem(dbType);
        }
        int pos = m_pTypeCB->findText(m_pConSet->Type);
        if( pos >= 0 )m_pTypeCB->setCurrentIndex(pos);
    }

}
int EditConn::hasChanged()
{
    return m_iHasChanged;
}

void EditConn::setNewCsData()
{
    GString options = "";
    if( m_pConSet->Type == _POSTGRES )
    {
        if( _noSSLRB->isChecked() ) options = _CON_NO_SSL;
        else if( _allowSSLRB->isChecked() ) options = _CON_ALLOW_SSL;
        else if( _preferSSLRB->isChecked() ) options = _CON_PREFER_SSL;
        else if( _requireSSLRB->isChecked() ) options = _CON_REQUIRE_SSL;
        m_pNewCS->PwdCmd  = m_pPwdCmdLE->text();
        m_pNewCS->ReconnTimeout = GString(m_pReconnLE->text()).asInt();
    }

    m_pNewCS->Type =  m_pTypeCB->currentText();
    m_pNewCS->DB   = m_pDbLE->text();
    m_pNewCS->Host = m_pHostLE->text();
    m_pNewCS->Port = m_pPortLE->text();
    m_pNewCS->UID  = m_pUidLE->text();
    m_pNewCS->PWD  = m_pPwdLE->text();
    m_pNewCS->Options  = options;
}

void EditConn::OKClicked()
{
    setNewCsData();
    if( m_iLocked )
    {
        m_iHasChanged = 1;
        this->close();
        return;
    }
    if( m_pParent->isInList(m_pNewCS) )
    {
        msg("This connection already exists.");
        return;
    }
    m_iHasChanged = 1;
    this->close();
}




void EditConn::msg(GString message)
{
    QMessageBox::information(this, "PMF", message);
}

void EditConn::cancelClicked()
{
    m_iHasChanged = 0;
    this->close();
}

void EditConn::setColorFromString(GString rgbColor)
{

    QString col = "background-color: ";
    col += (char*) rgbColor;
    selColorBt->setStyleSheet(col);
    GString txt = selColorBt->palette().color(QPalette::Button).name();
    return;

    QColor color = QColor((char*)rgbColor);
    if( !color.isValid() ) return;
    setColor(color);
}

void EditConn::setColor(QColor color)
{    
    if( !color.isValid() ) return;
    QPalette pal = selColorBt->palette();
    pal.setColor(QPalette::Button, color);
    selColorBt->setAutoFillBackground(true);
    selColorBt->setPalette(pal);
    selColorBt->update();
}

void EditConn::resetColorClicked()
{
    //setColorFromString("");
    m_pNewCS->Color = "";
    msg("Color has been reset");
    m_iHasChanged;
}

void EditConn::testConnClicked()
{
    GString res, cmdErr;
    int erc;
    setNewCsData();

    DSQLPlugin * pDSQL = new DSQLPlugin(m_pNewCS->Type);
    if( !pDSQL )
    {
        msg("Could not load plugin for Type "+m_pNewCS->Type);
        return;
    }
    if( m_pNewCS->PwdCmd.length() )
    {
        erc = Helper::runCommandInProcess(m_pNewCS->PwdCmd, res, cmdErr);
        if( erc )
        {
            msg("Running PwdCmd failed:\n\n"+cmdErr);
            return;
        }
        m_pNewCS->PWD = res;
    }
    GString err = pDSQL->connect(m_pNewCS);
    if( !err.length() )
    {
        msg("Success! Connection established.");
        m_pPwdLE->setText(res);
    }
    else msg("Could not connect: \n"+err);
    delete pDSQL;
}

void EditConn::selectColorClicked()
{
    QColor res = QColorDialog::getColor();
    if( !res.isValid() ) return;
    m_pNewCS->Color = res.name();
    //setColor(res);
}

void EditConn::keyPressEvent(QKeyEvent *event)
{
    switch(event->key())
    {
        case Qt::Key_Enter:
        case Qt::Key_Return:
            OKClicked();
            break;
        case Qt::Key_Escape:
            cancelClicked();
            break;
    }
}

void EditConn::togglePwdClicked()
{
    QPixmap pixmap3(":eye3.png.piko");
    QPixmap pixmap2(":eye2.png.piko");

    if( m_pPwdLE->echoMode() == QLineEdit::Password)
    {
        QIcon ButtonIcon(pixmap2);
        viewPwdBt->setIcon(ButtonIcon);
        viewPwdBt->setIconSize(QSize(16,16));
        m_pPwdLE->setEchoMode(QLineEdit::Normal);
    }
    else
    {
        QIcon ButtonIcon(pixmap3);
        viewPwdBt->setIcon(ButtonIcon);
        viewPwdBt->setIconSize(QSize(16,16));
        m_pPwdLE->setEchoMode(QLineEdit::Password);
    }
}

