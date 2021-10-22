try:
    from m2m2_common import *
    import Queue
    import math
    import numpy as np
    import struct
    import pyqtgraph as pg
    import yaml
    from abc import ABCMeta
    from abc import abstractmethod
    from abc import abstractproperty
except ImportError as e:
    print "Oops, looks like you're missing a Python module!"
    print "Try installing it with pip: `pip install MODULE_NAME`"
    print "Error message is: {}".format(e)
    print "Exiting..."
    raise SystemExit


class plot():
    __metaclass__ = ABCMeta
    name = ""
    subplots = {}
    window = None
    data_buffer = Queue.Queue()
    first_run = True
    sequence_num = 0
    first_run1 = True
    sequence_num1 = 0
    first_run_cl1 = [True, True, True, True, True, True, True, True, True, True, True, True]
    first_run_cl2 = [True, True, True, True, True, True, True, True, True, True, True, True]
    sequence_num_cl1 = [0,0,0,0,0,0,0,0,0,0,0,0]
    sequence_num_cl2 = [0,0,0,0,0,0,0,0,0,0,0,0]

    def __init__(self, window_obj):
        self.window = window_obj
        self.window.setWindowTitle(self.name)
        self.setup()

    def add_subplot(self, subplot_name, row, col, xlabel, xunit, ylabel, yunit, data_series):
        # Create a new subplot, add it to the subplots dict with its name as a key
        self.subplots[subplot_name] = subplot(subplot_obj = self.window.addPlot(title=subplot_name, row = row, col=col),
                                            subplot_name = subplot_name,
                                            xlabel = xlabel,
                                            xunit = xunit,
                                            ylabel = ylabel,
                                            yunit = yunit,
                                            data_series = data_series)

    def _update(self):
        # Assemble all of the new data we've received since the last _update() and re-draw the subplots with it
        data_list = []
        while not self.data_buffer.empty():
            data_list.append(self.data_buffer.get())
        data = self.update(data_list)
        self.plot(data)

    def plot(self, data):
        # Re-draw subplots with new data
        if data == None:
            return
        for name in self.subplots:
            if name in data:
                self.subplots[name]._plot(data[name])

    def submit(self, packet):
        # Submit a new packet to the plot. The packet should be raw (string)
        self.data_buffer.put(packet)

    def check_seq_num(self, new_seq_num, stream_name=''):
        packet_loss = False
        #current_seq_num = struct.unpack('H', struct.pack('H', new_seq_num))[0]
        current_seq_num = new_seq_num
        if self.first_run:
            # Don't complain about a dropped packet if this is the first packet we've received.
            self.first_run = False
        else:
            # Python has ints of unlimited precision, so we have to manually do a 16 bit rollover.
            overflow = 0
            if self.sequence_num == 65535:
                self.sequence_num = 0
                overflow = 1
            if current_seq_num != self.sequence_num + 1 - overflow:
                print "Dropped packet detected! Previous seq number:{} Expected:{} Actual:{}".format(self.sequence_num, self.sequence_num + 1, current_seq_num)
                packet_loss = True
        self.sequence_num = current_seq_num
        if stream_name and packet_loss:
            if not pkt_loss_dict[stream_name]:
                try:
                    update_pkt_loss(stream_name, True)
                    with open('pkt_loss.yaml', 'w') as f_ref:
                        yaml.dump(pkt_loss_dict, f_ref)
                except Exception as e:
                    print('Warning: Unable to update packet loss yaml file!')

    def check_seq_num1(self, new_seq_num):
        packet_loss = False
        current_seq_num = struct.unpack('H', struct.pack('H', new_seq_num))[0]
        if self.first_run1:
            # Don't complain about a dropped packet if this is the first packet we've received.
            self.first_run1 = False
            print "first check run! {}".format(current_seq_num)
        else:
            # Python has ints of unlimited precision, so we have to manually do a 16 bit rollover.
            overflow = 0
            if self.sequence_num1 == 65535:
                self.sequence_num1 = 0
                overflow = 1
            if current_seq_num != self.sequence_num1 + 1 - overflow:
                print "Dropped packet detected! Previous seq number:{} Expected:{} Actual:{}".format(self.sequence_num1, self.sequence_num1 + 1, current_seq_num)
                packet_loss = True
        self.sequence_num1 = current_seq_num
        if packet_loss:
            if not pkt_loss_dict['radpd']:
                try:
                    update_pkt_loss('radpd', True)
                    with open('pkt_loss.yaml', 'w') as f_ref:
                        yaml.dump(pkt_loss_dict, f_ref)
                except Exception as e:
                    print('Warning: Unable to update packet loss yaml file!')
        
    def check_seq_num_cl1(self, slot_num, new_seq_num):
        packet_loss = False
        current_seq_num = struct.unpack('H', struct.pack('H', new_seq_num))[0]
        if self.first_run_cl1[slot_num]:
            # Don't complain about a dropped packet if this is the first packet we've received.
            self.first_run_cl1[slot_num] = False
            print "first check run! {}".format(current_seq_num)
        else:
            # Python has ints of unlimited precision, so we have to manually do a 16 bit rollover.
            overflow = 0
            if self.sequence_num_cl1[slot_num] == 65535:
                self.sequence_num_cl1[slot_num] = 0
                overflow = 1
            if current_seq_num != self.sequence_num_cl1[slot_num] + 1 - overflow:
                print "Dropped packet detected! Previous seq number:{} Expected:{} Actual:{}".format(self.sequence_num_cl1[slot_num], self.sequence_num_cl1[slot_num] + 1, current_seq_num)
                packet_loss = True
        self.sequence_num_cl1[slot_num] = current_seq_num
        if packet_loss:
            if not pkt_loss_dict['radpd']:
                try:
                    update_pkt_loss('radpd', True)
                    with open('pkt_loss.yaml', 'w') as f_ref:
                        yaml.dump(pkt_loss_dict, f_ref)
                except Exception as e:
                    print('Warning: Unable to update packet loss yaml file!')
    
    def check_seq_num_cl2(self, slot_num, new_seq_num):
        packet_loss = False
        current_seq_num = struct.unpack('H', struct.pack('H', new_seq_num))[0]
        if self.first_run_cl2[slot_num]:
            # Don't complain about a dropped packet if this is the first packet we've received.
            self.first_run_cl2[slot_num] = False
            print "first check run! {}".format(current_seq_num)
        else:
            # Python has ints of unlimited precision, so we have to manually do a 16 bit rollover.
            overflow = 0
            if self.sequence_num_cl2[slot_num] == 65535:
                self.sequence_num_cl2[slot_num] = 0
                overflow = 1
            if current_seq_num != self.sequence_num_cl2[slot_num] + 1 - overflow:
                print "Dropped packet detected! Previous seq number:{} Expected:{} Actual:{}".format(self.sequence_num_cl2[slot_num], self.sequence_num_cl2[slot_num] + 1, current_seq_num)
                packet_loss = True
        self.sequence_num_cl2[slot_num] = current_seq_num
        if packet_loss:
            if not pkt_loss_dict['radpd']:
                try:
                    update_pkt_loss('radpd', True)
                    with open('pkt_loss.yaml', 'w') as f_ref:
                        yaml.dump(pkt_loss_dict, f_ref)
                except Exception as e:
                    print('Warning: Unable to update packet loss yaml file!')

    @abstractmethod
    def setup(self):
        pass

    @abstractmethod
    def update(self, data_list):
        pass


class subplot():
    plot_ptr = 0

    def __init__(self, subplot_obj, subplot_name, xlabel, xunit, ylabel, yunit, data_series):
        self.name = subplot_name
        self.subplot_obj = subplot_obj
        self.subplot_obj.addLegend()
        self.subplot_obj.showGrid(x=True, y=True)
        self.subplot_obj.setLabels(left=(ylabel, yunit),
                                    bottom=(xlabel, xunit))
        self.data_series = data_series
        for series_name in self.data_series:
            # Create a line on the subplot for each series in the subplot
            series = self.data_series[series_name]
            series["curve"] = self.subplot_obj.plot(series["data"],
                                                    pen=pg.mkPen(series["format"]["colour"]))

    def _plot(self, data):
        # Redraw the plot with any new data that has been added to the data series
        self._extend_data(data)
        for series in data:
            name = series["series_name"]
            self.data_series[name]["curve"].setPos(self.plot_ptr, 0)
            self.data_series[name]["curve"].setData(self.data_series[name]["data"])


    def _extend_data(self, data):
        # Scrolls the data in the subplot's series_name, and adds data to it
        new_data_items = 0
        for data_series in data:
            if data_series["series_name"] in self.data_series:
                name = data_series["series_name"]
                d_len = len(data_series["series_data"])
                if d_len == 0:
                    continue
                s_data = self.data_series[name]["data"]
                self.data_series[name]["data"] = np.roll(s_data, -d_len)
                self.data_series[name]["data"][-d_len:] = data_series["series_data"]
                new_data_items = len(data_series["series_data"])
        self.plot_ptr += new_data_items


class adpd1_plot(plot):
    # The name used by the user to select this plotter
    selection_name = "radpd1"
    # The plot title
    name = "ADPD400x Data"
    # Names used to refer to the two sub-plots
    slot_a_key = "SlotA Ch1 Data"
    slot_a_ch1_key = "Slot A Ch1"
    slot_a_key2 = "Ch2 Data"
    slot_a_ch2_key = "Slot A Ch2"
    slot_a_ch1_series_name = "ADPD Slot A Ch1"
    slot_a_ch2_series_name = "ADPD Slot B Ch1"
    ch1_sample_count = 0
    ch2_sample_count = 0
    data_ch1 = []
    data_ch2 = []
    ts_ch1 = []
    ts_ch2 = []
    # Stream address that this plot uses
    stream_addr = M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_ADPD_STREAM1
    enable_csv_logs = 0
    fname = "adpd1Stream.csv"
    def setup(self):
        # Create a subplot for slot A data
        self.add_subplot(   subplot_name = self.slot_a_key,
                            row = 1,
                            col = 1,
                            xlabel = "Time",
                            xunit = "Samples",
                            ylabel = "Amplitude",
                            yunit = "LSBs",
                            # A single series called Slot A
                            data_series =   {  self.slot_a_ch1_key:
                                                {"format":
                                                    # Series Display name
                                                    {"name": self.slot_a_ch1_series_name,
                                                    # Colour format string for this series
                                                    "colour":"r"},
                                                # Empty data array for this series
                                                "data": np.zeros(250),
                                                },
                                            })
        # Create a subplot for slot B data
        self.add_subplot(   subplot_name = self.slot_a_key2,
                            row = 2,
                            col = 1,
                            xlabel = "Time",
                            xunit = "Samples",
                            ylabel = "Amplitude",
                            yunit = "LSBs",
                            # A single series called Slot B
                            data_series =   {  self.slot_a_ch2_key:
                                                {"format":
                                                    # Series Display name
                                                    {"name": self.slot_a_ch2_series_name,
                                                    # Colour format string for this series
                                                    "colour":"r"},
                                                # Empty data array for this series
                                                "data": np.zeros(250),
                                                },
                                            })

    def update(self, data_list):
        # Function that's called to update the plot. Should return a dictionary with each subplot's data
        new_data_a_ch1 = []
        new_data_a_ch2 = []

        for data in data_list:
            # Peek at the header to find out what kind of data we have
            hdr = m2m2_packet(0, adpd4000_data_header_t())
            hdr.unpack(data, override_size = True)
            if(hdr.payload.data_format & 0x100 == 0):
                # Decide which subplot to send this packet to
                signalSize = (hdr.payload.data_format & 0xF)
                darkSize = (hdr.payload.data_format & 0xF0) >> 4
                dataSize = signalSize + darkSize
                # Unpack all of the data into the correct packet structure
                pkt = m2m2_packet(0, m2m2_sensor_adpd4000_data_stream_t())
                pkt.unpack(data)
    
                sampleSize = dataSize
                two_channels = pkt.payload.channel_num
                # print "SampleSize:{} signalSize:{} channels:{} seq:{}".format(dataSize, signalSize, two_channels, pkt.payload.sequence_num,)
                for i in range (darkSize, pkt.payload.sample_num*dataSize, dataSize):
                    adpd_data = pkt.payload.adpddata[i]
                    for j in range (1, signalSize):
                        adpd_data |= pkt.payload.adpddata[i+j] << (j*8)
    
                    if (two_channels == 1):
                        new_data_a_ch1.append(adpd_data)
                        self.data_ch1.append(adpd_data)
                        self.ts_ch1.append(pkt.payload.timestamp)
                        self.ch1_sample_count += 1
                        # print "S_B Ch1 Data:{} ".format(hex(adpd_data))
                    else:
                        new_data_a_ch2.append(adpd_data)
                        self.data_ch2.append(adpd_data)
                        self.ts_ch2.append(pkt.payload.timestamp)
                        self.ch2_sample_count += 1
                        # print "S_B Ch2 Data:{} ".format(hex(adpd_data))

                    if self.enable_csv_logs:
                        if(self.ch1_sample_count > pkt.payload.sample_num):
                            for i in range(self.ch1_sample_count):
                                fstream = open(self.fname, "a")
                                fstream.write ('{}, {}, {}, {}\n'.format(self.ts_ch1[i], self.data_ch1[i], 0, 0))
                                fstream.close()
                            self.ch1_sample_count = 0
                            self.data_ch1 = []
                            self.ts_ch1 = []
                        elif(self.ch2_sample_count > pkt.payload.sample_num):
                            for i in range(self.ch2_sample_count):
                                fstream = open(self.fname, "a")
                                fstream.write ('{}, {},{}, {}\n'.format(0, 0, self.ts_ch2[i], self.data_ch2[i]))
                                fstream.close()
                            self.ch2_sample_count = 0
                            self.data_ch2 = []
                            self.ts_ch2 = []
                        elif((self.ch1_sample_count == self.ch2_sample_count) and (self.ch1_sample_count == pkt.payload.sample_num)):      
                            for i in range(pkt.payload.sample_num):
                                fstream = open(self.fname, "a")
                                fstream.write ('{},{},{}, {}\n'.format(self.ts_ch1[i], self.data_ch1[i], self.ts_ch2[i], self.data_ch2[i]))
                                fstream.close()
                            self.ch1_sample_count = 0
                            self.ch2_sample_count = 0 
                            self.data_ch1 = []
                            self.data_ch2 = []
                            self.ts_ch1 = []
                            self.ts_ch2 = []

            else:
                pkt = m2m2_packet(0, m2m2_sensor_adpd4000_impulse_stream_t())
                pkt.unpack(data)
                two_channels = pkt.payload.channel_num
                signalSize = (hdr.payload.data_format & 0xff)
                for i in range (0, signalSize,2):
                    adpd_data = pkt.payload.adpddata[i]
                    adpd_data |= pkt.payload.adpddata[i+1] << 8
                    new_data_a_ch1.append(adpd_data)

            if (two_channels == 1):
                self.check_seq_num_cl1(1, pkt.payload.sequence_num)
            else:
                self.check_seq_num_cl2(1, pkt.payload.sequence_num)

        if len(new_data_a_ch1) == 0 and len(new_data_a_ch2) == 0:
            # No new data
            return None
        # Dictionary that associates the correct data with each subplot
        if (two_channels == 1):
            return {self.slot_a_key: [{
                        "series_name":self.slot_a_ch1_key,
                        "series_data":new_data_a_ch1},
		                ]
                    }
        else:
            return {self.slot_a_key: [{
                        "series_name":self.slot_a_ch1_key,
                        "series_data":new_data_a_ch1},
		                ],
                    self.slot_a_key2: [{
                        "series_name":self.slot_a_ch2_key,
                        "series_data":new_data_a_ch2},
                        ]
		            }

    def save_csv_option(self, option):
        self.enable_csv_logs=option
        if self.enable_csv_logs:
            print "creating."
            fstream = open(self.fname, "w+")
            fstream.write ('{},{}, {}, {}'.format('TimeStamp_ch1', 'adpd_data_SlotA_ch1', 'TimeStamp_ch2','adpd_data_SlotA_ch2\n'))
            fstream.close()
        return

class adpd2_plot(plot):
    # The name used by the user to select this plotter
    selection_name = "radpd2"
    # The plot title
    name = "ADPD400x Data"
    # Names used to refer to the two sub-plots
    slot_b_key = "SlotB Ch1 Data"
    slot_b_ch1_key = "Slot B Ch1"
    slot_b_key2 = "Ch2 Data"
    slot_b_ch2_key = "Slot B Ch2"
    slot_b_ch1_series_name = "ADPD Slot B Ch1"
    slot_b_ch2_series_name = "ADPD Slot B Ch2"
    ch1_sample_count = 0
    ch2_sample_count = 0
    data_ch1 = []
    data_ch2 = []
    ts_ch1 = []
    ts_ch2 = []
    # Stream address that this plot uses
    stream_addr = M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_ADPD_STREAM2
    enable_csv_logs = 0
    fname = "adpd2Stream.csv"

    def setup(self):
        # Create a subplot for slot A data
        self.add_subplot(   subplot_name = self.slot_b_key,
                            row = 1,
                            col = 1,
                            xlabel = "Time",
                            xunit = "Samples",
                            ylabel = "Amplitude",
                            yunit = "LSBs",
                            # A single series called Slot A
                            data_series =   {  self.slot_b_ch1_key:
                                                {"format":
                                                    # Series Display name
                                                    {"name": self.slot_b_ch1_series_name,
                                                    # Colour format string for this series
                                                    "colour":"r"},
                                                # Empty data array for this series
                                                "data": np.zeros(250),
                                                },
                                            })
        # Create a subplot for slot B data
        self.add_subplot(   subplot_name = self.slot_b_key2,
                            row = 2,
                            col = 1,
                            xlabel = "Time",
                            xunit = "Samples",
                            ylabel = "Amplitude",
                            yunit = "LSBs",
                            # A single series called Slot B
                            data_series =   {  self.slot_b_ch2_key:
                                                {"format":
                                                    # Series Display name
                                                    {"name": self.slot_b_ch2_series_name,
                                                    # Colour format string for this series
                                                    "colour":"r"},
                                                # Empty data array for this series
                                                "data": np.zeros(250),
                                                },
                                            })

    def update(self, data_list):
        # Function that's called to update the plot. Should return a dictionary with each subplot's data
        new_data_b_ch1 = []
        new_data_b_ch2 = []

        for data in data_list:
            # Peek at the header to find out what kind of data we have
            hdr = m2m2_packet(0, adpd4000_data_header_t())
            hdr.unpack(data, override_size = True)
            if(hdr.payload.data_format & 0x100 == 0):
                # Decide which subplot to send this packet to
                signalSize = (hdr.payload.data_format & 0xF)
                darkSize = (hdr.payload.data_format & 0xF0) >> 4
                dataSize = signalSize + darkSize
                # Unpack all of the data into the correct packet structure
                pkt = m2m2_packet(0, m2m2_sensor_adpd4000_data_stream_t())
                pkt.unpack(data)
    
                sampleSize = dataSize
                two_channels = pkt.payload.channel_num
                # print "SampleSize:{} signalSize:{} channels:{} seq:{}".format(dataSize, signalSize, two_channels, pkt.payload.sequence_num,)
                for i in range (darkSize, pkt.payload.sample_num*dataSize, dataSize):
                    adpd_data = pkt.payload.adpddata[i]
                    for j in range (1, signalSize):
                        adpd_data |= pkt.payload.adpddata[i+j] << (j*8)
    
                    if (two_channels == 1):
                        new_data_b_ch1.append(adpd_data)
                        self.data_ch1.append(adpd_data)
                        self.ts_ch1.append(pkt.payload.timestamp)
                        self.ch1_sample_count += 1
                        # print "S_B Ch1 Data:{} ".format(hex(adpd_data))
                    else:
                        new_data_b_ch2.append(adpd_data)
                        self.data_ch2.append(adpd_data)
                        self.ts_ch2.append(pkt.payload.timestamp)
                        self.ch2_sample_count += 1
                        # print "S_B Ch2 Data:{} ".format(hex(adpd_data))

                    if self.enable_csv_logs:
                        if(self.ch1_sample_count > pkt.payload.sample_num):
                            for i in range(self.ch1_sample_count):
                                fstream = open(self.fname, "a")
                                fstream.write ('{}, {}, {}, {}\n'.format(self.ts_ch1[i], self.data_ch1[i], 0, 0))
                                fstream.close()
                            self.ch1_sample_count = 0
                            self.data_ch1 = []
                            self.ts_ch1 = []
                        elif(self.ch2_sample_count > pkt.payload.sample_num):
                            for i in range(self.ch2_sample_count):
                                fstream = open(self.fname, "a")
                                fstream.write ('{},{},{}, {}\n'.format(0, 0, self.ts_ch2[i], self.data_ch2[i]))
                                fstream.close()
                            self.ch2_sample_count = 0
                            self.data_ch2 = []
                            self.ts_ch2 = []
                        elif((self.ch1_sample_count == self.ch2_sample_count) and (self.ch1_sample_count == pkt.payload.sample_num)):      
                            for i in range(pkt.payload.sample_num):
                                fstream = open(self.fname, "a")
                                fstream.write ('{},{},{}, {}\n'.format(self.ts_ch1[i], self.data_ch1[i], self.ts_ch2[i], self.data_ch2[i]))
                                fstream.close()
                            self.ch1_sample_count = 0
                            self.ch2_sample_count = 0 
                            self.data_ch1 = []
                            self.data_ch2 = []
                            self.ts_ch1 = []
                            self.ts_ch2 = []

            else:
                pkt = m2m2_packet(0, m2m2_sensor_adpd4000_impulse_stream_t())
                pkt.unpack(data)
                two_channels = pkt.payload.channel_num
                signalSize = (hdr.payload.data_format & 0xff)
                for i in range (0, signalSize,2):
                    adpd_data = pkt.payload.adpddata[i]
                    adpd_data |= pkt.payload.adpddata[i+1] << 8
                    new_data_b_ch1.append(adpd_data)
            if (two_channels == 1):
                self.check_seq_num_cl1(2, pkt.payload.sequence_num)
            else:
                self.check_seq_num_cl2(2, pkt.payload.sequence_num)

        if len(new_data_b_ch1) == 0 and len(new_data_b_ch2) == 0:
            # No new data
            return None
        # Dictionary that associates the correct data with each subplot
        if (two_channels == 1):
            return {self.slot_b_key: [{
                        "series_name":self.slot_b_ch1_key,
                        "series_data":new_data_b_ch1},
		                ]
                    }
        else:
            return {self.slot_b_key: [{
                        "series_name":self.slot_b_ch1_key,
                        "series_data":new_data_b_ch1},
		                ],
                    self.slot_b_key2: [{
                        "series_name":self.slot_b_ch2_key,
                        "series_data":new_data_b_ch2},
                        ]
		            }
    def save_csv_option(self, option):
        self.enable_csv_logs=option
        if self.enable_csv_logs:
            print "creating."
            fstream = open(self.fname, "w+")
            fstream.write ('{},{}, {}, {}'.format('TimeStamp_ch1', 'adpd_data_SlotB_ch1', 'TimeStamp_ch2','adpd_data_SlotB_ch2\n'))
            fstream.close()
        return
