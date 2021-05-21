import os
import yaml
import argparse
from jira import JIRA


# Global Variables
perseus_comment_tag = '[PERSEUS-COMMENT]'
awt_comment_tag = '[AWT-COMMENT]'


def read_station_cfg():
    """
    This function reads station config yaml file and returns cfg_dict dictionary
    :return:
    """
    cfg_dict = {'username': '', 'pswd':''}
    station_cfg_path = os.path.join(os.getenv('APPDATA'), 'station_cfg.yaml')
    print (station_cfg_path)
    if os.path.exists(station_cfg_path) and os.path.isfile(station_cfg_path):
        with open(station_cfg_path, 'r') as f_ref:
            cfg_dict = yaml.load(f_ref, Loader=yaml.FullLoader)
    else:
        with open(station_cfg_path, 'w') as f_ref:
            yaml.dump(cfg_dict, f_ref)
    return cfg_dict


def clone_perseus_issues_to_awt(JIRA_USER, USER_PSWD):
    """
    This function searches for AWT issues from perseus and clones it into AWT project
    :param cfg_dict:
    :return:
    """
    # Opening AWT and PERSEUS JIRA instances
    ext_jira = JIRA(server='https://jira-ext.analog.com/',
                    basic_auth=(JIRA_USER, USER_PSWD))  # Perseus JIRA
    int_jira = JIRA(server='https://jira.analog.com/',
                    basic_auth=(JIRA_USER, USER_PSWD))  # WaveTool JIRA

    # Creating list of AWT jira issue summaries
    awt_issue_list = int_jira.search_issues('project=HCPIOP AND component=PERSEUS_AWT', )
    awt_summary_list = []
    for awt_issue in awt_issue_list:
        awt_summary_list.append(awt_issue.fields.summary)

    # Searching for AWT issues in PERSEUS and cloning them to AWT project
    perseus_issue_list = ext_jira.search_issues('project=PERSEUS AND component=WaveTool')
    for perseus_issue in perseus_issue_list:
        perseus_issue_labels_list = perseus_issue.fields.labels
        if 'HCPIOP' not in perseus_issue_labels_list:  # Checking if issue has already been linked to HCPIOP
            summary = perseus_issue.fields.summary
            perseus_issue_type = perseus_issue.fields.issuetype  # TODO: map issue type
            perseus_issue_descr = perseus_issue.fields.description
            perseus_issue_link = perseus_issue.permalink()
            awt_issue_descr = '[PERSEUS-JIRA-LINK]: {}\n\n{}'.format(perseus_issue_link, perseus_issue_descr)

            if summary not in awt_summary_list:
                issue = int_jira.create_issue(fields={'project': 'HCPIOP',
                                                      'summary': summary,
                                                      'issuetype': {'name': 'Story'},
                                                      'components': [{'name': 'PERSEUS_AWT'},
                                                                     {'name': 'Applications WaveTool'}],
                                                      'description': awt_issue_descr})
                print('Issue created:', issue)
                awt_issue_link = issue.permalink()
                perseus_issue.add_field_value('labels', 'HCPIOP')
                perseus_issue.update(fields={'description': '[AWT-JIRA-LINK]: {}\n\n{}'.format(awt_issue_link,
                                                                                           perseus_issue_descr)})
    # Sync Comments
    sync_comments(ext_jira, int_jira)

    # Closing AWT and PERSEUS JIRA instances
    ext_jira.close()
    int_jira.close()


def check_if_comment_exists(comment_str, comment_list):
    exists = False
    for comment in comment_list:
        if comment_str.strip() in comment.body.encode('utf-8'):
            exists = True
            break
    return exists


def check_and_updated_comments(issue_list, issue_jira, clone_issue_jira, issue_comment_tag, clone_issue_comment_tag,
                               descr_tag='[AWT-JIRA-LINK]:'):
    print('*** Syncing - {} ***'.format(issue_comment_tag))
    for issue in issue_list:
        try:
            issue_descr = issue.fields.description.encode('utf-8')
            descr_list = issue_descr.split(descr_tag)
            if len(descr_list) >= 2:
                clone_issue_key = descr_list[1].split('\n')[0].strip().split(r'/')[-1]
                clone_issue_comment_list = clone_issue_jira.comments(clone_issue_key)
                issue_comment_list = issue_jira.comments(issue.key)
                for clone_issue_comment in clone_issue_comment_list:
                    clone_comment_str = clone_issue_comment.body.encode('utf-8')
                    if issue_comment_tag not in clone_comment_str:  # Verifying if comment not a cloned comment
                        if not check_if_comment_exists(clone_comment_str, issue_comment_list):
                            new_comment_str = '{}-[{}]\n{}'.format(clone_issue_comment_tag,
                                                                   clone_issue_comment.author.name.encode('utf-8'),
                                                                   clone_comment_str)
                            issue_jira.add_comment(issue.key, new_comment_str)
                            print('{} - Comment Updated'.format(issue.key))
        except Exception as e:
            print('{} - ERROR ({})'.format(issue.key, e))


def sync_comments(ext_jira, int_jira):
    """

    :param ext_jira:
    :param int_jira:
    :return:
    """
    # Creating list of jira issues
    awt_issue_list = int_jira.search_issues('project=HCPIOP AND component=PERSEUS_AWT', )
    perseus_issue_list = ext_jira.search_issues('project=PERSEUS AND component=WaveTool')

    # Sync Perseus issues with AWT issue comments
    check_and_updated_comments(issue_list=perseus_issue_list, issue_jira=ext_jira, clone_issue_jira=int_jira,
                               issue_comment_tag=perseus_comment_tag, clone_issue_comment_tag=awt_comment_tag,
                               descr_tag='[AWT-JIRA-LINK]:')

    # Sync AWT issues with PERSEUS issue comments
    check_and_updated_comments(issue_list=awt_issue_list, issue_jira=int_jira, clone_issue_jira=ext_jira,
                               issue_comment_tag=awt_comment_tag, clone_issue_comment_tag=perseus_comment_tag,
                               descr_tag='[PERSEUS-JIRA-LINK]:')


def parse_arguments():
    """
    Argument parser is used for parsing command line arguments
    :return: args
    """
    parser = argparse.ArgumentParser()
    parser.add_argument("JIRA_USER", nargs='?', default='')
    parser.add_argument("USER_PSWD", nargs='?', default='')
    args_obj = parser.parse_args()
    return args_obj


if __name__ == '__main__':
    args = parse_arguments()
    clone_perseus_issues_to_awt(args.JIRA_USER, args.USER_PSWD)





