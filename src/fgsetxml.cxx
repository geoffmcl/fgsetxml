/*\
 * fgsetxml.cxx
 *
 * Copyright (c) 2015 - Geoff R. McLane
 * Licence: GNU GPL version 2
 *
\*/
/*
   20170419 - fgsetxml project
   Yet Another Attempt - YAA to parse a FG aircraft *set* file, and show various parts
   of the content.

   After writing xmlEnum, decided to apply that to a FLightGear [1] aircraft set XML file [2],
   using the xmpParser library [3] - 

   [1] http://www.flightgear.org/
   [2] http://wiki.flightgear.org/Aircraft-set.xml
   [3] https://www.applied-mathematics.net/tools/xmlparser_doc/html/index.html
        http://www.applied-mathematics.net/tools/xmlParser.html
        source: Download here: small, simple, multi-Plateform XMLParser library with examples (zipfile).
        version: v2.44: May 19, 2013
        license: Aladdin Free Public License (Version 8, November 18, 1999) - see xmlParser/AFPL-license.txt
        copyright: Copyright (c) 2002, Frank Vanden Berghen - All rights reserved.

    A previous attempt, fgxmlset project [4], used the libXML2 library [5], pull XML parsing, but that requires 
    that you keep track of each open, data, close callbacks - WAY too difficult! I had already tried SG so called
    'easyxml', [6], but again this PULL parser seemed too difficult, so abandoned it...

    [4] fgxmlset - https://github.com/geoffmcl/fgxmlset
    [5] libXML2 - http://www.xmlsoft.org/
    [6] easyxml - https://sourceforge.net/p/flightgear/simgear/ci/next/tree/simgear/xml/easyxml.cxx

    Using the above xmlParser library, I first wrote an 'xmlEnum' app, to enumerate through ALL the nodes 
    collected during the xmlParser file load, and this started to show me a way forward with this quite 
    speciaised [aero]-set.xml used in SG/FG... whihc can INLCUDE many other XML files, located in various 
    places, soem of which reuire FG_ROOT to be set (ENV or -r path option). This is the result.

*/

/*
    The purpose here is to have a set of default XPaths to find, and show the results
    Like: 'sim/aircraft-version' and show the text '2017.1', and so on... And that static list can be 
    extended with the '-x xpath' option...

    It was more difficult to enumerate all those nodes that begin with 'sim/multiplay/generic/float[n]|int[n]|string[n]`
    since you do **not** know beforehand which will actually exist, so had to develop a special iterator for this -
    see void check_multiplay(XMLNode xnode, int dep, vSTG &vX);.

    Could of course add all from the static list in the FG source [7], but that seems **too much**... and must be 
    updateted when that source is changed, which it did recently...

    [7] FG MP Source - https://sourceforge.net/p/flightgear/flightgear/ci/next/tree/src/MultiPlayer/multiplaymgr.cxx#l128

    What I have now seems **very messy**, BUT it WORKS! ;=))

*/

#include <stdio.h>
#include <string.h> // for strdup(), ...
#include <algorithm>    // for std::find() in vector
// #include <sstream>
#include "xmlParser.h"
#include "sprtf.hxx"
#include "fgs_utils.hxx"
// other includes

#ifndef SPRTF
#define SPRTF printf
#endif

static const char *module = "fgsetxml";

static const char *usr_input = 0;

static int show_child_names = 0;    // we are enumerating children
static int show_clear_tags = 0; // seems these are comments
static int show_unhandled_paths = 0;    // MAYBE! if required...
static int show_level_0 = 0;
static const char *fg_root = 0; // FG_ROOT...
static const char *def_log = "tempfgset.txt";   // set DEAFAULT log file
static std::string suggested_root;
static const char *mp_list = 0;

static vSTG vIncludes;
static vSTG vDonePaths; // usually full path names
static vSTG vDoneFiles; // usually just the file names
static vSTG vacFiles;   // *.ac found
static vSTG vrgbFiles;  // *.rgb found

#ifndef DEF_TEST_FILE
#define DEF_TEST_FILE "X:\\fgdata\\Aircraft\\c172p\\c172p-set.xml"
#endif

static int verbosity = 0;

#define VERB1   (verbosity >= 1)
#define VERB2   (verbosity >= 2)
#define VERB3   (verbosity >= 3)
#define VERB4   (verbosity >= 4)
#define VERB5   (verbosity >= 5)
#define VERB6   (verbosity >= 6)
#define VERB7   (verbosity >= 7)
#define VERB8   (verbosity >= 8)
#define VERB9   (verbosity >= 9)

#ifndef ISDIGIT
#define ISDIGIT(a) ((a >= '0') && (a <= '9'))
#endif

