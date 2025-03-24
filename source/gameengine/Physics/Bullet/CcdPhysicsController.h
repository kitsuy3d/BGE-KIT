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

/** \file CcdPhysicsController.h
 *  \ingroup physbullet
 */


#ifndef __CCDPHYSICSCONTROLLER_H__
#define __CCDPHYSICSCONTROLLER_H__

#include "CM_RefCount.h"

#include <vector>
#include <map>

#include "PHY_IPhysicsController.h"

#include "CcdMathUtils.h"

///	PHY_IPhysicsController is the abstract simplified Interface to a physical object.
///	It contains the IMotionState and IDeformableMesh Interfaces.
#include "btBulletDynamicsCommon.h"
#include "BulletDynamics/Character/btKinematicCharacterController.h"
#include "LinearMath/btTransform.h"

#include "PHY_IMotionState.h"
#include "PHY_ICharacter.h"

extern float gDeactivationTime; // Time in seconds before a physics object is deactivated (put to sleep) when it's not moving much.
extern float gLinearSleepingTreshold; // Linear velocity threshold below which a physics object is considered to be sleeping.
extern float gAngularSleepingTreshold; // Angular velocity threshold below which a physics object is considered to be sleeping.
extern bool gDisableDeactivation; // Flag to disable the deactivation (sleeping) of physics objects, preventing them from entering a low-power state.
class CcdPhysicsEnvironment; // Manages the physics world and its associated objects, providing an interface for physics simulation.
class CcdPhysicsController; // Encapsulates a physics object (rigid body, soft body, etc.) and its behavior, providing control and access to its properties.
class btMotionState; // Handles the motion and transformation of physics objects in Bullet Physics, updating their positions and orientations.
class RAS_Mesh; // Represents a mesh data structure, often used for collision shape creation or visual representation.
class RAS_Deformer; // Handles mesh deformation, potentially used in creating deformable collision shapes or animating meshes.
class btCollisionShape; // Defines the geometric shape of a physics object, used for collision detection and response.

#define CCD_BSB_SHAPE_MATCHING     2  // Enables shape matching constraints for soft bodies, used to maintain a desired shape.
#define CCD_BSB_BENDING_CONSTRAINTS 8  // Enables bending constraints for soft bodies, which resist deformation along edges.
#define CCD_BSB_AERO_VPOINT        16 /* aero model, Vertex normals are oriented toward velocity */ // Enables an aerodynamic model where vertex normals are aligned with the velocity direction.
#define CCD_BSB_AERO_VTWOSIDE      32 /* aero model, Vertex normals are flipped to match velocity */ // Enables an aerodynamic model where vertex normals are flipped to match the velocity direction on both sides.

/* BulletSoftBody.collisionflags */
#define CCD_BSB_COL_SDF_RS         2  /* SDF based rigid vs soft */ // Enables collision detection between rigid and soft bodies using Signed Distance Fields (SDF).
#define CCD_BSB_COL_CL_RS          4  /* Cluster based rigid vs soft */ // Enables collision detection between rigid and soft bodies using collision clusters.
#define CCD_BSB_COL_CL_SS          8  /* Cluster based soft vs soft */ // Enables collision detection between soft bodies using collision clusters.
#define CCD_BSB_COL_VF_SS          16 /* Vertex/Face based soft vs soft */ // Enables collision detection between soft bodies using vertex and face information.

// Shape contructor
// It contains all the information needed to create a simple bullet shape at runtime
class CcdShapeConstructionInfo : public CM_RefCount<CcdShapeConstructionInfo>, public mt::SimdClassAllocator
{
public:

    struct UVco // Structure to hold UV coordinates for texture mapping.
    {
        float uv[2]; // Array of two floats representing the U and V texture coordinates.
    };
    static CcdShapeConstructionInfo *FindMesh(RAS_Mesh *mesh, RAS_Deformer *deformer, PHY_ShapeType shapeType); // Searches the static mesh shape map for a matching shape construction info based on the provided mesh, deformer, and shape type.

    CcdShapeConstructionInfo() 
        :m_shapeType(PHY_SHAPE_NONE), // Initializes the shape type to 'none', indicating no specific shape.
        m_radius(1.0f), // Initializes the radius to 1.0f, used for shapes like spheres or cylinders.
        m_height(1.0f), // Initializes the height to 1.0f, used for shapes like cylinders or capsules.
        m_halfExtend(0.0f, 0.0f, 0.0f), // Initializes the half-extents to (0,0,0), used for box shapes.
        m_childScale(1.0f, 1.0f, 1.0f), // Initializes the child scale to (1,1,1), used for compound shapes.
        m_userData(nullptr), // Initializes the user data pointer to null.
        m_mesh(nullptr), // Initializes the mesh pointer to null.
        m_triangleIndexVertexArray(nullptr), // Initializes the triangle index vertex array pointer to null.
        m_forceReInstance(false), // Initializes the force re-instance flag to false.
        m_weldingThreshold1(0.0f), // Initializes the vertex welding threshold to 0.0f.
        m_shapeProxy(nullptr) // Initializes the shape proxy pointer to null.
    {
        m_childTrans.setIdentity(); // Sets the child transform to the identity matrix.
    }

    ~CcdShapeConstructionInfo(); // Destructor to release any allocated resources for the shape construction info.

    void AddShape(CcdShapeConstructionInfo *shapeInfo); // Adds a new child shape construction info to the compound shape.

    btStridingMeshInterface *GetMeshInterface()
    {
        return m_triangleIndexVertexArray; // Returns the Bullet striding mesh interface, used for mesh collision shapes.
    }

    CcdShapeConstructionInfo *GetChildShape(int i)
    {
        if (i < 0 || i >= (int)m_shapeArray.size())
            return nullptr; // Returns nullptr if the index is out of bounds.

        return m_shapeArray.at(i); // Returns the child shape construction info at the specified index.
    }
    int FindChildShape(CcdShapeConstructionInfo *shapeInfo, void *userData)
    {
        if (shapeInfo == nullptr)
            return -1; // If the provided shapeInfo pointer is null, return -1 indicating no shape was found.

        for (int i = 0; i < (int)m_shapeArray.size(); i++) {
            CcdShapeConstructionInfo *childInfo = m_shapeArray.at(i); // Get the shape information for the child shape at index 'i'.

            // Check if the current child shape matches the provided shapeInfo based on userData and shape type.
            if ((userData == nullptr || userData == childInfo->m_userData) && 
                (childInfo == shapeInfo || 
                 (childInfo->m_shapeType == PHY_SHAPE_PROXY && childInfo->m_shapeProxy == shapeInfo)))
            {
                return i; // If a match is found, return the index 'i' of the child shape.
            }
        }
        return -1; // If no matching child shape is found after iterating through the array, return -1.
    }

