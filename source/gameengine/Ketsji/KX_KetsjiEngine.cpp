/*
 * ***** BEGIN GPL LICENSE BLOCK *****
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * The Original Code is Copyright (C) 2001-2002 by NaN Holding BV.
 * All rights reserved.
 *
 * The Original Code is: all of this file.
 *
 * Contributor(s): none yet.
 *
 * ***** END GPL LICENSE BLOCK *****
 * The engine ties all game modules together.
 */

/** \file gameengine/Ketsji/KX_KetsjiEngine.cpp
 *  \ingroup ketsji
 */

#ifdef _MSC_VER
#  pragma warning (disable:4786)
#endif

#include <thread>

extern "C" {
	#include "BKE_appdir.h"
}

#include "CM_Message.h"

#include <boost/format.hpp>

#include "BLI_task.h"

#include "KX_KetsjiEngine.h"

#include "EXP_ListValue.h"
#include "EXP_IntValue.h"
#include "EXP_BoolValue.h"
#include "EXP_FloatValue.h"
#include "RAS_BucketManager.h"
#include "RAS_Rasterizer.h"
#include "RAS_ICanvas.h"
#include "RAS_OffScreen.h"
#include "RAS_Query.h"
#include "RAS_ILightObject.h"
#include "SCA_IInputDevice.h"
#include "KX_Camera.h"
#include "KX_LightObject.h"
#include "KX_Globals.h"
#include "KX_PyConstraintBinding.h"
#include "PHY_IPhysicsEnvironment.h"

#include "KX_NetworkMessageScene.h"

#include "DEV_Joystick.h" // for DEV_Joystick::HandleEvents
#include "KX_PythonInit.h" // for updatePythonJoysticks

#include "KX_WorldInfo.h"

#include "BL_Converter.h"
#include "BL_SceneConverter.h"

#include "RAS_FramingManager.h"
#include "DNA_world_types.h"
#include "DNA_scene_types.h"

#include "KX_NavMeshObject.h"

#define DEFAULT_LOGIC_TIC_RATE 60.0

KX_ExitInfo::KX_ExitInfo()
	:m_code(NO_REQUEST)
{
}

KX_KetsjiEngine::CameraRenderData::CameraRenderData(KX_Camera *rendercam, KX_Camera *cullingcam, const RAS_Rect& area,
                                                    const RAS_Rect& viewport, RAS_Rasterizer::StereoMode stereoMode, RAS_Rasterizer::StereoEye eye)
	:m_renderCamera(rendercam),
	m_cullingCamera(cullingcam),
	m_area(area),
	m_viewport(viewport),
	m_stereoMode(stereoMode),
	m_eye(eye)
{
	m_renderCamera->AddRef();
}

KX_KetsjiEngine::CameraRenderData::CameraRenderData(const CameraRenderData& other)
	:m_renderCamera(CM_AddRef(other.m_renderCamera)),
	m_cullingCamera(other.m_cullingCamera),
	m_area(other.m_area),
	m_viewport(other.m_viewport),
	m_stereoMode(other.m_stereoMode),
	m_eye(other.m_eye)
{
}

KX_KetsjiEngine::CameraRenderData::~CameraRenderData()
{
	m_renderCamera->Release();
}

KX_KetsjiEngine::SceneRenderData::SceneRenderData(KX_Scene *scene)
	:m_scene(scene)
{
}

KX_KetsjiEngine::FrameRenderData::FrameRenderData(RAS_OffScreen::Type ofsType)
	:m_ofsType(ofsType)
{
}

KX_KetsjiEngine::RenderData::RenderData(RAS_Rasterizer::StereoMode stereoMode, bool renderPerEye)
	:m_stereoMode(stereoMode),
	m_renderPerEye(renderPerEye)
{
}


const std::string KX_KetsjiEngine::m_profileLabels[tc_numCategories] = {
	"Physics", // tc_physics
	"Logic", // tc_logic
	"Render", // tc_rasterizer
	"Shadows", // tc_overhead
	"Animations", // tc_animations
	"This Profile", // tc_network
	"Video Card", // tc_latency
	"Parent Update", // tc_scenegraph
	"Culling", // tc_services
	"Time Free" // tc_outside
};
/*
const std::string KX_KetsjiEngine::m_renderQueriesLabels[QUERY_MAX] = {
	"Samples:", // QUERY_SAMPLES
	"Primitives:", // QUERY_PRIMITIVES
	"Time:" // QUERY_TIME
};
*/
/**
 * Constructor of the Ketsji Engine
 */
KX_KetsjiEngine::KX_KetsjiEngine()
	:m_canvas(nullptr),
	m_rasterizer(nullptr),
	m_converter(nullptr),
	m_networkMessageManager(nullptr),
//#ifdef WITH_PYTHON
	m_pyprofiledict(PyDict_New()),
//#endif
	m_inputDevice(nullptr),
	m_pythonMouse(nullptr),
	m_scenes(new EXP_ListValue<KX_Scene>()),
	m_bInitialized(false),
	m_isfirstscene(false),
	m_flags(AUTO_ADD_DEBUG_PROPERTIES),
	m_frameTime(0.0f),
	m_logicTime(0.0f),
	m_physicsTime(0.0f),
	m_animationsTime(0.0f),
	//m_frames(0),
	//m_width(640),
	//m_height(480),
	//m_left(1),
	//m_bottom(1),
	m_timestep(0.0f),
	m_sleeptime(0),
	m_overframetime(0.0f),
	m_lastframetime(0.0f),
	m_logictimestart(0.0f),
	m_logictime(0.0f),
	m_lastlogictime(0.0f),
	m_tottime(0.0f),
	m_rendertime(0.0f),
	m_lastrendertime(0.0f),
	m_rendertimeaverage(0.0f),
	m_rendertimestart(0.0f),
	m_overrendertime(0.0f),
	m_animationtime(0.0f),
	m_lastanimationtime(0.0f),
	m_animationtimeaverage(0.0f),
	m_animationtimestart(0.0f),
	m_overanimationtime(0.0f),
	m_time(0.0f),
	m_i(0),
	m_addrem(0),
	m_framestep(0.0f),
	m_clockTime(0.0f),
	m_timeScale(1.0f),
	m_logicScale(1.0f),
	m_physicsScale(1.0f),
	m_animationsScale(1.0f),
	m_previousRealTime(0.0f),
	m_maxLogicFrame(1.0f),
	m_maxPhysicsFrame(true),
	m_ticrate(DEFAULT_LOGIC_TIC_RATE),
	m_renderrate(0.0f),
	m_animationrate(0.0f),
	m_deltaTime(0.0f),
	m_previous_deltaTime(0.0f),
	m_anim_framerate(25.0),
	m_needsRender(true),
	m_needsAnimation(true),
	m_doRender(true),
	m_exitKey(SCA_IInputDevice::ENDKEY),
	m_average_framerate(0.0),
	m_showBoundingBox(KX_DebugOption::DISABLE),
	m_showArmature(KX_DebugOption::DISABLE),
	m_showCameraFrustum(KX_DebugOption::DISABLE),
	m_showShadowFrustum(KX_DebugOption::DISABLE),
	m_globalsettings({0}),
	m_taskscheduler(BLI_task_scheduler_create(TASK_SCHEDULER_AUTO_THREADS)),
	m_logger(KX_TimeCategoryLogger(m_clock, 200))
{
	for (int i = tc_first; i < tc_numCategories; i++) {
		m_logger.AddCategory((KX_TimeCategory)i);
	}

	//m_renderQueries.emplace_back(RAS_Query::SAMPLES);
	//m_renderQueries.emplace_back(RAS_Query::PRIMITIVES);
	//m_renderQueries.emplace_back(RAS_Query::TIME);
}

/**
 *	Destructor of the Ketsji Engine, release all memory
 */
KX_KetsjiEngine::~KX_KetsjiEngine()
{
//#ifdef WITH_PYTHON
	Py_CLEAR(m_pyprofiledict);
//#endif

	if (m_taskscheduler) {
		BLI_task_scheduler_free(m_taskscheduler);
	}

	m_scenes->Release();
}

void KX_KetsjiEngine::SetInputDevice(SCA_IInputDevice *inputDevice)
{
	BLI_assert(inputDevice);
	m_inputDevice = inputDevice;
}

void KX_KetsjiEngine::SetPythonMouse(SCA_PythonMouse *pythonMouse)
{
	BLI_assert(pythonMouse);
	m_pythonMouse = pythonMouse;
}

void KX_KetsjiEngine::SetCanvas(RAS_ICanvas *canvas)
{
	BLI_assert(canvas);
	m_canvas = canvas;
}

void KX_KetsjiEngine::SetRasterizer(RAS_Rasterizer *rasterizer)
{
	BLI_assert(rasterizer);
	m_rasterizer = rasterizer;
}

void KX_KetsjiEngine::SetNetworkMessageManager(KX_NetworkMessageManager *manager)
{
	m_networkMessageManager = manager;
}

//#ifdef WITH_PYTHON
PyObject *KX_KetsjiEngine::GetPyProfileDict()
{
	Py_INCREF(m_pyprofiledict);
	return m_pyprofiledict;
}
//#endif

void KX_KetsjiEngine::SetConverter(BL_Converter *converter)
{
	BLI_assert(converter);
	m_converter = converter;
}

void KX_KetsjiEngine::StartEngine()
{
	// Reset the clock to start at 0.0.
	m_clock.Reset();
	
	m_bInitialized = true;

	// wip fix for now
	m_renderrate = 1.0 / m_ticrate;
	m_animationrate = 1.0 / m_ticrate;
	//m_timestep = 1.0 / m_ticrate;
	//m_framestep = m_timestep * m_timeScale;
}

void KX_KetsjiEngine::BeginFrame()
{
	/*if (m_flags & SHOW_RENDER_QUERIES) {
		m_logger.StartLog(tc_overhead);

		for (RAS_Query& query : m_renderQueries) {
			query.Begin();
		}
	}*/

	m_logger.StartLog(tc_rasterizer);

	BeginFrameRun();

	BeginDraw();
}

