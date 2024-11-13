//
//  This file is part of PMF (Poor Man's Flight), see README.PMF for details
//  Author: Gregor Leipelt (gregor@leipelt.de)  (C) G. Leipelt
//


#ifndef _GKEYVAL_
#define _GKEYVAL_

#ifdef  __IBMCPP__
#define MAKE_VA
#define NO_QT
#endif

#ifdef  _MSC_VER
  #ifndef MAKE_VC
    #define MAKE_VC
  #endif
#endif

//************** Visual C++ ********************
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
#ifdef MakeGKeyVal
  #define GKeyValExp_Imp _Export
#else
  #define GKeyValExp_Imp _Import
//  #pragma library( "GKeyVal.LIB" )
  #pragma library( "GLEngine.LIB" )
#endif
#endif
//************** Visual Age END *****************

#include <gstring.hpp>
#include <gseq.hpp>
#include <gdebug.hpp>


typedef struct KV_PAIR {
    GString key;
    GString val;
} KEY_VAL;


#ifdef MakeGKeyVal
  class GKeyValExp_Imp GKeyVal
#else
  class GKeyVal
#endif
{
private:
    int m_iDebug;
    void deb(GString msg);
    GSeq <KEY_VAL*> _keyValSeq;
    GDebug * m_pGDB;
public:
   VCExport   GKeyVal(int debugMode = 0);
   VCExport   ~GKeyVal();
   VCExport   void setGDebug(GDebug *pGDB);
   VCExport   KEY_VAL*  operator [] (int idx);
   VCExport   void add(GString key, GString val = "");
   VCExport   void add(KEY_VAL kv);
   VCExport   GString keyAtPos(int idx);
   VCExport   GString valAtPos(int idx);
   VCExport   int count();
   VCExport   KEY_VAL *elementAtPosition(int idx);
   VCExport   GString getValForKey(GString key);
   VCExport   int hasKey(GString key);
   VCExport   int toFile(GString fileName);
   VCExport   int readFromFile(GString fileName);
   VCExport   int replaceValue(GString key, GString value);
   VCExport   void addOrReplace(GString key, GString value);

};
#endif
