#include <QBoxLayout>
#include <QGridLayout>


#include "passwdFld.h"


PasswdFld::PasswdFld(QWidget* parent) : QWidget(parent)
{
    QWidget *window = new QWidget(this);
    QHBoxLayout *layout = new QHBoxLayout(window);
    layout->setSpacing(0);
    _pwdLE = new QLineEdit();
    _pwdLE->setStyleSheet("color: red;");
    layout->addWidget(_pwdLE);
    /*
    _parent = parent;
    //QHBoxLayout *myLayout = new QHBoxLayout(this);
    QGridLayout * grid = new QGridLayout(this);
    //myLayout->addLayout(grid);
    grid->addWidget(_pwdLE, 0, 0);
//    this->setLayout(myLayout);
//    myLayout->addWidget(_pwdLE);

    _pwdLE->setEchoMode(QLineEdit::Password);
*/

    _viewPwdB = new QPushButton(this);
    QPixmap pixmap(":eye3.png.piko");
    QIcon ButtonIcon(pixmap);
    _viewPwdB->setIcon(ButtonIcon);
    _viewPwdB->setIconSize(QSize(16,16));
    layout->addWidget(_viewPwdB);
    //grid->addWidget(_viewPwdB, 0, 1);
    connect( _viewPwdB, SIGNAL(clicked()), SLOT(togglePwdClicked()) );
    //this->setMinimumHeight(60);
    //myLayout->addWidget(_pwdLE);
    //myLayout->addWidget(_viewPwdB);

}

void PasswdFld::setText(GString txt)
{
    _pwdLE->setText(txt);
}

GString PasswdFld::text()
{
    return _pwdLE->text();
}


void PasswdFld::togglePwdClicked()
{
    QPixmap pixmap3(":eye3.png.piko");
    QPixmap pixmap2(":eye2.png.piko");

    if( _pwdLE->echoMode() == QLineEdit::Password)
    {
        QIcon ButtonIcon(pixmap2);
        _viewPwdB->setIcon(ButtonIcon);
        _viewPwdB->setIconSize(QSize(16,16));
        _pwdLE->setEchoMode(QLineEdit::Normal);
    }
    else
    {
        QIcon ButtonIcon(pixmap3);
        _viewPwdB->setIcon(ButtonIcon);
        _viewPwdB->setIconSize(QSize(16,16));
        _pwdLE->setEchoMode(QLineEdit::Password);
    }
}


PasswdFld::~PasswdFld()
{
}

