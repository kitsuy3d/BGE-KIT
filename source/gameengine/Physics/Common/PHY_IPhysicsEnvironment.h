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
 */

/** \file PHY_IPhysicsEnvironment.h
 *  \ingroup phys
 */

#ifndef __PHY_IPHYSICSENVIRONMENT_H__
#define __PHY_IPHYSICSENVIRONMENT_H__

#include "PHY_DynamicTypes.h"

#include <array>

class PHY_IConstraint;
class PHY_IVehicle;
class PHY_ICharacter;
class RAS_Mesh;
class PHY_IPhysicsController;

class RAS_Mesh;
class KX_GameObject;
class KX_Scene;
class BL_SceneConverter;

class PHY_IMotionState;
struct bRigidBodyJointConstraint;

/**
 * pass back information from rayTest
 */
struct PHY_RayCastResult {
	PHY_IPhysicsController *m_controller;
	mt::vec3 m_hitPoint;
	mt::vec3 m_hitNormal;
	RAS_Mesh *m_meshObject; // !=nullptr for mesh object (only for Bullet controllers)
	int m_polygon; // index of the polygon hit by the ray, only if m_meshObject != nullptr
	int m_hitUVOK; // !=0 if UV coordinate in m_hitUV is valid
	mt::vec2 m_hitUV; // UV coordinates of hit point
};

/**
 * This class replaces the ignoreController parameter of rayTest function.
 * It allows more sophisticated filtering on the physics controller before computing the ray intersection to save CPU.
 * It is only used to its full extend by the Ccd physics environment (Bullet).
 */
class PHY_IRayCastFilterCallback
{
public:
	PHY_IPhysicsController *m_ignoreController;
	bool m_faceNormal;
	bool m_faceUV;

	virtual ~PHY_IRayCastFilterCallback()
	{
	}

	virtual bool needBroadphaseRayCast(PHY_IPhysicsController *controller)
	{
		return true;
	}

	virtual void reportHit(PHY_RayCastResult *result) = 0;

	PHY_IRayCastFilterCallback(PHY_IPhysicsController *ignoreController, bool faceNormal = false, bool faceUV = false)
		: m_ignoreController(ignoreController),
		m_faceNormal(faceNormal),
		m_faceUV(faceUV)
	{
	}
};

/**
 * Physics Environment takes care of stepping the simulation and is a container for physics entities
 * (rigidbodies,constraints, materials etc.)
 * A derived class may be able to 'construct' entities by loading and/or converting
 */
class PHY_IPhysicsEnvironment
{
public:
	virtual ~PHY_IPhysicsEnvironment()
	{
	}
	/// Perform an integration step of duration 'timeStep'.
	virtual void ProceedDeltaTime(float timeStep, float interval) = 0;
	//virtual void ProceedDeltaTimeCar(float timeStep, float interval) = 0;
	/// draw debug lines (make sure to call this during the render phase, otherwise lines are not drawn properly)
	virtual void DebugDrawWorld()
	{
	}
	virtual void SetFixedTimeStep(bool useFixedTimeStep, float fixedTimeStep) = 0;
	// returns 0.f if no fixed timestep is used
	virtual float GetFixedTimeStep() = 0;

	/// getDebugMode return the actual debug visualization state
	virtual int GetDebugMode() const = 0;
	/// setDebugMode is used to support several ways of debug lines, contact point visualization
	virtual void SetDebugMode(int debugMode)
	{
	}
	/// setNumIterations set the number of iterations for iterative solvers
	virtual void SetNumIterations(int numIterations)
	{
	}
	/// setErp // splitImpulse penetration depth before for use, used if ccdMode is on.
	virtual void SetErp(float erp)
	{
	}
	/// setErp2 // splitImpulse penetration depth before for use, used if ccdMode is off.
	virtual void SetErp2(float erp2)
	{
	}
	/// setGlobalCfm constraint force mixing for contacts and non-contacts
	virtual void SetGlobalCfm(float globalCfm)
	{
	}
	/// setSplitImpulse // by default, Bullet solves positional constraints and velocity constraints coupled together.
	virtual void SetSplitImpulse(bool splitImpulse)
	{
	}
	/// setSplitImpulsePenetrationThreshold // Penetration Threshold before splitImpulse is used.
	virtual void SetSplitImpulsePenetrationThreshold(float splitImpulsePenetrationThreshold)
	{
	}
	/// setSplitImpulseTurnErp // constraints fix for splitImpulse.
	virtual void SetSplitImpulseTurnErp(float splitImpulseTurnErp)
	{
	}
	/// setLinearSlop // Defines the penetration depth for object collisions.
	virtual void SetLinearSlop(float slop)
	{
	}
	/// setWarmstartingFactor Defines how much of the previous impulse is used for the next calculation step. A value of zero will turn off warmstarting a value of 1 will use the original value.
	virtual void SetWarmstartingFactor(float factor)
	{
	}
	/// setMaxGyroscopicForce it is only used for 'explicit' version of gyroscopic force.
	virtual void SetMaxGyroscopicForce(float maxGyroscopicForce)
	{
	}
	/// setNumTimeSubSteps set the number of divisions of the timestep. Tradeoff quality versus performance.
	virtual void SetNumTimeSubSteps(int numTimeSubSteps)
	{
	}
	virtual int GetNumTimeSubSteps()
	{
		return 0;
	}
	/// setDeactivationTime sets the minimum time that an objects has to stay within the velocity tresholds until it gets fully deactivated
	virtual void SetDeactivationTime(float dTime)
	{
	}
	/// setDeactivationLinearTreshold sets the linear velocity treshold, see setDeactivationTime
	virtual void SetDeactivationLinearTreshold(float linTresh)
	{
	}
	/// setDeactivationAngularTreshold sets the angular velocity treshold, see setDeactivationTime
	virtual void SetDeactivationAngularTreshold(float angTresh)
	{
	}
	/// setContactBreakingTreshold sets tresholds to do with contact point management
	virtual void SetContactBreakingTreshold(float contactBreakingTreshold)
	{
	}
	/// continuous collision detection mode, very experimental for Bullet
	virtual void SetCcdMode(int ccdMode)
	{
	}
	/// successive overrelaxation constant, in case PSOR is used, values in between 1 and 2 guarantee converging behavior
	virtual void SetSolverSorConstant(float sor)
	{
	}
	/// setSolverType, internal setting, chooses solvertype, PSOR, Dantzig, impulse based, penalty based
	virtual void SetSolverType(PHY_SolverType solverType)
	{
	}
	/// setTau sets the spring constant of a penalty based solver
	virtual void SetSolverTau(float tau)
	{
	}
	/// setDamping sets the damper constant of a penalty based solver
	virtual void SetSolverDamping(float damping)
	{
	}
	/// linear air damping for rigidbodies
	virtual void SetLinearAirDamping(float damping)
	{
	}
	/// penetrationdepth setting
	virtual void SetUseEpa(bool epa)
	{
	}

