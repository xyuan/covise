/* This file is part of COVISE.

   You can use it under the terms of the GNU Lesser General Public License
   version 2.1 or later, see lgpl-2.1.txt.

 * License: LGPL 2+ */

#ifndef OPEN_COVER_PLUGIN_SUPPORT
#define OPEN_COVER_PLUGIN_SUPPORT

/*! \file
 \brief  provide a stable interface to the most important OpenCOVER classes and calls
         through a single pointer

 \author
 \author (C)
         Computer Centre University of Stuttgart,
         Allmandring 30,
         D-70550 Stuttgart,
         Germany

 \date
 */

/// @cond INTERNAL

#ifdef _WIN32
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x501 // This specifies WinXP or later - it is needed to access rawmouse from the user32.dll
#endif
#if (_MSC_VER >= 1300) && !(defined(MIDL_PASS) || defined(RC_INVOKED))
#define POINTER_64 __ptr64
#else
#define POINTER_64
#endif
#include <winsock2.h>
#include <io.h>
#ifndef PATH_MAX
#define PATH_MAX 512
#endif
#endif

#include <limits.h>
#include <osg/Matrix>
#include <osg/Geode>
#include <osg/ClipNode>
#include <osgViewer/GraphicsWindow>
#include <osg/BoundingBox>

#include <deque>
#include "VRPinboard.h"
#include "coVRPlugin.h"

#define MAX_NUMBER_JOYSTICKS 64

namespace osg
{
class MatrixTransform;
}

namespace osgText
{
class Font;
}

namespace vrui
{
class coUpdateManager;
class coMenu;
class coToolboxMenu;
}
namespace vrml
{
class Player;
}

namespace covise
{
class TokenBuffer;
}
/// @endcond INTERNAL
namespace opencover
{
class coVRPlugin;
class RenderObject;
class coInteractor;
struct Isect
{
    enum IntersectionBits
    {
        Collision = 1,
        Intersection = 2,
        Walk = 4,
        Touch = 8,
        Pick = 16,
        Visible = 32,
        NoMirror = 64,
        Left = 128,
        Right = 256,
        OsgEarthSecondary = 0x80000000,
    };

private:
};

/*! \class coPointerButton coVRPluginSupport.h cover/coVRPluginSupport.h
 * Access to buttons and wheel of interaction devices
 */
class COVEREXPORT coPointerButton
{
    friend class coVRPluginSupport;
    friend class coVRMSController;

public:
    coPointerButton(const std::string &name);
    ~coPointerButton();
    //! button state
    //! @return button press mask
    unsigned int getState();
    //! previous button state
    //! @return old button state
    unsigned int oldState();
    //! buttons pressed since last frame
    unsigned int wasPressed();
    //! buttons released since last frame
    unsigned int wasReleased();
    //! is no button pressed
    bool notPressed();
    //! accumulated number of wheel events
    int getWheel();
    //! set number wheel events
    void setWheel(int);
    //! button name
    const std::string &name() const;

private:
    //! set button state
    void setState(unsigned int);

    unsigned int buttonStatus;
    unsigned int lastStatus;
    int wheelCount;
    std::string m_name;
};

/*! \class coVRPluginSupport coVRPluginSupport.h cover/coVRPluginSupport.h
 * Provide a stable interface and a single entry point to the most import
 * OpenCOVER functions
 */
class COVEREXPORT coVRPluginSupport
{
    friend class OpenCOVER;
    friend class coVRMSController;
    friend class coIntersection;
    friend class VRPinboard;

public:
    //! returns true if level <= debugLevel
    /*! debug levels should be used like this:
          0 no output,
          1 covise.config entries, coVRInit,
          2 constructors, destructors,
          3 all functions which are not called continously,
          4,
          5 all functions which are called continously */
    bool debugLevel(int level) const;

    // OpenGL clipping
    /// @cond INTERNAL
    enum
    {
        MAX_NUM_CLIP_PLANES = 6
    };
    /// @endcond INTERNAL

    //! return the number of clipPlanes reserved for the kernel, others are available to VRML ClippingPlane Node
    int getNumClipPlanes();

    //! return pointer to a clipping plane
    osg::ClipPlane *getClipPlane(int num)
    {
        return clipPlanes[num].get();
    }

