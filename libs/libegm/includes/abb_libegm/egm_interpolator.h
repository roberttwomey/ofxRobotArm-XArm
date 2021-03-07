/***********************************************************************************************************************
 *
 * Copyright (c) 2015, ABB Schweiz AG
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that
 * the following conditions are met:
 *
 *    * Redistributions of source code must retain the
 *      above copyright notice, this list of conditions
 *      and the following disclaimer.
 *    * Redistributions in binary form must reproduce the
 *      above copyright notice, this list of conditions
 *      and the following disclaimer in the documentation
 *      and/or other materials provided with the
 *      distribution.
 *    * Neither the name of ABB nor the names of its
 *      contributors may be used to endorse or promote
 *      products derived from this software without
 *      specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ***********************************************************************************************************************
 */

#ifndef EGM_INTERPOLATOR_H
#define EGM_INTERPOLATOR_H

#include <boost/array.hpp>

#include "abb_libegm_export.h"

#include "egm_wrapper_trajectory.pb.h" // Generated by Google Protocol Buffer compiler protoc

#include "egm_common.h"

namespace abb
{
namespace egm
{
/**
 * \brief Class for managing interpolations.
 *
 * The used approaches, depending on the conditions, are:
 * - 5th (or lower) degree spline polynomials.
 * - Slerp interpolation.
 * - Ramping in or ramping down values.
 *
 * Warning: No kinematics are considered. I.e. joint limits can be exceeded (which can be a problem).
 */
class EGMInterpolator
{
public:
  /**
   * \brief Enum for the operations the interpolator can handle.
   */
  enum Operation
  {
    Normal,         ///< Normal operation, i.e. use spline and Slerp interpolation.
    RampDown,       ///< Ramp down operation, i.e. use special spline and orientation interpolation.
    RampInPosition, ///< Ramp in position operation, i.e. used for static goals.
    RampInVelocity  ///< Ramp in velocity operation, i.e. used for static goals.
  };

  /**
   * \brief Struct for containing conditions for the interpolator.
   */
  struct Conditions
  {
    /**
     * \brief Default constructor.
     */
    Conditions()
    :
    duration(0.0),
    mode(EGMJoint),
    operation(Normal),
    ramp_down_factor(0.0),
    spline_method(TrajectoryConfiguration::Quintic)
    {}

    /**
     * \brief Duration [s] of the interpolation session.
     */
    double duration;

    /**
     * \brief The active EGM mode.
     */
    EGMModes mode;

    /**
     * \brief The requested interpolation operation.
     */
    Operation operation;

    /**
     * \brief Value specifying the factor, of the current velocity, to use as end velocity for ramp down calculations.
     *
     * Note: The value should be between 0.0 and 1.0.
     */
    double ramp_down_factor;

    /**
     * \brief The spline method to use for normal operation.
     */
    TrajectoryConfiguration::SplineMethod spline_method;
  };

  /**
   * \brief Update the interpolator for upcoming calculations. E.g. used after a new goal has been chosen.
   *
   * \param start containing the start point.
   * \param goal containing the goal point.
   * \param conditions for specifying conditions for the interpolator.
   */
  void update(const wrapper::trajectory::PointGoal& start,
              const wrapper::trajectory::PointGoal& goal,
              const Conditions& conditions);

  /**
   * \brief Evaluate the interpolator at a specific time instance.
   *
   * \param p_output for storing the evaluated output.
   * \param sample_time specifying the used sample time [s].
   * \param t for the time instance [s] that the interpolation should be calculated at.
   */
  void evaluate(wrapper::trajectory::PointGoal* p_output, const double sample_time, double t);

  /**
   * \brief Retrive the valid duration [s] for the current interpolation session.
   *
   * \return double containing the duration.
   */
  double getDuration()
  {
    return conditions_.duration;
  }

private:
  /**
   * \brief Enum for specifying which Cartesian axis to consider in the spline polynomials.
   */
  enum Axis
  {
    X, ///< Cartesian x axis.
    Y, ///< Cartesian y axis.
    Z  ///< Cartesian z axis.
  };

  /**
   * \brief Class for containing conditions for a spline polynomial.
   */
  class SplineConditions
  {
  public:
    /**
     * \brief A constructor.
     *
     * \param conditions specifying the general conditions for the interpolator.
     */
    SplineConditions(const Conditions conditions)
    :
    duration(conditions.duration),
    alfa(0.0),
    d_alfa(0.0),
    dd_alfa(0.0),
    beta(0.0),
    d_beta(0.0),
    dd_beta(0.0),
    spline_method(conditions.spline_method),
    do_ramp_down(conditions.operation == RampDown),
    ramp_down_factor(conditions.ramp_down_factor)
    {}

