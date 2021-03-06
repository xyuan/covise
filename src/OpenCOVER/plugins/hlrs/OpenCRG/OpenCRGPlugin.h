/* This file is part of COVISE.

   You can use it under the terms of the GNU Lesser General Public License
   version 2.1 or later, see lgpl-2.1.txt.

 * License: LGPL 2+ */

#ifndef _OPENCRG_PLUGIN_H
#define _OPENCRG_PLUGIN_H
/****************************************************************************\ 
 **                                                            (C)2001 HLRS  **
 **                                                                          **
 ** Description: OpenCRG Plugin (does nothing)                              **
 **                                                                          **
 **                                                                          **
 ** Author: F.Seybold, S. Franz		                                                **
 **                                                                          **
 ** History:  								                                **
 ** Nov-01  v1	    				       		                            **
 **                                                                          **
 **                                                                          **
\****************************************************************************/
#include <cover/coVRPlugin.h>
#include <cover/coTabletUI.h>
#include <cover/coVRShader.h>

#include "opencrg/crgSurface.h"
using namespace opencover;

class OpenCRGPlugin : public coVRPlugin, public coTUIListener
{
public:
    OpenCRGPlugin();
    ~OpenCRGPlugin();

    //initialization
    bool init();

    // this will be called in PreFrame
    void preFrame();

    // this will be called if an object with feedback arrives
    void newInteractor(RenderObject *container, coInteractor *i);

    // this will be called if a COVISE object arrives
    void addObject(RenderObject *container,
                   RenderObject *obj, RenderObject *normObj,
                   RenderObject *colorObj, RenderObject *texObj,
                   osg::Group *root,
                   int numCol, int colorBinding, int colorPacking,
                   float *r, float *g, float *b, int *packedCol,
                   int numNormals, int normalBinding,
                   float *xn, float *yn, float *zn, float transparency);

    // this will be called if a COVISE object has to be removed
    void removeObject(const char *objName, bool replace);

private:
    void tabletEvent(coTUIElement *);

    coTUITab *opencrgTab;
    coTUIFileBrowserButton *openCrgFileButton;
    coTUIFileBrowserButton *shadeRoadSurfaceButton;

    opencrg::Surface *surface;
    osg::Geode *surfaceGeode;
};
#endif
