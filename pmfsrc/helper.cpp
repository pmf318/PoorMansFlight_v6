#ifndef HELPER_H
#include "helper.h"
#endif


#ifdef MAKE_VC
#include <windows.h>
#include <process.h>
#include "zlib.h"
#else
#include "../zlib/zlib.h"
#endif

#include "pmfdefines.h"

#include<ctime>

#include <QMessageBox>
#include <QTextStream>
#include <QDir>
#include <QSettings>
#include <QErrorMessage>
#include <QCheckBox>
#include <QDomDocument>
#include <QHeaderView>
#include <QDate>
#include <QProcess>
#include <QtGlobal>
#if QT_VERSION > 0x060500
#include <QStyleHints>
#endif

#if QT_VERSION >= 0x060000
#include <QScreen>
#else
#include <QDesktopWidget>
#endif

#ifdef MAKE_VC
#include <io.h>          // open, close, lseek,...
#include <fcntl.h>       // O_RDONLY,...
#include <stdio.h>       // flushall
#include <sys/stat.h>    // stat
#endif

#include "pmfdefines.h"
#include "debugMessenger.hpp"
#include "messenger.hpp"
#include "gstuff.hpp"


#include <gfile.hpp>

/***************************************************
 *  On Win7_64 this code crashes:
 *    QItemSelectionModel* selectionModel = mainLV->selectionModel();
 *    QModelIndexList selected = selectionModel->selectedRows();
 *  on destructing QModelIndexList.
 *  Env: VS2010, Qt 5.3.0
 *
 *  This helper is a workarond.
 */
QModelIndexList Helper::getSelectedRows(QItemSelectionModel *selectionModel)
{
#if QT_VERSION >= 0x050000
    QModelIndexList selected = selectionModel->selectedRows();
    return selected;
#else

    QModelIndexList lstIndex ;

    QItemSelection ranges = selectionModel->selection();
    for (int i = 0; i < ranges.count(); ++i)
    {
        QModelIndex parent = ranges.at(i).parent();
        int right = ranges.at(i).model()->columnCount(parent) - 1;
        if (ranges.at(i).left() == 0 && ranges.at(i).right() == right)
            for (int r = ranges.at(i).top(); r <= ranges.at(i).bottom(); ++r)
                lstIndex.append(ranges.at(i).model()->index(r, 0, parent));
    }
    return lstIndex;
#endif
}

void Helper::msgBox(QWidget *parent, QString title, QString msg )
{
    QMessageBox mbox(QMessageBox::Information, title, msg, QMessageBox::Ok, parent);
    mbox.setText(msg);
    mbox.exec();
}
GString Helper::tempPath()
{
    GString path = QDir::tempPath();
#if defined(MAKE_VC) || defined (__MINGW32__)
    path = path.translate('/', '\\').stripTrailing("\\")+"\\";
    QDir tmpPath(path);
    if (!tmpPath.exists()){
        tmpPath.mkdir(".");
    }
#else
    path = path.stripTrailing("/")+"/";
#endif
    return path;
}

GString Helper::filePath(GString path, GString file)
{
#if defined(MAKE_VC) || defined (__MINGW32__)
    path = path.translate('/', '\\').stripTrailing("\\")+"\\";
#else
    path = path.stripTrailing("/")+"/";
#endif
    return path+file;
}
bool Helper::removeDir(const QString & dirName)
{
    //In Qt5 this can be done via "QDir::removeRecursively()."

    bool result = true;
    QDir dir(dirName);

    if (dir.exists(dirName))
    {
        Q_FOREACH(QFileInfo info, dir.entryInfoList(QDir::NoDotAndDotDot | QDir::System | QDir::Hidden  | QDir::AllDirs | QDir::Files, QDir::DirsFirst))
        {
            if (info.isDir()) result = removeDir(info.absoluteFilePath());
            else result = QFile::remove(info.absoluteFilePath());
            if (!result) return result;
        }
        result = dir.rmdir(dirName);
    }
    return result;
}

