
#ifndef _GXML
#include "gxml.hpp"
#endif

#include "gstuff.hpp"
#include "gdebug.hpp"

#include <iostream>
#include <ctype.h>

#include <gseq.hpp>

#include <iostream>
#include <fstream>
/**************************************************************************
 *  GXMLNODE *
 *  GXMLNODE *
 *  GXMLNODE *
**************************************************************************/

GXmlNode::GXmlNode()
{
    _parent = NULL;
    _inComment = 0;
}

GXmlNode::GXmlNode( GXmlNode * parent )
{
    _parent = parent; _inComment = 0;
}

GXmlNode::~GXmlNode()
{
        GXML_ATTR* pAttr;
        _attrSeq.deleteAll();
        for(int i = 1; i <= _attrSeq.numberOfElements(); ++i )
        {
            //pAttr = _attrSeq.elementAtPosition(i);
            //delete [] pAttr;
        }
        GXmlNode* pNode;
        for(int i = 1; i <= _childSeq.numberOfElements(); ++i )
        {
            pNode = _childSeq.elementAtPosition(i);
            delete [] pNode;
        }
}

GXmlNode &GXmlNode::operator = (const GXmlNode)
{
       return *this;
}

void GXmlNode::setNodeValue(GString value)
{
    _nodeValue = value;
}

void GXmlNode::setAttributeSeq(GSeq<GXML_ATTR> seq)
{
    for(int i = 1; i <= seq.numberOfElements(); ++i )
    {
        GXML_ATTR* pAttr = new GXML_ATTR;
        pAttr->Name = seq.elementAtPosition(i).Name;
        pAttr->Value = seq.elementAtPosition(i).Value;
        _attrSeq.add(pAttr);
    }
}


void GXmlNode::addAttribute(GXML_ATTR* xmlAttr)
{
    GXML_ATTR* pAttr = new GXML_ATTR;
    pAttr->Name = xmlAttr->Name;
    pAttr->Value = xmlAttr->Value;
    _attrSeq.add(pAttr);
}

int GXmlNode::attributeCount()
{
    return _attrSeq.numberOfElements();
}
GXML_ATTR * GXmlNode::attributeAtPostion(int i)
{
    return _attrSeq.elementAtPosition(i);
}

GString GXmlNode::getAttributeValue(GString name)
{
    for( int i = 1; i <= _attrSeq.numberOfElements(); ++i )
    {
        if( _attrSeq.elementAtPosition(i)->Name == name )return _attrSeq.elementAtPosition(i)->Value;
    }
    return "";
}


void GXmlNode::setTagName(GString name)
{
    _tagName = name;
}

GString GXmlNode::tagName()
{
    return _tagName;
}

void GXmlNode::setFullPath(GString path)
{
    _fullPath = path;
}

GString GXmlNode::fullPath()
{
    return _fullPath;
}

void GXmlNode::addChildNode(GXmlNode * pNode)
{
    _childSeq.add(pNode);
}

int GXmlNode::childCount()
{
    return _childSeq.numberOfElements();
}

GXmlNode * GXmlNode::child(GString tagName)
{
    for( int i=1; i <= _childSeq.numberOfElements(); ++i )
    {
        if( _childSeq.elementAtPosition(i)->tagName() == tagName ) return _childSeq.elementAtPosition(i);
    }
    return NULL;
}

GXmlNode * GXmlNode::childAtPosition(int i)
{
    if( i > _childSeq.numberOfElements() ) return NULL;
    return _childSeq.elementAtPosition(i);
}

GXmlNode * GXmlNode::parent()
{
    return _parent;
}

int GXmlNode::inComment()
{
    return _inComment;
}

void GXmlNode::deb(GString msg)
{
    printf("GXmlNode> %s\n", (char*) msg);
#ifdef MAKE_VC
    _flushall();
#endif

}

/**************************************************************************
 *  GXML *
 *  GXML *
 *  GXML *
**************************************************************************/

GXml::GXml()
{
    deb("Ctor default");
    _blockSeq.removeAll();
    _lastErrCode = 0;
    _isInComment = 0;
}

GXml::GXml( const GXml &x )
{
    deb("Ctor copy");
    _lastErrCode = 0;
    _xmlAsString = x._xmlAsString;
    _isInComment = 0;
}

GXml::GXml(GString xmlAsString)
{
    deb("Ctor from string");
    _blockSeq.removeAll();
    _xmlAsString = xmlAsString.strip();
    _xmlFullSource = _xmlAsString;
    _lastErrCode = 0;
    _isInComment = 0;
    //fillCommentSeq();
}

