/* -*- c++ -*- */

#define HYDRA_API

%include "gnuradio.i" // the common stuff

//load generated python docstrings
%include "hydra_swig_doc.i"

%{
#include "hydra/hydra_gr_server.h"
#include "hydra/hydra_gr_client_sink.h"
#include "hydra/hydra_gr_client_source.h"
#include "hydra/hydra_gr_server_network.h"
#include "hydra/hydra_gr_client_network_sink.h"
#include "hydra/hydra_gr_client_network_source.h"
%}

%include "hydra/hydra_gr_client_sink.h"
GR_SWIG_BLOCK_MAGIC2(hydra, hydra_gr_client_sink);

%include "hydra/hydra_gr_client_source.h"
GR_SWIG_BLOCK_MAGIC2(hydra, hydra_gr_client_source);

%include "hydra/hydra_gr_server.h"
GR_SWIG_BLOCK_MAGIC2(hydra, hydra_gr_server);

%include "hydra/hydra_gr_client_network_sink.h"
GR_SWIG_BLOCK_MAGIC2(hydra, hydra_gr_client_network_sink);
%include "hydra/hydra_gr_client_network_source.h"
GR_SWIG_BLOCK_MAGIC2(hydra, hydra_gr_client_network_source);
%include "hydra/hydra_gr_server_network.h"
GR_SWIG_BLOCK_MAGIC2(hydra, hydra_gr_server_network);