void Helper::showHintMessage(QWidget * parent, int id)
{

    QSettings hintSettings(_CFG_DIR, "pmf6");
    GString notAgain = hintSettings.value("Hint_"+GString(id), "").toString();
    if( notAgain.asInt() > 0 ) return;


    GString msg;
    if(id == 1001 ) 
	{
        msg = "Plase note:\nThe LOB data will be copied to a local file.\nAny changes you make there will NOT be stored in the database.";
		msg += "\n\nTo update LOB data, simply drag&drop a file into a cell\n";
		msg += "or put a question mark (without quotes) into a cell and click 'save'";
	}
    else if(id == 1002 ) 
	{
		msg = "Hint: When editing cells, the quotes around strings will be set automatically.\nSaves you some typing.";
	}
    else if(id == 1003 )
    {
        msg = "Closing PMF via ESC key has been disabled\n(to prevent PMF from preventing Windows-shutdown)\n\nTo enable ESC, click 'Settings'.";
    }
    else return;

    QMessageBox *mb = new QMessageBox(parent);
    mb->setText(msg);
    mb->setIcon(QMessageBox::Information);
    mb->addButton(QMessageBox::Ok);
    QCheckBox notAgainCB("don't show this message again");
    notAgainCB.blockSignals(true);
    mb->addButton(&notAgainCB, QMessageBox::ResetRole);
    mb->exec();

    if(notAgainCB.checkState() == Qt::Checked) hintSettings.setValue((char*)("Hint_"+GString(id)), 1);
    else hintSettings.setValue((char*)("Hint_"+GString(id)), 0);	
}
int Helper::askFileOverwrite(QWidget *parent, GString fileName, GString question)
{
#ifdef MAKE_VC
    if( (_access( fileName, 0 )) == -1 ) return 0;
#else
    if( (access( fileName, 0 )) == -1 ) return 0;
#endif
    GString qst = "File exists. Overwrite?";
    if( question.length() ) qst = question;
    if( QMessageBox::question(parent, "PMF", qst, QMessageBox::Yes, QMessageBox::No) != QMessageBox::Yes ) return 1;
	return 0;	
}


int Helper::fileExists(GString fileName)
{
#ifdef MAKE_VC
    if( (_access( fileName, 0 )) != -1 ) return 1;
#else
    if( (access( fileName, F_OK )) != -1 ) return 1;
#endif
    return 0;
}

int Helper::fileIsUtf8(GString filename)
{
    //1. via BOM
    FILE *file;

    printf("Start, in: %s\n", (char*) filename);

    file=fopen(filename,"rb");
    if (!file) return 0 ;
    char buffer[4];
    fread(buffer,sizeof(buffer),1,file);
    fclose(file);
    if (buffer[0] == 0XEF && buffer[1] == 0XBB && buffer[2] == 0XBF) return 1;

    //2. the hard way
    int ret = 0;
    GString path = GString(filename);
    QFile f(path);
    if (!f.open(QFile::ReadOnly | QFile::Text)) return 0;
    QTextStream in(&f);
    while(!in.atEnd())
    {
        GString line = in.readAll();
        if(line.isProbablyUtf8()) ret = 1;
    }
    f.close();
    printf("end, in: %s, res: %i\n", (char*) filename, ret);
    return ret;
}

GString Helper::fileNameFromPath(GString fullPath)
{
    fullPath = fullPath.translate('\\', '/');
    if( fullPath.occurrencesOf('/') ) return fullPath.subString(fullPath.lastIndexOf('/')+1, fullPath.length()).strip();
    return fullPath;
}

int Helper::uncompressGZ(GString fileName)
{
    QFile qfile(fileName);
    if (!qfile.open(QIODevice::ReadOnly)) return 1;
    QByteArray blob = qfile.readAll();
    qfile.close();
    remove(fileName);

    QByteArray out = gUncompress(blob);
    QFile outFile(fileName);
    outFile.open(QIODevice::WriteOnly);
    outFile.write(out);
    outFile.close();
    if( dataIsXML(&out)) return 2;
	return 0;
}

bool Helper::dataIsXML(QByteArray * array)
{

    if( array->length() )
    {
        QDomDocument doc;
        return doc.setContent(*array, false);
    }
    return false;
}

int Helper::convertBase64toPNG(GString fileName, GString *newExt)
{
    *newExt = "";
    FILE *dataFile = fopen(fileName, "rb");
    if( !dataFile ) return 1;
    fseek(dataFile, 0, SEEK_END);
    long fsize = ftell(dataFile);
    if( fsize < 21 )
    {
        fclose(dataFile);
        return 2;
    }
    fseek(dataFile, 0, SEEK_SET);
    char *data = (char*)malloc(fsize + 1);
    fread(data, 21, 1, dataFile);
    if( GString(data).subString(1, 21) == "data:image/png;base64" ) *newExt = "PNG";
    else if( GString(data).subString(1, 21) == "data:image/jpg;base64" ) *newExt = "JPG";
    else
    {
        free(data);
        fclose(dataFile);
        return 3;
    }
    fseek(dataFile, 0, SEEK_SET);
    fread(data, fsize, 1, dataFile);
    fclose(dataFile);
    unsigned char *out = (unsigned char*)malloc(fsize/4*3);
    size_t outLen = fsize/4*3;
    int rc = GStuff::base64_decode(data+22, fsize-22, out, &outLen);
    if( !rc )
    {
        remove(fileName);
        dataFile = fopen(fileName, "wb");
        fwrite(out, 1, outLen, dataFile);
        fclose(dataFile);
    }
    free(data);
    free(out);
    return rc;
}

