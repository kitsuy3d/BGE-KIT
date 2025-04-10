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

/** \file gameengine/Ketsji/KX_PyConstraintBinding.cpp
 *  \ingroup ketsji
 */

#include "KX_PyConstraintBinding.h"
#include "PHY_IPhysicsEnvironment.h"
#include "KX_ConstraintWrapper.h"
#include "KX_VehicleWrapper.h"
#include "KX_CharacterWrapper.h"
#include "PHY_IPhysicsController.h"
#include "PHY_IVehicle.h"
#include "PHY_DynamicTypes.h"

#include "KX_GameObject.h" // ConvertPythonToGameObject()
#include "KX_Globals.h"

#include "EXP_PyObjectPlus.h"

#ifdef WITH_BULLET
#  include "LinearMath/btIDebugDraw.h"
#endif

#ifdef WITH_PYTHON

// macro copied from KX_PythonInit.cpp
#define KX_MACRO_addTypesToDict(dict, name, name2) PyDict_SetItemString(dict, #name, item = PyLong_FromLong(name2)); Py_DECREF(item)


PyDoc_STRVAR(PhysicsConstraints_module_documentation,
             "This is the Python API for the Physics Constraints"
             );

PyDoc_STRVAR(gPySetGravity__doc__,
             "setGravity(float x,float y,float z)\n"
             ""
             );
PyDoc_STRVAR(gPySetDebugMode__doc__,
             "setDebugMode(int mode)\n"
             ""
             );

PyDoc_STRVAR(gPySetNumIterations__doc__,
             "setNumIterations(int numiter)\n"
             "This sets the number of iterations for an iterative constraint solver"
             );

PyDoc_STRVAR(gPySetErp__doc__,
             "setErp(float numiter)\n"
             "SplitImpulse penetration depth before for use, default 0.2"
             );

PyDoc_STRVAR(gPySetErp2__doc__,
             "setErp2(float numiter)\n"
             "SplitImpulse penetration depth before for use, default 0.8"
             );

PyDoc_STRVAR(gPySetGlobalCfm__doc__,
             "setGlobalCfm(float numiter)\n"
             "constraint force mixing for contacts and non-contacts, default 0.0"
             );

PyDoc_STRVAR(gPySetSplitImpulse__doc__,
             "setSplitImpulse(bool value)\n"
             "By default, Bullet solves positional constraints and velocity constraints coupled together."
             );

PyDoc_STRVAR(gPySetSplitImpulsePenetrationThreshold__doc__,
             "setSplitImpulsePenetrationThreshold(float numiter)\n"
             "Penetration Threshold before splitImpulse is used, default -0.04"
             );

PyDoc_STRVAR(gPySetSplitImpulseTurnErp__doc__,
             "setSplitImpulseTurnErp(float numiter)\n"
             "Constraints fix for splitImpulse, default 0.1"
             );

PyDoc_STRVAR(gPySetLinearSlop__doc__,
             "setLinearSlop(float numiter)\n"
             "Defines the penetration depth for object collisions. A value of 1 equals to 1 world unit penetration depth (overlapping)."
             );

PyDoc_STRVAR(gPySetWarmstartingFactor__doc__,
             "setWarmstartingFactor(float numiter)\n"
             "This sets the number of physics warmstarting factor, last frames physics with 1.0 or not with 0.0"
             );

PyDoc_STRVAR(gPySetMaxGyroscopicForce__doc__,
             "setMaxGyroscopicForce(float numiter)\n"
             "It is only used for 'explicit' version of gyroscopic force."
             );

PyDoc_STRVAR(gPySetNumTimeSubSteps__doc__,
             "setNumTimeSubSteps(int numsubstep)\n"
             "This sets the number of substeps for each physics proceed. Tradeoff quality for performance."
             );

PyDoc_STRVAR(gPySetDeactivationTime__doc__,
             "setDeactivationTime(float time)\n"
             "This sets the time after which a resting rigidbody gets deactived"
             );
PyDoc_STRVAR(gPySetDeactivationLinearTreshold__doc__,
             "setDeactivationLinearTreshold(float linearTreshold)\n"
             ""
             );
PyDoc_STRVAR(gPySetDeactivationAngularTreshold__doc__,
             "setDeactivationAngularTreshold(float angularTreshold)\n"
             ""
             );
