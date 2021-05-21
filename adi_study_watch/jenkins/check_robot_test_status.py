import xml.etree.ElementTree as ET

xmlfile = 'output.xml'
root = ET.parse(xmlfile).getroot()
stat = root.find('./statistics/total/stat')
fail_count = int(stat.get('fail'))
if fail_count > 0:
    raise AssertionError('{} Robot Test Failures Detected!'.format(fail_count))
