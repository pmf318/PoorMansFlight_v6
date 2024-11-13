//
//  This file is part of PMF (Poor Man's Flight), see README.PMF for details
//  Author: Gregor Leipelt (gregor@leipelt.de)  (C) G. Leipelt 2000
//
#include "pmfSchemaCB.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h> 
//#include <windows.h> 
#include <gstring.hpp>
#include <QComboBox>
#include <qmessagebox.h>


#include "pmfdefines.h"

PmfSchemaCB::PmfSchemaCB(QWidget *parent, GString currentSchema )
 :QComboBox(parent)
{    
	m_gstrCurrentSchema = currentSchema;
}

int PmfSchemaCB::fill(DSQLPlugin * pDSQL, GString schema, int hideSysTabs, int caseSensitive)
{
    m_pGDeb = NULL;

    //no m_pGDeb yet. Needs setGDebg
    //deb(__FUNCTION__, "start");

    this->clear();
	GString s;
    DSQLPlugin *dsql = new DSQLPlugin(*pDSQL);
    dsql->getTabSchema();


	int cur = -1;
	this->addItem( _selStringCB); //First item is "<select>"
    for( unsigned int i = 1; i <= dsql->numberOfRows(); ++i)
	{        
        s = dsql->rowElement(i,1).strip().strip("'").strip();
        if( hideSysTabs && isSysTab(pDSQL, s) ) continue;

        if( !caseSensitive )
        {
            if( GString(schema).upperCase() == GString(s).upperCase() ) cur = i;
        }
        else if( schema == s ) cur = i;
		this->addItem(s);
	}
	if( cur >= 0 ) 
	{
		this->setCurrentIndex(cur);
	}
    delete dsql;
    //deb(__FUNCTION__, "end");
    return cur;
}

int PmfSchemaCB::isSysTab(DSQLPlugin * pDSQL, GString in)
{
    if( pDSQL->getDBType() == DB2 ||  pDSQL->getDBType() == DB2ODBC )
    {
        if( in == "SYSCAT" || in == "SYSIBM" || in == "SYSIBMADM" || in == "SYSPUBLIC" ||
            in == "SYSSTAT" || in == "SYSTOOLS") return 1;
    }
    else if( pDSQL->getDBType() == POSTGRES )
    {
        if( in == "information_schema" || in == "pg_catalog" ||in == "pg_toast" ) return 1;
    }
    return 0;
}


void PmfSchemaCB::deb(GString fnName, GString txt)
{
    if( m_pGDeb ) m_pGDeb->debugMsg("tabEdit", 1, "::"+fnName+" "+txt);
}