static std::string active_path;
static vSTG vActPaths;
// DEFAULTS
static mSTGvSTG xpaths;
const char *xpath_defaults[] = {
    "sim/description",
    "sim/aircraft-version",
    "sim/author",
    "sim/aero",
    "sim/flight-model",
    "sim/status",
    "sim/tags/tag",
    "sim/rating/FDM",
    "sim/rating/systems",
    "sim/rating/model",
    "sim/rating/cockpit",
    // terminator
    0
};

// fwd refs
int process_include_file(const char *ifile);
void enum_children(XMLNode xnode, int dep, vSTG &vX);

/*
    Convert the above 'default' list to a map of 
    xpaths, each containing a vSTG *...
*/
void init_xpaths()
{
    int i;
    std::string s;
    for (i = 0; ; i++) {
        if (!xpath_defaults[i])
            break;
        s = xpath_defaults[i];
        xpaths[s] = new vSTG;
    }
}

void show_default_xpaths()
{
    int i;
    std::string s;
    for (i = 0; ; i++) {
        if (!xpath_defaults[i])
            break;
        s = xpath_defaults[i];
        SPRTF("  %s\n", s.c_str());
    }
}


void add_to_vSTG_ptr(vSTG *vsp, std::string v)
{
    if (vsp) {
        if (vsp->size()) {
            if (v.size()) { // can be a blank - see G:\D\fgaddon\Aircraft\737-300\737-300-set.xml
                // only if this string VALUE is NOT already in the string vector
                if (std::find(vsp->begin(), vsp->end(), v) == vsp->end()) {
                    // add a NEW value
                    if (vsp->at(0).size())  // check string has size
                        vsp->at(0) += ", "; // ok, add comma
                    vsp->at(0) += v;
                }
            }
        }
        else {
            // see G:\D\fgaddon\Aircraft\737-300\737-300-set.xml
            vsp->push_back(v);  // NOTE: This can be a 'blank', like <string n = "0"/>
        }
    }
}

vSTG *add_to_xpaths(std::string n, std::string v)
{
    vSTG *vsp = 0;
    iSTGvSTG it = xpaths.find(n);
    if (it != xpaths.end()) {
        vsp = it->second;
    }
    else {
        vsp = new vSTG;
        xpaths[n] = vsp;
    }
    add_to_vSTG_ptr(vsp, v);
    return vsp;
}

void clear_xpaths()
{
    vSTG *vsp;
    iSTGvSTG it;
    for (it = xpaths.begin(); it != xpaths.end(); it++) {
        vsp = it->second;
        delete vsp;
    }
    xpaths.clear();
}

vSTG *path_in_xpaths(std::string path)
{
    iSTGvSTG it = xpaths.find(path);
    if (it != xpaths.end())
        return it->second;
    return 0;
}

void show_xpaths(const char *in_file)
{
    std::string n, v;
    vSTG *vsp;
    iSTGvSTG it;
    size_t ii, max, maxlen = 0;
    vSTG missed;
    int count = 0;
    // std::stringstream ss;

    ii = 0;
    for (it = xpaths.begin(); it != xpaths.end(); it++) {
        n = it->first;
        vsp = it->second;
        max = vsp->size();
        if (max) {
            max = n.size();
            if (max > maxlen)
                maxlen = max;
            ii++;
        }
        else {
            missed.push_back(n);
        }
    }
    SPRTF("Show of %d of %d xpaths found, and their values...\n", (int)ii, (int)xpaths.size());
    vOFF vO;
    size_t off = 0;
    for (it = xpaths.begin(); it != xpaths.end(); it++) {
        vsp = it->second;
        max = vsp->size();
        if (max) {
            n = it->first;
            std::string::size_type pos = n.find("sim/multiplay/generic", 0);
            if (pos != std::string::npos) {
                count++;
                vO.push_back(off);
            }
            while (n.size() < maxlen)
                n += " ";
            // with a recent change, technically this should ALWAYS only ever be ONE!
            for (ii = 0; ii < max; ii++) {
                v = vsp->at(ii);
                SPRTF("%s: %s\n", n.c_str(), v.c_str());
            }
        }
        off++;
    }
    ////////////////////////////////////////////////////////////////////////////
    //// very specialized! Seeking 'sim/multiplay/generic' entries only
    if (mp_list) {
        FILE *fp = fopen(mp_list, "a");
        if (fp) {
            size_t jj, cnt = vO.size();
            jj = fprintf(fp, "[%s]%d\n", in_file, (int)count);
            if (jj < 0) {
                SPRTF("%s: Failed write mp file '%s'\n", module, in_file);
            }
            for (jj = 0; jj < cnt; jj++) {
                off = vO[jj];
                // it = (xpaths.begin() + off);    // hmmm, why does this not work????
                it = xpaths.begin();    // oh, well, do it the hard way...
                while (off && (it != xpaths.end())) {
                    it++;
                    off--;
                }
                if (it != xpaths.end()) {
                    vsp = it->second;
                    max = vsp->size();
                    if (max) {
                        n = it->first;
                        // with a recent change, technically this should ALWAYS only ever be ONE!
                        for (ii = 0; ii < max; ii++) {
                            v = vsp->at(ii);
                            fprintf(fp,"%s: %s\n", n.c_str(), v.c_str());
                        }
                    }
                }
            }
            fclose(fp);
        }
    }
    ////////////////////////////////////////////////////////////////////////////

    max = missed.size();
    if (VERB1 && max) {
        SPRTF("Missed finding %d xpaths...\n", (int)max);
        for (ii = 0; ii < max; ii++) {
            n = missed[ii];
            SPRTF(" %s\n", n.c_str());
        }
    }
}

