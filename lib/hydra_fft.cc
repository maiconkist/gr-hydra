#include "hydra/hydra_fft.h"

namespace gr {
   namespace hydra {

fft_complex::fft_complex(size_t fft_size, bool forward):
   g_fft_size(fft_size),
   g_forward(forward)
{
   g_inbuf  = (gr_complex *) fftw_malloc(sizeof(fftw_complex) * fft_size) ;
   g_outbuf = (gr_complex *) fftw_malloc(sizeof(fftw_complex) * fft_size) ;

   g_plan = fftwf_plan_dft_1d(fft_size,
                   (fftwf_complex *)(g_inbuf),
                   (fftwf_complex *)(g_outbuf),
                   forward? FFTW_FORWARD:FFTW_BACKWARD,
                   FFTW_MEASURE);
}

int
fft_complex::set_data(const gr_complex *data, size_t len)
{
   // Copy samples to fft buffer
   std::copy(data, data + len, g_inbuf);
}

gr_complex*
fft_complex::get_inbuf()
{
   return g_inbuf;
}

gr_complex*
fft_complex::get_outbuf()
{
   return g_outbuf;
}

void
fft_complex::execute()
{
   if (g_forward == false)
   {
      std::rotate(g_inbuf,
            g_inbuf + g_fft_size/2,
            g_inbuf + g_fft_size);
   }

   fftwf_execute(g_plan);

   if (g_forward == true)
   {
      std::rotate(g_outbuf,
            g_outbuf + g_fft_size/2,
            g_outbuf + g_fft_size);

      for (size_t i = 0; i < g_fft_size; ++i)
         g_outbuf[i] /= float(g_fft_size);
   }
}

} /* namespace hydra */
} /* namespace gr */