// EndFrame from Render thread
void KX_KetsjiEngine::EndFrame()
{
	//m_logger.StartLog(tc_network);
	if (m_needsRender) {
		MotionBlur();
	}

	/*if (m_flags & SHOW_RENDER_QUERIES) {
		for (RAS_Query& query : m_renderQueries) {
			query.End();
		}
	}*/

	// Start logging time spent outside main loop
	m_logger.StartLog(tc_outside);
	// update delta Time
	ClockTiming();
	// If We have time sleep here
	if (m_flags & FIXED_FRAMERATE) {// timings for old games
		while (m_timestep > m_deltaTime - m_overframetime + 6e-6) {
			std::this_thread::sleep_for(std::chrono::milliseconds((long)((2e-3 * m_maxLogicFrame))));
			ClockTiming();
		}
		m_overframetime = 0.0;
	}
	else {// new timing mode
		m_sleeptime = 2;
		if (m_timestep > m_deltaTime - m_overframetime + 6e-6) {
			while (m_timestep > m_deltaTime - m_overframetime + 6e-6) {
				if (m_timestep > (m_deltaTime * 1.5)) {
					m_sleeptime += 2;
					std::this_thread::sleep_for(std::chrono::milliseconds((long)(((m_timestep - m_deltaTime)) * m_maxLogicFrame) / m_sleeptime));
					ClockTiming();
				}
				else if (m_timestep > (m_deltaTime * 1.2)) {
					m_sleeptime += 4;
					std::this_thread::sleep_for(std::chrono::milliseconds((long)(((m_timestep - m_deltaTime)) * m_maxLogicFrame) / m_sleeptime));
					ClockTiming();
				}
				else {
					m_sleeptime += 40;
					std::this_thread::sleep_for(std::chrono::milliseconds((long)(((m_timestep - m_deltaTime)) * m_maxLogicFrame) / m_sleeptime));
					ClockTiming();
				}
			}
			m_overframetime = 0.0;
		}
		else {
			m_overframetime = m_overframetime * 0.5;
		}
	}
	// Go to next profiling measurement, time spent after this call is shown in the next frame.
	m_logger.NextMeasurement();
	m_logger.StartLog(tc_logic);

	// Get last render start time.
	m_logictime = m_clock.GetTimeSecond();
	m_tottime = m_logictime - m_logictimestart;
	m_logictimestart = m_logictime;
	m_animationtime = m_logictime;
	m_rendertime = m_logictime;
	m_lastrendertime = m_rendertime - m_rendertimestart;
	FrameOver(); // logic time fix code
	// If we do not need render yet pass it.
	if (m_doRender && m_lastrendertime + m_overrendertime > m_renderrate) {
		m_needsRender = true;
		// get the render time for next time
		m_rendertimestart = m_rendertime;
		m_overrendertime = (m_lastrendertime - m_renderrate + m_overrendertime);
		if (m_flags & (SHOW_FRAMERATE)) {
			m_rendertimeaverage = ((m_rendertimeaverage * (m_ticrate - 1.0)) + m_lastrendertime) / m_ticrate;
		}

		//if (m_overrendertime > (1.5 / m_renderrate)) {
		//	m_overrendertime = 1.5 / m_renderrate;
		//}
	}
	else {
		// no render this time run endframe for timings
		m_needsRender = false;
	}

	// Get last Animation start time.
	//m_animationtime = m_clock.GetTimeSecond();
	m_lastanimationtime = m_animationtime - m_animationtimestart;
	// If we do not need render yet pass it.
	if (m_doRender && m_lastanimationtime + m_overanimationtime > m_animationrate) {
		m_needsAnimation = true;
		// get the render time for next time
		m_animationtimestart = m_animationtime;
		m_overanimationtime = ((m_lastanimationtime - m_animationrate) + m_overanimationtime);
		if (m_flags & (SHOW_FRAMERATE)) {
			m_animationtimeaverage = ((m_animationtimeaverage * (m_ticrate - 1.0)) + m_lastanimationtime) / m_ticrate;
		}
		//if (m_overanimationtime > (1.5 / m_animationrate)) {
		//	m_overanimationtime = 1.5 / m_animationrate;
		//}
	}
	else {
		m_needsAnimation = false;
	}

	// we get the real logic time here
	//m_logictime = m_clock.GetTimeSecond();
	//m_tottime = m_logictime - m_logictimestart;
	//m_logictimestart = m_logictime;
	//m_tottime = m_logger.GetAverage(); // old way

	LogAverage(); // python profile stuff

	//frame timing scale stuff
	FrameTiming();

	if (m_needsRender) {// needed or profile bugs
		if (m_flags & (SHOW_PROFILE | SHOW_FRAMERATE | SHOW_DEBUG_PROPERTIES)) {
			m_logger.StartLog(tc_network);
			RenderDebugProperties();
		}
		m_logger.StartLog(tc_rasterizer);

		EndFrameRun();

		m_logger.StartLog(tc_logic);

		FlushScreenshots();

		// swap backbuffer (drawing into this buffer) <-> front/visible buffer
		m_logger.StartLog(tc_latency);

		SwapBuffers();
		m_logger.StartLog(tc_rasterizer);

		EndDrawRun();
	}
	m_logger.StartLog(tc_logic);
}

bool KX_KetsjiEngine::NextFrame()
{
	return true;
}

KX_KetsjiEngine::CameraRenderData KX_KetsjiEngine::GetCameraRenderData(KX_Scene *scene, KX_Camera *camera,
                                                                       const RAS_Rect& displayArea, RAS_Rasterizer::StereoMode stereoMode, RAS_Rasterizer::StereoEye eye)
{
	KX_Camera *rendercam;
	/* In case of stereo we must copy the camera because it is used twice with different settings
	 * (modelview matrix). This copy use the same transform settings that the original camera
	 * and its name is based on with the eye number in addition.
	 */
	const bool usestereo = (stereoMode != RAS_Rasterizer::RAS_STEREO_NOSTEREO);
	if (usestereo) {
		rendercam = new KX_Camera(scene, KX_Scene::m_callbacks, *camera->GetCameraData(), true);
		rendercam->SetName("__stereo_" + camera->GetName() + "_" + std::to_string(eye) + "__");
		rendercam->NodeSetGlobalOrientation(camera->NodeGetWorldOrientation());
		rendercam->NodeSetWorldPosition(camera->NodeGetWorldPosition());
		rendercam->NodeSetWorldScale(camera->NodeGetWorldScaling());
		rendercam->NodeUpdate();
	}
	// Else use the native camera.
	else {
		rendercam = camera;
	}

	KX_Camera *cullingcam = rendercam;

	KX_SetActiveScene(scene);
//#ifdef WITH_PYTHON
	scene->RunDrawingCallbacks(KX_Scene::PRE_DRAW_SETUP, rendercam);
//#endif

	RAS_Rect area;
	RAS_Rect viewport;
	// Compute the area and the viewport based on the current display area and the optional camera viewport.
	GetSceneViewport(scene, rendercam, displayArea, area, viewport);
	// Compute the camera matrices: modelview and projection.
	rendercam->UpdateView(m_rasterizer, scene, stereoMode, eye, viewport, area);

	CameraRenderData cameraData(rendercam, cullingcam, area, viewport, stereoMode, eye);

	if (usestereo) {
		rendercam->Release();
	}

	return cameraData;
}

KX_KetsjiEngine::RenderData KX_KetsjiEngine::GetRenderData()
{
	const RAS_Rasterizer::StereoMode stereomode = m_rasterizer->GetStereoMode();
	const bool usestereo = (stereomode != RAS_Rasterizer::RAS_STEREO_NOSTEREO);
	// Set to true when each eye needs to be rendered in a separated off screen.
	const bool renderpereye = stereomode == RAS_Rasterizer::RAS_STEREO_INTERLACED ||
	                          stereomode == RAS_Rasterizer::RAS_STEREO_VINTERLACE ||
	                          stereomode == RAS_Rasterizer::RAS_STEREO_ANAGLYPH;

	RenderData renderData(stereomode, renderpereye);

	// The number of eyes to manage in case of stereo.
	const unsigned short numeyes = (usestereo) ? 2 : 1;
	// The number of frames in case of stereo, could be multiple for interlaced or anaglyph stereo.
	const unsigned short numframes = (renderpereye) ? 2 : 1;

	// The off screen corresponding to the frame.
	static const RAS_OffScreen::Type ofsType[] = {
		RAS_OffScreen::RAS_OFFSCREEN_EYE_LEFT0,
		RAS_OffScreen::RAS_OFFSCREEN_EYE_RIGHT0
	};

	// Pre-compute the display area used for stereo or normal rendering.
	std::vector<RAS_Rect> displayAreas;
	for (unsigned short eye = 0; eye < numeyes; ++eye) {
		displayAreas.push_back(m_rasterizer->GetRenderArea(m_canvas, stereomode, (RAS_Rasterizer::StereoEye)eye));
	}

	// Prepare override culling camera of each scenes, we don't manage stereo currently.
	for (KX_Scene *scene : m_scenes) {
		KX_Camera *overrideCullingCam = scene->GetOverrideCullingCamera();

		if (overrideCullingCam) {
			RAS_Rect area;
			RAS_Rect viewport;
			// Compute the area and the viewport based on the current display area and the optional camera viewport.
			GetSceneViewport(scene, overrideCullingCam, displayAreas[RAS_Rasterizer::RAS_STEREO_LEFTEYE], area, viewport);
			// Compute the camera matrices: modelview and projection.
			overrideCullingCam->UpdateView(m_rasterizer, scene, stereomode, RAS_Rasterizer::RAS_STEREO_LEFTEYE, viewport, area);
		}
	}

	for (unsigned short frame = 0; frame < numframes; ++frame) {
		renderData.m_frameDataList.emplace_back(ofsType[frame]);
		FrameRenderData& frameData = renderData.m_frameDataList.back();

		// Get the eyes managed per frame.
		std::vector<RAS_Rasterizer::StereoEye> eyes;
		// One eye per frame but different.
		if (renderpereye) {
			eyes = {(RAS_Rasterizer::StereoEye)frame};
		}
		// Two eyes for unique frame.
		else if (usestereo) {
			eyes = {RAS_Rasterizer::RAS_STEREO_LEFTEYE, RAS_Rasterizer::RAS_STEREO_RIGHTEYE};
		}
		// Only one eye for unique frame.
		else {
			eyes = {RAS_Rasterizer::RAS_STEREO_LEFTEYE};
		}

		for (KX_Scene *scene : m_scenes) {
			frameData.m_sceneDataList.emplace_back(scene);
			SceneRenderData& sceneFrameData = frameData.m_sceneDataList.back();

			KX_Camera *activecam = scene->GetActiveCamera();
			//KX_Camera *overrideCullingCam = scene->GetOverrideCullingCamera();
			for (KX_Camera *cam : scene->GetCameraList()) {
				if (cam != activecam && !cam->UseViewport()) {
					continue;
				}

				for (RAS_Rasterizer::StereoEye eye : eyes) {
					sceneFrameData.m_cameraDataList.push_back(GetCameraRenderData(scene, cam, displayAreas[eye],
					                                                              stereomode, eye));
				}
			}
		}
	}

	return renderData;
}

