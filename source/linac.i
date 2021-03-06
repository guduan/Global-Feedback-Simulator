%module linac


%{
#include "doublecompress.h"
#include "filter.h"
#include "linac_param.h"
#include "step_llrf.h"
#include "state_space_top.h"
#include "beam_based_feedback.h"
#include "dynamic_noise.h"
#include "cavity.h"
%}

%include "complex.i"
%include "carrays.i"
%array_class(int, intArray);
%array_class(double complex, complexdouble_Array);
%array_class(double, double_Array);
%array_class(LINP, Linac_Param_Array);
%array_class(LINS, Linac_State_Array);

%include "cpointer.i"
%pointer_class(int, intp);
%pointer_class(double complex, compp);
%pointer_class(LINP, LINPp);
%pointer_class(LINS, LINSp);

%include "doublecompress.h"
%include "filter.h"
%include "linac_param.h"
%include "step_llrf.h"
%include "state_space_top.h"
%include "beam_based_feedback.h"
%include "dynamic_noise.h"
%include "cavity.h"