class adpd3_plot(plot):
    # The name used by the user to select this plotter
    selection_name = "radpd3"
    # The plot title
    name = "ADPD400x Data"
    # Names used to refer to the two sub-plots
    slot_c_key1 = "SlotC Ch1 Data"
    slot_c_ch1_key = "SlotC Ch1"
    slot_c_key2 = "SlotC Ch2 Data"
    slot_c_ch2_key = "SlotC Ch2"
    slot_c_ch1_series_name = "ADPD SlotC Ch1"
    slot_c_ch2_series_name = "ADPD SlotC Ch2"
    ch1_sample_count = 0
    ch2_sample_count = 0
    data_ch1 = []
    data_ch2 = []
    ts_ch1 = []
    ts_ch2 = []
    # Stream address that this plot uses
    stream_addr = M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_ADPD_STREAM3
    enable_csv_logs = 0
    fname = "adpd3Stream.csv"

    def setup(self):
        # Create a subplot for slot A data
        self.add_subplot(   subplot_name = self.slot_c_key1,
                            row = 1,
                            col = 1,
                            xlabel = "Time",
                            xunit = "Samples",
                            ylabel = "Amplitude",
                            yunit = "LSBs",
                            # A single series called Slot A
                            data_series =   {  self.slot_c_ch1_key:
                                                {"format":
                                                    # Series Display name
                                                    {"name": self.slot_c_ch1_series_name,
                                                    # Colour format string for this series
                                                    "colour":"r"},
                                                # Empty data array for this series
                                                "data": np.zeros(250),
                                                },
                                            })
        # Create a subplot for slot B data
        self.add_subplot(   subplot_name = self.slot_c_key2,
                            row = 2,
                            col = 1,
                            xlabel = "Time",
                            xunit = "Samples",
                            ylabel = "Amplitude",
                            yunit = "LSBs",
                            # A single series called Slot B
                            data_series =   {  self.slot_c_ch2_key:
                                                {"format":
                                                    # Series Display name
                                                    {"name": self.slot_c_ch2_series_name,
                                                    # Colour format string for this series
                                                    "colour":"r"},
                                                # Empty data array for this series
                                                "data": np.zeros(250),
                                                },
                                            })

    def update(self, data_list):
        # Function that's called to update the plot. Should return a dictionary with each subplot's data
        new_data_ch1 = []
        new_data_ch2 = []

        for data in data_list:
            # Peek at the header to find out what kind of data we have
            hdr = m2m2_packet(0, adpd4000_data_header_t())
            hdr.unpack(data, override_size = True)
            if(hdr.payload.data_format & 0x100 == 0):
                # Decide which subplot to send this packet to
                signalSize = (hdr.payload.data_format & 0xF)
                darkSize = (hdr.payload.data_format & 0xF0) >> 4
                dataSize = signalSize + darkSize
                # Unpack all of the data into the correct packet structure
                pkt = m2m2_packet(0, m2m2_sensor_adpd4000_data_stream_t())
                pkt.unpack(data)
    
                sampleSize = dataSize
                two_channels = pkt.payload.channel_num
                # print "SampleSize:{} signalSize:{} channels:{} seq:{}".format(dataSize, signalSize, two_channels, pkt.payload.sequence_num,)
                for i in range (darkSize, pkt.payload.sample_num*dataSize, dataSize):
                    adpd_data = pkt.payload.adpddata[i]
                    for j in range (1, signalSize):
                        adpd_data |= pkt.payload.adpddata[i+j] << (j*8)
    
                    if (two_channels == 1):
                        new_data_ch1.append(adpd_data)
                        self.data_ch1.append(adpd_data)
                        self.ts_ch1.append(pkt.payload.timestamp)
                        self.ch1_sample_count += 1
                        # print "S_B Ch1 Data:{} ".format(hex(adpd_data))
                    else:
                        new_data_ch2.append(adpd_data)
                        self.data_ch2.append(adpd_data)
                        self.ts_ch2.append(pkt.payload.timestamp)
                        self.ch2_sample_count += 1
                        # print "S_B Ch2 Data:{} ".format(hex(adpd_data))

                    if self.enable_csv_logs:
                        if(self.ch1_sample_count > pkt.payload.sample_num):
                            for i in range(self.ch1_sample_count):
                                fstream = open(self.fname, "a")
                                fstream.write ('{},{},{}, {}\n'.format(self.ts_ch1[i], self.data_ch1[i], 0, 0))
                                fstream.close()
                            self.ch1_sample_count = 0
                            self.data_ch1 = []
                            self.ts_ch1 = []
                        elif(self.ch2_sample_count > pkt.payload.sample_num):
                            for i in range(self.ch2_sample_count):
                                fstream = open(self.fname, "a")
                                fstream.write ('{},{},{}, {}\n'.format(0, 0, self.ts_ch2[i], self.data_ch2[i]))
                                fstream.close()
                            self.ch2_sample_count = 0
                            self.data_ch2 = []
                            self.ts_ch2 = []
                        elif((self.ch1_sample_count == self.ch2_sample_count) and (self.ch1_sample_count == pkt.payload.sample_num)):      
                            for i in range(pkt.payload.sample_num):
                                fstream = open(self.fname, "a")
                                fstream.write ('{},{},{}, {}\n'.format(self.ts_ch1[i], self.data_ch1[i], self.ts_ch2[i], self.data_ch2[i]))
                                fstream.close()
                            self.ch1_sample_count = 0
                            self.ch2_sample_count = 0 
                            self.data_ch1 = []
                            self.data_ch2 = []
                            self.ts_ch1 = []
                            self.ts_ch2 = []

            else:
                pkt = m2m2_packet(0, m2m2_sensor_adpd4000_impulse_stream_t())
                pkt.unpack(data)
                two_channels = pkt.payload.channel_num
                signalSize = (hdr.payload.data_format & 0xff)
                for i in range (0, signalSize,2):
                    adpd_data = pkt.payload.adpddata[i]
                    adpd_data |= pkt.payload.adpddata[i+1] << 8
                    new_data_ch1.append(adpd_data)

            if (two_channels == 1):
                self.check_seq_num_cl1(3, pkt.payload.sequence_num)
            else:
                self.check_seq_num_cl2(3, pkt.payload.sequence_num)

        if len(new_data_ch1) == 0 and len(new_data_ch2) == 0:
            # No new data
            return None
        # Dictionary that associates the correct data with each subplot
        if (two_channels == 1):
            return {self.slot_c_key1: [{
                        "series_name":self.slot_c_ch1_key,
                        "series_data":new_data_ch1},
		                ]
                    }
        else:
            return {self.slot_c_key1: [{
                        "series_name":self.slot_c_ch1_key,
                        "series_data":new_data_ch1},
		                ],
                    self.slot_c_key2: [{
                        "series_name":self.slot_c_ch2_key,
                        "series_data":new_data_ch2},
                        ]
		            }
    def save_csv_option(self, option):
        self.enable_csv_logs=option
        if self.enable_csv_logs:
            print "creating."
            fstream = open(self.fname, "w+")
            fstream.write ('{},{}, {}, {}'.format('TimeStamp_ch1', 'adpd_data_SlotC_ch1', 'TimeStamp_ch2','adpd_data_SlotC_ch2\n'))
            fstream.close()
        return
class adpd4_plot(plot):
    # The name used by the user to select this plotter
    selection_name = "radpd4"
    # The plot title
    name = "ADPD400x Data"
    # Names used to refer to the two sub-plots
    slot_d_key1 = "SlotD Ch1 Data"
    slot_d_ch1_key = "SlotD Ch1"
    slot_d_key2 = "SlotD Ch2 Data"
    slot_d_ch2_key = "SlotD Ch2"
    slot_d_ch1_series_name = "ADPD SlotD Ch1"
    slot_d_ch2_series_name = "ADPD SlotD Ch2"
    ch1_sample_count = 0
    ch2_sample_count = 0
    data_ch1 = []
    data_ch2 = []
    ts_ch1 = []
    ts_ch2 = []
    # Stream address that this plot uses
    stream_addr = M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_ADPD_STREAM4
    enable_csv_logs = 0
    fname = "adpd4Stream.csv"

    def setup(self):
        # Create a subplot for slot A data
        self.add_subplot(subplot_name=self.slot_d_key1,
                         row=1,
                         col=1,
                         xlabel="Time",
                         xunit="Samples",
                         ylabel="Amplitude",
                         yunit="LSBs",
                         # A single series called Slot A
                         data_series={self.slot_d_ch1_key:
                                          {"format":
                                           # Series Display name
                                               {"name": self.slot_d_ch1_series_name,
                                                # Colour format string for this series
                                                "colour": "r"},
                                           # Empty data array for this series
                                           "data": np.zeros(250),
                                           },
                                      })
        # Create a subplot for slot B data
        self.add_subplot(subplot_name=self.slot_d_key2,
                         row=2,
                         col=1,
                         xlabel="Time",
                         xunit="Samples",
                         ylabel="Amplitude",
                         yunit="LSBs",
                         # A single series called Slot B
                         data_series={self.slot_d_ch2_key:
                                          {"format":
                                           # Series Display name
                                               {"name": self.slot_d_ch2_series_name,
                                                # Colour format string for this series
                                                "colour": "r"},
                                           # Empty data array for this series
                                           "data": np.zeros(250),
                                           },
                                      })

    def update(self, data_list):
        # Function that's called to update the plot. Should return a dictionary with each subplot's data
        new_data_ch1 = []
        new_data_ch2 = []

        for data in data_list:
            # Peek at the header to find out what kind of data we have
            hdr = m2m2_packet(0, adpd4000_data_header_t())
            hdr.unpack(data, override_size=True)
            if(hdr.payload.data_format & 0x100 == 0):
                # Decide which subplot to send this packet to
                signalSize = (hdr.payload.data_format & 0xF)
                darkSize = (hdr.payload.data_format & 0xF0) >> 4
                dataSize = signalSize + darkSize
                # Unpack all of the data into the correct packet structure
                pkt = m2m2_packet(0, m2m2_sensor_adpd4000_data_stream_t())
                pkt.unpack(data)
    
                sampleSize = dataSize
                two_channels = pkt.payload.channel_num
                # print "SampleSize:{} signalSize:{} channels:{} seq:{}".format(dataSize, signalSize, two_channels, pkt.payload.sequence_num,)
                for i in range (darkSize, pkt.payload.sample_num*dataSize, dataSize):
                    adpd_data = pkt.payload.adpddata[i]
                    for j in range (1, signalSize):
                        adpd_data |= pkt.payload.adpddata[i+j] << (j*8)
    
                    if (two_channels == 1):
                        new_data_ch1.append(adpd_data)
                        self.data_ch1.append(adpd_data)
                        self.ts_ch1.append(pkt.payload.timestamp)
                        self.ch1_sample_count += 1
                        # print "S_B Ch1 Data:{} ".format(hex(adpd_data))
                    else:
                        new_data_ch2.append(adpd_data)
                        self.data_ch2.append(adpd_data)
                        self.ts_ch2.append(pkt.payload.timestamp)
                        self.ch2_sample_count += 1
                        # print "S_B Ch2 Data:{} ".format(hex(adpd_data))

                    if self.enable_csv_logs:
                        if(self.ch1_sample_count > pkt.payload.sample_num):
                            for i in range(self.ch1_sample_count):
                                fstream = open(self.fname, "a")
                                fstream.write ('{},{},{}, {}\n'.format(self.ts_ch1[i], self.data_ch1[i], 0, 0))
                                fstream.close()
                            self.ch1_sample_count = 0
                            self.data_ch1 = []
                            self.ts_ch1 = []
                        elif(self.ch2_sample_count > pkt.payload.sample_num):
                            for i in range(self.ch2_sample_count):
                                fstream = open(self.fname, "a")
                                fstream.write ('{},{},{}, {}\n'.format(0, 0, self.ts_ch2[i], self.data_ch2[i]))
                                fstream.close()
                            self.ch2_sample_count = 0
                            self.data_ch2 = []
                            self.ts_ch2 = []
                        elif((self.ch1_sample_count == self.ch2_sample_count) and (self.ch1_sample_count == pkt.payload.sample_num)):      
                            for i in range(pkt.payload.sample_num):
                                fstream = open(self.fname, "a")
                                fstream.write ('{},{},{}, {}\n'.format(self.ts_ch1[i], self.data_ch1[i], self.ts_ch2[i], self.data_ch2[i]))
                                fstream.close()
                            self.ch1_sample_count = 0
                            self.ch2_sample_count = 0 
                            self.data_ch1 = []
                            self.data_ch2 = []
                            self.ts_ch1 = []
                            self.ts_ch2 = []

            else:
                pkt = m2m2_packet(0, m2m2_sensor_adpd4000_impulse_stream_t())
                pkt.unpack(data)
                two_channels = pkt.payload.channel_num
                signalSize = (hdr.payload.data_format & 0xff)
                for i in range (0, signalSize,2):
                    adpd_data = pkt.payload.adpddata[i]
                    adpd_data |= pkt.payload.adpddata[i+1] << 8
                    new_data_ch1.append(adpd_data)

            if (two_channels == 1):
                self.check_seq_num_cl1(3, pkt.payload.sequence_num)
            else:
                self.check_seq_num_cl2(3, pkt.payload.sequence_num)

        if len(new_data_ch1) == 0 and len(new_data_ch2) == 0:
            # No new data
            return None
        # Dictionary that associates the correct data with each subplot
        if (two_channels == 1):
            return {self.slot_d_key1: [{
                "series_name": self.slot_d_ch1_key,
                "series_data": new_data_ch1},
            ]
            }
        else:
            return {self.slot_d_key1: [{
                "series_name": self.slot_d_ch1_key,
                "series_data": new_data_ch1},
            ],
                self.slot_d_key2: [{
                    "series_name": self.slot_d_ch2_key,
                    "series_data": new_data_ch2},
                ]
            }
    def save_csv_option(self, option):
        self.enable_csv_logs=option
        if self.enable_csv_logs:
            print "creating."
            fstream = open(self.fname, "w+")
            fstream.write ('{},{}, {}, {}'.format('TimeStamp_ch1', 'adpd_data_SlotD_ch1', 'TimeStamp_ch2','adpd_data_SlotD_ch2\n'))
            fstream.close()
        return
class adpd5_plot(plot):
    # The name used by the user to select this plotter
    selection_name = "radpd5"
    # The plot title
    name = "ADPD400x Data"
    # Names used to refer to the two sub-plots
    slot_e_key1 = "SlotE Ch1 Data"
    slot_e_ch1_key = "SlotE Ch1"
    slot_e_key2 = "SlotE Ch2 Data"
    slot_e_ch2_key = "SlotE Ch2"
    slot_e_ch1_series_name = "ADPD SlotE Ch1"
    slot_e_ch2_series_name = "ADPD SlotE Ch2"
    ch1_sample_count = 0
    ch2_sample_count = 0
    data_ch1 = []
    data_ch2 = []
    ts_ch1 = []
    ts_ch2 = []
    # Stream address that this plot uses
    stream_addr = M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_ADPD_STREAM5
    enable_csv_logs = 0
    fname = "adpd5Stream.csv"

    def setup(self):
        # Create a subplot for slot A data
        self.add_subplot(subplot_name=self.slot_e_key1,
                         row=1,
                         col=1,
                         xlabel="Time",
                         xunit="Samples",
                         ylabel="Amplitude",
                         yunit="LSBs",
                         # A single series called Slot A
                         data_series={self.slot_e_ch1_key:
                                          {"format":
                                           # Series Display name
                                               {"name": self.slot_e_ch1_series_name,
                                                # Colour format string for this series
                                                "colour": "r"},
                                           # Empty data array for this series
                                           "data": np.zeros(250),
                                           },
                                      })
        # Create a subplot for slot B data
        self.add_subplot(subplot_name=self.slot_e_key2,
                         row=2,
                         col=1,
                         xlabel="Time",
                         xunit="Samples",
                         ylabel="Amplitude",
                         yunit="LSBs",
                         # A single series called Slot B
                         data_series={self.slot_e_ch2_key:
                                          {"format":
                                           # Series Display name
                                               {"name": self.slot_e_ch2_series_name,
                                                # Colour format string for this series
                                                "colour": "r"},
                                           # Empty data array for this series
                                           "data": np.zeros(250),
                                           },
                                      })

    def update(self, data_list):
        # Function that's called to update the plot. Should return a dictionary with each subplot's data
        new_data_ch1 = []
        new_data_ch2 = []

        for data in data_list:
            # Peek at the header to find out what kind of data we have
            hdr = m2m2_packet(0, adpd4000_data_header_t())
            hdr.unpack(data, override_size=True)
            if(hdr.payload.data_format & 0x100 == 0):
                # Decide which subplot to send this packet to
                signalSize = (hdr.payload.data_format & 0xF)
                darkSize = (hdr.payload.data_format & 0xF0) >> 4
                dataSize = signalSize + darkSize
                # Unpack all of the data into the correct packet structure
                pkt = m2m2_packet(0, m2m2_sensor_adpd4000_data_stream_t())
                pkt.unpack(data)
    
                sampleSize = dataSize
                two_channels = pkt.payload.channel_num
                # print "SampleSize:{} signalSize:{} channels:{} seq:{}".format(dataSize, signalSize, two_channels, pkt.payload.sequence_num,)
                for i in range (darkSize, pkt.payload.sample_num*dataSize, dataSize):
                    adpd_data = pkt.payload.adpddata[i]
                    for j in range (1, signalSize):
                        adpd_data |= pkt.payload.adpddata[i+j] << (j*8)
    
                    if (two_channels == 1):
                        new_data_ch1.append(adpd_data)
                        self.data_ch1.append(adpd_data)
                        self.ts_ch1.append(pkt.payload.timestamp)
                        self.ch1_sample_count += 1
                        # print "S_B Ch1 Data:{} ".format(hex(adpd_data))
                    else:
                        new_data_ch2.append(adpd_data)
                        self.data_ch2.append(adpd_data)
                        self.ts_ch2.append(pkt.payload.timestamp)
                        self.ch2_sample_count += 1
                        # print "S_B Ch2 Data:{} ".format(hex(adpd_data))

                    if self.enable_csv_logs:
                        if(self.ch1_sample_count > pkt.payload.sample_num):
                            for i in range(self.ch1_sample_count):
                                fstream = open(self.fname, "a")
                                fstream.write ('{},{},{}, {}\n'.format(self.ts_ch1[i], self.data_ch1[i], 0, 0))
                                fstream.close()
                            self.ch1_sample_count = 0
                            self.data_ch1 = []
                            self.ts_ch1 = []
                        elif(self.ch2_sample_count > pkt.payload.sample_num):
                            for i in range(self.ch2_sample_count):
                                fstream = open(self.fname, "a")
                                fstream.write ('{},{},{}, {}\n'.format(0, 0, self.ts_ch2[i], self.data_ch2[i]))
                                fstream.close()
                            self.ch2_sample_count = 0
                            self.data_ch2 = []
                            self.ts_ch2 = []
                        elif((self.ch1_sample_count == self.ch2_sample_count) and (self.ch1_sample_count == pkt.payload.sample_num)):      
                            for i in range(pkt.payload.sample_num):
                                fstream = open(self.fname, "a")
                                fstream.write ('{},{},{}, {}\n'.format(self.ts_ch1[i], self.data_ch1[i], self.ts_ch2[i], self.data_ch2[i]))
                                fstream.close()
                            self.ch1_sample_count = 0
                            self.ch2_sample_count = 0 
                            self.data_ch1 = []
                            self.data_ch2 = []
                            self.ts_ch1 = []
                            self.ts_ch2 = []

            else:
                pkt = m2m2_packet(0, m2m2_sensor_adpd4000_impulse_stream_t())
                pkt.unpack(data)
                two_channels = pkt.payload.channel_num
                signalSize = (hdr.payload.data_format & 0xff)
                for i in range (0, signalSize,2):
                    adpd_data = pkt.payload.adpddata[i]
                    adpd_data |= pkt.payload.adpddata[i+1] << 8
                    new_data_ch1.append(adpd_data)

            if (two_channels == 1):
                self.check_seq_num_cl1(3, pkt.payload.sequence_num)
            else:
                self.check_seq_num_cl2(3, pkt.payload.sequence_num)

        if len(new_data_ch1) == 0 and len(new_data_ch2) == 0:
            # No new data
            return None
        # Dictionary that associates the correct data with each subplot
        if (two_channels == 1):
            return {self.slot_e_key1: [{
                "series_name": self.slot_e_ch1_key,
                "series_data": new_data_ch1},
            ]
            }
        else:
            return {self.slot_e_key1: [{
                "series_name": self.slot_e_ch1_key,
                "series_data": new_data_ch1},
            ],
                self.slot_e_key2: [{
                    "series_name": self.slot_e_ch2_key,
                    "series_data": new_data_ch2},
                ]
            }
    def save_csv_option(self, option):
        self.enable_csv_logs=option
        if self.enable_csv_logs:
            print "creating."
            fstream = open(self.fname, "w+")
            fstream.write ('{},{}, {}, {}'.format('TimeStamp_ch1', 'adpd_data_SlotE_ch1', 'TimeStamp_ch2','adpd_data_SlotE_ch2\n'))
            fstream.close()
        return