    /**
     * \brief Extract and set the boundary conditions for joint interpolation.
     *
     * \param index of the joint.
     * \param start containing the start values.
     * \param goal containing the goal values.
     */
    void setConditions(int index,
                       const wrapper::trajectory::JointGoal& start,
                       const wrapper::trajectory::JointGoal& goal);

    /**
     * \brief Extract and set the boundary conditions for Cartesian interpolation.
     *
     * \param axis specifying the axis to consider.
     * \param start containing the start values.
     * \param goal containing the goal values.
     */
    void setConditions(const Axis axis,
                       const wrapper::trajectory::CartesianGoal& start,
                       const wrapper::trajectory::CartesianGoal& goal);

    /**
     * \brief Duration of the interpolation.
     */
    double duration;

    /**
     * \brief The start position.
     */
    double alfa;

    /**
     * \brief The start velocity.
     */
    double d_alfa;

    /**
     * \brief The start acceleration.
     */
    double dd_alfa;

    /**
     * \param The goal position.
     */
    double beta;

    /**
     * \param The goal velocity.
     */
    double d_beta;

    /**
     * \param The goal acceleration.
     */
    double dd_beta;

    /**
     * \brief Specifies which spline method to use.
     */
    TrajectoryConfiguration::SplineMethod spline_method;

    /**
     * \brief Flag indicating if ramp down interpolation should be performed or not.
     */
    bool do_ramp_down;

    /**
     * \brief Value specifying the factor, of the current velocity, to use as end velocity for ramp down calculations.
     *
     * Note: The value should be between 0.0 and 1.0.
     */
    double ramp_down_factor;
  };

  /**
   * \brief Class for a spline interpolation polynomial of degree 5 or lower.
   *
   * I.e. A + B*t + C*t^2 + D*t^3 + E*t^4 + F*t^5.
   */
  class SplinePolynomial
  {
  public:
    /**
     * \brief Default constructor.
     */
    SplinePolynomial() : a_(0.0), b_(0.0), c_(0.0), d_(0.0), e_(0.0), f_(0.0) {}

    /**
     * \brief Update the polynomial's coefficients.
     *
     * \param conditions containing the spline's conditions.
     */
    void update(const SplineConditions& conditions);

    /**
     * \brief Evaluate the polynomial, for robot or external joints values.
     *
     * \param p_output for storing the evaluated values.
     * \param index of the joint.
     * \param t for the time instance [s] to evaluate at.
     */
    void evaluate(wrapper::trajectory::JointGoal* p_output, const int index, const double t);

    /**
     * \brief Evaluate the polynomial, for Cartesian values.
     *
     * \param p_output for storing the evaluated values.
     * \param axis specifying the axis to consider.
     * \param t for the time instance [s] to evaluate at.
     */
    void evaluate(wrapper::trajectory::CartesianGoal* p_output, const Axis axis, const double t);

  private:
    /**
     * \brief Calculate the position.
     *
     * \param t for the time instance [s] to calculate at.
     *
     * \return double containing the calculated position.
     */
    double calculatePosition(const double t)
    {
      return a_ + b_*t + c_*std::pow(t, 2) + d_*std::pow(t, 3) + e_*std::pow(t, 4) + f_*std::pow(t, 5);
    }

    /**
     * \brief Calculate the velocity.
     *
     * \param t for the time instance [s] to calculate at.
     *
     * \return double containing the calculated velocity.
     */
    double calculateVelocity(const double t)
    {
      return b_ + 2.0*c_*t + 3.0*d_*std::pow(t, 2) + 4.0*e_*std::pow(t, 3) + 5.0*f_*std::pow(t, 4);
    }

    /**
     * \brief Calculate the acceleration.
     *
     * \param t for the time instance [s] to calculate at.
     *
     * \return double containing the calculated acceleration.
     */
    double calculateAcceleration(const double t)
    {
      return 2.0*c_ + 6.0*d_*t + 12.0*e_*std::pow(t, 2) + 20.0*f_*std::pow(t, 3);
    }

    /**
     * \brief Coefficient A.
     */
    double a_;

    /**
     * \brief Coefficient B.
     */
    double b_;

    /**
     * \brief Coefficient C.
     */
    double c_;

    /**
     * \brief Coefficient D.
     */
    double d_;

    /**
     * \brief Coefficient E.
     */
    double e_;

    /**
     * \brief Coefficient F.
     */
    double f_;
  };

  /**
   * \brief Class for Slerp (Spherical linear interpolation) for quaternions.
   *        Slerp with unit quaternions produce a rotation with uniform angular speed.
   *
   * I.e. Slerp(q0, q1; t) = [sin((1-t)*omega)/sin(omega)]*q0 + [sin(t*omega)/sin(omega)]*q1.
   *      Where q0 and q1 are quaternions and cos(omega) = q0*q1 (dot product).
   *
   * Note: 0 <= t <= 1.
   *
   * See for example https://en.wikipedia.org/wiki/Slerp for the equations.
   */
  class Slerp
  {
  public:
    /**
     * \brief Default constructor.
     */
    Slerp()
    :
    DOT_PRODUCT_THRESHOLD(0.9995),
    duration_(0.0),
    omega_(0.0),
    use_linear_(false)
    {
      q0_.set_u0(1.0);
      q1_.set_u0(1.0);
    }