GXml::~GXml()
{
    _blockSeq.removeAll();
}

int GXml::readFromFile(GString fileName)
{
    deb("readFromFile start");
    _blockSeq.removeAll();
    _lastErrCode = 0;

    FILE *fp;
    fp = fopen(fileName, "r");
    if( !fp )
    {
        _lastErrCode = 1001;
        return _lastErrCode;
    }
	fseek(fp, 0L, SEEK_END);
	int sz = ftell(fp);	
	rewind(fp);
	
    char ch;
    char cr = 10;
    char lf = 13;
    int inComment = 0;
    char* buf = new char[sz+1];

    int idx = -1;
    while((ch = fgetc(fp)) != EOF)
    {

        if( ch == '<' )
        {
            GString last = ch;
            buf[++idx] = ch;
            for(int i = 1; i <= 3; ++i )
            {
                ch = fgetc(fp);
                buf[++idx] = ch;
                last += ch;
            }
            /*
            last += (char)fgetc(fp);
            last += (char)fgetc(fp);
            last += (char)fgetc(fp);
            */
            if( last == "<!--") inComment = 1;
            //_xmlAsString += last;
            continue;
        }
        else if( ch == '-'  )
        {
            GString last = ch;
            buf[++idx] = ch;
            for(int i = 1; i <= 2; ++i )
            {
                ch = fgetc(fp);
                buf[++idx] = ch;
                last += ch;
            }
            /*
            last += (char)fgetc(fp);
            last += (char)fgetc(fp);
            */
            if( last == "-->") inComment = 0;
            //_xmlAsString += last;
            continue;
        }
        if( !inComment && (ch == lf || ch == cr || ch == '\t'))  continue;

        buf[++idx] = ch;
        //printf("ptr(4): %s\n", ptr);

        //_xmlAsString += ch;

    }
    fclose(fp);
    buf[++idx] = 0;
    _xmlAsString = GString(buf);
    delete [] buf;
    printf("_xmlAsString: %s\n", (char*)_xmlAsString);
    cleanInput();
    deb("readFromFile done reading");



    //removeComments();
    readAllNodes(_xmlAsString);
    deb("readFromFile end");
    //fillCommentSeq();
    return 0;
}

GString GXml::formatXmlString(GString data)
{
    deb("formatXmlString start");
    _blockSeq.removeAll();
    _lastErrCode = 0;

    char * buf = (char*) malloc(data.length());

    char ch, lastCh =' ', nextCh;
    int inComment = 0;

    int indentCount = -1;
    int pos = 0;
    char* fp = (char*) data;
    long size = data.length();
    while((ch = *fp++) != EOF && pos < size)
    {
        if( inComment && ch != '-')
        {
            buf[pos] = ch;
            lastCh = ch;
            pos++;
            continue;
        }
        if( inComment && ch == '-')
        {
            GString data ="-";
            data += (char) *fp++;
            data += (char) *fp++;
            if( data == "-->")
            {
                data = data;
                inComment = 0;
                lastCh = '>';
            }
            buf = addGString(buf,&pos, data, &size);
            continue;
        }
        if( lastCh == '>' &&ch == ' ')
        {
            continue;
        }
        if( ch == '>' )
        {
            buf[pos] = ch;
            pos++;
            lastCh = ch;
            while( (ch = *fp++) == ' ') continue;
        }
        if( ch == '/' )
        {
            nextCh = *fp++;
            if( nextCh == '>' )
            {
                indentCount--;
            }
            buf = addChars(buf,&pos, ch,nextCh, &size);
            lastCh = nextCh;
            continue;
        }
        if( ch == '<' )
        {
            nextCh = *fp++;
            if( nextCh == '?' )
            {
                buf = addChars(buf,&pos, ch,nextCh, &size);
                continue;
            }
            if( nextCh == '!'  )
            {
                GString data = "<!";
                data += (char)*fp++;
                data += (char)*fp++;
                if( data == "<!--")
                {
                    data = "\n"+data;
                    inComment = 1;
                }
                buf = addGString(buf,&pos, data, &size);
                continue;
            }
            else if( lastCh == '>' && nextCh != '/')
            {
                indentCount++;
                buf = addIndentation(buf, &pos, indentCount, &size);
                buf = addChars(buf,&pos, ch,nextCh, &size);
                continue;
            }
            else if( lastCh == '>' && nextCh == '/' )
            {
                buf = addIndentation(buf, &pos, indentCount, &size);
                buf = addChars(buf,&pos, ch,nextCh, &size);
                indentCount--;
                continue;
            }
            else if( nextCh == '/' )
            {
                indentCount--;
                buf = addChars(buf,&pos, ch,nextCh, &size);
                continue;
            }
            else
            {
                buf = addChars(buf,&pos, ch,nextCh, &size);
                continue;
            }
        }
        buf[pos] = ch;
        lastCh = ch;
        pos++;
    }

    buf[pos-1] = 0;
    //printf("buf: %s, pos: %i, size: %i\n", buf, pos, size);
//    fclose(fp);
    GString out = buf;
    free(buf);
    deb("formatXmlString end");
    return out;

}