class adpd6_plot(plot):
    # The name used by the user to select this plotter
    selection_name = "radpd6"
    # The plot title
    name = "ADPD400x Data"
    # Names used to refer to the two sub-plots
    slot_f_key1 = "SlotF Ch1 Data"
    slot_f_ch1_key = "SlotF Ch1"
    slot_f_key2 = "SlotF Ch2 Data"
    slot_f_ch2_key = "SlotF Ch2"
    slot_f_ch1_series_name = "ADPD SlotF Ch1"
    slot_f_ch2_series_name = "ADPD SlotF Ch2"
    ch1_sample_count = 0
    ch2_sample_count = 0
    data_ch1 = []
    data_ch2 = []
    ts_ch1 = []
    ts_ch2 = []
    # Stream address that this plot uses
    stream_addr = M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_ADPD_STREAM6
    enable_csv_logs = 0
    fname = "adpd6Stream.csv"

    def setup(self):
        # Create a subplot for slot A data
        self.add_subplot(subplot_name=self.slot_f_key1,
                         row=1,
                         col=1,
                         xlabel="Time",
                         xunit="Samples",
                         ylabel="Amplitude",
                         yunit="LSBs",
                         # A single series called Slot A
                         data_series={self.slot_f_ch1_key:
                                          {"format":
                                           # Series Display name
                                               {"name": self.slot_f_ch1_series_name,
                                                # Colour format string for this series
                                                "colour": "r"},
                                           # Empty data array for this series
                                           "data": np.zeros(250),
                                           },
                                      })
        # Create a subplot for slot B data
        self.add_subplot(subplot_name=self.slot_f_key2,
                         row=2,
                         col=1,
                         xlabel="Time",
                         xunit="Samples",
                         ylabel="Amplitude",
                         yunit="LSBs",
                         # A single series called Slot B
                         data_series={self.slot_f_ch2_key:
                                          {"format":
                                           # Series Display name
                                               {"name": self.slot_f_ch2_series_name,
                                                # Colour format string for this series
                                                "colour": "r"},
                                           # Empty data array for this series
                                           "data": np.zeros(250),
                                           },
                                      })

    def update(self, data_list):
        # Function that's called to update the plot. Should return a dictionary with each subplot's data
        new_data_ch1 = []
        new_data_ch2 = []

        for data in data_list:
            # Peek at the header to find out what kind of data we have
            hdr = m2m2_packet(0, adpd4000_data_header_t())
            hdr.unpack(data, override_size=True)
            if(hdr.payload.data_format & 0x100 == 0):
                # Decide which subplot to send this packet to
                signalSize = (hdr.payload.data_format & 0xF)
                darkSize = (hdr.payload.data_format & 0xF0) >> 4
                dataSize = signalSize + darkSize
                # Unpack all of the data into the correct packet structure
                pkt = m2m2_packet(0, m2m2_sensor_adpd4000_data_stream_t())
                pkt.unpack(data)

                sampleSize = dataSize
                two_channels = pkt.payload.channel_num
                # print "SampleSize:{} signalSize:{} channels:{} seq:{}".format(dataSize, signalSize, two_channels, pkt.payload.sequence_num,)
                for i in range (darkSize, pkt.payload.sample_num*dataSize, dataSize):
                    adpd_data = pkt.payload.adpddata[i]
                    for j in range (1, signalSize):
                        adpd_data |= pkt.payload.adpddata[i+j] << (j*8)
    
                    if (two_channels == 1):
                        new_data_ch1.append(adpd_data)
                        self.data_ch1.append(adpd_data)
                        self.ts_ch1.append(pkt.payload.timestamp)
                        self.ch1_sample_count += 1
                        # print "S_B Ch1 Data:{} ".format(hex(adpd_data))
                    else:
                        new_data_ch2.append(adpd_data)
                        self.data_ch2.append(adpd_data)
                        self.ts_ch2.append(pkt.payload.timestamp)
                        self.ch2_sample_count += 1
                        # print "S_B Ch2 Data:{} ".format(hex(adpd_data))

                    if self.enable_csv_logs:
                        if(self.ch1_sample_count > pkt.payload.sample_num):
                            for i in range(self.ch1_sample_count):
                                fstream = open(self.fname, "a")
                                fstream.write ('{},{},{}, {}\n'.format(self.ts_ch1[i], self.data_ch1[i], 0, 0))
                                fstream.close()
                            self.ch1_sample_count = 0
                            self.data_ch1 = []
                            self.ts_ch1 = []
                        elif(self.ch2_sample_count > pkt.payload.sample_num):
                            for i in range(self.ch2_sample_count):
                                fstream = open(self.fname, "a")
                                fstream.write ('{},{},{}, {}\n'.format(0, 0, self.ts_ch2[i], self.data_ch2[i]))
                                fstream.close()
                            self.ch2_sample_count = 0
                            self.data_ch2 = []
                            self.ts_ch2 = []
                        elif((self.ch1_sample_count == self.ch2_sample_count) and (self.ch1_sample_count == pkt.payload.sample_num)):      
                            for i in range(pkt.payload.sample_num):
                                fstream = open(self.fname, "a")
                                fstream.write ('{},{},{}, {}\n'.format(self.ts_ch1[i], self.data_ch1[i], self.ts_ch2[i], self.data_ch2[i]))
                                fstream.close()
                            self.ch1_sample_count = 0
                            self.ch2_sample_count = 0 
                            self.data_ch1 = []
                            self.data_ch2 = []
                            self.ts_ch1 = []
                            self.ts_ch2 = []
                    
            else:
                pkt = m2m2_packet(0, m2m2_sensor_adpd4000_impulse_stream_t())
                pkt.unpack(data)
                two_channels = pkt.payload.channel_num
                signalSize = (hdr.payload.data_format & 0xff)
                for i in range (0, signalSize,2):
                    adpd_data = pkt.payload.adpddata[i]
                    adpd_data |= pkt.payload.adpddata[i+1] << 8
                    new_data_ch1.append(adpd_data)

            if (two_channels == 1):
                self.check_seq_num_cl1(3, pkt.payload.sequence_num)
            else:
                self.check_seq_num_cl2(3, pkt.payload.sequence_num)

        if len(new_data_ch1) == 0 and len(new_data_ch2) == 0:
            # No new data
            return None
        # Dictionary that associates the correct data with each subplot
        if (two_channels == 1):
            return {self.slot_f_key1: [{
                "series_name": self.slot_f_ch1_key,
                "series_data": new_data_ch1},
            ]
            }
        else:
            return {self.slot_f_key1: [{
                "series_name": self.slot_f_ch1_key,
                "series_data": new_data_ch1},
            ],
                self.slot_f_key2: [{
                    "series_name": self.slot_f_ch2_key,
                    "series_data": new_data_ch2},
                ]
            }
    def save_csv_option(self, option):
        self.enable_csv_logs=option
        if self.enable_csv_logs:
            print "creating."
            fstream = open(self.fname, "w+")
            fstream.write ('{}, {}, {}, {}'.format('TimeStamp_ch1', 'adpd_data_SlotF_ch1', 'TimeStamp_ch2','adpd_data_SlotF_ch2\n'))
            fstream.close()
        return
class adpd7_plot(plot):
    # The name used by the user to select this plotter
    selection_name = "radpd7"
    # The plot title
    name = "ADPD400x Data"
    # Names used to refer to the two sub-plots
    slot_g_key1 = "SlotG Ch1 Data"
    slot_g_ch1_key = "SlotG Ch1"
    slot_g_ch2_key = "SlotG Ch2"
    slot_g_key2 = "SlotG Ch2 Data"
    slot_g_ch1_series_name = "ADPD SlotG Ch1"
    slot_g_ch2_series_name = "ADPD SlotG Ch2"
    ch1_sample_count = 0
    ch2_sample_count = 0
    data_ch1 = []
    data_ch2 = []
    ts_ch1 = []
    ts_ch2 = []
    # Stream address that this plot uses
    stream_addr = M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_ADPD_STREAM7
    enable_csv_logs = 0
    fname = "adpd7Stream.csv"

    def setup(self):
        # Create a subplot for slot A data
        self.add_subplot(subplot_name=self.slot_g_key1,
                         row=1,
                         col=1,
                         xlabel="Time",
                         xunit="Samples",
                         ylabel="Amplitude",
                         yunit="LSBs",
                         # A single series called Slot A
                         data_series={self.slot_g_ch1_key:
                                          {"format":
                                           # Series Display name
                                               {"name": self.slot_g_ch1_series_name,
                                                # Colour format string for this series
                                                "colour": "r"},
                                           # Empty data array for this series
                                           "data": np.zeros(250),
                                           },
                                      })
        # Create a subplot for slot B data
        self.add_subplot(subplot_name=self.slot_g_key2,
                         row=2,
                         col=1,
                         xlabel="Time",
                         xunit="Samples",
                         ylabel="Amplitude",
                         yunit="LSBs",
                         # A single series called Slot B
                         data_series={self.slot_g_ch2_key:
                                          {"format":
                                           # Series Display name
                                               {"name": self.slot_g_ch2_series_name,
                                                # Colour format string for this series
                                                "colour": "r"},
                                           # Empty data array for this series
                                           "data": np.zeros(250),
                                           },
                                      })

    def update(self, data_list):
        # Function that's called to update the plot. Should return a dictionary with each subplot's data
        new_data_ch1 = []
        new_data_ch2 = []

        for data in data_list:
            # Peek at the header to find out what kind of data we have
            hdr = m2m2_packet(0, adpd4000_data_header_t())
            hdr.unpack(data, override_size=True)
            if(hdr.payload.data_format & 0x100 == 0):
                # Decide which subplot to send this packet to
                signalSize = (hdr.payload.data_format & 0xF)
                darkSize = (hdr.payload.data_format & 0xF0) >> 4
                dataSize = signalSize + darkSize
                # Unpack all of the data into the correct packet structure
                pkt = m2m2_packet(0, m2m2_sensor_adpd4000_data_stream_t())
                pkt.unpack(data)
    
                sampleSize = dataSize
                two_channels = pkt.payload.channel_num
                # print "SampleSize:{} signalSize:{} channels:{} seq:{}".format(dataSize, signalSize, two_channels, pkt.payload.sequence_num,)
                for i in range (darkSize, pkt.payload.sample_num*dataSize, dataSize):
                    adpd_data = pkt.payload.adpddata[i]
                    for j in range (1, signalSize):
                        adpd_data |= pkt.payload.adpddata[i+j] << (j*8)
    
                    if (two_channels == 1):
                        new_data_ch1.append(adpd_data)
                        self.data_ch1.append(adpd_data)
                        self.ts_ch1.append(pkt.payload.timestamp)
                        self.ch1_sample_count += 1
                        # print "S_B Ch1 Data:{} ".format(hex(adpd_data))
                    else:
                        new_data_ch2.append(adpd_data)
                        self.data_ch2.append(adpd_data)
                        self.ts_ch2.append(pkt.payload.timestamp)
                        self.ch2_sample_count += 1
                        # print "S_B Ch2 Data:{} ".format(hex(adpd_data))

                    if self.enable_csv_logs:
                        if(self.ch1_sample_count > pkt.payload.sample_num):
                            for i in range(self.ch1_sample_count):
                                fstream = open(self.fname, "a")
                                fstream.write ('{},{},{}, {}\n'.format(self.ts_ch1[i], self.data_ch1[i], 0, 0))
                                fstream.close()
                            self.ch1_sample_count = 0
                            self.data_ch1 = []
                            self.ts_ch1 = []
                        elif(self.ch2_sample_count > pkt.payload.sample_num):
                            for i in range(self.ch2_sample_count):
                                fstream = open(self.fname, "a")
                                fstream.write ('{},{},{}, {}\n'.format(0, 0, self.ts_ch2[i], self.data_ch2[i]))
                                fstream.close()
                            self.ch2_sample_count = 0
                            self.data_ch2 = []
                            self.ts_ch2 = []
                        elif((self.ch1_sample_count == self.ch2_sample_count) and (self.ch1_sample_count == pkt.payload.sample_num)):      
                            for i in range(pkt.payload.sample_num):
                                fstream = open(self.fname, "a")
                                fstream.write ('{},{},{}, {}\n'.format(self.ts_ch1[i], self.data_ch1[i], self.ts_ch2[i], self.data_ch2[i]))
                                fstream.close()
                            self.ch1_sample_count = 0
                            self.ch2_sample_count = 0 
                            self.data_ch1 = []
                            self.data_ch2 = []
                            self.ts_ch1 = []
                            self.ts_ch2 = []

            else:
                pkt = m2m2_packet(0, m2m2_sensor_adpd4000_impulse_stream_t())
                pkt.unpack(data)
                two_channels = pkt.payload.channel_num
                signalSize = (hdr.payload.data_format & 0xff)
                for i in range (0, signalSize,2):
                    adpd_data = pkt.payload.adpddata[i]
                    adpd_data |= pkt.payload.adpddata[i+1] << 8
                    new_data_ch1.append(adpd_data)

            if (two_channels == 1):
                self.check_seq_num_cl1(3, pkt.payload.sequence_num)
            else:
                self.check_seq_num_cl2(3, pkt.payload.sequence_num)

        if len(new_data_ch1) == 0 and len(new_data_ch2) == 0:
            # No new data
            return None
        # Dictionary that associates the correct data with each subplot
        if (two_channels == 1):
            return {self.slot_g_key1: [{
                "series_name": self.slot_g_ch1_key,
                "series_data": new_data_ch1},
            ]
            }
        else:
            return {self.slot_g_key1: [{
                "series_name": self.slot_g_ch1_key,
                "series_data": new_data_ch1},
            ],
                self.slot_g_key2: [{
                    "series_name": self.slot_g_ch2_key,
                    "series_data": new_data_ch2},
                ]
            }
    def save_csv_option(self, option):
        self.enable_csv_logs=option
        if self.enable_csv_logs:
            print "creating."
            fstream = open(self.fname, "w+")
            fstream.write ('{},{}, {}, {}'.format('TimeStamp_ch1', 'adpd_data_SlotG_ch1', 'TimeStamp_ch2','adpd_data_SlotG_ch2\n'))
            fstream.close()
        return

class adpd8_plot(plot):
    # The name used by the user to select this plotter
    selection_name = "radpd8"
    # The plot title
    name = "ADPD400x Data"
    # Names used to refer to the two sub-plots
    slot_h_key1 = "SlotH Ch1 Data"
    slot_h_ch1_key = "SlotH Ch1"
    slot_h_key2 = "SlotH Ch2 Data"
    slot_h_ch2_key = "SlotH Ch2"
    slot_h_ch1_series_name = "ADPD SlotH Ch1"
    slot_h_ch2_series_name = "ADPD SlotH Ch2"
    ch1_sample_count = 0
    ch2_sample_count = 0
    data_ch1 = []
    data_ch2 = []
    ts_ch1 = []
    ts_ch2 = []
    # Stream address that this plot uses
    stream_addr = M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_ADPD_STREAM8
    enable_csv_logs = 0
    fname = "adpd8Stream.csv"

    def setup(self):
        # Create a subplot for slot A data
        self.add_subplot(subplot_name=self.slot_h_key1,
                         row=1,
                         col=1,
                         xlabel="Time",
                         xunit="Samples",
                         ylabel="Amplitude",
                         yunit="LSBs",
                         # A single series called Slot A
                         data_series={self.slot_h_ch1_key:
                                          {"format":
                                           # Series Display name
                                               {"name": self.slot_h_ch1_series_name,
                                                # Colour format string for this series
                                                "colour": "r"},
                                           # Empty data array for this series
                                           "data": np.zeros(250),
                                           },
                                      })
        # Create a subplot for slot B data
        self.add_subplot(subplot_name=self.slot_h_key2,
                         row=2,
                         col=1,
                         xlabel="Time",
                         xunit="Samples",
                         ylabel="Amplitude",
                         yunit="LSBs",
                         # A single series called Slot B
                         data_series={self.slot_h_ch2_key:
                                          {"format":
                                           # Series Display name
                                               {"name": self.slot_h_ch2_series_name,
                                                # Colour format string for this series
                                                "colour": "r"},
                                           # Empty data array for this series
                                           "data": np.zeros(250),
                                           },
                                      })

    def update(self, data_list):
        # Function that's called to update the plot. Should return a dictionary with each subplot's data
        new_data_ch1 = []
        new_data_ch2 = []

        for data in data_list:
            # Peek at the header to find out what kind of data we have
            hdr = m2m2_packet(0, adpd4000_data_header_t())
            hdr.unpack(data, override_size=True)
            if(hdr.payload.data_format & 0x100 == 0):
                # Decide which subplot to send this packet to
                signalSize = (hdr.payload.data_format & 0xF)
                darkSize = (hdr.payload.data_format & 0xF0) >> 4
                dataSize = signalSize + darkSize
                # Unpack all of the data into the correct packet structure
                pkt = m2m2_packet(0, m2m2_sensor_adpd4000_data_stream_t())
                pkt.unpack(data)
    
                sampleSize = dataSize
                two_channels = pkt.payload.channel_num
                # print "SampleSize:{} signalSize:{} channels:{} seq:{}".format(dataSize, signalSize, two_channels, pkt.payload.sequence_num,)
                for i in range (darkSize, pkt.payload.sample_num*dataSize, dataSize):
                    adpd_data = pkt.payload.adpddata[i]
                    for j in range (1, signalSize):
                        adpd_data |= pkt.payload.adpddata[i+j] << (j*8)
    
                    if (two_channels == 1):
                        new_data_ch1.append(adpd_data)
                        self.data_ch1.append(adpd_data)
                        self.ts_ch1.append(pkt.payload.timestamp)
                        self.ch1_sample_count += 1
                        # print "S_B Ch1 Data:{} ".format(hex(adpd_data))
                    else:
                        new_data_ch2.append(adpd_data)
                        self.data_ch2.append(adpd_data)
                        self.ts_ch2.append(pkt.payload.timestamp)
                        self.ch2_sample_count += 1
                        # print "S_B Ch2 Data:{} ".format(hex(adpd_data))

                    if self.enable_csv_logs:
                        if(self.ch1_sample_count > pkt.payload.sample_num):
                            for i in range(self.ch1_sample_count):
                                fstream = open(self.fname, "a")
                                fstream.write ('{},{},{}, {}\n'.format(self.ts_ch1[i], self.data_ch1[i], 0, 0))
                                fstream.close()
                            self.ch1_sample_count = 0
                            self.data_ch1 = []
                            self.ts_ch1 = []
                        elif(self.ch2_sample_count > pkt.payload.sample_num):
                            for i in range(self.ch2_sample_count):
                                fstream = open(self.fname, "a")
                                fstream.write ('{},{},{}, {}\n'.format(0, 0, self.ts_ch2[i], self.data_ch2[i]))
                                fstream.close()
                            self.ch2_sample_count = 0
                            self.data_ch2 = []
                            self.ts_ch2 = []
                        elif((self.ch1_sample_count == self.ch2_sample_count) and (self.ch1_sample_count == pkt.payload.sample_num)):      
                            for i in range(pkt.payload.sample_num):
                                fstream = open(self.fname, "a")
                                fstream.write ('{},{},{}, {}\n'.format(self.ts_ch1[i], self.data_ch1[i], self.ts_ch2[i], self.data_ch2[i]))
                                fstream.close()
                            self.ch1_sample_count = 0
                            self.ch2_sample_count = 0 
                            self.data_ch1 = []
                            self.data_ch2 = []
                            self.ts_ch1 = []
                            self.ts_ch2 = []

            else:
                pkt = m2m2_packet(0, m2m2_sensor_adpd4000_impulse_stream_t())
                pkt.unpack(data)
                two_channels = pkt.payload.channel_num
                signalSize = (hdr.payload.data_format & 0xff)
                for i in range (0, signalSize,2):
                    adpd_data = pkt.payload.adpddata[i]
                    adpd_data |= pkt.payload.adpddata[i+1] << 8
                    new_data_ch1.append(adpd_data)

            if (two_channels == 1):
                self.check_seq_num_cl1(3, pkt.payload.sequence_num)
            else:
                self.check_seq_num_cl2(3, pkt.payload.sequence_num)

        if len(new_data_ch1) == 0 and len(new_data_ch2) == 0:
            # No new data
            return None
        # Dictionary that associates the correct data with each subplot
        if (two_channels == 1):
            return {self.slot_h_key1: [{
                "series_name": self.slot_h_ch1_key,
                "series_data": new_data_ch1},
            ]
            }
        else:
            return {self.slot_h_key1: [{
                "series_name": self.slot_h_ch1_key,
                "series_data": new_data_ch1},
            ],
                self.slot_h_key2: [{
                    "series_name": self.slot_h_ch2_key,
                    "series_data": new_data_ch2},
                ]
            }
    def save_csv_option(self, option):
        self.enable_csv_logs=option
        if self.enable_csv_logs:
            print "creating."
            fstream = open(self.fname, "w+")
            fstream.write ('{},{}, {}, {}'.format('TimeStamp_ch1', 'adpd_data_SlotH_ch1', 'TimeStamp_ch2','adpd_data_SlotH_ch2\n'))
            fstream.close()
        return

