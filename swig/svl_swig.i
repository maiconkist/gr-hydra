/* -*- c++ -*- */

#define SVL_API

%include "gnuradio.i"			// the common stuff

//load generated python docstrings
%include "svl_swig_doc.i"

%{
#include "svl/svl-sink.h"
%}


%include "svl/svl-sink.h"
GR_SWIG_BLOCK_MAGIC2(svl, svl_sink);