GString GXml::formatXmlFromFile(GString fileName)
{
    deb("formatXmlFromFile start");
    _blockSeq.removeAll();
    _lastErrCode = 0;
    FILE *fp;
    fp = fopen(fileName, "r");
    if( !fp )
    {
        _lastErrCode = 1013;
        return _lastErrCode;
    }
    fseek(fp, 0L, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0L, SEEK_SET);

    char * buf = (char*) malloc(size);

    char ch, lastCh =' ', nextCh;
    int inComment = 0;

    int indentCount = -1;
    int pos = 0;
    while((ch = fgetc(fp)) != EOF && pos < size)
    {
        if( inComment && ch != '-')
        {
            buf[pos] = ch;
            lastCh = ch;
            pos++;
            continue;
        }
        if( inComment && ch == '-')
        {
            GString data ="-";
            data += (char)fgetc(fp);
            data += (char)fgetc(fp);
            if( data == "-->")
            {
                data = data;
                inComment = 0;
                lastCh = '>';
            }
            buf = addGString(buf,&pos, data, &size);
            continue;
        }
        if( lastCh == '>' &&ch == ' ')
        {
            continue;
        }
        if( ch == '>' )
        {
            buf[pos] = ch;
            pos++;
            lastCh = ch;
            while( (ch=fgetc(fp)) == ' ') continue;
        }
        if( ch == '/' )
        {
            nextCh = fgetc(fp);
            if( nextCh == '>' )
            {
                indentCount--;
            }
            buf = addChars(buf,&pos, ch,nextCh, &size);
            lastCh = nextCh;
            continue;
        }
        if( ch == '<' )
        {
            nextCh = fgetc(fp);
            if( nextCh == '?' )
            {
                buf = addChars(buf,&pos, ch,nextCh, &size);
                continue;
            }
            if( nextCh == '!'  )
            {
                GString data = "<!";
                data += (char)fgetc(fp);
                data += (char)fgetc(fp);
                if( data == "<!--")
                {
                    data = "\n"+data;
                    inComment = 1;
                }
                buf = addGString(buf,&pos, data, &size);
                continue;
            }
            else if( lastCh == '>' && nextCh != '/')
            {
                indentCount++;
                buf = addIndentation(buf, &pos, indentCount, &size);
                buf = addChars(buf,&pos, ch,nextCh, &size);
                continue;
            }
            else if( lastCh == '>' && nextCh == '/' )
            {
                buf = addIndentation(buf, &pos, indentCount, &size);
                buf = addChars(buf,&pos, ch,nextCh, &size);
                indentCount--;
                continue;
            }
            else if( nextCh == '/' )
            {
                indentCount--;
                buf = addChars(buf,&pos, ch,nextCh, &size);
                continue;
            }
            else
            {
                buf = addChars(buf,&pos, ch,nextCh, &size);
                continue;
            }
        }
        buf[pos] = ch;
        lastCh = ch;
        pos++;
    }

    buf[pos-1] = 0;
    //printf("buf: %s, pos: %i, size: %i\n", buf, pos, size);
    fclose(fp);
    GString out = buf;
    free(buf);
    deb("formatXmlFromFile end");
    return out;

}


char* GXml::addIndentation(char* buf, int *pos, int indentCount, long *size)
{
    indentCount *= 3;
    if( indentCount > 0 )*size += indentCount;
    buf = (char*) realloc(buf, *size);
    buf[*pos] = '\n';
    (*pos)++;
    for(int i = 0; i < indentCount; ++i )
    {
        buf[*pos] = ' ';
        (*pos)++;
    }
    return buf;
}

char* GXml::addGString(char* buf, int *pos, GString in, long *size)
{
    *size += in.length();
    buf = (char*) realloc(buf, *size);
    for(int i =1; i <= in.length();++i)
    {
        buf[*pos] = in[i];
        (*pos)++;
    }
    return buf;
}