class adpd9_plot(plot):
    # The name used by the user to select this plotter
    selection_name = "radpd9"
    # The plot title
    name = "ADPD400x Data"
    # Names used to refer to the two sub-plots
    slot_i_key1 = "SlotI Ch1 Data"
    slot_i_ch1_key = "SlotI Ch1"
    slot_i_key2 = "SlotI Ch2 Data"
    slot_i_ch2_key = "SlotI Ch2"
    slot_i_ch1_series_name = "ADPD SlotI Ch1"
    slot_i_ch2_series_name = "ADPD SlotI Ch2"
    ch1_sample_count = 0
    ch2_sample_count = 0
    data_ch1 = []
    data_ch2 = []
    ts_ch1 = []
    ts_ch2 = []
    # Stream address that this plot uses
    stream_addr = M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_ADPD_STREAM9
    enable_csv_logs = 0
    fname = "adpd9Stream.csv"

    def setup(self):
        # Create a subplot for slot A data
        self.add_subplot(subplot_name=self.slot_i_key1,
                         row=1,
                         col=1,
                         xlabel="Time",
                         xunit="Samples",
                         ylabel="Amplitude",
                         yunit="LSBs",
                         # A single series called Slot A
                         data_series={self.slot_i_ch1_key:
                                          {"format":
                                           # Series Display name
                                               {"name": self.slot_i_ch1_series_name,
                                                # Colour format string for this series
                                                "colour": "r"},
                                           # Empty data array for this series
                                           "data": np.zeros(250),
                                           },
                                      })
        # Create a subplot for slot B data
        self.add_subplot(subplot_name=self.slot_i_key2,
                         row=2,
                         col=1,
                         xlabel="Time",
                         xunit="Samples",
                         ylabel="Amplitude",
                         yunit="LSBs",
                         # A single series called Slot B
                         data_series={self.slot_i_ch2_key:
                                          {"format":
                                           # Series Display name
                                               {"name": self.slot_i_ch2_series_name,
                                                # Colour format string for this series
                                                "colour": "r"},
                                           # Empty data array for this series
                                           "data": np.zeros(250),
                                           },
                                      })

    def update(self, data_list):
        # Function that's called to update the plot. Should return a dictionary with each subplot's data
        new_data_ch1 = []
        new_data_ch2 = []

        for data in data_list:
            # Peek at the header to find out what kind of data we have
            hdr = m2m2_packet(0, adpd4000_data_header_t())
            hdr.unpack(data, override_size=True)
            if(hdr.payload.data_format & 0x100 == 0):
                # Decide which subplot to send this packet to
                signalSize = (hdr.payload.data_format & 0xF)
                darkSize = (hdr.payload.data_format & 0xF0) >> 4
                dataSize = signalSize + darkSize
                # Unpack all of the data into the correct packet structure
                pkt = m2m2_packet(0, m2m2_sensor_adpd4000_data_stream_t())
                pkt.unpack(data)
    
                sampleSize = dataSize
                two_channels = pkt.payload.channel_num
                # print "SampleSize:{} signalSize:{} channels:{} seq:{}".format(dataSize, signalSize, two_channels, pkt.payload.sequence_num,)
                for i in range (darkSize, pkt.payload.sample_num*dataSize, dataSize):
                    adpd_data = pkt.payload.adpddata[i]
                    for j in range (1, signalSize):
                        adpd_data |= pkt.payload.adpddata[i+j] << (j*8)
    
                    if (two_channels == 1):
                        new_data_ch1.append(adpd_data)
                        self.data_ch1.append(adpd_data)
                        self.ts_ch1.append(pkt.payload.timestamp)
                        self.ch1_sample_count += 1
                        # print "S_B Ch1 Data:{} ".format(hex(adpd_data))
                    else:
                        new_data_ch2.append(adpd_data)
                        self.data_ch2.append(adpd_data)
                        self.ts_ch2.append(pkt.payload.timestamp)
                        self.ch2_sample_count += 1
                        # print "S_B Ch2 Data:{} ".format(hex(adpd_data))

                    if self.enable_csv_logs:
                        if(self.ch1_sample_count > pkt.payload.sample_num):
                            for i in range(self.ch1_sample_count):
                                fstream = open(self.fname, "a")
                                fstream.write ('{},{},{}, {}\n'.format(self.ts_ch1[i], self.data_ch1[i], 0, 0))
                                fstream.close()
                            self.ch1_sample_count = 0
                            self.data_ch1 = []
                            self.ts_ch1 = []
                        elif(self.ch2_sample_count > pkt.payload.sample_num):
                            for i in range(self.ch2_sample_count):
                                fstream = open(self.fname, "a")
                                fstream.write ('{},{},{}, {}\n'.format(0, 0, self.ts_ch2[i], self.data_ch2[i]))
                                fstream.close()
                            self.ch2_sample_count = 0
                            self.data_ch2 = []
                            self.ts_ch2 = []
                        elif((self.ch1_sample_count == self.ch2_sample_count) and (self.ch1_sample_count == pkt.payload.sample_num)):      
                            for i in range(pkt.payload.sample_num):
                                fstream = open(self.fname, "a")
                                fstream.write ('{},{},{}, {}\n'.format(self.ts_ch1[i], self.data_ch1[i], self.ts_ch2[i], self.data_ch2[i]))
                                fstream.close()
                            self.ch1_sample_count = 0
                            self.ch2_sample_count = 0 
                            self.data_ch1 = []
                            self.data_ch2 = []
                            self.ts_ch1 = []
                            self.ts_ch2 = []

            else:
                pkt = m2m2_packet(0, m2m2_sensor_adpd4000_impulse_stream_t())
                pkt.unpack(data)
                two_channels = pkt.payload.channel_num
                signalSize = (hdr.payload.data_format & 0xff)
                for i in range (0, signalSize,2):
                    adpd_data = pkt.payload.adpddata[i]
                    adpd_data |= pkt.payload.adpddata[i+1] << 8
                    new_data_ch1.append(adpd_data)

            if (two_channels == 1):
                self.check_seq_num_cl1(3, pkt.payload.sequence_num)
            else:
                self.check_seq_num_cl2(3, pkt.payload.sequence_num)

        if len(new_data_ch1) == 0 and len(new_data_ch2) == 0:
            # No new data
            return None
        # Dictionary that associates the correct data with each subplot
        if (two_channels == 1):
            return {self.slot_i_key1: [{
                "series_name": self.slot_i_ch1_key,
                "series_data": new_data_ch1},
            ]
            }
        else:
            return {self.slot_i_key1: [{
                "series_name": self.slot_i_ch1_key,
                "series_data": new_data_ch1},
            ],
                self.slot_i_key2: [{
                    "series_name": self.slot_i_ch2_key,
                    "series_data": new_data_ch2},
                ]
            }

    def save_csv_option(self, option):
        self.enable_csv_logs=option
        if self.enable_csv_logs:
            print "creating."
            fstream = open(self.fname, "w+")
            fstream.write ('{},{}, {}, {}'.format('TimeStamp_ch1', 'adpd_data_SlotI_ch1', 'TimeStamp_ch2','adpd_data_SlotI_ch2\n'))
            fstream.close()
        return

class adpd10_plot(plot):
    # The name used by the user to select this plotter
    selection_name = "radpd10"
    # The plot title
    name = "ADPD400x Data"
    # Names used to refer to the two sub-plots
    slot_j_key1 = "SlotJ Ch1 Data"
    slot_j_ch1_key = "SlotJ Ch1"
    slot_j_key2 = "SlotJ Ch2 Data"
    slot_j_ch2_key = "SlotJ Ch2"
    slot_j_ch1_series_name = "ADPD SlotJ Ch1"
    slot_j_ch2_series_name = "ADPD SlotJ Ch2"
    ch1_sample_count = 0
    ch2_sample_count = 0
    data_ch1 = []
    data_ch2 = []
    ts_ch1 = []
    ts_ch2 = []
    # Stream address that this plot uses
    stream_addr = M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_ADPD_STREAM10
    enable_csv_logs = 0
    fname = "adpd10Stream.csv"

    def setup(self):
        # Create a subplot for slot A data
        self.add_subplot(subplot_name=self.slot_j_key1,
                         row=1,
                         col=1,
                         xlabel="Time",
                         xunit="Samples",
                         ylabel="Amplitude",
                         yunit="LSBs",
                         # A single series called Slot A
                         data_series={self.slot_j_ch1_key:
                                          {"format":
                                           # Series Display name
                                               {"name": self.slot_j_ch1_series_name,
                                                # Colour format string for this series
                                                "colour": "r"},
                                           # Empty data array for this series
                                           "data": np.zeros(250),
                                           },
                                      })
        # Create a subplot for slot B data
        self.add_subplot(subplot_name=self.slot_j_key2,
                         row=2,
                         col=1,
                         xlabel="Time",
                         xunit="Samples",
                         ylabel="Amplitude",
                         yunit="LSBs",
                         # A single series called Slot B
                         data_series={self.slot_j_ch2_key:
                                          {"format":
                                           # Series Display name
                                               {"name": self.slot_j_ch2_series_name,
                                                # Colour format string for this series
                                                "colour": "r"},
                                           # Empty data array for this series
                                           "data": np.zeros(250),
                                           },
                                      })

    def update(self, data_list):
        # Function that's called to update the plot. Should return a dictionary with each subplot's data
        new_data_ch1 = []
        new_data_ch2 = []

        for data in data_list:
            # Peek at the header to find out what kind of data we have
            hdr = m2m2_packet(0, adpd4000_data_header_t())
            hdr.unpack(data, override_size=True)
            if(hdr.payload.data_format & 0x100 == 0):
                # Decide which subplot to send this packet to
                signalSize = (hdr.payload.data_format & 0xF)
                darkSize = (hdr.payload.data_format & 0xF0) >> 4
                dataSize = signalSize + darkSize
                # Unpack all of the data into the correct packet structure
                pkt = m2m2_packet(0, m2m2_sensor_adpd4000_data_stream_t())
                pkt.unpack(data)
    
                sampleSize = dataSize
                two_channels = pkt.payload.channel_num
                # print "SampleSize:{} signalSize:{} channels:{} seq:{}".format(dataSize, signalSize, two_channels, pkt.payload.sequence_num,)
                for i in range (darkSize, pkt.payload.sample_num*dataSize, dataSize):
                    adpd_data = pkt.payload.adpddata[i]
                    for j in range (1, signalSize):
                        adpd_data |= pkt.payload.adpddata[i+j] << (j*8)
    
                    if (two_channels == 1):
                        new_data_ch1.append(adpd_data)
                        self.data_ch1.append(adpd_data)
                        self.ts_ch1.append(pkt.payload.timestamp)
                        self.ch1_sample_count += 1
                        # print "S_B Ch1 Data:{} ".format(hex(adpd_data))
                    else:
                        new_data_ch2.append(adpd_data)
                        self.data_ch2.append(adpd_data)
                        self.ts_ch2.append(pkt.payload.timestamp)
                        self.ch2_sample_count += 1
                        # print "S_B Ch2 Data:{} ".format(hex(adpd_data))

                    if self.enable_csv_logs:
                        if(self.ch1_sample_count > pkt.payload.sample_num):
                            for i in range(self.ch1_sample_count):
                                fstream = open(self.fname, "a")
                                fstream.write ('{},{},{}, {}\n'.format(self.ts_ch1[i], self.data_ch1[i], 0, 0))
                                fstream.close()
                            self.ch1_sample_count = 0
                            self.data_ch1 = []
                            self.ts_ch1 = []
                        elif(self.ch2_sample_count > pkt.payload.sample_num):
                            for i in range(self.ch2_sample_count):
                                fstream = open(self.fname, "a")
                                fstream.write ('{},{},{}, {}\n'.format(0, 0, self.ts_ch2[i], self.data_ch2[i]))
                                fstream.close()
                            self.ch2_sample_count = 0
                            self.data_ch2 = []
                            self.ts_ch2 = []
                        elif((self.ch1_sample_count == self.ch2_sample_count) and (self.ch1_sample_count == pkt.payload.sample_num)):      
                            for i in range(pkt.payload.sample_num):
                                fstream = open(self.fname, "a")
                                fstream.write ('{},{},{}, {}\n'.format(self.ts_ch1[i], self.data_ch1[i], self.ts_ch2[i], self.data_ch2[i]))
                                fstream.close()
                            self.ch1_sample_count = 0
                            self.ch2_sample_count = 0 
                            self.data_ch1 = []
                            self.data_ch2 = []
                            self.ts_ch1 = []
                            self.ts_ch2 = []

            else:
                pkt = m2m2_packet(0, m2m2_sensor_adpd4000_impulse_stream_t())
                pkt.unpack(data)
                two_channels = pkt.payload.channel_num
                signalSize = (hdr.payload.data_format & 0xff)
                for i in range (0, signalSize,2):
                    adpd_data = pkt.payload.adpddata[i]
                    adpd_data |= pkt.payload.adpddata[i+1] << 8
                    new_data_ch1.append(adpd_data)

            if (two_channels == 1):
                self.check_seq_num_cl1(3, pkt.payload.sequence_num)
            else:
                self.check_seq_num_cl2(3, pkt.payload.sequence_num)

        if len(new_data_ch1) == 0 and len(new_data_ch2) == 0:
            # No new data
            return None
        # Dictionary that associates the correct data with each subplot
        if (two_channels == 1):
            return {self.slot_j_key1: [{
                "series_name": self.slot_j_ch1_key,
                "series_data": new_data_ch1},
            ]
            }
        else:
            return {self.slot_j_key1: [{
                "series_name": self.slot_d_ch1_key,
                "series_data": new_data_ch1},
            ],
                self.slot_j_key2: [{
                    "series_name": self.slot_j_ch2_key,
                    "series_data": new_data_ch2},
                ]
            }

    def save_csv_option(self, option):
        self.enable_csv_logs=option
        if self.enable_csv_logs:
            print "creating."
            fstream = open(self.fname, "w+")
            fstream.write ('{},{}, {}, {}'.format('TimeStamp_ch1', 'adpd_data_SlotJ_ch1', 'TimeStamp_ch2','adpd_data_SlotJ_ch2\n'))
            fstream.close()
        return

class adpd11_plot(plot):
    # The name used by the user to select this plotter
    selection_name = "radpd11"
    # The plot title
    name = "ADPD400x Data"
    # Names used to refer to the two sub-plots
    slot_k_key1 = "SlotK Ch1 Data"
    slot_k_ch1_key = "SlotK Ch1"
    slot_k_key2 = "SlotK Ch2 Data"
    slot_k_ch2_key = "SlotK Ch2"
    slot_k_ch1_series_name = "ADPD SlotK Ch1"
    slot_k_ch2_series_name = "ADPD SlotK Ch2"
    ch1_sample_count = 0
    ch2_sample_count = 0
    data_ch1 = []
    data_ch2 = []
    ts_ch1 = []
    ts_ch2 = []
    # Stream address that this plot uses
    stream_addr = M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_ADPD_STREAM11
    enable_csv_logs = 0
    fname = "adpd11Stream.csv"

    def setup(self):
        # Create a subplot for slot A data
        self.add_subplot(subplot_name=self.slot_k_key1,
                         row=1,
                         col=1,
                         xlabel="Time",
                         xunit="Samples",
                         ylabel="Amplitude",
                         yunit="LSBs",
                         # A single series called Slot A
                         data_series={self.slot_k_ch1_key:
                                          {"format":
                                           # Series Display name
                                               {"name": self.slot_k_ch1_series_name,
                                                # Colour format string for this series
                                                "colour": "r"},
                                           # Empty data array for this series
                                           "data": np.zeros(250),
                                           },
                                      })
        # Create a subplot for slot B data
        self.add_subplot(subplot_name=self.slot_k_key2,
                         row=2,
                         col=1,
                         xlabel="Time",
                         xunit="Samples",
                         ylabel="Amplitude",
                         yunit="LSBs",
                         # A single series called Slot B
                         data_series={self.slot_k_ch2_key:
                                          {"format":
                                           # Series Display name
                                               {"name": self.slot_k_ch2_series_name,
                                                # Colour format string for this series
                                                "colour": "r"},
                                           # Empty data array for this series
                                           "data": np.zeros(250),
                                           },
                                      })

    def update(self, data_list):
        # Function that's called to update the plot. Should return a dictionary with each subplot's data
        new_data_ch1 = []
        new_data_ch2 = []

        for data in data_list:
            # Peek at the header to find out what kind of data we have
            hdr = m2m2_packet(0, adpd4000_data_header_t())
            hdr.unpack(data, override_size=True)
            if(hdr.payload.data_format & 0x100 == 0):
                # Decide which subplot to send this packet to
                signalSize = (hdr.payload.data_format & 0xF)
                darkSize = (hdr.payload.data_format & 0xF0) >> 4
                dataSize = signalSize + darkSize
                # Unpack all of the data into the correct packet structure
                pkt = m2m2_packet(0, m2m2_sensor_adpd4000_data_stream_t())
                pkt.unpack(data)
    
                sampleSize = dataSize
                two_channels = pkt.payload.channel_num
                # print "SampleSize:{} signalSize:{} channels:{} seq:{}".format(dataSize, signalSize, two_channels, pkt.payload.sequence_num,)
                for i in range (darkSize, pkt.payload.sample_num*dataSize, dataSize):
                    adpd_data = pkt.payload.adpddata[i]
                    for j in range (1, signalSize):
                        adpd_data |= pkt.payload.adpddata[i+j] << (j*8)
    
                    if (two_channels == 1):
                        new_data_ch1.append(adpd_data)
                        self.data_ch1.append(adpd_data)
                        self.ts_ch1.append(pkt.payload.timestamp)
                        self.ch1_sample_count += 1
                        # print "S_B Ch1 Data:{} ".format(hex(adpd_data))
                    else:
                        new_data_ch2.append(adpd_data)
                        self.data_ch2.append(adpd_data)
                        self.ts_ch2.append(pkt.payload.timestamp)
                        self.ch2_sample_count += 1
                        # print "S_B Ch2 Data:{} ".format(hex(adpd_data))

                    if self.enable_csv_logs:
                        if(self.ch1_sample_count > pkt.payload.sample_num):
                            for i in range(self.ch1_sample_count):
                                fstream = open(self.fname, "a")
                                fstream.write ('{},{},{}, {}\n'.format(self.ts_ch1[i], self.data_ch1[i], 0, 0))
                                fstream.close()
                            self.ch1_sample_count = 0
                            self.data_ch1 = []
                            self.ts_ch1 = []
                        elif(self.ch2_sample_count > pkt.payload.sample_num):
                            for i in range(self.ch2_sample_count):
                                fstream = open(self.fname, "a")
                                fstream.write ('{},{},{}, {}\n'.format(0, 0, self.ts_ch2[i], self.data_ch2[i]))
                                fstream.close()
                            self.ch2_sample_count = 0
                            self.data_ch2 = []
                            self.ts_ch2 = []
                        elif((self.ch1_sample_count == self.ch2_sample_count) and (self.ch1_sample_count == pkt.payload.sample_num)):      
                            for i in range(pkt.payload.sample_num):
                                fstream = open(self.fname, "a")
                                fstream.write ('{},{},{}, {}\n'.format(self.ts_ch1[i], self.data_ch1[i], self.ts_ch2[i], self.data_ch2[i]))
                                fstream.close()
                            self.ch1_sample_count = 0
                            self.ch2_sample_count = 0 
                            self.data_ch1 = []
                            self.data_ch2 = []
                            self.ts_ch1 = []
                            self.ts_ch2 = []

            else:
                pkt = m2m2_packet(0, m2m2_sensor_adpd4000_impulse_stream_t())
                pkt.unpack(data)
                two_channels = pkt.payload.channel_num
                signalSize = (hdr.payload.data_format & 0xff)
                for i in range (0, signalSize,2):
                    adpd_data = pkt.payload.adpddata[i]
                    adpd_data |= pkt.payload.adpddata[i+1] << 8
                    new_data_ch1.append(adpd_data)

            if (two_channels == 1):
                self.check_seq_num_cl1(3, pkt.payload.sequence_num)
            else:
                self.check_seq_num_cl2(3, pkt.payload.sequence_num)

        if len(new_data_ch1) == 0 and len(new_data_ch2) == 0:
            # No new data
            return None
        # Dictionary that associates the correct data with each subplot
        if (two_channels == 1):
            return {self.slot_k_key1: [{
                "series_name": self.slot_k_ch1_key,
                "series_data": new_data_ch1},
            ]
            }
        else:
            return {self.slot_k_key1: [{
                "series_name": self.slot_k_ch1_key,
                "series_data": new_data_ch1},
            ],
                self.slot_k_key2: [{
                    "series_name": self.slot_k_ch2_key,
                    "series_data": new_data_ch2},
                ]
            }

    def save_csv_option(self, option):
        self.enable_csv_logs=option
        if self.enable_csv_logs:
            print "creating."
            fstream = open(self.fname, "w+")
            fstream.write ('{},{}, {}, {}'.format('TimeStamp_ch1', 'adpd_data_SlotK_ch1', 'TimeStamp_ch2','adpd_data_SlotK_ch2\n'))
            fstream.close()
        return
		
