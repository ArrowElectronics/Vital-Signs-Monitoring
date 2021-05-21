import time
# **************** Added for robot file execution ***************** #
import sys
import os
sys.path.insert(0, os.path.join(os.path.abspath(__file__), '../../'))
# **************** ****************************** ***************** #
import common
from common import ConditionCheckFailure
from utils import qa_utils


def ad8233_dev_id_test():
    """

    :return:
    """
    # qa_utils.dev_id_test('AD8233')
    pass  # TODO: Not supported by CLI/Firmware


if __name__ == '__main__':
    # This section can be used for testing and verifying individual test cases
    common.initialize_setup()
    # Add test cases to be verified here
    ad8233_dev_id_test()
    common.close_setup()
