#include <stdio.h>
#include <gstring.hpp>
#include <gstringlist.hpp>
#include <gstuff.hpp>
#include <gsocket.hpp>
#include <gxml.hpp>
#include <gfile.hpp>

#include <QString>

#include <sstream>


//#include "utf8.h"

#include <iostream>
#include <locale>
#include <clocale>
#include <cwchar>

#include <fcntl.h>
#ifndef _O_U16TEXT
  #define _O_U16TEXT 0x20000
#endif
#include <QApplication>
#include <QTextEdit>
#include <QTableWidget>
#include <QFile>
#include <QTextStream>

char consInput[1000];



void deb(char * msg);
void formatXml(GString file);
void formatXmlFromString(GString in);
void createXml();
void fillXml();
void updateXml();


int main(int argv, char **args)
{

    createXml();
    fillXml();
    updateXml();
    //return 0;

//    formatXml("db2.xml");
//    return 0;

    printf("Start\n");
    formatXmlFromString("");
    //return 0;
    GXml xml;
    xml.readFromFile("test.xml");
    GXmlNode *pRoot = xml.getRootNode();
    deb("RootName: "+pRoot->tagName());
    GXmlNode *WsConf = pRoot->childAtPosition(1);
    deb("WsConf: "+WsConf->tagName());
    GXmlNode *Cmds = WsConf->childAtPosition(1);
    deb("Cmds: "+Cmds->tagName());
    GXmlNode *Devs = Cmds->childAtPosition(2);
    deb("Devs: "+Devs->tagName());
    deb("---Devices: "+GString(Devs->childCount()));
    for( int i = 1; i <= Devs->childCount(); ++ i )
    {
        GXmlNode *dev = Devs->childAtPosition(i);
        if( dev == NULL ) break;
        deb("Id: "+dev->getAttributeValue("id")+", inComment: "+GString(dev->inComment()));
        GXmlNode *devCmd = dev->childAtPosition(1);
        if( devCmd == NULL ) break;
        deb("Name1: "+devCmd->getAttributeValue("name")+", nodeName: "+devCmd->tagName()+", fullPath: "+devCmd->fullPath()+", in Comment: "+GString(devCmd->inComment()));
        devCmd = dev->childAtPosition(2);
        if( devCmd == NULL ) break;
        deb("Name2: "+devCmd->getAttributeValue("name"));

    }


    GXmlNode *pNode = xml.nodeFromXPath("/WebServiceConfiguration/commands/devices/device/");
    if( pNode == NULL ) deb("Got NULL node");
    else deb("Id: "+pNode->getAttributeValue("id")+", inComment: "+GString(pNode->inComment()));


    deb("PreAdd: "+GString(pNode->childCount()));
    GXmlNode aNode;
    aNode.setTagName("TEST");
    GXML_ATTR attr;
    attr.Name = "ID";
    attr.Value = "MyNewID_001";
    aNode.addAttribute(&attr);

    pNode = xml.getRootNode();
    pNode->addNode(&aNode);
    deb("PostAdd: "+GString(pNode->childCount()));
    deb("--------------------------------");
    deb(pNode->toString());
    deb("--------------------------------");

    return 0;
}

void updateXml()
{
    GXml gXml;
    gXml.readFromFile("someTest.xml");

    GXmlNode *encNodes = gXml.nodeFromXPath("/Encodings");
    GXmlNode *encNode;
    if( encNodes == NULL ) return;

    encNode = encNodes->addNode("Enc");
    encNode->addAttribute("pos", "Added");
    encNode->addAttribute("host", "bla");
    GFile f( "someTest.xml", GF_OVERWRITE );
    if( f.initOK() )    {
        f.addLine(gXml.toString());
    }
}

void fillXml()
{
    createXml();
    GXml gXml;
    gXml.readFromFile("someTest.xml");


    GXmlNode *encNodes = gXml.nodeFromXPath("/Encodings");
    GXmlNode *encNode;
    if( encNodes == NULL ) return;

    for(int i=1; i <= 2; ++i )
    {
        encNode = encNodes->addNode("Enc");
        encNode->addAttribute("pos", GString(i));
        encNode->addAttribute("host", "bla");
    }
    GFile f( "someTest2.xml", GF_OVERWRITE );
    if( f.initOK() )    {
        f.addLine(gXml.toString());
    }
}

void createXml()
{
    remove("someTest.xml");
    GFile f("someTest.xml");
    if( f.initOK() ) return;

    GXml gXml;
    gXml.create();
    GXmlNode * rootNode = gXml.getRootNode();
    rootNode->addNode("Encodings");

    GFile out( "someTest.xml", GF_OVERWRITE );
    if( out.initOK() )    {
        out.addLine(gXml.toString());
    }
}

void formatXmlFromString(GString fileName)
{
    GXml xml;
    GString in = "<a><b><cval=\"inner\"/></b><d><eval=\"inner2\"/></d></a>";
    GString out = xml.formatXmlString(in);
    printf("Formatted\n%s\n", (char*) out);

}

void formatXml(GString fileName)
{
    GXml xml;
    xml.readFromFile(fileName);


//    GXmlNode *pNode;
//    pNode = xml.getRootNode();
    deb(xml.getFormattedXml());
    return;

}

void deb(char * msg)
{

    printf("Test> %s\n", msg);
    #ifdef MAKE_VC
    flushall();
    #endif
}