void KX_KetsjiEngine::Render()
{
	if (m_needsRender) {
		m_logger.StartLog(tc_rasterizer);
		BeginFrame();
		// Set vsync one time per frame
		SetSwapControl();
	}
	m_logger.StartLog(tc_logic);

	ReleaseMoveEvent();

//#ifdef WITH_SDL
		// Handle all SDL Joystick events here to share them for all scenes properly.
	short m_addrem[JOYINDEX_MAX] = {0};
	if (DEV_Joystick::HandleEvents(m_addrem)) {
//#  ifdef WITH_PYTHON
		updatePythonJoysticks(m_addrem);
//#  endif  // WITH_PYTHON
	}
//#endif  // WITH_SDL
	
	// for each scene, call the proceed functions
	for (KX_Scene *scene : m_scenes) {
		/* Suspension holds the physics and logic processing for an
		 * entire scene. Objects can be suspended individually, and
		 * the settings for that precede the logic and physics
		 * update. */

		m_logger.StartLog(tc_services); //culling

		UpdateObjectActivity(scene);

		if (!scene->IsSuspended()) {
			m_logger.StartLog(tc_physics);
			// set Python hooks for each scene
			KX_SetActiveScene(scene);
			// Perform physics calculations on the scene. This can involve
			// many iterations of the physics solver.
			//UpdatePhysics(scene);

			// If we do not need render yet pass it.
			if (m_needsAnimation) {
				m_logger.StartLog(tc_animations);
				// animations are real time
				UpdateAnimations(scene);
			}

			// Process sensors, and controllers
			m_logger.StartLog(tc_logic);

			LogicBeginFrame(scene);

			if (m_flags & FIXED_FRAMERATE) {
				m_logger.StartLog(tc_scenegraph);
				UpdateParents(scene);
				m_logger.StartLog(tc_logic);
			}

			Thread_Logic_1(scene);

			Thread_Logic_2(scene);

			Thread_Logic_3(scene);

			//if (m_logger.GetAverage(tc_logic) < m_logger.GetAverage(tc_outside)) {
			Low_Logic_1(scene);
			//}
			//else {
			High_Logic_1(scene);
			//}

			// Do some cleanup work for this logic frame
			LogicUpdateFrame(scene);

			LogicEndFrame(scene);

			if (m_flags & FIXED_FRAMERATE) {
				m_logger.StartLog(tc_scenegraph);
				UpdateParents(scene);
				//m_logger.StartLog(tc_physics);
				//scene->GetPhysicsEnvironment()->ProceedDeltaTimeCar(m_timestep, m_framestep);
			} //else {
			m_logger.StartLog(tc_physics);
			scene->GetPhysicsEnvironment()->ProceedDeltaTime(m_timestep, m_physicsTime);
			//}

			//m_logger.StartLog(tc_physics);
			// Perform physics calculations on the scene. This can involve
			// many iterations of the physics solver.
			//UpdatePhysics(scene);

			m_logger.StartLog(tc_scenegraph);
			UpdateParents(scene);
		}
		else {
			m_logger.StartLog(tc_logic);
			scene->SetSuspendedDelta(scene->GetSuspendedDelta() + m_framestep);
		}
		if (m_needsRender) {
			m_logger.StartLog(tc_overhead);
			RenderShadowBuffers(scene);
			m_logger.StartLog(tc_rasterizer);
			// Render only independent texture renderers here.
			RenderTextureRenderers(scene);
		}
	}

	m_logger.StartLog(tc_logic);

	ClearMessages();

	// update system devices
	//m_logger.StartLog(tc_logic);

	ClearInputs();

	ProcessScheduledLibraries();

	// scene management
	ProcessScheduledScenes();
	if (m_needsRender) {
		m_logger.StartLog(tc_rasterizer);

		RenderData renderData = GetRenderData();

		const int width = m_canvas->GetWidth();
		const int height = m_canvas->GetHeight();
		// clear the entire game screen with the border color
		// only once per frame
		m_rasterizer->SetViewport(0, 0, width, height);
		m_rasterizer->SetScissor(0, 0, width, height);

		//CanvasSize();

		FirstScene();

		// Used to detect when a camera is the first rendered an then doesn't request a depth clear.
		//unsigned short pass = 0;

		for (FrameRenderData& frameData : renderData.m_frameDataList) {
			// Current bound off screen.
			RAS_OffScreen *offScreen = m_canvas->GetOffScreen(frameData.m_ofsType);
			offScreen->Bind();

			// Clear off screen only before the first scene render.
			m_rasterizer->Clear(RAS_Rasterizer::RAS_COLOR_BUFFER_BIT | RAS_Rasterizer::RAS_DEPTH_BUFFER_BIT);

			// for each scene, call the proceed functions
			for (m_i = 0, m_size = frameData.m_sceneDataList.size(); m_i < m_size; ++m_i) {
				const SceneRenderData& sceneFrameData = frameData.m_sceneDataList[m_i];
				KX_Scene *scene = sceneFrameData.m_scene;

				m_isfirstscene = (m_i == 0);
				m_islastscene = (m_i == (m_size - 1));

				// pass the scene's worldsettings to the rasterizer
				//scene->GetWorldInfo()->UpdateWorldSettings(m_rasterizer);
				GetWorldInfoUpdateWorldSettings(scene);

				//m_rasterizer->SetAuxilaryClientInfo(scene);
				SetAuxilaryClientInfo(scene);

				// Draw the scene once for each camera with an enabled viewport or an active camera.
				for (const CameraRenderData& cameraFrameData : sceneFrameData.m_cameraDataList) {
					// do the rendering
					RenderCamera(scene, cameraFrameData, offScreen);
				}

				/* Choose final render off screen target. If the current off screen is using multisamples we
				 * are sure that it will be copied to a non-multisamples off screen before render the filters.
				 * In this case the targeted off screen is the same as the current off screen. */
				RAS_OffScreen::Type target;
				if (offScreen->GetSamples() > 0) {
					/* If the last scene is rendered it's useless to specify a multisamples off screen, we use then
					 * a non-multisamples off screen and avoid an extra off screen blit. */
					if (m_islastscene) {
						target = RAS_OffScreen::NextRenderOffScreen(frameData.m_ofsType);
					}
					else {
						target = frameData.m_ofsType;
					}
				}
				/* In case of non-multisamples a ping pong per scene render is made between a potentially multisamples
				 * off screen and a non-multisamples off screen as the both doesn't use multisamples. */
				else {
					target = RAS_OffScreen::NextRenderOffScreen(frameData.m_ofsType);
				}

				// Render filters and get output off screen.
				offScreen = PostRenderScene(scene, offScreen, m_canvas->GetOffScreen(target));
				frameData.m_ofsType = offScreen->GetType();
			}
		}

		m_canvas->SetViewPort(0, 0, width, height);

		// Compositing per eye off screens to screen.
		if (renderData.m_renderPerEye) {
			RAS_OffScreen *leftofs = m_canvas->GetOffScreen(renderData.m_frameDataList[0].m_ofsType);
			RAS_OffScreen *rightofs = m_canvas->GetOffScreen(renderData.m_frameDataList[1].m_ofsType);
			m_rasterizer->DrawStereoOffScreen(m_canvas, leftofs, rightofs, renderData.m_stereoMode);
		}
		// Else simply draw the off screen to screen.
		else {
			m_rasterizer->DrawOffScreen(m_canvas, m_canvas->GetOffScreen(renderData.m_frameDataList[0].m_ofsType));
		}
	}
	m_logger.StartLog(tc_logic);
	EndFrame();
}

void KX_KetsjiEngine::RequestExit(KX_ExitInfo::Code code)
{
	RequestExit(code, "");
}

void KX_KetsjiEngine::RequestExit(KX_ExitInfo::Code code, const std::string& fileName)
{
	m_exitInfo.m_code = code;
	m_exitInfo.m_fileName = fileName;
}

const KX_ExitInfo& KX_KetsjiEngine::GetExitInfo() const
{
	return m_exitInfo;
}

void KX_KetsjiEngine::EnableCameraOverride(const std::string& forscene, const mt::mat3& orientation,
		const mt::vec3& position, const RAS_CameraData& camdata)
{
	SetFlag(CAMERA_OVERRIDE, true);

	m_overrideSceneName = forscene;
	m_overrideCamOrientation = orientation;
	m_overrideCamPosition = position;
	m_overrideCamData = camdata;
}


