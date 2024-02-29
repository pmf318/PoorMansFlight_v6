//
//  This file is part of PMF (Poor Man's Flight), see README.PMF for details
//  Author: Gregor Leipelt (gregor@leipelt.de)  (C) G. Leipelt
//

#ifndef _POSTGRES_
#include <postgres.hpp>
#endif

#include <QVariant>
#include <QStringList>
#include <QDebug>
#include <QUuid>
#include <QMessageBox>

#include <idsql.hpp>
#include <gdebug.hpp>
#include <gfile.hpp>
#include <gstuff.hpp>


#include <fstream>
#include "pmf_pgsql_def.hpp"
#ifdef MAKE_VC
/////#include <windows.h>
#endif

#ifndef max
  #define max(A,B) ((A) >(B) ? (A):(B))
#endif

///////////////////////////////////////////
//Global static instance counter.
//Each instance saves its private instance value in m_iMyInstance.
static int m_postgresobjCounter = 0;

#define XML_MAX 250



/***********************************************************************
 * This class can either be instatiated or loaded via dlopen/loadlibrary
 ***********************************************************************/

//Define functions with C symbols (create/destroy instance).
#ifndef MAKE_VC
extern "C" postgres* create()
{
    //printf("Calling postgres creat()\n");
    return new postgres();
}
extern "C" void destroy(postgres* pODBCSQL)
{
   if( pODBCSQL ) delete pODBCSQL ;
}
#else
extern "C" __declspec( dllexport ) postgres* create()
{
    //printf("Calling postgres creat()\n");
    _flushall();
    return new postgres();
}	
extern "C" __declspec( dllexport ) void destroy(postgres* pODBCSQL)
{
    if( pODBCSQL ) delete pODBCSQL ;
}	
#endif
/***********************************************************
 * CLASS
 **********************************************************/

postgres::postgres(postgres  const &o)
{
    m_pRes = NULL;
	m_iLastSqlCode = 0;
    m_postgresobjCounter++;
    m_iTruncationLimit = 500;
    m_iMyInstance = m_postgresobjCounter;
    m_pGDB = o.m_pGDB;
    deb("CopyCtor start");

    m_odbcDB  = POSTGRES;
    m_iReadUncommitted = o.m_iReadUncommitted;
    m_strCurrentDatabase = o.m_strCurrentDatabase;
    m_iIsTransaction = o.m_iIsTransaction;
    m_strDB = o.m_strDB;
    m_strHost = o.m_strHost;
    m_strPWD = o.m_strPWD;
    m_strUID = o.m_strUID;
    m_strPort  = o.m_strPort;
    m_strEncoding = o.m_strEncoding;
    deb("Copy CTor set encoding: "+m_strEncoding);
    deb("CopyCtor, orgPort: "+GString(o.m_strPort)+", new: "+GString(m_strPort));
    m_iReadClobData = o.m_iReadClobData;
    m_strCurrentDatabase = o.m_strCurrentDatabase;
    m_strLastSqlSelectCommand = o.m_strLastSqlSelectCommand;

    GString db, uid, pwd;
    o.getConnData(&db, &uid, &pwd);

    //m_pgConn = o.m_pgConn;
    deb("CopyCtor: calling connect, connection data: "+m_strDB+", uid: "+m_strUID+", host: "+m_strHost+", port: "+GString(m_strPort));
    this->connect(m_strDB, m_strUID, m_strPWD, m_strHost, GString(m_strPort));
    this->setEncoding(m_strEncoding);
    //if( m_strCurrentDatabase.length() ) this->initAll("USE "+m_strCurrentDatabase);
    //!!TODO
//    if( m_iReadUncommitted ) readRowData("set transaction isolation level read uncommitted");
//    else readRowData("set transaction isolation level REPEATABLE READ");
    deb("Copy CTor done");
}

postgres::postgres()
{
    m_postgresobjCounter++;
    //m_IDSQCounter++;
    m_iTruncationLimit = 500;
    m_iMyInstance = m_postgresobjCounter;

	m_iLastSqlCode = 0;
    m_iIsTransaction = 0;	
    m_odbcDB  = POSTGRES;

    m_strDB ="";
    m_strUID ="";
    m_strPWD = "";
    m_strHost = "localhost";
    m_strPort = 5432;
    m_strEncoding = "";

    m_pGDB = NULL;
    m_strLastSqlSelectCommand = "";
    m_iReadUncommitted = 0;
    m_iReadClobData = 0;
    m_strCurrentDatabase = "";
    m_pgConn = NULL;    
    m_pRes = NULL;
    //printf("postgres[%i]> DefaultCtor done. Port: %i\n", m_iMyInstance, m_iPort );
}
postgres* postgres::clone() const
{
    return new postgres(*this);
}

postgres::~postgres()
{
    deb("Dtor start, current count: "+GString(m_postgresobjCounter));
    disconnect();

    m_postgresobjCounter--;
    //m_IDSQCounter--;
    deb("Dtor, clearing RowData...");
    clearSequences();
    deb("Dtor done, current count: "+GString(m_postgresobjCounter));
}

int postgres::getConnData(GString * db, GString *uid, GString *pwd) const
{
    *db = m_strDB;
    *uid = m_strUID;
    *pwd = m_strPWD;
	return 0;
}


GString postgres::connect(GString db, GString uid, GString pwd, GString host, GString port)
{    
    deb("::connect, connect to DB: "+db+", uid: "+uid+", host: "+host+", port: "+port);

    if( !port.length() || port == "0") port = "5432";
    m_strDB = db;
    m_strUID = uid;
    m_strPWD = pwd;
    m_strNode = host;
    m_strHost = host;
    m_strPort = port;
    m_strCurrentDatabase = db;
    deb("::connect, connection data: "+m_strDB+", uid: "+m_strUID+", host: "+host+", port: "+port);

    m_pgConn = PQsetdbLogin(host, port, "", "", db, uid, pwd);
    if (PQstatus(m_pgConn) != CONNECTION_OK)
    {
        return sqlError();
    }
    return "";
}

GString postgres::connect(CON_SET * pCs)
{
    m_strDB = pCs->DB;
    m_strUID = pCs->UID;
    m_strPWD = pCs->PWD;
    m_strNode = pCs->Host;
    m_strHost = pCs->Host;
    m_strPort = pCs->Port;
    m_strCurrentDatabase = pCs->DB;

    GString connInfo = "host="+pCs->Host;
    connInfo += " dbname="+pCs->DB;
    connInfo += " user="+pCs->UID;
    connInfo += " password="+pCs->PWD;
    connInfo += " port="+pCs->Port;
    m_pgConn = PQconnectdb(connInfo);
    if (PQstatus(m_pgConn) != CONNECTION_OK)
    {
        return sqlError();
    }
    return "";

//    return this->connect(pCs->DB, pCs->UID, pCs->PWD, pCs->Host, pCs->Port);
}

int postgres::disconnect()
{	

    deb("Disconnecting and closing...");
    if( !m_pgConn ) return 0;
    deb("Disconnecting and closing...calling PQfinish...");
    PQfinish(m_pgConn);
    m_pgConn = NULL;
    deb("Disconnecting and closing...done.");
	return 0;
}


GString postgres::initAllInternal(GString message, unsigned long maxRows,  int getLen, int maskLargeVals)
{
    printf("initall start, cmd: %s, getLen: %i\n", (char*) message, getLen);
    deb("::initAll, start. msg: "+message);

    GRowHdl * pRow;

    resetAllSeq();

    m_strLastSqlSelectCommand = message;
    m_iNumberOfRows = 0;
    m_iLastSqlCode = 0;



    GString firstWord = message.stripLeading(' ').stripLeading('\t');
    if( firstWord.subString(1, 6).upperCase() != "SELECT" )
    {
        m_pRes = PQexec(m_pgConn, message);
        if (PQresultStatus(m_pRes) == PGRES_COMMAND_OK)
        {
            return "";
        }
        else
        {
            sqlError();
            if(m_strLastError.length()) return m_strLastError;
        }
    }
    else
    {
        if( maxRows > 0 )  message = message + " LIMIT "+GString(maxRows);
    }

    // PQexec(m_pgConn, message); will fetch EVERYTHING,including LARGE bytea - this is completely dumb.
    // We rewrite the SELECT and do somthing like "if( bytea ) -> select substring(bytea)"
    // This prevents bytea ols from being FULLY transferred to the client.
    tryFastCursor(message);

    m_pRes = PQexec(m_pgConn, "FETCH ALL in myCRS");
    if (PQresultStatus(m_pRes) != PGRES_TUPLES_OK)
    {
        return sqlError();
    }
    if( getLen )
    {
        for(int i = 1; i <= m_iNumberOfColumns; ++i ) sqlLenSeq.add(hostVariable(i).length()+1);
    }
    GString data;
    long maxLen;

    for (int i = 0; i < PQntuples(m_pRes); i++)
    {
        pRow = new GRowHdl;
        for (int j = 0; j < m_iNumberOfColumns; j++)
        {

            if( PQgetisnull(m_pRes, i, j) ) data = "NULL";
            else if( isLOBCol(j+1) ) data = "@DSQL@BLOB";
            else if( isXMLCol(j+1) ) data = "@DSQL@XML";
            else if( isNumType(j+1) ) data = PQgetvalue(m_pRes, i, j);
            else data = "'"+GString(PQgetvalue(m_pRes, i, j))+"'";

            pRow->addElement(data);
            if( getLen )
            {
                maxLen = max((signed) data.strip("'").strip().length()+1, sqlLenSeq.elementAtPosition(j+1));
                sqlLenSeq.replaceAt(j+1, maxLen);
            }
        }

        if( maxRows > 0 && m_iNumberOfRows >= maxRows ) break;
        m_iNumberOfRows++;
        allRowsSeq.add( pRow );
    }
    PQclear(m_pRes);
    m_pRes = PQexec(m_pgConn, "CLOSE myCRS");

    PQclear(m_pRes);
    m_pRes = PQexec(m_pgConn, "END");

    return sqlError();
}

GString postgres::getConstraint(GString sqlCmd)
{
    GString tmp = sqlCmd;
    int start = tmp.upperCase().indexOf(" FROM ");
    if( start > 0 ) return sqlCmd.subString(start, sqlCmd.length()).strip();
    return "";
}

int postgres::tryFastCursor(GString sqlCmd)
{
    GString outerSelect = fillHostVarSeq(sqlCmd);
    GString tryCmd = outerSelect + " FROM ("+sqlCmd +")";


    m_pRes = PQexec(m_pgConn, "BEGIN");

    tryCmd = "DECLARE myCRS CURSOR FOR "+tryCmd;
    m_pRes = PQexec(m_pgConn, tryCmd);
    if (PQresultStatus(m_pRes) == PGRES_COMMAND_OK)
    {
        return 0;
    }
    m_pRes = PQexec(m_pgConn, "END");
    PQclear(m_pRes);


    m_pRes = PQexec(m_pgConn, "BEGIN");
    PQclear(m_pRes);
    tryCmd = "DECLARE myCRS CURSOR FOR "+sqlCmd;
    m_pRes = PQexec(m_pgConn, tryCmd);
    if (PQresultStatus(m_pRes) != PGRES_COMMAND_OK)
    {
        sqlError();
        m_pRes = PQexec(m_pgConn, "END");
        PQclear(m_pRes);
        return 1;
    }
    return 0;
}


