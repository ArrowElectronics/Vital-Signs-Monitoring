from adi_study_watch_cli import cli


class CLIMap(cli.CLI):

    def __init__(self, signaller, testing=True):
        super().__init__(signaller, testing)
        # self.watch_shell = watch_shell_obj
        self.plot_dict = {'adxl': False, 'ecg': False, 'ppg': False, 'eda': False, 'temp': False, 'ped': False,
                          'bia': False, 'sqi': False, 'adpd1': False, 'adpd2': False, 'adpd3': False, 'adpd4': False,
                          'adpd5': False, 'adpd6': False, 'adpd7': False, 'adpd8': False, 'adpd9': False,
                          'adpd10': False, 'adpd11': False, 'adpd12': False, 'sync_ppg': False}

    def check_err_stat(self, pkt, success_val=0x97):
        if success_val or success_val == 0:
            err_stat = 0 if pkt['payload']['status'].value[0] == success_val else 1
        else:
            err_stat = 0
        return err_stat

    def get_system_info(self):
        pkt = self.do_get_system_info('')
        sys_info_dict = {'version': pkt['payload']['version'], 'mac_addr': pkt['payload']['mac_address'],
                         'device_id': pkt['payload']['device_id'], 'model_number': pkt['payload']['model_number'],
                         'hw_id': pkt['payload']['hw_id'], 'bom_id': pkt['payload']['bom_id'],
                         'batch_id': pkt['payload']['batch_id'], 'date': pkt['payload']['date']}
        err_stat = self.check_err_stat(pkt, 65)
        return err_stat, sys_info_dict

    def get_version_cli(self):
        pkt = self.do_get_version('pm')
        fw_ver_dict = {'major': pkt['payload']['major_version'], 'minor': pkt['payload']['minor_version'],
                       'patch': pkt['payload']['patch_version'],
                       'info': pkt['payload']['version_string'],
                       'build': pkt['payload']['build_version']}
        err_stat = self.check_err_stat(pkt, 65)
        return err_stat, fw_ver_dict

    def get_chip_id(self, dev):
        pkt = self.do_get_chip_id(dev)
        chip_id = pkt['payload']['chip_id']
        err_stat = self.check_err_stat(pkt, 65)
        return err_stat, chip_id

    def quick_start(self, sensor, stream, plot=False, fs=False):
        if fs:
            self.do_fs_sub('{} add'.format(stream))
        self.do_sensor('{} start'.format(sensor))
        if not fs:
            self.do_csv_log('{} start'.format(stream))
            if sensor == 'ppg':
                self.do_sub("sync_ppg add")
                self.do_csv_log('sync_ppg start')
            self.do_sub('{} add'.format(stream))
            if plot:
                self.do_plot(stream)

    def quick_stop(self, sensor, stream, fs=False):
        if fs:
            self.do_sensor('{} stop'.format(sensor))
            self.do_fs_sub('{} remove'.format(stream))
        else:
            self.do_sub('{} remove'.format(stream))
            self.do_csv_log('{} stop'.format(stream))
            if sensor == 'ppg':
                self.do_sub("sync_ppg remove")
                self.do_csv_log('sync_ppg stop')
            self.do_sensor('{} stop'.format(sensor))

    def fs_ls(self):
        fs_list = self.do_fs_ls('')
        if fs_list:
            fs_list = [{'file': fs[0], 'file_type': fs[1], 'file_size': fs[2]} for fs in fs_list]
        err_stat = 0
        return err_stat, fs_list

    def reg_read(self, dev, addr):
        if type(addr) == str:
            pass
        elif type(addr) == int:
            addr = hex(addr)
        elif type(addr) == list:
            raise Exception('List Reg Read yet to be implemented!')
            pass  # TODO: Implement list reg read
        pkt = self.do_reg('r {} {}'.format(dev, addr))
        err_stat = self.check_err_stat(pkt, 0)
        reg_val = [(data[0], data[1]) for data in pkt['payload']['data']]
        return err_stat, reg_val

    def ping(self, ping_count, pkt_size):
        pkts = self.do_ping('{} {}'.format(ping_count, pkt_size))
        received_pkt_count = len(pkts)
        # TBD: Implement pong

    def quick_start_uc(self, stream_list, fs_log=False, plot=False, stream_args=None):
        """
        This function can be used to perform multiple streams in a sequence like in various usecases.
        :param stream_list:
        :param fs_log:
        :param plot:
        :param stream_args:
        :return:
        """
        if not stream_args:  # TODO: Use this for setting frequencies from inside the loop if needed
            stream_args = [None for i in stream_list]

        for i, stream in enumerate(stream_list):  # Stream Subscription Loop
            if stream == 'adpd':
                sensor = stream
                stream = 'adpd6'
            else:
                sensor = stream  # except for adpd all other sensors have same stream names
            if fs_log:
                self.do_fs_sub('{} add'.format(stream))
            else:
                self.do_sub('{} add'.format(stream))

        for i, stream in enumerate(stream_list):  # Sensor Start Loop
            if stream == 'adpd':
                sensor = stream
                stream = 'adpd6'
            else:
                sensor = stream  # except for adpd all other sensors have same stream names
            self.do_sensor('{} start'.format(sensor))
            if not fs_log:  # Starting csv logging for each stream
                self.do_csv_log('{} start'.format(stream))
            if plot:  # Starting plot for each stream
                self.do_plot(stream)  #TODO: Add start/stop plot

        if fs_log:  # Starting FS logging for all streams
            self.do_start_logging('')

    def quick_stop_uc(self, stream_list, fs_log=False, plot=False):
        """
        This function can be used to stop multiple streams in a sequence like in various usecases
        :param stream_list:
        :param fs_log:
        :param plot:
        :return:
        """
        for i, stream in enumerate(stream_list):  # Sensor Stop Loop
            if stream == 'adpd':
                sensor = stream
                stream = 'adpd6'
            else:
                sensor = stream  # except for adpd all other sensors have same stream names
            self.do_sensor('{} stop'.format(sensor))

        for i, stream in enumerate(stream_list):  # Stream Unsubscribe Loop
            if stream == 'adpd':
                sensor = stream
                stream = 'adpd6'
            else:
                sensor = stream  # except for adpd all other sensors have same stream names
            if fs_log:
                self.do_fs_sub('{} remove'.format(stream))
            else:
                self.do_csv_log('{} stop'.format(stream))
                self.do_sub('{} remove'.format(stream))

            if plot:  # Stopping plots for each stream
                pass
                # self.do_plot(stream)  #TODO: Add start/stop plot

        if fs_log:  # Stopping FS logging for all streams
            self.do_stop_logging('')