char* GXml::addChars(char* buf, int *pos, char ch, char nextCh, long *size)
{
    *size += 2;
    buf = (char*) realloc(buf, *size);
    buf[*pos] = ch;
    (*pos)++;
    buf[*pos] = nextCh;
    (*pos)++;
    return buf;
}

int GXml::removeComments()
{
    if( _xmlAsString.occurrencesOf("<!--") != _xmlAsString.occurrencesOf("-->") )
    {
        _lastErrCode = 1002;
        return 1;
    }

    //deb("--removeComments in: "+_xmlAsString);
    while(_xmlAsString.occurrencesOf("<!--"))
    {
        _xmlAsString = _xmlAsString.subString(1, _xmlAsString.indexOf("<!--")-1) + _xmlAsString.subString(_xmlAsString.indexOf("-->")+3, _xmlAsString.length()).strip();
    }
    //deb("--removeComments out:"+_xmlAsString);
    return 0;
}

int GXml::fillCommentSeq()
{

    /*
    <!-- <foo/>
       <!-- <bar/> -->
       <!-- <xyz/> -->
    -->
    <!-- <tag/> -->
    */
    int startPos = 1;
    int cmtBlockCount = 0;
    COMMENT * pCmt;
    for(int i = 1; i <= _commentSeq.numberOfElements(); ++i )
    {
        pCmt = _commentSeq.elementAtPosition(i);
        delete[] pCmt;
    }


    GString data = _xmlAsString;

    while( data.indexOf("<!--", startPos) )
    {

        pCmt = new COMMENT;
        pCmt->Begin = data.indexOf("<!--", startPos);
        startPos = pCmt->Begin + 4;
        pCmt->End   = data.indexOf("-->", startPos)+3;
        startPos = pCmt->End;
        _commentSeq.add(pCmt);

        /*
        while( data.indexOf("<!--", startPos) || data.indexOf("-->", startPos) )
        {
            if( data.indexOf("<!--", startPos) > 0 && (data.indexOf("<!--", startPos) < data.indexOf("-->", startPos)) )
            {
                cmtBlockCount++;
                startPos = data.indexOf("<!--", startPos) + 4;
            }
            else if( data.indexOf("-->", startPos) > 0 )
            {
                cmtBlockCount--;
            }

            if( cmtBlockCount <= 0 )
            {
                pCmt->End = data.indexOf("-->", startPos);
                startPos = pCmt->End+3;
                _commentSeq.add(pCmt);
            }

        }
        */
    }

    //printf("XML:\n%s\n", (char*)_xmlAsString);

    for(int i = 1; i <= _commentSeq.numberOfElements(); ++i )
    {
        //printf("Start: %i, Stop: %i\n", _commentSeq.elementAtPosition(i)->Begin, _commentSeq.elementAtPosition(i)->End);
    }


    return 0;
}

int GXml::countBlocks(GString tag)
{
    int c1 = _xmlAsString.occurrencesOf("<"+tag+">");
    int c2 = _xmlAsString.occurrencesOf("<"+tag+" ");
    int c3 = _xmlAsString.occurrencesOf("<"+tag+"/>");
    return c1 + c2 + c3;
     //return _xmlAsString.occurrencesOf("<"+tag+">") + _xmlAsString.occurrencesOf("<"+tag+" ") + _xmlAsString.occurrencesOf("<"+tag+"/>");
}

GXml GXml::getBlocks(GString tag)
{
    GString blockXml;
    GString data = _xmlAsString;

    int count = countBlocks(tag);
    for(int i = 1; i <= count; ++i )
    {
        blockXml += getBlock(data, tag);
    }
    return GXml(blockXml);
}

GXml GXml::getBlocksFromXPath(GString tag)
{
    GSeq <GString> xPathElmtSeq;
    GString data = _xmlAsString;
    xPathElmtSeq = tag.stripLeading('/').split('/');
    GXml tempXml(data);
    for( int i = 1; i <= xPathElmtSeq.numberOfElements(); ++i )
    {
//        printf("--------\n");
//        printf("tag: %s\n", (char*) xPathElmtSeq.elementAtPosition(i));
//        printf("--------\n");
        tempXml = tempXml.getBlocks(xPathElmtSeq.elementAtPosition(i));
//        printf("tempXml for tag %s: %s\n", (char*)xPathElmtSeq.elementAtPosition(i), (char*) tempXml.toString());
//        printf("--------\n");
    }
    return tempXml;
}


int GXml::attributeCount()
{
    GString data = _xmlAsString;
    GSeq <GString> attrSeq = data.split(' ');
    return attrSeq.numberOfElements()-1;
}