class adpd12_plot(plot):
    # The name used by the user to select this plotter
    selection_name = "radpd12"
    # The plot title
    name = "ADPD400x Data"
    # Names used to refer to the two sub-plots
    slot_l_key1 = "SlotL Ch1 Data"
    slot_l_ch1_key = "SlotL Ch1"
    slot_l_key2 = "SlotL Ch2 Data"
    slot_l_ch2_key = "SlotL Ch2"
    slot_l_ch1_series_name = "ADPD SlotL Ch1"
    slot_l_ch2_series_name = "ADPD SlotL Ch2"
    ch1_sample_count = 0
    ch2_sample_count = 0
    data_ch1 = []
    data_ch2 = []
    ts_ch1 = []
    ts_ch2 = []
    # Stream address that this plot uses
    stream_addr = M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_ADPD_STREAM12
    enable_csv_logs = 0
    fname = "adpd12Stream.csv"

    def setup(self):
        # Create a subplot for slot A data
        self.add_subplot(subplot_name=self.slot_l_key1,
                         row=1,
                         col=1,
                         xlabel="Time",
                         xunit="Samples",
                         ylabel="Amplitude",
                         yunit="LSBs",
                         # A single series called Slot A
                         data_series={self.slot_l_ch1_key:
                                          {"format":
                                           # Series Display name
                                               {"name": self.slot_l_ch1_series_name,
                                                # Colour format string for this series
                                                "colour": "r"},
                                           # Empty data array for this series
                                           "data": np.zeros(250),
                                           },
                                      })
        # Create a subplot for slot B data
        self.add_subplot(subplot_name=self.slot_l_key2,
                         row=2,
                         col=1,
                         xlabel="Time",
                         xunit="Samples",
                         ylabel="Amplitude",
                         yunit="LSBs",
                         # A single series called Slot B
                         data_series={self.slot_l_ch2_key:
                                          {"format":
                                           # Series Display name
                                               {"name": self.slot_l_ch2_series_name,
                                                # Colour format string for this series
                                                "colour": "r"},
                                           # Empty data array for this series
                                           "data": np.zeros(250),
                                           },
                                      })

    def update(self, data_list):
        # Function that's called to update the plot. Should return a dictionary with each subplot's data
        new_data_ch1 = []
        new_data_ch2 = []

        for data in data_list:
            # Peek at the header to find out what kind of data we have
            hdr = m2m2_packet(0, adpd4000_data_header_t())
            hdr.unpack(data, override_size=True)
            if(hdr.payload.data_format & 0x100 == 0):
                # Decide which subplot to send this packet to
                signalSize = (hdr.payload.data_format & 0xF)
                darkSize = (hdr.payload.data_format & 0xF0) >> 4
                dataSize = signalSize + darkSize
                # Unpack all of the data into the correct packet structure
                pkt = m2m2_packet(0, m2m2_sensor_adpd4000_data_stream_t())
                pkt.unpack(data)
    
                sampleSize = dataSize
                two_channels = pkt.payload.channel_num
                # print "SampleSize:{} signalSize:{} channels:{} seq:{}".format(dataSize, signalSize, two_channels, pkt.payload.sequence_num,)
                for i in range (darkSize, pkt.payload.sample_num*dataSize, dataSize):
                    adpd_data = pkt.payload.adpddata[i]
                    for j in range (1, signalSize):
                        adpd_data |= pkt.payload.adpddata[i+j] << (j*8)
    
                    if (two_channels == 1):
                        new_data_ch1.append(adpd_data)
                        self.data_ch1.append(adpd_data)
                        self.ts_ch1.append(pkt.payload.timestamp)
                        self.ch1_sample_count += 1
                        # print "S_B Ch1 Data:{} ".format(hex(adpd_data))
                    else:
                        new_data_ch2.append(adpd_data)
                        self.data_ch2.append(adpd_data)
                        self.ts_ch2.append(pkt.payload.timestamp)
                        self.ch2_sample_count += 1
                        # print "S_B Ch2 Data:{} ".format(hex(adpd_data))

                    if self.enable_csv_logs:
                        if(self.ch1_sample_count > pkt.payload.sample_num):
                            for i in range(self.ch1_sample_count):
                                fstream = open(self.fname, "a")
                                fstream.write ('{},{},{}, {}\n'.format(self.ts_ch1[i], self.data_ch1[i], 0, 0))
                                fstream.close()
                            self.ch1_sample_count = 0
                            self.data_ch1 = []
                            self.ts_ch1 = []
                        elif(self.ch2_sample_count > pkt.payload.sample_num):
                            for i in range(self.ch2_sample_count):
                                fstream = open(self.fname, "a")
                                fstream.write ('{},{},{}, {}\n'.format(0, 0, self.ts_ch2[i], self.data_ch2[i]))
                                fstream.close()
                            self.ch2_sample_count = 0
                            self.data_ch2 = []
                            self.ts_ch2 = []
                        elif((self.ch1_sample_count == self.ch2_sample_count) and (self.ch1_sample_count == pkt.payload.sample_num)):      
                            for i in range(pkt.payload.sample_num):
                                fstream = open(self.fname, "a")
                                fstream.write ('{},{},{}, {}\n'.format(self.ts_ch1[i], self.data_ch1[i], self.ts_ch2[i], self.data_ch2[i]))
                                fstream.close()
                            self.ch1_sample_count = 0
                            self.ch2_sample_count = 0 
                            self.data_ch1 = []
                            self.data_ch2 = []
                            self.ts_ch1 = []
                            self.ts_ch2 = []

            else:
                pkt = m2m2_packet(0, m2m2_sensor_adpd4000_impulse_stream_t())
                pkt.unpack(data)
                two_channels = pkt.payload.channel_num
                signalSize = (hdr.payload.data_format & 0xff)
                for i in range (0, signalSize,2):
                    adpd_data = pkt.payload.adpddata[i]
                    adpd_data |= pkt.payload.adpddata[i+1] << 8
                    new_data_ch1.append(adpd_data)

            if (two_channels == 1):
                self.check_seq_num_cl1(3, pkt.payload.sequence_num)
            else:
                self.check_seq_num_cl2(3, pkt.payload.sequence_num)

        if len(new_data_ch1) == 0 and len(new_data_ch2) == 0:
            # No new data
            return None
        # Dictionary that associates the correct data with each subplot
        if (two_channels == 1):
            return {self.slot_l_key1: [{
                "series_name": self.slot_l_ch1_key,
                "series_data": new_data_ch1},
            ]
            }
        else:
            return {self.slot_l_key1: [{
                "series_name": self.slot_l_ch1_key,
                "series_data": new_data_ch1},
            ],
                self.slot_l_key2: [{
                    "series_name": self.slot_l_ch2_key,
                    "series_data": new_data_ch2},
                ]
            }

    def save_csv_option(self, option):
        self.enable_csv_logs=option
        if self.enable_csv_logs:
            print "creating."
            fstream = open(self.fname, "w+")
            fstream.write ('{},{}, {}, {}'.format('TimeStamp_ch1', 'adpd_data_SlotL_ch1', 'TimeStamp_ch2','adpd_data_SlotL_ch2\n'))
            fstream.close()
        return

class adxl_plot(plot):
    selection_name = "radxl"
    name = "ADXL Data"
    adxl_name = "ADXL"
    x_key = "X"
    y_key = "Y"
    z_key = "Z"
    x_series_name = "X"
    y_series_name = "Y"
    z_series_name = "Z"
    stream_addr = M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_ADXL_STREAM
    enable_csv_logs = 0
    fname = "adxlStream.csv"
    fstream = None
    update_pkt_loss('radxl', False)
    
    def __del__(self):
        if self.fstream:
            self.fstream.close()
    
    def setup(self):
        self.add_subplot(   subplot_name = self.adxl_name,
                            row = 1,
                            col = 1,
                            xlabel = "Time",
                            xunit = "Samples",
                            ylabel = "Amplitude",
                            yunit = "LSBs",
                            data_series =   {  self.x_key:
                                                {"format":
                                                    {"name": self.x_series_name,
                                                    "colour":"r"},
                                                "data": np.zeros(250),
                                                },
                                            self.y_key:
                                                {"format":
                                                    {"name": self.y_series_name,
                                                    "colour":"g"},
                                                "data": np.zeros(250),
                                                },
                                            self.z_key:
                                                {"format":
                                                    {"name": self.z_series_name,
                                                    "colour":"b"},
                                                "data": np.zeros(250),
                                                }
                                            })

    def update(self, data_list):
        # Function that's called to update the plot. Should return a dictionary with each subplot's data
        new_data_x = []
        new_data_y = []
        new_data_z = []

        for data in data_list:
            # Peek at the header to find out what kind of data we have
            pkt = m2m2_packet(0, m2m2_sensor_adxl_data_no_compress_stream_t())
            pkt.unpack(data)
            self.check_seq_num(pkt.payload.sequence_num, 'radxl')
            pkt.payload.first_xdata = pkt.payload.first_xdata | ((pkt.payload.first_xdata & 0x3000) << 2)
            pkt.payload.first_ydata = pkt.payload.first_ydata | ((pkt.payload.first_ydata & 0x3000) << 2)
            pkt.payload.first_zdata = pkt.payload.first_zdata | ((pkt.payload.first_zdata & 0x3000) << 2)
            new_data_x.append(pkt.payload.first_xdata)
            new_data_y.append(pkt.payload.first_ydata)
            new_data_z.append(pkt.payload.first_zdata)
            # print "Plot Data.: {} {} {}".format(pkt.payload.first_xdata, pkt.payload.first_ydata, pkt.payload.first_zdata)
            ts = pkt.payload.timestamp
            if self.enable_csv_logs:
                # fstream = open(self.fname, "a")
                self.fstream.write ('{}, {},{},{}\n'.format(ts, pkt.payload.first_xdata, pkt.payload.first_ydata, pkt.payload.first_zdata))
                # fstream.close()
            for adxl_data in pkt.payload.adxldata:
                adxl_data.xdata = adxl_data.xdata | ((adxl_data.xdata & 0x3000) << 2)
                adxl_data.ydata = adxl_data.ydata | ((adxl_data.ydata & 0x3000) << 2)
                adxl_data.zdata = adxl_data.zdata | ((adxl_data.zdata & 0x3000) << 2)
                new_data_x.append(adxl_data.xdata)
                new_data_y.append(adxl_data.ydata)
                new_data_z.append(adxl_data.zdata)
                # print "Plot Data.: {} {} {}".format(adxl_data.xdata,adxl_data.ydata,adxl_data.zdata)
                ts = ts + adxl_data.incTS
                if self.enable_csv_logs:
                    # fstream = open(self.fname, "a")
                    self.fstream.write ('{},{},{},{}\n'.format(ts, adxl_data.xdata,adxl_data.ydata,adxl_data.zdata))
                    # fstream.close()
            if len(new_data_x) == 0 and len(new_data_y) == 0 and len(new_data_z) == 0:
                return None

        return {self.adxl_name: [{
                    "series_name":self.x_key,
                    "series_data":new_data_x},
                    {"series_name":self.y_key,
                    "series_data":new_data_y},
                    {"series_name":self.z_key,
                    "series_data":new_data_z},
                    ]
                }

    def save_csv_option(self, option):
        self.enable_csv_logs=option
        if self.enable_csv_logs:
            print "creating."
            self.fstream = open(self.fname, "w+")
            self.fstream.write ('TimeStamp, adxl_xdata, adxl_ydata, adxl_zdata\n')
            # fstream.close()
        return

class ppg_plot(plot):
    # The name used by the user to select this plotter
    selection_name = "rppg"
    # The plot title
    name = "PPG Data"
    # Names used to refer to the three sub-plots
    ppg_hr_key = "HR"
    ppg_confidence_key = "Confidence"
    ppg_hr_type_key = "HR Type"
    ppg_hr_series_name = "PPG HR"
    ppg_confidence_series_name = "PPG Confidence"
    ppg_hr_type_series_name = "PPG HR Type"
    # Stream address that this plot uses
    stream_addr = M2M2_ADDR_ENUM_t.M2M2_ADDR_MED_PPG_STREAM
    enable_csv_logs = 0
    fname = "ppgStream.csv"
    fstream = None
    update_pkt_loss('rppg', False)
    
    def __del__(self):
        if self.fstream:
            self.fstream.close()
    
    def setup(self):
        # Create a subplot for PPG HR data
        self.add_subplot(   subplot_name = self.ppg_hr_key,
                            row = 1,
                            col = 1,
                            xlabel = "Time",
                            xunit = "Samples",
                            ylabel = "Value",
                            yunit = "LSBs",
                            # A single series called PPG HR data
                            data_series =   {  self.ppg_hr_key:
                                                {"format":
                                                    # Series Display name
                                                    {"name": self.ppg_hr_series_name,
                                                    # Colour format string for this series
                                                    "colour":"r"},
                                                    # Empty data array for this series
                                                    "data": np.zeros(60),
                                                }
                                            })
        # Create a subplot for PPG Confidence data
        self.add_subplot(   subplot_name = self.ppg_confidence_key,
                            row = 2,
                            col = 1,
                            xlabel = "Time",
                            xunit = "Samples",
                            ylabel = "Amplitude",
                            yunit = "LSBs",
                            data_series =   {  self.ppg_confidence_key:     # Series name
                                                {"format":      # Formatting for the series
                                                    {"name": self.ppg_confidence_series_name, # Display name
                                                    "colour":"g"},          # Line display parameters
                                                    "data": np.zeros(60),      # Data array
                                                }
                                            })
        # Create a subplot for PPG HR Type data
        self.add_subplot(   subplot_name = self.ppg_hr_type_key,
                            row = 3,
                            col = 1,
                            xlabel = "Time",
                            xunit = "Samples",
                            ylabel = "Amplitude",
                            yunit = "LSBs",
                            data_series =   {  self.ppg_hr_type_key:     # Series name
                                                {"format":      # Formatting for the series
                                                    {"name": self.ppg_hr_type_series_name, # Display name
                                                    "colour":"b"},          # Line display parameters
                                                    "data": np.zeros(60),      # Data array
                                                }
                                            })

    def update(self, data_list):
        # Function that's called to update the plot. Should return a dictionary with each subplot's data
        new_ppg_hr = []
        new_ppg_confidence = []
        new_ppg_hr_type = []

        for data in data_list:
            # Peek at the header to find out what kind of data we have
            pkt = m2m2_packet(0, ppg_app_hr_debug_stream_t())
            pkt.unpack(data)
            hr_in_float = FIXED16Q4_TO_FLOAT(pkt.payload.hr)
            new_ppg_hr.append(hr_in_float)
            confidence_in_float = (float(100) * (FIXED16Q10_TO_FLOAT(pkt.payload.confidence)))
            new_ppg_confidence.append(confidence_in_float)
            new_ppg_hr_type.append(pkt.payload.hr_type)
            if self.enable_csv_logs:
                # fstream = open(self.fname, "a")
                self.fstream.write ('{}, {},{},{}\n'.format(pkt.payload.timestamp, hr_in_float,confidence_in_float,pkt.payload.hr_type))
                # fstream.close()
            self.check_seq_num(pkt.payload.sequence_num, 'rppg')

        if len(new_ppg_hr) == 0 and len(new_ppg_confidence) == 0 and len(new_ppg_hr_type) == 0:
            # No new data
            return None
        # Dictionary that associates the correct data with each subplot
        return {self.ppg_hr_key: [{
                    "series_name":self.ppg_hr_key,
                    "series_data":new_ppg_hr},
                    ],
                self.ppg_confidence_key: [{
                    "series_name":self.ppg_confidence_key,
                    "series_data":new_ppg_confidence},
                    ],
                self.ppg_hr_type_key: [{
                    "series_name":self.ppg_hr_type_key,
                    "series_data":new_ppg_hr_type},
                    ]
                }

    def save_csv_option(self, option):
        self.enable_csv_logs=option
        if self.enable_csv_logs:
            print "creating."
            self.fstream = open(self.fname, "w+")
            self.fstream.write ('TimeStamp, hr_in_float,confidence_in_float,hr_type\n')
            # fstream.close()
        return