QByteArray Helper::gUncompress(const QByteArray &data)
{
    if (data.size() <= 4) {
        qWarning("gUncompress: Input data is truncated");
        return QByteArray();
    }

    QByteArray result;

    int ret;
    z_stream strm;
    static const int CHUNK_SIZE = 1024;
    char out[CHUNK_SIZE];

    /* allocate inflate state */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = data.size();
    strm.next_in = (Bytef*)(data.data());

    ret = inflateInit2(&strm, 15 +  32); // gzip decoding
    if (ret != Z_OK)
        return QByteArray();

    // run inflate()
    do {
        strm.avail_out = CHUNK_SIZE;
        strm.next_out = (Bytef*)(out);

        ret = inflate(&strm, Z_NO_FLUSH);
        Q_ASSERT(ret != Z_STREAM_ERROR);  // state not clobbered

        switch (ret) {
        case Z_NEED_DICT:
            ret = Z_DATA_ERROR;     // and fall through
        case Z_DATA_ERROR:
        case Z_MEM_ERROR:
            (void)inflateEnd(&strm);
            return QByteArray();
        }

        result.append(out, CHUNK_SIZE - strm.avail_out);
    } while (strm.avail_out == 0);

    // clean up and return
    inflateEnd(&strm);
    return result;
}

GString Helper::createIndexSortString(GString in)
{
    GString out;
    int pos;
    in = in.strip();
    if( in.occurrencesOf('+') == 0 && in.occurrencesOf('-') == 0 && in.occurrencesOf('*') == 0 ) return in;
    while( in.length() )
    {
        pos = Helper::findNextSortToken(in) - 1;
        if( pos == 0 ) break;
        if( in[1] == '+')  out += in.subString(2, pos) + " ASC, ";
        if( in[1] == '-')  out += in.subString(2, pos) + " DESC, ";
        if( in[1] == '*')  out += in.subString(2, pos) + " RAND, ";
        in = in.remove(1, pos+1);
    }
    return out.strip().stripTrailing(',');
}

int Helper::findNextSortToken(GString in)
{
    in = in.subString(2, in.length()).strip();
    if( in.occurrencesOf('+') == 0 && in.occurrencesOf('-') == 0 && in.occurrencesOf('*') == 0 ) return in.length()+1;
    for( int i = 1; i <= in.length(); ++i )
    {
        if( in[i] == '+' || in[i] == '-' || in[i] == '*' ) return i;
    }
    return in.length()+1;
}

GString Helper::tableName(GString table, ODBCDB type)
{
    table = table.removeAll('\"');
    if( type == SQLSERVER)
    {
        if( table.occurrencesOf(".") != 2 && table.occurrencesOf(".") != 1) return "@ErrTabString";
        if( table.occurrencesOf(".") == 2 ) table = table.remove(1, table.indexOf("."));
    }
    return table.subString(table.indexOf(".")+1, table.length()).strip();
}

GString Helper::tableContext(GString table, ODBCDB type)
{
    if( type != SQLSERVER ) return "";
    table.removeAll('\"');
    if( table.occurrencesOf(".") != 2 ) return "";
    return table.subString(1, table.indexOf(".")-1);
}

GString Helper::tableSchema(GString table, ODBCDB type)
{
    table.removeAll('\"');
    if( type == SQLSERVER)
    {
        if( table.occurrencesOf(".") != 2 && table.occurrencesOf(".") != 1) return "@ErrTabString";
        if( table.occurrencesOf(".") == 2 ) table.remove(1, table.indexOf("."));
    }
    return table.subString(1, table.indexOf(".")-1);
}

GString Helper::pmfVersion()
{
    return "v."+GString(PMF_VER).insert(".", 3).insert(".", 2);
}