GString GXml::getAttribute(GString name)
{
    deb("getAttribute start");
    if( _attrSeq.numberOfElements() == 0 ) _attrSeq = fillAttributeSeq();
    for( int i = 1; i <= _attrSeq.numberOfElements(); ++i )
    {
        if( _attrSeq.elementAtPosition(i).Name == name )
        {
            deb("getAttribute, item found, end");
            return _attrSeq.elementAtPosition(i).Value;
        }
    }
    deb("getAttribute, item not found, end");
    return "";
}

GString GXml::getValue(GString in)
{
    int pos = in.indexOf('>')+1;
    in = in.subString(pos, in.length()).strip();
    pos = in.indexOf('<');
    in = in.subString(1, pos-1).strip();
    return in;
}

GSeq<GXML_ATTR> GXml::fillAttributeSeq(GString in)
{
    deb("fillAttributeSeq start");
    if( !in.length() ) in = _xmlAsString;

    GString data = in.subString(1, in.indexOf(">")-1).strip("<").stripTrailing("/").strip();
    GSeq <GXML_ATTR> xmlAttrSeq;

    int pos = data.indexOf(" ");
    if( pos > 0 ) data = data.remove(1, pos);

    while(data.length())
    {

        if( data.occurrencesOf("=") == 0 ) break;
        GXML_ATTR * pAttr = new GXML_ATTR;
        pos = data.indexOf("=");
        pAttr->Name = data.subString(1, pos-1).strip();

        //Value is enclosed in '"'
        if( data.occurrencesOf("\"") < 2 ) break;

        data = data.remove(1, data.indexOf("\""));        
        pAttr->Value = data.subString(1, data.indexOf("\"")-1);
        data = data.remove(1, data.indexOf("\""));
        xmlAttrSeq.add(*pAttr);
    }
//    GSeq <GString> attrSeq = data.split(' ');
//    GString attr;
//    GSeq <GXML_ATTR> xmlAttrSeq;
//    for( int i = 2; i <= attrSeq.numberOfElements(); ++i )
//    {
//        attr = attrSeq.elementAtPosition(i);
//        GXML_ATTR * pAttr = new GXML_ATTR;
//        pAttr->Name = attr.subString(1, attr.indexOf("=")-1);
//        pAttr->Value = attr.subString(attr.indexOf("=")+1, attr.length()).strip("\"").strip();
//        xmlAttrSeq.add(*pAttr);
//    }
    deb("fillAttributeSeq end");
    return xmlAttrSeq;
}


GXml GXml::getBlockAtPosition(GString tag, int position)
{
    deb("getBlockAtPosition start");
    GString blockXml;
    GString data = _xmlAsString;

    int count = countBlocks(tag);
    for(int i = 1; i <= count; ++i )
    {
        blockXml = getBlock(data, tag);
        if( i == position ) return GXml(blockXml);
    }
    deb("getBlockAtPosition end");
    return GXml("");
}

int GXml::isInComment()
{
    return _isInComment;
}


GXmlNode *GXml::getRootNode()
{
    return pRootNode;
}

GString GXml::getFormattedXml()
{   
    GString out = _xmlStartDeclaration;
    out += pRootNode->toString();
    return out;
}

GString GXmlNode::toString()
{
    return internal_toString(0);
}

GString GXmlNode::internal_toString(int indentCount)
{
    GString indent;
    GString out;
    GString attrString;
    if( _tagName.strip().length() == 0 && _nodeValue.length() )
    {
        return _nodeValue;
    }
    if( _tagName.strip().length() > 0 )
    {
        for(int i = 0; i < indentCount; ++i ) indent += "   ";
        out += indent+"<"+_tagName+" ";
        for(int i = 1; i <= _attrSeq.numberOfElements(); ++i )
        {
            GXML_ATTR *pAttr = _attrSeq.elementAtPosition(i);
            attrString += pAttr->Name+"=\""+pAttr->Value+"\" ";
        }
        out += attrString+">"+_nodeValue;
    }
    for( int i = 1; i <= _childSeq.numberOfElements(); ++i )
    {
        out += "\n"+_childSeq.elementAtPosition(i)->internal_toString(indentCount+1);
    }
    if( _tagName.strip().length() > 0 )
    {
        if( _childSeq.numberOfElements() ) out += "\n"+indent+"</"+_tagName+">";
        else out += "</"+_tagName+">";
    }
    return out;
}


