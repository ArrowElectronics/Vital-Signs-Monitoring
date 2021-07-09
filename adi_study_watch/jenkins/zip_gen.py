import os
import shutil
import time
import comtypes.client
#from docx2pdf import convert

src_path = os.path.abspath(os.path.join(__file__, '../../'))
dest_path = r'zip_gen_tmp/adi_study_watch'
excludes_file_path = os.path.join(src_path, 'jenkins', 'zip_gen_excludes.txt')


def copy_repo_package():
    shutil.copytree(src_path, dest_path, ignore=shutil.ignore_patterns('.git'))


def convert_docx_to_pdf():
    doc_dir_list = [r'nrf5_sdk_15.2.0/adi_study_watch/doc']
    word = comtypes.client.CreateObject('Word.Application')
    for doc_dir in doc_dir_list:
        doc_dir_path = os.path.abspath(os.path.join(dest_path, doc_dir))
        for file in os.listdir(doc_dir_path):
            if 'doc' in os.path.splitext(file)[-1].lower():
                file_name = os.path.splitext(file)[0]
                doc_file_path = os.path.abspath(os.path.join(doc_dir_path, file))
                pdf_file_path = os.path.abspath(os.path.join(doc_dir_path, file_name+'.pdf'))
                print('Doc File Path:', doc_file_path)
                doc = word.Documents.Open(doc_file_path)
                doc.SaveAs(pdf_file_path, FileFormat=17)
                doc.Close()
                #convert(doc_file_path)
                time.sleep(3)
    word.Quit()
    

def filter_out_dir(excludes_file):
    with open(excludes_file, 'r') as f:
        line_list = f.readlines()
    line_list = [line.strip() for line in line_list if line.strip()]
    for line in line_list:
        path = os.path.join(os.path.abspath(dest_path), line)
        if os.path.exists(path) and os.path.isfile(path):
            os.remove(path)
        elif os.path.exists(path) and os.path.isdir(path):
            shutil.rmtree(path)
        else:
            print('Filter Ignoring File/Dir Path:', path)


def update_project_excludes():
    proj_file = os.path.abspath(os.path.join(dest_path, 'nrf5_sdk_15.2.0/adi_study_watch/app/nRF52840_app/ses/watchv4_nrf52840.emProject'))
    ref_str = r'<file file_name="../../../utilities/tracealyser/trace_recorder/streamports/Jlink_RTT/SEGGER_RTT_tracealyser.c" />'
    exclude_str = r'''<file file_name="../../../utilities/tracealyser/trace_recorder/streamports/Jlink_RTT/SEGGER_RTT_tracealyser.c">
            <configuration Name="Debug" build_exclude_from_build="Yes" />
            <configuration Name="Release" build_exclude_from_build="Yes" />
          </file>'''
    with open(proj_file, "r+") as f:
        proj_file_str = f.read()
        proj_file_str = proj_file_str.replace(ref_str, exclude_str)
        f.seek(0)
        f.write(proj_file_str)
        f.truncate()


def generate_zip():
    shutil.make_archive('adi_study_watch', 'zip', os.path.abspath(os.path.join(dest_path, '../')))


if __name__ == '__main__':
    print('Start Time:', round(time.time()))
    copy_repo_package()
    print('Copy Done Time:', round(time.time()))
    # convert_docx_to_pdf()
    # print('Doc2Pdf Conversion Done Time:', round(time.time()))
    filter_out_dir(excludes_file_path)
    print('Filter Done Time:', round(time.time()))
    update_project_excludes()
    print('Project File Update Done Time:', round(time.time()))
    generate_zip()
    print('Zip Generation Done Time:', round(time.time()))