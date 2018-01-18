#!/usr/bin/python
# -*- coding: utf-8 -*-
import os
import stat

from subprocess import call

import pandas as pd

from scipy.optimize import curve_fit


APROF_LOGGER = 'aprof_logger.txt'
PATH = os.path.dirname(os.path.realpath(__file__))


def run_command():
    for i in range(0, 1):
        if os.path.exists(APROF_LOGGER):
            new_name = APROF_LOGGER.split('.')[0] + '_' + str(i - 1) + '.txt'
            os.rename(APROF_LOGGER, new_name)

        command = ['./a.out ', ' ', '20']
        with open('test.sh', 'w') as run_script:
            run_script.writelines(command)
            st = os.stat('test.sh')
            os.chmod('test.sh', st.st_mode | stat.S_IEXEC)

        call(['/bin/bash', 'test.sh'])

    # delete temp test.sh
    os.remove('test.sh')


def clean_txt():
    for dirname, dirnames, filenames in os.walk('.'):
        # print path to all filenames.
        for filename in filenames:
            if filename.endswith('.txt'):
                os.remove(
                    os.path.join(PATH, os.path.join(
                        dirname[2:], filename)))

    print('Clean all txt file!')


def _parser(file_name, run_id):
    result = []
    with open(file_name, 'r') as f:
        lines = f.readlines()
        for line in lines:
            # FIXME:: TOO BAD!!!
            items = line.strip('\n')[line.rindex(':') + 1:].split(';')
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
    df.to_csv('fib_result.csv', index=False)


def calculate_curve_fit():
    """
    calculate the expression of target function
    :return:
    """

    def fund(x, a, b):
        return a**x + b

    df = pd.read_csv('fib_result.csv')
    df = df.loc[df['func_id'] == 2]
    xdata = df[['rms']].apply(pd.to_numeric)
    ydata = df[['cost']].apply(pd.to_numeric)

    xdata = [item[0] for item in xdata.values]
    ydata = [item[0] for item in ydata.values]

    popt, pcov = curve_fit(fund, xdata, ydata)
    # print [a, b]
    print(popt)


if __name__ == '__main__':
    clean_txt()
    run_command()
    parse_log_file()
    clean_txt()
    calculate_curve_fit()