GString postgres::fillHostVarSeq(GString message)
{
    deb("fillHostVarSeq start");
    GString hostVar;
    m_pRes = PQexec(m_pgConn, "BEGIN");
    message = "DECLARE myCRS CURSOR FOR "+message;
    m_pRes = PQexec(m_pgConn, message);

    if (PQresultStatus(m_pRes) != PGRES_COMMAND_OK)
    {
        sqlError();
        m_pRes = PQexec(m_pgConn, "END");
        PQclear(m_pRes);
        return m_strLastError;
    }
    PQclear(m_pRes);
    m_iNumberOfColumns = PQnfields(m_pRes);
    m_pRes = PQexec(m_pgConn, "FETCH FORWARD 0 from myCRS");
    if (PQresultStatus(m_pRes) != PGRES_TUPLES_OK)
    {
        return sqlError();
    }

    GString newSelect = "Select ";
    deb("cols: "+GString(m_iNumberOfColumns));
    m_iNumberOfColumns = PQnfields(m_pRes);
    for (int i = 0; i < m_iNumberOfColumns; i++)
    {
        hostVar = GString(PQfname(m_pRes, i));
        hostVarSeq.add(GString(hostVar).strip("\""));

        //For bytea and xml, this will get replaced with DSQL@BLOB and DSQL@XML in ::initAll
        if( PQftype(m_pRes, i) == PSTGRS_BYTEA )
        {
            //newSelect += "case when "+hostVar+" is null then NULL else 'bta' end, ";
            //newSelect += "'--',";
            newSelect += "substr(\""+hostVar+"\", 1, 1),";
        }
        else if( PQftype(m_pRes, i) == PSTGRS_XML )
        {
//            //newSelect += "case when "+hostVar+" is null then NULL else 'xml' end, ";
            newSelect += "substr(cast(\""+hostVar+"\" as text), 1, 1),";
        }
        else newSelect += "\""+GString(PQfname(m_pRes, i))+"\",";

        sqlTypeSeq.add(PQftype(m_pRes, i));
        deb("HostVars: col "+GString(i)+", name: "+GString(PQfname(m_pRes, i))+", type: "+GString(PQftype(m_pRes, i)));
    }
    PQclear(m_pRes);
    m_pRes = PQexec(m_pgConn, "CLOSE myCRS");

    PQclear(m_pRes);
    m_pRes = PQexec(m_pgConn, "END");
    deb("fillHostVarSeq donw");
    return newSelect.stripTrailing(',');
}


GString postgres::initAll(GString message, unsigned long maxRows,  int getLen)
{
    //return initAllInternal(message, maxRows, getLen);

    printf("initall start, cmd: %s, getLen: %i\n", (char*) message, getLen);
    deb("::initAll, start. msg: "+message);

    GRowHdl * pRow;

    resetAllSeq();

    m_strLastSqlSelectCommand = message;
    m_iNumberOfRows = 0;
    m_iLastSqlCode = 0;

    GString firstWord = message;
    if( firstWord.stripLeading(' ').subString(1, 6).upperCase() != "SELECT" )
    {
        //Bad
        m_pRes = PQexec(m_pgConn, message);
        if (PQresultStatus(m_pRes) == PGRES_COMMAND_OK)
        {
            return "";
        }
        else
        {
            sqlError();
            if(m_strLastError.length()) return m_strLastError;
        }
    }
    else
    {
        if( maxRows > 0 )  message = message + " LIMIT "+GString(maxRows);
    }


    m_pRes = PQexec(m_pgConn, "BEGIN");
    if (PQresultStatus(m_pRes) != PGRES_COMMAND_OK)
    {
        return sqlError();
    }

    PQclear(m_pRes);


    message = "DECLARE myCRS CURSOR FOR "+message;
    m_pRes = PQexec(m_pgConn, message);
    if (PQresultStatus(m_pRes) != PGRES_COMMAND_OK)
    {
        sqlError();
        m_pRes = PQexec(m_pgConn, "END");
        PQclear(m_pRes);
        return m_strLastError;
    }
    PQclear(m_pRes);
    //Bad
    m_pRes = PQexec(m_pgConn, "FETCH ALL in myCRS");
    //m_pRes = PQexec(m_pgConn, "FETCH FIRST in myCRS");
    if (PQresultStatus(m_pRes) != PGRES_TUPLES_OK)
    {
        return sqlError();
    }
    deb("cols: "+GString(m_iNumberOfColumns));
    m_iNumberOfColumns = PQnfields(m_pRes);
    for (int i = 0; i < m_iNumberOfColumns; i++)
    {
        hostVarSeq.add(GString(PQfname(m_pRes, i)).strip("\""));
        sqlTypeSeq.add(PQftype(m_pRes, i));

        deb("HostVars: col "+GString(i)+", name: "+GString(PQfname(m_pRes, i))+", type: "+GString(PQftype(m_pRes, i)));
    }


    if( getLen )
    {
        for(int i = 1; i <= m_iNumberOfColumns; ++i ) sqlLenSeq.add(hostVariable(i).length()+1);
    }
    GString data;
    long maxLen;
    printf("Start get\n");
    //while (PQresultStatus(res) == PGRES_TUPLES_OK)
    for (int i = 0; i < PQntuples(m_pRes); i++)
    {
        pRow = new GRowHdl;
        for (int j = 0; j < m_iNumberOfColumns; j++)
        {

            if( PQgetisnull(m_pRes, i, j) )data = "NULL";
            else if( isLOBCol(j+1) ) data = "@DSQL@BLOB";
            else if( isNumType(j+1) ) data = PQgetvalue(m_pRes, i, j);
            else data = "'"+GString(PQgetvalue(m_pRes, i, j))+"'";

            pRow->addElement(data);
            if( getLen )
            {
                maxLen = max((signed) data.strip("'").strip().length()+1, sqlLenSeq.elementAtPosition(j+1));
                sqlLenSeq.replaceAt(j+1, maxLen);
            }

        }

        if( maxRows > 0 && m_iNumberOfRows >= maxRows ) break;
        m_iNumberOfRows++;
        allRowsSeq.add( pRow );
    }
    printf("Stop get\n");
    PQclear(m_pRes);
    m_pRes = PQexec(m_pgConn, "CLOSE myCRS");

    PQclear(m_pRes);
    m_pRes = PQexec(m_pgConn, "END");

    return sqlError();
}

//void postgres::setSimpleColType(enum_field_types type)
//{
//    switch(type)
//    {
//    case MYSQL_TYPE_SHORT:
//    case MYSQL_TYPE_INT24:
//        simpleColTypeSeq.add(CT_INTEGER);
//        break;

//    case MYSQL_TYPE_LONG:
//    case MYSQL_TYPE_DOUBLE:
//    case MYSQL_TYPE_LONGLONG:
//        simpleColTypeSeq.add(CT_LONG);
//        break;

//    case MYSQL_TYPE_NEWDECIMAL:
//        simpleColTypeSeq.add(CT_DECIMAL);
//        break;


//    case MYSQL_TYPE_FLOAT:
//        simpleColTypeSeq.add(CT_FLOAT);
//        break;

//    case MYSQL_TYPE_TIMESTAMP:
//    case MYSQL_TYPE_DATE:
//    case MYSQL_TYPE_TIME:
//    case MYSQL_TYPE_DATETIME:
//    case MYSQL_TYPE_YEAR:
//    case MYSQL_TYPE_NEWDATE:
//    case MYSQL_TYPE_TIMESTAMP2:
//    case MYSQL_TYPE_DATETIME2:
//    case MYSQL_TYPE_TIME2:
//        simpleColTypeSeq.add(CT_DATE);
//        break;

//    case MYSQL_TYPE_NULL:
//    case MYSQL_TYPE_VARCHAR:
//    case MYSQL_TYPE_VAR_STRING:
//    case MYSQL_TYPE_STRING:
//    case MYSQL_TYPE_BIT:
//        simpleColTypeSeq.add(CT_STRING);
//        break;

//    case MYSQL_TYPE_ENUM:
//    case MYSQL_TYPE_SET:

//    case MYSQL_TYPE_TINY_BLOB:
//    case MYSQL_TYPE_MEDIUM_BLOB:
//    case MYSQL_TYPE_LONG_BLOB:
//    case MYSQL_TYPE_BLOB:
//    case MYSQL_TYPE_GEOMETRY:
//        simpleColTypeSeq.add(CT_CLOB);
//        break;

//    default:
//        simpleColTypeSeq.add(CT_STRING);
//        break;
//    }
//}

int postgres::commit()
{
    //m_db.commit();
	return 0;
}

int postgres::rollback()
{
    //m_db.rollback();
	return 0;
}


int postgres::initRowCrs()
{
	deb("::initRowCrs");
    if( allRowsSeq.numberOfElements() == 0 ) return 1;
    m_pRowAtCrs = allRowsSeq.initCrs();
    return 0;
}
int postgres::nextRowCrs()
{	
    if( m_pRowAtCrs == NULL ) return 1;
    m_pRowAtCrs = allRowsSeq.setCrsToNext();
    return 0;
}
long  postgres::dataLen(const short & pos)
{
    if( pos < 1 || pos > (short) sqlLenSeq.numberOfElements()) return 0;
    deb("Len at "+GString(pos)+": "+GString(sqlLenSeq.elementAtPosition(pos)));    
    return sqlLenSeq.elementAtPosition(pos);
}

int postgres::isDateTime(int pos)
{
    if( pos < 1 || pos > (int)sqlTypeSeq.numberOfElements() ) return 0;
    switch( sqlTypeSeq.elementAtPosition(pos) )
    {
        case DATE_PMF:
        case TIME_PMF:
        case TIMESTAMP_PMF:
        case TIMESTAMPTZ_PMF:
            return 1;
    }
    return 0;
}

int postgres::isNumType(int pos)
{
    if( pos < 1 || pos > (int) sqlTypeSeq.numberOfElements() ) return 0;

    switch( sqlTypeSeq.elementAtPosition(pos) )
    {
        case INT8_PMF:
        case INT2_PMF:
        case INT4_PMF:
        case FLOAT4_PMF:
        case FLOAT8_PMF:
        case INT4ARRAY_PMF:
    case NUMERIC_PMF:
            return 1;
    }
    return 0;
}

int postgres::isXMLCol(int pos)
{
    if( pos < 1 || pos > (int) sqlTypeSeq.numberOfElements() ) return 0;
    if( sqlTypeSeq.elementAtPosition(pos) == PSTGRS_XML ) return 1;
    else return 0;
    //return xmlSeq.elementAtPosition(pos);
}


int postgres::hasForBitData()
{
    return 0;
}
int postgres::isForBitCol(int pos)
{
    if( pos < 1 || (unsigned long)pos > sqlTypeSeq.numberOfElements() ) return 0;
    if( sqlTypeSeq.elementAtPosition(pos) == PSTGRS_UUID ) return 1;
    else return 0;
}
int postgres::isBitCol(int i)
{
    if( i < 1 || (unsigned long)i > sqlBitSeq.numberOfElements() ) return 0;
    return sqlBitSeq.elementAtPosition(i);
}