    bool RemoveChildShape(int i)
    {
        if (i < 0 || i >= (int)m_shapeArray.size()) // Checks if the index 'i' is within the valid range of the 'm_shapeArray' vector.
            return false; // Returns false if the index is out of bounds.
        m_shapeArray.at(i)->Release(); // Releases the shape construction info at the given index.
        if (i < (int)m_shapeArray.size() - 1)
            m_shapeArray[i] = m_shapeArray.back(); // Moves the last element to the removed element's position.
        m_shapeArray.pop_back(); // Removes the last element.
        return true; // Returns true if the child shape was removed successfully.
    }

    bool UpdateMesh(class KX_GameObject *gameobj, class RAS_Mesh *mesh); // Updates the mesh data used for collision shape generation.

    CcdShapeConstructionInfo *GetReplica(); // Creates and returns a replica of the shape construction info.

    void ProcessReplica(); // Processes the replica to update internal data.

    bool SetProxy(CcdShapeConstructionInfo *shapeInfo); // Sets a proxy shape construction info.
    CcdShapeConstructionInfo *GetProxy(void)
    {
        return m_shapeProxy; // Returns the proxy shape construction info.
    }

    RAS_Mesh *GetMesh() const; // Returns the mesh associated with the shape construction info.

    btCollisionShape *CreateBulletShape(btScalar margin, bool useGimpact = false, bool useBvh = true); // Creates a Bullet collision shape from the shape construction info.

    // member variables
    PHY_ShapeType m_shapeType; // Type of the collision shape.
    btScalar m_radius; // Radius of the collision shape (e.g., for spheres or cylinders).
    btScalar m_height; // Height of the collision shape (e.g., for cylinders or capsules).
    btVector3 m_halfExtend; // Half-extents of the collision shape (e.g., for boxes).
    btTransform m_childTrans; // Transform of a child shape, used for compound shapes.
    btVector3 m_childScale; // Scale of a child shape, used for compound shapes.
    void *m_userData; // User-defined data associated with the shape.

    /** Vertex mapping from original vertex index to shape vertex index. */
    std::vector<int> m_vertexRemap; // Maps original mesh vertex indices to indices in the collision shape's vertex array.

    /** Contains both vertex array for polytope shape and triangle array for concave mesh shape.
     * Each vertex is 3 consecutive values. In this case a triangle is made of 3 consecutive points
     */
    btAlignedObjectArray<btScalar> m_vertexArray; // Stores vertex data for collision shapes, used for both convex and concave shapes.

    /** Contains the array of polygon index in the original mesh that correspond to shape triangles.
     * only set for concave mesh shape.
     */
    std::vector<int> m_polygonIndexArray; // Stores polygon indices from the original mesh, used for concave mesh shapes.

    /// Contains an array of triplets of face indices quads turn into 2 tris
    std::vector<int> m_triFaceArray; // Stores face indices for triangles generated from quads.

    /// Contains an array of pair of UV coordinate for each vertex of faces quads turn into 2 tris
    std::vector<UVco> m_triFaceUVcoArray; // Stores UV coordinates for each vertex of triangles generated from quads.

    void setVertexWeldingThreshold1(float threshold)
    {
        m_weldingThreshold1 = threshold * threshold; // Sets the vertex welding threshold, squaring the input for internal use.
    }
protected:
    using MeshShapeKey = std::tuple<RAS_Mesh *, RAS_Deformer *, PHY_ShapeType>; // Defines a key for mesh shapes, consisting of mesh, deformer, and shape type.
    using MeshShapeMap = std::map<MeshShapeKey, CcdShapeConstructionInfo *>; // Defines a map to store mesh shape construction information, keyed by MeshShapeKey.

    static MeshShapeMap m_meshShapeMap; // Static map to store mesh shape construction information for reuse.
    /// Converted original mesh.
    RAS_Mesh *m_mesh; // Pointer to the converted original mesh.
    /// The list of vertexes and indexes for the triangle mesh, shared between Bullet shape.
    btTriangleIndexVertexArray *m_triangleIndexVertexArray; // Pointer to the Bullet triangle index vertex array, used for mesh collision shapes.
    /// for compound shapes
    std::vector<CcdShapeConstructionInfo *> m_shapeArray; // Vector to store shape construction information for compound shapes.
    ///use gimpact for concave dynamic/moving collision detection
    bool m_forceReInstance; // Flag to force re-instantiation of the collision shape, often used with GImpact.
    ///welding closeby vertices together can improve softbody stability etc.
    float m_weldingThreshold1; // Threshold for welding close vertices together to improve soft body stability.
    /// only used for PHY_SHAPE_PROXY, pointer to actual shape info
    CcdShapeConstructionInfo *m_shapeProxy; // Pointer to the actual shape information, used when the shape is a proxy.
};

struct CcdConstructionInfo {

	/** CollisionFilterGroups provides some optional usage of basic collision filtering
	 * this is done during broadphase, so very early in the pipeline
	 * more advanced collision filtering should be done in btCollisionDispatcher::NeedsCollision
	 */
    enum CollisionFilterGroups
    {
        DynamicFilter = 1, // Filter group for dynamic objects.
        StaticFilter = 2, // Filter group for static objects.
        KinematicFilter = 4, // Filter group for kinematic objects.
        DebrisFilter = 8, // Filter group for debris objects.
        SensorFilter = 16, // Filter group for sensor objects.
        CharacterFilter = 32, // Filter group for character objects.
        AllFilter = DynamicFilter | StaticFilter | KinematicFilter | DebrisFilter | SensorFilter | CharacterFilter, // Filter group that includes all other groups.
    };