    //! returns true if clipping is on
    bool isClippingOn() const;

    //! return number of clipping plane user is possibly interacting with
    int getActiveClippingPlane() const;

    //! set number of clipping plane user is possibly interacting with
    void setActiveClippingPlane(int plane);

    // access to scene graph nodes and transformations

    //! get scene group node
    osg::Group *getScene() const;

    //! get the group node for all COVISE and model geometry
    osg::ClipNode *getObjectsRoot() const;

    //! get the MatrixTransform node of the hand
    // (in VRSceneGraph handTransform)
    osg::MatrixTransform *getPointer() const;

    //! get matrix of hand transform (same as getPointer()->getMatrix())
    const osg::Matrix &getPointerMat() const;
    //       void setPointerMat(osg::Matrix);

    //! get matrix of current 2D mouse matrix
    //! (the same as getPointerMat for MOUSE tracking)
    const osg::Matrix &getMouseMat() const;

    //! get the MatrixTransform for objects translation and rotation
    osg::MatrixTransform *getObjectsXform() const;

    //! same as getObjectsXform()->getMatrix()
    const osg::Matrix &getXformMat() const;

    //! same as getObjectsXform()->setMatrix()
    void setXformMat(const osg::Matrix &mat);

    //! get the MatrixTransform for objects scaling
    osg::MatrixTransform *getObjectsScale() const;

    //! set the scale matrix of the scale node
    void setScale(float s);

    //! get the scale factor of the scale node
    float getScale() const;

    //! transformation matrix from object coordinates to world coordinates
    /*! multiplied matrices from scene node to objects root node */
    const osg::Matrix &getBaseMat() const
    {
        return baseMatrix;
    }

    //! transformation from world coordinates to object coordinates
    /*! use this cached value instead of inverting getBaseMat() yourself */
    const osg::Matrix &getInvBaseMat() const;

    vrui::coUpdateManager *getUpdateManager() const;

    //! get the scene size defined in covise.config
    float getSceneSize() const;

    bool isVRBconnected();

    //! send a message either via COVISE connection or via VRB
    bool sendVrbMessage(const covise::Message *msg) const;

    // tracker data

    //! get the position and orientation of the user in world coordinates
    const osg::Matrix &getViewerMat() const;

    //! search geodes under node and set Visible bit in node mask
    void setNodesIsectable(osg::Node *n, bool isect);

    //! returns a pointer to a coPointerButton object for the main button device
    coPointerButton *getPointerButton() const;

    //! returns a pointer to a coPointerButton object representing the mouse buttons state
    coPointerButton *getMouseButton() const;

    //! returns the COVER Menu (Pinboard)
    vrui::coMenu *getMenu() const;

    //! return group node of menus
    osg::Group *getMenuGroup() const;

    // interfacing with plugins

    //! load a new plugin
    coVRPlugin *addPlugin(const char *name);

    //! get plugin called name
    coVRPlugin *getPlugin(const char *name);

    //! remove the plugin by pointer
    void removePlugin(coVRPlugin *);

    //! remove a plugin by name
    int removePlugin(const char *name);

    //! informs other plugins that this plugin extended the scene graph
    void addedNode(osg::Node *node, coVRPlugin *myPlugin);

    //! remove node from the scene graph,
    /*! use this method when removing nodes from the scene graph in order to update
       * OpenCOVER's internal state */
    //! @return if a node was removed
    bool removeNode(osg::Node *node, bool isGroup = false);

    //! send a message to other plugins
    void sendMessage(coVRPlugin *sender, int toWhom, int type, int len, const void *buf);

    //! send a message to a named plugins
    void sendMessage(coVRPlugin *sender, const char *destination, int type, int len, const void *buf, bool localonly = false);
    //! grab keyboard input
    /*! other plugins will not get key event notifications,
          returns true if keyboard could be grabbed,
          returns false if keyboard is already grabbed by another plugin */
    bool grabKeyboard(coVRPlugin *);

    //! release keyboard input, all plugins will get key events
    void releaseKeyboard(coVRPlugin *);

    //! check if keyboard is grabbed
    bool isKeyboardGrabbed();

    //! returns a new unique button group id
    /*! if you want to create a new button group you need this id */
    int createUniqueButtonGroupId();

    // add Buttons to Pinboard