unsigned int postgres::numberOfColumns()
{
    //deb("::numberOfColumns called");
    return m_iNumberOfColumns;
}

unsigned long postgres::numberOfRows()
{
    return m_iNumberOfRows;
}
GString postgres::dataAtCrs(int col)
{
    if( m_pRowAtCrs == NULL ) return "@CrsNotOpen";
    if( col < 1 || (unsigned long)col > m_pRowAtCrs->elements() ) return "@OutOfReach";
    return m_pRowAtCrs->rowElementData(col);
}
GString postgres::rowElement( unsigned long row, int col)
{
    //Overload 1
    //tm("::rowElement for "+GString(line)+", col: "+GString(col));
    if( row < 1 || row > allRowsSeq.numberOfElements() ) return "@OutOfReach";
    GRowHdl *aRow;
    aRow = allRowsSeq.elementAtPosition(row);
    if( row < 1 || (unsigned long) col > aRow->elements() ) return "OutOfReach";
    return aRow->rowElementData(col);
}
int postgres::positionOfHostVar(const GString& hostVar)
{
    unsigned long i;
    for( i = 1; i <= hostVarSeq.numberOfElements(); ++i )
    {
        if( hostVarSeq.elementAtPosition(i) == hostVar ) return i;
    }
    return 0;
}
GString postgres::rowElement( unsigned long row, GString hostVar)
{
    //Overload 2
    unsigned long pos;
    if( row < 1 || row > allRowsSeq.numberOfElements() ) return "@OutOfReach";
    GRowHdl *aRow;
    aRow = allRowsSeq.elementAtPosition(row);
    pos = positionOfHostVar(hostVar);
    if( pos < 1 || pos > aRow->elements() ) return "OutOfReach";
    return aRow->rowElementData(pos);
}


GString postgres::hostVariable(int col)
{
    //deb("::hostVariable called, col: "+GString(col));
    if( col < 1 || col > (short) hostVarSeq.numberOfElements() ) return "@hostVariable:OutOfReach";
    return hostVarSeq.elementAtPosition(col);
}



GString postgres::currentCursor(GString filter, GString command, long curPos, short commitIt, GSeq <GString> *fileList, GSeq <long> *lobType)
{
    deb(__FUNCTION__, "filter: "+filter+", cmd: "+command);
    int rc;


    if( curPos < 0 ) return "CurrentCursor(): POS < 0";
    deb(__FUNCTION__, "done, rc: "+GString(rc));
    return rc ? m_strLastError : GString("");
}



int postgres::sqlCode()
{
	return m_iLastSqlCode;
}
GString postgres::sqlError()
{    
    char* sqlCode = NULL;
    if(m_pRes)
    {
        sqlCode = PQresultErrorField(m_pRes, PG_DIAG_SQLSTATE);
        deb("::sqlCode(1): "+GString(sqlCode));
        if( sqlCode ) printf("ERC: %s\n", sqlCode);
    }

    m_iLastSqlCode = 0;
    deb("::sqlError: m_iLastSqlCode = "+GString(m_iLastSqlCode));
    m_strLastError = PQerrorMessage(m_pgConn);
    deb("::sqlError: m_strLastError = "+GString(m_strLastError));

    if( m_strLastError.length() )
    {
        m_iLastSqlCode = -1;
//        if( m_pgConn) PQfinish(m_pgConn);
//        m_pgConn = NULL;
    }
    printf("POSTGRES: -------------------------\nError: %s\nLastCmd: %s\n------------------\n", (char*) m_strLastError, (char*) m_strLastSqlSelectCommand);
    deb("FULL: "+m_strLastError+" <-> "+m_strLastSqlSelectCommand);
    if( sqlCode )
    {
        deb("::sqlCode(2): "+GString(sqlCode));
        return GString(sqlCode)+": "+m_strLastError;
    }
    return m_strLastError;
}		

int postgres::getTabSchema()
{
    GString cmd = "SELECT DISTINCT(nspname) FROM pg_catalog.pg_namespace ORDER BY nspname";
    //GString cmd = "SELECT distinct table_schema from information_schema.tables where table_catalog='"+m_strCurrentDatabase+"'";
    this->initAll(cmd);
    return 0;
}
int postgres::getTables(GString schema)
{
    GString cmd = "SELECT table_name FROM information_schema.tables where table_schema='"+schema+"' and table_catalog='"+m_strCurrentDatabase+"' order by table_name";
    this->initAll(cmd);
    return 0;
}
void postgres::convToSQL( GString& input )
{
    GStuff::convToSQL(input);
    return;
}

int postgres::getAllTables(GSeq <GString > * tabSeq, GString filter)
{
    PMF_UNUSED(tabSeq);
    PMF_UNUSED(filter);
    /*
	QStringList sl = m_db.tables(QSql::AllTables);
	tabSeq->removeAll();
	for (int i = 0; i < sl.size(); ++i) tabSeq->add(sl.at(i));
    */
	return 0;
}
short postgres::sqlType(const short & col)
{
   if( col < 1 || col > (short) sqlTypeSeq.numberOfElements() ) return -1;
   else return sqlTypeSeq.elementAtPosition(col);
}
short postgres::sqlType(const GString & colName)
{
   for( short i = 1; i <= (short) hostVarSeq.numberOfElements(); ++ i )
   {
      if( hostVarSeq.elementAtPosition(i) == GString(colName).strip("\"") )
      {
          return sqlTypeSeq.elementAtPosition(i);
      }
   }
   return -1;
}
GString postgres::realName(const short & sqlType)
{
	switch(sqlType)
	{
            return "Numeric";
			
            return "Time";
			
		default:
			return "'string'";
	}
	return "";
}

int postgres::loadFileIntoBufNoConvert(GString fileName, char** fileBuf, int *size)
{
    FILE * f;
    //Apparently CR/LF is not counted in fread.
    //Even XML sources should be opened with "rb".
    f = fopen(fileName, "rb");
    if( f != NULL )
    {
        char* fb;
        int sz;

        deb("::loadFileIntoBuf reading file....");
        fseek(f, 0, SEEK_END);
        sz = ftell(f);
        fseek(f, 0, SEEK_SET);

        fb = new char[sz+1];
        fread(fb, 1, sz, f);
        fclose(f);
        fb[sz] = '\0';
        deb("::loadFileIntoBuf reading file...OK, size: "+GString(sz));
        *size = sz;
        *fileBuf = fb;
        return 0;
    }
    return 1;
}

int postgres::loadFileIntoBuf(GString fileName, char** fileBuf, int *size)
{

    FILE * f;
    //Apparently CR/LF is not counted in fread.
    //Even XML sources should be opened with "rb".
    f = fopen(fileName, "rb");
    if( f != NULL )
    {
        char* fb;
        int sz;
        deb("::loadFileIntoBuf reading file....");
        fseek(f, 0, SEEK_END);
        sz = ftell(f);
        fseek(f, 0, SEEK_SET);
        fb = new char[sz+1];
        int res = fread(fb,sizeof(char),sz, f);
        fclose(f);
        deb("::trying to encode....");
        std::string enc = GStuff::base64_encode((unsigned char*) fb, sz);
        deb("::trying to encode....Done.");
        delete [] fb;
        sz = enc.size();
        fb = new char[sz+1];
        strncpy(fb, (char*)GString(enc), sz);
        fb[sz] = '\0';

        *size = sz;
        *fileBuf = fb;
        deb("::loadFileIntoBuf reading file...OK, encoded size: "+GString(*size)+", bytesRead from orig. file: "+GString(res)+", GString(enc): "+GString(GString(enc).length()));

        /*
        FILE * fOut;
        fOut = fopen("/home/moi/develop/c/pmf6/decoded.bin", "ab");
        std::string ret = GStuff::base64_decode((char*) fb);
        fwrite((char*) ret.c_str(),1,ret.size(),fOut);
        fclose(fOut);
        */
        return 0;
    }
    return 1;
}

/*
int postgres::loadFileIntoBuf(GString fileName, char** fileBuf, int *size)
{

    FILE * f;
    //Apparently CR/LF is not counted in fread.
    //Even XML sources should be opened with "rb".
    f = fopen(fileName, "rb");
    if( f != NULL )
    {
        deb("::loadFileIntoBuf reading file....");
        fseek(f, 0, SEEK_END);
        *size = ftell(f);
        fseek(f, 0, SEEK_SET);        
        *fileBuf = new char[(*size)+1];
        int res = fread(*fileBuf,sizeof(char),*size, f);
        fclose(f);
        std::string enc = GStuff::base64_encode((unsigned char*) *fileBuf, *size);

        delete [] *fileBuf;
        *fileBuf = new char[enc.size()+1];
        memset(*fileBuf, '0', enc.size());
        memcpy(*fileBuf, GString(enc), enc.size());
        *size = enc.size();
        *fileBuf[*size] = 0;

        printf("BUF: %s\n", *fileBuf);
        deb("::loadFileIntoBuf reading file...OK, encoded size: "+GString(*size)+", bytesRead from orig. file: "+GString(res)+", GString(enc): "+GString(GString(enc).length()));

        return 0;
    }
    return 1;
}
*/

void postgres::impExpLob()
{
    Oid         lobjOid;

    m_pRes = PQexec(m_pgConn, "begin");
    PQclear(m_pRes);

    lobjOid = lo_import(m_pgConn, "/home/moi/tok.txt");

    lo_export(m_pgConn, lobjOid, "/home/moi/tok3.txt");

    m_pRes = PQexec(m_pgConn, "end");
    PQclear(m_pRes);
}