void add_2_active_path(std::string path)
{
    ensure_native_sep(path);
    active_path = path;
    size_t ii, max = vActPaths.size();
    std::string s;
    for (ii = 0; ii < max; ii++) {
        s = vActPaths[ii];
        if (s == path)
            return;
    }
    vActPaths.push_back(path);
    //////////////////////////////////////////////////////////
    //// If the path is like 'X:\fgdata\Aircraft\c172p'
    //// also add 'X:\fgdata' to active list
    std::string::size_type pos = path.find("Aircraft", 2);
    if (pos != std::string::npos) {
        vSTG vs = PathSplit(path);
        max = vs.size();
        if (max > 2) {  // path has MORE than 2 components
            std::string np; // get path up 'Aircraft' if it exists
            for (ii = 0; ii < max; ii++) {
                s = vs[ii]; // get component
                if (s == "Aircraft")
                    break;  // stop at aircraft
                if (ii) // if nore than one
                    add_path_sep(np);   // add PATH_SEP
                np += s;    // add component
            }
            if (ii && (ii < max)) {
                max = vActPaths.size(); // get current active PATHS
                for (ii = 0; ii < max; ii++) {
                    s = vActPaths[ii];  // extract
                    if (s == np)    // already have this one...
                        return; // out of here
                }
                vActPaths.push_back(np);    // store this new PATH
            }
        }
    }
    //////////////////////////////////////////////////////////
}

int add_2_rgb_files(std::string file)
{
    size_t ii, max = vrgbFiles.size();
    std::string s;
    for (ii = 0; ii < max; ii++) {
        s = vrgbFiles[ii];
        if (s == file)
            return 0;
    }
    vrgbFiles.push_back(file);
    return 1;
}
int add_2_wav_files(std::string file)
{
    return add_2_rgb_files(file);
}

int add_2_ac_files(std::string file)
{
    size_t ii, max = vacFiles.size();
    std::string s;
    for (ii = 0; ii < max; ii++) {
        s = vacFiles[ii];
        if (s == file)
            return 0;
    }
    vacFiles.push_back(file);
    return 1;
}


void show_ac_files()
{
    size_t ii, max = vacFiles.size();
    std::string s;
    SPRTF("%s: Found %d *.ac files in xml...\n", module, (int)max);
    for (ii = 0; ii < max; ii++) {
        s = vacFiles[ii];
        SPRTF(" %s\n", s.c_str());
    }
}

void show_version()
{
    SPRTF("%s date %s version %s\n", module, FGSETX_DATE, FGSETX_VERSION);
}

void give_help( char *name )
{
    show_version();
    SPRTF("\n");
    SPRTF("%s: usage: [options] xml_input\n", module);
    SPRTF("Options:\n");
    SPRTF(" --help   (-h or -?) = This help and exit(0)\n");
    SPRTF(" --verb[n]      (-v) = Bump or set verbosity to n. Values 0,1,...9 (def=%d)\n", verbosity);
    SPRTF(" --root <dir>   (-r) = Set the FG_ROOT directory. (def %s)\n",
        fg_root ? fg_root : "Not set");
    SPRTF(" --log <file>   (-l) = Set log output file. Use 'none' to disable. (def=%s)", def_log);
    SPRTF(" --mp <file>    (-m) = Append 'sim/multiplay/generic' props to this file. (def=%s)\n",
        mp_list ? mp_list : "none");
    SPRTF(" --xpath <path> (-x) = Add a seekable xpath to find and display.\n");
    SPRTF("\n");
    SPRTF(" In essence, given a FlightGear 'aero-set.xml' file, see 'http://wiki.flightgear.org/Aircraft-set.xml',\n");
    SPRTF(" find, show the default set of xpaths. The default set consists of -\n");
    show_default_xpaths();
    SPRTF("\n");
    SPRTF(" The above '-x path' option allows other specific paths to be added. In addition special\n");
    SPRTF(" MP paths of the form 'sim/multiplay/generic/float[n]|int[n]|string[n]' will be enumerated,\n");
    SPRTF(" and if desired 'appended' to any '-m file' given.\n");
    SPRTF("\n");
    SPRTF(" These 'aero-set.xml files can include other xml files, sometimes from the FG_ROOT directory,\n");
    SPRTF(" so it may be necessary to either set FG_ROOT in the environmnet, of using the '-r path' option.\n");
    SPRTF("\n");
    SPRTF(" This app uses the 'xmlParser' library from 'http://www.applied-mathematics.net/tools/xmlParser.html',\n");
    SPRTF(" to do the actual XML loading. My thanks to 'Frank Berghen' for this fast efficient parser. So far I\n");
    SPRTF(" I have not made any corrections to this 2013 code.\n");


}