    CcdConstructionInfo()
        :m_localInertiaTensor(1.0f, 1.0f, 1.0f), // Initial local inertia tensor.
        m_gravity(0.0f, 0.0f, 0.0f), // Initial gravity vector.
        m_scaling(1.0f, 1.0f, 1.0f), // Initial scaling.
        m_linearFactor(0.0f, 0.0f, 0.0f), // Initial linear factor.
        m_angularFactor(0.0f, 0.0f, 0.0f), // Initial angular factor.
        m_mass(0.0f), // Initial mass.
        m_clamp_vel_min(-1.0f), // Initial minimum linear velocity clamp.
        m_clamp_vel_max(-1.0f), // Initial maximum linear velocity clamp.
        m_clamp_angvel_min(0.0f), // Initial minimum angular velocity clamp.
        m_clamp_angvel_max(0.0f), // Initial maximum angular velocity clamp.
        m_restitution(0.1f), // Initial restitution.
        m_friction(0.5f), // Initial friction.
        m_rollingFriction(0.0f), // Initial rolling friction.
        m_linearDamping(0.1f), // Initial linear damping.
        m_angularDamping(0.1f), // Initial angular damping.
        m_margin(0.0f), // Initial collision margin.
        m_gamesoftFlag(0), // Initial game specific soft body flag.
        m_softBendingDistance(2), // Initial soft body bending distance.
        m_soft_linStiff(1.0f), // Initial soft body linear stiffness.
        m_soft_angStiff(1.0f), // Initial soft body angular stiffness.
        m_soft_volume(1.0f), // Initial soft body volume preservation.
        m_soft_viterations(0), // Initial soft body velocity solver iterations.
        m_soft_piterations(1), // Initial soft body position solver iterations.
        m_soft_diterations(0), // Initial soft body drift solver iterations.
        m_soft_citerations(4), // Initial soft body cluster solver iterations.
        m_soft_kSRHR_CL(0.1f), // Initial soft vs rigid hardness (cluster).
        m_soft_kSKHR_CL(1.0f), // Initial soft vs kinetic hardness (cluster).
        m_soft_kSSHR_CL(0.5f), // Initial soft vs soft hardness (cluster).
        m_soft_kSR_SPLT_CL(0.5f), // Initial soft vs rigid impulse split (cluster).
        m_soft_kSK_SPLT_CL(0.5f), // Initial soft vs kinetic impulse split (cluster).
        m_soft_kSS_SPLT_CL(0.5f), // Initial soft vs soft impulse split (cluster).
        m_soft_kVCF(1.0f), // Initial soft body velocities correction factor.
        m_soft_kDP(0.0f), // Initial soft body damping.
        m_soft_kDG(0.0f), // Initial soft body drag.
        m_soft_kLF(0.0f), // Initial soft body lift.
        m_soft_kPR(0.0f), // Initial soft body pressure.
        m_soft_kVC(0.0f), // Initial soft body volume conservation.
        m_soft_kDF(0.2f), // Initial soft body dynamic friction.
        m_soft_kMT(0), // Initial soft body pose matching.
        m_soft_kCHR(1.0f), // Initial soft body rigid contact hardness.
        m_soft_kKHR(0.1f), // Initial soft body kinetic contact hardness.
        m_soft_kSHR(1.0f), // Initial soft body soft contact hardness.
        m_soft_kAHR(0.7f), // Initial soft body anchor hardness.
        m_collisionFlags(0), // Initial collision flags.
        m_bDyna(false), // Initial dynamic flag.
        m_bRigid(false), // Initial rigid body flag.
        m_bSoft(false), // Initial soft body flag.
        m_bSensor(false), // Initial sensor flag.
        m_bCharacter(false), // Initial character flag.
        m_bGimpact(false), // Initial gimpact collision flag.
        m_collisionFilterGroup(DynamicFilter), // Initial collision filter group.
        m_collisionFilterMask(AllFilter), // Initial collision filter mask.
        m_collisionGroup(0xFFFF), // Initial collision group.
        m_collisionMask(0xFFFF), // Initial collision mask.
        m_collisionShape(nullptr), // Initial collision shape pointer.
        m_MotionState(nullptr), // Initial motion state pointer.
        m_shapeInfo(nullptr), // Initial shape info pointer.
        m_physicsEnv(nullptr), // Initial physics environment pointer.
        m_inertiaFactor(1.0f), // Initial inertia factor.
        m_do_anisotropic(false), // Initial anisotropic friction flag.
        m_anisotropicFriction(1.0f, 1.0f, 1.0f), // Initial anisotropic friction.
        m_do_fh(false), // Initial linear fh spring flag.
        m_do_rot_fh(false), // Initial angular fh spring flag.
        m_fh_spring(0.0f), // Initial fh spring constant.
        m_fh_damping(0.0f), // Initial fh damping.
        m_fh_distance(1.0f), // Initial fh distance.
        m_fh_normal(false), // Initial fh normal flag.
        m_ccd_motion_threshold(0.0f), // Initial ccd motion threshold.
        m_ccd_swept_sphere_radius(0.0f) // Initial ccd swept sphere radius.
		// m_contactProcessingThreshold(1e10f)
    {
    }

    btVector3 m_localInertiaTensor; // Local inertia tensor of the object.
    btVector3 m_gravity; // Gravity vector applied to the object.
    btVector3 m_scaling; // Scaling of the object.
    btVector3 m_linearFactor; // Linear velocity scaling factor.
    btVector3 m_angularFactor; // Angular velocity scaling factor.
    btScalar m_mass; // Mass of the object.
    btScalar m_clamp_vel_min; // Minimum linear velocity clamp.
    btScalar m_clamp_vel_max; // Maximum linear velocity clamp.
    /// Minimum angular velocity, in radians/sec.
    btScalar m_clamp_angvel_min; // Minimum angular velocity clamp.
    /// Maximum angular velocity, in radians/sec.
    btScalar m_clamp_angvel_max; // Maximum angular velocity clamp.
    btScalar m_restitution; // Restitution coefficient (bounciness).
    btScalar m_friction; // Friction coefficient.
    btScalar m_rollingFriction; // Rolling friction coefficient.
    btScalar m_linearDamping; // Linear damping factor.
    btScalar m_angularDamping; // Angular damping factor.
    btScalar m_margin; // Collision margin.

    float m_stepHeight; // Height of steps the character can climb.
    float m_jumpSpeed; // Initial vertical speed of a jump.
    float m_fallSpeed; // Maximum downward velocity of the character.
    float m_maxSlope; // Maximum slope angle the character can walk on.
    unsigned char m_maxJumps; // Maximum number of jumps the character can perform.
    float m_smoothMovement; // Smoothing factor for character movement.
    btVector3 m_jumpAxis; // Axis along which the character jumps.

    int m_gamesoftFlag; // Game-specific soft body flags.
    unsigned short m_softBendingDistance; // Bending distance for soft body edges.
    /// linear stiffness 0..1
    float m_soft_linStiff; // Linear stiffness of the soft body.
    /// angular stiffness 0..1
    float m_soft_angStiff; // Angular stiffness of the soft body.
    ///  volume preservation 0..1
    float m_soft_volume; // Volume preservation factor for the soft body.

