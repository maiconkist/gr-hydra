/* -*- c++ -*- */

#define HYDRA_API

%include "gnuradio.i" // the common stuff

//load generated python docstrings
%include "hydra_swig_doc.i"

%{
#include "hydra/hydra_gr_server.h"
#include "hydra/hydra_gr_client_sink.h"
#include "hydra/hydra_gr_client_source.h"

#include "../../include/hydra//hydra_client.h"
%}

%include "hydra/hydra_gr_client_sink.h"
GR_SWIG_BLOCK_MAGIC2(hydra, hydra_gr_client_sink);

%include "hydra/hydra_gr_client_source.h"
GR_SWIG_BLOCK_MAGIC2(hydra, hydra_gr_client_source);

%include "hydra/hydra_gr_server.h"
GR_SWIG_BLOCK_MAGIC2(hydra, hydra_gr_server);