PyDoc_STRVAR(gPySetContactBreakingTreshold__doc__,
             "setContactBreakingTreshold(float breakingTreshold)\n"
             "Reasonable default is 0.02 (if units are meters)"
             );

PyDoc_STRVAR(gPySetCcdMode__doc__,
             "setCcdMode(int ccdMode)\n"
             "Very experimental, not recommended"
             );
PyDoc_STRVAR(gPySetSorConstant__doc__,
             "setSorConstant(float sor)\n"
             "Very experimental, not recommended"
             );
PyDoc_STRVAR(gPySetSolverTau__doc__,
             "setTau(float tau)\n"
             "Very experimental, not recommended"
             );
PyDoc_STRVAR(gPySetSolverDamping__doc__,
             "setDamping(float damping)\n"
             "Very experimental, not recommended"
             );
PyDoc_STRVAR(gPySetLinearAirDamping__doc__,
             "setLinearAirDamping(float damping)\n"
             "Very experimental, not recommended"
             );
PyDoc_STRVAR(gPySetUseEpa__doc__,
             "setUseEpa(int epa)\n"
             "Very experimental, not recommended"
             );
PyDoc_STRVAR(gPySetSolverType__doc__,
             "setSolverType(int solverType)\n"
             "Very experimental, not recommended"
             );

PyDoc_STRVAR(gPyCreateConstraint__doc__,
             "createConstraint(ob1,ob2,float restLength,float restitution,float damping)\n"
             ""
             );
PyDoc_STRVAR(gPyCreateVehicle__doc__,
             "createVehicle(chassis)\n"
             ""
             );
PyDoc_STRVAR(gPyGetVehicleConstraint__doc__,
             "getVehicleConstraint(int constraintId)\n"
             ""
             );
PyDoc_STRVAR(gPyGetCharacter__doc__,
             "getCharacter(KX_GameObject obj)\n"
             ""
             );
PyDoc_STRVAR(gPyRemoveConstraint__doc__,
             "removeConstraint(int constraintId)\n"
             ""
             );
PyDoc_STRVAR(gPyGetAppliedImpulse__doc__,
             "getAppliedImpulse(int constraintId)\n"
             ""
             );




static PyObject *gPySetGravity(PyObject *self,
                               PyObject *args,
                               PyObject *kwds)
{
	float x, y, z;
	if (PyArg_ParseTuple(args, "fff", &x, &y, &z)) {
		if (KX_GetPhysicsEnvironment()) {
			KX_GetPhysicsEnvironment()->SetGravity(x, y, z);
		}
	}
	else {
		return nullptr;
	}

	Py_RETURN_NONE;
}

static PyObject *gPySetDebugMode(PyObject *self,
                                 PyObject *args,
                                 PyObject *kwds)
{
	int mode;
	if (PyArg_ParseTuple(args, "i", &mode)) {
		if (KX_GetPhysicsEnvironment()) {
			KX_GetPhysicsEnvironment()->SetDebugMode(mode);

		}

	}
	else {
		return nullptr;
	}

	Py_RETURN_NONE;
}



static PyObject *gPySetNumTimeSubSteps(PyObject *self,
                                       PyObject *args,
                                       PyObject *kwds)
{
	int substep;
	if (PyArg_ParseTuple(args, "i", &substep)) {
		if (KX_GetPhysicsEnvironment()) {
			KX_GetPhysicsEnvironment()->SetNumTimeSubSteps(substep);
		}
	}
	else {
		return nullptr;
	}
	Py_RETURN_NONE;
}


static PyObject *gPySetNumIterations(PyObject *self,
                                     PyObject *args,
                                     PyObject *kwds)
{
	int iter;
	if (PyArg_ParseTuple(args, "i", &iter)) {
		if (KX_GetPhysicsEnvironment()) {
			KX_GetPhysicsEnvironment()->SetNumIterations(iter);
		}
	}
	else {
		return nullptr;
	}
	Py_RETURN_NONE;
}

static PyObject *gPySetErp(PyObject *self,
                                    PyObject *args,
                                    PyObject *kwds)
{
	float erp;
	if (PyArg_ParseTuple(args, "f", &erp)) {
		if (KX_GetPhysicsEnvironment()) {
			KX_GetPhysicsEnvironment()->SetErp(erp);
		}
	}
	else {
		return nullptr;
	}
	Py_RETURN_NONE;
}

