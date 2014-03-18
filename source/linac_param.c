#include "linac_param.h"

#include <math.h>

//#ifndef M_PI
//#define M_PI 3.14159265358979323846264338327
//#endif

void Linac_Config(Linac_Param * linp,
		  // General (nonphysical) simulation paramters
		  double dt,
		  // Linac paramters
		  double dE, double R56, double T566, double phi,
		  double lam, double s0, double a, double L,
		  // Double compress parameters,
		  //double Eg,
		  // Properties of the triode system
		  double saturate_c,
		  double complex * p_TRF1, double complex * p_TRF2,
		  // Properties of the RX Filter
		  double complex * p_RXF,
		  //Properties of the cavity 
		  int n_cav, double nom_grad,
		  double psd_llrf, double w0, double bunch_rep,
		  double Q_L, double R_Q,
		  double beta_in, double beta_out, double beta_beam,
		  double kly_max_v, double stable_gbw
		  )
{
  int i;
  /*
   * Assign the scalar variables to the fields of the structure
   */
  linp->dE = dE;
  linp->R56 = R56;
  linp->T566 = T566;
  linp->phi = phi;
  linp->lam = lam;
  linp->s0 = s0;
  linp->a = a;
  linp->L = L;

  linp->n_cav = n_cav;
  linp->nom_grad = nom_grad;
  linp->saturate_c = saturate_c;

  /*
   * Configure the Filters using their poles
   */
  Filter_Allocate_In(&linp->TRF1,2,2);
  for(i=0;i<2;i++) {
    Filter_Append_Modes(&linp->TRF1, p_TRF1+i, 1,dt);
  }

  Filter_Allocate_In(&linp->TRF2,1,1);
  Filter_Append_Modes(&linp->TRF2, p_TRF2, 1, dt);

  Filter_Allocate_In(&linp->RXF,3,3);
  for(i=0;i<3;i++) {
    Filter_Append_Modes(&linp->RXF,p_RXF+i,1,dt);
    //printf("%f+i%f\n",creal(p_RXF[i]),cimag(p_RXF[i]));
  }

  /*
   * Configure the Cavity
   */
  Cavity_Config(&linp->cav,
		dt,n_cav,
		psd_llrf,w0,bunch_rep,Q_L,R_Q,
		beta_in,beta_out,beta_beam);
  //double dE = (E - Eg)*1.0e9;

  linp->cav.nom_beam_phase = linp->phi;
  linp->cav.rf_phase = 0.0;
  linp->cav.design_voltage = dE / fabs(cos(linp->cav.nom_beam_phase));
  linp->cav.unity_voltage = n_cav*kly_max_v;
  linp->cav.k = linp->cav.k/kly_max_v;

  double complex cav_p = -0.5*w0/Q_L;
  Filter_Allocate_In(&linp->Cav_Fil,1,1);
  Filter_Append_Modes(&linp->Cav_Fil,&cav_p,1,dt);

  /*
   * Configure the FPGA
   */

  double cav_open_loop_bw = w0 / (2.0*M_PI * 2.0*Q_L);
  double kp =  -stable_gbw/cav_open_loop_bw;
  double complex set_point = linp->cav.design_voltage*cexp(I*linp->cav.rf_phase)
                         / linp->cav.unity_voltage;

  FPGA_Config(&linp->fpga,kp,kp*0.1,
	      set_point);


}


void Cavity_Config(Cavity * cav,
		   double dt,int n_cav,
		   double psd_llrf, double w0, double bunch_rep,
		   double Q_L, double R_Q,
		   double beta_in, double beta_out, double beta_beam)
{
  /*
   * Input paramters
   */
  cav->psd_llrf = psd_llrf;
  cav->w0 = w0;
  cav->bunch_rep = bunch_rep;
  cav->Q_L = Q_L;
  cav->R_Q = R_Q;
  cav->beta_in = beta_in;
  cav->beta_out = beta_out;
  cav->beta_beam = beta_beam;
  /*
   * Calculated quantities
   */
  cav->bandw = 0.5/dt;
  cav->noise_rms = 1.5*sqrt( 0.5*psd_llrf*cav->bandw / (double)n_cav );
  cav->bw_ol = w0 / Q_L;

  cav->k = bunch_rep * R_Q * Q_L;
}

void FPGA_Config(FPGA_Param * fpga,
		 double kp, double ki, double complex set_point)
{

  fpga->gain = kp;
  if(kp==0) fpga->int_gain=0.0;
  else fpga->int_gain = ki/kp; //TODO: This is stupid?
  fpga->set_point = set_point;
}

void Gun_Config(Gun_Param * gun, 
		 double E, double sz0, double sd0, double Q)
{
  /*
   * Input paramters
   */
  gun->E = E;
  gun->sz0 = sz0;
  gun->sd0 = sd0;
  gun->Q = Q;
}


void Linac_Deallocate(Linac_Param * linp) {
  Filter_Deallocate(&linp->RXF);
  Filter_Deallocate(&linp->TRF1);
  Filter_Deallocate(&linp->TRF2);
  Filter_Deallocate(&linp->Cav_Fil);
}