class eda_plot(plot):
    selection_name = "reda"
    name = "EDA Data Plot"
    fname = "EdaAppStream.csv"
    enable_csv_logs = 0
    fstream = None
    admitance_real_key = "Real Admitance"
    admitance_real_series_name = "Admitance real Data"
    admitance_img_key = "Img Admitance"
    admitance_img_series_name = "Admitance img Data"
    admitance_mod_key = "Admitance Module "
    admitance_mod_series_name = "Admitance module Data"
    admitance_phase_key = "Admitance Phase"
    admitance_phase_series_name = "Admitance phase Data"

    impedance_real_key = "Real Impedance"
    impedance_real_series_name = "Impedance real Data"
    impedance_img_key = "Img Impedance"
    impedance_img_series_name = "Impedance img Data"
    impedance_mod_key = "Impedance Module "
    impedance_mod_series_name = "Impedance module Data"
    impedance_phase_key = "Impedance Phase"
    impedance_phase_series_name = "Impedance phase Data"

    stream_addr = M2M2_ADDR_ENUM_t.M2M2_ADDR_MED_EDA_STREAM
    update_pkt_loss('reda', False)
    
    def __del__(self):
        if self.fstream:
            self.fstream.close()
    
    def setup(self):
        self.add_subplot(   subplot_name = self.admitance_real_key,
                            row = 1,
                            col = 1,
                            #title = "Admitance real",
                            xlabel = "Time",
                            xunit = "Samples",
                            ylabel = "Value",
                            yunit = "Siemens",
                            data_series =   {  self.admitance_real_key:
                                                {"format":
                                                    {"name": self.admitance_real_series_name,
                                                    "colour":"r"},
                                                    "data": np.zeros(200),
                                                },
                                            })
        self.add_subplot(   subplot_name = self.impedance_real_key,
                            row = 1,
                            col = 2,
                            #title = "Admitance real",
                            xlabel = "Time",
                            xunit = "Samples",
                            ylabel = "Value",
                            yunit = "Ohms",
                            data_series =   {  self.impedance_real_key:
                                                {"format":
                                                    {"name": self.impedance_real_series_name,
                                                    "colour":"r"},
                                                    "data": np.zeros(200),
                                                },
                                            })
        self.add_subplot(   subplot_name = self.admitance_img_key,
                            row = 2,
                            col = 1,
                            #title = "Admitance img",
                            xlabel = "Time",
                            xunit = "Samples",
                            ylabel = "Amplitude",
                            yunit = "Siemens",
                            data_series =   {  self.admitance_img_key:     # Series name
                                                {"format":      # Formatting for the series
                                                    {"name": self.admitance_img_series_name, # Display name
                                                    "colour":"g"},          # Line display parameters
                                                    "data": np.zeros(200),      # Data array
                                                }
                                            })
        self.add_subplot(   subplot_name = self.impedance_img_key,
                            row = 2,
                            col = 2,
                            #title = "Admitance real",
                            xlabel = "Time",
                            xunit = "Samples",
                            ylabel = "Value",
                            yunit = "Ohms",
                            data_series =   {  self.impedance_img_key:
                                                {"format":
                                                    {"name": self.impedance_img_series_name,
                                                    "colour":"g"},
                                                    "data": np.zeros(200),
                                                },
                                            })
        self.add_subplot(   subplot_name = self.admitance_mod_key,
                            row = 3,
                            col = 1,
                            #title = "Admitance magnitude",
                            xlabel = "Time",
                            xunit = "Samples",
                            ylabel = "Amplitude",
                            yunit = "Siemens",
                            data_series =   {  self.admitance_mod_key:     # Series name
                                                {"format":      # Formatting for the series
                                                    {"name": self.admitance_mod_series_name, # Display name
                                                    "colour":"b"},          # Line display parameters
                                                    "data": np.zeros(200),      # Data array
                                                }
                                            })
        self.add_subplot(   subplot_name = self.impedance_mod_key,
                            row = 3,
                            col = 2,
                            #title = "Admitance magnitude",
                            xlabel = "Time",
                            xunit = "Samples",
                            ylabel = "Amplitude",
                            yunit = "Ohms",
                            data_series =   {  self.impedance_mod_key:     # Series name
                                                {"format":      # Formatting for the series
                                                    {"name": self.impedance_mod_series_name, # Display name
                                                    "colour":"b"},          # Line display parameters
                                                    "data": np.zeros(200),      # Data array
                                                }
                                            })
        self.add_subplot(   subplot_name = self.admitance_phase_key,
                            row = 4,
                            col = 1,
                            #title = "Admitance phase",
                            xlabel = "Time",
                            xunit = "Samples",
                            ylabel = "Amplitude",
                            yunit = "Radians",
                            data_series =   {  self.admitance_phase_key:     # Series name
                                                {"format":      # Formatting for the series
                                                    {"name": self.admitance_phase_series_name, # Display name
                                                    "colour":"y"},          # Line display parameters
                                                    "data": np.zeros(200),      # Data array
                                                }
                                            })
        self.add_subplot(   subplot_name = self.impedance_phase_key,
                            row = 4,
                            col = 2,
                            #title = "Admitance magnitude",
                            xlabel = "Time",
                            xunit = "Samples",
                            ylabel = "Amplitude",
                            yunit = "Radians",
                            data_series =   {  self.impedance_phase_key:     # Series name
                                                {"format":      # Formatting for the series
                                                    {"name": self.impedance_phase_series_name, # Display name
                                                    "colour":"y"},          # Line display parameters
                                                    "data": np.zeros(200),      # Data array
                                                }
                                            })

    def update(self, data_list):
        # Function that's called to update the plot. Should return a dictionary with each subplot's data
        new_data_admitance_real = []
        new_data_admitance_img = []
        new_data_admitance_module = []
        new_data_admitance_phase = []

        new_data_impedance_real = []
        new_data_impedance_img = []
        new_data_impedance_module = []
        new_data_impedance_phase = []

        for data in data_list:
            # Peek at the header to find out what kind of data we have
            pkt = m2m2_packet(0, eda_app_stream_t())
            pkt.unpack(data)
            self.check_seq_num(pkt.payload.sequence_num, 'reda') 
            for my_eda_data in pkt.payload.eda_data:
                #eda data calculation

                if my_eda_data.realdata == 0:
                    my_eda_data.realdata = 1

                impedance_real = my_eda_data.realdata*1000
                impedance_img = my_eda_data.imgdata*1000

               # print impedance_real
               # print impedance_img
                
                new_data_impedance_real.append(impedance_real)
                new_data_impedance_img.append(impedance_img)

                impedance_module = math.sqrt(impedance_real*impedance_real + impedance_img*impedance_img)
                impedance_phase = math.atan2(impedance_img,impedance_real)

              #  print  impedance_module
               # print  impedance_phase
                
                new_data_impedance_module.append(impedance_module)
                new_data_impedance_phase.append(impedance_phase)

                admitance_real = float(impedance_real / float(impedance_real*impedance_real + impedance_img*impedance_img))
                admitance_img = -float(impedance_img / float(impedance_real*impedance_real + impedance_img*impedance_img))
                admitance_module = 1/impedance_module
                admitance_phase = math.atan2(admitance_img,admitance_real)

                new_data_admitance_real.append(admitance_real)
                new_data_admitance_img.append(admitance_img)

                new_data_admitance_module.append(admitance_module)
                new_data_admitance_phase.append(admitance_phase)

                if self.enable_csv_logs:
                    # fstream = open(self.fname, "a")
                    self.fstream.write ('{},{},{},{}\n'.format(my_eda_data.timestamp, pkt.payload.sequence_num,impedance_module,impedance_phase))
                    # fstream.close()
            if len(new_data_admitance_real) == 0:
                return None

        return {self.admitance_real_key: [{
                    "series_name":self.admitance_real_key,
                    "series_data":new_data_admitance_real},
                    ],
                self.impedance_real_key: [{
                    "series_name":self.impedance_real_key,
                    "series_data":new_data_impedance_real},
                    ],
                self.admitance_img_key: [{
                    "series_name":self.admitance_img_key,
                    "series_data":new_data_admitance_img},
                    ],
                self.impedance_img_key: [{
                    "series_name":self.impedance_img_key,
                    "series_data":new_data_impedance_img},
                    ],
                self.admitance_mod_key: [{
                    "series_name":self.admitance_mod_key,
                    "series_data":new_data_admitance_module},
                    ],
                self.impedance_mod_key: [{
                    "series_name":self.impedance_mod_key,
                    "series_data":new_data_impedance_module},
                    ],
                self.admitance_phase_key: [{
                    "series_name":self.admitance_phase_key,
                    "series_data":new_data_admitance_phase},
                    ],
                self.impedance_phase_key: [{
                    "series_name":self.impedance_phase_key,
                    "series_data":new_data_impedance_phase},
                    ]
                }
    def save_csv_option(self, option):
        self.enable_csv_logs=option
        if self.enable_csv_logs:
            print "creating."
            self.fstream = open(self.fname, "w+")
            self.fstream.write ('TimeStamp, Seq.No., Impedance_Module, Impedance_phase\n')
            # fstream.close()
        return

class ecg_plot(plot):
    selection_name = "recg"
    name = "ECG Data Plot"
    ecg_key = "ECG"
    ecg_series_name = "ECG Data"
    stream_addr = M2M2_ADDR_ENUM_t.M2M2_ADDR_MED_ECG_STREAM
    enable_csv_logs = 0
    fname = "EcgAppStream.csv"
    fstream = None
    update_pkt_loss('recg', False)
    
    def __del__(self):
        if self.fstream:
            self.fstream.close()
    
    def setup(self):
        self.add_subplot(   subplot_name = self.ecg_key,
                            row = 1,
                            col = 1,
                            xlabel = "Time",
                            xunit = "Samples",
                            ylabel = "Value",
                            yunit = "LSBs",
                            data_series =   {  self.ecg_key:
                                                {"format":
                                                    {"name": self.ecg_series_name,
                                                    "colour":"g"},
                                                    "data": np.zeros(1000),
                                                },
                                            })

    def update(self, data_list):
        # Function that's called to update the plot. Should return a dictionary with each subplot's data
        new_data_ecg = []
        for data in data_list:
            # Peek at the header to find out what kind of data we have
            pkt = m2m2_packet(0, ecg_app_stream_t())
            pkt.unpack(data)
            self.check_seq_num(pkt.payload.sequence_num, 'recg')
            #print(pkt.payload.sequence_num)
            new_data_ecg.append(pkt.payload.firstecgdata)
            ts = pkt.payload.timestamp
            if self.enable_csv_logs:
                # fstream = open(self.fname, "a")
                self.fstream.write ('{},{},{}\n'.format(ts, pkt.payload.sequence_num,pkt.payload.firstecgdata))
                # fstream.close()
            for my_ecg_data in pkt.payload.ecg_data:
                new_data_ecg.append(my_ecg_data.ecgdata)
                #print(my_ecg_data.timestamp)
                ts = ts + my_ecg_data.timestamp
                if self.enable_csv_logs:
                    # fstream = open(self.fname, "a")
                    self.fstream.write ('{},{},{}\n'.format(ts, pkt.payload.sequence_num,my_ecg_data.ecgdata))
                    # fstream.close()

            if len(new_data_ecg) == 0:
                return None

        return {self.ecg_key: [{
                    "series_name":self.ecg_key,
                    "series_data":new_data_ecg},
                    ]
                }
    def save_csv_option(self, option):
        self.enable_csv_logs=option
        if self.enable_csv_logs:
            print "creating."
            self.fstream = open(self.fname, "w+")
            self.fstream.write ('TimeStamp, SeqNo., ECG_Data\n')
            # fstream.close()
        return

class syncppg_plot(plot):
    # The name used by the user to select this plotter
    selection_name = "rsyncppg"
    # The plot title
    name = "Sync PPG Data"
    # Names used to refer to the three sub-plots
    syncppg_adpd_key = "ADPD"
    syncppg_adxlx_key = "ADXL X"
    syncppg_adxly_key = "ADXL Y"
    syncppg_adxlz_key = "ADXL Z"
    syncppg_adpd_series_name = "SYNC PPG ADPD"
    syncppg_adxlx_series_name = "SYNC PPG ADXL X"
    syncppg_adxly_series_name = "SYNC PPG ADXL Y"
    syncppg_adxlz_series_name = "SYNC PPG ADXL Z"
    # Stream address that this plot uses
    stream_addr = M2M2_ADDR_ENUM_t.M2M2_ADDR_MED_SYNC_ADPD_ADXL_STREAM
    enable_csv_logs = 0
    fname = "syncppgStream.csv"
    fstream = None
    update_pkt_loss('rsyncppg', False)
    
    def __del__(self):
        if self.fstream:
            self.fstream.close()
    
    def setup(self):
        # Create a subplot for Sync PPG ADPD data
        self.add_subplot(   subplot_name = self.syncppg_adpd_key,
                            row = 1,
                            col = 1,
                            xlabel = "Time",
                            xunit = "Samples",
                            ylabel = "Value",
                            yunit = "LSBs",
                            # A single series called Sync PPG ADPD data
                            data_series =   {  self.syncppg_adpd_key:
                                                {"format":
                                                    # Series Display name
                                                    {"name": self.syncppg_adpd_series_name,
                                                    # Colour format string for this series
                                                    "colour":"r"},
                                                    # Empty data array for this series
                                                    "data": np.zeros(300),
                                                }
                                            })
        # Create a subplot for Sync PPG ADXL X-axis data
        self.add_subplot(   subplot_name = self.syncppg_adxlx_key,
                            row = 2,
                            col = 1,
                            xlabel = "Time",
                            xunit = "Samples",
                            ylabel = "Amplitude",
                            yunit = "LSBs",
                            data_series =   {  self.syncppg_adxlx_key:     # Series name
                                                {"format":      # Formatting for the series
                                                    {"name": self.syncppg_adxlx_series_name, # Display name
                                                    "colour":"g"},          # Line display parameters
                                                    "data": np.zeros(300),      # Data array
                                                }
                                            })
        # Create a subplot for Sync PPG ADXL Y-axis data
        self.add_subplot(   subplot_name = self.syncppg_adxly_key,
                            row = 3,
                            col = 1,
                            xlabel = "Time",
                            xunit = "Samples",
                            ylabel = "Amplitude",
                            yunit = "LSBs",
                            data_series =   {  self.syncppg_adxly_key:     # Series name
                                                {"format":      # Formatting for the series
                                                    {"name": self.syncppg_adxly_series_name, # Display name
                                                    "colour":"b"},          # Line display parameters
                                                    "data": np.zeros(300),      # Data array
                                                }
                                            })
        # Create a subplot for Sync PPG ADXL Z-axis data
        self.add_subplot(   subplot_name = self.syncppg_adxlz_key,
                            row = 4,
                            col = 1,
                            xlabel = "Time",
                            xunit = "Samples",
                            ylabel = "Amplitude",
                            yunit = "LSBs",
                            data_series =   {  self.syncppg_adxlz_key:     # Series name
                                                {"format":      # Formatting for the series
                                                    {"name": self.syncppg_adxlz_series_name, # Display name
                                                    "colour":"y"},          # Line display parameters
                                                    "data": np.zeros(300),      # Data array
                                                }
                                            })

    def update(self, data_list):
        # Function that's called to update the plot. Should return a dictionary with each subplot's data
        new_data_adpdsyncdata = []
        new_data_adxlsyncdata_x = []
        new_data_adxlsyncdata_y = []
        new_data_adxlsyncdata_z = []
        ts_adpd = []
        ts_adxl = []
        index = 0

        for data in data_list:
            # Peek at the header to find out what kind of data we have
            pkt = m2m2_packet(0, adpd_adxl_sync_data_stream_t())
            pkt.unpack(data)
            ts_adpd.append(pkt.payload.syncData.ppgTS)
            ts_adxl.append(pkt.payload.syncData.adxlTS)
            ts0_adpd = pkt.payload.syncData.ppgTS
            ts0_adxl = pkt.payload.syncData.adxlTS
            for ts in pkt.payload.syncData.incPpgTS:
                ts0_adpd += ts
                ts_adpd.append(ts0_adpd)
            for ts in pkt.payload.syncData.incAdxlTS:
                ts0_adxl += ts
                ts_adxl.append(ts0_adxl)    
            for ppg_data in pkt.payload.syncData.ppgData:
		        new_data_adpdsyncdata.append(ppg_data)
            for x_data in pkt.payload.syncData.xData:
		        x_data = x_data | ((x_data & 0x3000) << 2)
		        new_data_adxlsyncdata_x.append(x_data)
            for y_data in pkt.payload.syncData.yData:
		        y_data = y_data | ((y_data & 0x3000) << 2)
		        new_data_adxlsyncdata_y.append(y_data)
            for z_data in pkt.payload.syncData.zData:
		        z_data = z_data | ((z_data & 0x3000) << 2)
		        new_data_adxlsyncdata_z.append(z_data)

            if self.enable_csv_logs:
                for i in range(index, len(new_data_adpdsyncdata)):
                    # fstream = open(self.fname, "a")
                    self.fstream.write ('{},{},{},{},{}, {}\n'.format(ts_adpd[i], new_data_adpdsyncdata[i], ts_adxl[i], new_data_adxlsyncdata_x[i], new_data_adxlsyncdata_y[i], new_data_adxlsyncdata_z[i]))
                    # fstream.close()
                index = len(new_data_adpdsyncdata)
            self.check_seq_num(pkt.payload.sequence_num, 'rsyncppg')

        if len(new_data_adpdsyncdata) == 0 and len(new_data_adxlsyncdata_x) == 0 and len(new_data_adxlsyncdata_y) == 0 and len(new_data_adxlsyncdata_z) == 0:
            # No new data
            return None
        # Dictionary that associates the correct data with each subplot
        return {self.syncppg_adpd_key: [{
                    "series_name":self.syncppg_adpd_key,
                    "series_data":new_data_adpdsyncdata},
                    ],
                self.syncppg_adxlx_key: [{
                    "series_name":self.syncppg_adxlx_key,
                    "series_data":new_data_adxlsyncdata_x},
                    ],
                self.syncppg_adxly_key: [{
                    "series_name":self.syncppg_adxly_key,
                    "series_data":new_data_adxlsyncdata_y},
                    ],
                self.syncppg_adxlz_key: [{
                    "series_name":self.syncppg_adxlz_key,
                    "series_data":new_data_adxlsyncdata_z},
                    ]
                }

    def save_csv_option(self, option):
        self.enable_csv_logs=option
        if self.enable_csv_logs:
            print "creating."
            self.fstream = open(self.fname, "w+")
            self.fstream.write ('adpd_ts, adpd_data, adxl_ts, x_data, y_data, z_data\n')
            # fstream.close()
        return

class tempr3_plot(plot):
    selection_name = "rtempr3"
    name = "Temperature3 slot C Data Plot"
    temperature_name = "TEMPERATURE"
    temperature1_key = "TEMPERATURE(in Degree Celsius)"
    temperature2_key = "Resistance measured(in 100s of ohm)"
    temperature1_series_name = "Temperature1 Data"
    temperature2_series_name = "Temperature2 Data"
    stream_addr = M2M2_ADDR_ENUM_t.M2M2_ADDR_MED_TEMPERATURE_STREAM3
    enable_csv_logs = 0
    fname = "TempAppStream3.csv"
    fstream = None
    update_pkt_loss('rtempr3', False)

    def __del__(self):
        if self.fstream:
            self.fstream.close()

    def setup(self):
        self.add_subplot(subplot_name=self.temperature1_key,
                         row=1,
                         col=1,
                         xlabel="Time",
                         xunit="Samples",
                         ylabel="Value",
                         yunit="LSBs",
                         data_series={self.temperature1_key:
                                          {"format":
                                               {"name": self.temperature1_series_name,
                                                "colour": "g"},
                                           "data": np.zeros(20),
                                           }
                                      })
        self.add_subplot(subplot_name=self.temperature2_key,
                         row=2,
                         col=1,
                         xlabel="Time",
                         xunit="Samples",
                         ylabel="Value",
                         yunit="LSBs",
                         data_series={self.temperature2_key:
                                          {"format":
                                               {"name": self.temperature2_series_name,
                                                "colour": "r"},
                                           "data": np.zeros(20),
                                           }
                                      })

    def update(self, data_list):
        # Function that's called to update the plot. Should return a dictionary with each subplot's data
        new_data_temperature1 = []
        new_data_temperature2 = []

        for data in data_list:
            # Peek at the header to find out what kind of data we have
            pkt = m2m2_packet(0, temperature_app_stream_t())
            pkt.unpack(data)
            self.check_seq_num(pkt.payload.sequence_num, 'rtempr3')

            R1 = 39.0
            R0 = 47.0
            VDD = 2.5
            fullScale = 65535
            B = 4025
            T0 = 25 + 273.16

            '''vAdc = (VDD/fullScale)*pkt.payload.nTemperature1
            alpha = float(R1/R0)
            alpha = alpha*(vAdc/(VDD-vAdc))
            temp1 =  B*T0/(B + math.log(alpha)*T0)-273.15


            vAdc = (VDD/fullScale)*pkt.payload.nTemperature2
            alpha = float(R1/R0)
            alpha = alpha*(vAdc/(VDD-vAdc))
            temp2 =  B*T0/(B + math.log(alpha)*T0)-273.15

            new_data_temperature1.append(temp1)
            new_data_temperature2.append(temp2)'''
            new_data_temperature1.append(pkt.payload.nTemperature1 / 1000.0)
            new_data_temperature2.append(pkt.payload.nTemperature2)

            if self.enable_csv_logs:
                # fstream = open(self.fname, "a")
                self.fstream.write('{},{},{}\n'.format(pkt.payload.nTS, ((pkt.payload.nTemperature1) / 1000.0),
                                                       pkt.payload.nTemperature2))
                # fstream.close()

            if len(new_data_temperature1) == 0 and len(new_data_temperature2) == 0:
                return None

        return {self.temperature1_key: [{
            "series_name": self.temperature1_key,
            "series_data": new_data_temperature1},
        ],
            self.temperature2_key: [{
                "series_name": self.temperature2_key,
                "series_data": new_data_temperature2},
            ]
        }

    def save_csv_option(self, option):
        self.enable_csv_logs = option
        if self.enable_csv_logs:
            print "creating."
            self.fstream = open(self.fname, "w+")
            self.fstream.write('TimeStamp, Temperature,Resistance measured(in 100s of ohm)\n')
            # fstream.close()
        return

class tempr4_plot(plot):
    selection_name = "rtempr4"
    name = "Temperature4 slot D Data Plot"
    temperature_name = "TEMPERATURE"
    temperature1_key = "TEMPERATURE(in Degree Celsius)"
    temperature2_key = "Resistance measured(in 100s of ohm)"
    temperature1_series_name = "Temperature1 Data"
    temperature2_series_name = "Temperature2 Data"
    stream_addr = M2M2_ADDR_ENUM_t.M2M2_ADDR_MED_TEMPERATURE_STREAM
    enable_csv_logs = 0
    fname = "TempAppStream4.csv"
    fstream = None
    update_pkt_loss('rtempr4', False)

    def __del__(self):
        if self.fstream:
            self.fstream.close()

    def setup(self):
        self.add_subplot(subplot_name=self.temperature1_key,
                         row=1,
                         col=1,
                         xlabel="Time",
                         xunit="Samples",
                         ylabel="Value",
                         yunit="LSBs",
                         data_series={self.temperature1_key:
                                          {"format":
                                               {"name": self.temperature1_series_name,
                                                "colour": "g"},
                                           "data": np.zeros(20),
                                           }
                                      })
        self.add_subplot(subplot_name=self.temperature2_key,
                         row=2,
                         col=1,
                         xlabel="Time",
                         xunit="Samples",
                         ylabel="Value",
                         yunit="LSBs",
                         data_series={self.temperature2_key:
                                          {"format":
                                               {"name": self.temperature2_series_name,
                                                "colour": "r"},
                                           "data": np.zeros(20),
                                           }
                                      })

    def update(self, data_list):
        # Function that's called to update the plot. Should return a dictionary with each subplot's data
        new_data_temperature1 = []
        new_data_temperature2 = []

        for data in data_list:
            # Peek at the header to find out what kind of data we have
            pkt = m2m2_packet(0, temperature_app_stream_t())
            pkt.unpack(data)
            self.check_seq_num(pkt.payload.sequence_num, 'rtempr4')

            R1 = 39.0
            R0 = 47.0
            VDD = 2.5
            fullScale = 65535
            B = 4025
            T0 = 25 + 273.16

            '''vAdc = (VDD/fullScale)*pkt.payload.nTemperature1
            alpha = float(R1/R0)
            alpha = alpha*(vAdc/(VDD-vAdc))
            temp1 =  B*T0/(B + math.log(alpha)*T0)-273.15


            vAdc = (VDD/fullScale)*pkt.payload.nTemperature2
            alpha = float(R1/R0)
            alpha = alpha*(vAdc/(VDD-vAdc))
            temp2 =  B*T0/(B + math.log(alpha)*T0)-273.15

            new_data_temperature1.append(temp1)
            new_data_temperature2.append(temp2)'''
            new_data_temperature1.append(pkt.payload.nTemperature1 / 1000.0)
            new_data_temperature2.append(pkt.payload.nTemperature2)

            if self.enable_csv_logs:
                # fstream = open(self.fname, "a")
                self.fstream.write('{},{},{}\n'.format(pkt.payload.nTS, ((pkt.payload.nTemperature1) / 1000.0),
                                                       pkt.payload.nTemperature2))
                # fstream.close()

            if len(new_data_temperature1) == 0 and len(new_data_temperature2) == 0:
                return None

        return {self.temperature1_key: [{
            "series_name": self.temperature1_key,
            "series_data": new_data_temperature1},
        ],
            self.temperature2_key: [{
                "series_name": self.temperature2_key,
                "series_data": new_data_temperature2},
            ]
        }

    def save_csv_option(self, option):
        self.enable_csv_logs = option
        if self.enable_csv_logs:
            print "creating."
            self.fstream = open(self.fname, "w+")
            self.fstream.write('TimeStamp, Temperature,Resistance measured(in 100s of ohm)\n')
            # fstream.close()
        return

