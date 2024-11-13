#include <QLineEdit>
#include <QPushButton>
#include <gstring.hpp>

#ifndef PASSWDFLD_H
#define PASSWDFLD_H

class PasswdFld: public QWidget
{
    Q_OBJECT
    public:
        explicit PasswdFld( QWidget* parent=0 );
        ~PasswdFld();
        void setText(GString txt);
        GString text();

    private slots:
        void togglePwdClicked();

    private:
        QWidget *_parent;
        QLineEdit * _pwdLE;
        QPushButton * _viewPwdB;
};

#endif