int chk_log_file(int argc, char **argv)
{
    int i, i2, c;
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
            case 'l':
                if (i2 < argc) {
                    i++;
                    sarg = argv[i];
                    def_log = strdup(sarg);
                }
                else {
                    SPRTF("Error: Expected log file name to follow '%s'!", arg);
                    return 1;
                }
            }
        }
    }
    set_log_file((char *)def_log, 0);
    return 0;
}

/*
    Parse the command line
*/
int parse_args( int argc, char **argv )
{
    int i,i2,c;
    char *arg, *sarg;
    if (chk_log_file(argc, argv))
        return 1;
    for (i = 1; i < argc; i++) {
        arg = argv[i];
        //////////////////////////////////////////////////////////////
        // always accept a '--version' argument, show and exit(0)!
        if (strcmp(arg, "--version") == 0) {
            show_version();
            return 2;
        }
        //////////////////////////////////////////////////////////////
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
            case 'v':
                verbosity++;
                sarg++;
                while (*sarg)
                {
                    if (ISDIGIT(*sarg)) {
                        verbosity = atoi(sarg);
                        break;
                    }
                    if (*sarg == 'v')
                        verbosity++;
                    sarg++;
                }
                break;
            case 'r':
                if (i2 < argc) {
                    i++;
                    sarg = argv[i];
                    fg_root = strdup(sarg);
                }
                else {
                    SPRTF("%s: Expected FG_ROOT dir to follow %s!", module, arg);
                    return 1;
                }
                break;
            case 'l':
                i++;    // already dealt with
                break;
            case 'm':
                if (i2 < argc) {
                    i++;
                    sarg = argv[i];
                    mp_list = strdup(sarg);
                }
                else {
                    SPRTF("%s: Expected file name to follow %s!", module, arg);
                    return 1;
                }
                break;
            case 'x':
                if (i2 < argc) {
                    i++;
                    sarg = argv[i];
                    add_to_xpaths(sarg, "");
                }
                else {
                    SPRTF("%s: Expected xpath to follow %s!", module, arg);
                    return 1;
                }
                break;
            default:
                SPRTF("%s: Unknown argument '%s'. Try -? for help...\n", module, arg);
                return 1;
            }
        } else {
            // bear argument
            if (usr_input) {
                SPRTF("%s: Already have input '%s'! What is this '%s'?\n", module, usr_input, arg );
                return 1;
            }
            usr_input = strdup(arg);
        }
    }

#if !defined(NDEBUG)
    if (!usr_input) {
        usr_input = strdup(DEF_TEST_FILE);
        // verbosity = 9;
    }
#endif

    if (!usr_input) {
        SPRTF("%s: No user input found in command!\n", module);
        return 1;
    }
    return 0;
}

int add_2_includes(const char *nf)
{
    std::string file = nf;
    std::string s;
    size_t ii, max = vIncludes.size();
    for (ii = 0; ii < max; ii++) {
        s = vIncludes[ii];
        if (s == file)
            return 0;
    }
    vIncludes.push_back(file);
    return 1;   // new file added
}