    //! append a button to a menu which opens a submenu if switched on
    //! @param buttonName text on the button
    //! @param  parentMenuName to which menu it will be appended, NULL = main menu
    //! @param  subMenuName header of the submenu
    //! @param  state false=off, true=on
    //! @param  callback function which is called on press/release
    //! @param  groupId if a button is in the same group with others it is automatically
    //!         switched off if another button is pressed
    //! @param  classPtr ptr to the class which calls the callback
    void addSubmenuButton(const char *buttonName, const char *parentMenuName,
                          const char *subMenuName, int state, ButtonCallback callback, int groupId,
                          void *classPtr);

    //! append a toggle button to a menu
    //! @param buttonName text on the button
    //! @param parentMenuName to which menu it will be appended, NULL = main menu
    //! @param state false=off, true=on
    //! @param callback function which is called on press/release
    //! @param classPtr ptr to the class which calls the callback
    //! @param userData ptr to data to pass to callback
    void addToggleButton(const char *buttonName, const char *parentMenuName,
                         int state, ButtonCallback callback, void *classPtr, void *userData = NULL);

    //! append a group button to a menu
    //! @param buttonName text on the button
    //! @param parentMenuName to which menu it will be appended, NULL = main menu
    //! @param state false=off, true=on
    //! @param callback function which is called on press/release
    //! @param groupId if a button is in the same group with others it is automatically
    //          switched off if another button is pressed
    //! @param classPtr ptr to the class which calls the callback
    //! @param userData ptr to data to pass to callback
    void addGroupButton(const char *buttonName, const char *parentMenuName,
                        int state, ButtonCallback callback, int groupId, void *classPtr, void *userData = NULL);

    //! add a function button, a function button just calls the callback
    //! @param buttonName text on the button
    //! @param parentMenuName to which menu it will be appended, NULL = main menu
    //! @param callback function which is called on press/release
    //! @param classPtr ptr to the class which calls the callback
    //! @param userData ptr to data to pass to callback
    void addFunctionButton(const char *buttonName, const char *parentMenuName,
                           ButtonCallback callback, void *classPtr, void *userData = NULL);

    //! add a slider to a menu
    //! @param buttonName text on the slider
    //! @param parentMenuName to which menu it will be appended, NULL = main menu
    //! @param min minimum slider value
    //! @param max maximum slider value
    //! @param value initial slider value
    //! @param callback function which is called on press/release
    //! @param classPtr ptr to the class which calls the callback
    //! @param userData ptr to data to pass to callback
    void addSliderButton(const char *buttonName, const char *parentMenuName,
                         float min, float max, float value, ButtonCallback callback, void *classPtr, void *userData = NULL);

    //! set slider value
    //! @param buttonName text on the slider
    //! @param value new slider value
    bool setSliderValue(const char *buttonName, float value);

    //! set the state of a button, this doesn't call the callback,
    //! set the state of a group, toggle or submenu button
    //! @param buttonName text on the button
    //! @param state new button state
    bool setButtonState(const char *buttonName, int state);

    //! get the current state of a button
    //! @param buttonName text on the button
    //! @param menuName name of the button's menu
    //! @return current button state
    int getButtonState(const char *buttonName, const char *menuName);

    //! get the current value of a slider
    //! @param buttonName text on the slider
    //! @param menuName name of the slider's menu
    //! @return current slider value
    float getSliderValue(const char *buttonName, const char *menuName);

    //! remove a button
    //! @buttonName name of the button to remove
    //! @parentMenuName name of submenu, if NULL remove from main menu
    void removeButton(const char *buttonName, const char *parentMenuName);

    // COVER functions

    //! check if a function is visible in pinboard
    //! @return true if this function is built-in and in pinboard
    int isBuiltInFunctionInPinboard(const char *functionName);

    // set the state of a the menu button/slider and the function
    // same as setButtonState but with the functionName as parameter
    // for built in functions the button names are configurable thus we
    // don't know the names any more
    // functionName = {"XFORM", "SCALE", ...}
    // this sets only the appearance of the button, it doesn't call teh callback
    // for group and toggle buttons
    void setBuiltInFunctionState(const char *functionName, int state);
    // for slider buttons
    void setBuiltInFunctionValue(const char *functionName, float val);