GString postgres::allPurposeFunction(GKeyVal *pKeyVal)
{
    //GString s =  (*pKeyVal)[1]->key;
    if( pKeyVal->hasKey("UNLINK_BLOB"))
    {
        GString oid = pKeyVal->getValForKey("UNLINK_BLOB");
        m_pRes = PQexec(m_pgConn, "begin");
        PQclear(m_pRes);
        lo_unlink(m_pgConn, oid.asInt());
        m_pRes = PQexec(m_pgConn, "end");
        return sqlError();
    }
    if( pKeyVal->hasKey("EXPORT_TO_CSV"))
    {
        GString cmd = pKeyVal->getValForKey("SQLCMD");
        GString trgFile = pKeyVal->getValForKey("TARGET_FILE");
        GString delim = pKeyVal->getValForKey("DELIM");
        int wrHead = pKeyVal->getValForKey("WRITE_HEADER").asInt();
        int byteaAsFile = pKeyVal->getValForKey("BYTEA_AS_FILE").asInt();
        int xmlAsFile = pKeyVal->getValForKey("XML_AS_FILE").asInt();
        int exportLobs = pKeyVal->getValForKey("EXPORT_LOBS").asInt();
        return exportCsvBytea(cmd, trgFile, delim, wrHead, byteaAsFile, xmlAsFile, exportLobs);
    }
    return "";
}
/*
GString postgres::importCsvBytea(GString table, GString srcFile, GString logFile, GString delim, int hasHeader, int commitCount, int byteaAsFile)
{
    GFile gf(srcFile);
    GString line;
    if( gf.lines() == 0 ) return "";
    remove(logFile);

    GString cmd, err;
    line = gf.initLineCrs();
    int count = 0;
    int fullCount = 0;
    if( commitCount > 0 ) err = initAll("BEGIN");
    while( 1 )
    {
        line = gf.lineAtCrs();
        fullCount++;
        cmd = createInsertCmd(table, line, delim);
        if( cmd == ERR_COL_NUMBER )
        {
            writeLog("Line #"+GString(fullCount)+" cannot be imported: Wrong number of columns");
            if( !f.nextLineCrs() ) break;
            continue;
        }
        err = m_pDSQL->initAll(cmd);
        if( err.length() ) writeLog("Line #"+GString(fullCount)+" cannot be imported: "+err);
        count++;

        if( count >= commitCount && commitCount > 0 )
        {
            count = 0;
            err = m_pDSQL->initAll("COMMIT");
            err = m_pDSQL->initAll("BEGIN");
        }
        if( !f.nextLineCrs() ) break;
    }
    if( commitCount > 0 ) err = m_pDSQL->initAll("COMMIT");
}

GString postgres::createInsertCmd(GString table, GString line, GString delim)
{
    GString cmd = "INSERT INTO "+table+" VALUES (";
    GSeq <GString> elmts = line.split(delim);
    for(int i = 1; i <= elmts.numberOfElements(); ++i )
    {
        GString data = elmts.elementAtPosition(i)+GString(",");
        if( data )
        cmd += elmts.elementAtPosition(i)+GString(",");
    }
    cmd = cmd.stripTrailing(",") + ")";
    return cmd;
}
*/
void postgres::convertToBin(GString inFile, GString outFile)
{
    char buf[3];
    buf[2] = 0;

    std::string src = (char*)(inFile.stripLeading("\\x"));
    std::stringstream inStream(src);
    inStream.flags(std::ios_base::hex);
    std::ofstream outStream((char*)outFile, std::ios_base::binary | std::ios_base::out);
    int count = 0;
    while (inStream)
    {
        inStream >> buf[0] >> buf[1];
        long val = strtol(buf, nullptr, 16);
        outStream << static_cast<unsigned char>(val & 0xFF);
        count++;
    }

    int size = sizeof(outStream);
    printf("COUNT: %i, SIZE: %i\n", count, size);
}



GString postgres::exportCsvBytea(GString message, GString targetFile, GString delim, int writeHeader, int byteaAsFile, int xmlAsFile, int exportLobs)
{

    deb("::exportByteArrayToFile, start. msg: "+message);

/*
    if( sqlType(colName) == PSTGRS_BYTEA ) cmd = cmd.change(colName, "encode("+colName+",'base64')");
    //if( sqlType(colName) == PSTGRS_BYTEA ) cmd = cmd.change(colName, "encode("+colName+",'hex')");
    GString err = ps.initAll(cmd);
    if( err.length() ) return err;

    GString raw = ps.rowElement(1,1).strip("'");
    FILE * f;
    f = fopen(blobFile, "ab");
    if( sqlType(colName) == PSTGRS_BYTEA )
    {
        GString g = "";

        std::string ret = GStuff::base64_decode((char*) raw);
        *outSize = fwrite((char*) ret.c_str(),1,ret.size(),f);
    }
    else *outSize = fwrite((char*) raw, 1, raw.length(), f);
*/


    GFile trgFile(targetFile, GF_OVERWRITE);
    m_pRes = PQexec(m_pgConn, message);
    if (PQresultStatus(m_pRes) == PGRES_COMMAND_OK) return "";
    else
    {
        sqlError();
        if(m_strLastError.length()) return m_strLastError;
    }
    m_pRes = PQexec(m_pgConn, "BEGIN");
    if (PQresultStatus(m_pRes) != PGRES_COMMAND_OK)
    {
        return sqlError();
    }

    PQclear(m_pRes);

    message = "DECLARE myCRS CURSOR FOR "+message;
    m_pRes = PQexec(m_pgConn, message);
    if (PQresultStatus(m_pRes) != PGRES_COMMAND_OK)
    {
        sqlError();
        m_pRes = PQexec(m_pgConn, "END");
        PQclear(m_pRes);
        return m_strLastError;
    }
    PQclear(m_pRes);

    m_pRes = PQexec(m_pgConn, "FETCH ALL in myCRS");
    if (PQresultStatus(m_pRes) != PGRES_TUPLES_OK)
    {
        return sqlError();
    }
    deb("cols: "+GString(m_iNumberOfColumns));
    int colCount = PQnfields(m_pRes);
    GString out;

    //Header
    for (int i = 0; i < colCount; i++)
    {
        out += GString(PQfname(m_pRes, i)).strip("\"")+delim;
    }
    out = out.stripTrailing(delim);
    if( writeHeader ) trgFile.addLine(out);

    //Rows
    GString data;
    out = "";
    for (int i = 0; i < PQntuples(m_pRes); i++)
    {
        deb("::exportByteArrayToFile, fetching. --> Row: "+GString(i));
        out = "";
        for (int j = 0; j < colCount; j++)
        {
            deb("::exportByteArrayToFile, fetching. Col: "+GString(j));
            if( PQgetisnull(m_pRes, i, j) )data = "NULL";
            else if( isNumType(j+1) ) data = PQgetvalue(m_pRes, i, j);
            else if ( isXMLCol(j+1) && xmlAsFile == 0 )
            {
                deb("::exportByteArrayToFile, fetching. Col: "+GString(j)+ " is XML");
                data = "'"+GString(PQgetvalue(m_pRes, i, j)).removeAll('\n')+"'";
                deb("::exportByteArrayToFile, fetching. Col: "+GString(j)+ " is XML, data fetched");
            }
            else if( isXMLCol(j+1) && xmlAsFile )
            {
                GFile ba_file(targetFile+"_PMF_"+GString(i)+":"+GString(j), GF_OVERWRITE);
                data = "'"+GString(PQgetvalue(m_pRes, i, j))+"'";
                ba_file.addLine(data);
                data = targetFile+"_PMF_"+GString(i)+":"+GString(j);
            }
            else if( isLOBCol(j+1) && byteaAsFile && exportLobs )
            {
//                GStuff::writeHexToBinFile(GString(PQgetvalue(m_pRes, i, j)), targetFile+"_PMF_"+GString(i)+":"+GString(j));
//                data = targetFile+"_PMF_"+GString(i)+":"+GString(j);
                GFile ba_file(targetFile+"_PMF_"+GString(i)+":"+GString(j), GF_OVERWRITE);
                data = "'"+GString(PQgetvalue(m_pRes, i, j))+"'";
                ba_file.addLine(data);
                data = targetFile+"_PMF_"+GString(i)+":"+GString(j);
            }
            else if( isLOBCol(j+1) && !exportLobs ) data ="NULL";
            else data = "'"+GString(PQgetvalue(m_pRes, i, j))+"'";
            out += data + delim;
        }
        deb("::exportByteArrayToFile, row "+GString(i)+ " done.");
        out = out.stripTrailing(delim);
        trgFile.addLine(out);
        deb("::exportByteArrayToFile, row "+GString(i)+ " done, line added to trgFile");
    }
    deb("::exportByteArrayToFile, rows done.");
    PQclear(m_pRes);
    m_pRes = PQexec(m_pgConn, "CLOSE myCRS");
    PQclear(m_pRes);

    m_pRes = PQexec(m_pgConn, "END");
    return sqlError();
}


GString postgres::exportBlob(unsigned int oid, GString target)
{
    m_pRes = PQexec(m_pgConn, "begin");
    PQclear(m_pRes);
    int erc = lo_export(m_pgConn, oid, target);
    m_pRes = PQexec(m_pgConn, "end");
    if( erc == 1 ) return "";
    return sqlError();
}

long postgres::importBlob(GSeq <GString> *fileSeq)
{
    if( fileSeq->numberOfElements() == 0 ) return -1;
    Oid         lobjOid;
    m_pRes = PQexec(m_pgConn, "begin");
    PQclear(m_pRes);
    lobjOid = lo_import(m_pgConn, fileSeq->elementAtPosition(1));
    m_pRes = PQexec(m_pgConn, "end");
    PQclear(m_pRes);
    return lobjOid;
}

long postgres::uploadBlob(GString cmd, GSeq <GString> *fileSeq, GSeq <long> *lobType)
{    
    deb("::uploadBlob, cmd: "+cmd);

    if( lobType->elementAtPosition(1) == PSTGRS_LOB)
    {
        return importBlob(fileSeq);
    }

    const char   *const *fileBuf = new char* [fileSeq->numberOfElements()];
    int  *const fileSize = new int[fileSeq->numberOfElements()];

    int rc = 0;
    for( int i = 1; i <= fileSeq->numberOfElements(); ++i )
    {
        int pos = cmd.indexOf("?");
        cmd = cmd.replaceAt(pos, " ");

        if( lobType->elementAtPosition(i) == PSTGRS_BYTEA )
        {
            //if( sqlType(colName) == PSTGRS_BYTEA ) cmd = cmd.change(colName, "encode("+colName+",'escape')");
            rc += loadFileIntoBuf(fileSeq->elementAtPosition(i),  (char**) &(fileBuf[i-1]), &(fileSize[i-1]));
            cmd = cmd.insert("decode($"+GString(i)+", 'base64')", pos);
        }
        else
        {
            rc += loadFileIntoBufNoConvert(fileSeq->elementAtPosition(i),  (char**) &(fileBuf[i-1]), &(fileSize[i-1]));
            cmd = cmd.insert("$"+GString(i)+"::XML", pos);
        }
    }
//    if( rc )
//    {
//        delete [] fileBuf;
//        delete [] fileSize;
//        m_iLastSqlCode = 1;
//        return 1;
//    }
    m_pRes = PQexecParams(m_pgConn, cmd, fileSeq->numberOfElements(), NULL, fileBuf, fileSize, NULL, 0);
    GString s = PQresultStatus(m_pRes);
    for( int i = 0; i < fileSeq->numberOfElements(); ++i )
    {
        if(!rc) delete [] fileBuf[i];
    }

    delete [] fileBuf;
    delete [] fileSize;

    printf("PQresultStatus(res): %i\n", PQresultStatus(m_pRes));
    if (PQresultStatus(m_pRes) != PGRES_COMMAND_OK) return -1;
    m_iLastSqlCode = 0;
    return 0;
}

