//
//  This file is part of PMF (Poor Man's Flight), see README.PMF for details
//  Author: Gregor Leipelt (gregor@leipelt.de)  (C) G. Leipelt 2000
//

#include "connectionInfo.h"
#include "pmfdefines.h"
#include "gfile.hpp"
#include "helper.h"
#include <qlayout.h>
//Added by qt3to4:
#include <QVBoxLayout>
#include <QGridLayout>
#include <QMessageBox>



ConnectionInfo::ConnectionInfo(DSQLPlugin* pDSQL, QWidget *parent )
  :QDialog(parent) //, f("Charter", 48, QFont::Bold)
{
    this->resize(250, 200);
    this->setWindowTitle("Connection info");

    m_pDSQL = pDSQL;

	QBoxLayout *topLayout = new QVBoxLayout(this);
	QGridLayout * grid = new QGridLayout();
    topLayout->addLayout(grid, 9);

	ok = new QPushButton(this);
    ok->setDefault(true);
    ok->setText("OK");

	ok->setFixedHeight( ok->sizeHint().height());
	connect(ok, SIGNAL(clicked()), SLOT(OKClicked()));

    CON_SET curSet;
    pDSQL->currentConnectionValues(&curSet);

    QLineEdit * pTypeLE, *pDBLE, *pHostLE, *pPortLE, *pUserLE;
    pTypeLE = new QLineEdit(curSet.Type, this);
    pTypeLE->setReadOnly(true);
//    pTypeLE->setDisabled(true);
    pDBLE = new QLineEdit(curSet.DB, this);
    pDBLE->setReadOnly(true);
    //pDBLE->setDisabled(true);
    pHostLE = new QLineEdit(curSet.Host, this);
    pHostLE->setReadOnly(true);
    //pHostLE->setDisabled(true);
    pPortLE = new QLineEdit(curSet.Port, this);
    pPortLE->setReadOnly(true);
    //pPortLE->setDisabled(true);
    pUserLE = new QLineEdit(curSet.UID, this);
    pUserLE->setReadOnly(true);
    //pUserLE->setDisabled(true);


    grid->addWidget(new QLabel("Type: ", this), 0, 0);
    grid->addWidget(pTypeLE, 0, 1, 1, 2);

    grid->addWidget(new QLabel("DB: ", this), 1, 0);
    grid->addWidget(pDBLE, 1, 1, 1, 2);

    grid->addWidget(new QLabel("Host: ", this), 2, 0);
    grid->addWidget(pHostLE, 2, 1, 1, 2);

    grid->addWidget(new QLabel("Port: ", this), 3, 0);
    grid->addWidget(pPortLE, 3, 1, 1, 2);

    grid->addWidget(new QLabel("User: ", this), 4, 0);
    grid->addWidget(pUserLE, 4, 1, 1, 2);


    QHBoxLayout *buttonLayout = new QHBoxLayout;
    QWidget * buttonWdiget = new QWidget();
    buttonWdiget->setLayout(buttonLayout);
    buttonLayout->addWidget(ok);

    grid->addWidget(buttonWdiget, 5, 1, 1, 1);

}


ConnectionInfo::~ConnectionInfo()
{
	delete ok;
}

void ConnectionInfo::OKClicked()
{
    close();
}