/*
    Enumerate one XMLNode
*/
void enum_node(XMLNode xnode, int dep, int lev, vSTG &vX)
{
    char buf[256];
    int i, len;
    XMLElementPosition ii = 0;
    const char *n;
    const char *v;
    std::string tmp;
    bool isPath = false;    // found ......<data>... text ... can be XML to include, or an AC file

    // get the name and counts
    const char *name = xnode.getName();
    XMLElementPosition aa = xnode.nAttribute();
    XMLElementPosition nn = xnode.nElement();
    XMLElementPosition cc = xnode.nChildNode();
    XMLElementPosition cl = xnode.nClear();
    XMLElementPosition tt = xnode.nText();

    len = (int)vX.size();
    if (lev && name) {
        i = lev - 1;
        while (len > i) {
            vX.pop_back();
            len--;
        }
        vX.push_back(name);
        if (strcmp(name, "path") == 0)
            isPath = true;
    }
    len = (int)vX.size();
    std::string path;
    for (i = 0; i < len; i++) {
        if (path.size())
            path += "/";
        path += vX[i];
    }
    if (len && name) {
        for (ii = 0; ii < nn; ii++)
        {
            XMLNodeContents xnc = xnode.enumContents(ii);
            if (xnc.etype == eNodeAttribute) {
                n = xnc.attrib.lpszName;
                v = xnc.attrib.lpszValue;
                if (n && v) {
                    if (strcmp(n, "n") == 0) {
                        tmp = "[";
                        tmp += v;
                        tmp += "]";
                        path += tmp;
                        vX[len - 1] += tmp;
                    }
                }
            }
        }
    }

    if ((lev == 0) && !show_level_0)
        return;

    vSTG *vsp = path_in_xpaths(path);

    if (VERB5) {
        SPRTF("%d:%d: '%s' node, %i attr, %i ele, %i child, %i clear, %i text\n", lev, dep,
            name ? name : "none",
            aa, nn, cc, cl, tt);
    }

    if (VERB6) {
        SPRTF("XPath: '%s'\n", path.c_str());
        //if (path == "sim/menubar")
        if (path == "sim/help/text")
            tt = 0;
    }
    for (ii = 0; ii < nn; ii++)
    {
        XMLNodeContents xnc = xnode.enumContents(ii);
        sprintf(buf," %2i is", (ii + 1));
        switch (xnc.etype)
        {
        case eNodeNULL:
            if (VERB9)
                SPRTF("%s a NULL node\n", buf);
            break;
        case eNodeAttribute:
            n = xnc.attrib.lpszName;
            v = xnc.attrib.lpszValue;
            if (VERB7) {
                SPRTF("%s attr %s=%s\n", buf,
                    n ? n : "null",
                    v ? v : "null");
            }
            if (n && v) {
                if (strcmp(n, "include") == 0) {
                    if (i = process_include_file(v)) {
                        add_2_includes(v);  // show missing later
                        if (VERB9)
                            SPRTF("%s: Failed to find include '%s'\n", module, v);
                    }
                }
            }
            break;
        case eNodeChild:
            if (show_child_names)
                SPRTF("%s child '%s'\n", buf, xnc.child.getName());
            break;
        case eNodeText:
            if (VERB6) {
                if (strlen(xnc.text) > MX_SPRTF_TEXT) {
                    SPRTF("%s text ", buf);
                    direct_out_it((char *)xnc.text);
                    SPRTF("\n");
                }
                else {
                    SPRTF("%s text '%s'\n", buf, xnc.text);
                }
            }
            if (vsp) {
                add_to_vSTG_ptr(vsp, xnc.text);
            }
            if (isPath) {   // .../.../<path>... text
                ////////////////////////////////////////////////////////////////
                //// get the extension, if any
                tmp = get_extension(xnc.text);
                if (tmp.size()) {
                    if (tmp == ".xml") {
                        if (i = process_include_file(xnc.text)) {
                            add_2_includes(xnc.text);   // show missed later
                            if (VERB9)
                                SPRTF("%s: Failed to find include '%s'\n", module, xnc.text);
                        }
                    }
                    else if (tmp == ".rgb") {
                            add_2_rgb_files(xnc.text);
                    }
                    else if (tmp == ".wav") {
                        add_2_wav_files(xnc.text);
                    }
                    else if (tmp == ".ac") {
                        add_2_ac_files(xnc.text);
                    }
                    else {
                        if (VERB9) {
                            SPRTF("%s:[v9] Extension '%s', path '%s', NOT handled.\n", module, tmp.c_str(), xnc.text);
                        }
                    }
                }
                else {
                    if (show_unhandled_paths && VERB9) {
                        SPRTF("%s: path '%s', NOT handled.\n", module, xnc.text);
                    }
                }
            }
            break;
        case eNodeClear:
            if (show_clear_tags) {   // These seem to be COMMENTS only
                SPRTF("%s clear open '%s'\n", buf,
                    xnc.clear.lpszOpenTag ? xnc.clear.lpszOpenTag : "none");
                SPRTF("%s clear text '%s'\n", buf,
                    xnc.clear.lpszValue ? xnc.clear.lpszValue : "null");
                SPRTF("%s clear close '%s'\n", buf,
                    xnc.clear.lpszCloseTag ? xnc.clear.lpszCloseTag : "none");
            }
            break;
        default:
            SPRTF("%s CHECK ME: a type %i not enum!\n", buf, xnc.etype);
            break;
        }
    }
}