    /// Velocities solver iterations
    int m_soft_viterations; // Number of iterations for the velocity solver in soft body simulation.
    /// Positions solver iterations
    int m_soft_piterations; // Number of iterations for the position solver in soft body simulation.
    /// Drift solver iterations
    int m_soft_diterations; // Number of iterations for the drift solver in soft body simulation.
    /// Cluster solver iterations
    int m_soft_citerations; // Number of iterations for the cluster solver in soft body simulation.

    /// Soft vs rigid hardness [0,1] (cluster only)
    float m_soft_kSRHR_CL; // Soft vs rigid hardness coefficient for cluster collisions.
    /// Soft vs kinetic hardness [0,1] (cluster only)
    float m_soft_kSKHR_CL; // Soft vs kinetic hardness coefficient for cluster collisions.
    /// Soft vs soft hardness [0,1] (cluster only)
    float m_soft_kSSHR_CL; // Soft vs soft hardness coefficient for cluster collisions.
    /// Soft vs rigid impulse split [0,1] (cluster only)
    float m_soft_kSR_SPLT_CL; // Soft vs rigid impulse split coefficient for cluster collisions.
    /// Soft vs rigid impulse split [0,1] (cluster only)
    float m_soft_kSK_SPLT_CL; // Soft vs kinetic impulse split coefficient for cluster collisions.
    /// Soft vs rigid impulse split [0,1] (cluster only)
    float m_soft_kSS_SPLT_CL; // Soft vs soft impulse split coefficient for cluster collisions.
    /// Velocities correction factor (Baumgarte)
    float m_soft_kVCF; // Velocities correction factor for Baumgarte stabilization.
    /// Damping coefficient [0,1]
    float m_soft_kDP; // Damping coefficient for soft body dynamics.

    /// Drag coefficient [0,+inf]
    float m_soft_kDG; // Drag coefficient for soft body aerodynamics.
    /// Lift coefficient [0,+inf]
    float m_soft_kLF; // Lift coefficient for soft body aerodynamics.
    /// Pressure coefficient [-inf,+inf]
    float m_soft_kPR; // Pressure coefficient for soft body aerodynamics.
    /// Volume conversation coefficient [0,+inf]
    float m_soft_kVC; // Volume conservation coefficient for soft body dynamics.

    /// Dynamic friction coefficient [0,1]
    float m_soft_kDF; // Dynamic friction coefficient for soft body contacts.
    /// Pose matching coefficient [0,1]
    float m_soft_kMT; // Pose matching coefficient for soft body shape matching.
    /// Rigid contacts hardness [0,1]
    float m_soft_kCHR; // Rigid contacts hardness coefficient for soft body interactions.
    /// Kinetic contacts hardness [0,1]
    float m_soft_kKHR; // Kinetic contacts hardness coefficient for soft body interactions.

    /// Soft contacts hardness [0,1]
    float m_soft_kSHR; // Soft contacts hardness coefficient for soft body interactions.
    /// Anchors hardness [0,1]
    float m_soft_kAHR; // Anchors hardness coefficient for soft body constraints.
    /// Vertex/Face or Signed Distance Field(SDF) or Clusters, Soft versus Soft or Rigid
    int m_soft_collisionflags; // Collision flags for soft body collision handling.
    /// number of iterations to refine collision clusters
    int m_soft_numclusteriterations; // Number of iterations for refining collision clusters in soft bodies.

 /// Ccd
    btScalar m_ccd_motion_threshold; // CCD motion threshold for continuous collision detection.
    btScalar m_ccd_swept_sphere_radius; // CCD swept sphere radius for continuous collision detection.

    int m_collisionFlags; // Collision flags for object behavior.
    bool m_bDyna; // Flag indicating if the object is dynamic.
    bool m_bRigid; // Flag indicating if the object is a rigid body.
    bool m_bSoft; // Flag indicating if the object is a soft body.
    bool m_bSensor; // Flag indicating if the object is a sensor (no physical interaction).
    bool m_bCharacter; // Flag indicating if the object is a character controller.
    bool m_bGimpact; // Flag indicating if GImpact collision detection is used for mesh bodies.

	/** optional use of collision group/mask:
	 * only collision with object goups that match the collision mask.
	 * this is very basic early out. advanced collision filtering should be
	 * done in the btCollisionDispatcher::NeedsCollision and NeedsResponse
	 * both values default to 1
	 */
    short int m_collisionFilterGroup; // Collision filter group for collision filtering.
    short int m_collisionFilterMask; // Collision filter mask for collision filtering.

    unsigned short m_collisionGroup; // Collision group for collision detection.
    unsigned short m_collisionMask; // Collision mask for collision detection.

	/** these pointers are used as argument passing for the CcdPhysicsController constructor
	 * and not anymore after that
	 */
    class btCollisionShape *m_collisionShape; // Pointer to the Bullet collision shape.
    class PHY_IMotionState *m_MotionState; // Pointer to the motion state interface.
    class CcdShapeConstructionInfo *m_shapeInfo; // Pointer to shape construction information.

    /// needed for self-replication
    CcdPhysicsEnvironment *m_physicsEnv; // Pointer to the physics environment, used for replication.
    /// tweak the inertia (hooked up to Blender 'formfactor'
    float m_inertiaFactor; // Factor to tweak the inertia tensor.
    bool m_do_anisotropic; // Flag indicating if anisotropic friction is enabled.
    btVector3 m_anisotropicFriction; // Anisotropic friction coefficients.

    /// Should the object have a linear Fh spring?
    bool m_do_fh; // Flag indicating if linear Fh spring is enabled.
    /// Should the object have an angular Fh spring?
    bool m_do_rot_fh; // Flag indicating if angular Fh spring is enabled.
    /// Spring constant (both linear and angular)
    btScalar m_fh_spring; // Spring constant for linear and angular Fh.
    /// Damping factor (linear and angular) in range [0, 1]
    btScalar m_fh_damping; // Damping factor for linear and angular Fh.
    /// The range above the surface where Fh is active.
    btScalar m_fh_distance; // Distance above the surface where Fh is active.
    /// Should the object slide off slopes?
    bool m_fh_normal; // Flag indicating if the object should slide off slopes.
    /// for fh backwards compatibility
    float m_radius; // Radius, used for backwards compatibility with Fh.

