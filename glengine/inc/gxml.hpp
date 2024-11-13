#ifndef GXml_HPP
#define GXml_HPP


#include "gstring.hpp"
#include "gseq.hpp"

//************** Visual C++ ********************
#undef VCExport
#ifdef MAKE_VC
#define DllImport   __declspec( dllimport )
#define DllExport   __declspec( dllexport )
#define VCExport DllExport
#else
#define VCExport
#endif
//(************** Visual C++ END *****************

//************** Visual Age *********************
#ifdef MAKE_VA
#include <istring.hpp>
#ifdef MakeGXml
#define GXmlExp_Imp _Export
#else
#define GXmlExp_Imp _Import
#pragma library( "GLEngine.LIB" )
#endif
#endif
//************** Visual Age END *****************

typedef struct {
    GString Name;
    GString Value;
} GXML_ATTR;


typedef struct {
    int Begin;
    int End;
} COMMENT;


class GXmlNode
{
    friend class GXml;

public:
    GXmlNode();
    GXmlNode( GXmlNode * parent );
    ~GXmlNode();
    GXmlNode &operator = (const GXmlNode);

    int attributeCount();
    GXML_ATTR * attributeAtPostion(int i);
    GString getAttributeValue(GString tagName);
    GString tagName();
    GString fullPath();
    int childCount();
    GXmlNode * childAtPosition(int i);
    GXmlNode * child(GString tagName);
    GXmlNode * parent();
    GXmlNode * nodeFromXPath(GString xpath);
    int inComment();
    void addChildNode(GXmlNode * pNode);
    void addAttribute(GXML_ATTR* xmlAttr);
    void setTagName(GString tagName);
    GString toString();

private:
    void setFullPath(GString path);        
    void setInComment(int inComment){ _inComment = inComment; }
    void setAttributeSeq(GSeq<GXML_ATTR> seq);
    void setAttributeSeq();
    void setNodeValue(GString value);
    GString internal_toString(int indentCount);
    void deb(GString msg);

    GString _tagName;
    GString _fullPath;
    GString _nodeValue;
    int _inComment;
    GSeq <GXML_ATTR*> _attrSeq;
    GSeq <GXmlNode*> _childSeq;
    GXmlNode * _parent;
};


#ifdef MakeGXml
class GXmlExp_Imp GXml
        #else
class GXml
        #endif
{


public:
    GXml();
    GXml(GString xmlAsString);
    GXml( const GXml &x );
    ~GXml();
    int readFromFile(GString fileName);
    int lastErrCode();
    GString toString();
    GXml getBlocks(GString tag);
    GXml getBlocksFromXPath(GString tag);
    GXml getBlockAtPosition(GString tag, int position);
    int countBlocks(GString tag);

    int attributeCount();
    GString getAttribute(GString name);
    int isInComment();

    GXmlNode *getRootNode();
    GXmlNode * nodeFromXPath(GString xpath);

    GString getFormattedXml();
    GString formatXmlFromFile(GString data);
    GString formatXmlString(GString data);


private:
    int endTagPos(GString data, GString tag);
    GString getBlock(GString &data, GString tag);
    GXml getBlock(GString tag, int &startPos);
    void cleanInput();
    int fillCommentSeq();
    int removeComments();
    void setIsInComment(int start, int end);
    void deb(GString in);
    GString _xmlAsString;
    GString _xmlFullSource;
    int _lastErrCode;
    int _isInComment;
    GSeq <GXml> _blockSeq;
    GSeq <COMMENT*> _commentSeq;
    GString _xmlStartDeclaration;

    GString removeStartToken(GString in);
    GSeq <GString> allWords;
    void readAllWords();
    GSeq <GXML_ATTR> fillAttributeSeq(GString in = "");
    GString getValue(GString in);

    void readAllNodes(GString input);
    GString getNodeName(GString in);
    int lastIndexOfStartNode(int startPos, int endPos, GString node);
    int findEndNode(GString input, int startPos, GString nodeName);

    char* addIndentation(char* buf, int *pos, int indentCount, long *size);
    char* addChars(char* buf, int *pos, char ch, char nextCh, long *size);
    char* addGString(char* buf, int *pos, GString in, long *size);
    GXmlNode * pRootNode;
    GSeq <GXML_ATTR> _attrSeq;
};
#endif
