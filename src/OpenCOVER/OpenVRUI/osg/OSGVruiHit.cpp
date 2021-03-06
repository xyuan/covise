/* This file is part of COVISE.

   You can use it under the terms of the GNU Lesser General Public License
   version 2.1 or later, see lgpl-2.1.txt.

 * License: LGPL 2+ */

#include <OpenVRUI/osg/OSGVruiHit.h>
#include <OpenVRUI/osg/OSGVruiNode.h>
#include <OpenVRUI/osg/OSGVruiMatrix.h>
#include <OpenVRUI/sginterface/vruiRendererInterface.h>

#include <osg/Vec3>

using namespace osg;
using namespace osgUtil;

namespace vrui
{

OSGVruiHit::OSGVruiHit(Hit &hit, bool mouseHit)
{
    this->hit = hit;
    this->mouseHit = mouseHit;
    isecLocalPoint = 0;
    isecWorldPoint = 0;
    isecNormal = 0;
    node = 0;
}

OSGVruiHit::~OSGVruiHit()
{
    delete isecLocalPoint;
    delete isecWorldPoint;
    delete isecNormal;
    delete node;
}

coVector &OSGVruiHit::getLocalIntersectionPoint() const
{
    if (!isecLocalPoint)
    {
        isecLocalPoint = new coVector(3);
        Vec3 tmp = hit.getLocalIntersectPoint();
        for (int ctr = 0; ctr < 3; ++ctr)
            isecLocalPoint->vec[ctr] = tmp[ctr];
    }

    return *isecLocalPoint;
}

coVector &OSGVruiHit::getWorldIntersectionPoint() const
{
    if (!isecWorldPoint)
    {
        isecWorldPoint = new coVector(3);
        Vec3 tmp = hit.getWorldIntersectPoint();
        for (int ctr = 0; ctr < 3; ++ctr)
            isecWorldPoint->vec[ctr] = tmp[ctr];
    }

    return *isecWorldPoint;
}

coVector &OSGVruiHit::getIntersectionNormal() const
{
    if (!isecNormal)
    {
        isecNormal = new coVector(3);
        Vec3 tmp = hit._intersectNormal;
        for (int ctr = 0; ctr < 3; ++ctr)
            isecNormal->vec[ctr] = tmp[ctr];
    }

    return *isecNormal;
}

vruiNode *OSGVruiHit::getNode()
{
    if (!node)
    {
        node = new OSGVruiNode(hit._geode.get());
    }
    return node;
}

const Hit &OSGVruiHit::getHit() const
{
    return hit;
}

bool OSGVruiHit::isMouseHit() const
{
    return mouseHit;
}
}