GString postgres::descriptorToFile( GString cmd, GString &blobFile, int * outSize )
{
    deb("::descriptorToFile, cmd: "+cmd);
    if(cmd.subString(1, 10) == "SAVEPGSQL ")
    {
        GSeq <GString> list = cmd.split(' ');
        int oid = list.elementAtPosition(2).asInt();
        return exportBlob(oid, blobFile);
    }
    *outSize = 0;
    postgres ps(*this);
    GSeq<GString> cmdSeq = cmd.split(' ');
    if( cmdSeq.numberOfElements() < 2) return "Invalid sqlCmd: "+cmd;

    GString colName = cmdSeq.elementAtPosition(2);
    if( colName == "*" ) return "Invalid column";

    if( sqlType(colName) == PSTGRS_BYTEA ) cmd = cmd.change(colName, "translate(encode("+colName+",'base64'), E'\n', '')");
    //if( sqlType(colName) == PSTGRS_BYTEA ) cmd = cmd.change(colName, "translate(encode("+colName+",'hex'), E'\n', '')");
    if( sqlType(colName) == PSTGRS_XML ) cmd = cmd.change(colName, "cast("+colName+" as text)");

    //if( sqlType(colName) == PSTGRS_BYTEA ) cmd = cmd.change(colName, "encode("+colName+",'hex')");
    //if( sqlType(colName) == PSTGRS_BYTEA ) cmd = cmd.change(colName, "encode("+colName+",'escape')");

    //GString err = ps.initAllInternal(cmd, 0, 0, 0);
    GString err = ps.initAllInternal(cmd);
    if( ps.numberOfColumns() > 1 ) return "Invalid result (colCount > 1) for  '"+cmd+"'";
    if( err.length() ) return err;

    GString raw = ps.rowElement(1,1).strip("'");

//    This failes with really big bytea
//    if( sqlType(colName) == PSTGRS_BYTEA )
//    {
//        GStuff::writeHexToBinFile(raw, blobFile);
//    }
//    else
//    {
//        FILE * f;
//        f = fopen(blobFile, "wb");
//        *outSize = fwrite((char*) raw, 1, raw.length(), f);
//        fclose(f);
//    }
    //If data was fetched as base64, use this:
    FILE * f;
    f = fopen(blobFile, "wb");
    if( sqlType(colName) == PSTGRS_BYTEA )
    {
        std::string ret = GStuff::base64_decode((char*) raw);
        *outSize = fwrite((char*) ret.c_str(),1,ret.size(),f);
    }
    else *outSize = fwrite((char*) raw, 1, raw.length(), f);
    //printf("Raw: %s<--\n", (char*) raw);
    fclose(f);

    return sqlError();
}

int postgres::writeToFile(GString fileName, GString data, int len)
{
    FILE * f;
    f = fopen(fileName, "ab");
    int written = fwrite((char*) data,1,len,f);
    fclose(f);
    return written;
}

signed long postgres::getCost()
{
    return -1;
}

int postgres::simpleColType(int i)
{
    if( i < 1 || (unsigned long)i > sqlTypeSeq.numberOfElements() ) return CT_UNKNOWN;

    if(isXMLCol(i)) return CT_XML;
    if(isLOBCol(i)) return CT_BLOB;
    if(isNumType(i)) return CT_INTEGER;
    if(isDateTime(i)) return CT_DATE;
    return CT_STRING;
}

int postgres::isLOBCol(int i)
{
    if( i < 1 || (unsigned long)i > sqlTypeSeq.numberOfElements() ) return 0;
    switch(sqlTypeSeq.elementAtPosition(i))
    {
    case PSTGRS_BYTEA:
        return 1;

    }
    return 0;
}

int postgres::isNullable(int i)
{
    if( i < 1 || (unsigned long)i > sqlIndVarSeq.numberOfElements() ) return 1;
    return sqlIndVarSeq.elementAtPosition(i);
}


int postgres::isFixedChar(int i)
{
    if( i < 1 || (unsigned long)i > sqlTypeSeq.numberOfElements() ) return 0;
    return 0;
}

int postgres::getDataBases(GSeq <CON_SET*> *dbList)
{
    this->initAll("SELECT datname FROM pg_database order by datname");
    CON_SET * pCS;
    for( int i = 1; i <= this->numberOfRows(); ++i )
    {        
        pCS = new CON_SET;
        pCS->DB = this->rowElement(i, 1);
        deb("getDataBases, got "+pCS->DB);
        pCS->Host = m_strNode;
        pCS->Type = _POSTGRES;
        pCS->Port = m_strPort;
        pCS->UID = m_strUID;
        pCS->PWD = m_strPWD;
        pCS->CltEnc = "";
        dbList->add(pCS);
    }
	deb("getDataBases done");
    return 0;    
}


void postgres::setCurrentDatabase(GString db)
{
    m_strCurrentDatabase = db;
    //this->initAll("use "+m_strCurrentDatabase);
}
GString postgres::currentDatabase()
{
    return m_strCurrentDatabase;
}

/*****************************************************************************
*
* PRIVATE METHOD
*
*****************************************************************************/
void postgres::resetAllSeq()
{
    GRowHdl * aLine;
    hostVarSeq.removeAll();
    sqlTypeSeq.removeAll();
    sqlVarLengthSeq.removeAll();
    sqlIndVarSeq.removeAll();
    sqlForBitSeq.removeAll();
	sqlBitSeq.removeAll();
    sqlLenSeq.removeAll();
    simpleColTypeSeq.removeAll();
    m_iNumberOfRows   = 0;
    m_iNumberOfColumns = 0;

    while( !allRowsSeq.isEmpty() )
    {
      aLine = allRowsSeq.firstElement();
      //aLine->deleteLine();
      delete aLine;
      allRowsSeq.removeFirst();
    }
}
int postgres::getColSpecs(GString table, GSeq<COL_SPEC*> *specSeq)
{

    GString tabSchema = this->tabSchema(table, 1);
    GString tabName   = this->tabName(table, 1);
    COL_SPEC *cSpec;

    this->setCLOBReader(1); //This will read any CLOB into fieldData

    GString cmd = "SELECT column_name, data_type, character_maximum_length, numeric_precision, numeric_scale, is_nullable, column_default, is_identity, "
                  "identity_generation, identity_start, identity_increment, identity_maximum, identity_minimum, identity_cycle, is_updatable "
                  "FROM information_schema.COLUMNS WHERE TABLE_SCHEMA='"+tabSchema+"' AND table_name='"+tabName+"'";
    deb("::getColSpecs: "+cmd);

    this->initAll(cmd);




    for( unsigned i=1; i<=this->numberOfRows(); ++i )
    {
        cSpec = new COL_SPEC;
        cSpec->init();

        cSpec->ColName = this->rowElement(i,1).strip("'");

        /* colType CHARACTER (from syscat.tables) needs to be truncated to CHAR */
        cSpec->ColType = this->rowElement(i,2).strip("'");
        if( cSpec->ColType == "CHARACTER" ) cSpec->ColType = "CHAR";


        /* SMALLINT, INTEGER, BIGINT, DOUBLE */
        if( cSpec->ColType.occurrencesOf("int") > 0 ||
                cSpec->ColType.occurrencesOf("double") > 0 ||
                cSpec->ColType.occurrencesOf("float") > 0 ||
                cSpec->ColType.occurrencesOf("date") > 0 ||
                cSpec->ColType.occurrencesOf("serial") > 0 ||
                cSpec->ColType.occurrencesOf("box") > 0 ||
                cSpec->ColType.occurrencesOf("boolean") > 0 ||
                cSpec->ColType.occurrencesOf("json") > 0 ||
                cSpec->ColType.occurrencesOf("line") > 0 ||
                cSpec->ColType.occurrencesOf("time"))
        {
            cSpec->Length = "N/A";
        }
        /* DECIMAL: (LENGTH, SCALE) */
        else if( cSpec->ColType.occurrencesOf("decimal") || cSpec->ColType.occurrencesOf("numeric") )
        {
            cSpec->Length = this->rowElement(i,3)+", "+this->rowElement(i,4);
        }
        else if( cSpec->ColType.occurrencesOf("xml") || cSpec->ColType.occurrencesOf("bytea") )
        {
            cSpec->Length = "N/A";
        }
        else
        {
            cSpec->Length = this->rowElement(i,3).strip("'");
        }
        if( this->rowElement(i, 8) == "'YES'" )
        {
            cSpec->Identity = "Y";
            cSpec->Misc = "GENERATED "+this->rowElement(i, 9).strip("\'")+" AS IDENTITY (START WITH "+this->rowElement(i, 10).strip("\'")+" INCREMENT BY "+this->rowElement(i, 11).strip("\'")+")";
        }


        /* check for NOT NULL, DEFAULT */
        if( this->rowElement(i,6) == "'NO'" )
        {
            cSpec->Nullable = "NOT NULL";
        }
        if( !this->isNull(i, 7) )
        {
            cSpec->Default = this->rowElement(i,7);
        }
        specSeq->add(cSpec);

    }

    this->setCLOBReader(0);
    return 0;
}

int postgres::getIdentityColParams(GString table, int *seed, int * incr)
{
    postgres * tmp = new postgres(*this);
    tmp->initAll("select IDENT_SEED ( '"+table+"' )");
    *seed = tmp->rowElement(1,1).asInt();

    tmp->initAll("select IDENT_INCR ( '"+table+"' )");
    *incr = tmp->rowElement(1,1).asInt();

    delete tmp  ;
	return 0;
}


int postgres::getTriggers(GString table, GString *text)
{
    *text = "";
    GSeq <GString> trgSeq = getTriggerSeq(table);
    for(int i = 1; i <= trgSeq.numberOfElements(); ++i)
    {
        *text += "\n-- Trigger #"+GString(i)+":\n"+trgSeq.elementAtPosition(i);
    }
    return 0;
}

GSeq <GString> postgres::getTriggerSeq(GString table)
{
    deb("getTriggerSeq start");
    GString tabSchema = this->tabSchema(table, 1);
    GString tabName   = this->tabName(table, 1);
    GString trig;

    GSeq <GString> triggerSeq;

    this->setCLOBReader(1); //This will read any CLOB into fieldData

    GString cmd = "select trigger_schema, trigger_name, action_timing, event_manipulation, action_statement "
            "from information_schema.triggers where event_object_schema='"+tabSchema+"' and event_object_table='"+tabName+"' order by action_order";

    this->initAll(cmd);

    deb("getTriggers, cmd: "+cmd);
    for( unsigned long i = 1; i <= this->numberOfRows(); ++i )
    {
        trig = "CREATE TRIGGER "+this->rowElement(i, 1).strip("'")+"."+this->rowElement(i, 1).strip("'")+" "+this->rowElement(i, 3).strip("'")+" ";
        trig += this->rowElement(i, 4).strip("'")+" ON "+table+" "+this->rowElement(i, 5).strip("'");
        triggerSeq.add(trig);
    }
    this->setCLOBReader(0);
    return triggerSeq;
}

