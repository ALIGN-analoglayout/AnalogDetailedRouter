
from subprocess import Popen, PIPE
import os
from contextlib import contextmanager
import re

@contextmanager
def working_directory(directory):
    owd = os.getcwd()
    try:
        os.chdir(directory)
        yield directory
    finally:
        os.chdir(owd)

def test_one():
    with working_directory('tests/vga'):
        cmd = ['../../bin/amsr.exe','-file','INPUT/ctrl.txt']

        with Popen( cmd, stdout=PIPE) as proc:
            p = re.compile( r'.*Net (\S+) has no opens\.\s*$')
            p2 = re.compile( r'.*open.*')
            p3 = re.compile( r'.*ISSUE.*')

            for line in proc.stdout:
                line = line.decode().rstrip('\n')
                if p.match(line):
                    pass
#                    print(line)
                elif p2.match(line):
                    print("Other", line)
                elif p3.match(line):
                    print("ISSUE", line)

        assert proc.returncode == 0

        diff = ['diff','-r','out','out-gold']

        with Popen( cmd, stdout=PIPE) as proc_diff:
            proc_diff.communicate()

        assert proc_diff.returncode == 0

def test_two():
    with working_directory('tests/vga2'):
        cmd = ['../../bin/amsr.exe','-file','INPUT/ctrl.txt']

        with Popen( cmd, stdout=PIPE) as proc:
            p = re.compile( r'.*Net (\S+) has no opens\.\s*$')
            p2 = re.compile( r'.*open.*')
            p3 = re.compile( r'.*ISSUE.*')

            for line in proc.stdout:
                line = line.decode().rstrip('\n')
                if p.match(line):
                    pass
#                    print(line)
                elif p2.match(line):
                    print("Other", line)
                elif p3.match(line):
                    print("ISSUE", line)

        assert proc.returncode == 0

        diff = ['diff','-r','out','out-gold']

        with Popen( cmd, stdout=PIPE) as proc_diff:
            proc_diff.communicate()

        assert proc_diff.returncode == 0

def test_three():
    with working_directory('tests/vga3'):
        cmd = ['../../bin/amsr.exe','-file','INPUT/ctrl.txt']

        with Popen( cmd, stdout=PIPE) as proc:
            p = re.compile( r'.*Net (\S+) has no opens\.\s*$')
            p2 = re.compile( r'.*open.*')
            p3 = re.compile( r'.*ISSUE.*')

            for line in proc.stdout:
                line = line.decode().rstrip('\n')
                if p.match(line):
                    pass
#                    print(line)
                elif p2.match(line):
                    print("Other", line)
                elif p3.match(line):
                    print("ISSUE", line)

        assert proc.returncode == 0

        diff = ['diff','-r','out','out-gold']

        with Popen( cmd, stdout=PIPE) as proc_diff:
            proc_diff.communicate()

        assert proc_diff.returncode == 0