GString Helper::pmfNameAndVersion(GString database)
{
    QDate aDate = QLocale("en_US").toDate(QString(__DATE__).simplified(), "MMM d yyyy");
    if( database.length() ) database = database +" - ";
#if defined( _WIN64 ) || defined( __x86_64__ )
    return database + "Poor Man's Flight v."+GString(PMF_VER).insert(".", 3).insert(".", 2) + " (64bit, Build "+GString(aDate.year()).subString(3,2)+GString(aDate.dayOfYear()).rightJustify(3, '0')+")";
#elif defined(__MINGW32__ )
    return database + "Poor Man's Flight v."+GString(PMF_VER).insert(".", 3).insert(".", 2)+" Portable (Build "+GString(aDate.year()).subString(3,2)+GString(aDate.dayOfYear()).rightJustify(3, '0')+", MinGW32)";
#elif defined(MSVC_STATIC_BUILD)
    return database + "Poor Man's Flight v."+GString(PMF_VER).insert(".", 3).insert(".", 2)+" Portable (Build "+GString(aDate.year()).subString(3,2)+GString(aDate.dayOfYear()).rightJustify(3, '0')+", MSVC)";
#else
    return database + "Poor Man's Flight v."+GString(PMF_VER).insert(".", 3).insert(".", 2)+" (Build "+GString(aDate.year()).subString(3,2)+GString(aDate.dayOfYear()).rightJustify(3, '0')+")";
#endif
}



void Helper::setVHeader(QTableWidget * someLV, bool movable)
{
    someLV->setUpdatesEnabled(false);
    someLV->setWordWrap(false);
#if QT_VERSION >= 0x050000
    someLV->horizontalHeader()->setSectionsMovable(movable);
#else
    someLV->horizontalHeader()->setMovable(movable);
#endif

    if( !someLV->isSortingEnabled() ) someLV->setSortingEnabled(true);
    QTableWidgetItem * newItem;
    for ( int i = 0; i < someLV->rowCount(); ++i)
    {
        newItem = new QTableWidgetItem(i);
        someLV->setRowHeight(i, QFontMetrics( someLV->font()).height()+5);
        someLV->setVerticalHeaderItem(i, newItem);
    }
//    for(int i = 0; i < someLV->columnCount(); ++i)
//    {
//        someLV->resizeColumnToContents ( i );
//        if( someLV->columnWidth(i) > 200 ) someLV->setColumnWidth(i, 200);
//    }
    someLV->setUpdatesEnabled(true);
}

void Helper::setLastSelectedPath(GString className, GString path)
{
    QSettings settings(_CFG_DIR, "pmf6");
    settings.setValue(className, QString((char*)path));
}

GString Helper::getLastSelectedPath(GString className)
{
    QSettings settings(_CFG_DIR, "pmf6");
    if( settings.value(className, -1).toInt() >= 0 )
    {
        return settings.value(className).toString();
    }
    return "";
}

GString Helper::createSearchConstraint(DSQLPlugin * pDSQL, GSeq<COL_SPEC*> *colDescSeq, GString val, int col, int exactMatch)
{
    if( !val.strip().length() ) return "";

    val = pDSQL->cleanString(val);
    GString modifier1 = "";
    GString modifier2 = "";

    if( val.occurrencesOf("@DSQL@") || (pDSQL->isXMLCol(col) && val != "NULL") )
    {
        return "";
    }
    if( pDSQL->isForBitCol(col) >= 3  && val != "NULL"  )
    {
        int lng = 0;
        for(int i = 1; i <= (int)colDescSeq->numberOfElements(); ++i)
        {
            if( i == col ) lng =  colDescSeq->elementAtPosition(i)->Length.asInt();
        }
        modifier1 = "";
        //val = " LIKE '%' || "+formatForHex(m_pMainDSQL, "'"+val+"'")+" || '%'";

        if( lng*2 != (int)val.length() && !exactMatch ) val = " LIKE '%' || "+Helper::formatForHex(pDSQL, "'"+val+"'")+" || '%'";
        else val = " = "+Helper::formatForHex(pDSQL, "'"+val+"'");
    }
    else if( val == "NULL") val = " IS NULL";
    else if( pDSQL->isNumType(col) && pDSQL->getDBType() == SQLSERVER)
    {
        if( exactMatch) val = " = '"+val+"'";
        else  val = " LIKE '%"+val+"%'";
        modifier1 = " CAST ";
        modifier2 = " as NVARCHAR";
    }
    else if( pDSQL->isNumType(col) && pDSQL->getDBType() == POSTGRES)
    {
        if( exactMatch) val = " = '"+val+"'";
        else  val = " LIKE '%"+val.upperCase()+"%'";
        modifier1 = " CAST ";
        modifier2 = " as VARCHAR";
    }
    else if( pDSQL->isNumType(col) )
    {
        modifier1 = "CHAR";
        if( exactMatch) val = "="+val.upperCase();
        //else val = " LIKE '%"+val.upperCase()+"%'";
        else val = " LIKE upper('%"+val+"%')";
    }
//    else if( GString(val).upperCase() == "CURRENT TIMESTAMP" || GString(val).upperCase() == "CURRENT DATE" ||
//             GString(val).upperCase() == "CURRENT_TIMESTAMP" || GString(val).upperCase() == "CURRENT_DATE")
//    {
//        val = "=" + val;
//    }
    else if( pDSQL->isDateTime(col) )
    {
        if( exactMatch) val = " = '"+val+"'";
        else  val = " LIKE '%"+val+"%'";
        modifier1 = " CAST ";
        modifier2 = " as VARCHAR";
    }
    else if( GString(val).upperCase() == "GETDATE()" )
    {
        val = "=" + val;
    }
    else
    {        
        pDSQL->convToSQL(val);
        if( exactMatch )
        {
            modifier1 = "";
            val = " = '"+val+"'";
        }
        else
        {
            //val = " LIKE '%"+val.upperCase()+"%'";
            val = "LIKE upper('%"+val+"%')";
            modifier1 = "UPPER";
        }
    }
    return modifier1+"("+GStuff::wrap(pDSQL->hostVariable(col))+modifier2+")"+val;
}

