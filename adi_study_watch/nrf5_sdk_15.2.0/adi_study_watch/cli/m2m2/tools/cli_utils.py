import logging
import time

def cli_logger(func):
    def inner(*args):
        # logging.info('[{}({})]'.format(func.__name__, args))
        t1 = time.time()
        result = func(*args)
        t2 = time.time() - t1
        arg_list = list(args)
        if arg_list:
            if '<' in str(arg_list[0]) and '>' in str(arg_list[0]):
                # removing class obj type args from tuple
                del arg_list[0]
        args_str = ','.join([str(arg) for arg in arg_list])
        args = tuple(arg_list)
        logging.info('[{}{} | ExecTime:{}ms]'.format(func.__name__, args, ('{0:.3f}'.format(t2 * 1000))))
        return result
    return inner