static PyObject *gPySetErp2(PyObject *self,
                                    PyObject *args,
                                    PyObject *kwds)
{
	float erp2;
	if (PyArg_ParseTuple(args, "f", &erp2)) {
		if (KX_GetPhysicsEnvironment()) {
			KX_GetPhysicsEnvironment()->SetErp2(erp2);
		}
	}
	else {
		return nullptr;
	}
	Py_RETURN_NONE;
}

static PyObject *gPySetGlobalCfm(PyObject *self,
                                 PyObject *args,
                                 PyObject *kwds)
{
	float globalCfm;
	if (PyArg_ParseTuple(args, "f", &globalCfm)) {
		if (KX_GetPhysicsEnvironment()) {
			KX_GetPhysicsEnvironment()->SetGlobalCfm(globalCfm);
		}
	}
	else {
		return nullptr;
	}
	Py_RETURN_NONE;
}

static PyObject *gPySetSplitImpulse(PyObject *self,
                                    PyObject *args,
                                    PyObject *kwds)
{
	int splitImpulse;
	if (PyArg_ParseTuple(args, "i", &splitImpulse)) {
		if (KX_GetPhysicsEnvironment()) {
			KX_GetPhysicsEnvironment()->SetSplitImpulse(splitImpulse);
		}
	}
	else {
		return nullptr;
	}
	Py_RETURN_NONE;
}

static PyObject *gPySetSplitImpulsePenetrationThreshold(PyObject *self,
                                    PyObject *args,
                                    PyObject *kwds)
{
	float splitImpulsePenetrationThreshold;
	if (PyArg_ParseTuple(args, "f", &splitImpulsePenetrationThreshold)) {
		if (KX_GetPhysicsEnvironment()) {
			KX_GetPhysicsEnvironment()->SetSplitImpulsePenetrationThreshold(splitImpulsePenetrationThreshold);
		}
	}
	else {
		return nullptr;
	}
	Py_RETURN_NONE;
}

static PyObject *gPySetSplitImpulseTurnErp(PyObject *self,
                                    PyObject *args,
                                    PyObject *kwds)
{
	float splitImpulseTurnErp;
	if (PyArg_ParseTuple(args, "f", &splitImpulseTurnErp)) {
		if (KX_GetPhysicsEnvironment()) {
			KX_GetPhysicsEnvironment()->SetSplitImpulseTurnErp(splitImpulseTurnErp);
		}
	}
	else {
		return nullptr;
	}
	Py_RETURN_NONE;
}

static PyObject *gPySetLinearSlop(PyObject *self,
                                  PyObject *args,
                                  PyObject *kwds)
{
	float slop;
	if (PyArg_ParseTuple(args, "f", &slop)) {
		if (KX_GetPhysicsEnvironment()) {
			KX_GetPhysicsEnvironment()->SetLinearSlop(slop);
		}
	}
	else {
		return nullptr;
	}
	Py_RETURN_NONE;
}


static PyObject *gPySetWarmstartingFactor(PyObject *self,
                                          PyObject *args,
                                          PyObject *kwds)
{
	float factor;
	if (PyArg_ParseTuple(args, "f", &factor)) {
		if (KX_GetPhysicsEnvironment()) {
			KX_GetPhysicsEnvironment()->SetWarmstartingFactor(factor);
		}
	}
	else {
		return nullptr;
	}
	Py_RETURN_NONE;
}

static PyObject *gPySetMaxGyroscopicForce(PyObject *self,
                                          PyObject *args,
                                          PyObject *kwds)
{
	float maxGyroscopicForce;
	if (PyArg_ParseTuple(args, "f", &maxGyroscopicForce)) {
		if (KX_GetPhysicsEnvironment()) {
			KX_GetPhysicsEnvironment()->SetMaxGyroscopicForce(maxGyroscopicForce);
		}
	}
	else {
		return nullptr;
	}
	Py_RETURN_NONE;
}

static PyObject *gPySetDeactivationTime(PyObject *self,
                                        PyObject *args,
                                        PyObject *kwds)
{
	float deactive_time;
	if (PyArg_ParseTuple(args, "f", &deactive_time)) {
		if (KX_GetPhysicsEnvironment()) {
			KX_GetPhysicsEnvironment()->SetDeactivationTime(deactive_time);
		}
	}
	else {
		return nullptr;
	}
	Py_RETURN_NONE;
}