/*
    Special check for 'sim/multiplay/generic' nodes
*/
void check_multiplay(XMLNode xnode, int dep, vSTG &vX)
{
    // xchild = xnode.getChildNode("does_not_exist");
    // Deal with sim/multiplay/generic
    std::string base = "sim/multiplay/generic/";
    std::string path, alias;
    const char *n;
    const char *v;
    const char *nm;
    int count = 0;
    if (xnode.isEmpty()) {
        SPRTF("%s: CHECK ME - check_multiplay passed EMPTY xnode!\n", module);
        return;
    }
    XMLNode xchild = xnode.getChildNode("sim");
    if (!xchild.isEmpty()) {
        xchild = xchild.getChildNode("multiplay");
        if (!xchild.isEmpty()) {
            xchild = xchild.getChildNode("generic");
            if (!xchild.isEmpty()) {
                XMLElementPosition ii, jj, cc = xchild.nChildNode();
                if (VERB6)
                    SPRTF("%s:[v6] Got node 'sim/multiplay/generic'... with %i children...\n", module, cc);

                for (ii = 0; ii < cc; ii++) {
                    //XMLNodeContents xnc = xchild.enumContents(ii);
                    XMLNode xc = xchild.getChildNode(ii);   // get CHILD
                    const char *name = xc.getName();
                    XMLElementPosition aa = xc.nAttribute();
                    XMLElementPosition nn = xc.nElement();
                    //XMLElementPosition cc = xnode.nChildNode();   // should be NONE
                    //XMLElementPosition cl = xc.nClear();
                    //XMLElementPosition tt = xc.nText();
                    if (name) {
                        path = base;
                        path += name;
                        alias = "";
                        for (jj = 0; jj < nn; jj++) {
                            XMLNodeContents xnc = xc.enumContents(jj);

                            if (xnc.etype == eNodeChild) {
                                //nm = xc.getName();
                                nm = xnc.child.getName();
                            }
                            else if (xnc.etype == eNodeAttribute) {
                                n = xnc.attrib.lpszName;
                                v = xnc.attrib.lpszValue;
                                if (strcmp(n, "n") == 0) {
                                    path += "[";
                                    path += v;
                                    path += "]";
                                }
                                else if (strcmp(n, "alias") == 0) {
                                    if (alias.size())
                                        alias += ", ";
                                    // alias += "alias=";
                                    alias += v;
                                }
                            }
                            else if (xnc.etype == eNodeText) {
                                if (alias.size())
                                    alias += ", ";
                                // alias += "text=";
                                alias += xnc.text;
                            }
                        }
                        // to even add this
                        //  <sim><multiplay><generic><string n = "0" / >
                        // forget this - if (alias.size()) - add them all
                        {
                            vSTG *vsp = add_to_xpaths(path, alias);
                            count++;
                            alias = "";
                        }
                    }
                }
                if (cc && !count) {
                    SPRTF("%s: CHECK ME: Add no xpaths added, but total %d! input %s\n", module, (int)cc, usr_input);
                }
                else if (count < (int)cc) {
                    SPRTF("%s: CHECK ME2: Added %d xpaths, but total of %d! input %s\n", module, count, (int)cc, usr_input);
                }
            }
        }
    }
}

/*
    Enumerate the CHILDREN of an XMLNode
*/
void enum_children(XMLNode xnode, int dep, vSTG &vX)
{
    XMLElementPosition ii;
    XMLElementPosition nn = xnode.nChildNode();
    if (nn)
        check_multiplay(xnode, dep, vX);
    for (ii = 0; ii < nn; ii++) {
        XMLNode xchild = xnode.getChildNode(ii);
        enum_node(xchild, ii+1, dep, vX);
        if (xchild.nChildNode()) {
            enum_children(xchild, dep + 1, vX);
        }
    }
}

/*
    Check if a file name is in the 'vDoneFiles' vector
*/
int in_done_files(std::string file)
{
    size_t ii, max = vDoneFiles.size();
    std::string s;
    for (ii = 0; ii < max; ii++) {
        s = vDoneFiles[ii];
        if (s == file)
            return 1;
    }
    return 0;
}

/*
    Check if a file path is in the 'vDonePaths' vector
*/
int in_done_paths(std::string file)
{
    size_t ii, max = vDonePaths.size();
    std::string s;
    for (ii = 0; ii < max; ii++) {
        s = vDonePaths[ii];
        if (s == file)
            return 1;
    }
    return 0;
}

/*
    Output the list of DONE files
*/
void show_done_paths()
{
    if (VERB1) {
        size_t ii, max = vDonePaths.size();
        size_t ac = vacFiles.size();
        size_t med = vrgbFiles.size();
        std::string s;
        const char *show = (VERB2) ? "listed..." : "-v2 to list";
        SPRTF("%s:[v1] Done %d xml files... noted %d *.ac file, %d media (rgb/wav)... %s\n", module, (int)max, (int)ac, (int)med, show);
        if (VERB2) {
            for (ii = 0; ii < max; ii++) {
                s = vDonePaths[ii];
                SPRTF(" %2d: %s\n", (int)(ii + 1), s.c_str());
            }
        }
    }
}

/*
    Potentially add a new file name to 'vDoneFiles'
*/
int add_2_done_files(std::string file)
{
    if (in_done_files(file))
        return 0;
    vDoneFiles.push_back(file);
    return 1;
}