    int getBuiltInFunctionState(const char *functionName, int *state);
    int getBuiltInFunctionValue(const char *functionName, float *val);

    // this sets the state/value and calls the callback
    void callBuiltInFunctionCallback(const char *functionName);

    vrui::coMenuItem *getBuiltInFunctionMenuItem(const char *functionName);

    // enable navigation and disable navigation
    // now obsolete, use cover.setBuiltInFunctionState(functionName,state)
    void enableNavigation(const char *navigationName);

    // deactivate 3D menu functions
    void disableNavigation(const char *navigationName);

    // "DriveSpeed"         set drive speed
    void setNavigationValue(const char *navigationName, float value);

    // make a configurable button

    // looks in covise.config scope PinboardConfig
    // if available custom buttonlabels and menulabelare used,
    // else the default labels
    //
    // example:
    //
    // PinboardConfig
    // {
    //    XFORM "move world"
    //    WALK  "move wolrd" "navigation"
    //    MYFUNCTION "mylabel" "mymenu"
    // }
    //
    void addConfigurableButton(const char *functionName, const char *defButtonName, const char *defMenuName, int type, void *callback, void *inst, int groupId = -1, void *userData = NULL);

    void protectScenegraph();

    //! returns the time in seconds since Jan. 1, 1970 at the beginning of this frame,
    //! use this function if you can since it is faster than currentTime(),
    //! this is the time for which the rendering should be correct,
    //! might differ from system time
    //! @return number of seconds since Jan. 1, 1970 at the beginning of this frame
    double frameTime() const;

    //! returns the duration of the last frame in seconds
    //! @return render time of the last frame in seconds
    double frameDuration() const;

    //! returns the time in seconds since Jan. 1, 1970 at the beginning of this frame,
    //! use this function if you can since it is faster than currentTime(),
    double frameRealTime() const;

    //! returns the current time in seconds since Jan. 1, 1970,
    //! if possible, use frameTime() as it does not require a system call
    //! @return number of seconds since Jan. 1, 1970
    double currentTime() const;

    //! get the number of the active cursor shape
    osgViewer::GraphicsWindow::MouseCursor getCurrentCursor() const;

    //! set cursor shape
    //! @param type number of cursor shape
    void setCurrentCursor(osgViewer::GraphicsWindow::MouseCursor type);

    //! make the cursor visible or invisible
    void setCursorVisible(bool visible);

    //! get node currently intersected by pointer
    osg::Node *getIntersectedNode() const;

    //! get path to node currently intersected by pointer
    const osg::NodePath &getIntersectedNodePath() const;

    //! get world coordinates of intersection hit point
    const osg::Vec3 &getIntersectionHitPointWorld() const;

    //! get normal of intersection hit
    const osg::Vec3 &getIntersectionHitPointWorldNormal() const;

    /*********************************************************************/
    // do not use anything beyond this line

    /// @cond INTERNAL
    // deprecated, use coInteraction with high priority instead
    // returns true if another plugin locked the pointer
    int isPointerLocked();

    // deprecated, use coInteraction instead
    // returns true if USER is currently navigating
    bool isNavigating();

    // old COVISE Messages
    int sendBinMessage(const char *keyword, const char *data, int len);
    // update frametime
    void updateTime();
    // update matrices
    void update();

    //! update internal state related to current person being tracked - called Input system
    void personSwitched(size_t personNumber);

    osg::Matrix envCorrectMat;
    osg::Matrix invEnvCorrectMat;

    int registerPlayer(vrml::Player *player);
    int unregisterPlayer(vrml::Player *player);
    vrml::Player *usePlayer(void (*playerUnavailableCB)());
    int unusePlayer(void (*playerUnavailableCB)());

    int numJoysticks;
    unsigned char number_buttons[MAX_NUMBER_JOYSTICKS];
    int *buttons[MAX_NUMBER_JOYSTICKS];
    unsigned char number_axes[MAX_NUMBER_JOYSTICKS];
    float *axes[MAX_NUMBER_JOYSTICKS];
    unsigned char number_sliders[MAX_NUMBER_JOYSTICKS];
    float *sliders[MAX_NUMBER_JOYSTICKS];
    unsigned char number_POVs[MAX_NUMBER_JOYSTICKS];
    float *POVs[MAX_NUMBER_JOYSTICKS];