	/** m_contactProcessingThreshold allows to process contact points with positive distance
	 * normally only contacts with negative distance (penetration) are solved
	 * however, rigid body stacking is more stable when positive contacts are still passed into the constraint solver
	 * this might sometimes lead to collisions with 'internal edges' such as a sliding character controller
	 * so disable/set m_contactProcessingThreshold to zero for sliding characters etc.
	 */
	// float		m_contactProcessingThreshold;///< Process contacts with positive distance in range [0..INF]
};

class btRigidBody; // Represents a rigid body in the Bullet physics engine, used for objects with fixed shape and mass.
class btCollisionObject; // Base class for all collision objects in Bullet, including rigid bodies, soft bodies, and static objects.
class btSoftBody; // Represents a deformable body in the Bullet physics engine, used for simulating cloth, ropes, and other flexible objects.
class btPairCachingGhostObject; // A ghost object that keeps track of overlapping pairs, used for collision detection without physical interaction.

class CcdCharacter : public btKinematicCharacterController, public PHY_ICharacter // A custom character controller class that extends Bullet's kinematic character controller and implements a custom character interface (PHY_ICharacter), providing character-specific movement and physics.
{
private:
    CcdPhysicsController *m_ctrl; // Pointer to the associated CcdPhysicsController.
    btMotionState *m_motionState; // Pointer to the Bullet motion state for the character.
    unsigned char m_jumps; // Current number of jumps performed by the character.
    unsigned char m_maxJumps; // Maximum number of jumps allowed for the character.

public:
    CcdCharacter(CcdPhysicsController *ctrl, btMotionState *motionState, btPairCachingGhostObject *ghost, btConvexShape *shape, float stepHeight); // Constructor for CcdCharacter, initializes with controller, motion state, ghost object, shape, and step height.

    virtual void updateAction(btCollisionWorld *collisionWorld, btScalar dt); // Updates the character's movement and physics in the collision world.

    unsigned char getMaxJumps() const; // Gets the maximum number of jumps allowed for the character.

    void setMaxJumps(unsigned char maxJumps); // Sets the maximum number of jumps allowed for the character.

    unsigned char getJumpCount() const; // Gets the current number of jumps the character has performed.

    virtual bool canJump() const; // Checks if the character is allowed to jump.

    virtual void jump(); // Makes the character jump.

    const btVector3& getWalkDirection(); // Gets the current walk direction of the character.
    const btVector3& getJumpDirection(); // Gets the current jump direction of the character.
    const float getSmoothMovement(); // Gets the current smooth movement value of the character.

    void SetVelocity(const btVector3& vel, float time, bool local); // Sets the velocity of the character.

    /// Replace current convex shape.
    void ReplaceShape(btConvexShape *shape); // Replaces the current convex collision shape of the character.

    // PHY_ICharacter interface
    virtual void Jump()
    {
        jump();
    } // Makes the character jump.
    virtual bool OnGround()
    {
        return onGround();
    } // Checks if the character is on the ground.
    virtual float GetGravity()
    {
        return getGravity();
    } // Gets the gravity applied to the character.
    virtual void SetGravity(float gravity)
    {
        setGravity(gravity);
    } // Sets the gravity applied to the character.
    virtual unsigned char GetMaxJumps()
    {
        return getMaxJumps();
    } // Gets the maximum number of jumps allowed.
    virtual void SetMaxJumps(unsigned char maxJumps)
    {
        setMaxJumps(maxJumps);
    } // Sets the maximum number of jumps allowed.
    virtual unsigned char GetJumpCount()
    {
        return getJumpCount();
    } // Gets the current jump count.
    virtual void SetWalkDirection(const mt::vec3& dir)
    {
        setWalkDirection(ToBullet(dir));
    } // Sets the walk direction of the character.

    virtual mt::vec3 GetWalkDirection()
    {
        return ToMt(getWalkDirection());
    } // Gets the walk direction of the character.

    virtual void SetJumpDirection(const mt::vec3& dir)
    {
        setJumpDirection(ToBullet(dir));
    } // Sets the jump direction of the character.

    virtual mt::vec3 GetJumpDirection()
    {
        return ToMt(getJumpDirection());
    } // Gets the jump direction of the character.

    virtual void SetSmoothMovement(float smoothMovement)
    {
        setSmoothMovement(smoothMovement);
    } // Sets the smoothness of the character's movement.

    virtual const float GetSmoothMovement()
    {
        return getSmoothMovement();
    } // Gets the smoothness of the character's movement.

    virtual float GetFallSpeed() const; // Gets the fall speed of the character.
    virtual void SetFallSpeed(float fallSpeed); // Sets the fall speed of the character.

    virtual float GetMaxSlope() const; // Gets the maximum slope the character can climb.
    virtual void SetMaxSlope(float maxSlope); // Sets the maximum slope the character can climb.

    virtual float GetJumpSpeed() const; // Gets the jump speed of the character.
    virtual void SetJumpSpeed(float jumpSpeed); // Sets the jump speed of the character.

    virtual void SetVelocity(const mt::vec3& vel, float time, bool local); // Sets the velocity of the character.

    virtual void Reset(); // Resets the character controller to its initial state.
};

class CleanPairCallback : public btOverlapCallback
{
    btBroadphaseProxy *m_cleanProxy; // Pointer to the broadphase proxy that is being cleaned.
    btOverlappingPairCache *m_pairCache; // Pointer to the overlapping pair cache being used.
    btDispatcher *m_dispatcher; // Pointer to the Bullet dispatcher.

public:
    CleanPairCallback(btBroadphaseProxy *cleanProxy, btOverlappingPairCache *pairCache, btDispatcher *dispatcher)
        :m_cleanProxy(cleanProxy),
        m_pairCache(pairCache),
        m_dispatcher(dispatcher)
    {
    } // Constructor to initialize the callback with necessary Bullet objects.

    virtual bool processOverlap(btBroadphasePair &pair); // Processes each overlapping pair, used to clean up invalid pairs.
};

/// CcdPhysicsController is a physics object that supports continuous collision detection and time of impact based physics resolution.
class CcdPhysicsController : public PHY_IPhysicsController, public mt::SimdClassAllocator
{
protected:
    btCollisionObject *m_object; // Pointer to the Bullet collision object (rigid body, soft body, etc.).
    CcdCharacter *m_characterController; // Pointer to the custom character controller, if this object is a character.

    class PHY_IMotionState *m_MotionState; // Pointer to the custom motion state interface.
    btMotionState *m_bulletMotionState; // Pointer to the Bullet motion state object.
    class btCollisionShape *m_collisionShape; // Pointer to the Bullet collision shape.
    class CcdShapeConstructionInfo *m_shapeInfo; // Pointer to custom shape construction info.
    btCollisionShape *m_bulletChildShape; // Pointer to a child collision shape, used for compound or scaled shapes.