static PyObject *gPySetDeactivationLinearTreshold(PyObject *self,
                                                  PyObject *args,
                                                  PyObject *kwds)
{
	float linearDeactivationTreshold;
	if (PyArg_ParseTuple(args, "f", &linearDeactivationTreshold)) {
		if (KX_GetPhysicsEnvironment()) {
			KX_GetPhysicsEnvironment()->SetDeactivationLinearTreshold(linearDeactivationTreshold);
		}
	}
	else {
		return nullptr;
	}
	Py_RETURN_NONE;
}


static PyObject *gPySetDeactivationAngularTreshold(PyObject *self,
                                                   PyObject *args,
                                                   PyObject *kwds)
{
	float angularDeactivationTreshold;
	if (PyArg_ParseTuple(args, "f", &angularDeactivationTreshold)) {
		if (KX_GetPhysicsEnvironment()) {
			KX_GetPhysicsEnvironment()->SetDeactivationAngularTreshold(angularDeactivationTreshold);
		}
	}
	else {
		return nullptr;
	}
	Py_RETURN_NONE;
}

static PyObject *gPySetContactBreakingTreshold(PyObject *self,
                                               PyObject *args,
                                               PyObject *kwds)
{
	float contactBreakingTreshold;
	if (PyArg_ParseTuple(args, "f", &contactBreakingTreshold)) {
		if (KX_GetPhysicsEnvironment()) {
			KX_GetPhysicsEnvironment()->SetContactBreakingTreshold(contactBreakingTreshold);
		}
	}
	else {
		return nullptr;
	}
	Py_RETURN_NONE;
}


static PyObject *gPySetCcdMode(PyObject *self,
                               PyObject *args,
                               PyObject *kwds)
{
	float ccdMode;
	if (PyArg_ParseTuple(args, "f", &ccdMode)) {
		if (KX_GetPhysicsEnvironment()) {
			KX_GetPhysicsEnvironment()->SetCcdMode(ccdMode);
		}
	}
	else {
		return nullptr;
	}
	Py_RETURN_NONE;
}

static PyObject *gPySetSorConstant(PyObject *self,
                                   PyObject *args,
                                   PyObject *kwds)
{
	float sor;
	if (PyArg_ParseTuple(args, "f", &sor)) {
		if (KX_GetPhysicsEnvironment()) {
			KX_GetPhysicsEnvironment()->SetSolverSorConstant(sor);
		}
	}
	else {
		return nullptr;
	}
	Py_RETURN_NONE;
}

static PyObject *gPySetSolverTau(PyObject *self,
                                 PyObject *args,
                                 PyObject *kwds)
{
	float tau;
	if (PyArg_ParseTuple(args, "f", &tau)) {
		if (KX_GetPhysicsEnvironment()) {
			KX_GetPhysicsEnvironment()->SetSolverTau(tau);
		}
	}
	else {
		return nullptr;
	}
	Py_RETURN_NONE;
}


static PyObject *gPySetSolverDamping(PyObject *self,
                                     PyObject *args,
                                     PyObject *kwds)
{
	float damping;
	if (PyArg_ParseTuple(args, "f", &damping)) {
		if (KX_GetPhysicsEnvironment()) {
			KX_GetPhysicsEnvironment()->SetSolverDamping(damping);
		}
	}
	else {
		return nullptr;
	}
	Py_RETURN_NONE;
}

static PyObject *gPySetLinearAirDamping(PyObject *self,
                                        PyObject *args,
                                        PyObject *kwds)
{
	float damping;
	if (PyArg_ParseTuple(args, "f", &damping)) {
		if (KX_GetPhysicsEnvironment()) {
			KX_GetPhysicsEnvironment()->SetLinearAirDamping(damping);
		}
	}
	else {
		return nullptr;
	}
	Py_RETURN_NONE;
}


static PyObject *gPySetUseEpa(PyObject *self,
                              PyObject *args,
                              PyObject *kwds)
{
	int epa;
	if (PyArg_ParseTuple(args, "i", &epa)) {
		if (KX_GetPhysicsEnvironment()) {
			KX_GetPhysicsEnvironment()->SetUseEpa(epa);
		}
	}
	else {
		return nullptr;
	}
	Py_RETURN_NONE;
}
static PyObject *gPySetSolverType(PyObject *self,
                                  PyObject *args,
                                  PyObject *kwds)
{
	int solverType;
	if (PyArg_ParseTuple(args, "i", &solverType)) {
		if (KX_GetPhysicsEnvironment()) {
			KX_GetPhysicsEnvironment()->SetSolverType((PHY_SolverType)solverType);
		}
	}
	else {
		return nullptr;
	}
	Py_RETURN_NONE;
}



