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



AddDatabaseHost::AddDatabaseHost(DSQLPlugin* pDSQL, GSeq <CON_SET*> *conSetList, QWidget *parent )
  :QDialog(parent) //, f("Charter", 48, QFont::Bold)
{
    this->resize(290, 200);
    this->setWindowTitle("Add Host");
    m_pDSQL = pDSQL;
    m_pConSetList = conSetList;

	QBoxLayout *topLayout = new QVBoxLayout(this);
	QGridLayout * grid = new QGridLayout();
    topLayout->addLayout(grid, 9);

	ok = new QPushButton(this);
    ok->setDefault(true);

	cancel = new QPushButton(this);
    ok ->setText("OK");
	ok->setFixedHeight( ok->sizeHint().height());
    cancel->setText("Cancel");
	cancel->setFixedHeight( cancel->sizeHint().height());
	connect(ok, SIGNAL(clicked()), SLOT(OKClicked()));
	connect(cancel, SIGNAL(clicked()), SLOT(CancelClicked()));

    m_pHostLE = new QLineEdit(this);    
    m_pPortLE = new QLineEdit(this);

    if( m_pDSQL->getDBTypeName() == _POSTGRES ) m_pPortLE->setText("5432");
    else if( m_pDSQL->getDBTypeName() == _MARIADB ) m_pPortLE->setText("3306");

    m_pUidLE = new QLineEdit(this);
    m_pPwdLE = new QLineEdit(this);
    m_pPwdLE->setEchoMode(QLineEdit::Password);

    grid->addWidget(new QLabel("Host: ", this), 0, 0);
    grid->addWidget(m_pHostLE, 0, 1);

    grid->addWidget(new QLabel("Port: ", this), 1, 0);
    grid->addWidget(m_pPortLE, 1, 1);

    grid->addWidget(new QLabel("User: ", this), 2, 0);
    grid->addWidget(m_pUidLE, 2, 1);

    grid->addWidget(new QLabel("Password: ", this), 3, 0);
    grid->addWidget(m_pPwdLE, 3, 1);

    m_excludeCkB = new QCheckBox("Exclude 'template' databases");
    //grid->addWidget(m_excludeCkB, 4, 1);


    QHBoxLayout *buttonLayout = new QHBoxLayout;
    QWidget * buttonWdiget = new QWidget();
    buttonWdiget->setLayout(buttonLayout);
    buttonLayout->addWidget(ok);
    buttonLayout->addWidget(cancel);

    grid->addWidget(buttonWdiget, 5, 0, 1, 3);

}
AddDatabaseHost::~AddDatabaseHost()
{
	delete ok;
	delete cancel;
}

void AddDatabaseHost::OKClicked()
{
    GString err = m_pDSQL->connect("", m_pUidLE->text(), m_pPwdLE->text(), m_pHostLE->text(), m_pPortLE->text());
    if( err.length())
    {
        msg(err);
        return;
    }

    m_pDSQL->getDataBases(m_pConSetList);
    if( m_pConSetList->numberOfElements() == 0 )
    {
        msg("Found no databases on this host.");
        return;
    }
    GString defaultPort;
    if( m_pDSQL->getDBTypeName() == _POSTGRES ) defaultPort == "5432";
    else if( m_pDSQL->getDBTypeName() == _MARIADB ) defaultPort == "3306";
    for(int i = 1; i <= m_pConSetList->numberOfElements(); ++i )
    {
        GString port = m_pPortLE->text();
        if(port.strip().length() == 0) port = defaultPort;
        CON_SET *pCS = m_pConSetList->elementAtPosition(i);
        pCS->Host = m_pHostLE->text();
        pCS->Port = m_pPortLE->text();
        pCS->UID  = m_pUidLE->text();
        pCS->PWD  = m_pPwdLE->text();
    }
    close();
}
void AddDatabaseHost::msg(GString message)
{
    QMessageBox::information(this, "PMF", message);
}

void AddDatabaseHost::CancelClicked()
{
    close();
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

