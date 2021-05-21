# ********************** Test Variables ********************************
arduino_port = 'COM7'
watch_port = 'COM12'
# arduino = None
# watch_shell = None
ecg_stream_file_name = 'EcgAppStream.csv'
ppg_stream_file_name = 'ppgAppStream.csv'
adxl_stream_file_name = 'adxlAppStream.csv'
temperature_stream_file_name = 'TempAppStream.csv'
adpd_stream_file_name = 'adpd6Stream.csv'
volt_scale_range = (0, 5)
# The switch map dictionary maps the various switches to the arduino digital pins (24-42)
switch_map = {'SNOISE1': 22, 'SNOISE2': 23, 'ECG_NEGSEL': 24, 'ECG_POSSEL': 25}
# **********************************************************************