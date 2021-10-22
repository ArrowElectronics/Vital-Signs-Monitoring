import os, sys
from cStringIO import StringIO
sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.append(os.path.abspath('../../cli/m2m2/tools'))
import CLI
class m2m2_error(Exception):
    pass

g_shell = CLI.m2m2_shell()#stdout=self.stdout)

class m2m2_test_lib():            
    ROBOT_LIBRARY_SCOPE = 'GLOBAL'

    def __init__(self, serial_port, connect_mode='usb', connect=True):
        self.stdout = StringIO()
        sys.stdout = self.stdout
        sys.stderr = self.stdout
        self.shell = g_shell
        self.pointer_info = "NA"
        self.dvt_ver = None
        self.clk_calib_val = None
        # TODO: Any newly added check function has to be added to the 'self.checkFuncs' dictionary
        self.checkFuncs = {'must_contain':self._must_contain_check,
                           'must_not_contain':self._must_not_contain_check,
                           'parse_eda_dcfg_check':self._parse_eda_dcfg_check,
                           'parse_bia_dcfg_check':self._parse_bia_dcfg_check}
        if connect == True:
            if connect_mode == 'ble':
                connect_command = "connect_dongle {}".format(serial_port)
            else:
                connect_command = "connect_usb {}".format(serial_port)
            self._runCommand(connect_command)
            self._check_dvt_version()
            get_default_config = ["delete_dcb_config adxl", "delete_dcb_config adpd4000", "delete_dcb_config ecg", "delete_dcb_config ppg", "delete_dcb_config eda", "delete_dcb_config bcm","delete_dcb_config ad7156", "delete_dcb_config low_touch", "delete_config_file", "delete_dcb_config lt_app_lcfg", "delete_dcb_config user0_config"]
            for i in range(len(get_default_config)):
                self._runCommand(get_default_config[i])
                self._runCommand("delay 2")
        
    def _runCommand(self, cmd, expected_returns=None, check_type='must_contain'):
        sys.stdout = self.stdout
        sys.stderr = self.stdout
        cmd_return = self.shell.onecmd(cmd)
        if expected_returns != None:
            if check_type in self.checkFuncs.keys():
                self.checkFuncs[check_type](cmd=cmd, expected_returns=expected_returns, cmd_return=cmd_return)
        ret_str = self.stdout.getvalue()
        print ret_str
        self._purge_output()
        return ret_str
    
    def _check_dvt_version(self):
        global clk_calib_val
        err_stat, chip_id = self.shell.do_getChipID('2')
        if chip_id == 0xc0:
            print("DVT1 Watch Connected")
            self.dvt_ver = 1
            self.clk_calib_val = 6
        else:
            print("DVT2 Watch Connected")
            self.dvt_ver = 2
            self.clk_calib_val = 2

    def _runquickstart(self, cmd, returns=None, must_contain=True):
        sys.stdout = self.stdout
        sys.stderr = self.stdout
        if cmd in self.quickstarts:
            for command in self.quickstarts[cmd]["commands"]:
                self.shell.onecmd(command)
        if returns != None:
            for ret in returns:
                if must_contain:
                    if not ret in self.stdout.getvalue():
                        raise m2m2_error("Command '{}' failed. Expected to see:\n'{}'\n, actual output was:\n'{}'\n".format(cmd, ret, self.stdout.getvalue()))
                else:
                    if ret in self.stdout.getvalue():
                        raise m2m2_error("Command '{}' failed. :\n'{}'\n, actual output was:\n'{}'\n".format(cmd, ret, self.stdout.getvalue()))
        ret_str = self.stdout.getvalue()
        print ret_str

    def run_quickstart_cmd(self, cmd, returns=None):
        self._runquickstart(cmd, returns)
       
    
    def _purge_output(self):
        self.stdout.seek(0)
        self.stdout.truncate(0)

    
    # ****** Measurement & Pass/Fail Check Functions ****** #
    def _parse_eda_dcfg_check(self, **kwargs):
        """
        
        """
        cmd = kwargs['cmd']
        expected_returns = kwargs['expected_returns']
        cmd_return = kwargs['cmd_return']
        std_out = self.stdout.getvalue()
        out_list = std_out.split('\n')
        if len(out_list) >= 5:
            line_list = out_list[5].split('|')
            addr = int(line_list[1].strip().strip('L'), 16)
            val = int(line_list[2].strip().strip('L'), 16)
            exp_val = int(expected_returns[0].strip(), 16)
            if exp_val == val:
                pass
            else:
                raise m2m2_error("Read value check failed!\nExpVal: {} | ActualVal: {}".format(exp_val, val))
        else:
            raise m2m2_error("Read output is invalid!\nCmd Return: {}".format(std_out))

    def _parse_bia_dcfg_check(self, **kwargs):
        """
        
        """
        cmd = kwargs['cmd']
        expected_returns = kwargs['expected_returns']
        cmd_return = kwargs['cmd_return']
        std_out = self.stdout.getvalue()
        out_list = std_out.split('\n')
        if len(out_list) >= 5:
            line_list = out_list[5].split('|')
            addr = int(line_list[1].strip().strip('L'), 16)
            val = int(line_list[2].strip().strip('L'), 16)
            exp_val = int(expected_returns[0].strip(), 16)
            if exp_val == val:
                pass
            else:
                raise m2m2_error("Read value check failed!\nExpVal: {} | ActualVal: {}".format(exp_val, val))
        else:
            raise m2m2_error("Read output is invalid!\nCmd Return: {}".format(std_out))

    def _check_func_template(self, **kwargs):
        """
        DO NOT EDIT THIS FUNCTION!
        Create a copy of this function, rename it and edit/add needed functionality
        """
        cmd = kwargs['cmd']
        expected_returns = kwargs['expected_returns']
        cmd_return = kwargs['cmd_return']
        if cmd_return != None:
            if type(cmd_return) == tuple:
                err_status = cmd_return[0]
            elif type(cmd_return) == int:
                err_status = cmd_return
            
            if err_status:  # Error status will be 0 if the command was successful
                    raise m2m2_error("Command failed!\nCommand: {}".format(cmd))
                
        for exp_ret in expected_returns:
            # TODO: Implement measurement or conditional check logic and update 'err_status' variable
            err_status = False
            if err_status:
                #TODO: Update error message with necessary details
                raise m2m2_error("Command Return Check Failed!\nCommand: {}\nReturn:{}".format(cmd, exp_ret))

    def _must_contain_check(self, **kwargs):
        cmd = kwargs['cmd']
        expected_returns = kwargs['expected_returns']
        cmd_return = kwargs['cmd_return']
        if cmd.lower() == 'fs_req_debug_info':
            std_out = self.stdout.getvalue()
            self.pointer_info = str(int(std_out.split('\n')[0].split(' ')[-3])-1)
        for exp_ret in expected_returns:
            if not exp_ret in self.stdout.getvalue():
                raise m2m2_error("Command '{}' failed. Expected to see:\n'{}'\n, actual output was:\n'{}'\n".format(cmd, exp_ret, self.stdout.getvalue()))
    
    def _must_not_contain_check(self, **kwargs):
        cmd = kwargs['cmd']
        expected_returns = kwargs['expected_returns']
        cmd_return = kwargs['cmd_return']
        for exp_ret in expected_returns:
            if exp_ret in self.stdout.getvalue():
                raise m2m2_error("Command '{}' failed. :\n'{}'\n, actual output was:\n'{}'\n".format(cmd, exp_ret, self.stdout.getvalue()))

    
    # ********** TestCase Extenal Call Functions *********** #
    def commandMustContain(self, cmd, expected_returns=None):
        self._runCommand(cmd, expected_returns, check_type='must_contain')
    
    def testPassFailCheck(self, cmd, check_type, expected_returns=None):
        self._runCommand(cmd, expected_returns, check_type)

    def commandMustNotContain(self, cmd, expected_returns=None):
        self._runCommand(cmd, expected_returns, check_type='must_not_contain')



    
#m = m2m2_test_lib("COM15")







