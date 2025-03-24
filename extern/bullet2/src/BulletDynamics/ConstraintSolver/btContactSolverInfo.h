/*
Bullet Continuous Collision Detection and Physics Library
Copyright (c) 2003-2006 Erwin Coumans  http://continuousphysics.com/Bullet/

This software is provided 'as-is', without any express or implied warranty.
In no event will the authors be held liable for any damages arising from the use of this software.
Permission is granted to anyone to use this software for any purpose, 
including commercial applications, and to alter it and redistribute it freely, 
subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
*/

#ifndef BT_CONTACT_SOLVER_INFO
#define BT_CONTACT_SOLVER_INFO

#include "LinearMath/btScalar.h"

enum	btSolverMode
{
	SOLVER_RANDMIZE_ORDER = 1,
	SOLVER_FRICTION_SEPARATE = 2,
	SOLVER_USE_WARMSTARTING = 4,
	SOLVER_USE_2_FRICTION_DIRECTIONS = 16,
	SOLVER_ENABLE_FRICTION_DIRECTION_CACHING = 32,
	SOLVER_DISABLE_VELOCITY_DEPENDENT_FRICTION_DIRECTION = 64,
	SOLVER_CACHE_FRIENDLY = 128,
	SOLVER_SIMD = 256,
	SOLVER_INTERLEAVE_CONTACT_AND_FRICTION_CONSTRAINTS = 512,
	SOLVER_ALLOW_ZERO_LENGTH_FRICTION_DIRECTIONS = 1024
};

struct btContactSolverInfoData
{
	

	btScalar	m_tau;
	btScalar	m_damping;//global non-contact constraint damping, can be locally overridden by constraints during 'getInfo2'.
	btScalar	m_friction;
	btScalar	m_timeStep;
	btScalar	m_restitution;
	int		m_numIterations;
	btScalar	m_maxErrorReduction;
	btScalar	m_sor;//successive over-relaxation term
	btScalar	m_erp;// error reduction for non-contact constraints
	btScalar	m_erp2;// error reduction for contact constraints
	btScalar	m_globalCfm;// constraint force mixing for contacts and non-contacts
	int			m_splitImpulse;
	btScalar	m_splitImpulsePenetrationThreshold;
	btScalar	m_splitImpulseTurnErp;
	btScalar	m_linearSlop;
	btScalar	m_warmstartingFactor;

	int			m_solverMode;
	int	m_restingContactRestitutionThreshold;
	int			m_minimumSolverBatchSize;
	btScalar	m_maxGyroscopicForce;
	btScalar	m_singleAxisRollingFrictionThreshold;


};

struct btContactSolverInfo : public btContactSolverInfoData
{

	

