/* -*- c++ -*- */

#define SVL_API

%include "gnuradio.i"			// the common stuff

//load generated python docstrings
%include "svl_swig_doc.i"

%{
#include "svl/svl_sink.h"
#include "svl/svl_source.h"
#include "svl/svl_block.h"
#include "svl/svl_fft.h"
#include "svl/svl_virtual_radio.h"
#include "svl/svl_hypervisor.h"
%}


%include "svl/svl_sink.h"
GR_SWIG_BLOCK_MAGIC2(svl, svl_sink);
%include "svl/svl_source.h"
GR_SWIG_BLOCK_MAGIC2(svl, svl_source);
%include "svl/svl_block.h"
GR_SWIG_BLOCK_MAGIC2(svl, svl_block);
%include "svl/svl_fft.h"
GR_SWIG_BLOCK_MAGIC2(svl, fft_complex);
%include "svl/svl_virtual_radio.h"
GR_SWIG_BLOCK_MAGIC2(svl, VirtualRadio);
%include "svl/svl_hypervisor.h"
GR_SWIG_BLOCK_MAGIC2(svl, Hypervisor);