    /// keep track of typed constraints referencing this rigid body
    btAlignedObjectArray<btTypedConstraint *> m_ccdConstraintRefs; // Array of Bullet constraints that reference this object.
    /// needed when updating the controller
    friend class CcdPhysicsEnvironment; // Allows CcdPhysicsEnvironment to access private members for updates.

    //some book keeping for replication
    bool m_softBodyTransformInitialized; // Flag indicating if the soft body transform has been initialized.
    btTransform m_softbodyStartTrans; // Stores the starting transform of the soft body.

    /// Soft body indices for all original vertices.
    std::vector<unsigned int> m_softBodyIndices; // Stores the indices of the original vertices of a soft body.

    void *m_newClientInfo; // Stores client-specific information, used for raycasts or other custom data.
    int m_registerCount;        // Counts the number of times this controller is registered, used for shared controllers among sensors.
    CcdConstructionInfo m_cci; // Stores construction information, necessary for replicating the controller.

    CcdPhysicsController *m_parentRoot; // Pointer to the root parent controller in a compound object hierarchy.

    int m_savedCollisionFlags; // Stores the collision flags before suspending physics.
    short m_savedCollisionFilterGroup; // Stores the collision filter group before suspending physics.
    short m_savedCollisionFilterMask; // Stores the collision filter mask before suspending physics.
    float m_savedMass; // Stores the mass before suspending physics.
    float m_savedFriction; // Stores the friction before suspending physics.
    bool m_savedDyna; // Stores whether the object was dynamic before suspending physics.
    bool m_suspended; // Indicates whether the physics or dynamics of the object are currently suspended.

    void GetWorldOrientation(btMatrix3x3& mat); // Gets the world orientation as a Bullet matrix.

    void CreateRigidbody(); // Creates a Bullet rigid body for the controller.
    bool CreateSoftbody(); // Creates a Bullet soft body for the controller. Returns true on success.
    bool CreateCharacterController(); // Creates a Bullet character controller for the controller. Returns true on success.

    bool Register()
    {
        return (m_registerCount++ == 0);
    } // Increments the register count and returns true if it was the first registration.
    bool Unregister()
    {
        return (--m_registerCount == 0);
    } // Decrements the register count and returns true if it was the last unregistration.

    bool Registered() const
    {
        return (m_registerCount != 0);
    } // Checks if the controller is currently registered.

    void addCcdConstraintRef(btTypedConstraint *c); // Adds a Bullet constraint reference to the controller.
    void removeCcdConstraintRef(btTypedConstraint *c); // Removes a Bullet constraint reference from the controller.
    btTypedConstraint *getCcdConstraintRef(int index); // Gets a Bullet constraint reference at a given index.
    int getNumCcdConstraintRefs() const; // Gets the number of Bullet constraint references.

    void SetWorldOrientation(const btMatrix3x3& mat); // Sets the world orientation using a Bullet matrix.
    void ForceWorldTransform(const btMatrix3x3& mat, const btVector3& pos); // Forces the world transform using a Bullet matrix and position.

public:

	CcdPhysicsController(const CcdConstructionInfo& ci); // Constructor for CcdPhysicsController, initializes with construction information.


	/**
	 * Delete the current Bullet shape used in the rigid body.
	 */
	bool DeleteControllerShape();

	/**
	 * Delete the old Bullet shape and set the new Bullet shape : newShape
	 * \param newShape The new Bullet shape to set, if is nullptr we create a new Bullet shape
	 */
   bool ReplaceControllerShape(btCollisionShape *newShape); // Replaces the collision shape of the controller.

    virtual ~CcdPhysicsController(); // Destructor for CcdPhysicsController.

    CcdConstructionInfo& GetConstructionInfo()
    {
        return m_cci;
    } // Gets the construction information for the controller.

    const CcdConstructionInfo& GetConstructionInfo() const
    {
        return m_cci;
    } // Gets the const construction information for the controller.

    btRigidBody *GetRigidBody(); // Gets the rigid body associated with the controller.
    const btRigidBody *GetRigidBody() const; // Gets the const rigid body associated with the controller.
    btCollisionObject *GetCollisionObject(); // Gets the collision object associated with the controller.
    btSoftBody *GetSoftBody(); // Gets the soft body associated with the controller.
    btKinematicCharacterController *GetCharacterController(); // Gets the kinematic character controller associated with the controller.

    CcdShapeConstructionInfo *GetShapeInfo()
    {
        return m_shapeInfo;
    } // Gets the shape construction information.

    btCollisionShape *GetCollisionShape()
    {
        return m_object->getCollisionShape();
    } // Gets the collision shape of the controller.

    const std::vector<unsigned int>& GetSoftBodyIndices() const; // Gets the indices of the soft body.
	////////////////////////////////////
	// PHY_IPhysicsController interface
	////////////////////////////////////

	/**
	 * SynchronizeMotionStates ynchronizes dynas, kinematic and deformable entities (and do 'late binding')
	 */
	virtual bool SynchronizeMotionStates(float time);

	/**
	 * Called for every physics simulation step. Use this method for
	 * things like limiting linear and angular velocity.
	 */
	void SimulationTick(float timestep);

	/**
	 * WriteMotionStateToDynamics ynchronizes dynas, kinematic and deformable entities (and do 'late binding')
	 */

    virtual void WriteMotionStateToDynamics(bool nondynaonly); // Writes the motion state to the dynamics world.
    virtual void WriteDynamicsToMotionState(); // Writes the dynamics world state to the motion state.

    // controller replication
    virtual void PostProcessReplica(class PHY_IMotionState *motionstate, class PHY_IPhysicsController *parentctrl); // Post-processes a replicated physics controller.
    virtual void SetPhysicsEnvironment(class PHY_IPhysicsEnvironment *env); // Sets the physics environment for the controller.

    // kinematic methods
    virtual void RelativeTranslate(const mt::vec3& dloc, bool local); // Translates the object relative to its current position.
    virtual void RelativeRotate(const mt::mat3&rotval, bool local); // Rotates the object relative to its current orientation.
    virtual mt::mat3 GetOrientation(); // Gets the orientation of the object.
    virtual void SetOrientation(const mt::mat3& orn); // Sets the orientation of the object.
    virtual void SetPosition(const mt::vec3& pos); // Sets the position of the object.
    virtual mt::vec3 GetPosition() const; // Gets the position of the object.
    virtual void SetScaling(const mt::vec3& scale); // Sets the scaling of the object.
    virtual void SetTransform(); // Updates the transform of the object based on position, orientation, and scaling.

