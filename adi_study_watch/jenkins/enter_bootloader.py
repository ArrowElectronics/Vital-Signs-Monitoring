import sys
import os

cli_dir = os.path.abspath((os.path.join(os.path.abspath(__file__), '../../nrf5_sdk_15.2.0/validation')))
sys.path.insert(0, cli_dir)

import common

common.initialize_setup()
common.watch_shell.do_enter_boot_loader_mode('')
common.close_setup()