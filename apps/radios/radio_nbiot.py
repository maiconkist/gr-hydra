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
    defaultconf = bc.RCONF['NBIOT']

    nbiot_options = parser.add_option_group("NB-IoT Options")
    nbiot_options.add_option("", "--nbiot-bandwidth", type="eng_float", default=defaultconf['BW'],
         help="set bandwidth for VR 1 [default=%default]")
    nbiot_options.add_option("", "--nbiot-freq", type="eng_float", default=defaultconf['CF'],
         help="set central frequency for VR 1 [default=%default]")
    nbiot_options.add_option("", "--nbiot-tx-amplitude", type="eng_float", default=defaultconf['AMPLITUDE'], metavar="AMPL",
         help="set transmitter digital amplitude: 0 <= AMPL < 1.0 [default=%default]")
    nbiot_options.add_option("", "--nbiot-file", type="string", default='./nbiot_file',
         help="set the file to obtain data [default=%default]")
    nbiot_options.add_option("", "--nbiot-buffersize", type="intx", default=16,
         help="set number of bytes to read from buffer size for VR1 [default=%default]")
    nbiot_options.add_option("-m", "--nbiot-modulation", type="string", default=defaultconf['MODULATION'],
         help="set modulation type (bpsk, qpsk, 8psk, qam{16,64}) [default=%default]")
    nbiot_options.add_option("", "--nbiot-fft-length", type="intx", default=defaultconf['FFT_LENGTH'],
         help="set the number of FFT bins [default=%default]")
    nbiot_options.add_option("", "--nbiot-occupied-tones", type="intx", default=defaultconf['OCCUPIED_TONES'],
         help="set the number of occupied FFT bins [default=%default]")
    nbiot_options.add_option("", "--nbiot-cp-length", type="intx", default=defaultconf['CP_LENGTH'],
            help="set the number of bits in the cyclic prefix [default=%default]")
    nbiot_options.add_option("", "--nbiot-log", action="store_true", default=False,
            help="logging [default=%default]")
    nbiot_options.add_option("", "--nbiot-verbose", action="store_true", default=False,
            help="verbose [default=%default]")
    return parser


def _parse_options(options):
    if callable(hasattr(options, 'parse_args')):
        options = options.parse_args()

    return dict2obj({'tx_amplitude': options.nbiot_tx_amplitude,
                            'freq': options.nbiot_freq,
                            'bandwidth': options.nbiot_bandwidth,
                            'file': options.nbiot_file,
                            'buffersize': options.nbiot_buffersize,
                            'modulation': options.nbiot_modulation,
                            'fft_length': options.nbiot_fft_length,
                            'occupied_tones': options.nbiot_occupied_tones,
                            'cp_length': options.nbiot_cp_length,
                            'modulation': options.nbiot_modulation,
                            'verbose': False,
                            'log': False})

def _build_options():
    from gnuradio.eng_option import eng_option
    from optparse import OptionParser

    _parser = OptionParser(option_class=eng_option, conflict_handler="resolve")
    (options, args) = add_options(_parser)
    return options

def build(options = None):
    # create options using defaults if none is provided
    options = options or _build_options()
    nbiot = tp.TransmitPath(_parse_options(options))
    return nbiot

def build_rx(rx_callback, options = None):
    options = options or _build_options()
    nbiot = rp.ReceivePath(rx_callback, _parse_options(options))
    return nbiot

def build_usrp_rx(options = None):
    if options == None:
        options = _build_options()
        uhd_receiver.add_options(options)
        options = options.parse_args()

    return  uhd_receiver('',
            options.nbiot_bandwidth, options.nbiot_freq,
            options.lo_offset, options.rx_gain,
            options.spec, options.antenna,
            options.clock_source, options.verbose)