    virtual float GetMass(); // Gets the mass of the object.
    virtual void SetMass(float newmass); // Sets the mass of the object.

    virtual float GetFriction(); // Gets the friction coefficient of the object.
    virtual void SetFriction(float newfriction); // Sets the friction coefficient of the object.

    float GetInertiaFactor() const; // Gets the inertia factor of the object.

	// physics methods
    virtual void ApplyImpulse(const mt::vec3& attach, const mt::vec3& impulsein, bool local); // Applies an impulse to the physics object.
    virtual void ApplyTorque(const mt::vec3& torque, bool local); // Applies a torque to the physics object.
    virtual void ApplyForce(const mt::vec3& force, bool local); // Applies a force to the physics object.
    virtual void SetAngularVelocity(const mt::vec3& ang_vel, bool local); // Sets the angular velocity of the physics object.
    virtual void SetLinearVelocity(const mt::vec3& lin_vel, bool local); // Sets the linear velocity of the physics object.
    virtual void Jump(); // Makes the physics object jump.
    virtual void SetActive(bool active); // Sets the active state of the physics object.

    virtual unsigned short GetCollisionGroup() const; // Gets the collision group of the physics object.
    virtual unsigned short GetCollisionMask() const; // Gets the collision mask of the physics object.
    virtual void SetCollisionGroup(unsigned short group); // Sets the collision group of the physics object.
    virtual void SetCollisionMask(unsigned short mask); // Sets the collision mask of the physics object.

    virtual float GetLinearDamping() const; // Gets the linear damping of the physics object.
    virtual float GetAngularDamping() const; // Gets the angular damping of the physics object.
    virtual void SetLinearDamping(float damping); // Sets the linear damping of the physics object.
    virtual void SetAngularDamping(float damping); // Sets the angular damping of the physics object.
    virtual void SetDamping(float linear, float angular); // Sets both linear and angular damping of the physics object.

    virtual void SetSoftMargin(float val); // Sets the soft margin of the physics object.

    // CCD methods
    virtual void SetCcdMotionThreshold(float val); // Sets the CCD motion threshold for continuous collision detection.
    virtual void SetCcdSweptSphereRadius(float val); // Sets the CCD swept sphere radius for continuous collision detection.

    // Soft body methods
    virtual void SetSoftLinStiff(float val); // Sets linear stiffness for soft body.
    virtual void SetSoftAngStiff(float val); // Sets angular stiffness for soft body.
    virtual void SetSoftVolume(float val); // Sets volume for soft body.
    virtual void SetSoftVsRigidHardness(float val); // Sets soft vs rigid hardness.
    virtual void SetSoftVsKineticHardness(float val); // Sets soft vs kinetic hardness.

    virtual void SetSoftVsSoftHardness(float val); // Soft vs soft hardness [0,1] (cluster only)
    virtual void SetSoftVsRigidImpulseSplitCluster(float val); // Soft vs rigid impulse split [0,1] (cluster only)
    virtual void SetSoftVsKineticImpulseSplitCluster(float val); // Soft vs rigid impulse split [0,1] (cluster only)
    virtual void SetSoftVsSoftImpulseSplitCluster(float val); // Soft vs rigid impulse split [0,1] (cluster only)
    virtual void SetVelocitiesCorrectionFactor(float val); // Velocities correction factor (Baumgarte)
    virtual void SetDampingCoefficient(float val); // Damping coefficient [0,1]
    virtual void SetDragCoefficient(float val); // Drag coefficient [0,+inf]
    virtual void SetLiftCoefficient(float val); // Lift coefficient [0,+inf]
    virtual void SetPressureCoefficient(float val); // Pressure coefficient [-inf,+inf]
    virtual void SetVolumeConversationCoefficient(float val); // Volume conversation coefficient [0,+inf]
    virtual void SetDynamicFrictionCoefficient(float val); // Dynamic friction coefficient [0,1]
    virtual void SetPoseMatchingCoefficient(float val); // Pose matching coefficient [0,1]
    virtual void SetRigidContactsHardness(float val); // Rigid contacts hardness [0,1]
    virtual void SetKineticContactsHardness(float val); // Kinetic contacts hardness [0,1]
    virtual void SetSoftContactsHardness(float val); // Soft contacts hardness [0,1]
    virtual void SetAnchorsHardness(float val); // Anchors hardness [0,1]

    virtual void SetVelocitySolverIterations(int iterations); // Velocity solver iterations
    virtual void SetPositionSolverIterations(int iterations); // Position solver iterations
    virtual void SetDriftSolverIterations(int iterations); // Drift solver iterations
    virtual void SetClusterSolverIterations(int iterations); // Cluster solver iterations
    virtual void SetSoftPoseMatching(bool enableShapeMatching); // Shape matching enabled: disable pose update, relative pose.


    virtual void SetGravity(const mt::vec3 &gravity); // Sets the gravity vector.

    // reading out information from physics
    virtual mt::vec3 GetLinearVelocity(); // Gets the linear velocity of the physics object.
    virtual mt::vec3 GetAngularVelocity(); // Gets the angular velocity of the physics object.
    virtual mt::vec3 GetVelocity(const mt::vec3& posin); // Gets the velocity at a specific position.
    virtual mt::vec3 GetLocalInertia(); // Gets the local inertia tensor of the physics object.
    virtual mt::vec3 GetGravity(); // Gets the current gravity vector.

    // dyna's that are rigidbody are free in orientation, dyna's with non-rigidbody are restricted
    virtual void SetRigidBody(bool rigid); // Sets whether the physics object is a rigid body.

    virtual void RefreshCollisions(); // Refreshes the collision data.
    virtual void SuspendPhysics(bool freeConstraints); // Suspends physics simulation, optionally freeing constraints.
    virtual void RestorePhysics(); // Restores physics simulation.
    virtual void SuspendDynamics(bool ghost); // Suspends dynamic simulation, optionally making it a ghost object.
    virtual void RestoreDynamics(); // Restores dynamic simulation.

    // Shape control
    virtual void AddCompoundChild(PHY_IPhysicsController *child); // Adds a child physics controller to a compound shape.
    virtual void RemoveCompoundChild(PHY_IPhysicsController *child); // Removes a child physics controller from a compound shape.

