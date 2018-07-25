/* -*- c++ -*- */

#define HYDRA_API

%include "gnuradio.i" // the common stuff

//load generated python docstrings
%include "hydra_swig_doc.i"

%{
#include "hydra/types.h"
#include "hydra/hydra_fft.h"
#include "hydra/hydra_uhd_interface.h"
#include "hydra/hydra_virtual_radio.h"
#include "hydra/hydra_hypervisor.h"
#include "hydra/hydra_buffer.h"
#include "hydra/hydra_socket.h"
#include "hydra/hydra_stats.h"
#include "hydra/hydra_resource.h"
#include "hydra/hydra_core.h"
#include "hydra/hydra_main.h"
#include "hydra/hydra_server.h"
#include "hydra/hydra_client.h"

#include "hydra/hydra_block.h"
#include "hydra/hydra_source.h"
#include "hydra/hydra_sink.h"
#include "hydra/hydra_async_sink.h"
#include "hydra/hydra_gr_client_sink.h"
#include "hydra/hydra_gr_server.h"
%}

#include "hydra/hydra_core.h"
#include "hydra/hydra_stats.h"
#include "hydra/hydra_server.h"
%include "hydra/hydra_main.h"
%include "hydra/hydra_gr_client_sink.h"
GR_SWIG_BLOCK_MAGIC2(hydra, hydra_gr_client_sink);

%include "hydra/types.h"
%include "hydra/hydra_gr_server.h"
GR_SWIG_BLOCK_MAGIC2(hydra, hydra_gr_server);
