This file contains the newly added test commands in CLI2.

1. write_dcb_config, read_dcb_config, delete_dcb_config has been extended with new DCB block -> user0_config
2. get_user0_config_app_state
3. set_user0_config_app_state
4. lcfgUser0ConfigAppWrite
5. lcfgUser0ConfigAppRead
6. ltAppTuning modified to add new LT mode 3 -> LT_APP_INTERMITTENT_TRIGGER
7. exp_id
8. hw_id

Added from PERSEUS-895:
9. clear_user0_prev_state_event
10. get_user0_prev_state_event