GSeq <IDX_INFO*> postgres::getIndexeInfo(GString table)
{
    GSeq <IDX_INFO*> indexSeq;
    GString cmd = "SELECT contype, connamespace::regnamespace, conrelid::regclass, attname, conname, indexdef "
          "FROM pg_attribute a JOIN pg_constraint c ON attrelid = conrelid AND attnum = ANY (conkey) "
          "JOIN pg_catalog.pg_indexes i on schemaname='"+tabSchema(table, 1)+"' and tablename='"+tabName(table, 1)+"' "
          "and indexname=c.conname "
          "where connamespace='"+tabSchema(table)+"'::regnamespace and conrelid ='"+tabSchema(table)+"."+tabName(table)+"'::regclass "
          //"where connamespace='"+tabSchema(table)+"'::regnamespace and conrelid ='"+tabName(table)+"'::regclass "
          "and contype != 'f' "
          "order by c.contype, c.conname";

//    cmd ="select *,idx.relname as index_name, insp.nspname as index_schema, tbl.relname as table_name, tnsp.nspname as table_schema,"
//         "pgi.indisunique,pgi.indisprimaryfrom pg_index pgi"
//         "join pg_class idx on idx.oid = pgi.indexrelid  join pg_namespace insp on insp.oid = idx.relnamespace "
//         "join pg_class tbl on tbl.oid = pgi.indrelid  join pg_namespace tnsp on tnsp.oid = tbl.relnamespace  where "
//         "tnsp.nspname = '"+tabSchema(table)+"' and tbl.relname ='"+tabName(table)+"'";
    getIndexInfo(cmd, &indexSeq, table);


    //foreign keys
//    cmd = "SELECT conrelid::regclass AS table_from, conname, pg_get_constraintdef(oid) FROM   pg_constraint "
//          "WHERE  contype IN ('f') AND    connamespace = '"+tabSchema(table)+"'::regnamespace "
//          "ORDER  BY conrelid::regclass::text, contype DESC;";
//    getIndexInfo(cmd, &indexSeq);


    cmd = "SELECT contype,'"+tabSchema(table)+"'::regnamespace, conrelid::regclass AS table_from, 'N/A',conname, pg_get_constraintdef(oid) FROM   pg_constraint "
          "WHERE  contype IN ('f') AND connamespace = '"+tabSchema(table)+"'::regnamespace "
          "and (conrelid::regclass::text = '"+table+"' OR conrelid::regclass::text = '"+tabName(table, 1)+"') "
          "ORDER  BY conrelid::regclass::text, contype DESC";

    getIndexInfo(cmd, &indexSeq, table);

/*
    cmd = "SELECT contype,connamespace::regnamespace, conrelid::regclass, attname, conname, pg_get_constraintdef(c.oid) as cmd "
           "FROM pg_attribute a JOIN pg_constraint c ON attrelid = conrelid AND attnum = ANY (conkey) "
           "where connamespace='"+tabSchema(table)+"'::regnamespace and conrelid ='"+tabName(table)+"'::regclass "
           "and contype = 'f' "
           "order by c.contype, c.conname";
    getIndexInfo(cmd, &indexSeq);
*/

    cmd = "select 's', '"+tabSchema(table, 1)+"', '', 'N/A', indexname, indexdef from pg_catalog.pg_indexes where schemaname='"+tabSchema(table, 1)+"' and tablename='"+tabName(table,1)+"'";
    getIndexInfo(cmd, &indexSeq);
    return indexSeq;

}

void postgres::getIndexInfo(GString cmd, GSeq <IDX_INFO*> *indexSeq, GString tableName)
{

    GString err = this->initAll(cmd);
    deb("::getIndexInfo, cmd: "+cmd);
    deb("::getIndexeInfo, err: "+err);
    deb("::getIndexInfo, found: "+GString(this->numberOfRows()));

    GString indName = "";
    IDX_INFO * pIdx;
    for(int  i=1; i<=(int)this->numberOfRows(); ++i )
    {
        if( this->rowElement(i, 5).strip("'") == indName )
        {
            pIdx = indexSeq->lastElement();
            pIdx->Columns += ", "+this->rowElement(i, 4).strip("'");
            continue;
        }
        pIdx = new IDX_INFO;
        pIdx->Iidx = GString(i);
        pIdx->Schema = this->rowElement(i, 2).strip("'").strip('\"');
        pIdx->Name = this->rowElement(i, 5).strip("'");
        if( this->rowElement(i, 1).strip("'") == "p") pIdx->Type = DEF_IDX_PRIM;
        else if( this->rowElement(i, 1).strip("'") == "f") pIdx->Type = DEF_IDX_FORKEY;
        else if( this->rowElement(i, 1).strip("'") == "u") pIdx->Type = DEF_IDX_UNQ;
        else pIdx->Type = DEF_IDX_DUPL;
        if( this->rowElement(i, 4).strip("'") == "N/A")
        {
            pIdx->Columns = getColumnsFromCreateStmt(this->rowElement(i, 6).strip("'"));
        }
        else pIdx->Columns = this->rowElement(i, 4).strip("'");
        pIdx->CreateTime = "N/A";
        pIdx->StatsTime = "N/A";
        pIdx->IsDisabled = "N/A";
        pIdx->DeleteRule = "N/A";
        pIdx->StatsTime = "N/A";
        if( pIdx->Type == DEF_IDX_FORKEY ) pIdx->Stmt = "ALTER TABLE "+tableName+ " ADD CONSTRAINT "+this->rowElement(i, 5).strip("'")+" "+this->rowElement(i, 6).strip("'");
        else pIdx->Stmt = this->rowElement(i, 6).strip("'");
        indName = this->rowElement(i, 5).strip("'");

        addToIdxSeq(indexSeq, pIdx);
        deb("::fillIndexView, i: "+GString(i)+", currIDx: "+GString(i));
    }
}

void postgres::addToIdxSeq(GSeq <IDX_INFO*> *indexSeq, IDX_INFO *pIdx)
{
    for( int i = 1; i <= indexSeq->numberOfElements(); ++i )
    {
        IDX_INFO *exIdx = indexSeq->elementAtPosition(i);
        if( exIdx->Name == pIdx->Name ) return;
    }
    indexSeq->add(pIdx);
}

GString postgres::getColumnsFromCreateStmt(GString stmt)
{
    if( stmt.occurrencesOf("(") == 0  ) return "N/A";
    //if( stmt.occurrencesOf("(") > 1  ) return "[...]";
    stmt = stmt.strip('\'');
    stmt = stmt.remove(1, stmt.indexOf("("));
    stmt = stmt.remove(stmt.lastIndexOf(")"), 1);
    stmt = stmt.removeAll('\"');
//    GSeq<GString> cols = stmt.split(',');
//    for(int i = 1; i <= cols.numberOfElements(); ++i )
//    {

//    }
    return stmt;
}

/*
GSeq <IDX_INFO*> postgres::getIndexeInfo(GString table)
{                

//    this->initAll("use "+m_strCurrentDatabase);

    GString cmd = "select schemaname, indexname, indexdef from pg_catalog.pg_indexes "
     "where schemaname ='"+tabSchema(table)+"' and  tablename ='"+tabName(table)+"' ";

    m_iReadClobData = 1;
    this->initAll(cmd);
    deb("::getIndexeInfo, cmd: "+cmd);
    deb("::getIndexeInfo, found: "+GString(this->numberOfRows()));

    GSeq <IDX_INFO*> indexSeq;

    IDX_INFO * pIdx;
    for(int  i=1; i<=(int)this->numberOfRows(); ++i )
    {
        pIdx = new IDX_INFO;        
        deb("::getIndexeInfo, i: "+GString(i));
        GString cols = this->rowElement(i, 3).strip("'");
        cols = cols.subString(cols.indexOf("(")+1, cols.length()).strip();
        cols = cols.subString(1, cols.indexOf(")")-1);


        pIdx->Iidx = GString(i);
        pIdx->Schema = this->rowElement(i, 1).strip("'");
        pIdx->Name = this->rowElement(i, 2).strip("'");
        pIdx->Columns = cols;
        pIdx->CreateTime = "N/A";
        pIdx->StatsTime = "N/A";
        pIdx->IsDisabled = "N/A";
        pIdx->DeleteRule = "N/A";
        pIdx->StatsTime = "N/A";
        pIdx->Stmt = this->rowElement(i, 3).strip("'");

        indexSeq.add(pIdx);
        deb("::fillIndexView, i: "+GString(i)+", currIDx: "+GString(i));
    }
    m_iReadClobData = 0;

    //ForeignKeys
    cmd = "SELECT  conrelid::regclass, conname AS foreign_key, pg_get_constraintdef(oid) as cmd FROM   pg_constraint "
          "WHERE  contype = 'f' AND    connamespace = '"+tabSchema(table)+"'::regnamespace  "
          "AND conrelid::regclass='"+tabName(table)+"'::regclass ORDER  BY conrelid::regclass::text, contype DESC";
    this->initAll(cmd);
    for(int  i=1; i<=(int)this->numberOfRows(); ++i )
    {
        pIdx = new IDX_INFO;
        pIdx->Type        = DEF_IDX_FORKEY;

        deb("::getIndexeInfo, i: "+GString(i));
        pIdx->Name = this->rowElement(i,2).strip("'");
        pIdx->Stmt = this->rowElement(i,3).strip("'");
        indexSeq.add(pIdx);
    }

    //PrimaryKeys
    cmd = "SELECT pg_attribute.attname, format_type(pg_attribute.atttypid, pg_attribute.atttypmod) "
          "FROM pg_index, pg_class, pg_attribute, pg_namespace  WHERE pg_class.oid = '"+tabName(table)+"'::regclass AND "
          "indrelid = pg_class.oid AND nspname = '"+tabSchema(table)+"' AND pg_class.relnamespace = pg_namespace.oid AND "
          "pg_attribute.attrelid = pg_class.oid AND pg_attribute.attnum = any(pg_index.indkey) AND indisprimary";

    this->initAll(cmd);
    for(int  i=1; i<=(int)this->numberOfRows(); ++i )
    {
        pIdx = new IDX_INFO;
        pIdx->Type = DEF_IDX_PRIM;
        pIdx->Stmt = "PRIMARY KEY: "+this->rowElement(i, 1)+" (Type: "+this->rowElement(i,2)+")";
        indexSeq.add(pIdx);
    }
    return indexSeq;
}
*/

GString postgres::getChecks(GString, GString )
{
    return "";
}

GSeq <GString> postgres::getChecksSeq(GString, GString)
{
    GSeq <GString> aSeq;
    return aSeq;
}




GString postgres::getForeignKeyStatement(GString table, GString foreignKeyName )
{
    //this->initAll("use "+m_strCurrentDatabase);
    foreignKeyName = foreignKeyName.strip("'");

    GString out;
    GString cmd =
    "SELECT OBJECT_NAME(f.parent_object_id) t1, "               //TableName,
    " COL_NAME(fc.parent_object_id,fc.parent_column_id) t2, "   //ColName
    " OBJECT_NAME(f.referenced_object_id) t3, "                 //referenced table
    " OBJECT_NAME(f.object_id) t4, "                            //FKey name
    " COL_NAME(fc.referenced_object_id,fc.referenced_column_id) t5," //referenced col
    " (f.delete_referential_action) t6,"                          //on delete
    " (f.delete_referential_action_desc) t7,"                    //"CASCADE"
    " (f.update_referential_action) t8,"
    " (f.update_referential_action_desc) t9	"
    " FROM sys.foreign_keys AS f INNER JOIN sys.foreign_key_columns AS fc "
          " ON f.OBJECT_ID = fc.constraint_object_id INNER JOIN "
          " sys.tables t  ON t.OBJECT_ID = fc.referenced_object_id "
          " WHERE    OBJECT_NAME (f.parent_object_id) = '"+tabName(table)+"' AND OBJECT_NAME(f.object_id)='"+foreignKeyName+"'";

    GString keyName, cols, refCols;
    deb("::getForeignKeys, cmd: "+cmd);
    this->initAll(cmd);
    out = "";
    if( keyName == this->rowElement(1, 4).strip("'") || keyName == "")
    {
        cols += this->rowElement(1, 2).strip("'") +", ";
        refCols += this->rowElement(1, 5).strip("'") +", ";
        keyName = this->rowElement(1, 4).strip("'");
    }
    if( keyName != this->rowElement(1, 4).strip("'") || 1 == this->numberOfRows() )
    {
        out += "ALTER TABLE "+table+" ADD CONSTRAINT "+this->rowElement(1, 4).strip("'")+" FOREIGN KEY ("+cols.stripTrailing(", ")+") REFERENCES "+
                this->rowElement(1, 3).strip("'")+"("+refCols.stripTrailing(", ")+")";
        if( this->rowElement(1, "t6").strip("'").asInt() == 1 ) out += " ON DELETE "+this->rowElement(1, "t7").strip("'");
        if( this->rowElement(1, "t8").strip("'").asInt() == 1 ) out += " ON UPDATE "+this->rowElement(1, "t9").strip("'");
        keyName = "";
    }
    return out.removeButOne() + ";";
}