    /**
     * \brief Update the Slerp's coefficient.
     *
     * \param start containing the start quaternion.
     * \param goal containing the goal quaternion.
     * \param conditions containing the interpolator's conditions.
     */
    void update(const wrapper::Quaternion& start,
                const wrapper::Quaternion& goal,
                const Conditions& conditions);

    /**
     * \brief Evaluate the Slerp.
     *
     * \param p_output for storing the evaluated values.
     * \param t for the time instance [s] that the interpolation should be calculated at.
     */
    void evaluate(wrapper::trajectory::CartesianGoal* p_output, double t);

  private:
    /**
     * \brief Threshold for the dot product, used to decide if linear interpolation should be used.
     *
     * Note: That is if the quaternions are too close to each other.
     */
    const double DOT_PRODUCT_THRESHOLD;

    /**
     * \brief Duration [s] of the interpolation session.
     */
    double duration_;

    /**
     * \brief Coefficient omega.
     */
    double omega_;

    /**
     * \brief Start quaternion.
     */
    wrapper::Quaternion q0_;

    /**
     * \brief Goal quaternion.
     */
    wrapper::Quaternion q1_;

    /**
     * \brief Flag indicating if linear interpolation should be used or not.
     */
    bool use_linear_;
  };

  /**
   * \brief Class for ramping in positions or velocities. As well as ramping down angular velocities.
   *
   * The ramp factor is according to:
   * - Ramping down angular velocity  : 0.5*cos(pi*t) + 0.5      | I.e. from 1 -> 0.
   * - Ramping in position or velocity: 0.5*cos(pi*t + pi) + 0.5 | I.e. from 0 -> 1.
   *
   * Note: 0 <= t <= 1.
   */
  class SoftRamp
  {
  public:
    /**
     * \brief Default constructor.
     */
    SoftRamp() : duration_(0.0), operation_(RampDown) {}

    /**
     * \brief Update the ramp's internal data fields.
     *
     * \param start containing the start point.
     * \param goal containing the goal point.
     * \param conditions containing the interpolator's conditions.
     */
    void update(const wrapper::trajectory::PointGoal& start,
                const wrapper::trajectory::PointGoal& goal,
                const Conditions& conditions);

    /**
     * \brief Evaluate the ramp, for robot or external joints values.
     *
     * \param p_output for storing the evaluated values.
     * \param robot indicating if it is robot joints (otherwise external joints).
     * \param sample_time for the used sample time [s].
     * \param t for the time instance [s] to evaluate at.
     */
    void evaluate(wrapper::trajectory::JointGoal* p_output,
                  const bool robot,
                  const double sample_time,
                  double t);

    /**
     * \brief Evaluate the ramp, for Cartesian values.
     *
     * \param p_output for storing the evaluated values.
     * \param sample_time for the used sample time [s].
     * \param t for the time instance [s] to evaluate at.
     */
    void evaluate(wrapper::trajectory::CartesianGoal* p_output, const double sample_time, double t);

  private:
    /**
     * \brief Duration [s] of the interpolation session.
     */
    double duration_;

    /**
     * \brief The requested interpolation operation.
     */
    Operation operation_;

    /**
     * \brief A container for the start point.
     */
    wrapper::trajectory::PointGoal start_;

    /**
     * \brief A container for the starting angular velocity values.
     */
    wrapper::Euler start_angular_velocity_;

    /**
     * \brief A container for the goal point.
     */
    wrapper::trajectory::PointGoal goal_;
  };

  /**
   * \brief Static constant for the max number of spline polynomials.
   */
  static const size_t MAX_NUMBER_OF_SPLINES_ = 12;

  /**
   * \brief Offset in the spline polynomial array, to the external joint elements.
   */
  int offset_;

  /**
   * \brief Container for the spline interpolation polynomials.
   */
  boost::array<SplinePolynomial, MAX_NUMBER_OF_SPLINES_> spline_polynomials_;

  /**
   * \brief Container for the Slerp (for interpolating quaterions).
   */
  Slerp slerp_;

  /**
   * \brief Container for ramping in positions or velocities. As well as ramping down angular velocities.
   */
  SoftRamp soft_ramp_;

  /**
   * \brief The interpolator's conditions.
   */
  Conditions conditions_;
};

} // end namespace egm
} // end namespace abb

#endif // EGM_INTERPOLATOR_H
