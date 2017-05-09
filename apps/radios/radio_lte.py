import transmit_path as tp
import receive_path as rp
from uhd_interface import uhd_receiver, uhd_transmitter

def dict2obj(d):
        if isinstance(d, list):
            d = [dict2obj(x) for x in d]
        if not isinstance(d, dict):
            return d

        class C(object):
            pass

        o = C()
        for k in d:
            o.__dict__[k] = dict2obj(d[k])
        return o

def add_options(parser):
    import basicconfig as bc
    defaultconf = bc.RCONF['LTE']

    lte_options = parser.add_option_group("LTE Options")
    lte_options.add_option("", "--lte-bandwidth", type="eng_float", default=1.6e6,
         help="set bandwidth for VR 1 [default=%default]")
    lte_options.add_option("", "--lte-freq", type="eng_float", default=defaultconf['CF'],
         help="set central frequency for VR 1 [default=%default]")
    lte_options.add_option("", "--lte-tx-amplitude", type="eng_float", default=defaultconf['AMPLITUDE'], metavar="AMPL",
         help="set transmitter digital amplitude: 0 <= AMPL < 1.0 [default=%default]")
    lte_options.add_option("", "--lte-file", type="string", default='./lte_file',
         help="set the file to obtain data [default=%default]")
    lte_options.add_option("", "--lte-buffersize", type="intx", default=3072,
         help="set number of bytes to read from buffer size for VR1 [default=%default]")
    lte_options.add_option("-m", "--lte-modulation", type="string", default=defaultconf['MODULATION'],
         help="set modulation type (bpsk, qpsk, 8psk, qam{16,64}) [default=%default]")
    lte_options.add_option("", "--lte-fft-length", type="intx", default=defaultconf['FFT_LENGTH'],
         help="set the number of FFT bins [default=%default]")
    lte_options.add_option("", "--lte-occupied-tones", type="intx", default=defaultconf['OCCUPIED_TONES'],
         help="set the number of occupied FFT bins [default=%default]")
    lte_options.add_option("", "--lte-cp-length", type="intx", default=defaultconf['CP_LENGTH'],
            help="set the number of bits in the cyclic prefix [default=%default]")
    lte_options.add_option("", "--lte-log", action="store_true", default=False,
            help="logging [default=%default]")
    lte_options.add_option("", "--lte-verbose", action="store_true", default=False,
            help="verbose [default=%default]")

    return parser

def _parse_options(options):
    if callable(hasattr(options, 'parse_args')):
        options = options.parse_args()

    return dict2obj({'tx_amplitude': options.lte_tx_amplitude,
                            'freq': options.lte_freq,
                            'bandwidth': options.lte_bandwidth,
                            'file': options.lte_file,
                            'buffersize': options.lte_buffersize,
                            'modulation': options.lte_modulation,
                            'fft_length': options.lte_fft_length,
                            'occupied_tones': options.lte_occupied_tones,
                            'cp_length': options.lte_cp_length,
                            'modulation': options.lte_modulation,
                            'verbose': False,
                            'log': False})

def _build_options():
    from gnuradio.eng_option import eng_option
    from optparse import OptionParser

    _parser = OptionParser(option_class=eng_option, conflict_handler="resolve")
    options = add_options(_parser)
    return options

def build(options = None):
    # create options using defaults if none is provided
    options = options or _build_options()
    lte = tp.TransmitPath(_parse_options(options))
    return lte

def build_rx(rx_callback, options = None):
    options = options or _build_options()
    lte = rp.ReceivePath(rx_callback, _parse_options(options))
    return lte

def build_usrp_rx(options = None):
    if options == None:
        options = _build_options()
        uhd_receiver.add_options(options)
        options = options.parse_args()

    return  uhd_receiver('',
            options.lte_bandwidth, options.lte_freq,
            options.lo_offset, options.rx_gain,
            options.spec, options.antenna,
            options.clock_source, options.verbose)
