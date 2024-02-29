//
//  This file is part of PMF (Poor Man's Flight), see README.PMF for details
//  Author: Gregor Leipelt (gregor@leipelt.de)  (C) G. Leipelt 2000
//

#ifndef _LOGINBOX_
#define _LOGINBOX_

#include <QDialog>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QMessageBox>
#include <QGridLayout>
#include <QCheckBox>
#include <gstring.hpp>
#include <dsqlplugin.hpp>
#include "connSet.h"

#include <gdebug.hpp>

#define LGNBOX_HELP "Hint: Click <Help>!"
#define LGNBOX_MARIADB "<optional>"
#define LGNBOX_CREATE_NODE "<Catalog DB> (click OK)"
#define LGNBOX_RUN_STARTUP "<Run Catalog Cmds>"
#define STARTUP_FILE_NAME "pmf_startup.xml"
#define LGNBOX_FIND_DATABASES "<Find DBs> (click OK)"
#define LGNBOX_ALL_DATABASES "<All>"


class LoginBox : public QDialog
{
    Q_OBJECT
    GString dbType, dbName, userName, passWord, hostName, port;

public:
    LoginBox( GDebug *pGDeb, QWidget* parent = NULL );
    ~LoginBox();
    GString DBType();
    GString DBName();
    GString HostName();
    GString UserName();
    GString PassWord();
    GString Port();
    void tm(QString message){QMessageBox::information(this, "db-Connect", message);}
    void setDefaultCon();
    DSQLPlugin * getConnection();
    int checkPlugins(GString pluginPath);
    void msg(GString txt);

    void initBox();
protected slots:
    virtual void cancelClicked();
    virtual void okClicked();
    virtual void helpClicked();
    void closeEvent( QCloseEvent*);
    void keyPressEvent(QKeyEvent *event);
    void getConnData(int pos);
    void getAllDatabases(int pos);
    void getAllHosts();


protected:
    QComboBox* dbNameCB;
    QComboBox* dbTypeCB;
    QLineEdit* userNameLE;
    QLineEdit* passWordLE;
    QComboBox* hostNameCB;
    QCheckBox* setDefaultConnChkBx;
    //QLineEdit* portLE;
    QPushButton* okB;
    QPushButton* cancelB;
    QPushButton* helpB;

private:
    int bindAndConnect(DSQLPlugin *pDSQL, GString db, GString uid, GString pwd, GString node, GString port);
    GString checkBindFile(GString bndFile);
    void runAutoCatalog();
    int haveStartupFile();
    GString getPort(GString in);
    GString getHost(GString in);
    void findDatabases();



    void fillDbNameCB(GString host);
    void fillHostNameCB(GString dbType);
    void runPluginCheck();
    void createPluginInfo();
    int nodeNameHasChanged(GString alias, GString hostName);
    DSQLPlugin *  m_pIDSQL;
    ConnSet *m_pConnSet;

    void deb(GString msg);
    QGridLayout * m_pMainGrid;
    GDebug * m_pGDeb;
    QLabel * pInfoLBL;

};

#endif
