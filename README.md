# fgsetxml project -   20170419

## Description:

Yet Another Attempt - YAA to parse a FG aircraft *set* file, and show various parts  of the content.

After writing xmlEnum, decided to apply that to a FLightGear [1] aircraft set XML file [2], using the xmpParser library [3], started this fgsetxml project...

A previous attempt, fgxmlset project [4], used the libXML2 library [5], pull XML parsing, but that requires that you keep track of each open, data, close callbacks - WAY too difficult! I had already tried SG so called `easyxml`, [6], but again this PULL parser seemed too difficult, so abandoned it...

Using the xmlParser library [3], I first wrote an `xmlEnum` app, to enumerate through ALL the nodes collected during the xmlParser file load, and this started to show me a way forward with this quite  specialised [aero]-set.xml used in SG/FG... whihc can INLCUDE many other XML files, located in various places, soem of which reuire FG_ROOT to be set (ENV or -r path option). This is the result.

The purpose here is to have a set of default XPaths to find, and show the results Like: `sim/aircraft-version` and show the text `2017.1`, and so on... And that static list can be extended with the `-x xpath` option...

It was more difficult to enumerate all those nodes that begin with `sim/multiplay/generic/float[n]|int[n]|string[n]` since you do **not** know beforehand which will actually exist, so had to develop a special iterator for this - see `void check_multiplay(XMLNode xnode, int dep, vSTG &vX);`.

Could of course add all from the static list in the FG source [7], but that seems **too much**... and must be  updateted when that source is changed, which it did recently...

What I have now seems **very messy**, BUT it WORKS! ;=))

   [1] http://www.flightgear.org/
   [2] http://wiki.flightgear.org/Aircraft-set.xml
   [3] https://www.applied-mathematics.net/tools/xmlparser_doc/html/index.html
        http://www.applied-mathematics.net/tools/xmlParser.html
        source: Download here: small, simple, multi-Plateform XMLParser library with examples (zipfile).
        version: v2.44: May 19, 2013
        license: Aladdin Free Public License (Version 8, November 18, 1999) - see xmlParser/AFPL-license.txt
        copyright: Copyright (c) 2002, Frank Vanden Berghen - All rights reserved.
    [4] https://github.com/geoffmcl/fgxmlset - fgxmlset
    [5] http://www.xmlsoft.org/ - libXML2
    [6] https://sourceforge.net/p/flightgear/simgear/ci/next/tree/simgear/xml/easyxml.cxx - easyxml
    [7] https://sourceforge.net/p/flightgear/flightgear/ci/next/tree/src/MultiPlayer/multiplaymgr.cxx#l128 - FG MP Source

## Building:

The building is using cmake to generate the desired build files.

So for a standard unix makefile build  
$ cd build  
$ cmake .. [-DCMAKE_INSTALL_PREFIX]  
$ make  
$ [sudo] make install (if desired)  

For Windows, use the cmake GUI and set source directory, the binary directory to build, or with say `msbuild` from MSVC...

&gt; cd build  
&gt; cmake .. [-DCMAKE_INSTALL_PREFIX]  
&gt; cmake --build . --config Release  
&gt; cmake --build . --config Release --target INSTALL (if desired)  

Enjoy,  
Geoff.  
20170424


; eof