GString Helper::formatForHex(DSQLPlugin* pDSQL, QTableWidgetItem * pItem)
{
    if( !pItem ) return "<NULL Item>";
    GString text = pItem->text();
    //For anything but DB2, remove leading 'x' from a Hex string
    //if data is copied from DB2 to a non-DB2 table
    if( pDSQL->getDBType() != DB2 && pDSQL->getDBType() != DB2ODBC )
    {
        if( GString(text).strip().strip("'").indexOf('x') == 1 )return text.remove(1,1);
    }


    if( text.indexOf('x') == 1 )return text;
    if( pDSQL->getDBType() == DB2 || pDSQL->getDBType() == DB2ODBC ) return "x"+Helper::formatItemText(pDSQL, pItem);
    return text;
}

GString Helper::formatForHex(DSQLPlugin* pDSQL, GString in)
{
    //For anything but DB2, remove leading 'x' from a Hex string
    //if data is copied from DB2 to a non-DB2 table
    if( pDSQL->getDBType() != DB2 && pDSQL->getDBType() != DB2ODBC )
    {
        if( GString(in).strip().strip("'").indexOf('x') == 1 ) return in.remove(1,1);
    }
    //For DB2, we need to set 'x' before a Hex string
    if( GString(in).strip().strip("'").indexOf('x') == 1 )return in.remove(1,1);
    if( pDSQL->getDBType() == DB2 || pDSQL->getDBType() == DB2ODBC ) return "x"+in;
    return in;
}

GString Helper::formatItemText(DSQLPlugin * pDSQL, QTableWidgetItem * pItem)
{
    int colNr = pItem->column()-1;
    if( !pItem ) return "<NULL Item>";
    GString text = pItem->text();
    if( !text.length() ) return "NULL";

    if( text == "?" )
    {
        if( pDSQL->isLOBCol(colNr) )return text;
        if( pDSQL->isXMLCol(colNr) )return text;
    }
    if( text.subString(1, 6) == "@DSQL@" ) return text;
    if( Helper::isSystemString(text)) return text;
    if( pDSQL->isForBitCol(colNr) ||  pDSQL->isNumType(colNr) || GString(text).upperCase() == "NULL") return text;
    if( pDSQL->isDateTime(colNr) )
    {
        if( GString(text).upperCase().occurrencesOf("CURRENT") ) return text;
        else if( text[1UL] == '\'' && text[text.length()] == '\'' )  return text;
        return "'"+text+"'";
    }
    if( text[1UL] == '\'' && text[text.length()] == '\'' )  return text;
    return "'"+text+"'";
}

int Helper::isSystemString(GString in)
{
    in = in.upperCase();
    if( in.occurrencesOf("CURRENT"))
    {
        if( in.occurrencesOf("DATE") || in.occurrencesOf("TIMESTAMP")) return 1;
    }
    if( in.occurrencesOf("GETDATE()")) return 1;
    return 0;
}

int Helper::createFileIfNotExist(GString fileName)
{
    QDir dir;
    GString path = GStuff::pathFromFullPath(fileName);
    if (!dir.exists(path)) dir.mkpath(path);
    QFile file(path + GStuff::fileFromPath(fileName));
    file.open(QIODevice::WriteOnly);
    file.close();
    return 0;
}