GXmlNode * GXml::nodeFromXPath(GString xpath)
{
    GXmlNode* pNode = pRootNode;
    xpath = xpath.stripTrailing('/').stripLeading('/');

    GSeq<GString> elmtSeq= xpath.split("/");
    for( int i = 1; i <= elmtSeq.numberOfElements(); ++i )
    {
        deb("XPathElmt "+GString(i)+": "+elmtSeq.elementAtPosition(i));
        pNode = pNode->child(elmtSeq.elementAtPosition(i));
        if( pNode == NULL ) break;
        deb("Found "+pNode->tagName());
    }
    return pNode;
}

GString GXml::removeStartToken(GString in)
{
    //<?xml version="1.0" encoding="utf-8"?>
    int start = in.indexOf("<?xml");
    int end = in.indexOf("?>");
    _xmlStartDeclaration = in.subString(start, end+2-start);
    return in.remove(start, end+2-start);
}

void GXml::readAllNodes(GString input)
{
    deb("readAllNodes start");
    int pos = 0;
    GString currentNode = "";
    int inComment = 0;

    pRootNode = new GXmlNode();
    pRootNode->setTagName("");
    pRootNode->setFullPath("");

    GXmlNode * pXmlNode;    
    GXmlNode * pCurrentParent = pRootNode;

    input = removeStartToken(input);

    while( input.strip().length() )
    {

        pos = input.indexOf("<");
        deb("CURIN: "+input);
        //deb("pos: "+GString(pos));

        if( pCurrentParent ) deb("CurrentParent: "+pCurrentParent->tagName());
        else return;
        int newNode = 0;

//        if( input.subString(1, input.indexOf(">")) == "-->")
//        {
//            input = input.remove(1, input.indexOf(">"));
//            inComment = 0;
//            continue;
//        }
//
        if( input[pos+1] == '!' && input[pos+2] == '-' && input[pos+3] == '-')
        {
            deb("--Have Comment");
            inComment = 1;
            GString comment = input.subString(1, input.indexOf("-->")+2);
            input = input.remove(1, input.indexOf("-->")+2 );
            pXmlNode = new GXmlNode(pCurrentParent);
            pXmlNode->setNodeValue(comment);
            pCurrentParent->addChildNode(pXmlNode);

            //input = input.remove(1, 4);
        }
        else if(input[pos+1] !='/')
        {
            pXmlNode = new GXmlNode(pCurrentParent);
            pXmlNode->setInComment(inComment);
            GString nodeName = getNodeName(input);
            deb("NODE "+nodeName+" is child of "+currentNode);

            int end = input.indexOf(">");            


            pXmlNode->setTagName(nodeName);
            pXmlNode->setFullPath(currentNode);
            pXmlNode->setAttributeSeq(fillAttributeSeq(input.subString(1, end)));
            pXmlNode->setNodeValue(getValue(input));
            pCurrentParent->addChildNode(pXmlNode);
            deb("Adding "+pXmlNode->tagName()+" as child to "+pCurrentParent->tagName() );
            newNode = 1;

            if( input[end-1] != '/')
            {
                pCurrentParent = pXmlNode;
                currentNode = currentNode +"/"+ nodeName;
            }
            else if( input[end-1] == '-' && input[end-2] == '-')
            {
                inComment = 0;
                input = input.remove(1, 3);
            }
            else
            {             
                pCurrentParent = pXmlNode->parent();
                deb("--Setting currentParent to "+pCurrentParent->tagName());
            }
            input = input.remove(1, end);
        }
        else
        {
            if( input.subString(1, input.indexOf(">")) == "-->")
            {
                input = input.remove(1, input.indexOf(">"));
                inComment = 0;
            }
            else
            {
                input = input.remove(1, input.indexOf(">"));
                currentNode = currentNode.subString(1, currentNode.lastIndexOf("/")-1);
                if( newNode ) pCurrentParent = pXmlNode->parent();
                else
                {
                    deb("Trying to set pCurrentParent to parent of "+pCurrentParent->tagName());
                    pCurrentParent = pCurrentParent->parent();
                }                
            }
            if( pCurrentParent) deb("++Setting currentParent to "+pCurrentParent->tagName());
        }
    }
    deb("readAllNodes end");



    /*
    int pos = 1;
    int count = 0;
    while( input.length() && count < 15)
    {
        pos = input.indexOf("<");
        deb("INPUT: "+input);
        int nodeEnd = input.indexOf(">");
        GString nodeName = getNodeName(input, pos, nodeEnd);
        deb(">>NodeName: "+nodeName);
        if( input[nodeEnd-1] == '/')
        {
            //Create Node Obj
            //Read Attrs
            //Add To Node List
        }
        else //Find End Of Node
        {
            int end = findEndNode(input, pos, nodeName);
            GString sub = input.subString(input.indexOf(">")+1, end-(nodeName.length()+3 ));
            deb("Creating SUB from start: "+GString(input.indexOf(">")+1)+" to "+GString(end-(nodeName.length()+3 )));
            deb("SUB: "+sub);
            input = input.stripLeading(sub);
            deb("input remain: "+input);
            readAllNodes(sub);
        }
        count++;
    }
    */
}



