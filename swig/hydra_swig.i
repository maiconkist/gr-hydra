/* -*- c++ -*- */

#define SVL_API

%include "gnuradio.i" // the common stuff

//load generated python docstrings
%include "hydra_swig_doc.i"

%{
#include "hydra/types.h"
#include "hydra/hydra_virtual_radio.h"
#include "hydra/hydra_hypervisor.h"
#include "hydra/hydra_block.h"
#include "hydra/hydra_sink.h"
#include "hydra/hydra_source.h"
%}

%include "hydra/types.h"
%include "hydra/hydra_virtual_radio.h"
%include "hydra/hydra_hypervisor.h"
%include "hydra/hydra_block.h"
%include "hydra/hydra_sink.h"
GR_SWIG_BLOCK_MAGIC2(hydra, hydra_sink);

%include "hydra/hydra_source.h"
GR_SWIG_BLOCK_MAGIC2(hydra, hydra_source);