GString Helper::handleGuid(DSQLPlugin *pDSQL,  GString in, int convertGuid)
{
    if( in == "NULL" ) return in;
    //Not DB2
    if( pDSQL->getDBType() != DB2 && pDSQL->getDBType() != DB2ODBC )
    {
        //remove leading 'x'
        if( in.indexOf('x') == 1 ) in = in.remove(1,1);
        else return in; //Probably OK as it is.
        in = in.strip().strip("'");
        if( convertGuid ) in = Helper::convertGuid(in);
        return "'"+in+"'";
    }
    //DB2:
    //Seems ok as it is (DB2 <-> DB2)    
    if( in.indexOf('x') == 1 ) return in;
    in = in.strip().strip("'");
    if( convertGuid ) in = Helper::convertGuid(in);
    return "x'" + in.upperCase() + "'";
}

GString Helper::convertGuid(GString in)
{

    int toDb2 = 0;
    if( in.length() != 32 && in.length() != 36 && in.length() != 38 ) return in;
    if( in.occurrencesOf('-') != 0 && in.occurrencesOf('-') != 4 ) return in;
    if( in.occurrencesOf('-') == 0 ) toDb2 = 1; //convert to DB2 format

    in = in.removeAll('-');
    GSeq <GString> blockSeq;
    for( int i = 1; i < 32; i+=2 )
    {
        blockSeq.add(in.subString(i, 2));
    }
    GString out;
    out = blockSeq.elementAtPosition(4) + blockSeq.elementAtPosition(3)+blockSeq.elementAtPosition(2)+blockSeq.elementAtPosition(1);
    if( toDb2 ) out += "-";
    out += blockSeq.elementAtPosition(6)+blockSeq.elementAtPosition(5);
    if( toDb2 ) out += "-";
    out += blockSeq.elementAtPosition(8)+blockSeq.elementAtPosition(7);
    if( toDb2 ) out += "-";
    out += blockSeq.elementAtPosition(9)+blockSeq.elementAtPosition(10);
    if( toDb2 ) out += "-";
    out += blockSeq.elementAtPosition(11)+blockSeq.elementAtPosition(12)+blockSeq.elementAtPosition(13)+blockSeq.elementAtPosition(14);
    out += blockSeq.elementAtPosition(15)+blockSeq.elementAtPosition(16);
    return out;

}

void Helper::storeGeometry(QDialog *qd, GString key)
{
    QSettings settings(_CFG_DIR, "pmf6");
    settings.setValue(key, qd->saveGeometry());

#if QT_VERSION >= 0x060000
            QRect screenGeometry = QGuiApplication::primaryScreen()->geometry();
#else
            QRect screenGeometry = QApplication::desktop()->screenGeometry();
#endif
    settings.setValue("CurrentResolution", screenGeometry);
}

void Helper::setGeometry(QDialog *qd, GString key)
{
    QSettings settings(_CFG_DIR, "pmf6");
    QDialog dummy;

#if QT_VERSION >= 0x060000
            QRect screenGeometry = QGuiApplication::primaryScreen()->geometry();
#else
            QRect screenGeometry = QApplication::desktop()->screenGeometry();
#endif
    QRect prev = settings.value("CurrentResolution", "NotSet").toRect();
    if( prev != screenGeometry )
    {
        //screen reso changed - forget about restoring geometry.
        return;
    }

    dummy.restoreGeometry(settings.value(key).toByteArray());
    QRect qr = dummy.geometry();

    //Fallback
    if( qr.x() + qr.width() < 0 || qr.y() + qr.height() < 0 || qr.x() > screenGeometry.width() || qr.y() > screenGeometry.height())
    //if( qr.y() + qr.height() > screenGeometry.height() || qr.x() + qr.width() >screenGeometry.width() || qr.x() < 0 || qr.y() < 0 )
    {
        return;
    }
    else qd->restoreGeometry(settings.value(key).toByteArray());
}

GString Helper::connSetToString(CON_SET * pCS)
{
    return pCS->Type + _PMF_PASTE_SEP + pCS->Host+ _PMF_PASTE_SEP + pCS->Port + _PMF_PASTE_SEP + pCS->DB + _PMF_PASTE_SEP + pCS->CltEnc + _PMF_PASTE_SEP + pCS->UID;
}