/*
    Potentially add a new file path to 'vDonePaths'
*/
int add_2_done_paths(std::string file)
{
    ensure_native_sep(file);
    if (in_done_paths(file))
        return 0;
    vDonePaths.push_back(file);
    return 1;
}

/*
    Primary service, to load and enumerate an XML file
*/
int enum_xml(const char *file)
{
    std::string ff = file;
    ensure_native_sep(ff);
    std::string fn = get_file_only(ff);
    if (in_done_paths(ff)) {
        if (VERB9)
            SPRTF("%s: Avoided duplicate load of path '%s'...\n", module, ff.c_str());
        return 0;
    }
    if (in_done_files(fn)) {    // maybe this is EXCESSIVE
        // The SAME file name on a DIFFERENT path, should be a different file!!!
        if (VERB9)
            SPRTF("%s: Avoided duplicate load of file '%s' - CHECK ME!\n", module, ff.c_str());
        return 0;

    }

    add_2_done_paths(ff);
    add_2_done_files(fn);

    // this open and parse the XML file:
    // XMLNode xMainNode = XMLNode::openFileHelper("bad_file");
    // parse the file
    XMLResults pResults;
    // XMLNode xchild;
    XMLNode xnode = XMLNode::parseFile(file, 0, &pResults);
    // display error message (if any)
    if (pResults.error != eXMLErrorNone)
    {
        const char *err = XMLNode::getError(pResults.error);
        SPRTF("Failed file '%s'! '%s'\n", file, err ? err : "Error: Unknown");
        return 1;
    }
    if (VERB3) {
        SPRTF("%s:[v3] Doing file: '%s'...\n", module, file);
    }
    add_2_active_path(get_path_only(ff));

    vSTG vX;    // init an XPath for this file
    enum_node(xnode,0,0,vX);    // maybe not required, since it is only the declaration
    enum_children(xnode,0,vX);  // walk the tree of children

    //     xNode=xMainNode.getChildNode("RegressionModel").getChildNode("RegressionTable");
    if (VERB7) {
        SPRTF("%s:[v7] Done file: '%s'...\n", module, file);
    }
    return 0;
}

/*
    Check the environment for FG_ROOT
*/
void set_fg_root()
{
    char *env = getenv("FG_ROOT");
    if (env)
        fg_root = strdup(env);
}

/*
    process_include_file(file)

    Seems QUITE difficult, and messy to find an include file!!!
    An example fgsetxml: Fail to find 'Aircraft/747-400/Systems/autopilot.xml'
    when processing G:\D\fgaddon\Aircraft\747-400\747-400-set.xml
    when in the folder G:\D\fgaddon\Aircraft\747-400
    Simply drop the Aircraft\747-400 off the include name...
*/
int process_include_file(const char *ifile)
{
    int iret = 0;
    std::string sf = ifile;
    std::string path = get_path_only(sf);
    std::string file = get_file_only(sf);
    bool done = false;
    if ((ifile[0] == '/') || (ifile[0] == '\\'))
        sf = &ifile[1];
    ensure_native_sep(sf);
    std::string ff = sf;
    if (is_file_or_directory(ff.c_str()) == 1) {
        iret |= enum_xml(ff.c_str());
        done = true;
    }
    if (!done && active_path.size()) {
        ff = active_path;
        add_path_sep(ff);
        ff += sf;
        if (is_file_or_directory(ff.c_str()) == 1) {
            iret |= enum_xml(ff.c_str());
            done = true;
        }
    }
    if (!done && active_path.size()) {
        ff = active_path;
        add_path_sep(ff);
        ff += file;
        if (is_file_or_directory(ff.c_str()) == 1) {
            iret |= enum_xml(ff.c_str());
            done = true;
        }
    }
    size_t ii, max = vActPaths.size();
    if (!done) {
        for (ii = 0; ii < max; ii++) {
            ff = vActPaths[ii];
            if (ff == active_path)
                continue;
            add_path_sep(ff);
            ff += sf;
            if (is_file_or_directory(ff.c_str()) == 1) {
                iret |= enum_xml(ff.c_str());
                done = true;
                break;
            }
        }
    }
    if (!done && max) {
        for (ii = 0; ii < max; ii++) {
            ff = vActPaths[ii];
            if (ff == active_path)
                continue;
            add_path_sep(ff);
            ff += file;
            if (is_file_or_directory(ff.c_str()) == 1) {
                iret |= enum_xml(ff.c_str());
                done = true;
            }
        }
    }
    if (!done && fg_root) {
        ff = fg_root;
        add_path_sep(ff);
        ff += sf;
        if (is_file_or_directory(ff.c_str()) == 1) {
            iret |= enum_xml(ff.c_str());
            done = true;
        }
    }
    if (!done && suggested_root.size()) {
        if (!fg_root || (fg_root && strcmp(fg_root, suggested_root.c_str()))) {
            ff = suggested_root;
            add_path_sep(ff);
            ff += sf;
            if (is_file_or_directory(ff.c_str()) == 1) {
                iret |= enum_xml(ff.c_str());
                done = true;
            }
        }
    }

    // maybe fixed??? And this is NOT required - NOPE, can still fail
    if (!done) {
        vSTG vs = PathSplit(sf);
        max = vs.size();
        if ((max > 2) && (vs[0] == "Aircraft")) {
            // might have 'Aircraft/747-400/Systems/autopilot.xml'
            ff = "";
            for (ii = 2; ii < max; ii++) {
                if (ff.size())
                    add_path_sep(ff);
                ff += vs[ii];
            }
            iret = process_include_file(ff.c_str());
            if (iret == 0)
                done = true;
        }
    }
    /*
        TODO: If it is NOT found, what else can be done?
        assuming the file does exist...
    */
    if (!done)
        iret = 2;

    return iret;
}
////////////////////////////////////////////////////////////////////
//void test_gp(const char *file) {
//    std::string path = get_path_only(std::string(file));
//    exit(2); }

