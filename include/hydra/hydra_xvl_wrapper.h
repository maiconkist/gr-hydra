#ifndef INCLUDED_HYDRA_XVL_WRAPPER
#define INCLUDED_HYDRA_XVL_WRAPPER

#include <thread>

#include "hydra/hydra_hypervisor.h"


namespace hydra {

class HydraXVL
{
public:
   static void create_instance(size_t _fft_m_len, double central_frequency, double bandwidth);
   static HydraXVL *get_instance();

   static size_t create_virtual_radio(double cf, double bandwidth);
   static gr::hydra::VirtualRadioPtr get_virtual_radio(size_t id);
   void run();

private:
   static HydraXVL *pinstance;

   HydraXVL(size_t _fft_m_len, double central_frequency, double bandwidth);
   ~HydraXVL();

   gr::hydra::Hypervisor hypervisor;
   std::unique_ptr<std::thread> thread;

   bool running;
};

} // namespace hydra

#endif
