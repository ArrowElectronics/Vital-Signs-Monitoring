import visa
import os

visa_dll_path = 'C:\\Program Files\\IVI Foundation\\VISA\\Win64\\ktvisa\\ktbin\\visa32.dll'
if os.path.exists(visa_dll_path) and os.path.isfile(visa_dll_path):
    rm = visa.ResourceManager('C:\\Program Files\\IVI Foundation\\VISA\\Win64\\ktvisa\\ktbin\\visa32.dll')
else:
    rm = visa.ResourceManager()


def close_instrlib():
    """
    This function can be called to close resource manager
    :return:
    """
    if rm:
        rm.close()


class Instrument(object):
    def __init__(self):
        self.instr = None

    def instr_connect(self, instr_addr):
        """
        This function opens the resource specified by the isntr_addr
        :param instr_addr:
        :return:
        """
        self.instr = rm.open_resource(instr_addr)

    def instr_close(self):
        """
        This function closes the instrument resource reference
        :return:
        """
        self.instr.close()


class KeysightFG33522B(Instrument):

    def __init__(self):
        super(KeysightFG33522B, self).__init__()

    def cfg_waveform(self, freq_hz, vpp, offset, inv):
        """
        This function will generate a sinusoid waveform with the given configuration
        :param freq_hz:
        :param vpp:
        :param offset:
        :return:
        """
        self.instr.write(':OUTPut2:LOAD %s' %('INFinity'))
        self.instr.write(':OUTPut1:LOAD %s' %('INFinity'))
        self.instr.write(':SOURce1:APPLy:SINusoid %G HZ,%G VPP,%G V' % (freq_hz, vpp, offset))
        if inv == 'INV':
            self.instr.write(':SOURce2:TRACk %s' % 'INVerted')
        elif inv == 'ON':
            self.instr.write(':SOURce2:TRACk %s' % 'ON')
        else :
            self.instr.write(':SOURce2:TRACk %s' % 'OFF')
        #self.instr.write(':SOURce2:TRACk %s' % 'INVerted')

    def output_enable(self, state, channel):
        """
        This function enables/disables the output channels.
        :param state: 0-Disable, 1-Enable
        :param channel: 1 or 2. Passing -1 to this will toggle both channels
        :return:
        """
        if channel < 1:  # If ch -1, toggle both channels
            self.instr.write(':OUTPut%d %d' % (1, state))
            self.instr.write(':OUTPut%d %d' % (2, state))
        else:
            self.instr.write(':OUTPut%d %d' % (channel, state))


class KeithleySM2400(Instrument):

    def __init__(self):
        super(KeithleySM2400, self).__init__()

    def reset(self):
        self.instr.write('status:queue:clear;*RST;:stat:pres;:*CLS;')

    def cfg_source_mode(self, src_mode='CURR'):
        """
        A string property that controls the source mode, which can
        take the values 'CURR' or 'VOLT'
        :param src_mode:
        :return:
        """
        self.instr.write(':SOUR:FUNC %s' % src_mode)

    def cfg_output_state(self, state):
        """
        This function enables/disables the output channels.
        :param state: 0-Disable, 1-Enable
        :return:
        """
        self.instr.write('OUTPut %d' % state)

    def cfg_source_current_range(self, curr_range_amps=0):
        """
        A floating point property that controls the source current
        range in Amps, which can take values between -1.05 and +1.05 A.
        Auto-range is disabled when this property is set.

        :param curr_range_amps: values between -1.05 and 1.05
        :return:
        """
        if curr_range_amps == 0:  # 0 is considered Auto range
            self.instr.write(':SOUR:CURR:RANG:AUTO 0')
        else:
            self.instr.write(':SOUR:CURR:RANG %g' % curr_range_amps)

    def cfg_source_current(self, curr_amps=0):
        """
        A floating point property that controls the source current
        in Amps.

        :param curr_amps: values between -1.05 and 1.05
        :return:
        """
        self.instr.write(':SOUR:CURR:LEV %g' % curr_amps)

    def cfg_sense_current_range(self, curr_range_amps=0):
        """
        A floating point property that controls the measurement current
        range in Amps, which can take values between -1.05 and +1.05 A.
        Auto-range is disabled when this property is set.

        :param curr_range_amps: values between -1.05 and 1.05
        :return:
        """
        if curr_range_amps == 0:  # 0 is considered Auto range
            self.instr.write(':SENS:CURR:RANG:AUTO 0')
        else:
            self.instr.write(':SENS:CURR:RANG %g' % curr_range_amps)

    def cfg_source_voltage(self, voltage_volts=0):
        """
        A floating point property that controls the source voltage
        in Amps.

        :param voltage_volts: values between -210 and 210
        :return:
        """
        self.instr.write(':SOUR:VOLT:LEV %g' % voltage_volts)

    def cfg_source_voltage_range(self, voltage_range_volts=0):
        """
        A floating point property that controls the source voltage
        range in Volts, which can take values between -210 and +210 V.
        Auto-range is disabled when this property is set.

        :param voltage_range_volts: values between -210 and 210
        :return:
        """
        if voltage_range_volts == 0:  # 0 is considered Auto range
            self.instr.write(':SENS:VOLT:RANG:AUTO 0')
        else:
            self.instr.write(':SENS:VOLT:RANG %g' % voltage_range_volts)

    def use_front_terminals(self):
        """
        Enables the front terminals for measurement, and
        disables the rear terminals.
        :return:
        """
        self.instr.write(":ROUT:TERM FRON")

    def setup_battery_voltage(self):
        self.instr.write('*RST;*CLS')
        self.instr.write(':SOURce:FUNCtion VOLTage')
        self.instr.write(':SOURce:VOLTage 4.12')
        self.instr.write(':SENSe:CURRent:PROTection 0.105')
        self.instr.write(':SENSe:CURRent:RANGe 0.04')
        self.instr.write('OUTPut ON')


    def setup_battery_mode(self):
        import time
        self.instr.write(':FORM:DATA ASCii')
        #self.instr.write(':SOURce:FUNCtion VOLTage')
        #self.instr.write(':SOURce:VOLTage 4.4')
        #self.instr.write('OUTPut ON')
        #time.sleep(1)
        #self.instr.write(':SENSe:CURRent:PROTection 0.105')
        #self.instr.write(':SENSe:FUNCtion %s' 'CURR')
        #self.instr.write(':SENSe:CURRent:RANGe 0.1')
        self.instr.write(':FORM:ELEM CURR')

    def cfg_setup_buff(self, count):

        self.instr.write(':TRACe:CLEar')
        self.instr.write(':TRACe:FEED SENS')
        self.instr.write(':TRACe:POIN %g' % count)
        self.instr.write(":TRACe:FEED:CONT NEXT")
        self.instr.write(":TRIGger:COUNt %g" % count)
        # self.instr.write('OUTPut ON')
        # self.instr.write(":INIT")

    def measure_current(self):
        """ Configures the measurement of current.

        :param nplc: Number of power line cycles (NPLC) from 0.01 to 10
        """
        #self.instr.write('OUTPut ON')
        self.instr.write(':INIT')
        self.instr.write(':TRACE:DATA?')
        # self.instr.write('READ?')
        # self.instr.write('CALC3:FORM MEAN')
        # self.instr.write(':CALC3:DATA?')
        ret = self.instr.read()
        #self.instr.write('OUTPut OFF')
        return ret