/*
    Using an existing file name, add a suggested root path
*/

void set_suggested_root(const char *file)
{
    if (is_file_or_directory(file) == 1) {
        std::string path = file;
        vSTG vs = PathSplit(path);
        size_t ii, max = vs.size();
        std::string s, tmp, ac("Aircraft");
        for (ii = 0; ii < max; ii++ ) {
            tmp = vs[ii];
            if (tmp == ac) {
                suggested_root = s;
                break;
            }
            if (s.size())
                add_path_sep(s);
            s += tmp;
        }
    }
    else {
        SPRTF("%s: Warning: Can NOT 'stat' file '%s'!\n", module, file);
    }

}

/*
    Show any **missed** include files, with 
    an aim to learn how to find it in future
*/
void show_missed_includes(size_t missed)
{
    // if (VERB9)
    // show if MISSED finding some files...
    {
        std::string ff;
        size_t ii, max = vIncludes.size();
        if (missed)
            SPRTF("%s: Note failed to find %d files...\n", module, (int)missed);
        if (suggested_root.size())
            SPRTF("%s: Got suggested-root=%s\n", module, suggested_root.c_str());
        if (fg_root)
            SPRTF("%s: Given FG_ROOT=%s\n", module, fg_root);
        max = vActPaths.size();
        SPRTF("%s: Have %d active paths...\n", module, (int)max);
        for (ii = 0; ii < max; ii++) {
            ff = vActPaths[ii];
            SPRTF(" %s\n", ff.c_str());
        }
    }
}

/*
    Some cleanup on exit...
*/
void clean_up()
{
    // clear all vSTG vectors
    vIncludes.clear();
    vDonePaths.clear(); // usually full path names
    vDoneFiles.clear(); // usually just the file names
    vActPaths.clear();
    vacFiles.clear();
    vrgbFiles.clear();
    clear_xpaths();

}

/*
    main() OS entry
*/
int main(int argc, char **argv)
{
    //test_gp("xmlParser.sln");
    int iret = 0;
    size_t ii, max;
    set_fg_root();
    init_xpaths();
    iret = parse_args(argc, argv);
    if (iret) {
        if (iret == 2)
            iret = 0;
        goto exit;
    }
    set_suggested_root(usr_input);
    iret = enum_xml(usr_input); // actions of app
    if (!VERB5) {
        SPRTF("%s: Done file: '%s'...\n", module, usr_input);
    }

    //////////////////////////////////////////////////////
    /// Deal with the MISSING INCLUDES
    //////////////////////////////////////////////////////
    max = vIncludes.size();
    if (max) {
        // *** ALWAYS show when missed finding some include files ***
        SPRTF("%s: Failed to find %d include files...\n", module, (int)max);
        for (ii = 0; ii < max; ii++) {
            std::string nf = vIncludes[ii];
            SPRTF("%s: Fail to find '%s'\n", module, nf.c_str());
        }
        if (fg_root)
            SPRTF("%s: Is the given FG_ROOT=%s correct!\n", module, fg_root);
        else
            SPRTF("%s: Note: No FG_ROOT (-r dir) given. Some of these files could reside there...\n", module);
        show_missed_includes(max);    // ADD diagnostics in hope to find out ***WHY*** missed these files
        // somehow, expect to FIND THEM ALL!!! But in all recent cases explored, it seems the file
        // truely DOES NOT EXIST in the current source...
    }
    //////////////////////////////////////////////////////

    show_done_paths();  // list the DONE files... if VERB1

    if (VERB2) {
        show_ac_files();
    }

    show_xpaths(usr_input);

exit:
    clean_up();
    return iret;
}

// eof = fgsetxml.cxx

