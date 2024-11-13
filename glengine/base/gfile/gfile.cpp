//
//  This file is part of PMF (Poor Man's Flight), see README.PMF for details
//  Author: Gregor Leipelt (gregor@leipelt.de)  (C) G. Leipelt 
//
/*********************************************************************
*********************************************************************/

#ifndef _GFILE_
#include "gfile.hpp"
#endif
#include "gstuff.hpp"
#include "gdebug.hpp"

#include <iostream>
#include <ctype.h>

#include <gseq.hpp>

/*********************************************************************
*********************************************************************/
GString GFile::version()
{
   return "GFile v 1.0 (C) 1999-2001 Gregor Leipelt";
}
/*********************************************************************
*********************************************************************/
//Constructors....
GFile::GFile(int debugMode)
{ 
	init = 0; 
	m_iDebug = debugMode;
}

GFile::GFile(GString fName, int action, int debugMode)
{
	m_iDebug = debugMode;
   tm("Default Constructor, action: "+GString(action)+", file: "+fName);
   fileName = fName;
   FILE *f;
   init = 0;
   switch(action)
   {
      case GF_NOCREATE:
        f = fopen(fileName, "r+");
        break;
     case GF_OVERWRITE:
        f = fopen(fileName, "w+");
        break;
     case GF_APPENDCREATE:
        f = fopen(fileName, "a+");
        break;
     case GF_READONLY:
        f = fopen(fileName, "r");
        break;
	
     default:
        f = fopen(fileName, "r+");
        break;
   }
   if( !f  ) return;
   init = 1;
   fillSeq(f);
   fclose(f);
}

GFile::~GFile()
{
    m_seqLines.removeAll();
}

int GFile::readFile(GString fName)
{
   m_seqLines.removeAll();
   fileName = fName;
   FILE *f = fopen(fileName, "r+");
   init = 0;
   if( !f  ) return 1;
   init = 1;
   fillSeq(f);
   fclose(f);
   return 0;
}

int GFile::fillSeq(FILE *f)
{
#ifdef MAKE_VC
    int c;
    GString line;
    c = getc(f);
    if (c == EOF) return 0;
    fseek( f, 0, SEEK_SET );
    while( c != EOF )
    {
        c = getc(f);
        if( c == EOF ) break;
        if( c == '\n' )
        {
            m_seqLines.add(line);
            line = "";
        }
        else line += (char)c;
    }
    if( line.length() ) m_seqLines.add(line);
#else
    fseek( f, 0, SEEK_SET );
    char * line = NULL;
    size_t len = 0;
    ssize_t read;
    while ((read = getline(&line, &len, f)) != -1)
    {
        m_seqLines.add(line);
    }
    if (line) free(line);
#endif
    return 0;
}

GString GFile::initLineCrs()
{
    if( m_seqLines.numberOfElements() == 0 ) return "";
    return m_seqLines.initCrs();
}

int GFile::nextLineCrs()
{
    return m_seqLines.nextCrs();
}

GString GFile::lineAtCrs()
{
    return m_seqLines.itemAtCrs();
}

int GFile::setFileName(GString fName, int action)
{
   fileName = fName;
   FILE *f;
   init = 0;
   switch(action)
   {
      case 1:
        if( !(f = fopen(fileName, "r+")) ) init = 1;
        break;
     case 2:
        if( !(f = fopen(fileName, "w+")) ) init = 1;
        break;
     case 3:
        if( !(f = fopen(fileName, "a+"))  ) init = 1;
        break;
     default:
        if( !(f = fopen(fileName, "r+"))  ) init = 1;
        break;
   }
   if( init ) fclose(f);
   tm("setFileName: init: "+GString(init)+", name: "+fileName+"<-");
   return init;
}
int GFile::initOK()
{
   tm("initOK: "+GString(init));
   return init;
}
GString GFile::getKey(GString key, GString sep, GString rem)
{
   tm("getKey...");
   if( !init )
   {
      tm("getKey: Not initalised.");
      return "";
   }
   FILE * f;
   if( !(f = fopen(fileName, "r")) )
   {
      tm("getKey: Could not open File.");
      return "";
   }
   tm("getKey: File opened. key: "+key+", sep: "+sep+", rem: "+rem);
   char in[1024];
   GString line, out, temp;
   while( fgets(in, sizeof(in), f) )
   {
      line = GString(in).strip().stripTrailing("\n");	  	  
	  tm("getKey: as line "+line);
   }
   
   fseek( f, 0, SEEK_SET );
   tm("getKey: Now searching for key "+key);
   while( fgets(in, sizeof(in), f) )
   {
      //.stripTrailing("\n") is needed. strange.
	  // this code should not work at all, see docs for fgets()
      line = GString(in, 1023).strip().stripTrailing("\n");
      if( line.occurrencesOf(rem) > 0 ) line = line.subString(1, line.indexOf(rem)-1);
      //tm("getKey: searching line "+line);
	
      if( line.occurrencesOf(key) != 0)
      {
		 if( 0 == line.occurrencesOf(sep) )
		 {
			tm("getKey: bad usage, no separator. ");
			continue;  
		 } 	  
         temp = line.subString(1, line.indexOf(sep)-1).strip();
         if( temp != key ) continue; 
         tm("Found key in line...");
         out = line.subString(line.indexOf(sep)+1, 1023).strip();
		 fclose(f);

//             out = out.subString(1, out.length() -1);

         tm("Returning "+out);
		 return out;
      }

   }
   if( f ) fclose(f);
   tm("getKey done (no success)");
   return "";
}

