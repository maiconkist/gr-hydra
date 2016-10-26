#include "svl/svl_fft.h"

namespace gr {
   namespace svl {

fft_complex::fft_complex(size_t fft_size, bool forward)
{
   g_inbuf  = (gr_complex *) fftw_malloc(sizeof(fftw_complex) * fft_size) ;
   g_outbuf = (gr_complex *) fftw_malloc(sizeof(fftw_complex) * fft_size) ;

   g_plan = fftwf_plan_dft_1d(fft_size,
                   (fftwf_complex *)(g_inbuf),
                   (fftwf_complex *)(g_outbuf),
                   forward? FFTW_FORWARD:FFTW_BACKWARD,
                   FFTW_MEASURE);
}

gr_complex*
fft_complex::get_inbuf(){ return g_inbuf; }

gr_complex*
fft_complex::get_outbuf() { return g_outbuf; }

void
fft_complex::execute() { fftwf_execute(g_plan); }

} /* namespace svl */
} /* namespace gr */