    // clientinfo for raycasts for example
    virtual void *GetNewClientInfo(); // Gets a new client info pointer.
    virtual void SetNewClientInfo(void *clientinfo); // Sets the client info pointer.
    virtual PHY_IPhysicsController *GetReplica(); // Gets the replica physics controller.
    virtual PHY_IPhysicsController *GetReplicaForSensors(); // Gets the replica physics controller for sensors.

    ///There should be no 'SetCollisionFilterGroup' method, as changing this during run-time is will result in errors
    short int GetCollisionFilterGroup() const
    {
        return m_cci.m_collisionFilterGroup;
    } // Gets the collision filter group.


    ///There should be no 'SetCollisionFilterGroup' method, as changing this during run-time is will result in errors
    short int GetCollisionFilterMask() const
    {
        return m_cci.m_collisionFilterMask;
    } // Gets the collision filter mask.


    virtual void SetMargin(float margin)
    {
        if (m_collisionShape) {
            m_collisionShape->setMargin(margin);
            // if the shape use a unscaled shape we have also to set the correct margin in it
            if (m_collisionShape->getShapeType() == SCALED_TRIANGLE_MESH_SHAPE_PROXYTYPE)
                ((btScaledBvhTriangleMeshShape *)m_collisionShape)->getChildShape()->setMargin(margin);
        }
    } // Sets the collision shape margin.


    virtual float GetMargin() const
    {
        return (m_collisionShape) ? m_collisionShape->getMargin() : 0.0f;
    } // Gets the collision shape margin.


    virtual float GetRadius() const
    {
        // this is not the actual shape radius, it's only used for Fh support
        return m_cci.m_radius;
    } // Gets the radius (for Fh support).


    virtual void  SetRadius(float margin)
    {
        if (m_collisionShape && m_collisionShape->getShapeType() == SPHERE_SHAPE_PROXYTYPE) {
            btSphereShape *sphereShape = static_cast<btSphereShape *>(m_collisionShape);
            sphereShape->setUnscaledRadius(margin);
        }
        m_cci.m_radius = margin;
    } // Sets the radius (for sphere shapes).tRadius(float radius); // Sets the radius (for sphere shapes).

    /// velocity clamping
    virtual void SetLinVelocityMin(float val)
    {
        m_cci.m_clamp_vel_min = val;
    } // Sets the minimum linear velocity clamp value.

    virtual float GetLinVelocityMin() const
    {
        return m_cci.m_clamp_vel_min;
    } // Gets the minimum linear velocity clamp value.

    virtual void SetLinVelocityMax(float val)
    {
        m_cci.m_clamp_vel_max = val;
    } // Sets the maximum linear velocity clamp value.

    virtual float GetLinVelocityMax() const
    {
        return m_cci.m_clamp_vel_max;
    } // Gets the maximum linear velocity clamp value.

    virtual void SetAngularVelocityMin(float val)
    {
        m_cci.m_clamp_angvel_min = val;
    } // Sets the minimum angular velocity clamp value.

    virtual float GetAngularVelocityMin() const
    {
        return m_cci.m_clamp_angvel_min;
    } // Gets the minimum angular velocity clamp value.

    virtual void SetAngularVelocityMax(float val)
    {
        m_cci.m_clamp_angvel_max = val;
    } // Sets the maximum angular velocity clamp value.

    virtual float GetAngularVelocityMax() const
    {
        return m_cci.m_clamp_angvel_max;
    } // Gets the maximum angular velocity clamp value.


    bool WantsSleeping(); // Checks if the physics object wants to enter a sleeping state.

    void UpdateDeactivation(float timeStep); // Updates the deactivation timer based on the time step.

    void SetCenterOfMassTransform(btTransform& xform); // Sets the center of mass transform of the physics object.

    static btTransform GetTransformFromMotionState(PHY_IMotionState *motionState); // Gets the transform from a given motion state.

    class PHY_IMotionState *GetMotionState()
    {
        return m_MotionState;
    } // Gets the motion state of the physics object.

    const class PHY_IMotionState *GetMotionState() const
    {
        return m_MotionState;
    } // Gets the const motion state of the physics object.

    class CcdPhysicsEnvironment *GetPhysicsEnvironment()
    {
        return m_cci.m_physicsEnv;
    } // Gets the physics environment associated with the object.

    void SetParentRoot(CcdPhysicsController *parentCtrl)
    {
        m_parentRoot = parentCtrl;
    } // Sets the parent root controller for compound objects.

    CcdPhysicsController *GetParentRoot() const
    {
        return m_parentRoot;
    } // Gets the parent root controller for compound objects.

    virtual bool IsDynamic()
    {
        return GetConstructionInfo().m_bDyna;
    } // Checks if the physics object is dynamic.

    virtual bool IsDynamicsSuspended() const
    {
        return m_suspended;
    } // Checks if the dynamics of the physics object are suspended.

    virtual bool IsPhysicsSuspended(); // Checks if the physics of the object is suspended.

    virtual bool IsCompound()
    {
        return GetConstructionInfo().m_shapeInfo->m_shapeType == PHY_SHAPE_COMPOUND;
    } // Checks if the physics object is a compound shape.

    virtual bool ReinstancePhysicsShape(KX_GameObject *from_gameobj, RAS_Mesh *from_meshobj, bool dupli = false); // Reinstances the physics shape based on game object and mesh data.
    virtual bool ReplacePhysicsShape(PHY_IPhysicsController *phyctrl); // Replaces the current physics shape with another physics controller's shape.
	
};
/// DefaultMotionState implements standard motionstate, using btTransform
class DefaultMotionState : public PHY_IMotionState, public mt::SimdClassAllocator
{
public:
    DefaultMotionState(); // Constructor for DefaultMotionState.

    virtual ~DefaultMotionState(); // Destructor for DefaultMotionState.

    virtual mt::vec3 GetWorldPosition() const; // Gets the world position.
    virtual mt::vec3 GetWorldScaling() const; // Gets the world scaling.
    virtual mt::mat3 GetWorldOrientation() const; // Gets the world orientation as a matrix.

    virtual void SetWorldPosition(const mt::vec3& pos); // Sets the world position.
    virtual void SetWorldOrientation(const mt::mat3& ori); // Sets the world orientation from a matrix.
    virtual void SetWorldOrientation(const mt::quat& quat); // Sets the world orientation from a quaternion.

    virtual void CalculateWorldTransformations(); // Calculates and updates the world transformations.

    btTransform m_worldTransform; // Stores the world transform.
    btVector3 m_localScaling; // Stores the local scaling.
};

#endif  /* __CCDPHYSICSCONTROLLER_H__ */