void KX_KetsjiEngine::GetSceneViewport(KX_Scene *scene, KX_Camera *cam, const RAS_Rect& displayArea, RAS_Rect& area, RAS_Rect& viewport)
{
	// In this function we make sure the rasterizer settings are up-to-date.
	// We compute the viewport so that logic using this information is up-to-date.

	// Note we postpone computation of the projection matrix
	// so that we are using the latest camera position.

	if (cam->UseViewport()) {
		area = cam->GetViewport();
	}
	else {
		area = displayArea;
	}

	RAS_FramingManager::ComputeViewport(scene->GetFramingType(), area, viewport);
}

void KX_KetsjiEngine::UpdateAnimations(KX_Scene *scene)
{
	scene->UpdateAnimations(m_animationsTime, (m_flags & RESTRICT_ANIMATION) != 0);
}

void KX_KetsjiEngine::ClearInputs()
{
	m_inputDevice->ClearInputs();
}

void KX_KetsjiEngine::ReleaseMoveEvent()
{
	m_inputDevice->ReleaseMoveEvent();
}

void KX_KetsjiEngine::UpdateObjectActivity(KX_Scene *scene)
{
	scene->UpdateObjectActivity();
}

void KX_KetsjiEngine::LogicBeginFrame(KX_Scene *scene)
{
	scene->LogicBeginFrame(m_logicTime, m_framestep);
}

void KX_KetsjiEngine::LogicUpdateFrame(KX_Scene *scene)
{
	scene->LogicUpdateFrame(m_logicTime);
}

void KX_KetsjiEngine::LogicEndFrame(KX_Scene *scene)
{
	scene->LogicEndFrame();
}

//void KX_KetsjiEngine::UpdatePhysics(KX_Scene *scene)
//{
//	scene->GetPhysicsEnvironment()->ProceedDeltaTime(m_timestep, m_physicsTime);
//}

void KX_KetsjiEngine::UpdateParents(KX_Scene *scene)
{
	scene->UpdateParents();
}

void KX_KetsjiEngine::SetAuxilaryClientInfo(KX_Scene *scene)
{
	m_rasterizer->SetAuxilaryClientInfo(scene);
}

void KX_KetsjiEngine::ProcessScheduledLibraries()
{
	m_converter->ProcessScheduledLibraries();
}

void KX_KetsjiEngine::ClockTiming()
{
	m_clockTime = m_clock.GetTimeSecond();
	m_deltaTime = m_clockTime - m_previousRealTime;
}
void KX_KetsjiEngine::FrameOver()
{
	m_previousRealTime = m_clockTime;
	if (m_overframetime < 0.0) {
		if (m_timestep < (m_deltaTime - m_overframetime)) {
			m_overframetime = (m_timestep - m_deltaTime + m_overframetime + 6e-6);
		}
		else {
			m_overframetime = 0.0;
		}
	}
	else {
		if (m_timestep < (m_deltaTime)) {
			m_overframetime = (m_timestep - m_deltaTime + 6e-6);
		}
	}
}
void KX_KetsjiEngine::LogAverage()
{
	if (m_tottime < 1e-6) {
		m_tottime = 1e-6;
	}
//#ifdef WITH_PYTHON
	for (m_i = tc_first; m_i < tc_numCategories; ++m_i) {
		m_time = m_logger.GetAverage((KX_TimeCategory)m_i);
		PyObject *val = PyTuple_New(2);
		PyTuple_SetItem(val, 0, PyFloat_FromDouble(m_time * 1000.0));
		PyTuple_SetItem(val, 1, PyFloat_FromDouble(m_time / m_tottime * 100.0));

		PyDict_SetItemString(m_pyprofiledict, m_profileLabels[m_i].c_str(), val);
		Py_DECREF(val);
	}
//#endif

	m_average_framerate = 1.0 / m_tottime;
	//m_average_framerate = ((m_average_framerate * (m_ticrate - 1.0)) + 1.0 / m_tottime) / m_ticrate;
}
void KX_KetsjiEngine::FrameTiming()
{
	m_timestep = 1.0 / m_ticrate;
	m_framestep = m_timestep * m_timeScale;
	m_frameTime += m_framestep;
	if (m_flags & FIXED_FRAMERATE) {
		m_logicTime += m_framestep;
		m_physicsTime = m_framestep;
		m_animationsTime += m_framestep;
	}
	else {
		// Frame time with time scale, off set by frame rate change.
		m_logicTime += (m_timestep * m_logicScale);
		m_physicsTime = (m_framestep * m_physicsScale);
		m_animationsTime += (m_timestep * m_animationsScale);
	}
}

void KX_KetsjiEngine::Thread_Logic_1(KX_Scene *scene)
{
	scene->RunDrawingCallbacks(KX_Scene::THREAD_LOGIC_1, nullptr);
}

void KX_KetsjiEngine::Thread_Logic_2(KX_Scene *scene)
{
	scene->RunDrawingCallbacks(KX_Scene::THREAD_LOGIC_2, nullptr);
}

void KX_KetsjiEngine::Thread_Logic_3(KX_Scene *scene)
{
	scene->RunDrawingCallbacks(KX_Scene::THREAD_LOGIC_3, nullptr);
}

void KX_KetsjiEngine::Low_Logic_1(KX_Scene *scene)
{
	scene->RunDrawingCallbacks(KX_Scene::LOW_LOGIC_1, nullptr);
}

void KX_KetsjiEngine::High_Logic_1(KX_Scene *scene)
{
	scene->RunDrawingCallbacks(KX_Scene::HIGH_LOGIC_1, nullptr);
}


/*void KX_KetsjiEngine::CanvasSize()
{
	m_width = m_canvas->GetWidth();
	m_height = m_canvas->GetHeight();
	// clear the entire game screen with the border color
	// only once per frame
	m_rasterizer->SetViewport(0, 0, m_width, m_height);
	m_rasterizer->SetScissor(0, 0, m_width, m_height);
}*/

void KX_KetsjiEngine::FirstScene()
{
	KX_Scene *firstscene = m_scenes->GetFront();
	const RAS_FrameSettings &framesettings = firstscene->GetFramingType();
	// Use the framing bar color set in the Blender scenes
	m_rasterizer->SetClearColor(framesettings.BarRed(), framesettings.BarGreen(), framesettings.BarBlue(), 1.0f);
}

void KX_KetsjiEngine::GetWorldInfoUpdateWorldSettings(KX_Scene *scene)
{
	scene->GetWorldInfo()->UpdateWorldSettings(m_rasterizer);
}

void KX_KetsjiEngine::ClearMessages()
{
	m_networkMessageManager->ClearMessages();
}

void KX_KetsjiEngine::RenderShadowBuffers(KX_Scene *scene)
{
	//EXP_ListValue<KX_LightObject> *lightlist = scene->GetLightList();

	SetAuxilaryClientInfo(scene);

	for (KX_LightObject *light : scene->GetLightList()) {
		light->Update();
		RAS_ILightObject *raslight = light->GetLightData();
		if (light->GetVisible() && raslight->HasShadowBuffer() && raslight->NeedShadowUpdate()) {
			/* make temporary camera */
			RAS_CameraData camdata = RAS_CameraData();
			KX_Camera *cam = new KX_Camera(scene, KX_Scene::m_callbacks, camdata, true);
			cam->SetName("__shadow__cam__");

			mt::mat3x4 camtrans;

			/* binds framebuffer object, sets up camera .. */
			raslight->BindShadowBuffer(m_canvas, cam, camtrans);

			if (m_maxPhysicsFrame) {

				const std::vector<KX_GameObject *> objects = scene->CalculateVisibleMeshes(cam->GetFrustum(RAS_Rasterizer::RAS_STEREO_LEFTEYE), raslight->GetShadowLayer());

				//m_logger.StartLog(tc_overhead);

				/* render */
				m_rasterizer->Clear(RAS_Rasterizer::RAS_DEPTH_BUFFER_BIT | RAS_Rasterizer::RAS_COLOR_BUFFER_BIT);
				// Send a nullptr off screen because the viewport is binding it's using its own private one.
				scene->RenderBuckets(objects, RAS_Rasterizer::RAS_SHADOW, camtrans, m_rasterizer, nullptr);
			}
			else {
				m_logger.StartLog(tc_rasterizer);

				/* render */
				m_rasterizer->Clear(RAS_Rasterizer::RAS_DEPTH_BUFFER_BIT | RAS_Rasterizer::RAS_COLOR_BUFFER_BIT);
			}
			/* unbind framebuffer object, restore drawmode, free camera */
			raslight->UnbindShadowBuffer();
			cam->Release();
		}
	}
}

// Render independent texture renderers here.
void KX_KetsjiEngine::RenderTextureRenderers(KX_Scene *scene)
{
	scene->RenderTextureRenderers(KX_TextureRendererManager::VIEWPORT_INDEPENDENT, m_rasterizer, nullptr, nullptr, RAS_Rect(), RAS_Rect());
}

void KX_KetsjiEngine::FlushDebugDraw(KX_Scene *scene)
{
	scene->FlushDebugDraw(m_rasterizer, m_canvas);
}

void KX_KetsjiEngine::MotionBlur()
{
	m_rasterizer->MotionBlur();
}

void KX_KetsjiEngine::SwapBuffers()
{
	m_canvas->SwapBuffers();
}

void KX_KetsjiEngine::BeginFrameRun()
{
	m_rasterizer->BeginFrame(m_logicTime);
}

void KX_KetsjiEngine::BeginDraw()
{
	m_canvas->BeginDraw();
}

void KX_KetsjiEngine::EndFrameRun()
{
	m_rasterizer->EndFrame();
}

void KX_KetsjiEngine::FlushScreenshots()
{
	m_canvas->FlushScreenshots(m_rasterizer);
}

void KX_KetsjiEngine::EndDrawRun()
{
	m_canvas->EndDraw();
}

