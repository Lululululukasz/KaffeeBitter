#!/usr/bin/python

import sys

'''
Killbom removes all UTF-8 byte order marks from the specified files.
It removes all sequences of '%EF%BB%BF'.

**Unsuitable for binary files.**
'''

# Set this to whatever you want to name this script.
# (Command to enter at shell.)
script_name = 'killbom'


def killBOM(filename):
    '''
    Remove UTF-8 BOM from the file `filename`.
    '''

    try:
        text = open(filename, 'rb').read()

        outfile = open(filename, 'wb')
        outfile.write(text.replace(b'\xEF\xBB\xBF', b''))
        outfile.close()
    except IOError as error:
        sys.stderr.write("IO error '{}': {}\n".format(
            filename.replace("'","\\'"),
            error
        ))


def usage():
    '''
    Print usage message to stderr.
    '''

    sys.stderr.write('''Usage: {0} [file] ...
{0} removes all UTF-8 BOM marks from files.

WARNING: It really removes all sequences of '\\0xEF\\0xBB\\0xBF'.

WARNING: It accepts no options. All arguments are treated as filenames.
'''.format(script_name))


def main():
    '''
    Call `usage` and call `killBOM` for all specified arguments.
    '''

    if len(sys.argv) > 1:
        for filename in sys.argv[1:]:
            killBOM(filename)
    else:
        usage()


if __name__ == '__main__':
    main()
