//
//  This file is part of PMF (Poor Man's Flight), see README.PMF for details
//  Author: Gregor Leipelt (gregor@leipelt.de)  (C) G. Leipelt
//
/*********************************************************************
*********************************************************************/

#ifndef _GKEYVAL_
#include "gkeyval.hpp"
#endif


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