int postgres::hasUniqueConstraint(GString tableName)
{

    GSeq <GString>  colSeq;
    if( this->getUniqueCols(tableName, &colSeq)) return 0;
    return colSeq.numberOfElements();
}


int postgres::getUniqueCols(GString table, GSeq <GString> * colSeq)
{

    deb("::getUniqueCols start");
    GString cmd = "SELECT distinct(column_name) FROM information_schema.constraint_column_usage WHERE table_schema='"+tabSchema(table, 1)+"' AND "
                   "table_name='"+tabName(table, 1)+"'";

    deb("getUniqueCols, cmd: "+cmd);
    this->initAll(cmd);
    deb("getUniqueCols, cols: "+GString(this->numberOfRows()));
    if( this->numberOfRows() == 0 ) return 1;
    for(int i = 1; i <= this->numberOfRows(); ++i )
    {
        colSeq->add(this->rowElement(i, 1).strip("'"));
    }
    return 0;
}

GString postgres::tabName(GString table, int removeDoubleQuotes)
{
    if( removeDoubleQuotes) table = table.removeAll('\"');
    deb("postgres::tabName: "+table);
    if( table.occurrencesOf(".") != 2 && table.occurrencesOf(".") != 1) return "@ErrTabString";
    if( table.occurrencesOf(".") == 2 ) table = table.remove(1, table.indexOf("."));
    return table.subString(table.indexOf(".")+1, table.length()).strip();
}
GString postgres::context(GString table)
{
    table.removeAll('\"');
    deb("postgres::context for "+table);
    if( table.occurrencesOf(".") != 2 ) return "";
    return table.subString(1, table.indexOf(".")-1);
}

GString postgres::tabSchema(GString table, int removeDoubleQuotes)
{
    if( removeDoubleQuotes) table.removeAll('\"');
    deb("postgres::tabSchema: "+table);
    if( table.occurrencesOf(".") != 2 && table.occurrencesOf(".") != 1) return "@ErrTabString";
    if( table.occurrencesOf(".") == 2 ) table.remove(1, table.indexOf("."));
    return table.subString(1, table.indexOf(".")-1);
}

void postgres::createXMLCastString(GString &xmlData)
{
	deb("::createXMLCastString called");
	xmlData = " cast('"+(xmlData)+"' as xml)";
}

int postgres::isBinary(unsigned long row, int col)
{
    if( row < 1 || row > allRowsSeq.numberOfElements() ) return 0;
    GRowHdl *aRow;
    aRow = allRowsSeq.elementAtPosition(row);
    if( row < 1 || (unsigned long) col > aRow->elements() ) return 0;
    return aRow->rowElement(col)->isBinary;
}

int  postgres::isNull(unsigned long row, int col)
{
    if( row < 1 || row > allRowsSeq.numberOfElements() ) return 0;
    GRowHdl *aRow;
    aRow = allRowsSeq.elementAtPosition(row);
    if( col < 1 || (unsigned long) col > aRow->elements() ) return 0;
    deb("::isNull: "+GString(aRow->rowElement(col)->isNull)+", row: "+GString(row)+", col: "+GString(col));
    return aRow->rowElement(col)->isNull;
}

int postgres::isTruncated(unsigned long row, int col)
{
    if( row < 1 || row > allRowsSeq.numberOfElements() ) return 0;
    GRowHdl *aRow;
    aRow = allRowsSeq.elementAtPosition(row);
    if( row < 1 || (unsigned long) col > aRow->elements() ) return 0;
    return aRow->rowElement(col)->isTruncated;
}
void postgres::deb(GString fnName, GString txt)
{
    if( m_pGDB ) m_pGDB->debugMsg("postgres", m_iMyInstance, "::"+fnName+" "+txt);
}

void postgres::deb(GString text)
{
    if( m_pGDB ) m_pGDB->debugMsg("postgres", m_iMyInstance, text);
}
void postgres::setTruncationLimit(int limit)
{
    //Setting m_iTruncationLimit to 0 -> no truncation
    m_iTruncationLimit = limit;
}
int postgres::uploadLongBuffer( GString cmd, GString data, int isBinary )
{
    PMF_UNUSED(cmd);
    PMF_UNUSED(data);
    PMF_UNUSED(isBinary);
    return -1;
}
GString postgres::cleanString(GString in)
{
    if( in.length() < 2 ) return in;
    if( in[1UL] == '\'' && in[in.length()] == '\'' ) return in.subString(2, in.length()-2);
    return in;
}
int postgres::isLongTypeCol(int i)
{
    if( i < 1 || (unsigned long)i > sqlLongTypeSeq.numberOfElements() ) return 0;
    return sqlLongTypeSeq.elementAtPosition(i);
}
void postgres::getResultAsHEX(int asHex)
{
    PMF_UNUSED(asHex);
//TODO
}
void postgres::setReadUncommitted(short readUncommitted)
{
    m_iReadUncommitted = readUncommitted;
}

void postgres::setGDebug(GDebug *pGDB)
{
    m_pGDB = pGDB;
}
void postgres::setDatabaseContext(GString context)
{
    deb("setting context: "+context);
    m_strCurrentDatabase = context;
    //this->initAll("use "+m_strCurrentDatabase);
}

int postgres::exportAsTxt(int mode, GString sqlCmd, GString table, GString outFile, GSeq <GString>* startText, GSeq <GString>* endText, GString *err)
{
    PMF_UNUSED(mode);
    deb("exportAsTxt start. cmd: "+sqlCmd+", table: "+table+", out: "+outFile);
    GFile f(outFile, GF_APPENDCREATE);
    if( !f.initOK() )
    {
        *err = "Could not open file "+outFile+" for writing.";
        return 1;
    }
    GSeq <GString> aSeq;
    aSeq.add("--------------------------------------- ");
    aSeq.add("-- File created by Poor Man's Flight (PMF) www.leipelt.de");
    aSeq.add("-- Statement: "+sqlCmd);
    aSeq.add("-- ");
    aSeq.add("-- NOTE: LOBs will be given as NULL. ");
    aSeq.add("--------------------------------------- ");
    aSeq.add("");
    aSeq.add("-- Use the following line to set the target database:");
    aSeq.add("-- USE [DataBase] ");
    aSeq.add("");

	aSeq.add("-- SET IDENTITY_INSERT "+table+" ON");

    for(unsigned long i = 1; i <= startText->numberOfElements(); ++i) aSeq.add(startText->elementAtPosition(i));

//    *err = this->readRowData(sqlCmd, 0, 0, 1);
    if(err->length()) return this->sqlCode();

    remove(outFile);


    GString columns = "INSERT INTO "+table+"(";
    for(unsigned long i = 1; i <= this->numberOfColumns(); ++i)
    {
        columns += "["+this->hostVariable(i)+"], ";
    }
    columns = columns.stripTrailing(", ")+") VALUES (";

    deb("exportAsTxt cols is: "+columns);

    GString out, data;
    for(unsigned long i = 1; i <= this->numberOfRows(); ++i)
    {
        out = "";
        for(unsigned long j = 1; j <= this->numberOfColumns(); ++j)
        {
            data = this->rowElement(i, j);
            if( this->isNumType(j))  out += cleanString(data)+", ";
			else if( data == "NULL" )out += data+", ";
            else if( data.occurrencesOf("@DSQL@") ) out +="NULL, ";
            else out += data+", ";
        }
        out = out.stripTrailing(", ") + ")";
        aSeq.add(columns+out);
        deb("exportAsTxt adding  "+out);
        if( aSeq.numberOfElements() > 1000)
        {
            f.append(&aSeq);
            aSeq.removeAll();
            deb("exportAsTxt appending to "+outFile);
        }
    }
    for(unsigned long i = 1; i <= endText->numberOfElements(); ++i) aSeq.add(endText->elementAtPosition(i));
	aSeq.add("-- SET IDENTITY_INSERT "+table+" OFF");
    f.append(&aSeq);

    return 0;
}
int postgres::deleteTable(GString tableName)
{
    this->initAll("drop table "+tableName);
    this->sqlError();
	return sqlCode();
}


GString postgres::fillChecksView(GString, int)
{
    return "";
}

postgres::RowData::~RowData()
{
    rowDataSeq.removeAll();
}
void postgres::RowData::add(GString data)
{
    rowDataSeq.add(data);
}
GString postgres::RowData::elementAtPosition(int pos)
{
    if( pos < 1 || pos > (int)rowDataSeq.numberOfElements() ) return "";
    return rowDataSeq.elementAtPosition(pos);
}
int postgres::getHeaderData(int pos, GString * data)
{
    if( pos < 1 || pos > (int)headerSeq.numberOfElements()) return 1;
    *data = headerSeq.elementAtPosition(pos);
    return 0;
}

int postgres::getRowData(int row, int col, GString * data)
{
    if( row < 1 || row > (int)rowSeq.numberOfElements() ) return 1;
    if( col < 1 || col > (int)rowSeq.elementAtPosition(row)->numberOfElements() ) return 1;
    *data = rowSeq.elementAtPosition(row)->elementAtPosition(col);
    return 0;
}

unsigned long postgres::RowData::numberOfElements()
{
    return rowDataSeq.numberOfElements();
}
unsigned long postgres::getHeaderDataCount()
{
    return headerSeq.numberOfElements();
}
unsigned long postgres::getRowDataCount()
{
    return rowSeq.numberOfElements();
}

int postgres::isTransaction()
{
    return m_iIsTransaction;
}

void postgres::clearSequences()
{
    RowData *pRowData;
    while( !rowSeq.isEmpty() )
    {
        pRowData = rowSeq.firstElement();
        delete pRowData;
        rowSeq.removeFirst();
    }
	headerSeq.removeAll();
}

GString postgres::getDdlForView(GString tableName)
{       
    this->setCLOBReader(1);
    this->initAll("SELECT VIEW_DEFINITION FROM INFORMATION_SCHEMA.VIEWS WHERE TABLE_SCHEMA='"+ tabSchema(tableName, 1)+"' AND TABLE_NAME='"+tabName(tableName, 1)+"'");
    this->setCLOBReader(1);
    if( this->numberOfRows() == 0 ) return "";
    return "CREATE VIEW "+tabName(tableName)+" AS "+  cleanString(this->rowElement(1, 1).strip());
}

void postgres::setAutoCommmit(int commit)
{

}