class tempr10_plot(plot):
    selection_name = "rtempr10"
    name = "Temperature10 slot J Data Plot"
    temperature_name = "TEMPERATURE"
    temperature1_key = "TEMPERATURE(in Degree Celsius)"
    temperature2_key = "Resistance measured(in 100s of ohm)"
    temperature1_series_name = "Temperature1 Data"
    temperature2_series_name = "Temperature2 Data"
    stream_addr = M2M2_ADDR_ENUM_t.M2M2_ADDR_MED_TEMPERATURE_STREAM10
    enable_csv_logs = 0
    fname = "TempAppStream10.csv"
    fstream = None
    update_pkt_loss('rtempr10', False)

    def __del__(self):
        if self.fstream:
            self.fstream.close()

    def setup(self):
        self.add_subplot(subplot_name=self.temperature1_key,
                         row=1,
                         col=1,
                         xlabel="Time",
                         xunit="Samples",
                         ylabel="Value",
                         yunit="LSBs",
                         data_series={self.temperature1_key:
                                          {"format":
                                               {"name": self.temperature1_series_name,
                                                "colour": "g"},
                                           "data": np.zeros(20),
                                           }
                                      })
        self.add_subplot(subplot_name=self.temperature2_key,
                         row=2,
                         col=1,
                         xlabel="Time",
                         xunit="Samples",
                         ylabel="Value",
                         yunit="LSBs",
                         data_series={self.temperature2_key:
                                          {"format":
                                               {"name": self.temperature2_series_name,
                                                "colour": "r"},
                                           "data": np.zeros(20),
                                           }
                                      })

    def update(self, data_list):
        # Function that's called to update the plot. Should return a dictionary with each subplot's data
        new_data_temperature1 = []
        new_data_temperature2 = []

        for data in data_list:
            # Peek at the header to find out what kind of data we have
            pkt = m2m2_packet(0, temperature_app_stream_t())
            pkt.unpack(data)
            self.check_seq_num(pkt.payload.sequence_num, 'rtempr10')

            R1 = 39.0
            R0 = 47.0
            VDD = 2.5
            fullScale = 65535
            B = 4025
            T0 = 25 + 273.16

            '''vAdc = (VDD/fullScale)*pkt.payload.nTemperature1
            alpha = float(R1/R0)
            alpha = alpha*(vAdc/(VDD-vAdc))
            temp1 =  B*T0/(B + math.log(alpha)*T0)-273.15


            vAdc = (VDD/fullScale)*pkt.payload.nTemperature2
            alpha = float(R1/R0)
            alpha = alpha*(vAdc/(VDD-vAdc))
            temp2 =  B*T0/(B + math.log(alpha)*T0)-273.15

            new_data_temperature1.append(temp1)
            new_data_temperature2.append(temp2)'''
            new_data_temperature1.append(pkt.payload.nTemperature1 / 1000.0)
            new_data_temperature2.append(pkt.payload.nTemperature2)

            if self.enable_csv_logs:
                # fstream = open(self.fname, "a")
                self.fstream.write('{},{},{}\n'.format(pkt.payload.nTS, ((pkt.payload.nTemperature1) / 1000.0),
                                                       pkt.payload.nTemperature2))
                # fstream.close()

            if len(new_data_temperature1) == 0 and len(new_data_temperature2) == 0:
                return None

        return {self.temperature1_key: [{
            "series_name": self.temperature1_key,
            "series_data": new_data_temperature1},
        ],
            self.temperature2_key: [{
                "series_name": self.temperature2_key,
                "series_data": new_data_temperature2},
            ]
        }

    def save_csv_option(self, option):
        self.enable_csv_logs = option
        if self.enable_csv_logs:
            print "creating."
            self.fstream = open(self.fname, "w+")
            self.fstream.write('TimeStamp, Temperature,Resistance measured(in 100s of ohm)\n')
            # fstream.close()
        return

class tempr11_plot(plot):
    selection_name = "rtempr11"
    name = "Temperature11 slot K Data Plot"
    temperature_name = "TEMPERATURE"
    temperature1_key = "TEMPERATURE(in Degree Celsius)"
    temperature2_key = "Resistance measured(in 100s of ohm)"
    temperature1_series_name = "Temperature1 Data"
    temperature2_series_name = "Temperature2 Data"
    stream_addr = M2M2_ADDR_ENUM_t.M2M2_ADDR_MED_TEMPERATURE_STREAM11
    enable_csv_logs = 0
    fname = "TempAppStream11.csv"
    fstream = None
    update_pkt_loss('rtempr11', False)

    def __del__(self):
        if self.fstream:
            self.fstream.close()

    def setup(self):
        self.add_subplot(subplot_name=self.temperature1_key,
                         row=1,
                         col=1,
                         xlabel="Time",
                         xunit="Samples",
                         ylabel="Value",
                         yunit="LSBs",
                         data_series={self.temperature1_key:
                                          {"format":
                                               {"name": self.temperature1_series_name,
                                                "colour": "g"},
                                           "data": np.zeros(20),
                                           }
                                      })
        self.add_subplot(subplot_name=self.temperature2_key,
                         row=2,
                         col=1,
                         xlabel="Time",
                         xunit="Samples",
                         ylabel="Value",
                         yunit="LSBs",
                         data_series={self.temperature2_key:
                                          {"format":
                                               {"name": self.temperature2_series_name,
                                                "colour": "r"},
                                           "data": np.zeros(20),
                                           }
                                      })

    def update(self, data_list):
        # Function that's called to update the plot. Should return a dictionary with each subplot's data
        new_data_temperature1 = []
        new_data_temperature2 = []

        for data in data_list:
            # Peek at the header to find out what kind of data we have
            pkt = m2m2_packet(0, temperature_app_stream_t())
            pkt.unpack(data)
            self.check_seq_num(pkt.payload.sequence_num, 'rtempr11')

            R1 = 39.0
            R0 = 47.0
            VDD = 2.5
            fullScale = 65535
            B = 4025
            T0 = 25 + 273.16

            '''vAdc = (VDD/fullScale)*pkt.payload.nTemperature1
            alpha = float(R1/R0)
            alpha = alpha*(vAdc/(VDD-vAdc))
            temp1 =  B*T0/(B + math.log(alpha)*T0)-273.15


            vAdc = (VDD/fullScale)*pkt.payload.nTemperature2
            alpha = float(R1/R0)
            alpha = alpha*(vAdc/(VDD-vAdc))
            temp2 =  B*T0/(B + math.log(alpha)*T0)-273.15

            new_data_temperature1.append(temp1)
            new_data_temperature2.append(temp2)'''
            new_data_temperature1.append(pkt.payload.nTemperature1 / 1000.0)
            new_data_temperature2.append(pkt.payload.nTemperature2)

            if self.enable_csv_logs:
                # fstream = open(self.fname, "a")
                self.fstream.write('{},{},{}\n'.format(pkt.payload.nTS, ((pkt.payload.nTemperature1) / 1000.0),
                                                       pkt.payload.nTemperature2))
                # fstream.close()

            if len(new_data_temperature1) == 0 and len(new_data_temperature2) == 0:
                return None

        return {self.temperature1_key: [{
            "series_name": self.temperature1_key,
            "series_data": new_data_temperature1},
        ],
            self.temperature2_key: [{
                "series_name": self.temperature2_key,
                "series_data": new_data_temperature2},
            ]
        }

    def save_csv_option(self, option):
        self.enable_csv_logs = option
        if self.enable_csv_logs:
            print "creating."
            self.fstream = open(self.fname, "w+")
            self.fstream.write('TimeStamp, Temperature,Resistance measured(in 100s of ohm)\n')
            # fstream.close()
        return

class tempr12_plot(plot):
    selection_name = "rtempr12"
    name = "Temperature12 slot L Data Plot"
    temperature_name = "TEMPERATURE"
    temperature1_key = "TEMPERATURE(in Degree Celsius)"
    temperature2_key = "Resistance measured(in 100s of ohm)"
    temperature1_series_name = "Temperature1 Data"
    temperature2_series_name = "Temperature2 Data"
    stream_addr = M2M2_ADDR_ENUM_t.M2M2_ADDR_MED_TEMPERATURE_STREAM12
    enable_csv_logs = 0
    fname = "TempAppStream12.csv"
    fstream = None
    update_pkt_loss('rtempr12', False)

    def __del__(self):
        if self.fstream:
            self.fstream.close()

    def setup(self):
        self.add_subplot(subplot_name=self.temperature1_key,
                         row=1,
                         col=1,
                         xlabel="Time",
                         xunit="Samples",
                         ylabel="Value",
                         yunit="LSBs",
                         data_series={self.temperature1_key:
                                          {"format":
                                               {"name": self.temperature1_series_name,
                                                "colour": "g"},
                                           "data": np.zeros(20),
                                           }
                                      })
        self.add_subplot(subplot_name=self.temperature2_key,
                         row=2,
                         col=1,
                         xlabel="Time",
                         xunit="Samples",
                         ylabel="Value",
                         yunit="LSBs",
                         data_series={self.temperature2_key:
                                          {"format":
                                               {"name": self.temperature2_series_name,
                                                "colour": "r"},
                                           "data": np.zeros(20),
                                           }
                                      })

    def update(self, data_list):
        # Function that's called to update the plot. Should return a dictionary with each subplot's data
        new_data_temperature1 = []
        new_data_temperature2 = []

        for data in data_list:
            # Peek at the header to find out what kind of data we have
            pkt = m2m2_packet(0, temperature_app_stream_t())
            pkt.unpack(data)
            self.check_seq_num(pkt.payload.sequence_num, 'rtempr12')

            R1 = 39.0
            R0 = 47.0
            VDD = 2.5
            fullScale = 65535
            B = 4025
            T0 = 25 + 273.16

            '''vAdc = (VDD/fullScale)*pkt.payload.nTemperature1
            alpha = float(R1/R0)
            alpha = alpha*(vAdc/(VDD-vAdc))
            temp1 =  B*T0/(B + math.log(alpha)*T0)-273.15


            vAdc = (VDD/fullScale)*pkt.payload.nTemperature2
            alpha = float(R1/R0)
            alpha = alpha*(vAdc/(VDD-vAdc))
            temp2 =  B*T0/(B + math.log(alpha)*T0)-273.15

            new_data_temperature1.append(temp1)
            new_data_temperature2.append(temp2)'''
            new_data_temperature1.append(pkt.payload.nTemperature1 / 1000.0)
            new_data_temperature2.append(pkt.payload.nTemperature2)

            if self.enable_csv_logs:
                # fstream = open(self.fname, "a")
                self.fstream.write('{},{},{}\n'.format(pkt.payload.nTS, ((pkt.payload.nTemperature1) / 1000.0),
                                                       pkt.payload.nTemperature2))
                # fstream.close()

            if len(new_data_temperature1) == 0 and len(new_data_temperature2) == 0:
                return None

        return {self.temperature1_key: [{
            "series_name": self.temperature1_key,
            "series_data": new_data_temperature1},
        ],
            self.temperature2_key: [{
                "series_name": self.temperature2_key,
                "series_data": new_data_temperature2},
            ]
        }

    def save_csv_option(self, option):
        self.enable_csv_logs = option
        if self.enable_csv_logs:
            print "creating."
            self.fstream = open(self.fname, "w+")
            self.fstream.write('TimeStamp, Temperature,Resistance measured(in 100s of ohm)\n')
            # fstream.close()
        return

class temperature_plot(plot):
    selection_name = "rtemperature"
    name = "Temperature Data Plot"
    temperature_name = "TEMPERATURE"
    temperature1_key = "TEMPERATURE1"
    temperature2_key = "Resistance measured(in 100s of ohm)"
    temperature1_series_name = "Temperature1 Data"
    temperature2_series_name = "Temperature2 Data"
    stream_addr = M2M2_ADDR_ENUM_t.M2M2_ADDR_MED_TEMPERATURE_STREAM
    enable_csv_logs = 0
    fname = "TempAppStream.csv"
    fstream = None
    update_pkt_loss('rtemperature', False)
    
    def __del__(self):
        if self.fstream:
            self.fstream.close()
    
    def setup(self):
        self.add_subplot(   subplot_name = self.temperature1_key,
                            row = 1,
                            col = 1,
                            xlabel = "Time",
                            xunit = "Samples",
                            ylabel = "Value",
                            yunit = "LSBs",
                            data_series =   {  self.temperature1_key:
                                                {"format":
                                                    {"name": self.temperature1_series_name,
                                                    "colour":"g"},
                                                    "data": np.zeros(20),
                                                }
                                            })
        self.add_subplot(   subplot_name = self.temperature2_key,
                            row = 2,
                            col = 1,
                            xlabel = "Time",
                            xunit = "Samples",
                            ylabel = "Value",
                            yunit = "LSBs",
                            data_series =   { self.temperature2_key:
                                                {"format":
                                                    {"name": self.temperature2_series_name,
                                                    "colour":"r"},
                                                    "data": np.zeros(20),
                                                }
                                            })

    def update(self, data_list):
        # Function that's called to update the plot. Should return a dictionary with each subplot's data
        new_data_temperature1 = []
        new_data_temperature2 = []

        for data in data_list:
            # Peek at the header to find out what kind of data we have
            pkt = m2m2_packet(0, temperature_app_stream_t())
            pkt.unpack(data)
            self.check_seq_num(pkt.payload.sequence_num, 'rtemperature')

            R1 = 39.0
            R0 = 47.0
            VDD = 2.5
            fullScale = 65535
            B = 4025
            T0 = 25 + 273.16

            '''vAdc = (VDD/fullScale)*pkt.payload.nTemperature1
            alpha = float(R1/R0)
            alpha = alpha*(vAdc/(VDD-vAdc))
            temp1 =  B*T0/(B + math.log(alpha)*T0)-273.15


            vAdc = (VDD/fullScale)*pkt.payload.nTemperature2
            alpha = float(R1/R0)
            alpha = alpha*(vAdc/(VDD-vAdc))
            temp2 =  B*T0/(B + math.log(alpha)*T0)-273.15

            new_data_temperature1.append(temp1)
            new_data_temperature2.append(temp2)'''
            new_data_temperature1.append(pkt.payload.nTemperature1/1000.0)
            new_data_temperature2.append(pkt.payload.nTemperature2)

            if self.enable_csv_logs:
                # fstream = open(self.fname, "a")
                self.fstream.write ('{},{},{}\n'.format(pkt.payload.nTS,((pkt.payload.nTemperature1)/1000.0),pkt.payload.nTemperature2))
                # fstream.close()

            if len(new_data_temperature1) == 0 and len(new_data_temperature2) == 0:
                return None

        return {self.temperature1_key: [{
                    "series_name":self.temperature1_key,
                    "series_data":new_data_temperature1},
		    ],
		self.temperature2_key: [ {
		    "series_name":self.temperature2_key,
                    "series_data":new_data_temperature2},
                    ]
                }

    def save_csv_option(self, option):
        self.enable_csv_logs=option
        if self.enable_csv_logs:
            print "creating."
            self.fstream = open(self.fname, "w+")
            self.fstream.write ('TimeStamp, Temperature,Resistance measured(in 100s of ohm)\n')
            # fstream.close()
        return