static PyObject *gPyGetVehicleConstraint(PyObject *self,
                                         PyObject *args,
                                         PyObject *kwds)
{
#if defined(_WIN64)
	__int64 constraintid;
	if (PyArg_ParseTuple(args, "L", &constraintid))
#else
	long constraintid;
	if (PyArg_ParseTuple(args, "l", &constraintid))
#endif
	{
		if (KX_GetPhysicsEnvironment()) {

			PHY_IVehicle *vehicle = KX_GetPhysicsEnvironment()->GetVehicleConstraint(constraintid);
			if (vehicle) {
				KX_VehicleWrapper *pyWrapper = new KX_VehicleWrapper(vehicle);
				return pyWrapper->NewProxy(true);
			}

		}
	}
	else {
		return nullptr;
	}

	Py_RETURN_NONE;
}

static PyObject *gPyGetCharacter(PyObject *self,
                                 PyObject *args,
                                 PyObject *kwds)
{
	PyObject *pyob;
	KX_GameObject *ob;

	if (!PyArg_ParseTuple(args, "O", &pyob)) {
		return nullptr;
	}

	if (!ConvertPythonToGameObject(KX_GetActiveScene()->GetLogicManager(), pyob, &ob, false, "bge.constraints.getCharacter(value)")) {
		return nullptr;
	}

	if (KX_GetPhysicsEnvironment()) {

		PHY_ICharacter *character = KX_GetPhysicsEnvironment()->GetCharacterController(ob);
		if (character) {
			KX_CharacterWrapper *pyWrapper = new KX_CharacterWrapper(character);
			return pyWrapper->NewProxy(true);
		}

	}

	Py_RETURN_NONE;
}

static PyObject *gPyCreateConstraint(PyObject *self,
                                     PyObject *args,
                                     PyObject *kwds)
{
	/* FIXME - physicsid is a long being cast to a pointer, should at least use PyCapsule */
	unsigned long long physicsid = 0, physicsid2 = 0;
	int constrainttype = 0;
	int flag = 0;
	float pivotX = 0.0f, pivotY = 0.0f, pivotZ = 0.0f, axisX = 0.0f, axisY = 0.0f, axisZ = 0.0f;

	if (!EXP_ParseTupleArgsAndKeywords(args, kwds, "KKi|ffffffi:createConstraint", {"physicsid_1", "physicsid_2",
	                                                                                "constraint_type", "pivot_x", "pivot_y", "pivot_z", "axis_x", "axis_y", "axis_z", "flag", 0},
	                                   &physicsid, &physicsid2, &constrainttype, &pivotX, &pivotY, &pivotZ, &axisX, &axisY, &axisZ, &flag)) {
		return nullptr;
	}

	if (KX_GetPhysicsEnvironment()) {
		PHY_IPhysicsController *physctrl = (PHY_IPhysicsController *)physicsid;
		PHY_IPhysicsController *physctrl2 = (PHY_IPhysicsController *)physicsid2;
		if (physctrl) { //TODO:check for existence of this pointer!
			if (constrainttype == PHY_VEHICLE_CONSTRAINT) {
				PHY_IVehicle *vehicle = KX_GetPhysicsEnvironment()->CreateVehicle(physctrl);

				KX_VehicleWrapper *wrap = new KX_VehicleWrapper(vehicle);

				return wrap->NewProxy(true);
			}
			//convert from euler angle into axis
			const float deg2rad = 0.017453292f;

			//we need to pass a full constraint frame, not just axis
			//localConstraintFrameBasis
			mt::mat3 localCFrame(mt::vec3(deg2rad *axisX, deg2rad *axisY, deg2rad *axisZ));
			mt::vec3 axis0 = localCFrame.GetColumn(0);
			mt::vec3 axis1 = localCFrame.GetColumn(1);
			mt::vec3 axis2 = localCFrame.GetColumn(2);

			PHY_IConstraint *constraint = KX_GetPhysicsEnvironment()->CreateConstraint(
				physctrl, physctrl2, (enum PHY_ConstraintType)constrainttype, pivotX, pivotY, pivotZ,
				(float)axis0.x, (float)axis0.y, (float)axis0.z,
				(float)axis1.x, (float)axis1.y, (float)axis1.z,
				(float)axis2.x, (float)axis2.y, (float)axis2.z, flag);

			if (!constraint) {
				return nullptr;
			}

			KX_ConstraintWrapper *wrap = new KX_ConstraintWrapper(constraint);

			return wrap->NewProxy(true);
		}
	}
	Py_RETURN_NONE;
}

