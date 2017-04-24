/*\
 * xmlEnum.cxx
 *
 * Copyright (c) 2017 - Geoff R. McLane
 * Licence: GNU GPL version 2
 *
\*/

/*
    Just an idea to eumerate an XML file, uisng the xmpParser library [1] -

    [1] https://www.applied-mathematics.net/tools/xmlparser_doc/html/index.html
            http://www.applied-mathematics.net/tools/xmlParser.html
            source: Download here: small, simple, multi-Plateform XMLParser library with examples (zipfile). 
            version: v2.44: May 19, 2013
            license: Aladdin Free Public License (Version 8, November 18, 1999) - see xmlParser/AFPL-license.txt
            copyright: Copyright (c) 2002, Frank Vanden Berghen - All rights reserved.

*/

#include <stdio.h>
#include <string.h> // for strdup(), ...
#include <vector>
#include <string>
#include "xmlParser.h"
// other includes

typedef std::vector<std::string> vSTG;

static const char *module = "xmlEnum";

static const char *usr_input = 0;

static int show_child_names = 0;    // we are enumerating children, so perhaps redundant
static int show_clear_tags = 1; // seems these are comments

static vSTG vXPath;

#ifndef DEF_TEST_FILE
#define DEF_TEST_FILE "../samples/PMMLModel.xml"
#endif

void give_help( char *name )
{
    printf("%s: usage: [options] usr_input\n", module);
    printf("Options:\n");
    printf(" --help  (-h or -?) = This help and exit(0)\n");
    // TODO: More help
}

int parse_args( int argc, char **argv )
{
    int i,i2,c;
    char *arg, *sarg;
    for (i = 1; i < argc; i++) {
        arg = argv[i];
        i2 = i + 1;
        if (*arg == '-') {
            sarg = &arg[1];
            while (*sarg == '-')
                sarg++;
            c = *sarg;
            switch (c) {
            case 'h':
            case '?':
                give_help(argv[0]);
                return 2;
                break;
            // TODO: Other arguments
            default:
                printf("%s: Unknown argument '%s'. Try -? for help...\n", module, arg);
                return 1;
            }
        } else {
            // bear argument
            if (usr_input) {
                printf("%s: Already have input '%s'! What is this '%s'?\n", module, usr_input, arg );
                return 1;
            }
            usr_input = strdup(arg);
        }
    }
#if !defined(NDEBUG)
    if (!usr_input)
        usr_input = strdup(DEF_TEST_FILE);
#endif

    if (!usr_input) {
        printf("%s: No user input found in command!\n", module);
        return 1;
    }
    return 0;
}



void enum_node(XMLNode xnode, int dep, int lev)
{
    char buf[256];
    int i, len;
    XMLElementPosition ii = 0;

    // get the name and counts
    const char *name = xnode.getName();
    XMLElementPosition aa = xnode.nAttribute();
    XMLElementPosition nn = xnode.nElement();
    XMLElementPosition cc = xnode.nChildNode();
    XMLElementPosition cl = xnode.nClear();
    XMLElementPosition tt = xnode.nText();
    const char *node = xnode.isDeclaration() ? "decl" : "node";

    len = (int)vXPath.size();
    if (lev && name) {
        i = lev - 1;
        while (len > i) {
            vXPath.pop_back();
            len--;
        }
        vXPath.push_back(name);
    }
    len = (int)vXPath.size();
    std::string path;
    for (i = 0; i < len; i++) {
        if (path.size())
            path += "/";
        path += vXPath[i];
    }
    printf("%d:%d: '%s' %s, %i attr, %i elements, %i child, %i clear, %i text\n", lev, dep,
        name ? name : "none", node,
        aa, nn, cc, cl, tt);
    if (path.size())
        printf(" xPath: %s\n", path.c_str());

    for (ii = 0; ii < nn; ii++)
    {
        XMLNodeContents xnc = xnode.enumContents(ii);
        sprintf(buf," %2i is", (ii + 1));
        switch (xnc.etype)
        {
        case eNodeNULL:
            printf("%s a NULL node\n", buf);
            break;
        case eNodeAttribute:
            printf("%s attr %s=%s\n", buf,
                xnc.attrib.lpszName ? xnc.attrib.lpszName : "null", 
                xnc.attrib.lpszValue ? xnc.attrib.lpszValue : "null");
            break;
        case eNodeChild:
            if (show_child_names)
                printf("%s child '%s'\n", buf, xnc.child.getName());
            break;
        case eNodeText:
            printf("%s text '%s'\n", buf, xnc.text);
            break;
        case eNodeClear:
            if (show_clear_tags) {
                printf("%s clear open '%s'\n", buf, 
                    xnc.clear.lpszOpenTag ? xnc.clear.lpszOpenTag : "none");
                printf("%s clear text '%s'\n", buf, 
                    xnc.clear.lpszValue ? xnc.clear.lpszValue : "null");
                printf("%s clear close '%s'\n", buf, 
                    xnc.clear.lpszCloseTag ? xnc.clear.lpszCloseTag : "none");
            }
            break;
        default:
            printf("%s a type not enum\n", buf);
            break;
        }
    }
}

void enum_children(XMLNode xnode, int dep)
{
    XMLElementPosition ii;
    XMLElementPosition nn = xnode.nChildNode();
    for (ii = 0; ii < nn; ii++) {
        XMLNode xchild = xnode.getChildNode(ii);
        enum_node(xchild, ii+1, dep);
        if (xchild.nChildNode()) {
            enum_children(xchild, dep + 1);
        }
    }
}

int enum_xml(const char *file)
{
    // this open and parse the XML file:
    // XMLNode xMainNode = XMLNode::openFileHelper("bad_file");
    // parse the file
    XMLResults pResults;
    XMLNode xchild;
    XMLNode xnode = XMLNode::parseFile(file, 0, &pResults);
    // display error message (if any)
    if (pResults.error != eXMLErrorNone)
    {
        const char *err = XMLNode::getError(pResults.error);
        printf("Failed file '%s'! '%s'\n", file, err ? err : "Error: Unknown");
        return 1;
    }

    printf("%s: Doing: '%s'...\n", module, file);

    enum_node(xnode,0,0);
    enum_children(xnode,0);

    printf("%s: Done: '%s'...\n", module, file);

    return 0;
}

// main() OS entry
int main( int argc, char **argv )
{
    int iret = 0;
    iret = parse_args(argc,argv);
    if (iret) {
        if (iret == 2)
            iret = 0;
        return iret;
    }

    iret = enum_xml(usr_input); // actions of app

    return iret;
}


// eof = xmlEnum.cxx