class bia_plot(plot):
    selection_name = "rbia"
    name = "BIA Data Plot"
    fname = "BiaAppStream.csv"
    fstream = None
    enable_csv_logs = 0
    admitance_real_key = "Real Admitance"
    admitance_real_series_name = "Admitance real Data"
    admitance_img_key = "Img Admitance"
    admitance_img_series_name = "Admitance img Data"
    admitance_mod_key = "Admitance Magnitude "
    admitance_mod_series_name = "Admitance magnitude Data"
    admitance_phase_key = "Admitance Phase"
    admitance_phase_series_name = "Admitance phase Data"

    impedance_real_key = "Real Impedance"
    impedance_real_series_name = "Impedance real Data"
    impedance_img_key = "Img Impedance"
    impedance_img_series_name = "Impedance img Data"
    impedance_mod_key = "Impedance Magnitude "
    impedance_mod_series_name = "Impedance magnitude Data"
    impedance_phase_key = "Impedance Phase"
    impedance_phase_series_name = "Impedance phase Data"

    stream_addr = M2M2_ADDR_ENUM_t.M2M2_ADDR_MED_BIA_STREAM
    
    def __del__(self):
        if self.fstream:
            self.fstream.close()
    
    def setup(self):
        self.add_subplot(   subplot_name = self.admitance_real_key,
                            row = 1,
                            col = 1,
                            #title = "Admitance real",
                            xlabel = "Time",
                            xunit = "Samples",
                            ylabel = "Value",
                            yunit = "Siemens",
                            data_series =   {  self.admitance_real_key:
                                                {"format":
                                                    {"name": self.admitance_real_series_name,
                                                    "colour":"r"},
                                                    "data": np.zeros(200),
                                                },
                                            })
        self.add_subplot(   subplot_name = self.impedance_real_key,
                            row = 1,
                            col = 2,
                            #title = "Impedance real",
                            xlabel = "Time",
                            xunit = "Samples",
                            ylabel = "Value",
                            yunit = "Ohms",
                            data_series =   {  self.impedance_real_key:
                                                {"format":
                                                    {"name": self.impedance_real_series_name,
                                                    "colour":"r"},
                                                    "data": np.zeros(200),
                                                },
                                            })
        self.add_subplot(   subplot_name = self.admitance_img_key,
                            row = 2,
                            col = 1,
                            #title = "Admitance img",
                            xlabel = "Time",
                            xunit = "Samples",
                            ylabel = "Amplitude",
                            yunit = "Siemens",
                            data_series =   {  self.admitance_img_key:     # Series name
                                                {"format":      # Formatting for the series
                                                    {"name": self.admitance_img_series_name, # Display name
                                                    "colour":"g"},          # Line display parameters
                                                    "data": np.zeros(200),      # Data array
                                                }
                                            })
        self.add_subplot(   subplot_name = self.impedance_img_key,
                            row = 2,
                            col = 2,
                            #title = "Impedance img",
                            xlabel = "Time",
                            xunit = "Samples",
                            ylabel = "Value",
                            yunit = "Ohms",
                            data_series =   {  self.impedance_img_key:
                                                {"format":
                                                    {"name": self.impedance_img_series_name,
                                                    "colour":"g"},
                                                    "data": np.zeros(200),
                                                },
                                            })
        self.add_subplot(   subplot_name = self.admitance_mod_key,
                            row = 3,
                            col = 1,
                            #title = "Admitance magnitude",
                            xlabel = "Time",
                            xunit = "Samples",
                            ylabel = "Amplitude",
                            yunit = "Siemens",
                            data_series =   {  self.admitance_mod_key:     # Series name
                                                {"format":      # Formatting for the series
                                                    {"name": self.admitance_mod_series_name, # Display name
                                                    "colour":"b"},          # Line display parameters
                                                    "data": np.zeros(200),      # Data array
                                                }
                                            })
        self.add_subplot(   subplot_name = self.impedance_mod_key,
                            row = 3,
                            col = 2,
                            #title = "Impedance magnitude",
                            xlabel = "Time",
                            xunit = "Samples",
                            ylabel = "Amplitude",
                            yunit = "Ohms",
                            data_series =   {  self.impedance_mod_key:     # Series name
                                                {"format":      # Formatting for the series
                                                    {"name": self.impedance_mod_series_name, # Display name
                                                    "colour":"b"},          # Line display parameters
                                                    "data": np.zeros(200),      # Data array
                                                }
                                            })
        self.add_subplot(   subplot_name = self.admitance_phase_key,
                            row = 4,
                            col = 1,
                            #title = "Admitance phase",
                            xlabel = "Time",
                            xunit = "Samples",
                            ylabel = "Amplitude",
                            yunit = "Radians",
                            data_series =   {  self.admitance_phase_key:     # Series name
                                                {"format":      # Formatting for the series
                                                    {"name": self.admitance_phase_series_name, # Display name
                                                    "colour":"y"},          # Line display parameters
                                                    "data": np.zeros(200),      # Data array
                                                }
                                            })
        self.add_subplot(   subplot_name = self.impedance_phase_key,
                            row = 4,
                            col = 2,
                            #title = "Impedance phase",
                            xlabel = "Time",
                            xunit = "Samples",
                            ylabel = "Amplitude",
                            yunit = "Radians",
                            data_series =   {  self.impedance_phase_key:     # Series name
                                                {"format":      # Formatting for the series
                                                    {"name": self.impedance_phase_series_name, # Display name
                                                    "colour":"y"},          # Line display parameters
                                                    "data": np.zeros(200),      # Data array
                                                }
                                            })

    def update(self, data_list):
        # Function that's called to update the plot. Should return a dictionary with each subplot's data
        new_data_admitance_real = []
        new_data_admitance_img = []
        new_data_admitance_magnitude = []
        new_data_admitance_phase = []

        new_data_impedance_real = []
        new_data_impedance_img = []
        new_data_impedance_magnitude = []
        new_data_impedance_phase = []

        for data in data_list:
            # Peek at the header to find out what kind of data we have
            pkt = m2m2_packet(0, bia_app_stream_t())
            pkt.unpack(data)
            self.check_seq_num(pkt.payload.sequence_num)
            for my_bia_data in pkt.payload.bia_data:
                #bia data calculation
                if my_bia_data.real == 0:
                    my_bia_data.real = 1
                if my_bia_data.img == 0:
                    my_bia_data.img = 1

                impedance_real = float(my_bia_data.real)
                impedance_img = float(my_bia_data.img)
                #Divide data by 1000 to get the float precision*/
                impedance_real = impedance_real/1000.0
                impedance_img  = impedance_img/1000.0
                #print "Real " + str(impedance_real)
                #print "IMg  " + str(impedance_img)

                admitance_real = float(impedance_real / float(impedance_real*impedance_real + impedance_img*impedance_img))
                admitance_img =  -float(impedance_img / float(impedance_real*impedance_real + impedance_img*impedance_img))
                
                new_data_admitance_real.append(admitance_real)
                new_data_admitance_img.append(admitance_img)

                admitance_magnitude = math.sqrt(admitance_real*admitance_real + admitance_img*admitance_img)

                admitance_phase = math.atan2(admitance_img,admitance_real)

                new_data_admitance_magnitude.append(admitance_magnitude)
                new_data_admitance_phase.append(admitance_phase)

                impedance_magnitude = math.sqrt(impedance_real*impedance_real + impedance_img*impedance_img)
                impedance_phase = math.atan2(impedance_img,impedance_real)

                #print impedance_magnitude

                new_data_impedance_real.append(impedance_real)
                new_data_impedance_img.append(impedance_img)
                new_data_impedance_magnitude.append(impedance_magnitude)
                new_data_impedance_phase.append(impedance_phase)

                if self.enable_csv_logs:
                    # fstream = open(self.fname, "a")
                    self.fstream.write ('{},{},{},{}, {}, {}\n'.format(my_bia_data.timestamp, pkt.payload.sequence_num,impedance_magnitude,impedance_phase, impedance_real, impedance_img))
                    # fstream.close()

            if len(new_data_admitance_real) == 0:
                return None

        return {self.admitance_real_key: [{
                    "series_name":self.admitance_real_key,
                    "series_data":new_data_admitance_real},
                    ],
                self.impedance_real_key: [{
                    "series_name":self.impedance_real_key,
                    "series_data":new_data_impedance_real},
                    ],
                self.admitance_img_key: [{
                    "series_name":self.admitance_img_key,
                    "series_data":new_data_admitance_img},
                    ],
                self.impedance_img_key: [{
                    "series_name":self.impedance_img_key,
                    "series_data":new_data_impedance_img},
                    ],
                self.admitance_mod_key: [{
                    "series_name":self.admitance_mod_key,
                    "series_data":new_data_admitance_magnitude},
                    ],
                self.impedance_mod_key: [{
                    "series_name":self.impedance_mod_key,
                    "series_data":new_data_impedance_magnitude},
                    ],
                self.admitance_phase_key: [{
                    "series_name":self.admitance_phase_key,
                    "series_data":new_data_admitance_phase},
                    ],
                self.impedance_phase_key: [{
                    "series_name":self.impedance_phase_key,
                    "series_data":new_data_impedance_phase},
                    ]
                }
    def save_csv_option(self, option):
        self.enable_csv_logs=option
        if self.enable_csv_logs:
            print "creating."
            self.fstream = open(self.fname, "w+")
            self.fstream.write ('Time_Stamp, Sequence number, Magnitude, Phase, Real Impedance, Imaginary Impedance\n')
            # fstream.close()
        return

class bcm_algo_plot(plot):
    selection_name = "rbcm"
    name = "BCM Algo Data Plot"
    fname = "BCMAlgoStream.csv"    
    ffm_estimated_key = "ffm_estimated"
    bmi_key = "bmi"
    fat_percent_key = "fat_percent"
    ffm_estimated_series_name = "FFM Estimated"
    bmi_series_name = "bmi"
    fat_percent = "fat_percent"
    stream_addr_series_name = M2M2_ADDR_ENUM_t.M2M2_ADDR_BCM_ALGO_STREAM
    enable_csv_logs = 0
    fstream = None

    def __del__(self):
        if self.fstream:
            self.fstream.close()

    def setup(self):
        self.add_subplot(subplot_name=self.ffm_estimated_key,
                         row=1,
                         col=1,
                         xlabel="Time",
                         xunit="Samples",
                         ylabel="ffm_estimated",
                         yunit="unit",
                         data_series={self.ffm_estimated_key:
                                          {"format":
                                               {"name": self.ffm_estimated_series_name,
                                                "colour": "r"},
                                           "data": np.zeros(250),
                                           },
                                      })
        self.add_subplot(subplot_name=self.bmi_key,
                         row=2,
                         col=1,
                         xlabel="Time",
                         xunit="Samples",
                         ylabel="bmi",
                         yunit="unit",
                         data_series={self.bmi_key:
                                          {"format":
                                               {"name": self.bmi_series_name,
                                                "colour": "r"},
                                           "data": np.zeros(250),
                                           },
                                      })
        self.add_subplot(subplot_name=self.fat_percent_key,
                         row=3,
                         col=1,
                         xlabel="Time",
                         xunit="Samples",
                         ylabel="fat_percent",
                         yunit="%",
                         data_series={self.fat_percent_key:
                                          {"format":
                                               {"name": self.fat_percent,
                                                "colour": "r"},
                                           "data": np.zeros(250),
                                           },
                                      })

    def update(self, data_list):
        # Function that's called to update the plot. Should return a dictionary with each subplot's data
        new_data_ffm_estimated = []
        new_data_bmi = []
        new_data_fat_percent = []

        for data in data_list:
            # Peek at the header to find out what kind of data we have
            pkt = m2m2_packet(0, bcm_app_algo_out_stream_t())
            pkt.unpack(data)
            self.check_seq_num(pkt.payload.sequence_num, 'rbcm')
            new_data_ffm_estimated.append(pkt.payload.ffm_estimated)
            new_data_bmi.append(pkt.payload.bmi)
            new_data_fat_percent.append(pkt.payload.fat_percent)
            # print "Plot Data.: {} {} {}".format(pkt.payload.first_xdata, pkt.payload.first_ydata, pkt.payload.first_zdata)
            #ts = pkt.payload.timestamp
            if self.enable_csv_logs:
                # fstream = open(self.fname, "a")
                self.fstream.write('{}, {},{},{}\n'.format(pkt.payload.sequence_num, pkt.payload.ffm_estimated, pkt.payload.bmi, pkt.payload.fat_percent))
                # fstream.close()
            if len(new_data_ffm_estimated) == 0 and len(new_data_bmi) == 0 and len(new_data_fat_percent) == 0:
                return None

        return {self.ffm_estimated_key: [{
                    "series_name":self.ffm_estimated_key,
                    "series_data":new_data_ffm_estimated},
                    ],
                self.bmi_key: [{
                    "series_name":self.bmi_key,
                    "series_data":new_data_bmi},
                    ],
                self.fat_percent_key: [{
                    "series_name":self.fat_percent_key,
                    "series_data":new_data_fat_percent},
                    ]
                }

    def save_csv_option(self, option):
        self.enable_csv_logs = option
        if self.enable_csv_logs:
            print "creating."
            self.fstream = open(self.fname, "w+")
            self.fstream.write('sequence_num, ffm_estimated, bmi, fat_percent\n')
            # fstream.close()
        return

class ped_plot(plot):
    selection_name = "rped"
    name = "PED Data Plot"
    pedometer_key = "PED"
    ped_series_name = "PED Data"
    stream_addr = M2M2_ADDR_ENUM_t.M2M2_ADDR_MED_PED_STREAM
    enable_csv_logs = 0
    fname = "PEDAppStream.csv"
    fstream = None
    update_pkt_loss('rped', False)
    
    def __del__(self):
        if self.fstream:
            self.fstream.close()
    
    def setup(self):
        self.add_subplot(   subplot_name = self.pedometer_key,
                            row = 1,
                            col = 1,
                            xlabel = "Time",
                            xunit = "Samples",
                            ylabel = "Pedometer",
                            yunit = "StepCount",
                            data_series =   {  self.pedometer_key:
                                                {"format":
                                                    {"name": self.ped_series_name,
                                                    "colour":"g"},
                                                    "data": np.zeros(20),
                                                }
                                            })

    def update(self, data_list):
        # Function that's called to update the plot. Should return a dictionary with each subplot's data
        new_data_step_count = []

        for data in data_list:
            # Peek at the header to find out what kind of data we have
            pkt = m2m2_packet(0, pedometer_app_stream_t())
            pkt.unpack(data)
            self.check_seq_num(pkt.payload.sequence_num, 'rped')

            new_data_step_count.append(pkt.payload.nNumSteps)
            print("Ts:{}, Steps:{}".format(pkt.payload.nTimeStamp, pkt.payload.nNumSteps))
            if self.enable_csv_logs:
                # fstream = open(self.fname, "a")
                self.fstream.write ('{},{}\n'.format(pkt.payload.nTimeStamp,pkt.payload.nNumSteps))
                # fstream.close()

            if len(new_data_step_count) == 0 :
                return None

        return {self.pedometer_key: [ {
		            "series_name":self.pedometer_key,
                    "series_data":new_data_step_count},
                    ]
                }

    def save_csv_option(self, option):
        self.enable_csv_logs=option
        if self.enable_csv_logs:
            print "creating."
            self.fstream = open(self.fname, "w+")
            self.fstream.write ('Time_Stamp, Pedometer count\n')
            # fstream.close()
        return

class sqi_plot(plot):
    selection_name = "rsqi"
    name = "SQI Data Plot"
    sqi_key = "SQI"
    sqi_series_name = "SQI Data"
    stream_addr = M2M2_ADDR_ENUM_t.M2M2_ADDR_MED_SQI_STREAM
    enable_csv_logs = 0
    fname = "SQIAppStream.csv"
    fstream = None
    update_pkt_loss('rsqi', False)
    
    def __del__(self):
        if self.fstream:
            self.fstream.close()
    
    def setup(self):
        self.add_subplot(   subplot_name = self.sqi_key,
                            row = 1,
                            col = 1,
                            xlabel = "Time",
                            xunit = "Samples",
                            ylabel = "SQI",
                            yunit = "SQI Index(0.0/0.5/1.0)",
                            data_series =   {  self.sqi_key:
                                                {"format":
                                                    {"name": self.sqi_series_name,
                                                    "colour":"g"},
                                                    "data": np.zeros(20),
                                                }
                                            })

    def update(self, data_list):
        # Function that's called to update the plot. Should return a dictionary with each subplot's data
        new_data_sqi = []

        for data in data_list:
            # Peek at the header to find out what kind of data we have
            pkt = m2m2_packet(0, sqi_app_stream_t())
            pkt.unpack(data)
            self.check_seq_num(pkt.payload.sequence_num, 'rsqi')

            new_data_sqi.append(pkt.payload.sqi)
            print("Ts:{}, SQI Index:{}".format(pkt.payload.nTimeStamp, pkt.payload.sqi))
            if self.enable_csv_logs:
                # fstream = open(self.fname, "a")
                self.fstream.write ('{},{}\n'.format(pkt.payload.nTimeStamp,pkt.payload.sqi))
                # fstream.close()

            if len(new_data_sqi) == 0 :
                return None

        return {self.sqi_key: [ {
		            "series_name":self.sqi_key,
                    "series_data":new_data_sqi},
                    ]
                }

    def save_csv_option(self, option):
        self.enable_csv_logs=option
        if self.enable_csv_logs:
            print "creating."
            self.fstream = open(self.fname, "w+")
            self.fstream.write ('Time_Stamp, SQI\n')
            # fstream.close()
        return
        
class hrv_plot(plot):
    selection_name = "rhrv"
    name = "HRV Data Plot"
    hrv_rrinterval_key = "RR_Interval"
    hrv_rrinterval_series_name = "HRV RR_Interval"
    hrv_isgap_key = "HRVValid"
    hrv_isgap_series_name = "HRVValid"
    hrv_rmssd_key = "RMSSD"
    hrv_rmssd_series_name = "HRV RMSSD"
    stream_addr = M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_HRV_STREAM
    enable_csv_logs = 0
    fname = "HRVAppStream.csv"
    fstream = None
    update_pkt_loss('rhrv', False)
    
    def __del__(self):
        if self.fstream:
            self.fstream.close()
    
    def setup(self):
        # Create a subplot for HRV RR Interval data
        self.add_subplot(   subplot_name = self.hrv_rrinterval_key,
                            row = 1,
                            col = 1,
                            xlabel = "Time",
                            xunit = "Samples",
                            ylabel = "Value",
                            yunit = "LSBs",
                            data_series =   {  self.hrv_rrinterval_key:
                                                {"format":
                                                    {"name": self.hrv_rrinterval_series_name,
                                                    "colour":"r"},
                                                    "data": np.zeros(60),
                                                }
                                            })
        
        # Create a subplot for HRV Is Gap data
        self.add_subplot(   subplot_name = self.hrv_isgap_key,
                            row = 2,
                            col = 1,
                            xlabel = "Time",
                            xunit = "Samples",
                            ylabel = "Value",
                            yunit = "LSBs",
                            data_series =   {  self.hrv_isgap_key:
                                                {"format":
                                                    {"name": self.hrv_isgap_series_name,
                                                    "colour":"g"},
                                                    "data": np.zeros(60),
                                                }
                                            })
                                            
        # Create a subplot for HRV RMSSD data
        self.add_subplot(   subplot_name = self.hrv_rmssd_key,
                            row = 3,
                            col = 1,
                            xlabel = "Time",
                            xunit = "Samples",
                            ylabel = "Value",
                            yunit = "LSBs",
                            data_series =   {  self.hrv_rmssd_key:
                                                {"format":
                                                    {"name": self.hrv_rmssd_series_name,
                                                    "colour":"b"},
                                                    "data": np.zeros(60),
                                                }
                                            })                                    
    def update(self, data_list):
        # Function that's called to update the plot. Should return a dictionary with each subplot's data
        new_hrv_rrinterval = []
        new_hrv_isgap = []
        new_hrv_rmssd = []
        ts = []
        index = 0

        for data in data_list:
            # Peek at the header to find out what kind of data we have
            pkt = m2m2_packet(0, ppg_app_hrv_info_t())
            pkt.unpack(data)
            
            new_hrv_rrinterval.append(pkt.payload.first_rr_interval)
            new_hrv_isgap.append(pkt.payload.first_is_gap)
            new_hrv_rmssd.append(pkt.payload.first_rmssd/16.0)
            
            t0 = pkt.payload.timestamp
            ts.append(t0)
            for data in pkt.payload.hrv_data:
                t0 += data.timestamp
                ts.append(t0)
                new_hrv_rrinterval.append(data.rr_interval)
                new_hrv_isgap.append(data.is_gap)
                new_hrv_rmssd.append(data.rmssd/16.0)
                
            if self.enable_csv_logs:
                for i in range(index, len(ts)):
                    self.fstream.write ('{},{},{},{}\n'.format(ts[i], new_hrv_rrinterval[i], new_hrv_isgap[i], new_hrv_rmssd[i]))
                index = len(ts)
            self.check_seq_num(pkt.payload.sequence_num, 'rhrv')

        if len(new_hrv_rrinterval) == 0 and len(new_hrv_isgap) == 0 and len(new_hrv_rmssd):
                return None

        return {self.hrv_rrinterval_key: [ {
		            "series_name":self.hrv_rrinterval_key,
                    "series_data":new_hrv_rrinterval},
                    ],
                self.hrv_isgap_key: [ {
		            "series_name":self.hrv_isgap_key,
                    "series_data":new_hrv_isgap},
                    ],
                self.hrv_rmssd_key: [ {
		            "series_name":self.hrv_rmssd_key,
                    "series_data":new_hrv_rmssd},
                    ]  
                }

    def save_csv_option(self, option):
        self.enable_csv_logs=option
        if self.enable_csv_logs:
            print "creating."
            self.fstream = open(self.fname, "w+")
            self.fstream.write ('Time_Stamp, RRInterval, HRVValid, RMSSD\n')
            # fstream.close()
        return


class ad7156_plot(plot):
    selection_name = "rad7156"
    name = "AD7156 Data"
    ad7156_name1 = "AD7156 CH1"
    ad7156_name2 = "AD7156 CH2"
    ch1_cap_key = "CH1"
    ch2_cap_key = "CH2"
    ch1_cap_series_name = "CH1 Capacitance"
    ch2_cap_series_name = "CH2 Capacitance"
    stream_addr = M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_AD7156_STREAM
    enable_csv_logs = 0
    fname = "ad7156Stream.csv"
    fstream = None
    update_pkt_loss('rad7156', False)

    def __del__(self):
        if self.fstream:
            self.fstream.close()

    def setup(self):
        self.add_subplot(subplot_name=self.ad7156_name1,
                         row=1,
                         col=1,
                         xlabel="Time",
                         xunit="Samples",
                         ylabel="Capacitance",
                         yunit="pF",
                         data_series={self.ch1_cap_key:
                                          {"format":
                                               {"name": self.ch1_cap_series_name,
                                                "colour": "r"},
                                           "data": np.zeros(250),
                                           }
                                      })
        self.add_subplot(subplot_name=self.ad7156_name2,
                         row=2,
                         col=1,
                         xlabel="Time",
                         xunit="Samples",
                         ylabel="Capacitance",
                         yunit="pF",
                         data_series={self.ch2_cap_key:
                                          {"format":
                                               {"name": self.ch2_cap_series_name,
                                                "colour": "g"},
                                           "data": np.zeros(250),
                                           }
                                      })                             

    def update(self, data_list):
        # Function that's called to update the plot. Should return a dictionary with each subplot's data
        new_data_ch1_cap = []
        new_data_ch2_cap = []

        for data in data_list:
            # Peek at the header to find out what kind of data we have
            pkt = m2m2_packet(0, m2m2_sensor_ad7156_data_t())
            pkt.unpack(data)
            self.check_seq_num(pkt.payload.sequence_num, 'rad7156')
            new_data_ch1_cap.append(pkt.payload.ch1_cap)
            new_data_ch2_cap.append(pkt.payload.ch2_cap)
            # print "Plot Data.: {} {} {}".format(pkt.payload.first_xdata, pkt.payload.first_ydata, pkt.payload.first_zdata)
            ts = pkt.payload.timestamp
            if self.enable_csv_logs:
                # fstream = open(self.fname, "a")
                self.fstream.write('{},{},{},{}\n'.format(ts, pkt.payload.sequence_num, pkt.payload.ch1_cap, pkt.payload.ch2_cap))
                # fstream.close()
            if len(new_data_ch1_cap) == 0 and len(new_data_ch2_cap) == 0:
                return None

        return {self.ad7156_name1: [{
            "series_name": self.ch1_cap_key,
            "series_data": new_data_ch1_cap}],
            self.ad7156_name2: [{
             "series_name": self.ch2_cap_key,
             "series_data": new_data_ch2_cap}]
        }

    def save_csv_option(self, option):
        self.enable_csv_logs = option
        if self.enable_csv_logs:
            print "creating."
            self.fstream = open(self.fname, "w+")
            self.fstream.write('TimeStamp, SeqNo, ad7156_ch1_cap, ad7156_ch2_cap\n')
            # fstream.close()
        return


def FIXED16Q4_TO_FLOAT(x):
    result_in_float = ((float(x))/float(16))
    return result_in_float
def FIXED16Q10_TO_FLOAT(x):
    result_in_float = ((float(x))/float(1024))
    return result_in_float