static PyObject *gPyCreateVehicle(PyObject *self, PyObject *args)
{
	/* FIXME - physicsid is a long being cast to a pointer, should at least use PyCapsule */
	unsigned long long physicsid = 0;

	if (!PyArg_ParseTuple(args, "K:createVehicle", &physicsid)) {
		return nullptr;
	}

	if (!KX_GetPhysicsEnvironment()) {
		Py_RETURN_NONE;
	}

	PHY_IPhysicsController *physctrl = (PHY_IPhysicsController *)physicsid;
	if (!physctrl) { //TODO:check for existence of this pointer!
		return nullptr;
	}

	PHY_IVehicle *vehicle = KX_GetPhysicsEnvironment()->CreateVehicle(physctrl);
	if (!vehicle) {
		return nullptr;
	}

	KX_VehicleWrapper *wrap = new KX_VehicleWrapper(vehicle);

	return wrap->NewProxy(true);
}

static PyObject *gPyGetAppliedImpulse(PyObject *self,
                                      PyObject *args,
                                      PyObject *kwds)
{
	float appliedImpulse = 0.f;

#if defined(_WIN64)
	__int64 constraintid;
	if (PyArg_ParseTuple(args, "L", &constraintid))
#else
	long constraintid;
	if (PyArg_ParseTuple(args, "l", &constraintid))
#endif
	{
		if (KX_GetPhysicsEnvironment()) {
			appliedImpulse = KX_GetPhysicsEnvironment()->GetAppliedImpulse(constraintid);
		}
	}
	else {
		return nullptr;
	}

	return PyFloat_FromDouble(appliedImpulse);
}


static PyObject *gPyRemoveConstraint(PyObject *self,
                                     PyObject *args,
                                     PyObject *kwds)
{
#if defined(_WIN64)
	__int64 constraintid;
	if (PyArg_ParseTuple(args, "L", &constraintid))
#else
	long constraintid;
	if (PyArg_ParseTuple(args, "l", &constraintid))
#endif
	{
		if (KX_GetPhysicsEnvironment()) {
			KX_GetPhysicsEnvironment()->RemoveConstraintById(constraintid, true);
		}
	}
	else {
		return nullptr;
	}

	Py_RETURN_NONE;
}

static PyObject *gPyExportBulletFile(PyObject *, PyObject *args)
{
	char *filename;
	if (!PyArg_ParseTuple(args, "s:exportBulletFile", &filename)) {
		return nullptr;
	}

	if (KX_GetPhysicsEnvironment()) {
		KX_GetPhysicsEnvironment()->ExportFile(filename);
	}
	Py_RETURN_NONE;
}