	inline btContactSolverInfo()
	{
		m_tau = btScalar(0.6);
		m_damping = btScalar(1.0);
		m_friction = btScalar(0.3);
		m_timeStep = btScalar(1.f/60.f);
		m_restitution = btScalar(0.);
		m_maxErrorReduction = btScalar(20.);// not used I think.
		m_numIterations = 10;
		m_erp = btScalar(0.2);// used if not using Split Impulse, ERP/Baumgarte means that instead of computing the relative velocity at your contact to become zero you add a little bias proportional to the penetration depth.
		m_erp2 = btScalar(0.8);// used in Split Impulse,  "Solver Constraint Error Reduction Parameter" (ERP), which controls how aggressively the physics solver attempts to correct constraint violations during each simulation step; essentially, it determines how quickly the system tries to eliminate any errors that occur when objects are not adhering to the defined constraints, like joint limits or contact points
		m_globalCfm = btScalar(0.);// Constraint Force Mixing, [0;infty], A nonzero (positive) value of CFM allows the original constraint equation to be violated by an amount proportional to CFM times the restoring force \lambda that is needed to enforce the constraint.
		m_sor = btScalar(1.);// This parameter typically ranges from 0 to 1, where 0 means the solver will barely try to correct errors (leading to potential drifting) and 1 means it will aggressively attempt to fix all errors immediately (which could cause instability in certain scenarios).
		m_splitImpulse = true;  // by default, Bullet solves positional constraints and velocity constraints coupled together. This works well in many cases, but the error reduction of position coupled to velocity introduces extra energy (noticeable as 'bounce').
								// Instead of coupled positional and velocity constraint solving, the two can be solved separately using the 'split impulse' option. This means that recovering from deep penetrations doesn't add any velocity when this option is checked.
								// Although this removes most of the extra energy/bounce, it degenerates quality a little bit, in particular for stable stacking. Hence, the setting should be disabled.
								// Also note that the split impulse option is only enabled for contact constraints, and none of the other joints (hinge/ball socket and so on), for quality reasons
		m_splitImpulsePenetrationThreshold = -.02f;
		m_splitImpulseTurnErp = 0.1f;// constraint solving error reduction
		m_linearSlop = btScalar(0.0);// Defines the penetration depth for object collisions.
									// this value is measured in world units. A value of 1 equals to 1 unit penetration depth (overlapping).
									// It is recommended to use the default value of 0.
		
		m_warmstartingFactor=btScalar(0.85);// was 0.85 Bullet uses an iterative algorithm where each iteration is based on the solution of previous iteration.
											// If no warmstarting is used, the initial solution for Bullet is set to zero each frame.
											// When using warmstarting, the first iteration uses the last solution of the previous frame.
											// This improves convergence towards a better solution and hence stacking stability. 
											// Defines how much of the previous impulse is used for the next calculation step. A value of zero will turn off warmstarting a value of 1 will use the original value.

		//m_solverMode =  SOLVER_USE_WARMSTARTING |  SOLVER_SIMD | SOLVER_DISABLE_VELOCITY_DEPENDENT_FRICTION_DIRECTION|SOLVER_USE_2_FRICTION_DIRECTIONS|SOLVER_ENABLE_FRICTION_DIRECTION_CACHING;// | SOLVER_RANDMIZE_ORDER;
		m_solverMode = SOLVER_USE_WARMSTARTING | SOLVER_SIMD;// | SOLVER_RANDMIZE_ORDER;
		m_restingContactRestitutionThreshold = 2;//unused as of 2.81
		m_minimumSolverBatchSize = 512; //try to combine islands until the amount of constraints reaches this limit
		m_maxGyroscopicForce = 100.f; ///it is only used for 'explicit' version of gyroscopic force
		m_singleAxisRollingFrictionThreshold = 1e30f;///if the velocity is above this threshold, it will use a single constraint row (axis), otherwise 3 rows.
	}
};

///do not change those serialization structures, it requires an updated sBulletDNAstr/sBulletDNAstr64
struct btContactSolverInfoDoubleData
{
	double		m_tau;
	double		m_damping;//global non-contact constraint damping, can be locally overridden by constraints during 'getInfo2'.
	double		m_friction;
	double		m_timeStep;
	double		m_restitution;
	double		m_maxErrorReduction;
	double		m_sor;
	double		m_erp;//used as Baumgarte factor
	double		m_erp2;//used in Split Impulse
	double		m_globalCfm;//constraint force mixing
	double		m_splitImpulsePenetrationThreshold;
	double		m_splitImpulseTurnErp;
	double		m_linearSlop;
	double		m_warmstartingFactor;
	double		m_maxGyroscopicForce;///it is only used for 'explicit' version of gyroscopic force
	double		m_singleAxisRollingFrictionThreshold;

	int			m_numIterations;
	int			m_solverMode;
	int			m_restingContactRestitutionThreshold;
	int			m_minimumSolverBatchSize;
	int			m_splitImpulse;
	char		m_padding[4];

};
///do not change those serialization structures, it requires an updated sBulletDNAstr/sBulletDNAstr64
struct btContactSolverInfoFloatData
{
	float		m_tau;
	float		m_damping;//global non-contact constraint damping, can be locally overridden by constraints during 'getInfo2'.
	float		m_friction;
	float		m_timeStep;

	float		m_restitution;
	float		m_maxErrorReduction;
	float		m_sor;
	float		m_erp;//used as Baumgarte factor

	float		m_erp2;//used in Split Impulse
	float		m_globalCfm;//constraint force mixing
	float		m_splitImpulsePenetrationThreshold;
	float		m_splitImpulseTurnErp;

	float		m_linearSlop;// Defines the penetration depth for object collisions. this value is measured in world units. A value of 1 equals to 1 unit penetration depth (overlapping). It is recommended to use the default value of 0. 
	float		m_warmstartingFactor;
	float		m_maxGyroscopicForce;
	float		m_singleAxisRollingFrictionThreshold;

	int			m_numIterations;
	int			m_solverMode;
	int			m_restingContactRestitutionThreshold;
	int			m_minimumSolverBatchSize;

	int			m_splitImpulse;
	char		m_padding[4];
};



#endif //BT_CONTACT_SOLVER_INFO