	virtual void SetGravity(float x, float y, float z) = 0;
	virtual mt::vec3 GetGravity() const = 0;

	virtual PHY_IConstraint *CreateConstraint(class PHY_IPhysicsController *ctrl, class PHY_IPhysicsController *ctrl2, PHY_ConstraintType type,
								 float pivotX, float pivotY, float pivotZ,
								 float axis0X, float axis0Y, float axis0Z,
								 float axis1X = 0, float axis1Y = 0, float axis1Z = 0,
								 float axis2X = 0, float axis2Y = 0, float axis2Z = 0, int flag = 0) = 0;
	virtual PHY_IVehicle *CreateVehicle(PHY_IPhysicsController *ctrl) = 0;
	virtual void RemoveConstraintById(int constraintid, bool free) = 0;
	virtual float GetAppliedImpulse(int constraintid)
	{
		return 0.0f;
	}

	// complex constraint for vehicles
	virtual PHY_IVehicle *GetVehicleConstraint(int constraintId) = 0;
	// Character physics wrapper
	virtual PHY_ICharacter *GetCharacterController(class KX_GameObject *ob) = 0;

	virtual PHY_IPhysicsController *RayTest(PHY_IRayCastFilterCallback &filterCallback, float fromX, float fromY, float fromZ, float toX, float toY, float toZ) = 0;

	// culling based on physical broad phase
	// the plane number must be set as follow: near, far, left, right, top, botton
	// the near plane must be the first one and must always be present, it is used to get the direction of the view
	virtual bool CullingTest(PHY_CullingCallback callback, void *userData, const std::array<mt::vec4, 6>& planes,
							 int occlusionRes, const int *viewport, const mt::mat4& matrix) = 0;

	// Methods for gamelogic collision/physics callbacks
	virtual void AddSensor(PHY_IPhysicsController *ctrl) = 0;
	virtual void RemoveSensor(PHY_IPhysicsController *ctrl) = 0;
	virtual void AddCollisionCallback(int response_class, PHY_ResponseCallback callback, void *user) = 0;
	virtual bool RequestCollisionCallback(PHY_IPhysicsController *ctrl) = 0;
	virtual bool RemoveCollisionCallback(PHY_IPhysicsController *ctrl) = 0;
	virtual PHY_CollisionTestResult CheckCollision(PHY_IPhysicsController *ctrl0, PHY_IPhysicsController *ctrl1) = 0;
	//These two methods are *solely* used to create controllers for sensor! Don't use for anything else
	virtual PHY_IPhysicsController *CreateSphereController(float radius, const mt::vec3& position) = 0;
	virtual PHY_IPhysicsController *CreateConeController(float coneradius, float coneheight) = 0;

	virtual void ExportFile(const std::string& filename)
	{
	};

	virtual void MergeEnvironment(PHY_IPhysicsEnvironment *other_env) = 0;

	virtual void ConvertObject(BL_SceneConverter& converter,
							   KX_GameObject *gameobj,
	                           RAS_Mesh *meshobj,
	                           KX_Scene *kxscene,
	                           PHY_IMotionState *motionstate,
	                           int activeLayerBitInfo,
	                           bool isCompoundChild,
	                           bool hasCompoundChildren) = 0;

	/* Set the rigid body joints constraints values for converted objects and replicated group instances. */
	virtual void SetupObjectConstraints(KX_GameObject *obj_src, KX_GameObject *obj_dest,
	                                    bRigidBodyJointConstraint *dat)
	{
	}
};

#endif  /* __PHY_IPHYSICSENVIRONMENT_H__ */