static struct PyMethodDef physicsconstraints_methods[] = {
	{"setGravity", (PyCFunction)gPySetGravity,
	 METH_VARARGS, (const char *)gPySetGravity__doc__},
	{"setDebugMode", (PyCFunction)gPySetDebugMode,
	 METH_VARARGS, (const char *)gPySetDebugMode__doc__},
	{"setNumIterations", (PyCFunction)gPySetNumIterations,
	 METH_VARARGS, (const char *)gPySetNumIterations__doc__},
	{"setErp", (PyCFunction)gPySetErp,
	 METH_VARARGS, (const char *)gPySetErp__doc__},
	{"setErp2", (PyCFunction)gPySetErp2,
	 METH_VARARGS, (const char *)gPySetErp2__doc__},
	{"setGlobalCfm", (PyCFunction)gPySetGlobalCfm,
	 METH_VARARGS, (const char *)gPySetGlobalCfm__doc__},
	{"setSplitImpulse", (PyCFunction)gPySetSplitImpulse,
	 METH_VARARGS, (const char *)gPySetSplitImpulse__doc__},
	{"setSplitImpulsePenetrationThreshold", (PyCFunction)gPySetSplitImpulsePenetrationThreshold,
	 METH_VARARGS, (const char *)gPySetSplitImpulsePenetrationThreshold__doc__},
	{"setSplitImpulseTurnErp", (PyCFunction)gPySetSplitImpulseTurnErp,
	 METH_VARARGS, (const char *)gPySetSplitImpulseTurnErp__doc__},
	{"setLinearSlop", (PyCFunction)gPySetLinearSlop,
	 METH_VARARGS, (const char *)gPySetLinearSlop__doc__},
	{"setWarmstartingFactor", (PyCFunction)gPySetWarmstartingFactor,
	 METH_VARARGS, (const char *)gPySetWarmstartingFactor__doc__},
	{"setMaxGyroscopicForce", (PyCFunction)gPySetMaxGyroscopicForce,
	 METH_VARARGS, (const char *)gPySetMaxGyroscopicForce__doc__},
	/// settings that influence quality of the rigidbody dynamics

	{"setNumTimeSubSteps", (PyCFunction)gPySetNumTimeSubSteps,
	 METH_VARARGS, (const char *)gPySetNumTimeSubSteps__doc__},

	{"setDeactivationTime", (PyCFunction)gPySetDeactivationTime,
	 METH_VARARGS, (const char *)gPySetDeactivationTime__doc__},

	{"setDeactivationLinearTreshold", (PyCFunction)gPySetDeactivationLinearTreshold,
	 METH_VARARGS, (const char *)gPySetDeactivationLinearTreshold__doc__},
	{"setDeactivationAngularTreshold", (PyCFunction)gPySetDeactivationAngularTreshold,
	 METH_VARARGS, (const char *)gPySetDeactivationAngularTreshold__doc__},

	{"setContactBreakingTreshold", (PyCFunction)gPySetContactBreakingTreshold,
	 METH_VARARGS, (const char *)gPySetContactBreakingTreshold__doc__},
	{"setCcdMode", (PyCFunction)gPySetCcdMode,
	 METH_VARARGS, (const char *)gPySetCcdMode__doc__},
	{"setSorConstant", (PyCFunction)gPySetSorConstant,
	 METH_VARARGS, (const char *)gPySetSorConstant__doc__},
	{"setSolverTau", (PyCFunction)gPySetSolverTau,
	 METH_VARARGS, (const char *)gPySetSolverTau__doc__},
	{"setSolverDamping", (PyCFunction)gPySetSolverDamping,
	 METH_VARARGS, (const char *)gPySetSolverDamping__doc__},

	{"setLinearAirDamping", (PyCFunction)gPySetLinearAirDamping,
	 METH_VARARGS, (const char *)gPySetLinearAirDamping__doc__},

	{"setUseEpa", (PyCFunction)gPySetUseEpa,
	 METH_VARARGS, (const char *)gPySetUseEpa__doc__},
	{"setSolverType", (PyCFunction)gPySetSolverType,
	 METH_VARARGS, (const char *)gPySetSolverType__doc__},


	{"createConstraint", (PyCFunction)gPyCreateConstraint,
	 METH_VARARGS | METH_KEYWORDS, (const char *)gPyCreateConstraint__doc__},
	{"createVehicle", (PyCFunction)gPyCreateVehicle,
	 METH_VARARGS, (const char *)gPyCreateVehicle__doc__},
	{"getVehicleConstraint", (PyCFunction)gPyGetVehicleConstraint,
	 METH_VARARGS, (const char *)gPyGetVehicleConstraint__doc__},

	{"getCharacter", (PyCFunction)gPyGetCharacter,
	 METH_VARARGS, (const char *)gPyGetCharacter__doc__},

	{"removeConstraint", (PyCFunction)gPyRemoveConstraint,
	 METH_VARARGS, (const char *)gPyRemoveConstraint__doc__},
	{"getAppliedImpulse", (PyCFunction)gPyGetAppliedImpulse,
	 METH_VARARGS, (const char *)gPyGetAppliedImpulse__doc__},

	{"exportBulletFile", (PyCFunction)gPyExportBulletFile,
	 METH_VARARGS, "export a .bullet file"},

	//sentinel
	{ nullptr, (PyCFunction)nullptr, 0, nullptr }
};