int Helper::connSetFromString(GString txt, CON_SET * pCS)
{
    GSeq <GString> csSeq = txt.split(_PMF_PASTE_SEP);
    if( csSeq.numberOfElements() < 5)
    {
        pCS->Type = "";
        pCS->Host = "";
        pCS->Port = "";
        pCS->DB = "";
        pCS->CltEnc = "";
        pCS->UID = "";
        return 1;
    }
    pCS->Type = csSeq.elementAtPosition(1);
    pCS->Host = csSeq.elementAtPosition(2);
    pCS->Port = csSeq.elementAtPosition(3);
    pCS->DB = csSeq.elementAtPosition(4);
    pCS->CltEnc = csSeq.elementAtPosition(5);
    if( csSeq.numberOfElements() > 5 ) pCS->UID = csSeq.elementAtPosition(6);
    return 0;
}


int Helper::runCommandInProcess(GString cmd, GString &res, GString &err)
{
	cmd = cmd.strip();
	printf("cmd: %s\n", (char*) cmd);
    QProcess process;	

    process.start(cmd);
    bool finishedOK = process.waitForFinished(-1); // will wait forever until finished

	if( finishedOK ) printf("runStuffInProcess, finished OK\n");
	else printf("runStuffInProcess, finished not OK\n");
	process.setProcessChannelMode(QProcess::MergedChannels);
    QString st_out = process.readAllStandardOutput();
    QString st_err = process.readAllStandardError();
    res = GString(st_out).strip();
	if( !finishedOK ) err = GString(process.errorString()) + " " +GString(st_err).strip();
	
    return finishedOK ? 0 :1;
}

int Helper::runStuffInProcess(GString cmd, GString &res, GString &err)
{
	QStringList qList;
	cmd = cmd.strip();
	GSeq<GString> argList = cmd.split(' ');
	if( argList.numberOfElements() == 0 ) 
	{
		res = "";
		err = "No cmd given: Nothing to do.";
		return 1;
	}
	cmd = argList.elementAtPosition(1);
	printf("cmd: %s\n", (char*) cmd);
	for( int i = 2; i <= argList.numberOfElements(); ++i )
	{
		qList.append(argList.elementAtPosition(i));
	}	
    QProcess process;	
    process.start(cmd, qList);
    bool finishedOK = process.waitForFinished(-1); // will wait forever until finished

	if( finishedOK ) printf("runStuffInProcess, finished OK\n");
	else printf("runStuffInProcess, finished not OK\n");
	process.setProcessChannelMode(QProcess::MergedChannels);
    QString st_out = process.readAllStandardOutput();
    QString st_err = process.readAllStandardError();
    res = GString(st_out).strip();
	if( !finishedOK ) err = GString(process.errorString()) + " " +GString(st_err).strip();
	
    return finishedOK ? 0 :1;
}

bool Helper::isSystemDarkPalette()
{    
//0x060500 is equivalent to v6.5.00
#if QT_VERSION < 0x060500
	printf("Helper::isSystemDarkPalette: Qt version too low, returning false\n");
    return false;
#endif

	QSettings regSet("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",QSettings::NativeFormat);
	GString val = regSet.value("AppsUseLightTheme").toString();
	printf("Helper::isSystemDarkPalette: Mode from registry: %s\n", (char*) val);
	
#if QT_VERSION < 0x060800
	//User wants lightMode:
    QSettings settings(_CFG_DIR, "pmf6");
    GString prevStyle = settings.value("style", "").toString();
	if( prevStyle.length() && !prevStyle.occurrencesOf(_DARK_THEME) ) return false;
#endif	
	
#if QT_VERSION > 0x060500
    QStyleHints *styleHints = QGuiApplication::styleHints();
    printf("Helper::isSystemDarkPalette: Checking styleHints...\n");
	if( styleHints->colorScheme() == Qt::ColorScheme::Light ) 
	{
		printf("Helper::isSystemDarkPalette: is light\n");
		return false;		
	}
	else if( styleHints->colorScheme() == Qt::ColorScheme::Dark )
	{
		printf("Helper::isSystemDarkPalette: is dark\n");
		return true;
	}	
	else if( styleHints->colorScheme() == Qt::ColorScheme::Unknown ) printf("Helper::isSystemDarkPalette: is Unknown\n");
	else printf("Helper::isSystemDarkPalette: is not set\n");
	printf("Helper::isSystemDarkPalette: Checking styleHints...done.\n");
#endif
	
	
	const QPalette defaultPalette;
	int ltText = defaultPalette.color(QPalette::WindowText).lightness();
	int ltWin  = defaultPalette.color(QPalette::Window).lightness();
	printf("Helper::isSystemDarkPalette: lightness text: %i, lightness Window: %i\n", ltText, ltWin);
    
    bool isDark = defaultPalette.color(QPalette::WindowText).lightness() > defaultPalette.color(QPalette::Window).lightness();
    if( isDark ) printf("Helper::isSystemDarkPalette: Appears to be darkMode\n");
    else printf("Helper::isSystemDarkPalette: Appears to be lightMode\n");
    return isDark;
}