int GFile::writeLine(GString  line)
{
    if( !init ) return 0;
    FILE * f;
    fpos_t pos;
    if( !(f = fopen(fileName, "a+")) ) return 1;
    fprintf(f, line+"\n");
    fclose(f);
}

int GFile::writeToNewLine(GString  line)
{
   //Do a SEEK_SET.
   
 //tm("Start writeLine...");
   if( !init ) return 0;
   FILE * f;
   fpos_t pos;
   if( !(f = fopen(fileName, "ab")) ) return 1;
//   fseek(f, 0L, SEEK_END);
//   fputs("\n", f);
   fputs( line, f );
   fclose( f );
   return 0;

   char in[1024];

   while( fgets(in, 1024, f)){}
   if( fgetpos( f, &pos ) != 0 )
   {
       fclose(f);
       return 2;
   }
   fsetpos( f, &pos );
   //tm("Writing "+line);
   fputs("\n", f);
   fputs( line, f );
   fclose( f );
   return 0;
}

int GFile::addOrReplaceLine(GString key, GString  line, GString sep, GString rem)
{
    return replaceLine(key, line, sep, rem, 1);
}
int GFile::replaceLine(GString key, GString  line, GString sep, GString rem, int add)
{
   unsigned long i;
   tm("replaceLine: Start.");
   if( !init ) return 1;
   int found = 0;

   GSeq <GString> fSeq;

   FILE * f;
   if( !(f = fopen(fileName, "r+")) ) return 1;
   char in[1024];
   GString ln, temp;
   fseek( f, 0, SEEK_SET );
   tm("replaceLine: Getting lines...");
   while( fgets(in, sizeof(in), f) )
   {
      tm("replaceLine: Adding "+GString(in).strip()+" to Seq.");
      fSeq.add(GString(in).strip());
   }
   tm("replaceLine: Seq has "+GString(fSeq.numberOfElements())+" Elements.");
   for( i=1; i <= fSeq.numberOfElements(); ++ i)
   {
      ln = fSeq.elementAtPosition(i);
      if( ln.occurrencesOf(rem) > 0 ) ln = ln.subString(1, ln.indexOf(rem)-1);
      tm("replaceLine: searching line "+ln+" for key "+key);
      temp = ln.subString(1, ln.indexOf(sep)-1).strip();
      if( temp != key ) continue; 
      
      if( ln.occurrencesOf(key) != 0)
      {
          tm("replaceLine: FOUND KEY at pos "+GString(i));
          int rc = fSeq.replaceAt(i, line+"\n");
          tm("RC REPLACE: "+GString(rc));
          found = 1;
      }
   }
   fclose(f);

   if( !found && add) fSeq.add(line);
   if( !(f = fopen(fileName, "w")) ) return 1;
   tm("replaceLine: writing file...");
   for( i=1; i <= fSeq.numberOfElements(); ++ i)
   {
      ln = fSeq.elementAtPosition(i);
      fputs(ln, f);
   }
   tm("replaceLine: ...DONE.");

   fclose(f);
   return 0;
}

GString GFile::getLine(int index)
{
    if( index < 0 || (unsigned long)index > m_seqLines.numberOfElements() ) return "";
    return m_seqLines.elementAtPosition(index).stripTrailing("\n");
}

int GFile::addLineForOS(GString line)
{
    FILE * f;
    if( !(f = fopen(fileName, "a+b")) ) return 1;
    fputs(line, f);
#ifdef MAKE_VC
    fputc(13, f);
    fputc(10, f);
#else
    fputc(10, f);
#endif
    fclose(f);
    return 0;
}

int GFile::addLine(GString line)
{
    FILE * f;
    if( !(f = fopen(fileName, "a+b")) ) return 1;
    fputs(line+"\n", f);
    fclose(f);
	return 0;
}
int GFile::seqToFile(GSeq<GString> *aSeq, GString mode)
{
    if( !init ) return 1;
    FILE * f;
    if( !(f = fopen(fileName, mode)) ) return 2;

    for( unsigned long i = 1; i <= aSeq->numberOfElements(); ++i )
    {
       fputs( aSeq->elementAtPosition(i).stripTrailing('\n'), f );
       fputs("\n",f);
    }
    fclose( f );
    return 0;
}

int GFile::removeAt(int pos)
{
    if ( pos < 1 || pos > m_seqLines.numberOfElements() ) return 1;
    m_seqLines.removeAt(pos);
    seqToFile(&m_seqLines, "w+");
    return 0;

}
int GFile::replaceAt(int pos, GString line)
{
    if ( pos < 1 || pos > m_seqLines.numberOfElements() ) return 1;
    line = line.stripTrailing('\n') + "\n";
    m_seqLines.replaceAt(pos, line);
    seqToFile(&m_seqLines, "w+");
    return 0;
}

int GFile::append(GSeq <GString> * aSeq)
{
    return seqToFile(aSeq, "a");
}

int GFile::overwrite(GSeq <GString> * aSeq)
{
    return seqToFile(aSeq, "w+");
}
int GFile::lines()
{
    return m_seqLines.numberOfElements();
}

void GFile::tm(GString message)
{
    GDebug::debMsg("GFile", 1, message);
}