void KX_KetsjiEngine::SetSwapControl()
{
	m_canvas->SetSwapControl(m_canvas->GetSwapControl());
}

// update graphics
void KX_KetsjiEngine::RenderCamera(KX_Scene *scene, const CameraRenderData& cameraFrameData, RAS_OffScreen *offScreen)
{
	KX_Camera *rendercam = cameraFrameData.m_renderCamera;
	KX_Camera *cullingcam = cameraFrameData.m_cullingCamera;
	const RAS_Rect &area = cameraFrameData.m_area;
	const RAS_Rect &viewport = cameraFrameData.m_viewport;

	KX_SetActiveScene(scene);

	/* Render texture probes depending of the the current viewport and area, these texture probes are commonly the planar map
	 * which need to be recomputed by each view in case of multi-viewport or stereo.
	 */
	scene->RenderTextureRenderers(KX_TextureRendererManager::VIEWPORT_DEPENDENT, m_rasterizer, offScreen, rendercam, viewport, area);

	// set the viewport for this frame and scene
	const int left = viewport.GetLeft();
	const int bottom = viewport.GetBottom();
	const int width = viewport.GetWidth();
	const int height = viewport.GetHeight();
	m_rasterizer->SetViewport(left, bottom, width, height);
	m_rasterizer->SetScissor(left, bottom, width, height);

	/* Clear the depth after setting the scene viewport/scissor
	 * if it's not the first render pass. */
	//if (pass > 0) {
	m_rasterizer->Clear(RAS_Rasterizer::RAS_DEPTH_BUFFER_BIT);
	//}

	RAS_Rasterizer::StereoEye eye = cameraFrameData.m_eye;
	m_rasterizer->SetEye(eye);

	m_rasterizer->SetProjectionMatrix(rendercam->GetProjectionMatrix(eye));
	m_rasterizer->SetViewMatrix(rendercam->GetModelviewMatrix(eye), rendercam->NodeGetWorldScaling());

	if (m_isfirstscene) {
		KX_WorldInfo *worldInfo = scene->GetWorldInfo();
		// Update background and render it.
		worldInfo->UpdateBackGround(m_rasterizer);
		worldInfo->RenderBackground(m_rasterizer);
	}

	// The following actually reschedules all vertices to be
	// redrawn. There is a cache between the actual rescheduling
	// and this call though. Visibility is imparted when this call
	// runs through the individual objects.

	m_logger.StartLog(tc_services);

	const std::vector<KX_GameObject *> objects = scene->CalculateVisibleMeshes(cullingcam, eye, 0);

	
	// update levels of detail
	scene->UpdateObjectLods(cullingcam, objects);

	//if (m_flags & FIXED_FRAMERATE) {
	//	m_logger.StartLog(tc_animations);
	//	UpdateAnimations(scene);
	//}

	m_logger.StartLog(tc_rasterizer);

	// Draw debug infos like bouding box, armature ect.. if enabled.
	scene->DrawDebug(objects, m_showBoundingBox, m_showArmature);
	// Draw debug camera frustum.
	DrawDebugCameraFrustum(scene, cameraFrameData);
	DrawDebugShadowFrustum(scene);

//#ifdef WITH_PYTHON
	// Run any pre-drawing python callbacks
	scene->RunDrawingCallbacks(KX_Scene::PRE_DRAW, rendercam);
//#endif

	scene->RenderBuckets(objects, m_rasterizer->GetDrawingMode(), rendercam->GetWorldToCamera(), m_rasterizer, offScreen);

	if (scene->GetPhysicsEnvironment()) {
		scene->GetPhysicsEnvironment()->DebugDrawWorld();
	}
}

/*
 * To run once per scene
 */
RAS_OffScreen *KX_KetsjiEngine::PostRenderScene(KX_Scene *scene, RAS_OffScreen *inputofs, RAS_OffScreen *targetofs)
{
	KX_SetActiveScene(scene);

	FlushDebugDraw(scene);

	// We need to first make sure our viewport is correct (enabling multiple viewports can mess this up), only for filters.
	const int width = m_canvas->GetWidth();
	const int height = m_canvas->GetHeight();
	m_rasterizer->SetViewport(0, 0, width, height);
	m_rasterizer->SetScissor(0, 0, width, height);

	RAS_OffScreen *offScreen = scene->Render2DFilters(m_rasterizer, m_canvas, inputofs, targetofs);

//#ifdef WITH_PYTHON
	/* We can't deduce what camera should be passed to the python callbacks
	 * because the post draw callbacks are per scenes and not per cameras.
	 */
	scene->RunDrawingCallbacks(KX_Scene::POST_DRAW, nullptr);

	// Python draw callback can also call debug draw functions, so we have to clear debug shapes.
	FlushDebugDraw(scene);
//#endif

	return offScreen;
}

void KX_KetsjiEngine::StopEngine()
{
	if (m_bInitialized) {
		m_converter->FinalizeAsyncLoads();

		while (m_scenes->GetCount() > 0) {
			KX_Scene *scene = m_scenes->GetFront();
			DestructScene(scene);
			// WARNING: here the scene is a dangling pointer.
			m_scenes->Remove(0);
		}

		// cleanup all the stuff
		m_rasterizer->Exit();
	}
}

// Scene Management is able to switch between scenes
// and have several scenes running in parallel
void KX_KetsjiEngine::AddScene(KX_Scene *scene)
{
	m_scenes->Add(CM_AddRef(scene));
	PostProcessScene(scene);
}

void KX_KetsjiEngine::PostProcessScene(KX_Scene *scene)
{
	bool override_camera = (((m_flags & CAMERA_OVERRIDE) != 0) && (scene->GetName() == m_overrideSceneName));

	// if there is no activecamera, or the camera is being
	// overridden we need to construct a temporary camera
	if (!scene->GetActiveCamera() || override_camera) {
		CreateTemporaryCamera(scene, override_camera);
	}

	UpdateParents(scene);
}

void KX_KetsjiEngine::CreateTemporaryCamera(KX_Scene *scene, bool override_camera) {
	KX_Camera *activecam = nullptr;

	activecam = new KX_Camera(scene, KX_Scene::m_callbacks, override_camera ? m_overrideCamData : RAS_CameraData());
	activecam->SetName("__default__cam__");

	// set transformation
	if (override_camera) {
		activecam->NodeSetLocalPosition(m_overrideCamPosition);
		activecam->NodeSetLocalOrientation(m_overrideCamOrientation);
	}
	else {
		activecam->NodeSetLocalPosition(mt::zero3);
		activecam->NodeSetLocalOrientation(mt::mat3::Identity());
	}

	activecam->NodeUpdate();

	scene->GetCameraList()->Add(CM_AddRef(activecam));
	scene->SetActiveCamera(activecam);
	scene->GetObjectList()->Add(CM_AddRef(activecam));
	scene->GetRootParentList()->Add(CM_AddRef(activecam));
	//scene->GetOutSideList()->Add(CM_AddRef(activecam));
	// done with activecam
	activecam->Release();
}

