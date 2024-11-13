//
//  This file is part of PMF (Poor Man's Flight), see README.PMF for details
//  Author: Gregor Leipelt (gregor@leipelt.de)  (C) G. Leipelt 2000
//

#include "editBmDetail.h"
#include "gstuff.hpp"
#include "pmfdefines.h"
#include <qlayout.h>
#include <qfont.h>
//Added by qt3to4:
#include <QGridLayout>
#include <QVBoxLayout>
#include <QSettings>
#include <QGroupBox>


EditBmDetail::EditBmDetail(QString name, QWidget *parent )
  :QDialog(parent) //, f("Charter", 48, QFont::Bold)
{

  this->resize(460, 400);
  QBoxLayout *topLayout = new QVBoxLayout(this);

  QGridLayout * grid = new QGridLayout();
  QGroupBox * buttonGroupBox = new QGroupBox();
  topLayout->addLayout(grid, 2);
 
  m_gstrSettingsName = name;
  nameLE = new QLineEdit(this);
  tableLE = new QLineEdit(this);
  tableLE->setPlaceholderText("[Optional]");

  ok = new QPushButton(this);
  ok->setText("OK");
  ok->setDefault(true);
  ok->setText("OK");
  ok->setGeometry(20, 360, 80, 30);
  connect(ok, SIGNAL(clicked()), SLOT(OKClicked()));

  cancel = new QPushButton(this);
  cancel->setDefault(true);
  cancel->setText("Cancel");
  cancel->setGeometry(20, 360, 80, 30);
  connect(cancel, SIGNAL(clicked()), SLOT(CancelClicked()));

  okClicked = 0;

  infoTE = new TxtEdit(NULL, this);
  //infoTE->setGeometry( 20, 20, 420, 300);

  grid->addWidget(new QLabel("Name: "), 0, 0 );
  grid->addWidget(nameLE, 0, 1);
  grid->addWidget(new QLabel("Table: "), 0, 2 );
  grid->addWidget(tableLE, 0, 3);

  grid->addWidget(infoTE, 2, 0, 1, 4);

  QHBoxLayout *typeLayout = new QHBoxLayout;
  typeLayout ->addWidget(ok);
  typeLayout ->addWidget(cancel);
  buttonGroupBox->setLayout(typeLayout);
  grid->addWidget(buttonGroupBox, 3, 0, 1, 2);

  QSettings settings(_CFG_DIR, "pmf6");

  QByteArray ba = settings.value(m_gstrSettingsName+"Geometry").toByteArray();
  this->restoreGeometry(settings.value(m_gstrSettingsName+"Geometry").toByteArray());
}
EditBmDetail::~EditBmDetail()
{
	delete ok;
    delete infoTE;
}
void EditBmDetail::OKClicked()
{
    GString name = nameLE->text();
    if( name != _pBm->Name && _bookmarkSeq->checkNameExists(name))
    {
            QMessageBox::information(this, "Bookmark", "A bookmark with this name already exists.");
            return;
    }
    okClicked = 1;
    close();
}

int EditBmDetail::saveClicked()
{
    return okClicked;
}

void EditBmDetail::CancelClicked()
{
    close();
}

void EditBmDetail::reject()
{
    QSettings settings(_CFG_DIR, "pmf6");
    settings.setValue(m_gstrSettingsName+"Geometry", this->saveGeometry());
    QDialog::reject();
}

void EditBmDetail::setBookmarkData(BOOKMARK *pBm, BookmarkSeq *bookmarkSeq)
{
    nameLE->setText(pBm->Name);
    tableLE->setText(pBm->Table);
    m_gstrOrgText = pBm->SqlCmd;
    _bookmarkSeq = bookmarkSeq;
    _pBm = pBm;
    displayData();
}

void EditBmDetail::getBookmarkData(BOOKMARK *pBm)
{
    if( pBm == NULL ) return;
    pBm->Name = nameLE->text();
    pBm->Table = tableLE->text();
    pBm->SqlCmd = infoTE->toPlainText();
}


void EditBmDetail::setText(GString text)
{
    m_gstrOrgText = text;
    displayData();
}

void EditBmDetail::setSqlHighlighter(PmfColorScheme colorScheme, GSeq <GString>* list)
{
    SqlHighlighter * sqlHighlighter = new SqlHighlighter(colorScheme, infoTE->document(), list);
    infoTE->setSqlHighlighter(sqlHighlighter);
}

void EditBmDetail::displayData()
{
    infoTE->clear();    
    infoTE->setLineWrapMode(QTextEdit::NoWrap);
    infoTE->setText(m_gstrOrgText);
}

void EditBmDetail::setLineWrapping(QTextEdit::LineWrapMode mode)
{
    infoTE->setLineWrapMode(mode);
}
