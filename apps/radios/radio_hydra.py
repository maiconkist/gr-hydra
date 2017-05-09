def add_options(parser):
    import basicconfig as bc

    hydra_options = parser.add_option_group("HyDRA Options")
    hydra_options.add_option("", "--virtual-radio", type="string", action='append', default=bc.VIRTUAL_RADIO,
            help="Virtual radios to use [default=%default]")
    hydra_options.add_option("", "--hydra-fft-length", type="intx", default=bc.HYDRA_FFT_M_LEN,
            help="HyDRA FFT M size [default=%default]")
    hydra_options.add_option("", "--hydra-freq", type="eng_float", default=bc.HYDRA_CF,
            help="Hydra transmit frequency [default=%default]", metavar="FREQ")
    hydra_options.add_option("", "--hydra-bandwidth", type="eng_float", default=bc.HYDRA_BW,
            help="Hydra sample_rate [default=%default]")

    return parser

def build(options = None):
    if options == None:
        from gnuradio.eng_option import eng_option
        from optparse import OptionParser
        parser = OptionParser(option_class=eng_option, conflict_handler="resolve")

        # first parse to get radios enabled
        (options, args) = add_options(parser).parse_args()

        # try to add options for each virtual radio enabled 
        for radio in options.virtual_radio:
            exec('from radios import radio_' + radio)
            exec('radio_' + radio + '.add_options(parser)')

        # second parse to get all default configuration of all radios
        (options, args) = add_options(parser).parse_args()

    # build list of tuples with (cf, bw) for each enabled virtual radio
    vr_configs = []
    for radio in options.virtual_radio:
        t = [getattr(options, radio + '_freq'), getattr(options, radio + '_bandwidth')]
        vr_configs.append(t)

    # finally, build hydra
    import hydra
    hydra = hydra.hydra_sink(len(options.virtual_radio), # number of virtual radios
                             options.hydra_fft_length,   # fft m
                             options.hydra_freq,         # hydra cf -- need to configure this in USRP too
                             options.hydra_bandwidth,    # hydra bw -- need to configure this in USRP too
                             vr_configs)
    return hydra
