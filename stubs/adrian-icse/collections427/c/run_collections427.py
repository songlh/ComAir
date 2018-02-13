#!/usr/bin/python
# -*- coding: utf-8 -*-
import os
import stat

from subprocess import call

import pandas as pd

from scipy.optimize import curve_fit

APROF_LOGGER = 'aprof_logger.txt'
PATH = os.path.dirname(os.path.realpath(__file__))


def generate_test_case(i):
    return str((i + 1) * 100)


def run_command():
    for i in range(0, 30):
        if os.path.exists(APROF_LOGGER):
            new_name = APROF_LOGGER.split('.')[0] + '_' + str(i) + '.txt'
            os.rename(APROF_LOGGER, new_name)

        command = ['./a.out ', ' ', generate_test_case(i)]
        with open('test.sh', 'w') as run_script:
            run_script.writelines(command)
            st = os.stat('test.sh')
            os.chmod('test.sh', st.st_mode | stat.S_IEXEC)

        call(['/bin/bash', 'test.sh'])

    # delete temp test.sh
    os.remove('test.sh')


def clean_temp_files():
    for dirname, dirnames, filenames in os.walk('.'):
        # print path to all filenames.
        for filename in filenames:
            if filename.endswith('.txt') or filename.endswith('.csv'):
                os.remove(
                    os.path.join(PATH, os.path.join(
                        dirname[2:], filename)))

    print('Clean all temp files!')


def _parser(file_name, run_id):
    result = []
    with open(file_name, 'r') as f:
        lines = f.readlines()
        for line in lines:
            # FIXME:: TOO BAD!!!
            items = line.strip('\n').split(';')
            func_id = items[0].strip().split(' ')[1]
            rms = items[1].strip().split(' ')[1]
            cost = items[2].strip().split(' ')[1]
            result.append([func_id, rms, cost, run_id])

    return result


def parse_log_file():
    results = []
    for dirname, dirnames, filenames in os.walk('.'):
        # print path to all filenames.
        index = 1
        for filename in filenames:
            if filename.endswith('.txt') and 'aprof_logger' in filename:
                result = _parser(filename, index)
                index += 1
                for item in result:
                    results.append(item)

    df = pd.DataFrame(data=results, columns=['func_id', 'rms', 'cost', 'run_id'])
    df.to_csv('collections427_result.csv', index=False)


def calculate_curve_fit():
    """
    calculate the expression of target function
    :return:
    """

    def fund(x, a, b):
        return x ** a + b

    df = pd.read_csv('collections427_result.csv')
    df = df.loc[df['func_id'] == 52]
    xdata = df[['rms']].apply(pd.to_numeric)
    ydata = df[['cost']].apply(pd.to_numeric)

    xdata = [item[0] for item in xdata.values]
    ydata = [item[0] for item in ydata.values]

    popt, pcov = curve_fit(fund, xdata, ydata)
    # print y = x^a +/- b
    op_code = '+'
    if popt[1] < 0:
        op_code = '-'
        popt[1] = popt[1] * -1

    to_str = ['y = x^', '%.2f' % popt[0], ' ', op_code, ' ', '%.2f' % popt[1]]
    print(''.join(to_str))


if __name__ == '__main__':
    clean_temp_files()
    # run_command()
    # parse_log_file()
    # calculate_curve_fit()
    # if you want to save result csv,
    # you can comment the follow line code.
    # clean_temp_files()