static struct PyModuleDef PhysicsConstraints_module_def = {
	PyModuleDef_HEAD_INIT,
	"GamePhysics",  /* m_name */
	PhysicsConstraints_module_documentation,  /* m_doc */
	0,  /* m_size */
	physicsconstraints_methods,  /* m_methods */
	0,  /* m_reload */
	0,  /* m_traverse */
	0,  /* m_clear */
	0,  /* m_free */
};

PyMODINIT_FUNC initConstraintPythonBinding()
{

	PyObject *ErrorObject;
	PyObject *m;
	PyObject *d;
	PyObject *item;

	m = PyModule_Create(&PhysicsConstraints_module_def);
	PyDict_SetItemString(PySys_GetObject("modules"), PhysicsConstraints_module_def.m_name, m);

	// Add some symbolic constants to the module
	d = PyModule_GetDict(m);
	ErrorObject = PyUnicode_FromString("PhysicsConstraints.error");
	PyDict_SetItemString(d, "error", ErrorObject);
	Py_DECREF(ErrorObject);

#ifdef WITH_BULLET
	//Debug Modes constants to be used with setDebugMode() python function
	KX_MACRO_addTypesToDict(d, DBG_NODEBUG, btIDebugDraw::DBG_NoDebug);
	KX_MACRO_addTypesToDict(d, DBG_DRAWWIREFRAME, btIDebugDraw::DBG_DrawWireframe);
	KX_MACRO_addTypesToDict(d, DBG_DRAWAABB, btIDebugDraw::DBG_DrawAabb);
	KX_MACRO_addTypesToDict(d, DBG_DRAWFREATURESTEXT, btIDebugDraw::DBG_DrawFeaturesText);
	KX_MACRO_addTypesToDict(d, DBG_DRAWCONTACTPOINTS, btIDebugDraw::DBG_DrawContactPoints);
	KX_MACRO_addTypesToDict(d, DBG_NOHELPTEXT, btIDebugDraw::DBG_NoHelpText);
	KX_MACRO_addTypesToDict(d, DBG_DRAWTEXT, btIDebugDraw::DBG_DrawText);
	KX_MACRO_addTypesToDict(d, DBG_PROFILETIMINGS, btIDebugDraw::DBG_ProfileTimings);
	KX_MACRO_addTypesToDict(d, DBG_ENABLESATCOMPARISION, btIDebugDraw::DBG_EnableSatComparison);
	KX_MACRO_addTypesToDict(d, DBG_DISABLEBULLETLCP, btIDebugDraw::DBG_DisableBulletLCP);
	KX_MACRO_addTypesToDict(d, DBG_ENABLECCD, btIDebugDraw::DBG_EnableCCD);
	KX_MACRO_addTypesToDict(d, DBG_DRAWCONSTRAINTS, btIDebugDraw::DBG_DrawConstraints);
	KX_MACRO_addTypesToDict(d, DBG_DRAWCONSTRAINTLIMITS, btIDebugDraw::DBG_DrawConstraintLimits);
	KX_MACRO_addTypesToDict(d, DBG_FASTWIREFRAME, btIDebugDraw::DBG_FastWireframe);
#endif // WITH_BULLET

	//Constraint types to be used with createConstraint() python function
	KX_MACRO_addTypesToDict(d, POINTTOPOINT_CONSTRAINT, PHY_POINT2POINT_CONSTRAINT);
	KX_MACRO_addTypesToDict(d, LINEHINGE_CONSTRAINT, PHY_LINEHINGE_CONSTRAINT);
	KX_MACRO_addTypesToDict(d, ANGULAR_CONSTRAINT, PHY_ANGULAR_CONSTRAINT);
	KX_MACRO_addTypesToDict(d, CONETWIST_CONSTRAINT, PHY_CONE_TWIST_CONSTRAINT);
	KX_MACRO_addTypesToDict(d, VEHICLE_CONSTRAINT, PHY_VEHICLE_CONSTRAINT);
	KX_MACRO_addTypesToDict(d, GENERIC_6DOF_CONSTRAINT, PHY_GENERIC_6DOF_CONSTRAINT);

	// Check for errors
	if (PyErr_Occurred()) {
		Py_FatalError("can't initialize module PhysicsConstraints");
	}

	return m;
}

#endif // WITH_PYTHON