void KX_KetsjiEngine::RenderDebugProperties()
{
	std::string debugtxt;
	//std::string debugtxtPc;
	//std::string debugtxtfps;
	int title_xmargin = -7;
	int title_y_top_margin = 4;
	int title_y_bottom_margin = 2;

	int const_xindent = 4;
	int const_ysize = 14;

	int xcoord = 12;    // mmmm, these constants were taken from blender source
	int ycoord = 17;    // to 'mimic' behavior

	int profile_indent = 72;

	static const mt::vec4 white(1.0f, 1.0f, 1.0f, 1.0f);
	static const mt::vec4 gray(0.2f, 0.1f, 0.0f, 0.3f);
	//static const mt::vec4 yellow(1.0f, 1.0f, 0.0f, 1.0f);
	//static const mt::vec4 red(1.0f, 0.0f, 0.0f, 1.0f);
	// static const mt::vec4 redw(1.0f, 0.4f, 0.4f, 1.0f);
	//static const mt::vec4 color(1.0f, 1.0f, 1.0f, 1.0f);
	

	if (m_flags & (SHOW_FRAMERATE | SHOW_PROFILE)) {
		// Title for profiling("Profile")
		// Adds the constant x indent (0 for now) to the title x margin
		m_debugDraw.RenderBox2d(mt::vec2(xcoord + const_xindent, (ycoord + 190)), mt::vec2(160, 300), gray);
		m_debugDraw.RenderText2d(" RETRO UPBGE 26016a", mt::vec2(xcoord + const_xindent + title_xmargin, ycoord), white);

		// Increase the indent by default increase
		ycoord += const_ysize;
		// Add the title indent afterwards
		ycoord += title_y_bottom_margin;
	}

	// Framerate display
	if (m_flags & SHOW_FRAMERATE) {
		m_debugDraw.RenderText2d("Logic Rate", mt::vec2(xcoord + const_xindent, ycoord), white);
		debugtxt = (boost::format("%5.2f | %.1f") % (m_tottime * 1000.0f) % (1.0f / m_tottime)).str();
		m_debugDraw.RenderText2d(debugtxt, mt::vec2(xcoord + const_xindent + profile_indent, ycoord), white);
		ycoord += const_ysize;
		m_debugDraw.RenderText2d("Render", mt::vec2(xcoord + const_xindent, ycoord), white);
		debugtxt = (boost::format("%5.2f | %.1f") % (m_rendertimeaverage * 1000.0f) % (1.0f / m_rendertimeaverage)).str();
		m_debugDraw.RenderText2d(debugtxt, mt::vec2(xcoord + const_xindent + profile_indent, ycoord), white);
		ycoord += const_ysize;
		m_debugDraw.RenderText2d("Animation", mt::vec2(xcoord + const_xindent, ycoord), white);
		debugtxt = (boost::format("%5.2f | %.1f") % (m_animationtimeaverage * 1000.0f) % (1.0f / m_animationtimeaverage)).str();
		m_debugDraw.RenderText2d(debugtxt, mt::vec2(xcoord + const_xindent + profile_indent, ycoord), white);
		ycoord += const_ysize;
		//debugtxtfps = (boost::format("(%.1f)") % (1.0f / m_tottime)).str();
		
		//m_debugDraw.RenderText2d(debugtxt, mt::vec2(xcoord + const_xindent + profile_indent, ycoord), white);
		//m_debugDraw.RenderText2d("", mt::vec2((xcoord * (0.75f * debugtxt.length())) + const_xindent + profile_indent, ycoord), white);
		//m_debugDraw.RenderText2d(debugtxtfps, mt::vec2((xcoord * (0.5f * debugtxt.length() + 2.3f)) + const_xindent + profile_indent, ycoord), white);

		//const mt::vec2 framebox1((0.5f * ((debugtxt.length() + debugtxtfps.length() + 12) * 13.3f)), 14);
		//m_debugDraw.RenderBox2d(mt::vec2(xcoord + const_xindent, (ycoord+2)), framebox1, gray);
		// Increase the indent by default increase
		//ycoord += const_ysize;
		
		//m_debugDraw.RenderText2d("Render", mt::vec2(xcoord + const_xindent, ycoord), white);

		//debugtxt = (boost::format("%5.2f") % (m_rendertimeaverage * 1000.0f)).str();
		//debugtxtfps = (boost::format("(%.1f)") % (1.0f / m_rendertimeaverage)).str();
		
		/*if ((1.0f / m_tottime) < 30) {
			color = yellow;
		}
		if ((1.0f / m_tottime) < 24) {
			color = yellow;
		}*/
		//m_debugDraw.RenderText2d(debugtxt, mt::vec2(xcoord + const_xindent + profile_indent, ycoord), white);
		//m_debugDraw.RenderText2d("", mt::vec2((xcoord * (0.75f * debugtxt.length())) + const_xindent + profile_indent, ycoord), white);
		//m_debugDraw.RenderText2d(debugtxtfps, mt::vec2((xcoord * (0.5f * debugtxt.length() + 2.3f)) + const_xindent + profile_indent, ycoord), white);
		// m_debugDraw.RenderText2d(")", mt::vec2((xcoord * (0.5f * (debugtxt.length() + debugtxtfps.length() + 5.8f))) + const_xindent + profile_indent, ycoord), white);
		//const mt::vec2 framebox2((0.5f * ((debugtxt.length() + debugtxtfps.length() + 12) * 13.3f)), 14);
		//m_debugDraw.RenderBox2d(mt::vec2(xcoord + const_xindent, (ycoord+2)), framebox2, gray);
		// Increase the indent by default increase
		//ycoord += const_ysize;

		//m_debugDraw.RenderText2d("Animation", mt::vec2(xcoord + const_xindent, ycoord), white);

		//debugtxt = (boost::format("%5.2f") % (m_animationtimeaverage * 1000.0f)).str();
		//debugtxtfps = (boost::format("(%.1f)") % (1.0f / m_animationtimeaverage)).str();
		
		//m_debugDraw.RenderText2d(debugtxt, mt::vec2(xcoord + const_xindent + profile_indent, ycoord), white);
		//m_debugDraw.RenderText2d("", mt::vec2((xcoord * (0.75f * debugtxt.length())) + const_xindent + profile_indent, ycoord), white);
		//m_debugDraw.RenderText2d(debugtxtfps, mt::vec2((xcoord * (0.5f * debugtxt.length() + 2.3f)) + const_xindent + profile_indent, ycoord), white);
		// m_debugDraw.RenderText2d(")", mt::vec2((xcoord * (0.5f * (debugtxt.length() + debugtxtfps.length() + 5.8f))) + const_xindent + profile_indent, ycoord), white);
		//const mt::vec2 framebox3((0.5f * ((debugtxt.length() + debugtxtfps.length() + 12) * 13.3f)), 14);
		//m_debugDraw.RenderBox2d(mt::vec2(xcoord + const_xindent, (ycoord+2)), framebox3, gray);
		// Increase the indent by default increase
		//ycoord += const_ysize;
	}
	// Profile display
	if (m_flags & SHOW_PROFILE) {
		//m_debugDraw.RenderBox2d(mt::vec2(xcoord + const_xindent, (ycoord + 128)), mt::vec2(145, 140), gray);
		for (m_i = tc_first; m_i < tc_numCategories; m_i++) {
			m_debugDraw.RenderText2d(m_profileLabels[m_i], mt::vec2(xcoord + const_xindent, ycoord), white);
			m_time = m_logger.GetAverage((KX_TimeCategory)m_i);

			debugtxt = (boost::format("%5.2f | %d%%") % (m_time * 1000.f) %
                  (int)(m_time / m_tottime * 100.f))
                     .str();
            m_debugDraw.RenderText2d(debugtxt, mt::vec2(xcoord + const_xindent + profile_indent, ycoord), white);
			//const mt::vec2 boxSize(50 * (m_time / m_tottime), 10);
			//m_debugDraw.RenderBox2d(mt::vec2(xcoord + (int)(2.2 * profile_indent), ycoord), boxSize, white);
			ycoord += const_ysize;
			/*float percentage = (int)(m_time / m_tottime * 100.f);
			if (j != tc_latency) {
				if (percentage > 25) {
					color = yellow;
				}
				if (percentage > 50) {
					color = red;
				}
			}
			else {
				if (percentage < 50) {
					color = yellow;
				}
				if (percentage < 10) {
					color = red;
				}
			}*/
			//debugtxt = (boost::format("%5.2f    %d%%|") % (m_time * 1000.f) % (int)(m_time / m_tottime * 100.f)).str();
			/*debugtxt = (boost::format("%5.2f") % (m_time * 1000.f)).str();
			debugtxtPc = (boost::format("%d%%") % (int)(m_time / m_tottime * 100.f)).str();

			m_debugDraw.RenderText2d("", mt::vec2((xcoord * (0.75f *  5)) + const_xindent + profile_indent, ycoord), white);

			const mt::vec2 boxSize(50 * (m_time / m_tottime), 9);
			const mt::vec2 boxSizeout(50 * (m_time / m_tottime) + 2, 13);

			m_debugDraw.RenderText2d(debugtxt, mt::vec2(xcoord + const_xindent + profile_indent, ycoord), white);
			//if ((int)(m_time / m_tottime * 100.f) < 30) {
			//	m_debugDraw.RenderText2d(debugtxtPc, mt::vec2(xcoord + const_xindent + profile_indent + 50, ycoord), white);
			//}
			//else {
			m_debugDraw.RenderText2d(debugtxtPc, mt::vec2(xcoord + const_xindent + profile_indent + ((boxSize.x / 2) + 63), ycoord), white);
			//}
			

			m_debugDraw.RenderBox2d(mt::vec2(xcoord + (int)(2.05f * profile_indent + 1), (ycoord + 2)), boxSizeout, gray);
			m_debugDraw.RenderBox2d(mt::vec2(xcoord + (int)(2.05f * profile_indent), ycoord), boxSize, gray); // white box
			
			ycoord += const_ysize;*/
		}
	}

	/*if (m_flags & SHOW_RENDER_QUERIES) {
		m_debugDraw.RenderText2d("Render Queries :", mt::vec2(xcoord + const_xindent + title_xmargin, ycoord), white);
		ycoord += const_ysize;

		for (unsigned short i = 0; i < QUERY_MAX; ++i) {
			m_debugDraw.RenderText2d(m_renderQueriesLabels[i], mt::vec2(xcoord + const_xindent, ycoord), white);

			if (i == QUERY_TIME) {
				debugtxt = (boost::format("%.2fms") % (((float)m_renderQueries[i].Result()) / 1e6)).str();
			}
			else {
				debugtxt = (boost::format("%i") % m_renderQueries[i].Result()).str();
			}

			m_debugDraw.RenderText2d(debugtxt, mt::vec2(xcoord + const_xindent + profile_indent, ycoord), white);
			ycoord += const_ysize;
		}
	}
*/
	// Add the ymargin for titles below the other section of debug info
	ycoord += title_y_top_margin;

	/* Property display */
	if (m_flags & SHOW_DEBUG_PROPERTIES) {
		// Title for debugging("Debug properties")
		// Adds the constant x indent (0 for now) to the title x margin
		m_debugDraw.RenderText2d("Debug Properties", mt::vec2(xcoord + const_xindent + title_xmargin, ycoord), white);

		// Increase the indent by default increase
		ycoord += const_ysize;
		// Add the title indent afterwards
		ycoord += title_y_bottom_margin;

		/* Calculate amount of properties that can displayed. */
		const unsigned short propsMax = (m_canvas->GetHeight() - ycoord) / const_ysize;

		for (KX_Scene *scene : m_scenes) {
			scene->RenderDebugProperties(m_debugDraw, const_xindent, const_ysize, xcoord, ycoord, propsMax);
		}
	}

	m_debugDraw.Flush(m_rasterizer, m_canvas);
}


