/* This file is part of COVISE.

   You can use it under the terms of the GNU Lesser General Public License
   version 2.1 or later, see lgpl-2.1.txt.

 * License: LGPL 2+ */

#ifndef VR_VIEWER_H
#define VR_VIEWER_H

/*! \file
 \brief  tracked stereo viewer class

 \author Daniela Rainer
 \author Uwe Woessner <woessner@hlrs.de> (osgProducer port)
 \author Martin Aumueller <aumueller@uni-koeln.de> (osgViewer port)
 \author (C) 1996
         Computer Centre University of Stuttgart,
         Allmandring 30,
         D-70550 Stuttgart,
         Germany

 \date   20.08.1997
 \date   2004 (osgProducer)
 \date   2007 (osgViewer)
 */

#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>

#include <util/common.h>

#include <osg/Vec3>
#include <osg/Vec4>
#include <osg/Matrix>

namespace osg
{
class Group;
class DisplaySettings;
}

namespace opencover
{
class buttonSpecCell;

class coVRSceneHandler;
class coVRSceneView;
class MSEventHandler;
class ARToolKitMarker;
class angleStruct;

class COVEREXPORT VRViewer : public osgViewer::Viewer
{
    friend class OpenCOVER;
public:
    void setUpRenderingSupport();

    /** Updated the scene.  Handle any queued up events, do an update traversal and set the CameraGroup's setViewByMatrix if any camera manipulators are active.*/
    virtual void update();

    /** Dispatch the cull and draw for each of the Camera's for this frame.*/
    virtual void frame();

    void handleEvents();

    void redrawHUD(double interval);

    static int unsyncedFrames;

    /** Start any threads required by the viewer.*/
    virtual void startThreading();

    void addCamera(osg::Camera *camera);
    void removeCamera(osg::Camera *camera);
    void addPreRenderCamera(osg::Camera *camera);
    void removePreRenderCamera(osg::Camera *camera);

    void setClearColor(const osg::Vec4 &color);
    void setRenderToTexture(bool);
    osg::Texture2D *getTexture()
    {
        return renderTargetTexture;
    };
    void flipStereo();

private:
    MSEventHandler *myeh;
    // channel needs pointer to scene
    osg::ref_ptr<osg::Group> scene;
    osg::ref_ptr<osgViewer::Scene> PreRenderScene;

    // stereo parameters
    int isQuadbufferStereo;
    char stereoCommand[500];
    char monoCommand[500];
    int stereoMode, oldMode; // MONO, RE_STEREO, IMPACT_STEREO, TB_STEREO
    int stereoModeChange;
    double lastFrameTime;

    float optDist, dist;
    virtual void renderingTraversals();

    // view
    osg::Vec3 viewPos, viewDir; //, viewYDir;
    osg::Vec3 initialViewPos;
    osg::Vec3 leftViewPos, rightViewPos, middleViewPos;
    osg::Matrix viewMat;
    osgViewer::StatsHandler *statsHandler;

    osg::Texture2D *renderTargetTexture;
    bool RenderToTexture;

    angleStruct *screen_angle; // Screen angle: IWR movable screen

    void readConfigFile();

    void detectStereoMode();
    void setStereoMode();

    void createChannels(int i);

    int isHeadtracking;
    bool fixViewer;
    osg::Vec4 backgroundColor;

    bool arTracking;
    ARToolKitMarker *vpMarker;
    bool overwritePAndV;
    bool reEnableCulling;
    
    osg::Geode *distortionMesh(const char *fileName);
    void createViewportCameras(int i);
    void createBlendingCameras(int i);
    float requestedSeparation, separation;
    int animateSeparation;
    bool stereoOn;

public:
    void setFrustumAndView(int i);

    static VRViewer *instance();
    void setSeparation(float stereoSep);
    std::list<osg::ref_ptr<osg::Camera> > myCameras;
    std::list<osg::ref_ptr<osg::Camera> > myPreRenderCameras;

    VRViewer();

    virtual ~VRViewer();

    void config();

    virtual void getCameras(Cameras &cameras, bool onlyActive = true);
    virtual void getContexts(Contexts &contexts, bool onlyValid = true);
    virtual void getScenes(Scenes &scenes, bool onlyValid = true);

    void updateViewerMat(const osg::Matrix &mat);
    void setViewerMat(osg::Matrix &mat)
    {
        viewMat = mat;
    };
    osg::Matrix &getViewerMat()
    {
        return (viewMat);
    };
    osg::Vec3 &getViewerPos()
    {
        return viewPos;
    };
    osg::Vec3 &getInitialViewerPos()
    {
        return initialViewPos;
    };
    void setInitialViewerPos(osg::Vec3 &po)
    {
        initialViewPos = po;
    };
    static void freezeCallback(void *viewer, buttonSpecCell *spec);
    static void stereoSepCallback(void *viewer, buttonSpecCell *spec);
    static void orthographicCallback(void *, buttonSpecCell *spec);
    static void statisticsCallback(void *sceneGraph, buttonSpecCell *spec);

    void culling(bool enable, osg::CullSettings::CullingModeValues mode = osg::CullSettings::ENABLE_ALL_CULLING, bool once = false);

    // request angle for movable screen
    angleStruct *getAngleStruct()
    {
        return screen_angle;
    };
    float getSeparation()
    {
        return separation;
    };

    osg::Vec4 getBackgroundColor()
    {
        return backgroundColor;
    };

    bool clearWindow; // if set to true, the whole window is cleared once

    void statistics(bool enable);
    void overwriteViewAndProjectionMatrix(bool state)
    {
        overwritePAndV = state;
    };
    bool isMatrixOverwriteOn()
    {
        return overwritePAndV;
    };

    // assert: cull masks of all channels are equal!
    osg::Node::NodeMask getCullMask() /*const*/;
    osg::Node::NodeMask getCullMaskLeft() /*const*/;
    osg::Node::NodeMask getCullMaskRight() /*const*/;
};
}
#endif
