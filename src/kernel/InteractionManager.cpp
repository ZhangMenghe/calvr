#include <kernel/InteractionManager.h>
#include <input/TrackingManager.h>
#include <input/TrackerMouse.h>
#include <menu/MenuManager.h>
#include <kernel/ComController.h>
#include <kernel/Navigation.h>
#include <kernel/CVRViewer.h>
#include <kernel/SceneManager.h>
#include <kernel/ScreenConfig.h>
#include <kernel/ScreenBase.h>
#include <kernel/PluginManager.h>
#include <config/ConfigManager.h>

#include <osg/Matrix>

#include <vector>
#include <iostream>

using namespace cvr;

osg::Matrix cvr::tie2mat(TrackingInteractionEvent * tie)
{
    osg::Matrix m, t;
    t.makeTranslate(tie->xyz[0], tie->xyz[1], tie->xyz[2]);
    m.makeRotate(osg::Quat(tie->rot[0], tie->rot[1], tie->rot[2], tie->rot[3]));
    return m * t;
}

InteractionManager * InteractionManager::_myPtr = NULL;

InteractionManager::InteractionManager()
{
    _lastMouseButtonMask = 0;
    _mouseButtonMask = 0;
    //_mouseInfo = NULL;
    _mouseActive = false;

    //_mouseEvents = ConfigManager::getBool("Input.MouseEvents",true);
    //std::string tsystem = ConfigManager::getEntry("value","Input.TrackingSystem","NONE");
    //std::string bsystem = ConfigManager::getEntry("value","Input.ButtonSystem","NONE");

    _mouseX = 0;
    _mouseY = 0;

    /*if(tsystem == "MOUSE" || bsystem == "MOUSE")
    {
	_mouseTracker = true;
    }
    else
    {
	_mouseTracker = false;
    }*/
}

InteractionManager::~InteractionManager()
{
}

InteractionManager * InteractionManager::instance()
{
    if(!_myPtr)
    {
        _myPtr = new InteractionManager();
    }
    return _myPtr;
}

bool InteractionManager::init()
{
    return true;
}

void InteractionManager::update()
{
    if(ComController::instance()->getIsSyncError())
    {
	return;
    }
    //processMouse();

    handleEvents();
}

void InteractionManager::handleEvents()
{
    _queueLock.lock();

    while(_eventQueue.size())
    {
        handleEvent(_eventQueue.front());
        delete _eventQueue.front();
        _eventQueue.pop();
    }

    _queueLock.unlock();

    TrackingManager::instance()->cleanupCurrentEvents();

    //std::cerr << "Cleaning " << _mouseQueue.size() << " mouse events." << std::endl;
    while(_mouseQueue.size())
    {
	delete _mouseQueue.front();
	_mouseQueue.pop();
    }
}

void InteractionManager::handleEvent(InteractionEvent * event)
{
    if(MenuManager::instance()->processEvent(event))
    {
        return;
    }

    if(SceneManager::instance()->processEvent(event))
    {
	return;
    }

    if(PluginManager::instance()->processEvent(event))
    {
        return;
    }

    if(CVRViewer::instance()->processEvent(event))
    {
        return;
    }

    Navigation::instance()->processEvent(event);
}

void InteractionManager::addEvent(InteractionEvent * event)
{
    _queueLock.lock();

    _eventQueue.push(event);

    _queueLock.unlock();
}

void InteractionManager::setMouse(int x, int y)
{
    //std::cerr << "Mouse x: " << x << " y: " << y << std::endl;
    ScreenInfo * si = ScreenConfig::instance()->getMasterScreenInfo(CVRViewer::instance()->getActiveMasterScreen());

    if(!si)
    {
	return;
    }

    _mouseActive = true;
    _mouseX = x;
    _mouseY = y;

    float screenX, screenY;
    float frac;

    frac = ((float)_mouseX) / si->myChannel->width;
    frac = frac - 0.5;
    screenX = frac * si->width;

    //frac = ((float)(si->myChannel->height - _mouseY))
    //       / (si->myChannel->height);
    frac = ((float)_mouseY) / si->myChannel->height;
    frac = frac - 0.5;
    screenY = frac * si->height;

    osg::Vec3 screenPoint(screenX,0,screenY);
    // get Point in world space
    screenPoint = screenPoint * si->transform;
    // head mounted displays are relative to the head orientation
    if(si->myChannel->stereoMode == "HMD")
    {
	screenPoint = screenPoint * TrackingManager::instance()->getHeadMat(si->myChannel->head);
    }

    osg::Vec3 eyePoint;

    if(si->myChannel->stereoMode == "LEFT")
    {
	eyePoint = osg::Vec3(-ScreenBase::getEyeSeparation() * ScreenBase::getEyeSeparationMultiplier() / 2.0, 0,0);
    }
    else if(si->myChannel->stereoMode == "RIGHT")
    {
	eyePoint = osg::Vec3(-ScreenBase::getEyeSeparation() * ScreenBase::getEyeSeparationMultiplier() / 2.0, 0,0);
    }

    eyePoint = eyePoint * TrackingManager::instance()->getHeadMat(si->myChannel->head);

    osg::Vec3 v = screenPoint - eyePoint;
    v.normalize();

    _mouseMat.makeRotate(osg::Vec3(0, 1, 0), v);
    _mouseMat = _mouseMat * osg::Matrix::translate(eyePoint);
}