/*void KX_KetsjiEngine::RenderDebugProperties()
{
	std::string debugtxt;
	std::string debugtxtPc;
	std::string debugtxtfps;
	int title_xmargin = -7;
	int title_y_top_margin = 4;
	int title_y_bottom_margin = 2;

	int const_xindent = 4;
	int const_ysize = 14;

	int xcoord = 12;    // mmmm, these constants were taken from blender source
	int ycoord = 17;    // to 'mimic' behavior

	int profile_indent = 72;

	//float tottime = m_logger.GetAverage();
	//if (tottime < 1e-6f) {
	//	tottime = 1e-6f;
	//}

	//static const mt::vec4 white(0.9f, 0.9f, 0.9f, 0.9f);
	//static const mt::vec4 gray(0.0f, 0.0f, 0.0f, 0.0f);
	//static const mt::vec4 yellow(1.0f, 1.0f, 1.0f, 1.0f);
	//static const mt::vec4 red(0.9f, 0.9f, 0.9f, 0.9f);
	// static const mt::vec4 redw(1.0f, 0.4f, 0.4f, 1.0f);
	static const mt::vec4 color(0.9f, 0.9f, 0.9f, 0.9f);
	

	if (m_flags & (SHOW_FRAMERATE | SHOW_PROFILE)) {
		// Title for profiling("Profile")
		// Adds the constant x indent (0 for now) to the title x margin
		m_debugDraw.RenderText2d("UPBGE 0.2.6012a", mt::vec2(xcoord + const_xindent + title_xmargin, ycoord), color);

		// Increase the indent by default increase
		ycoord += const_ysize;
		// Add the title indent afterwards
		ycoord += title_y_bottom_margin;
	}

	// Framerate display
	if (m_flags & SHOW_FRAMERATE) {
		m_debugDraw.RenderText2d("Render Time", mt::vec2(xcoord + const_xindent, ycoord), color);

		debugtxt = (boost::format("%5.2f") % (m_tottime * 1000.0f)).str();
		debugtxtfps = (boost::format("%.1f RPS") % (1.0f / m_tottime)).str();
		/*if ((1.0f / m_tottime) < 55) {
			color = yellow;
		}
		if ((1.0f / m_tottime) < 25) {
			color = color;
		}
		m_debugDraw.RenderText2d(debugtxt, mt::vec2(xcoord + const_xindent + profile_indent, ycoord), color);
		//m_debugDraw.RenderText2d("", mt::vec2((xcoord * (0.75f * debugtxt.length())) + const_xindent + profile_indent, ycoord), color);
		m_debugDraw.RenderText2d(debugtxtfps, mt::vec2((xcoord * (0.5f * debugtxt.length() + 2.3f)) + const_xindent + profile_indent, ycoord), color);
		// m_debugDraw.RenderText2d(")", mt::vec2((xcoord * (0.5f * (debugtxt.length() + debugtxtfps.length() + 5.8f))) + const_xindent + profile_indent, ycoord), color);
		//const mt::vec2 framebox((0.5f * ((debugtxt.length() + debugtxtfps.length() + 12) * 13.3f)), 14);
		//m_debugDraw.RenderBox2d(mt::vec2(xcoord + const_xindent, (ycoord+2)), framebox, gray);
		// Increase the indent by default increase
		ycoord += const_ysize;
	}
	// Profile display
	if (m_flags & SHOW_PROFILE) {
		//m_debugDraw.RenderBox2d(mt::vec2(xcoord + const_xindent, (ycoord + 128)), mt::vec2(145, 140), gray);
		for (int j = tc_first; j < tc_numCategories; j++) {
			m_debugDraw.RenderText2d(m_profileLabels[j], mt::vec2(xcoord + const_xindent, ycoord), color);
			m_time = m_logger.GetAverage((KX_TimeCategory)j);
			/*float percentage = (int)(time / m_tottime * 100.f);
			if (j != tc_latency) {
				if (percentage > 30) {
					color = yellow;
				}
				if (percentage > 40) {
					color = color;
				}
			}
			else {
				if (percentage < 50) {
					color = yellow;
				}
				if (percentage < 30) {
					color = color;
				}
			}
			//debugtxt = (boost::format("%5.2f    %d%%|") % (time * 1000.f) % (int)(time / m_tottime * 100.f)).str();
			debugtxt = (boost::format("%5.2f") % (m_time * 1000.f)).str();
			debugtxtPc = (boost::format("%d%%") % (int)(m_time / m_tottime * 100.f)).str();

			m_debugDraw.RenderText2d("", mt::vec2((xcoord * (0.75f *  5)) + const_xindent + profile_indent, ycoord), color);

			const mt::vec2 boxSize(50 * (m_time / m_tottime), 9);
			const mt::vec2 boxSizeout(50 * (m_time / m_tottime) + 2, 13);

			m_debugDraw.RenderText2d(debugtxt, mt::vec2(xcoord + const_xindent + profile_indent, ycoord), color);
			if ((int)(m_time / m_tottime * 100.f) < 30) {
				m_debugDraw.RenderText2d(debugtxtPc, mt::vec2(xcoord + const_xindent + profile_indent + 50, ycoord), color);
			}
			else {
				m_debugDraw.RenderText2d(debugtxtPc, mt::vec2(xcoord + const_xindent + profile_indent + ((boxSize.x / 2) + 63), ycoord), color);
			}
			

			//m_debugDraw.RenderBox2d(mt::vec2(xcoord + (int)(2.05f * profile_indent + 1), (ycoord + 2)), boxSizeout, gray);
			//m_debugDraw.RenderBox2d(mt::vec2(xcoord + (int)(2.05f * profile_indent), ycoord), boxSize, gray); // color box
			
			ycoord += const_ysize;
		}
	}

	/*if (m_flags & SHOW_RENDER_QUERIES) {
		m_debugDraw.RenderText2d("Render Queries :", mt::vec2(xcoord + const_xindent + title_xmargin, ycoord), color);
		ycoord += const_ysize;

		for (unsigned short i = 0; i < QUERY_MAX; ++i) {
			m_debugDraw.RenderText2d(m_renderQueriesLabels[i], mt::vec2(xcoord + const_xindent, ycoord), color);

			if (i == QUERY_TIME) {
				debugtxt = (boost::format("%.2fms") % (((float)m_renderQueries[i].Result()) / 1e6)).str();
			}
			else {
				debugtxt = (boost::format("%i") % m_renderQueries[i].Result()).str();
			}

			m_debugDraw.RenderText2d(debugtxt, mt::vec2(xcoord + const_xindent + profile_indent, ycoord), color);
			ycoord += const_ysize;
		}
	}

	// Add the ymargin for titles below the other section of debug info
	ycoord += title_y_top_margin;

	/* Property display 
	if (m_flags & SHOW_DEBUG_PROPERTIES) {
		// Title for debugging("Debug properties")
		// Adds the constant x indent (0 for now) to the title x margin
		m_debugDraw.RenderText2d("Properties", mt::vec2(xcoord + const_xindent + title_xmargin, ycoord), color);

		// Increase the indent by default increase
		ycoord += const_ysize;
		// Add the title indent afterwards
		ycoord += title_y_bottom_margin;

		/* Calculate amount of properties that can displayed. */
		/*const unsigned short propsMax = (m_height - ycoord) / const_ysize;

		for (KX_Scene *scene : m_scenes) {
			scene->RenderDebugProperties(m_debugDraw, const_xindent, const_ysize, xcoord, ycoord, propsMax);
		}
	}

	m_debugDraw.Flush(m_rasterizer, m_canvas);
}
*/
void KX_KetsjiEngine::DrawDebugCameraFrustum(KX_Scene *scene, const CameraRenderData& cameraFrameData)
{
	if (m_showCameraFrustum == KX_DebugOption::DISABLE) {
		return;
	}

	RAS_DebugDraw& debugDraw = scene->GetDebugDraw();
	for (KX_Camera *cam : scene->GetCameraList()) {
		if (cam != cameraFrameData.m_renderCamera && (m_showCameraFrustum == KX_DebugOption::FORCE || cam->GetShowCameraFrustum())) {

			cam->UpdateView(m_rasterizer, scene, cameraFrameData.m_stereoMode, cameraFrameData.m_eye,
					cameraFrameData.m_viewport, cameraFrameData.m_area);

			debugDraw.DrawCameraFrustum(
				cam->GetProjectionMatrix(cameraFrameData.m_eye) * cam->GetModelviewMatrix(cameraFrameData.m_eye));
		}
	}
}

void KX_KetsjiEngine::DrawDebugShadowFrustum(KX_Scene *scene)
{
	if (m_showShadowFrustum == KX_DebugOption::DISABLE) {
		return;
	}

	RAS_DebugDraw& debugDraw = scene->GetDebugDraw();
	for (KX_LightObject *light : scene->GetLightList()) {
		RAS_ILightObject *raslight = light->GetLightData();
		if (m_showShadowFrustum == KX_DebugOption::FORCE || light->GetShowShadowFrustum()) {
			const mt::mat4 projmat(raslight->GetWinMat());
			const mt::mat4 viewmat(raslight->GetViewMat());

			debugDraw.DrawCameraFrustum(projmat * viewmat);
		}
	}
}

EXP_ListValue<KX_Scene> *KX_KetsjiEngine::CurrentScenes()
{
	return m_scenes;
}

KX_Scene *KX_KetsjiEngine::FindScene(const std::string& scenename)
{
	return m_scenes->FindValue(scenename);
}

void KX_KetsjiEngine::ConvertAndAddScene(const std::string& scenename, bool overlay)
{
	// only add scene when it doesn't exist!
	if (!FindScene(scenename)) {
		if (overlay) {
			m_addingOverlayScenes.push_back(scenename);
		}
		else {
			m_addingBackgroundScenes.push_back(scenename);
		}
	}
}

void KX_KetsjiEngine::RemoveScene(const std::string& scenename)
{
	if (FindScene(scenename)) {
		m_removingScenes.push_back(scenename);
	}
}

void KX_KetsjiEngine::RemoveScheduledScenes()
{
	if (!m_removingScenes.empty()) {
		std::vector<std::string>::iterator scenenameit;
		for (scenenameit = m_removingScenes.begin(); scenenameit != m_removingScenes.end(); scenenameit++) {
			std::string scenename = *scenenameit;

			KX_Scene *scene = FindScene(scenename);
			if (scene) {
				DestructScene(scene);
				m_scenes->RemoveValue(scene);
			}
		}
		m_removingScenes.clear();
	}
}

KX_Scene *KX_KetsjiEngine::CreateScene(Scene *scene)
{
	KX_Scene *tmpscene = new KX_Scene(m_inputDevice,
	                                  scene->id.name + 2,
	                                  scene,
	                                  m_canvas,
	                                  m_networkMessageManager);

	return tmpscene;
}

