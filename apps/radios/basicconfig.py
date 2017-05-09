# HyDRA configuration
HYDRA_FFT_M_LEN = 5120
VIRTUAL_RADIO = ['lte', 'nbiot']
HYDRA_BW = 4e6
HYDRA_CF = 2.9e9

# VR Configurations
RCONF = {
    'LTE': {
        'CF': HYDRA_CF - 500e3,
        'BW': 1.6e6,
        'AMPLITUDE': 0.1,
        'MODULATION': 'bpsk',
        'FFT_LENGTH': 1024,
        'OCCUPIED_TONES': 800,
        'CP_LENGTH': 2,
    },

    'NBIOT': {
        'CF': HYDRA_CF + 400e3,
        'BW': 200e3,
        'AMPLITUDE': 0.1,
        'MODULATION': 'bpsk',
        'FFT_LENGTH': 64,
        'OCCUPIED_TONES': 48,
        'CP_LENGTH': 2,
    }
}
