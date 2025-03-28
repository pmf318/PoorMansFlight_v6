//
//  This file is part of PMF (Poor Man's Flight), see README.PMF for details
//  Author: Gregor Leipelt (gregor@leipelt.de)  (C) G. Leipelt
//
/*********************************************************************
*********************************************************************/

#include "gfile.hpp"
#ifndef _GKEYVAL_
#include "gkeyval.hpp"
#endif
#include "gxml.hpp"
static GString _SEP   = "@PMF_SEP@";

GKeyVal::GKeyVal(int debugMode)
{
    m_iDebug = debugMode;
}

GKeyVal::~GKeyVal()
{
    KEY_VAL * pKV;
    while( !_keyValSeq.isEmpty() )
    {
        pKV = _keyValSeq.firstElement();
        delete pKV;
        _keyValSeq.removeFirst();
    }
}



void GKeyVal::add(GString key, GString val)
{
    KEY_VAL * pKV = new KEY_VAL;
    pKV->key = key;
    pKV->val = val;
    _keyValSeq.add(pKV);
}

void GKeyVal::add(KEY_VAL kv)
{
    KEY_VAL * pKV = new KEY_VAL;
    pKV->key = kv.key;
    pKV->val = kv.val;
    _keyValSeq.add(pKV);
}

KEY_VAL* GKeyVal::operator [] (int idx){
    if( idx < 1 || idx > _keyValSeq.numberOfElements() ) return NULL;
    return _keyValSeq.elementAtPosition(idx);
}

GString GKeyVal::keyAtPos(int idx)
{
    if( idx < 1 || idx > _keyValSeq.numberOfElements() ) return "";
    return _keyValSeq.elementAtPosition(idx)->key;
}

GString GKeyVal::valAtPos(int idx)
{
    if( idx < 1 || idx > _keyValSeq.numberOfElements() ) return "";
    return _keyValSeq.elementAtPosition(idx)->val;
}

int GKeyVal::count()
{
    return _keyValSeq.numberOfElements();
}

KEY_VAL *GKeyVal::elementAtPosition(int idx)
{
    if( idx < 1 || idx > _keyValSeq.numberOfElements() ) return NULL;
    return _keyValSeq.elementAtPosition(idx);
}

void GKeyVal::deb(GString message)
{
    GDebug::debMsg("GKeyVal", 1, message);
}

int GKeyVal::hasKey(GString key)
{
    for(int i = 1; i <= _keyValSeq.numberOfElements(); ++i )
    {
        if( _keyValSeq.elementAtPosition(i)->key == key) return 1;
    }
    return 0;
}

GString GKeyVal::getValForKey(GString key)
{
    for(int i = 1; i <= _keyValSeq.numberOfElements(); ++i )
    {
        if( _keyValSeq.elementAtPosition(i)->key == key) return _keyValSeq.elementAtPosition(i)->val;
    }
    return "@ValNotFound";
}

int GKeyVal::replaceValue(GString key, GString value)
{
    for(int i = 1; i <= _keyValSeq.numberOfElements(); ++i )
    {
        if( _keyValSeq.elementAtPosition(i)->key == key)
        {
            _keyValSeq.elementAtPosition(i)->val = value;
            return 0;
        }
    }
    return 1;
}

void GKeyVal::addOrReplace(GString key, GString value)
{
    if( !this->replaceValue(key, value) ) return;
    this->add(key, value);
}
/*
int GKeyVal::toFile(GString fileName)
{
    return toXmlFile(fileName);
    GString out;
    GFile f(fileName, GF_OVERWRITE);
    if( !f.initOK() ) return 1;

    for(int i = 1; i <= _keyValSeq.numberOfElements(); ++i )
    {
        out = _keyValSeq.elementAtPosition(i)->key + _SEP + _keyValSeq.elementAtPosition(i)->val;
        f.addLine(out);
    }
    return 0;
}
*/
int GKeyVal::readFromFile(GString fileName)
{
    return readFromXmlFile(fileName);
    GFile f;
    GString key, val, line;
    _keyValSeq.deleteAll();
    f.readFile(fileName);
    for(int i = 1; i <= f.lines(); ++i)
    {
        line = f.getLine(i);
        if( line.occurrencesOf(_SEP))
        {
            GSeq <GString> kvSeq = line.split(_SEP);
            key = kvSeq.elementAtPosition(1);
            val = kvSeq.elementAtPosition(2);
            this->add(key, val);
        }
    }
	return 0;
}


int GKeyVal::saveAsXml(GString fileName)
{
    GXml keyValXml;
    GXmlNode * rootNode = keyValXml.getRootNode();
    GXmlNode * pTopNode = rootNode->addNode("KeyValSettings");
    GString key, val;
    for(int i = 1; i <= (int)_keyValSeq.numberOfElements(); ++ i)
    {
        key = _keyValSeq.elementAtPosition(i)->key;
        val = _keyValSeq.elementAtPosition(i)->val;
        GXmlNode * pNode = pTopNode->addNode("data");
        pNode->addAttribute("key", key);
        pNode->addAttribute("val", val);
    }
    GFile f( fileName, GF_OVERWRITE );
    if( f.initOK() )    {
        f.addLine(keyValXml.toString());
        return 0;
    }
    return 1;
}
/*
int GKeyVal::toXmlFile(GString fileName)
{
    return saveAsXml(fileName);
    GString out,  key, val;
    GFile f(fileName, GF_OVERWRITE);
    if( !f.initOK() ) return 1;

    f.addLine("<KeyValSettings>");
    for(int i = 1; i <= _keyValSeq.numberOfElements(); ++i )
    {
        key = _keyValSeq.elementAtPosition(i)->key;
        val = _keyValSeq.elementAtPosition(i)->val;
        key = key.change("<", "&lt;").change(">", "&gt;");
        val = val.change("<", "&lt;").change(">", "&gt;");
        printf("toXmlFile, key: %s, out: %s\n", (char*)_keyValSeq.elementAtPosition(i)->key, (char*) key);
        printf("toXmlFile, val: %s, out: %s\n", (char*)_keyValSeq.elementAtPosition(i)->val, (char*) val);
        out = "<data key=\""+ key +"\" val=\""+val+"\" />";
        f.addLine(out);
    }
    f.addLine("</KeyValSettings>");
    return 0;
}
*/
int GKeyVal::readFromXmlFile(GString fileName)
{
    GXml fXml;
    GString key, val;
    int erc = fXml.readFromFile(fileName);
    if( erc ) return erc;

    _keyValSeq.deleteAll();
    GXml outXml = fXml.getBlocksFromXPath("/KeyValSettings/data");
    int count = outXml.countBlocks("data");
    for(int i=1; i <= count; ++i )
    {
        GXml inner = outXml.getBlockAtPosition("data", i);
        key = inner.getAttribute("key").change("&lt;", "<").change("&gt;", ">");
        val = inner.getAttribute("val").change("&lt;", "<").change("&gt;", ">");
        this->add(key, val);
    }
    return 0;
}
