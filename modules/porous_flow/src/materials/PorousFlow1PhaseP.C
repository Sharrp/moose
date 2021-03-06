//* This file is part of the MOOSE framework
//* https://www.mooseframework.org
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#include "PorousFlow1PhaseP.h"
#include "PorousFlowCapillaryPressure.h"

registerMooseObject("PorousFlowApp", PorousFlow1PhaseP);

template <>
InputParameters
validParams<PorousFlow1PhaseP>()
{
  InputParameters params = validParams<PorousFlowVariableBase>();
  params.addRequiredCoupledVar("porepressure",
                               "Variable that represents the porepressure of the single phase");
  params.addRequiredParam<UserObjectName>("capillary_pressure",
                                          "Name of the UserObject defining the capillary pressure");
  params.addClassDescription("This Material is used for the fully saturated single-phase situation "
                             "where porepressure is the primary variable");
  return params;
}

PorousFlow1PhaseP::PorousFlow1PhaseP(const InputParameters & parameters)
  : PorousFlowVariableBase(parameters),

    _porepressure_var(_nodal_material ? coupledDofValues("porepressure")
                                      : coupledValue("porepressure")),
    _gradp_qp_var(coupledGradient("porepressure")),
    _porepressure_varnum(coupled("porepressure")),
    _p_var_num(_dictator.isPorousFlowVariable(_porepressure_varnum)
                   ? _dictator.porousFlowVariableNum(_porepressure_varnum)
                   : 0),
    _pc_uo(getUserObject<PorousFlowCapillaryPressure>("capillary_pressure"))
{
  if (_num_phases != 1)
    mooseError("The Dictator proclaims that the number of phases is ",
               _dictator.numPhases(),
               " whereas PorousFlow1PhaseP can only be used for 1-phase simulations.  Be aware "
               "that the Dictator has noted your mistake.");
}

void
PorousFlow1PhaseP::initQpStatefulProperties()
{
  PorousFlowVariableBase::initQpStatefulProperties();
  buildQpPPSS();
}

void
PorousFlow1PhaseP::computeQpProperties()
{
  // size stuff correctly and prepare the derivative matrices with zeroes
  PorousFlowVariableBase::computeQpProperties();

  buildQpPPSS();
  const Real ds = _pc_uo.dSaturation(_porepressure_var[_qp]);

  if (!_nodal_material)
  {
    (*_gradp_qp)[_qp][0] = _gradp_qp_var[_qp];
    (*_grads_qp)[_qp][0] = ds * _gradp_qp_var[_qp];
  }

  // _porepressure is only dependent on _porepressure, and its derivative is 1
  if (_dictator.isPorousFlowVariable(_porepressure_varnum))
  {
    // _porepressure is a PorousFlow variable
    _dporepressure_dvar[_qp][0][_p_var_num] = 1.0;
    _dsaturation_dvar[_qp][0][_p_var_num] = ds;
    if (!_nodal_material)
    {
      (*_dgradp_qp_dgradv)[_qp][0][_p_var_num] = 1.0;
      (*_dgrads_qp_dgradv)[_qp][0][_p_var_num] = ds;
      (*_dgrads_qp_dv)[_qp][0][_p_var_num] =
          _pc_uo.d2Saturation(_porepressure_var[_qp]) * _gradp_qp_var[_qp];
    }
  }
}

void
PorousFlow1PhaseP::buildQpPPSS()
{
  _porepressure[_qp][0] = _porepressure_var[_qp];
  _saturation[_qp][0] = _pc_uo.saturation(_porepressure_var[_qp]);
}