GString Helper::getSensibleStyle()
{
	
    QSettings settings(_CFG_DIR, "pmf6");
    GString prevStyle = settings.value("style", "").toString();
	printf("Helper::getSensibleStyle(): prevStyle from settings: %s\n", (char*) prevStyle);
	if( Helper::isSystemDarkPalette() ) printf("Helper::getSensibleStyle(): OS is in DarkMode\n");
    else printf("Helper::getSensibleStyle(): OS is in LightMode\n");


	
    //We do not use darkMode in Qt versions below 6.5.0
    #if QT_VERSION < 0x060500
        if( prevStyle.occurrencesOf(_DARK_THEME) ) return "Fusion";
    #endif
	//In Qt 6.8.0 there is no way to enforce lightMode if OS is in darkMode
    #if QT_VERSION >= 0x060800		
		printf("Helper::getSensibleStyle, we are on 6.8.0++\n");
		if( Helper::isSystemDarkPalette() ) 
		{
			printf("Helper::getSensibleStyle, we are on 6.8.0++ and OS is darkMode. Returning dark style.\n");
			if( prevStyle.occurrencesOf(_DARK_THEME) ) return prevStyle;
			else return "Fusion" + _DARK_THEME;
		}
	#endif 


    //first install
    if( !prevStyle.length() )
    {
		printf("Helper::getSensibleStyle, no prevStyle\n");
        if( Helper::isSystemDarkPalette() ) 
		{
			printf("Helper::getSensibleStyle(), no prev. style, and DarkMode, returning Fusion dark\n");
			return "Fusion" + _DARK_THEME;
		}
        else
        {
#ifdef MAKE_VC
			printf("Helper::getSensibleStyle, disabling darkMode\n");
            qputenv("QT_QPA_PLATFORM", "windows:darkmode=0");
#endif
			printf("Helper::getSensibleStyle(), no prev. style, and LightMode, returning Fusion\n");
            return "Fusion";
        }
    }

    //Disable DarkMode completely if:
    // -user has explicitly set a non-dark style
    if( !prevStyle.occurrencesOf(_DARK_THEME) )
    {
        
#ifdef MAKE_VC
		printf("Helper::getSensibleStyle(), prev. style is LightMode, disabling darkmode\n");
        qputenv("QT_QPA_PLATFORM", "windows:darkmode=0");
		//QGuiApplication::styleHints()->setColorScheme(Qt::ColorScheme::Light);
		//qApp->setPalette(qApp->style()->standardPalette());
#endif
		printf("Helper::getSensibleStyle(), prev. style is LightMode, returning prevStyle\n");
        return prevStyle;
    }
    else if( prevStyle.occurrencesOf(_DARK_THEME) )
    {
        if( !Helper::isSystemDarkPalette() ) 
		{			
#ifdef MAKE_VC
			qputenv("QT_QPA_PLATFORM", "windows:darkmode=0");
			//QGuiApplication::styleHints()->setColorScheme(Qt::ColorScheme::Light);
			//qApp->setPalette(qApp->style()->standardPalette());
#endif	
			printf("Helper::getSensibleStyle(), prev. style is DarkMode, but OS is LightMode, forcing lightMode and returning Fusion\n");
			return "Fusion";
		}
    }
	printf("Helper::getSensibleStyle(), final, returning %s\n", (char*) prevStyle);
    return prevStyle;
}

/*
int Helper::convertFileToCodePage(GString inFile, GString outFile, GString codec)
{

//    for(auto codecstr: QTextCodec::availableCodecs())
//    {
//        printf("Codec: %s\n", (char*) GString(codecstr.data()));
//    }
    QFile fileIn(inFile);
    if (!fileIn.open(QIODevice::ReadOnly))
    {
        return -1;
    }
    QFile fileOut(outFile);
    if (!fileOut.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        return -2;
    }
    QTextStream streamFileOut(&fileOut);
    streamFileOut.setCodec((char*)codec);
    char cr = 13;
    char lf = 10;

    QTextStream in(&fileIn);
    while (!in.atEnd())
    {
        QString line = in.readLine();
#ifdef MAKE_VC
        streamFileOut << line+cr;
        streamFileOut << line+lf;
#else
        streamFileOut << line+"\n";
#endif
        streamFileOut.flush();
    }
    fileIn.close();
    fileOut.close();
    return 0;
}
*/
