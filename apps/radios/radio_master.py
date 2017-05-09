def build_rx_from_name(name, callback, options = None):
    exec('import radio_' + name + ' as radio')
    return radio.build_rx(callback, options)

def build_usrp_rx(name, options = None):
    exec('import radio_' + name + ' as radio')
    radio.build_usrp_rx(options)
