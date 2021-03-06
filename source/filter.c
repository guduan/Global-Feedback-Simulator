/**
 * @file filter.c
 * @brief Filter Model: Allocation, configuration and stepping functions for the filter model, which iterates over an arbitrary number of modes.
 * @author Carlos Serrano (Cserrano@lbl.gov)
 */

#include "filter.h"
#include "stdlib.h"


Filter * Filter_Allocate_New(
  int alloc_order,  ///< Number of modes
  int alloc_coeffs  ///< Number of coefficients per mode
  )
{
  Filter * fil;
  fil = calloc(1,sizeof(Filter));

  fil->alloc_order = alloc_order;
  fil->alloc_coeffs = alloc_coeffs;

  fil->n_coeffs=0;
  fil->order = 0;

  fil->modes = (int*)calloc(alloc_order,sizeof(int));
  fil->coeff_start = (int*)calloc(alloc_order,sizeof(int));
  fil->coeffs = (double complex *)calloc(3*alloc_coeffs,sizeof(double complex));
  fil->poles = (double complex *)calloc(alloc_coeffs,sizeof(double complex));
  return fil;
}

/** Takes a pointer to a Filter struct and fills allocates a filter of the depth indicated by the argument list. */
void Filter_Allocate_In(
  Filter * fil,     ///< Pointer to Filter struct
  int alloc_order,  ///< Number of modes
  int alloc_coeffs  ///< Number of coefficients per mode
  )
{

  fil->alloc_order = alloc_order;
  fil->alloc_coeffs = alloc_coeffs;

  fil->n_coeffs=0;
  fil->order = 0;

  fil->modes = (int*)calloc(alloc_order,sizeof(int));
  fil->coeff_start = (int*)calloc(alloc_order,sizeof(int));

  fil->coeffs = (double complex *)calloc(3*alloc_coeffs,sizeof(double complex));
  fil->poles = (double complex *)calloc(alloc_coeffs,sizeof(double complex));
}

/** Frees memory of a Filter struct */
void Filter_Deallocate(Filter * fil)
{
  free(fil->poles);
  free(fil->coeffs);
  free(fil->coeff_start);
  free(fil->modes);
  fil->order = 0;
  fil->n_coeffs = 0;
  fil->alloc_order = 0;
  fil->alloc_coeffs = 0;
}

/** Append one or more Filter modes to a Filter previously allocated.
 * given a list of complex poles in the argument list. */
void Filter_Append_Modes(
  Filter * fil,             ///< Pointer to Filter struct
  double complex * poles,   ///< Array of complex poles
  int mod,                  ///< Number of modes to be appended to Filter
  double dt                 ///< Simulation time step in seconds
  )
{
  int i;
  /*
   * Append to the data structures
   */
  // Increment order of the filter and reallocate needed arrays
  fil->order++;
  if(fil->order >= fil->alloc_order) {
    fil->modes = realloc(fil->modes,fil->order*sizeof(int));
    fil->alloc_order = fil->order;
    fil->coeff_start = realloc(fil->coeff_start,fil->order*sizeof(int));
  }
  // Update the indexing arrays for the new entry
  fil->modes[fil->order-1] = mod;
  if(fil->order>1) {
    fil->coeff_start[fil->order-1] =
      fil->coeff_start[fil->order-2]+fil->modes[fil->order-2];
  }
  // Allocate more coefficients
  fil->n_coeffs += mod;
  if(fil->n_coeffs >= fil->alloc_coeffs) {
    fil->coeffs = realloc(fil->coeffs, fil->n_coeffs* 3*sizeof(double complex));
    fil->poles = realloc(fil->poles, fil->n_coeffs *sizeof(double complex));

    fil->alloc_coeffs = fil->n_coeffs;
  }

  /*
   * Calculate new coefficients
   */
  for(i=0; i<mod; i++) {
    int cs = fil->coeff_start[fil->order-1]+i;
    fil->poles[cs]=poles[i];
    fil->coeffs[3*cs+0] = (1.0 + 0.5*dt*poles[i])
                        /(1.0 - 0.5*dt*poles[i]);
    fil->coeffs[3*cs+1] =     dt
                        /(1.0-0.5*dt*poles[i]);
    fil->coeffs[3*cs+2] = cabs(poles[i]);
  }

}

/** Takes a previously configured Filter and allocates its State struct accordingly. */
void Filter_State_Allocate(Filter_State * sf, Filter * fil) {
  sf->state = calloc(fil->n_coeffs,sizeof(double complex));
  sf->input = calloc(fil->order,sizeof(double complex));
}

/** Frees memory of Filter State struct. */
void Filter_State_Deallocate(Filter_State * sf) {
  free(sf->state);
  free(sf->input);
}

/** Helper routine to zero out Filter state. Useful to restore initial state in unit tests. */
void Filter_State_Clear(Filter * fil, Filter_State * sf)
{
  int o, m, cs;
  for(o=0;o<fil->order;o++){
    sf->input[o] = 0.0+0.0*_Complex_I;
    for(m=0;m<fil->modes[o];m++) {
      // Current state index
      cs = fil->coeff_start[o]+m;
      sf->state[cs] = 0.0+0.0*_Complex_I;
    }
  }
}

/** Step function for Filter model:
  * Calculates the state for the next simulation step.
  * Returns the output of the last cascaded pole. */
double complex Filter_Step(
  Filter * fil,               ///< Pointer to Filter struct
  double complex innow,       ///< Complex input
  Filter_State * fil_state    ///< Pointer to Filter State
  )
{
  // Indeces
  int o,m,cs;
  // Intermediate input signals
  double complex voltage_in, prev_in;
  // ODE coefficients
  double complex a,b,scale;
  // Signal connecting cascading poles (start with current input)
  double complex output = innow;

  // Iterate over poles
  for(o=0;o<fil->order;o++) {
    // Previous input
    prev_in = fil_state->input[o];
    // Store input for next step
    fil_state->input[o] = output;
    voltage_in = 0.5*(fil_state->input[o] + prev_in);

    // Clear output of current pole
    output = 0.0;

    // Iterate over modes
    for(m=0;m<fil->modes[o];m++) {
      // Current state index
      cs = fil->coeff_start[o]+m;
      // Grab a, b and scale coefficients for current mode
      a = fil->coeffs[3*cs+0];
      b = fil->coeffs[3*cs+1];
      scale = fil->coeffs[3*cs+2];

      // Store output for current mode
      fil_state->state[cs] = a*fil_state->state[cs]+b*voltage_in;

      // Add mode's output to form pole's output
      // (and scale to keep unity gain at DC)
      output += fil_state->state[cs]*scale;

    } // End mode iteration

  } // End pole iteration


  // Filter output is the output of the last cascaded pole
  return output;
}

void Filter_Set_State(Filter * fil, Filter_State * fil_state, double complex state)
{
  fil_state->state[0] = state/fil->coeffs[2];
}