int postgres::execByDescriptor( GString cmd, GSeq <GString> *dataSeq)
{
    PMF_UNUSED(cmd);
    PMF_UNUSED(dataSeq);
    return -1;
}
int postgres::execByDescriptor( GString cmd, GSeq <GString> *dataSeq, GSeq <int> *typeSeq,
                                GSeq <short>* sqlVarLengthSeq, GSeq <int> *forBitSeq )
{
    PMF_UNUSED(cmd);
    PMF_UNUSED(dataSeq);
    PMF_UNUSED(typeSeq);
    PMF_UNUSED(sqlVarLengthSeq);
    PMF_UNUSED(forBitSeq);
    return -1;
}
long postgres::uploadBlob(GString cmd, char * buffer, long size)
{
    PMF_UNUSED(cmd);
    PMF_UNUSED(buffer);
    PMF_UNUSED(size);
    return -1;
}

long postgres::retrieveBlob( GString cmd, GString &blobFile, int writeFile )
{
    PMF_UNUSED(cmd);
    PMF_UNUSED(blobFile);
    PMF_UNUSED(writeFile);
    return -1;
}

void postgres::currentConnectionValues(CON_SET * conSet)
{
    conSet->DB = m_strDB;
    conSet->Host = m_strHost;
    conSet->PWD = m_strPWD;
    conSet->UID = m_strUID;
    conSet->Port = m_strPort;
    conSet->CltEnc = m_strEncoding;
    conSet->Type = _POSTGRES;

}

GString postgres::lastSqlSelectCommand()
{
    return m_strLastSqlSelectCommand;;
}

TABLE_PROPS postgres::getTableProps(GString tableName)
{
    TABLE_PROPS tableProps;
    tableProps.init();

    this->initAll("SELECT TABLE_TYPE FROM information_schema.TABLES where TABLE_SCHEMA='"+ tabSchema(tableName, 1)+"' AND TABLE_NAME='"+tabName(tableName, 1)+"'");
    if( rowElement(1,1).strip("'") == "VIEW" ) tableProps.TableType = TYPE_TYPED_VIEW;
    else if( rowElement(1,1).strip("'") == "BASE TABLE" ) tableProps.TableType = TYPE_TYPED_TABLE;
    else if( rowElement(1,1).strip("'") == "SYSTEM VIEW" ) tableProps.TableType = TYPE_TYPED_VIEW;

    return tableProps;
}

int postgres::tableIsEmpty(GString tableName)
{
    GString err = this->initAll("select top 1 * from " + tableName);
    if( this->numberOfRows() == 0 || err.length() ) return 1;
    return 0;
}

GString postgres::addQuotes(GString in)
{
    return "\"" + in + "\"";
}

int postgres::deleteViaFetch(GString tableName, GSeq<GString> * colSeq, int rowCount, GString whereClause)
{
    GString cmd = "DELETE FROM "+tableName +" WHERE (";
    for( int i = 1; i <= colSeq->numberOfElements(); ++i )
    {
        cmd += addQuotes(colSeq->elementAtPosition(i))+",";
    }
    cmd = cmd.stripTrailing(",") + ") in (select ";
    for( int i = 1; i <= colSeq->numberOfElements(); ++i )
    {
        cmd += addQuotes(colSeq->elementAtPosition(i))+",";
    }
    cmd = cmd.stripTrailing(",");
    cmd += " from "+tableName+ " limit 2000)";
    GString err = this->initAll(cmd);
    deb("deleteViaFetch: cmd: "+cmd+"\nErr: "+err);
    if( err.length() ) return sqlCode();
    this->commit();
    return 0;
}

void postgres::setCLOBReader(short readCLOBData )
{
    m_iReadClobData = readCLOBData;
}

/******************************************************************
 *
 *  ifdef: Use QT for some tasks
 *
 *****************************************************************/
#ifdef  QT4_DSQL
GString postgres::getIdenticals(GString table, QWidget* parent, QListWidget *pLB, short autoDel)
{

	GString message = "SELECT * FROM "+table;
	GString retString = "";
	deb("::getIdenticals, cmd: "+message);


    return retString;
}

void postgres::writeToLB(QListWidget * pLB, GString message)
{
    for( int i = 0; i < pLB->count(); ++i )
        if( GString(pLB->item(i)->text()) == message ) return;
    new QListWidgetItem(message, pLB);
}


GString  postgres::fillIndexView(GString table, QWidget* parent, QTableWidget *pWdgt)
{
    PMF_UNUSED(parent);
	//Clear data:
	while( pWdgt->rowCount() ) pWdgt->removeRow(0);
	
	GString id, name, cols, unq, crt, mod, dis;
	int indexID = -1;
    //this->initAll("use "+m_strCurrentDatabase);
	
    GString cmd = "select i.index_id, i.name, c.name,  i.is_primary_key, i.is_unique_constraint,"
			"create_date, modify_date, i.is_disabled "
            "from sys.tables t  inner join sys.schemas s on t.schema_id = s.schema_id "
            "inner join sys.indexes i on i.object_id = t.object_id "
            "inner join sys.index_columns ic on ic.object_id = t.object_id "
            "inner join sys.columns c on c.object_id = t.object_id and ic.column_id = c.column_id "
            "where s.name='"+tabSchema(table, 1)+"' and t.name='"+tabName(table, 1)+"'";
    this->initAll(cmd);
	deb("::fillIndexView, cmd: "+cmd);
    deb("::fillIndexView, found: "+GString(this->numberOfRows()));

    pWdgt->setColumnCount(7);
	
	QTableWidgetItem * pItem;
	int i = 0;
	pItem = new QTableWidgetItem("IndexID"); pWdgt->setHorizontalHeaderItem(i++, pItem);
    pItem = new QTableWidgetItem("Name"); pWdgt->setHorizontalHeaderItem(i++, pItem);
	pItem = new QTableWidgetItem("Columns"); pWdgt->setHorizontalHeaderItem(i++, pItem);
    pItem = new QTableWidgetItem("Type"); pWdgt->setHorizontalHeaderItem(i++, pItem);
	pItem = new QTableWidgetItem("Created"); pWdgt->setHorizontalHeaderItem(i++, pItem);
	pItem = new QTableWidgetItem("Modified"); pWdgt->setHorizontalHeaderItem(i++, pItem);
	pItem = new QTableWidgetItem("IsDisabled"); pWdgt->setHorizontalHeaderItem(i++, pItem);
	
	int row = 0;
    for(int  i=1; i<= (int)this->numberOfRows(); ++i )
    {
		deb("::fillIndexView, i: "+GString(i));
        if( this->rowElement(i, 1).strip("'").asInt() != indexID )
        {
			deb("::fillIndexView, i: "+GString(i)+", indexChg, new: "+GString(indexID));
			if( cols.length() )
			{
				cols = cols.stripTrailing(", ");
				createIndexRow(pWdgt, row++, id, name, cols, unq, crt, mod, dis);
			}
			id   = this->rowElement(i, 1);
			name = this->rowElement(i, 2);
			cols = ""; 
			if( this->rowElement(i, 4) == "'1'" ) unq = "Primary Key";
			else if( this->rowElement(i, 5) == "'1'" ) unq = "Unique Key";
			else unq = "";
			crt = this->rowElement(i, 6);
			mod = this->rowElement(i, 7);
			dis = this->rowElement(i, 8);
            indexID = this->rowElement(i, 1).strip("'").asInt();
			deb("::fillIndexView, i: "+GString(i)+", currIDx: "+GString(id)+", cols: "+cols);
        }
		cols += this->rowElement(i, 3).strip("'")+", ";
		deb("::fillIndexView, i: "+GString(i)+", currIDx: "+GString(id)+", cols: "+cols);
    }
	if( cols.length() )
	{
		cols = cols.stripTrailing(", ");
		createIndexRow(pWdgt, row++, id, name, cols, unq, crt, mod, dis);
		deb("::fillIndexView, final: "+GString(id)+", cols: "+cols);
	}
	return "";
}

GString postgres::setEncoding(GString encoding)
{
    deb("setEncoding, in: "+encoding);
    if( m_strEncoding == encoding ) return "";
    if(!encoding.strip().length()) return "";
    m_strEncoding = encoding;    
    return this->initAll("SET CLIENT_ENCODING TO '"+encoding+"'; ");
}

void postgres::getAvailableEncodings(GSeq<GString> *encSeq)
{
    encSeq->add("BIG5");
    encSeq->add("EUC_CN");
    encSeq->add("EUC_JP");
    encSeq->add("EUC_JIS_2004");
    encSeq->add("EUC_KR");
    encSeq->add("EUC_TW");
    encSeq->add("GB18030");
    encSeq->add("GBK");
    encSeq->add("ISO_8859_5");
    encSeq->add("ISO_8859_6");
    encSeq->add("ISO_8859_7");
    encSeq->add("ISO_8859_8");
    encSeq->add("JOHAB");
    encSeq->add("KOI8R");
    encSeq->add("KOI8U");
    encSeq->add("LATIN1");
    encSeq->add("LATIN2");
    encSeq->add("LATIN3");
    encSeq->add("LATIN4");
    encSeq->add("LATIN5");
    encSeq->add("LATIN6");
    encSeq->add("LATIN7");
    encSeq->add("LATIN8");
    encSeq->add("LATIN9");
    encSeq->add("LATIN10");
    encSeq->add("MULE_INTERNAL");
    encSeq->add("SJIS");
    encSeq->add("SHIFT_JIS_2004");
    encSeq->add("SQL_ASCII");
    encSeq->add("UHC");
    encSeq->add("UTF8");
    encSeq->add("WIN866");
    encSeq->add("WIN874");
    encSeq->add("WIN1250");
    encSeq->add("WIN1251");
    encSeq->add("WIN1252");
    encSeq->add("WIN1253");
    encSeq->add("WIN1254");
    encSeq->add("WIN1255");
    encSeq->add("WIN1256");
    encSeq->add("WIN1257");
    encSeq->add("WIN1258");
}

void postgres::createIndexRow(QTableWidget *pWdgt, int row,
		GString id, GString name, GString cols, GString unq, GString crt, GString mod, GString dis)
{
	if( !id.length() ) return;
	deb("::fillIndexView, createRow, id: "+id);
	int j = 0;
	QTableWidgetItem * pItem;
	pWdgt->insertRow(pWdgt->rowCount());
	pItem = new QTableWidgetItem(); pItem->setText(id);   pWdgt->setItem(row, j++, pItem);
	pItem = new QTableWidgetItem(); pItem->setText(name); pWdgt->setItem(row, j++, pItem);
	pItem = new QTableWidgetItem(); pItem->setText(cols); pWdgt->setItem(row, j++, pItem);
	pItem = new QTableWidgetItem(); pItem->setText(unq);  pWdgt->setItem(row, j++, pItem);
	pItem = new QTableWidgetItem(); pItem->setText(crt);  pWdgt->setItem(row, j++, pItem);
	pItem = new QTableWidgetItem(); pItem->setText(mod);  pWdgt->setItem(row, j++, pItem);
	pItem = new QTableWidgetItem(); pItem->setText(dis);  pWdgt->setItem(row, j++, pItem);

}
#endif
/******************************************************************
 *
 *  END ifdef: Use QT for some tasks
 *
 *****************************************************************/


