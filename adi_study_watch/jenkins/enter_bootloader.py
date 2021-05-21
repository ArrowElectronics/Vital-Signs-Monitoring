import sys
import os

cli_dir = os.path.abspath((os.path.join(os.path.abspath(__file__), '../../nrf5_sdk_15.2.0/validation')))
sys.path.insert(0, cli_dir)

import common

shell = common.CLI.m2m2_shell()
common.read_station_cfg()
shell.do_connect_usb(common.watch_port)
shell.do_enterBootLoader('')