void InteractionManager::setMouseButtonMask(unsigned int mask)
{
    _mouseButtonMask = mask;
}

unsigned int InteractionManager::getMouseButtonMask()
{
    return _mouseButtonMask;
}

bool InteractionManager::mouseActive()
{
    return _mouseActive;
}

osg::Matrix & InteractionManager::getMouseMat()
{
    return _mouseMat;
}

int InteractionManager::getMouseX()
{
    return _mouseX;
}

int InteractionManager::getMouseY()
{
    return _mouseY;
}

void InteractionManager::processMouse()
{
    if(!_mouseActive)
    {
        return;
    }

    // TODO: get max mouse buttons from somewhere
    unsigned int bit = 1;
    for(int i = 0; i < 10; i++)
    {
        if((_lastMouseButtonMask & bit) != (_mouseButtonMask & bit))
        {
	    /*if(_mouseTracker)
	    {
		TrackingInteractionEvent * tie = new TrackingInteractionEvent;
		if(_lastMouseButtonMask & bit)
		{
		    tie->type = BUTTON_UP;
		}
		else
		{
		    tie->type = BUTTON_DOWN;
		}
		tie->button = i;
		tie->hand = 0;
		osg::Vec3 pos = _mouseMat.getTrans();
		osg::Quat rot = _mouseMat.getRotate();
		tie->xyz[0] = pos.x();
		tie->xyz[1] = pos.y();
		tie->xyz[2] = pos.z();
		tie->rot[0] = rot.x();
		tie->rot[1] = rot.y();
		tie->rot[2] = rot.z();
		tie->rot[3] = rot.w();
		addEvent(tie);
	    }*/
	    //else if(_mouseEvents)
	    {
		MouseInteractionEvent * buttonEvent = new MouseInteractionEvent;
		if(_lastMouseButtonMask & bit)
		{
		    buttonEvent->type = MOUSE_BUTTON_UP;
		}
		else
		{
		    buttonEvent->type = MOUSE_BUTTON_DOWN;
		}
		buttonEvent->button = i;
		buttonEvent->x = _mouseX;
		buttonEvent->y = _mouseY;
		buttonEvent->transform = _mouseMat;
		_mouseQueue.push(buttonEvent);
	    }
        }
        bit = bit << 1;
    }
    _lastMouseButtonMask = _mouseButtonMask;
}

void InteractionManager::createMouseDragEvents()
{
    if(!_mouseActive)
    {
        return;
    }

    unsigned int bit = 1;
    for(int i = 0; i < 10; i++)
    {
        if((_mouseButtonMask & bit))
        {
	    /*if(_mouseTracker)
	    {
		TrackingInteractionEvent * tie = new TrackingInteractionEvent;
		tie->type = BUTTON_DRAG;
		tie->button = i;
		tie->hand = 0;
		osg::Vec3 pos = _mouseMat.getTrans();
		osg::Quat rot = _mouseMat.getRotate();
		tie->xyz[0] = pos.x();
		tie->xyz[1] = pos.y();
		tie->xyz[2] = pos.z();
		tie->rot[0] = rot.x();
		tie->rot[1] = rot.y();
		tie->rot[2] = rot.z();
		tie->rot[3] = rot.w();
		addEvent(tie);
	    }*/
	    //else if(_mouseEvents)
	    {
		MouseInteractionEvent * dEvent = new MouseInteractionEvent;
		dEvent->type = MOUSE_DRAG;
		dEvent->button = i;
		dEvent->x = _mouseX;
		dEvent->y = _mouseY;
		dEvent->transform = _mouseMat;
		_mouseQueue.push(dEvent);
	    }
        }
        bit = bit << 1;
    }
}

void InteractionManager::createMouseDoubleClickEvent(int button)
{
    if(!_mouseActive)
    {
        return;
    }

    unsigned int bit = 1;
    for(int i = 0; i < 10; i++)
    {
        if((button & bit))
        {
	    /*if(_mouseTracker)
	    {
		TrackingInteractionEvent * tie = new TrackingInteractionEvent;
		tie->type = BUTTON_DOUBLE_CLICK;
		tie->button = i;
		tie->hand = 0;
		osg::Vec3 pos = _mouseMat.getTrans();
		osg::Quat rot = _mouseMat.getRotate();
		tie->xyz[0] = pos.x();
		tie->xyz[1] = pos.y();
		tie->xyz[2] = pos.z();
		tie->rot[0] = rot.x();
		tie->rot[1] = rot.y();
		tie->rot[2] = rot.z();
		tie->rot[3] = rot.w();
		addEvent(tie);
	    }*/
	    //else if(_mouseEvents)
	    {
		MouseInteractionEvent * dcEvent = new MouseInteractionEvent;
		dcEvent->type = MOUSE_DOUBLE_CLICK;
		dcEvent->button = i;
		dcEvent->x = _mouseX;
		dcEvent->y = _mouseY;
		dcEvent->transform = _mouseMat;
		_mouseQueue.push(dcEvent);
	    }
            _mouseButtonMask |= button;
            _lastMouseButtonMask |= button;
            return;
        }
        bit = bit << 1;
    }
}