KX_Scene *KX_KetsjiEngine::CreateScene(const std::string& scenename)
{
	Scene *scene = m_converter->GetBlenderSceneForName(scenename);
	if (!scene) {
		return nullptr;
	}

	return CreateScene(scene);
}

void KX_KetsjiEngine::AddScheduledScenes()
{
	if (!m_addingOverlayScenes.empty()) {
		for (const std::string& scenename : m_addingOverlayScenes) {
			KX_Scene *tmpscene = CreateScene(scenename);

			if (tmpscene) {
				m_converter->ConvertScene(tmpscene);
				m_scenes->Add(CM_AddRef(tmpscene));
				PostProcessScene(tmpscene);
				tmpscene->Release();
			}
		}
		m_addingOverlayScenes.clear();
	}

	if (!m_addingBackgroundScenes.empty()) {
		for (const std::string& scenename : m_addingBackgroundScenes) {
			KX_Scene *tmpscene = CreateScene(scenename);

			if (tmpscene) {
				m_converter->ConvertScene(tmpscene);
				m_scenes->Insert(0, CM_AddRef(tmpscene));
				PostProcessScene(tmpscene);
				tmpscene->Release();
			}
		}
		m_addingBackgroundScenes.clear();
	}
}

bool KX_KetsjiEngine::ReplaceScene(const std::string& oldscene, const std::string& newscene)
{
	// Don't allow replacement if the new scene doesn't exist.
	// Allows smarter game design (used to have no check here).
	// Note that it creates a small backward compatbility issue
	// for a game that did a replace followed by a lib load with the
	// new scene in the lib => it won't work anymore, the lib
	// must be loaded before doing the replace.
	if (m_converter->GetBlenderSceneForName(newscene)) {
		m_replace_scenes.emplace_back(oldscene, newscene);
		return true;
	}

	return false;
}

// replace scene is not the same as removing and adding because the
// scene must be in exact the same place (to maintain drawingorder)
// (nzc) - should that not be done with a scene-display list? It seems
// stupid to rely on the mem allocation order...
void KX_KetsjiEngine::ReplaceScheduledScenes()
{
	if (!m_replace_scenes.empty()) {
		std::vector<std::pair<std::string, std::string> >::iterator scenenameit;

		for (scenenameit = m_replace_scenes.begin();
		     scenenameit != m_replace_scenes.end();
		     scenenameit++)
		{
			std::string oldscenename = (*scenenameit).first;
			std::string newscenename = (*scenenameit).second;
			/* Scenes are not supposed to be included twice... I think */
			for (unsigned int sce_idx = 0; sce_idx < m_scenes->GetCount(); ++sce_idx) {
				KX_Scene *scene = m_scenes->GetValue(sce_idx);
				if (scene->GetName() == oldscenename) {
					// avoid crash if the new scene doesn't exist, just do nothing
					Scene *blScene = m_converter->GetBlenderSceneForName(newscenename);
					if (blScene) {
						DestructScene(scene);

						KX_Scene *tmpscene = CreateScene(blScene);
						m_converter->ConvertScene(tmpscene);

						m_scenes->SetValue(sce_idx, CM_AddRef(tmpscene));
						PostProcessScene(tmpscene);
						tmpscene->Release();
					}
				}
			}
		}
		m_replace_scenes.clear();
	}
}

void KX_KetsjiEngine::SuspendScene(const std::string& scenename)
{
	KX_Scene *scene = FindScene(scenename);
	if (scene) {
		scene->Suspend();
	}
}

void KX_KetsjiEngine::ResumeScene(const std::string& scenename)
{
	KX_Scene *scene = FindScene(scenename);
	if (scene) {
		scene->Resume();
	}
}

void KX_KetsjiEngine::DestructScene(KX_Scene *scene)
{
	scene->RunOnRemoveCallbacks();
	m_converter->RemoveScene(scene);
}

double KX_KetsjiEngine::GetTicRate()
{
	return m_ticrate;
}

void KX_KetsjiEngine::SetTicRate(double ticrate)
{
	m_ticrate = ticrate;
}

double KX_KetsjiEngine::GetRenderRate()
{
	return 1.0 / m_renderrate;
}

void KX_KetsjiEngine::SetRenderRate(double renderrate)
{
	m_renderrate = 1.0 / renderrate;
}

double KX_KetsjiEngine::GetAnimationRate()
{
	return 1.0 / m_animationrate;
}

void KX_KetsjiEngine::SetAnimationRate(double animationrate)
{
	m_animationrate = 1.0 / animationrate;
}

double KX_KetsjiEngine::GetTimeScale() const
{
	return m_timeScale;
}

void KX_KetsjiEngine::SetTimeScale(double timeScale)
{
	m_timeScale = timeScale;
}

double KX_KetsjiEngine::GetLogicScale() const
{
	return m_logicScale;
}

void KX_KetsjiEngine::SetLogicScale(double logicScale)
{
	m_logicScale = logicScale;
}

double KX_KetsjiEngine::GetPhysicsScale() const
{
	return m_physicsScale;
}

void KX_KetsjiEngine::SetPhysicsScale(double physicsScale)
{
	m_physicsScale = physicsScale;
}

double KX_KetsjiEngine::GetAnimationsScale() const
{
	return m_animationsScale;
}

void KX_KetsjiEngine::SetAnimationsScale(double animationsScale)
{
	m_animationsScale = animationsScale;
}

int KX_KetsjiEngine::GetMaxLogicFrame()
{
	return m_maxLogicFrame;
}

void KX_KetsjiEngine::SetMaxLogicFrame(int frame)
{
	m_maxLogicFrame = frame;
}

bool KX_KetsjiEngine::GetMaxPhysicsFrame()
{
	return m_maxPhysicsFrame;
}

void KX_KetsjiEngine::SetMaxPhysicsFrame(bool frame)
{
	m_maxPhysicsFrame = frame;
}

double KX_KetsjiEngine::GetEngineDeltaTime()
{
	return m_deltaTime;
}

double KX_KetsjiEngine::GetAnimFrameRate()
{
	return m_anim_framerate;
}

bool KX_KetsjiEngine::GetFlag(FlagType flag) const
{
	return (m_flags & flag) != 0;
}

void KX_KetsjiEngine::SetFlag(FlagType flag, bool enable)
{
	if (enable) {
		m_flags = (FlagType)(m_flags | flag);
	}
	else {
		m_flags = (FlagType)(m_flags & ~flag);
	}
}

double KX_KetsjiEngine::GetClockTime() const
{
	return m_clockTime;
}
void KX_KetsjiEngine::SetClockTime(double externalClockTime)
{
	m_clockTime = externalClockTime;
}

double KX_KetsjiEngine::GetFrameTime() const
{
	return m_frameTime;
}

double KX_KetsjiEngine::GetRealTime() const
{
	return m_clock.GetTimeSecond();
}

void KX_KetsjiEngine::SetAnimFrameRate(double framerate)
{
	m_anim_framerate = framerate;
}

double KX_KetsjiEngine::GetAverageFrameRate()
{
	return m_average_framerate;
}

double KX_KetsjiEngine::GetAverageRenderRate()
{
	return 1.0 / m_lastrendertime;
}

void KX_KetsjiEngine::SetExitKey(SCA_IInputDevice::SCA_EnumInputs key)
{
	m_exitKey = key;
}

SCA_IInputDevice::SCA_EnumInputs KX_KetsjiEngine::GetExitKey() const
{
	return m_exitKey;
}

void KX_KetsjiEngine::SetRender(bool render)
{
	m_doRender = render;
}

bool KX_KetsjiEngine::GetRender()
{
	return m_doRender;
}

void KX_KetsjiEngine::ProcessScheduledScenes()
{
	// Check whether there will be changes to the list of scenes
	if (!(m_addingOverlayScenes.empty() && m_addingBackgroundScenes.empty() &&
	      m_replace_scenes.empty() && m_removingScenes.empty())) {
		// Change the scene list
		ReplaceScheduledScenes();
		RemoveScheduledScenes();
		AddScheduledScenes();
	}

	if (m_scenes->Empty()) {
		RequestExit(KX_ExitInfo::NO_SCENES_LEFT);
	}
}

void KX_KetsjiEngine::SetShowBoundingBox(KX_DebugOption mode)
{
	m_showBoundingBox = mode;
}

KX_DebugOption KX_KetsjiEngine::GetShowBoundingBox() const
{
	return m_showBoundingBox;
}

void KX_KetsjiEngine::SetShowArmatures(KX_DebugOption mode)
{
	m_showArmature = mode;
}

KX_DebugOption KX_KetsjiEngine::GetShowArmatures() const
{
	return m_showArmature;
}

void KX_KetsjiEngine::SetShowCameraFrustum(KX_DebugOption mode)
{
	m_showCameraFrustum = mode;
}

KX_DebugOption KX_KetsjiEngine::GetShowCameraFrustum() const
{
	return m_showCameraFrustum;
}

void KX_KetsjiEngine::SetShowShadowFrustum(KX_DebugOption mode)
{
	m_showShadowFrustum = mode;
}

KX_DebugOption KX_KetsjiEngine::GetShowShadowFrustum() const
{
	return m_showShadowFrustum;
}

void KX_KetsjiEngine::Resize()
{
	/* extended mode needs to recalculate camera frusta when */
	KX_Scene *firstscene = m_scenes->GetFront();
	const RAS_FrameSettings &framesettings = firstscene->GetFramingType();

	if (framesettings.FrameType() == RAS_FrameSettings::e_frame_extend) {
		for (KX_Scene *scene : m_scenes) {
			KX_Camera *cam = scene->GetActiveCamera();
			cam->InvalidateProjectionMatrix();
		}
	}
}

void KX_KetsjiEngine::SetGlobalSettings(GlobalSettings *gs)
{
	m_globalsettings.glslflag = gs->glslflag;
}

GlobalSettings *KX_KetsjiEngine::GetGlobalSettings()
{
	return &m_globalsettings;
}