    osg::ref_ptr<osg::ColorMask> getNoFrameBuffer()
    {
        return NoFrameBuffer;
    }

    // utility
    float getSqrDistance(osg::Node *n, osg::Vec3 &p, osg::MatrixTransform **path, int pathLength) const;

    osg::Matrix *getWorldCoords(osg::Node *node) const;
    osg::Vec3 frontScreenCenter; ///< center of front screen
    float frontHorizontalSize; ///< width of front screen
    float frontVerticalSize; ///< height of front screen
    int frontWindowHorizontalSize; ///<  width of front window
    int frontWindowVerticalSize; ///<  width of front window

    //! returns scale factor for interactor-screen distance
    float getInteractorScale(osg::Vec3 &pos); // pos in World coordinates

    //! returns viewer-screen distance
    float getViewerScreenDistance();

    //! compute the box of all visible nodes above and included node
    osg::BoundingBox getBBox(osg::Node *node);

    //restrict interactors to visible scne
    bool restrictOn()
    {
        return resOn;
    };

    /// @endcond INTERNAL

    /// @cond INTERNAL
    enum MessageDestinations
    {
        TO_ALL,
        TO_ALL_OTHERS,
        TO_SAME,
        TO_SAME_OTHERS,
        VRML_EVENT, // Internal, do not use!!
        NUM_TYPES
    };
    /// @endcond INTERNAL

    vrui::coToolboxMenu *getToolBar()
    {
        return toolBar;
    };
    void setToolBar(vrui::coToolboxMenu *tb)
    {
        toolBar = tb;
    };

    //! use only during coVRPlugin::prepareFrame()
    void setFrameTime(double ft);

private:
    bool resOn; // flag for restrict interactors to visible scene

    void setFrameRealTime(double ft);

    //! calls the callback
    void callButtonCallback(const char *buttonName);

    float scaleFactor; ///< scale depending on viewer-screen FOV
    float viewerDist; ///< distance of viewer from screen
    osg::Vec3 eyeToScreen; ///< eye to screen center vector

    osg::ref_ptr<osg::ColorMask> NoFrameBuffer;

    osg::ref_ptr<osg::ClipPlane> clipPlanes[MAX_NUM_CLIP_PLANES];

    mutable vrui::coUpdateManager *updateManager;

    mutable int invCalculated;
    osg::Matrix baseMatrix;
    mutable osg::Matrix invBaseMatrix;
    int buttonGroup;
    double lastFrameStartTime;
    double frameStartTime, frameStartRealTime;
    osgViewer::GraphicsWindow::MouseCursor currentCursor;
    vrml::Player *player;
    list<void (*)()> playerUseList;

    int activeClippingPlane;

    osg::ref_ptr<osg::Geode> intersectedNode;
    //osg::ref_ptr<osg::NodePath> intersectedNodePath;
    osg::NodePath intersectedNodePath;
    osg::Vec3 intersectionHitPointWorld;
    osg::Vec3 intersectionHitPointWorldNormal;
    osg::Vec3 intersectionHitPointLocal;
    osg::Vec3 intersectionHitPointLocalNormal;
    osg::ref_ptr<osg::RefMatrix> intersectionMatrix;

    mutable coPointerButton *pointerButton;
    mutable coPointerButton *mouseButton;
    vrui::coToolboxMenu *toolBar;

    int numClipPlanes;

    coVRPluginSupport();
    ~coVRPluginSupport();
};

COVEREXPORT covise::TokenBuffer &operator<<(covise::TokenBuffer &buffer, const osg::Matrixd &matrix);
COVEREXPORT covise::TokenBuffer &operator>>(covise::TokenBuffer &buffer, osg::Matrixd &matrix);

COVEREXPORT covise::TokenBuffer &operator<<(covise::TokenBuffer &buffer, const osg::Vec3f &vec);
COVEREXPORT covise::TokenBuffer &operator>>(covise::TokenBuffer &buffer, osg::Vec3f &vec);

//============================================================================
// Useful inline Function Templates
//============================================================================

/// @return the result of value a clamped between left and right
template <class C>
inline C coClamp(const C a, const C left, const C right)
{
    if (a < left)
        return left;
    if (a > right)
        return right;
    return a;
}

extern COVEREXPORT coVRPluginSupport *cover;
}
#endif