int GXml::lastIndexOfStartNode(int startPos, int endPos, GString node)
{
    GString sub = _xmlAsString.subString(startPos, endPos);
    if( sub.occurrencesOf(node+" ") > 0 || sub.occurrencesOf(node+">") > 0)
    {
        int p1 = sub.lastIndexOf(node+" ");
        int p2 = sub.lastIndexOf(node+">");
        if( p1 > p2 ) return p1;
        else return p2;
    }
    return 0;
}

int GXml::findEndNode(GString input, int startPos, GString nodeName)
{
    deb("findEndNode StartPos: "+GString(startPos));
    GString startNode = "<"+nodeName;
    GString endNode = "</"+nodeName+">";

    startPos = startPos + startNode.length();

    int endPos = input.indexOf(endNode, startPos);
    GString sub = input.subString(startPos, endPos-startPos);
    //deb("--Full: "+input);

    //deb("--endPos: "+GString(endPos)+", startPos: "+GString(startPos));
    //deb("--SUB: "+sub);
    while( sub.occurrencesOf(startNode+" ") +  sub.occurrencesOf(startNode+">") > sub.occurrencesOf(endNode) )
    {
        endPos = input.indexOf(endNode, endPos);
        //endPos = _xmlAsString.indexOf()

        //endPos = lastIndexOfStartNode(startPos, endPos, startNode);
        sub = input.subString(startPos, endPos);
        //deb("--SUB/while: "+sub);
    }
    deb("findEndNode end, ret: "+GString(endPos));
    return endPos;
}



GString GXml::getNodeName(GString in)
{

    int start = in.indexOf("<");
    int end = in.indexOf(">");

    GString nodeName = in.subString(start+1, end-start-1);
    if( nodeName.occurrencesOf(' ')) nodeName = nodeName.subString(1, nodeName.indexOf(' ')-1);
    //deb("--getNodeName: in: "+in+", start: "+GString(start)+", end: "+GString(end));
    return nodeName;
}

void GXml::readAllWords()
{
    GString word;
    int inWord = 0;
    for(int i = 1; i <= _xmlAsString.length(); ++i)
    {
        if(_xmlAsString[i] == '<')
        {
            inWord = 1;
            word = "";
        }
        if( inWord ) word += _xmlAsString[i];
        if(_xmlAsString[i] == ' ') inWord = 0;
        else if(_xmlAsString[i] == '/') inWord = 0;
        else if(_xmlAsString[i] == '>') inWord = 0;
        if( inWord == 0 && word != "</")
        {
            deb("Adding word "+word);
            allWords.add(word.stripTrailing(">"));
            word = "";
        }
    }
    for(int i = 1; i <= allWords.numberOfElements(); ++i )deb("Word "+GString(i)+": "+word);
}
/*
int GXml::getTagStart(GString tag, int startFrom)
{
    int pos = startFrom;
    tag = "<"+tag;
    while(pos < _xmlAsString.length() )
    {
        pos = _xmlAsString.indexOf(tag, pos);
        char nextChar = _xmlAsString[pos+tag.length()+1];
        if( nextChar = " " && nextChar != "/")
        {
            pos += pos+tag.length()+1;
            continue;
        }

    }
    int pos1 = _xmlAsString.indexOf("<"+tag+">", startFrom);
    int pos2 = _xmlAsString.indexOf("<"+tag+" ", startFrom);
    if( pos1 > 0 && pos2 > 0 ) return pos1 ? pos1 < pos2 : pos2;
    if( pos1 > 0 ) return pos1;
    else return pos2;
}

int GXml::getTagEnd(GString tag, int startFrom)
{
    int pos1 = _xmlAsString.indexOf("</"+tag+">", startFrom);
    int pos2 = _xmlAsString.indexOf("/>", startFrom);
    if( pos1 > 0 && pos2 > 0 && pos < pos1 ) return pos1;
    if( pos1 == 0 && pos2 > 0 ) return pos1;


    if( pos1 > 0 && pos2 > 0 ) return pos1 ? pos1 < pos2 : pos2;
    if( pos1 > 0 ) return pos1;
    else return pos2;
}
*/
/*
GXml GXml::getBlock(GString tag, int &startFrom)
{

    int startPos = _xmlAsString.indexOf("<")
    int endPos, cmtBeg, cmtEnd;
    GString endTag = "</"+tag+">";

    GString out;

    int inComment = 0;
    cmtBeg = data.indexOf("<!--");
    cmtEnd = data.indexOf("-->");


//    printf("---getBlock--------\n");
//    printf("IN: %s, tagToFind: %s\n", (char*) data, (char*) tag);
//    printf("---getBlock--------\n");



    startPos = data.indexOf("<"+tag+">");
    endPos   = data.indexOf("</"+tag+">");


    // <tag> ... </tag>

    if( startPos > 0 && endPos > startPos )
    {
        out+= data.subString(startPos, endPos+endTag.length()-startPos);
        data = data.remove(1, endPos+endTag.length());
        if( cmtBeg > 0 && cmtBeg < startPos && cmtEnd > 0 && cmtEnd > startPos ) _isInComment = 1;
    }
}
*/


