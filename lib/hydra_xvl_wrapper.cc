#include "hydra/hydra_xvl_wrapper.h"

namespace hydra {

HydraXVL* HydraXVL::pinstance;

HydraXVL::HydraXVL(size_t _fft_m_len,
                   double central_frequency, double bandwidth):
   hypervisor(_fft_m_len, central_frequency, bandwidth),
   running(true)
{
   thread = std::make_unique<std::thread>(&HydraXVL::run, this);
}

void
HydraXVL::create_instance(size_t _fft_m_len,
                          double central_frequency,
                          double bandwidth)
{
   assert(!pinstance);
   pinstance = new HydraXVL(_fft_m_len, central_frequency, bandwidth);
}

HydraXVL *
HydraXVL::get_instance()
{
   assert(pinstance);
   return pinstance;
}

gr::hydra::VirtualRadioPtr
HydraXVL::get_virtual_radio(size_t id)
{
   return get_instance()->hypervisor.get_vradio(id);
}


HydraXVL::~HydraXVL()
{
   running = false;
   thread->join();
}

void
HydraXVL::run()
{
   // total
   size_t l_threshold = llrint(hypervisor.get_tx_fft() * 1e9 / hypervisor.get_tx_bandwidth());

   gr_complex optr[4000];

   while (running)
   {
      // Wait for "threshold" nanoseconds
      std::this_thread::sleep_for(std::chrono::nanoseconds(l_threshold));
      hypervisor.get_tx_window(&optr[0], 40000);
   }
}

size_t
HydraXVL::create_virtual_radio(double cf, double bandwidth)
{
   return get_instance()->hypervisor.create_vradio(cf, bandwidth);
}


} // namespace hydra