GString GXml::getBlock(GString &data, GString tag)
{
    int startPos, endPos, cmtBeg, cmtEnd;
    GString out;
    GString endTag = "</"+tag+">";

//    printf("---getBlock--------\n");
//    printf("IN: %s, tagToFind: %s\n", (char*) data, (char*) tag);
//    printf("---getBlock--------\n");



    startPos = data.indexOf("<"+tag+">");
    endPos   = data.indexOf(endTag);

    _isInComment = 0;
    cmtBeg = data.indexOf("<!--");
    cmtEnd = data.indexOf("-->");


    // <tag> ... </tag>
    if( startPos > 0 && endPos > startPos )
    {
        out+= data.subString(startPos, endPos+endTag.length()-startPos);
        data = data.remove(1, endPos+endTag.length());
        if( cmtBeg > 0 && cmtBeg < startPos && cmtEnd > 0 && cmtEnd > startPos ) _isInComment = 1;
    }

    // </tag>
    startPos = data.indexOf("<"+tag+"/>");
    if( startPos > 0 )
    {
        endPos = GString("<"+tag+"/>").length();
        out += data.subString(startPos, endPos-startPos);
        data = data.remove(1, endPos+endTag.length());
        if( cmtBeg > 0 && cmtBeg < startPos && cmtEnd > 0 && cmtEnd > startPos ) _isInComment = 1;
    }
    // <tag attr...> ... </tag>
    // - or -
    // <tag attr... />
    startPos = data.indexOf("<"+tag+" ");
    if( startPos > 0 )
    {
        data = data.subString(startPos, data.length()).strip();
        endPos = data.indexOf(">");
        if( data[endPos-1] == '/')
        {
            out += data.subString(1, endPos);
            data = data.remove(1, endPos);            
        }
        else
        {
            endPos = data.indexOf(endTag);
            out += data.subString(1, endPos+endTag.length()-1);
            data = data.remove(1, endPos+endTag.length()-1);
        }
        if( cmtBeg > 0 && cmtBeg < startPos && cmtEnd > 0 && cmtEnd > startPos ) _isInComment = 1;
    }
//    printf("-----------\n");
//    printf("OUT: %s\n", (char*) out);
//    printf("-----------\n");



    return out;
}

void GXml::setIsInComment(int start, int end)
{
    COMMENT * pCmt;
    for(int i = 1; i <= _commentSeq.numberOfElements(); ++i )
    {
        pCmt = _commentSeq.elementAtPosition(i);
        if( pCmt->Begin > start && pCmt->End < end )
        {
            _isInComment = 1;
            return;
        }
    }
    _isInComment = 0;
}

GString GXml::toString()
{
    return _xmlAsString;
}

void GXml::cleanInput()
{
    deb("cleanInput start");
    while( _xmlAsString.occurrencesOf(" />"))
    {
        _xmlAsString = _xmlAsString.change(" />", "/>");
    }
    while( _xmlAsString.occurrencesOf(" >"))
    {
        _xmlAsString = _xmlAsString.change(" >", ">");
    }
    while( _xmlAsString.occurrencesOf("> "))
    {
        _xmlAsString = _xmlAsString.change("> ", ">");
    }
    deb("cleanInput end");
}

int GXml::endTagPos(GString data, GString tag)
{
    int pos = data.indexOf("</"+tag+">");
    if( pos > 0 ) return pos;
    return -1;
}

int GXml::lastErrCode()
{
    return _lastErrCode;
}

void GXml::deb(GString in)
{
    in = "["+GStuff::GetTime()+"] GXml> "+in;
    printf("%s\n", (char*) in);
#ifdef MAKE_VC
    _flushall();
#endif
}
