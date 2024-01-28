#!/usr/bin/env python3
import pytest
import subprocess
import shlex
import sys
import os
import math
import shutil
from pathlib import Path

from datetime import datetime
from datetime import timedelta
from datetime import timezone


EXIT_FAILURE = 1
EXIT_SUCCESS = 0

DATA_TYPE_IMAGETTE  = 1
DATA_TYPE_IMAGETTE_ADAPTIVE  = 2

GENERIC_HEADER_SIZE = 32
IMAGETTE_HEADER_SIZE = GENERIC_HEADER_SIZE+4
IMAGETTE_ADAPTIVE_HEADER_SIZE = GENERIC_HEADER_SIZE+12
NON_IMAGETTE_HEADER_SIZE = GENERIC_HEADER_SIZE+32

WINE_TEST_SETUP = False
my_env=None
if sys.platform != 'win32' and sys.platform != 'cygwin':
    if Path('programs/cmp_tool.exe').exists():
        # try to detect cross compile setup
        # and use wine to run windows executable
        WINE_TEST_SETUP = True
        # disable wine debug output
        my_env = os.environ.copy()
        my_env["WINEDEBUG"] = f"-all"

def call_cmp_tool(args):
    if WINE_TEST_SETUP:
        args = shlex.split("wine64 programs/cmp_tool.exe " + args)
    else:
        args = shlex.split("./programs/cmp_tool " + args)
    print(args)

    try:
        with subprocess.Popen(args, stdout=subprocess.PIPE, stderr=subprocess.PIPE, universal_newlines=True, env=my_env) as proc:
            stdout, stderr = proc.communicate()
            return (proc.returncode, stdout, stderr)
    except:
        print('Could not find the cmp_tool or has no execute rights!')
        assert()


def parse_key_value(str):
    dir = {}
    for line in str.splitlines():
        if line == '':
            continue
        if line[0] == '#' or line[0] == '\n':
            continue
        name, var = line.partition(" = ")[::2]
        dir[name.strip()] = var
    return dir


def del_file(filePath):
    if os.path.exists(filePath):
        os.remove(filePath)
    else:
        print("The file %s could not be deleted because it does not exist!" % (filePath))


def del_directory(directoryPath):
    # Try to remove the tree; if it fails, throw an error using try...except.
    try:
        shutil.rmtree(directoryPath)
    except OSError as e:
        print("Error: %s - %s." % (e.filename, e.strerror))


def cuc_timestamp(now):
    epoch = datetime(2020, 1, 1, tzinfo=timezone.utc)
    timestamp = (now - epoch).total_seconds()

    cuc_coarse = int( math.floor(timestamp) * 256 * 256)
    cuc_fine = int(math.floor((timestamp - math.floor(timestamp))*256*256))

    cuc = int(cuc_coarse)+int(cuc_fine)

    return cuc


def read_in_cmp_header(compressed_string):

    header = { 'asw_version_id'     : { 'value': -1, 'bits': 32 },
               'cmp_ent_size'       : { 'value': -1, 'bits': 24 },
               'original_size'      : { 'value': -1, 'bits': 24 },
               'start_time'         : { 'value': -1, 'bits': 48 },
               'end_timestamp'      : { 'value': -1, 'bits': 48 },
               'data_type'          : { 'value': -1, 'bits': 16 },
               'cmp_mode_used'      : { 'value': -1, 'bits': 8 },
               'model_value_used'   : { 'value': -1, 'bits': 8 },
               'model_id'           : { 'value': -1, 'bits': 16 },
               'model_counter'      : { 'value': -1, 'bits': 8 },
               'spare'              : { 'value': -1, 'bits': 8 },
               'lossy_cmp_par_used' : { 'value': -1, 'bits': 16 },
               'spill_used'         : { 'value': -1, 'bits': 16 },
               'golomb_par_used'    : { 'value': -1, 'bits': 8 },
               'ap1_spill_used'     : { 'value': -1, 'bits': 16 },
               'ap1_golomb_par'     : { 'value': -1, 'bits': 8 },
               'ap2_spill_used'     : { 'value': -1, 'bits': 16 },
               'ap2_golomb_par'     : { 'value': -1, 'bits': 8 },
               'spare_ap_ima'       : { 'value': -1, 'bits': 24 },
               'spill_1_used'       : { 'value': -1, 'bits': 24 },
               'cmp_par_1_used'     : { 'value': -1, 'bits': 16 },
               'spill_2_used'       : { 'value': -1, 'bits': 24 },
               'cmp_par_2_used'     : { 'value': -1, 'bits': 16 },
               'spill_3_used'       : { 'value': -1, 'bits': 24 },
               'cmp_par_3_used'     : { 'value': -1, 'bits': 16 },
               'spill_4_used'       : { 'value': -1, 'bits': 24 },
               'cmp_par_4_used'     : { 'value': -1, 'bits': 16 },
               'spill_5_used'       : { 'value': -1, 'bits': 24 },
               'cmp_par_5_used'     : { 'value': -1, 'bits': 16 },
               'spill_6_used'       : { 'value': -1, 'bits': 24 },
               'cmp_par_6_used'     : { 'value': -1, 'bits': 16 },
               'spare_non_ima'      : { 'value': -1, 'bits': 16 },
               'compressed_data'    : { 'value': -1, 'bits': -1 }}

    l = 0
    compressed_string = compressed_string.replace(" ", "").replace("\n", "")  # remove white spaces

    # Generic Header
    for i, data_field in enumerate(header):
        if header[data_field]['bits'] % 4 != 0:
            raise Exception("only work with 4 bit aligned data fields")
        byte_len = header[data_field]['bits']//4
        header[data_field]['value'] = int(compressed_string[l:l+byte_len], 16)
        l += byte_len
        # end of the GENERIC_HEADER
        if l/2 >= GENERIC_HEADER_SIZE:
            break

    data_type = int(header['data_type']['value'])
    # Imagette Headers
    if data_type == 1 or data_type == 2:
        for data_field in list(header)[12:19]:
            if header[data_field]['bits'] % 4 != 0:
                raise Exception("only work with 4 bit aligned data fields")
            byte_len = header[data_field]['bits']//4
            header[data_field]['value'] = int(compressed_string[l:l+byte_len], 16)
            l += byte_len
            # skip adaptive stuff if non adaptive header
            if data_field == 'golomb_par_used' and  data_type == 1:
                l += 2  # spare bits
                assert(l/2==IMAGETTE_HEADER_SIZE)
                break
        if data_type == 2:
            assert(l/2==IMAGETTE_ADAPTIVE_HEADER_SIZE)

    # Non-Imagette Headers
    elif data_type < 24:
        for data_field in list(header)[19:31]:
            if header[data_field]['bits'] % 4 != 0:
                raise Exception("only work with 4 bit aligned data fields")
            byte_len = header[data_field]['bits']//4
            header[data_field]['value'] = int(compressed_string[l:l+byte_len], 16)
            l += byte_len
        assert(l/2 == NON_IMAGETTE_HEADER_SIZE)
    else:
        if (data_type >> 15) == 0: # check if raw mode is set
            raise Exception("data_type unknown")

    header['compressed_data']['value'] = compressed_string[l::]

    # version conversion fuu
    version_id = header['asw_version_id']['value']
    if version_id & 0x80000000:
        header['asw_version_id']['value'] = "%.2f" % (int(version_id & 0x7FFF0000)//0x8000 + int(version_id&0xFFFF)*0.01)

    return header


def build_generic_header(asw_version_id, cmp_ent_size, original_size,
                         start_time, end_time, data_type, cmp_mode_used,
                         model_value_used, model_id, model_counter,
                         lossy_cmp_par_used):

    generic_header = ("%08X " % asw_version_id + "%06X " % cmp_ent_size +
        "%06X " % original_size + "%012X " % start_time +
        "%012X " % end_time + "%04X " % data_type +
        "%02X " % cmp_mode_used + "%02X " % model_value_used +
        "%04X " % model_id + "%02X " % model_counter+ "00 " +
        "%04X " % lossy_cmp_par_used)

    assert(len(generic_header.replace(" ", ""))/2 == GENERIC_HEADER_SIZE)
    return generic_header


def build_imagette_header(spill_used, golomb_par_used):
   ima_header = "%04X %02X 00 " % (spill_used, golomb_par_used)
   assert(len(ima_header.replace(" ", ""))/2 == IMAGETTE_HEADER_SIZE - GENERIC_HEADER_SIZE)
   return ima_header


#get version
returncode, stdout, stderr = call_cmp_tool("--version")
assert(returncode == EXIT_SUCCESS)
assert(stderr == "")
VERSION = stdout.split()[2]

#get cmp_tool path
returncode, stdout, stderr = call_cmp_tool("")
assert(returncode == EXIT_FAILURE)
assert(stderr == "")
start_parh = stdout.rpartition('usage: ')[2]
PATH_CMP_TOOL = start_parh[: start_parh.find(' [options] [<argument>]\n')]

HELP_STRING = \
"""usage: %s [options] [<argument>]
General Options:
  -h, --help               Print this help text and exit
  -o <prefix>              Use the <prefix> for output files
  -n, --model_cfg          Print a default model configuration and exit
  --diff_cfg               Print a default 1d-differencing configuration and exit
  -b, --binary             Read and write files in binary format
  -a, --rdcu_par           Add additional RDCU control parameters
  -V, --version            Print program version and exit
  -v, -vv, --verbose       Print various debugging information, -vv is extra verbose
Compression Options:
  -c <file>                File containing the compressing configuration
  -d <file>                File containing the data to be compressed
  -m <file>                File containing the model of the data to be compressed
  --no_header              Do not add a compression entity header in front of the compressed data
  --rdcu_pkt               Generate RMAP packets for an RDCU compression
  --last_info <.info file> Generate RMAP packets for an RDCU compression with parallel read of the last results
Decompression Options:
  -d <file>                File containing the compressed data
  -m <file>                File containing the model of the compressed data
  -i <file>                File containing the decompression information (required if --no_header was used)
Guessing Options:
  --guess <mode>           Search for a good configuration for compression <mode>
  -d <file>                File containing the data to be compressed
  -m <file>                File containing the model of the data to be compressed
  --guess_level <level>    Set guess level to <level> (optional)
""" % (PATH_CMP_TOOL)

welcome_str = "### PLATO Compression/Decompression Tool Version %s ###"% (VERSION)

CMP_START_STR = \
    '#'*len(welcome_str)+'\n'+ \
    welcome_str+'\n'+ \
    '#'*len(welcome_str)+'\n'

CMP_START_STR_CMP = CMP_START_STR + "## Starting the compression ##\n"
CMP_START_STR_DECMP = CMP_START_STR + "## Starting the decompression ##\n"
CMP_START_STR_GUESS = CMP_START_STR + "## Search for a good set of compression parameters ##\n"


def test_no_option():
    returncode, stdout, stderr = call_cmp_tool("")
    assert(returncode == EXIT_FAILURE)
    assert(stdout == HELP_STRING)
    assert(stderr == "")


def test_invalid_option():
    args = ['-q', '--not_used']
    for arg in args:
        returncode, stdout, stderr = call_cmp_tool(arg)
        assert(returncode == EXIT_FAILURE)
        assert(stdout == HELP_STRING)
        if arg == '-q':
            if sys.platform == 'win32' or sys.platform == 'cygwin' or WINE_TEST_SETUP:
                assert(stderr == "%s: unknown option -- q\n" % (PATH_CMP_TOOL))
            elif sys.platform == 'linux':
                assert(stderr == "%s: invalid option -- 'q'\n" % (PATH_CMP_TOOL))
            else:
                assert(stderr == "cmp_tool: invalid option -- q\n")
        else:
            if sys.platform == 'win32' or sys.platform == 'cygwin' or WINE_TEST_SETUP:
                assert(stderr == "%s: unknown option -- not_used\n" % (PATH_CMP_TOOL))
            elif sys.platform == 'linux':
                assert(stderr == "%s: unrecognized option '--not_used'\n" % (PATH_CMP_TOOL))
            else:
                assert(stderr == "cmp_tool: unrecognized option `--not_used'\n")

# def option requires an argument


def test_help():
    args = ['-h', '--help']
    for arg in args:
        returncode, stdout, stderr = call_cmp_tool(arg)
        assert(returncode == EXIT_SUCCESS)
        assert(stdout == HELP_STRING)
        assert(stderr == "")


def test_version():
    args = ['-V', '--version']
    for arg in args:
        returncode, stdout, stderr = call_cmp_tool(arg)
        assert(returncode == EXIT_SUCCESS)
        assert(stdout == "cmp_tool version %s\n" % (VERSION))
        assert(stderr == "")


def test_print_diff_cfg():
    # comments in the cfg file is not checked
    args = ['--diff_cfg', '--diff_cfg -a', '--diff_cfg --rdcu_par']
    for i, arg in enumerate(args):
        returncode, stdout, stderr = call_cmp_tool(arg)
        assert(stderr == "")
        assert(returncode == EXIT_SUCCESS)

        cfg = parse_key_value(stdout)
        assert(cfg['cmp_mode'] == '2')
        assert(cfg['samples'] == '0')
        assert(cfg['buffer_length'] == '0')
        assert(cfg['golomb_par'] == '7')
        assert(cfg['spill'] == '60')
        assert(cfg['model_value'] == '8')
        assert(cfg['round'] == '0')
        if i > 0:
            assert(cfg['ap1_golomb_par'] == '6')
            assert(cfg['ap1_spill'] == '48')
            assert(cfg['ap2_golomb_par'] == '8')
            assert(cfg['ap2_spill'] == '72')
            assert(cfg['rdcu_data_adr'] == '0x000000')
            assert(cfg['rdcu_model_adr'] == '0x000000')
            assert(cfg['rdcu_new_model_adr'] == '0x000000')
            assert(cfg['rdcu_buffer_adr'] == '0x600000')


def test_print_model_cfg():
    # comments in the cfg file is not checked
    args = ['-n', '--model_cfg', '-na', '--model_cfg --rdcu_par']
    for i, arg in enumerate(args):
        returncode, stdout, stderr = call_cmp_tool(arg)
        assert(returncode == EXIT_SUCCESS)
        assert(stderr == "")

        cfg = parse_key_value(stdout)
        assert(cfg['cmp_mode'] == '3')
        assert(cfg['samples'] == '0')
        assert(cfg['buffer_length'] == '0')
        assert(cfg['golomb_par'] == '4')
        assert(cfg['spill'] == '48')
        assert(cfg['model_value'] == '8')
        assert(cfg['round'] == '0')
        if i > 1:
            assert(cfg['ap1_golomb_par'] == '3')
            assert(cfg['ap1_spill'] == '35')
            assert(cfg['ap2_golomb_par'] == '5')
            assert(cfg['ap2_spill'] == '60')
            assert(cfg['rdcu_data_adr'] == '0x000000')
            assert(cfg['rdcu_model_adr'] == '0x200000')
            assert(cfg['rdcu_new_model_adr'] == '0x400000')
            assert(cfg['rdcu_buffer_adr'] == '0x600000')


def test_no_data_file_name():
    returncode, stdout, stderr = call_cmp_tool("-d no_data.data --no_header")
    assert(returncode == EXIT_FAILURE)
    assert(stdout == CMP_START_STR)
    assert(stderr == "cmp_tool: No configuration file (-c option) or decompression information file (-i option) specified.\n")


def test_no_c_or_i_file():
    returncode, stdout, stderr = call_cmp_tool("-c no_cfg.cfg")
    assert(returncode == EXIT_FAILURE)
    assert(stdout == CMP_START_STR)
    assert(stderr == "cmp_tool: No data file (-d option) specified.\n")


def test_no_cfg_exist_test():
    returncode, stdout, stderr = call_cmp_tool("-d no_data.dat -c no_cfg.cfg")
    assert(returncode == EXIT_FAILURE)
    assert(stdout == CMP_START_STR_CMP +
           "Importing configuration file no_cfg.cfg ... FAILED\n")
    assert(stderr == "cmp_tool: no_cfg.cfg: No such file or directory\n")


def test_too_many_arg():
    returncode, stdout, stderr = call_cmp_tool("argument")
    assert(returncode == EXIT_FAILURE)
    assert(stdout == "cmp_tool: To many arguments.\n"+HELP_STRING)
    assert(stderr == "")


def test_compression_diff():
    # generate test data
    data = '00 01 00 02 00 03 00 04 00 05 \n'
    data_file_name = 'data.dat'
    cfg_file_name = 'diff.cfg'
    output_prefix = 'diff_cmp'
    try:
        with open(data_file_name, 'w', encoding='utf-8') as f:
            f.write(data)

        # generate test configuration
        with open(cfg_file_name, 'w', encoding='utf-8') as f:
            returncode, stdout, stderr = call_cmp_tool("--diff_cfg")
            assert(stderr == "")
            assert(returncode == EXIT_SUCCESS)
            f.write(stdout)

        add_args = [" --no_header", ""]
        for add_arg in add_args:
            # compression
            returncode, stdout, stderr = call_cmp_tool(
                " -c "+cfg_file_name+" -d "+data_file_name + " -o "+output_prefix+add_arg)

            # check compression results
            assert(stderr == "")
            assert(returncode == EXIT_SUCCESS)
            assert(stdout == CMP_START_STR_CMP +
                   "Importing configuration file %s ... DONE\n" % (cfg_file_name) +
                   "Importing data file %s ... \n" % (data_file_name) +
                   "No samples parameter set. Use samples = 5.\n" +
                   "... DONE\n" +
                   "No buffer_length parameter set. Use buffer_length = 12 as compression buffer size.\n" +
                   "Compress data ... DONE\n" +
                   "Write compressed data to file %s.cmp ... DONE\n" % (output_prefix) +
                   "%s" % ((lambda arg : "Write decompression information to file %s.info ... DONE\n" % (output_prefix)
                            if arg == " --no_header" else "")(add_arg))
                   )
             # check compressed data
            cmp_data = "44 44 40 \n"
            with open(output_prefix+".cmp", encoding='utf-8') as f:
                if add_arg == " --no_header":
                    # check compressed data file
                    assert(f.read() == cmp_data)
                    # check info file
                    with open(output_prefix+".info", encoding='utf-8') as f:
                        info = parse_key_value(f.read())
                    assert(info['cmp_mode_used'] == '2')
                    # assert(info['model_value_used'] == '8')  # not needed for diff
                    assert(info['round_used'] == '0')
                    assert(info['spill_used'] == '60')
                    assert(info['golomb_par_used'] == '7')
                    assert(info['samples_used'] == '5')
                    assert(info['cmp_size'] == '20')
                    assert(info['cmp_err'] == '0')
                else:
                    header = read_in_cmp_header(f.read())
                    assert(header['asw_version_id']['value'] == VERSION.split('-')[0])
                    assert(header['cmp_ent_size']['value'] == IMAGETTE_HEADER_SIZE+3)
                    assert(header['original_size']['value'] == 10)
                    # todo
                    assert(header['start_time']['value'] < cuc_timestamp(datetime.now(timezone.utc)))
                    # todo
                    assert(header['end_timestamp']['value'] < cuc_timestamp(datetime.now(timezone.utc)))
                    assert(header['data_type']['value'] == 1)
                    assert(header['cmp_mode_used']['value'] == 2)
                    # assert(header['model_value_used']['value'] == 8)
                    assert(header['model_id']['value'] == 53264)
                    assert(header['model_counter']['value'] == 0)
                    assert(header['lossy_cmp_par_used']['value'] == 0)
                    assert(header['spill_used']['value'] == 60)
                    assert(header['golomb_par_used']['value'] == 7)
                    assert(header['compressed_data']['value'] == "444440")

            # decompression
            if add_arg == " --no_header":
                returncode, stdout, stderr = call_cmp_tool(
                    " -i "+output_prefix+".info -d "+output_prefix+".cmp -o "+output_prefix)
            else:
                returncode, stdout, stderr = call_cmp_tool("-d "+output_prefix+".cmp -o "+output_prefix)

            # check decompression
            assert(stderr == "")
            assert(stdout == CMP_START_STR_DECMP +
                   "%s" % ((lambda arg : "Importing decompression information file %s.info ... DONE\n" % (output_prefix)
                            if "--no_header" in arg else "")(add_arg)) +
                   "Importing compressed data file %s.cmp ... DONE\n" % (output_prefix) +
                   # "%s" % ((lambda arg : "Parse the compression entity header ... DONE\n"
                   #          if not "--no_header" in arg else "")(add_arg)) +
                   "Decompress data ... DONE\n" +
                   "Write decompressed data to file %s.dat ... DONE\n" % (output_prefix))

            assert(returncode == EXIT_SUCCESS)
            with open(output_prefix+".dat", encoding='utf-8') as f:
                assert(f.read() == data)  # decompressed data == input data

    # clean up
    finally:
        del_file(data_file_name)
        del_file(cfg_file_name)
        del_file(output_prefix+'.cmp')
        del_file(output_prefix+'.info')
        del_file(output_prefix+'.dat')


def test_model_compression():
    # generate test data
    data = '00 01 00 02 00 03 00 04 00 05 \n'
    model = '00 00 00 01 00 02 00 03 00 04 \n'
    data_byte = bytearray.fromhex(data)
    model_byte = bytearray.fromhex(model)
    data_file_name = 'data.dat'
    model_file_name = 'model.dat'
    cfg_file_name = 'model.cfg'
    output_prefix1 = 'model_cmp'
    output_prefix2 = 'model_decmp'
    try:
        with open(data_file_name, 'w', encoding='utf-8') as f:
            f.write(data)
        with open(model_file_name, 'w', encoding='utf-8') as f:
            f.write(model)

        # generate test configuration
        with open(cfg_file_name, 'w', encoding='utf-8') as f:
            returncode, stdout, stderr = call_cmp_tool("--model_cfg --rdcu_par")
            assert(returncode == EXIT_SUCCESS)
            assert(stderr == "")
            cfg = parse_key_value(stdout)
            cfg['cmp_mode'] = 'MODE_MODEL_MULTI\r'
            cfg['model_value'] = '0'
            cfg["samples"] = '5'
            cfg["buffer_length"] = '2'
            cfg["ap1_golomb_par"] = '20'
            cfg["ap1_spill"] = '70'
            cfg["ap2_golomb_par"] = '63'
            cfg["ap1_spill"] = '6'
            for key, value in cfg.items():
                f.write(key + ' = ' + str(value) + '\n')

        add_args = [" --no_header", " --no_header --rdcu_par", "", " --binary"]
        for add_arg in add_args:
            if "--binary" in add_arg:
                with open(data_file_name, 'wb') as f:
                    f.write(data_byte)
                with open(model_file_name, 'wb') as f:
                    f.write(model_byte)
            # compression
            returncode, stdout, stderr = call_cmp_tool(
                " -c "+cfg_file_name+" -d "+data_file_name+" -m "+model_file_name+" -o " +output_prefix1+add_arg)

            # check compression results
            assert(stderr == "")
            assert(stdout == CMP_START_STR_CMP +
                   "Importing configuration file %s ... DONE\n" % (cfg_file_name) +
                   "Importing data file %s ... DONE\n" % (data_file_name) +
                   "Importing model file %s ... DONE\n" % (model_file_name) +
                   "Compress data ... DONE\n" +
                   "Write compressed data to file %s.cmp ... DONE\n" % (output_prefix1) +
                   "%s" % ((lambda arg : "Write decompression information to file %s.info ... DONE\n" % (output_prefix1)
                            if " --no_header" in add_arg else "")(add_arg)) +
                   "Write updated model to file %s_upmodel.dat ... DONE\n" % (output_prefix1)
            )
            assert(returncode == EXIT_SUCCESS)

        if "--no_header" in add_arg:
            # check compressed data
            with open(output_prefix1+".cmp", encoding='utf-8') as f:
                assert(f.read() == "49 24 \n")
            # check info file
            with open(output_prefix1+".info", encoding='utf-8') as f:
                info = parse_key_value(f.read())
            assert(info['cmp_mode_used'] == '3')
            assert(info['model_value_used'] == cfg['model_value'])
            assert(info['round_used'] == cfg['round'])
            assert(info['spill_used'] == cfg['spill'])
            assert(info['golomb_par_used'] == cfg['golomb_par'])
            assert(info['samples_used'] == cfg['samples'])
            assert(info['cmp_size'] == '15')
            assert(info['cmp_err'] == '0')
            if "--rdcu_par" in add_arg:
                assert(info['ap1_cmp_size'] == '25')
                assert(info['ap2_cmp_size'] == '35')
                assert(info['rdcu_new_model_adr_used'] == cfg['rdcu_new_model_adr'])
                assert(info['rdcu_cmp_adr_used'] == cfg['rdcu_buffer_adr'])
            else:
                if not "--binary" in add_arg:
                    with open(output_prefix1+".cmp", encoding='utf-8') as f:
                        header = read_in_cmp_header(f.read())
                else:
                    with open(output_prefix1+".cmp", 'rb') as f:
                        header = read_in_cmp_header(bytearray(f.read()).hex())

                assert(header['asw_version_id']['value'] == VERSION.split('-')[0])
                assert(header['cmp_ent_size']['value'] == IMAGETTE_ADAPTIVE_HEADER_SIZE+2)
                assert(header['original_size']['value'] == 10)
                # todo
                assert(header['start_time']['value'] < cuc_timestamp(datetime.now(timezone.utc)))
                #todo
                assert(header['end_timestamp']['value'] < cuc_timestamp(datetime.now(timezone.utc)))
                assert(header['data_type']['value'] == DATA_TYPE_IMAGETTE_ADAPTIVE)
                assert(header['cmp_mode_used']['value'] == 3)
                assert(header['model_value_used']['value'] == int(cfg['model_value']))
                assert(header['model_id']['value'] == 53264)
                assert(header['model_counter']['value'] == 1)
                assert(header['lossy_cmp_par_used']['value'] == int(cfg['round']))
                assert(header['spill_used']['value'] == int(cfg['spill']))
                assert(header['golomb_par_used']['value'] == int(cfg['golomb_par']))
                assert(header['compressed_data']['value'] == "4924")

            # decompression
            if "--no_header" in add_arg:
                returncode, stdout, stderr = call_cmp_tool(
                    " -i "+output_prefix1+".info -d "+output_prefix1+".cmp -m "+model_file_name+" -o "+output_prefix2)
            elif "--binary" in add_arg:
                returncode, stdout, stderr = call_cmp_tool("-d "+output_prefix1+".cmp -m "+model_file_name+" -o "+output_prefix2+" --binary")
            else:
                returncode, stdout, stderr = call_cmp_tool("-d "+output_prefix1+".cmp -m "+model_file_name+" -o "+output_prefix2)
            assert(stderr == "")
            assert(stdout == CMP_START_STR_DECMP +
                   "%s" % ((lambda arg : "Importing decompression information file %s.info ... DONE\n" % (output_prefix1)
                            if "--no_header" in arg else "")(add_arg)) +
                   "Importing compressed data file %s.cmp ... DONE\n" % (output_prefix1) +
                   "Importing model file %s ... DONE\n" % (model_file_name) +
                   "Decompress data ... DONE\n" +
                   "Write decompressed data to file %s.dat ... DONE\n" % (output_prefix2) +
                   "Write updated model to file %s_upmodel.dat ... DONE\n" % (output_prefix2))
            assert(returncode == EXIT_SUCCESS)
            # check compressed data

            if not "--binary" in add_arg:
                with open(output_prefix2+".dat", encoding='utf-8') as f:
                    assert(f.read() == data)
                with open(output_prefix1+"_upmodel.dat", encoding='utf-8') as f1:
                    with open(output_prefix2+"_upmodel.dat", encoding='utf-8') as f2:
                        assert(f1.read() == f2.read() == data) # upmodel == data -> model_value = 0
                               # '00 00 00 01 00 02 00 03 00 04 \n')
            else:
                with open(output_prefix2+".dat", 'rb') as f:
                    assert(f.read() == data_byte)
                with open(output_prefix1+"_upmodel.dat", 'rb') as f1:
                    with open(output_prefix2+"_upmodel.dat", 'rb') as f2:
                        assert(f1.read() == f2.read() == data_byte)
    # clean up
    finally:
        del_file(data_file_name)
        del_file(model_file_name)
        del_file(cfg_file_name)
        del_file(output_prefix1+'.cmp')
        del_file(output_prefix1+'.info')
        del_file(output_prefix1+'_upmodel.dat')
        del_file(output_prefix2+'.dat')
        del_file(output_prefix2+'_upmodel.dat')


def test_raw_mode_compression():
    data = '00 01 00 02 00 03 00 04 00 05 \n'
    cfg = "cmp_mode = 0\n" + "samples = 5\n" + "buffer_length = 6\n"

    cfg_file_name = 'raw.cfg'
    data_file_name = 'raw.data'
    output_prefix = 'raw_compression'
    args = ["-c %s -d %s -o %s --no_header" %(cfg_file_name, data_file_name, output_prefix),
            "-c %s -d %s -o %s " %(cfg_file_name, data_file_name, output_prefix)]

    try:
        with open(cfg_file_name, 'w') as f:
            f.write(cfg)
        with open(data_file_name, 'w') as f:
            f.write(data)

        for arg in args:
            returncode, stdout, stderr = call_cmp_tool(arg)
            assert(stderr == "")
            assert(returncode == EXIT_SUCCESS)
            assert(stdout == CMP_START_STR_CMP +
                   "Importing configuration file %s ... DONE\n" % (cfg_file_name) +
                   "Importing data file %s ... DONE\n" % (data_file_name) +
                   "Compress data ... DONE\n" +
                   "Write compressed data to file %s.cmp ... DONE\n" % (output_prefix) +
                   "%s" % ((lambda arg : "Write decompression information to file %s.info ... DONE\n" % (output_prefix)
                            if "--no_header" in arg  else "")(arg))
                   )

             # check compressed data
            with open(output_prefix+".cmp", encoding='utf-8') as f:
                if "--no_header" in arg:
                    # check compressed data file
                    assert(f.read() == data)#[:-1]+"00 00 \n")
                    # check info file
                    with open(output_prefix+".info", encoding='utf-8') as f:
                        info = parse_key_value(f.read())
                    assert(info['cmp_mode_used'] == '0')
                    # assert(info['model_value_used'] == '8')  # not needed for diff
                    assert(info['round_used'] == '0')
                    # assert(info['spill_used'] == '60')
                    # assert(info['golomb_par_used'] == '7')
                    assert(info['samples_used'] == '5')
                    assert(info['cmp_size'] == '80')
                    assert(info['cmp_err'] == '0')
                else:
                    header = read_in_cmp_header(f.read())
                    assert(header['asw_version_id']['value'] == VERSION.split('-')[0])
                    assert(header['cmp_ent_size']['value'] == GENERIC_HEADER_SIZE+10)
                    assert(header['original_size']['value'] == 10)
                    # todo
                    assert(header['start_time']['value'] < cuc_timestamp(datetime.now(timezone.utc)))
                    #todo
                    assert(header['end_timestamp']['value'] < cuc_timestamp(datetime.now(timezone.utc)))
                    assert(header['data_type']['value'] == 1+0x8000)
                    # assert(header['cmp_mode_used']['value'] == 2)
                    # assert(header['model_value_used']['value'] == 8)
                    assert(header['model_id']['value'] == 53264)
                    assert(header['model_counter']['value'] == 0)
                    assert(header['lossy_cmp_par_used']['value'] == 0)
                    # assert(header['spill_used']['value'] == 60)
                    # assert(header['golomb_par_used']['value'] == 7)
                    assert(header['compressed_data']['value'] == data[:-1].replace(" ",""))

            # decompression
            if "--no_header" in arg:
                returncode, stdout, stderr = call_cmp_tool(
                    " -i "+output_prefix+".info -d "+output_prefix+".cmp -o "+output_prefix)
            else:
                returncode, stdout, stderr = call_cmp_tool("-d "+output_prefix+".cmp -o "+output_prefix)


            # check decompression
            assert(stderr == "")
            assert(stdout == CMP_START_STR_DECMP +
                   "%s" % ((lambda arg : "Importing decompression information file %s.info ... DONE\n" % (output_prefix)
                            if "--no_header" in arg else "")(arg)) +
                   "Importing compressed data file %s.cmp ... DONE\n" % (output_prefix) +
                   "Decompress data ... DONE\n" +
                   "Write decompressed data to file %s.dat ... DONE\n" % (output_prefix))
            assert(returncode == EXIT_SUCCESS)
            with open(output_prefix+".dat", encoding='utf-8') as f:
                assert(f.read() == data)  # decompressed data == input data
    finally:
        del_file(cfg_file_name)
        del_file(data_file_name)
        del_file("%s.cmp" %(output_prefix))
        del_file("%s.info" %(output_prefix))
        del_file("%s.dat" %(output_prefix))


def test_guess_option():
    # generate test data
    data = '00 01 00 01 00 01 00 01 00 01 \n'
    model = '00 00 00 00 00 00 00 00 00 00 \n'
    data_file_name = 'data.dat'
    model_file_name = 'model.dat'
    args = [
        ['guess_RDCU_diff',  '--guess RDCU -d %s -o guess --rdcu_par --no_header' %
            (data_file_name)],
        ['guess_RDCU_model', '--guess RDCU -d %s -m %s -o guess --rdcu_par' %
            (data_file_name, model_file_name)],
        ['guess_level_3',    '--guess MODE_DIFF_MULTI --guess_level 3 -d %s -o guess' %
            (data_file_name)],
        ['cfg_folder_not_exist', '--guess RDCU -d ' + data_file_name+' -o not_exist/guess'],
        ['guess_level_not_supported',    '--guess MODE_DIFF_MULTI --guess_level 10 -d %s -o guess' %
            (data_file_name)],
        ['guess_unknown_mode', '--guess MODE_UNKNOWN -d %s -o guess' %
            (data_file_name)],
        ['guess_model_mode_no_model ', '--guess 1 -d %s -o guess' %
            (data_file_name)],
    ]
    try:
        with open(data_file_name, 'w', encoding='utf-8') as f:
            f.write(data)
        with open(model_file_name, 'w', encoding='utf-8') as f:
            f.write(model)

        for sub_test, arg in args:
            returncode, stdout, stderr = call_cmp_tool(arg)
            if sub_test == 'guess_RDCU_diff' or sub_test == 'guess_RDCU_model' or sub_test == 'guess_level_3':
                assert(stderr == "")
                assert(returncode == EXIT_SUCCESS)

                if sub_test == 'guess_RDCU_diff':
                    exp_out = ('', '2', '', '7.27')
                elif sub_test == 'guess_RDCU_model':
                    exp_out = (
                        'Importing model file model.dat ... DONE\n', '2', '', str(round((5*2)/(IMAGETTE_ADAPTIVE_HEADER_SIZE + 2), 2)))
                        #cmp_size:15bit-> 2byte cmp_data + 40byte header -> 16bit*5/(42Byte*8)
                elif sub_test == 'guess_level_3':
                    exp_out = (
                        '', '3', ' 0%... 6%... 13%... 19%... 25%... 32%... 38%... 44%... 50%... 57%... 64%... 72%... 80%... 88%... 94%... 100%',
                        str(round((5*2)/(IMAGETTE_HEADER_SIZE + 1), 3))) #11.43
                    # cmp_size:7 bit -> 1byte cmp_data + 34 byte header -> 16bit*5/(35Byte*8)
                else:
                    exp_out = ('', '', '')

                assert(stdout == CMP_START_STR_GUESS +
                       "Importing data file %s ... \n" % (data_file_name) +
                       "No samples parameter set. Use samples = 5.\n"
                       "... DONE\n"
                       "%s"
                       "Search for a good set of compression parameters (level: %s) ...%s DONE\n"
                       "Write the guessed compression configuration to file guess.cfg ... DONE\n"
                       "Guessed parameters can compress the data with a CR of %s.\n" % exp_out)

                # check configuration file
                with open('guess.cfg') as f:
                    cfg = parse_key_value(f.read())
                assert(cfg['samples'] == '5')
                assert(cfg['buffer_length'] == '2')
                assert(cfg['model_value'] == '11')
                assert(cfg['round'] == '0')
                if sub_test == 'guess_RDCU_diff':
                    assert(cfg['cmp_mode'] == '2')
                    assert(cfg['golomb_par'] == '2')
                    assert(cfg['spill'] == '22')
                    assert(cfg['ap1_golomb_par'] == '1')
                    assert(cfg['ap1_spill'] == '8')
                    assert(cfg['ap2_golomb_par'] == '3')
                    assert(cfg['ap2_spill'] == '35')
                    assert(cfg['rdcu_data_adr'] == '0x000000')
                    assert(cfg['rdcu_model_adr'] == '0x000000')
                    assert(cfg['rdcu_new_model_adr'] == '0x000000')
                    assert(cfg['rdcu_buffer_adr'] == '0x600000')
                elif sub_test == 'guess_RDCU_model':
                    assert(cfg['cmp_mode'] == '3')
                    assert(cfg['golomb_par'] == '1')
                    assert(cfg['spill'] == '8')
                    assert(cfg['ap1_golomb_par'] == '2')
                    assert(cfg['ap1_spill'] == '16')
                    assert(cfg['ap2_golomb_par'] == '3')
                    assert(cfg['ap2_spill'] == '23')
                    assert(cfg['rdcu_data_adr'] == '0x000000')
                    assert(cfg['rdcu_model_adr'] == '0x200000')
                    assert(cfg['rdcu_new_model_adr'] == '0x400000')
                    assert(cfg['rdcu_buffer_adr'] == '0x600000')
                else:
                    assert(cfg['cmp_mode'] == '4')
                    assert(cfg['golomb_par'] == '1')
                    assert(cfg['spill'] == '3')
            elif sub_test == 'cfg_folder_not_exist':
                # error case cfg directory does not exit
                assert(
                    stderr == "cmp_tool: not_exist/guess.cfg: No such file or directory\n")
                assert(returncode == EXIT_FAILURE)
                assert(stdout == CMP_START_STR_GUESS +
                       "Importing data file %s ... \n" % (data_file_name) +
                       "No samples parameter set. Use samples = 5.\n" +
                       "... DONE\n"
                       "Search for a good set of compression parameters (level: 2) ... DONE\n" +
                       "Write the guessed compression configuration to file not_exist/guess.cfg ... FAILED\n")
            elif sub_test == 'guess_level_not_supported':
                assert(stderr == "cmp_tool: guess level not supported!\n")
                assert(returncode == EXIT_FAILURE)
                assert(stdout == CMP_START_STR_GUESS +
                       "Importing data file %s ... \n" % (data_file_name) +
                       "No samples parameter set. Use samples = 5.\n" +
                       "... DONE\n"
                       "Search for a good set of compression parameters (level: 10) ... FAILED\n")
            elif sub_test == 'guess_unknown_mode':
                assert(
                    stderr == "cmp_tool: Error: unknown compression mode: MODE_UNKNOWN\n")
                assert(returncode == EXIT_FAILURE)
                assert(stdout == CMP_START_STR_GUESS +
                       "Importing data file %s ... \n" % (data_file_name) +
                       "No samples parameter set. Use samples = 5.\n" +
                       "... DONE\n"
                       "Search for a good set of compression parameters (level: 2) ... FAILED\n")
            else:
                assert(
                    stderr == "cmp_tool: Error: model mode needs model data (-m option)\n")
                assert(returncode == EXIT_FAILURE)
                assert(stdout == CMP_START_STR_GUESS +
                       "Importing data file %s ... \n" % (data_file_name) +
                       "No samples parameter set. Use samples = 5.\n" +
                       "... DONE\n"
                       "Search for a good set of compression parameters (level: 2) ... FAILED\n")
    # clean up
    finally:
        del_file(data_file_name)
        del_file(model_file_name)
        del_file('guess.cfg')


def test_small_buf_err():
    data = '00 01 0F 02 00 0C 00 42 00 23 \n'
    data_file_name = 'small_buf.dat'
    cfg_file_name = 'small_buf.cfg'
    arg = '-c %s -d %s' % (cfg_file_name, data_file_name)

    try:
        with open(data_file_name, 'w') as f:
            f.write(data)

        # create a compression file
        with open(cfg_file_name, 'w') as f:
            returncode, stdout, stderr = call_cmp_tool("--diff_cfg")
            assert(returncode == EXIT_SUCCESS)
            assert(stderr == "")
            cfg = parse_key_value(stdout)
            cfg["samples"] = '5'
            cfg["buffer_length"] = '2'
            for key, value in cfg.items():
                f.write(key + ' = ' + str(value) + '\n')

        returncode, stdout, stderr = call_cmp_tool(arg)
        assert(stdout == CMP_START_STR_CMP +
               "Importing configuration file %s ... DONE\n" % (cfg_file_name) +
               "Importing data file %s ... DONE\n" % (data_file_name) +
               "Compress data ... FAILED\n")
        # assert(stderr == "cmp_tool: the buffer for the compressed data is too small. Try a larger buffer_length parameter.\n")
        assert(stderr == "Error: The buffer for the compressed data is too small to hold the compressed data. Try a larger buffer_length parameter.\n")
        assert(returncode == EXIT_FAILURE)

    finally:
        del_file(data_file_name)
        del_file(cfg_file_name)


def test_empty_file():
    data = ''
    data_file_name = 'empty.dat'
    cfg_file_name = 'my.cfg'
    arg = '-c %s -d %s' % (cfg_file_name, data_file_name)

    try:
        with open(data_file_name, 'w') as f:
            f.write(data)

        # create a compression file
        with open(cfg_file_name, 'w') as f:
            returncode, stdout, stderr = call_cmp_tool("--diff_cfg")
            assert(returncode == EXIT_SUCCESS)
            assert(stderr == "")
            f.write(stdout)

        returncode, stdout, stderr = call_cmp_tool(arg)
        assert(stdout == CMP_START_STR_CMP +
               "Importing configuration file %s ... DONE\n" % (cfg_file_name) +
               "Importing data file %s ... FAILED\n" % (data_file_name))
        assert(stderr == "cmp_tool: %s: Warning: The file is empty.\n" % (data_file_name))
        assert(returncode == EXIT_FAILURE)

    finally:
        del_file(data_file_name)
        del_file(cfg_file_name)


def test_empty_cfg_and_info():
    data = '00 01 0F 02 00 0C 00 42 00 23 \n'
    empty_file_name = 'empty'
    args = ['-c %s -d %s' % (empty_file_name, empty_file_name),
            '-i %s -d %s' % (empty_file_name, empty_file_name)]

    try:
        with open(empty_file_name, 'w') as f:
            f.write(data)

        for i, arg in enumerate(args):
            returncode, stdout, stderr = call_cmp_tool(arg)
            if i == 0:
                assert(stdout == CMP_START_STR_CMP +
                       "Importing configuration file %s ... FAILED\n" % (empty_file_name))
                assert(stderr == "cmp_tool: Some parameters are missing. Check "
                       "if the following parameters: cmp_mode, golomb_par, "
                       "spill, samples and buffer_length are all set in the "
                       "configuration file.\n")
            else:
                assert(stdout == CMP_START_STR_DECMP +
                       "Importing decompression information file %s ... FAILED\n" % (empty_file_name))
                assert(stderr == "cmp_tool: Some parameters are missing. Check "
                       "if the following parameters: cmp_mode_used, "
                       "golomb_par_used, spill_used and samples_used "
                       "are all set in the information file.\n")
            assert(returncode == EXIT_FAILURE)

    finally:
        del_file(empty_file_name)


def test_wrong_formart_data_fiel():
    data = '42 Wrong format!'
    cfg = ("golomb_par = 1\n" + "spill = 4\n" + "cmp_mode = 2\n" +
           "samples = 5\n" + "buffer_length = 5\n")
    data_file_name = 'wrong.data'
    cfg_file_name = 'wrong.cfg'

    try:
        with open(data_file_name, 'w') as f:
            f.write(data)
        with open(cfg_file_name, 'w') as f:
            f.write(cfg)

        returncode, stdout, stderr = call_cmp_tool('-c %s -d %s' % (cfg_file_name, data_file_name))
        assert(stdout == CMP_START_STR_CMP +
               "Importing configuration file %s ... DONE\n" % (cfg_file_name) +
               "Importing data file %s ... FAILED\n" % (data_file_name))
        assert(stderr == "cmp_tool: wrong.data: Error read in 'W'. The data are not correct formatted.\n")
        assert(returncode == EXIT_FAILURE)
    finally:
        del_file(data_file_name)
        del_file(cfg_file_name)


def test_wrong_formart_cmp_fiel():
    cmp_data = 'wrong format! #comment'
    info = ("cmp_size = 32\n" + "golomb_par_used = 1\n" + "spill_used = 4\n"
            + "cmp_mode_used = 1\n" +"samples_used=5\n")
    cmp_file_name = 'wrong.cmp'
    info_file_name = 'wrong.info'

    try:
        with open(cmp_file_name, 'w') as f:
            f.write(cmp_data)
        with open(info_file_name, 'w') as f:
            f.write(info)

        returncode, stdout, stderr = call_cmp_tool('-i %s -d %s' % (info_file_name, cmp_file_name))
        assert(stdout == CMP_START_STR_DECMP +
               "Importing decompression information file %s ... DONE\n" % (info_file_name) +
               "Importing compressed data file %s ... FAILED\n" % (cmp_file_name))
        assert(stderr == "cmp_tool: wrong.cmp: Error read in 'w'. The data are not correct formatted.\n")
        assert(returncode == EXIT_FAILURE)
    finally:
        del_file(cmp_file_name)
        del_file(info_file_name)


def test_sample_used_is_to_big():
    cmp_data = '44444400'

    version_id = 0x8001_0042
    cmp_ent_size = IMAGETTE_HEADER_SIZE + len(cmp_data)//2
    original_size = 0xE # wrong original_size correct is 0xA

    start_time = cuc_timestamp(datetime.now(timezone.utc))
    end_time = cuc_timestamp(datetime.now(timezone.utc))

    data_type = 1
    cmp_mode_used = 2
    model_value_used = 8
    model_id = 0xBEEF
    model_counter = 0
    lossy_cmp_par_used = 0

    generic_header = build_generic_header(version_id, cmp_ent_size,
                        original_size, start_time, end_time, data_type,
                        cmp_mode_used, model_value_used, model_id,
                        model_counter, lossy_cmp_par_used)
    spill_used = 60
    golomb_par_used = 7

    ima_header = build_imagette_header(spill_used, golomb_par_used)
    cmp_data_header = generic_header + ima_header + cmp_data

    info = ("cmp_size = 20\n" + "golomb_par_used = 1\n" + "spill_used = 4\n"
            + "cmp_mode_used = 2\n" +"samples_used=5\n")
    #                                              ^ wrong samples_used
    cmp_file_name = 'big_cmp_size.cmp'
    info_file_name = 'big_cmp_size.info'
    pars = ['-i %s -d %s --no_header' % (info_file_name, cmp_file_name),
        '-d %s' % (cmp_file_name)]

    try:
        with open(info_file_name, 'w') as f:
            f.write(info)

        for par in pars:
            with open(cmp_file_name, 'w') as f:
                if '--no_header' in par:
                    f.write(cmp_data)
                else:
                    f.write(cmp_data_header)

            returncode, stdout, stderr = call_cmp_tool(par)

            if '--no_header' in par:
                assert(stdout == CMP_START_STR_DECMP +
                       "Importing decompression information file %s ... DONE\n" % (info_file_name) +
                       "Importing compressed data file %s ... DONE\n" % (cmp_file_name) +
                       "Decompress data ... FAILED\n")
            else:
                assert(stdout == CMP_START_STR_DECMP +
                       "Importing compressed data file %s ... DONE\n" % (cmp_file_name) +
                       "Decompress data ... FAILED\n")

            assert(stderr == "Error: The end of the compressed bit stream has been exceeded. Please check that the compression parameters match those used to compress the data and that the compressed data are not corrupted.\n")

            assert(returncode == EXIT_FAILURE)
    finally:
        del_file(cmp_file_name)
        del_file(info_file_name)


def test_cmp_entity_not_4_byte_aligned():
    cmp_data = '444444' # cmp data are not a multiple of 4

    version_id = 0x8001_0042
    cmp_ent_size = IMAGETTE_HEADER_SIZE + len(cmp_data)//2
    original_size = 0xC

    start_time = cuc_timestamp(datetime.now(timezone.utc))
    end_time = cuc_timestamp(datetime.now(timezone.utc))

    data_type = 1
    cmp_mode_used = 2
    model_value_used = 8
    model_id = 0xBEEF
    model_counter = 0
    lossy_cmp_par_used = 0

    generic_header = build_generic_header(version_id, cmp_ent_size,
                        original_size, start_time, end_time, data_type,
                        cmp_mode_used, model_value_used, model_id,
                        model_counter, lossy_cmp_par_used)
    spill_used = 60
    golomb_par_used = 7

    ima_header = build_imagette_header(spill_used, golomb_par_used)
    cmp_data_header = generic_header + ima_header + cmp_data

    data_exp = '00 01 00 02 00 03 00 04 00 05 00 06 \n'
    info = ("cmp_size = 20\n" + "golomb_par_used = 7\n" + "spill_used = 60\n"
            + "cmp_mode_used = 2\n" +"samples_used=6\n")

    cmp_file_name = 'unaligned_cmp_size.cmp'
    info_file_name = 'unaligned_cmp_size.info'
    output_prefix = '4_byte_aligned'
    pars = ['-i %s -d %s --no_header -o %s' % (info_file_name, cmp_file_name, output_prefix),
            '-d %s -o %s' % (cmp_file_name, output_prefix)]

    try:
        with open(info_file_name, 'w') as f:
            f.write(info)

        for par in pars:
            with open(cmp_file_name, 'w') as f:
                if '--no_header' in par:
                    f.write(cmp_data)
                else:
                    f.write(cmp_data_header)

            returncode, stdout, stderr = call_cmp_tool(par)

            assert(stderr == "")

            if '--no_header' in par:
                assert(stdout == CMP_START_STR_DECMP +
                       "Importing decompression information file %s ... DONE\n" % (info_file_name) +
                       "Importing compressed data file %s ... DONE\n" % (cmp_file_name) +
                       "Decompress data ... DONE\n" +
                       "Write decompressed data to file %s.dat ... DONE\n" % (output_prefix))
            else:
                assert(stdout == CMP_START_STR_DECMP +
                       "Importing compressed data file %s ... DONE\n" % (cmp_file_name) +
                       "Decompress data ... DONE\n"+
                       "Write decompressed data to file %s.dat ... DONE\n" % (output_prefix))

            assert(returncode == EXIT_SUCCESS)
            with open(output_prefix+".dat", encoding='utf-8') as f:
                assert(f.read() == data_exp)  # decompressed data == input data

    finally:
        del_file(cmp_file_name)
        del_file(info_file_name)
        del_file(output_prefix+".dat")


def test_header_wrong_formatted():
    cmp_file_name = 'read_wrong_format_header.cmp'

    # packet wrong formatted
    header = '80 07 00 00 !'
    try:
        with open(cmp_file_name, 'w') as f:
            f.write(header)
        returncode, stdout, stderr = call_cmp_tool('-d %s' % (cmp_file_name))
        assert(returncode == EXIT_FAILURE)
        assert(stdout == CMP_START_STR_DECMP +
               "Importing compressed data file %s ... FAILED\n" % (cmp_file_name))
        assert(stderr == "cmp_tool: %s: Error read in '!'. The data are not correct formatted.\n" % (cmp_file_name))

    finally:
        del_file(cmp_file_name)


def test_header_read_in():
    cmp_file_name = 'test_header_read_in.cmp'

    cmp_data = '444444'

    version_id = 0x8001_0042
    cmp_ent_size = IMAGETTE_HEADER_SIZE + len(cmp_data)//2
    original_size = 0xA

    start_time = cuc_timestamp(datetime.now(timezone.utc))
    end_time = cuc_timestamp(datetime.now(timezone.utc))

    data_type = 1
    cmp_mode_used = 2
    model_value_used = 8
    model_id = 0xBEEF
    model_counter = 0
    lossy_cmp_par_used = 0

    generic_header = build_generic_header(version_id, cmp_ent_size,
                        original_size, start_time, end_time, data_type,
                        cmp_mode_used, model_value_used, model_id,
                        model_counter, lossy_cmp_par_used)
    spill_used = 60
    golomb_par_used = 7

    ima_header = build_imagette_header(spill_used, golomb_par_used)
    cmp_ent = generic_header + ima_header + cmp_data
    # packet to small
    cmp_ent = cmp_ent[:-4]
    try:
        with open(cmp_file_name, 'w') as f:
            f.write(cmp_ent)
        returncode, stdout, stderr = call_cmp_tool('-d %s' % (cmp_file_name))
        assert(returncode == EXIT_FAILURE)
        assert(stdout == CMP_START_STR_DECMP +
               "Importing compressed data file %s ... FAILED\n" % (cmp_file_name))
        assert(stderr == "cmp_tool: %s: The size of the compression entity set in the "
               "header of the compression entity is not the same size as the read-in "
               "file has. Expected: 0x27, has 0x25.\n" %(cmp_file_name))

        # false data type
        data_type = 0x7FFE
        generic_header = build_generic_header(version_id, cmp_ent_size,
                        original_size, start_time, end_time, data_type,
                        cmp_mode_used, model_value_used, model_id,
                        model_counter, lossy_cmp_par_used)
        ima_header = build_imagette_header(spill_used, golomb_par_used)
        cmp_ent = generic_header + ima_header + cmp_data
        data_type = 1

        with open(cmp_file_name, 'w') as f:
            f.write(cmp_ent)
        returncode, stdout, stderr = call_cmp_tool('-d %s' % (cmp_file_name))
        assert(returncode == EXIT_FAILURE)
        assert(stdout == CMP_START_STR_DECMP +
               "Importing compressed data file %s ... FAILED\n" % (cmp_file_name))
        assert(stderr == "cmp_tool: %s: Error: Compression data type is not supported. The "
               "header of the compression entity may be corrupted.\n" % (cmp_file_name))

        # false cmp_mode_used
        cmp_mode_used = 0xFF
        generic_header = build_generic_header(version_id, cmp_ent_size,
                        original_size, start_time, end_time, data_type,
                        cmp_mode_used, model_value_used, model_id,
                        model_counter, lossy_cmp_par_used)
        ima_header = build_imagette_header(spill_used, golomb_par_used)
        cmp_ent = generic_header + ima_header + cmp_data
        cmp_mode_used = 2
        with open(cmp_file_name, 'w') as f:
            f.write(cmp_ent)
        returncode, stdout, stderr = call_cmp_tool('-d %s' % (cmp_file_name))
        assert(returncode == EXIT_FAILURE)
        assert(stdout == CMP_START_STR_DECMP +
               "Importing compressed data file %s ... DONE\n" % (cmp_file_name) +
               "Decompress data ... FAILED\n" )
        assert(stderr == "Error: The compression mode is not supported.\n")

    finally:
        del_file(cmp_file_name)


def test_model_fiel_erros():
    # generate test data
    data = '00 01 00 02 00 03 00 04 00 05 \n'
    model = '00 00 00 01 00 02 00 03 \n'
    cfg = ("golomb_par = 1\n" + "spill = 4\n" + "cmp_mode = 1\n" +
           "samples = 5\n" + "buffer_length = 5\n")
    data_file_name = 'data_model_err.dat'
    cfg_file_name = 'model_err.cfg'
    output_prefix = 'model_err_test'
    model_file_name = 'model.dat'
    github_win_action = os.environ.get('GITHUB_ACTIONS') == 'true'

    try:
        with open(data_file_name, 'w', encoding='utf-8') as f:
            f.write(data)
        with open(cfg_file_name, 'w', encoding='utf-8') as f:
            f.write(cfg)

        # no -m option in model mode
        add_args = [" --no_header", ""]
        for add_arg in add_args:
            returncode, stdout, stderr = call_cmp_tool(
                " -c "+cfg_file_name+" -d "+data_file_name + " -o "+output_prefix+add_arg)
            assert(returncode == EXIT_FAILURE)
            assert(stdout == CMP_START_STR_CMP +
                   "Importing configuration file %s ... DONE\n" % (cfg_file_name) +
                   "Importing data file %s ... DONE\n" % (data_file_name) +
                   "Importing model file  ... FAILED\n" )
            assert(stderr == "cmp_tool: No model file (-m option) specified.\n")

            # model file to small
            with open(model_file_name, 'w', encoding='utf-8') as f:
                f.write(model)
            returncode, stdout, stderr = call_cmp_tool(
                " -c "+cfg_file_name+" -d "+data_file_name + " -m "+model_file_name+" -o "+output_prefix)
            assert(returncode == EXIT_FAILURE)
            assert(stdout == CMP_START_STR_CMP +
                   "Importing configuration file %s ... DONE\n" % (cfg_file_name) +
                   "Importing data file %s ... DONE\n" % (data_file_name) +
                   "Importing model file %s ... FAILED\n" % (model_file_name) )
        assert(stderr == "cmp_tool: %s: Error: The files do not contain enough data. Expected: 0xa, has 0x8.\n" % (model_file_name))

        # updated model can not write
        with open(model_file_name, 'w', encoding='utf-8') as f:
            f.write(data)

        print(sys.platform)
        output_prefix = ("longlonglonglonglonglonglonglonglonglonglonglonglong"
                         "longlonglonglonglonglonglonglonglonglonglonglonglong"
                         "longlonglonglonglonglonglonglonglonglonglonglonglong"
                         "longlonglonglonglonglonglonglonglonglonglonglonglong"
                         "longlonglonglonglonglonglonglonglonglong")
        if (sys.platform == 'win32' and not github_win_action) or sys.platform == 'cygwin':
          output_prefix = ("longlonglonglonglonglonglonglonglonglonglonglonglong"
                           "longlonglonglonglonglonglonglonglonglonglonglonglong"
                           "longlonglonglonglonglonglonglonglonglonglonglonglong"
                           "longlonglonglonglonglonglonglonglonglonglong")
        returncode, stdout, stderr = call_cmp_tool(
            " -c "+cfg_file_name+" -d "+data_file_name + " -m "+model_file_name+" -o "+output_prefix)
        assert(returncode == EXIT_FAILURE)
        print(stdout)
        assert(stdout == CMP_START_STR_CMP +
               "Importing configuration file %s ... DONE\n" % (cfg_file_name) +
               "Importing data file %s ... DONE\n" % (data_file_name) +
               "Importing model file %s ... DONE\n" % (model_file_name) +
               "Compress data ... DONE\n" +
               "Write compressed data to file %s.cmp ... DONE\n" %(output_prefix) +
               "Write updated model to file %s_upmodel.dat ... FAILED\n" %(output_prefix))
        if sys.platform == 'win32' or sys.platform == 'cygwin' or WINE_TEST_SETUP:
          assert(stderr == "cmp_tool: %s_upmodel.dat: No such file or directory\n" % (output_prefix))
        else:
          assert(stderr == "cmp_tool: %s_upmodel.dat: File name too long\n" % (output_prefix))

    finally:
        del_file(data_file_name)
        del_file(cfg_file_name)
        del_file(model_file_name)
        del_file(output_prefix+".cmp")
        del_file(output_prefix+"_upmodel.dat")


def test_rdcu_pkt():
    # generate test data
    data = '00 01 00 02 00 03 00 04 00 05 \n'
    model = '00 00 00 01 00 02 00 03 00 04 \n'
    data_file_name = 'data_pkt.dat'
    model_file_name = 'model_pkt.dat'
    cfg_file_name = 'pkt.cfg'
    output_prefix1 = 'pkt_cmp'
    output_prefix2 = 'pkt_decmp'

    try:
        # generate test configuration
        with open(cfg_file_name, 'w', encoding='utf-8') as f:
            returncode, stdout, stderr = call_cmp_tool("--model_cfg --rdcu_par")
            assert(stderr == "")
            assert(returncode == EXIT_SUCCESS)
            f.write(stdout)
            cfg = parse_key_value(stdout)
        with open(data_file_name, 'w', encoding='utf-8') as f:
            f.write(data)
        with open(model_file_name, 'w', encoding='utf-8') as f:
            f.write(model)

        add_args = [" --rdcu_pkt", " --last_info "+output_prefix1+".info"]
        for add_arg in add_args:
            # compression
            returncode, stdout, stderr = call_cmp_tool(
                " -c "+cfg_file_name+" -d "+data_file_name+" -m "+model_file_name+" -o " +output_prefix1+add_arg)
            assert(stderr == "")
            assert(stdout == CMP_START_STR_CMP +
                   "Importing configuration file %s ... DONE\n" % (cfg_file_name) +
                   "Importing data file %s ... \n" % (data_file_name) +
                   "No samples parameter set. Use samples = 5.\n" +
                   "... DONE\n" +
                   "Importing model file %s ... DONE\n" % (model_file_name) +
                   "No buffer_length parameter set. Use buffer_length = 12 as compression buffer size.\n" +
                   "Generate compression setup packets ...\n" +
                   "Use ICU_ADDR = 0XA7, RDCU_ADDR = 0XFE and MTU = 4224 for the RAMP packets.\n"
                   "... DONE\n" +
                   "Compress data ... DONE\n" +
                   "Generate the read results packets ... DONE\n"
                   "Write compressed data to file %s.cmp ... DONE\n" % (output_prefix1) +
                   "Write decompression information to file %s.info ... DONE\n" % (output_prefix1) +
                   "Write updated model to file %s_upmodel.dat ... DONE\n" % (output_prefix1)
            )
            assert(returncode == EXIT_SUCCESS)

            # check compressed data
            with open(output_prefix1+".cmp", encoding='utf-8') as f:
                assert(f.read() == "49 24 \n")
            # check info file
            with open(output_prefix1+".info", encoding='utf-8') as f:
                info = parse_key_value(f.read())
            assert(info['cmp_mode_used'] == '3')
            assert(info['model_value_used'] == cfg['model_value'])
            assert(info['round_used'] == cfg['round'])
            assert(info['spill_used'] == cfg['spill'])
            assert(info['golomb_par_used'] == cfg['golomb_par'])
            assert(info['samples_used'] == '5')
            assert(info['cmp_size'] == '15')
            assert(info['cmp_err'] == '0')
            assert(info['ap1_cmp_size'] == '15')
            assert(info['ap2_cmp_size'] == '15')
            assert(info['rdcu_new_model_adr_used'] == cfg['rdcu_new_model_adr'])
            assert(info['rdcu_cmp_adr_used'] == cfg['rdcu_buffer_adr'])

            # decompression
            returncode, stdout, stderr = call_cmp_tool(
                " -i "+output_prefix1+".info -d "+output_prefix1+".cmp -m "+model_file_name+" -o "+output_prefix2)
            assert(stderr == "")
            assert(stdout == CMP_START_STR_DECMP +
                   "Importing decompression information file %s.info ... DONE\n" % (output_prefix1) +
                   "Importing compressed data file %s.cmp ... DONE\n" % (output_prefix1) +
                   "Importing model file %s ... DONE\n" % (model_file_name) +
                   "Decompress data ... DONE\n" +
                   "Write decompressed data to file %s.dat ... DONE\n" % (output_prefix2) +
                   "Write updated model to file %s_upmodel.dat ... DONE\n" % (output_prefix2))
            assert(returncode == EXIT_SUCCESS)
            with open(output_prefix2+".dat", encoding='utf-8') as f:
                 assert(f.read() == data)
            with open(output_prefix1+"_upmodel.dat", encoding='utf-8') as f1:
                 with open(output_prefix2+"_upmodel.dat", encoding='utf-8') as f2:
                     assert(f1.read() == f2.read())

    finally:
        pass
        del_directory('TC_FILES')
        del_file(data_file_name)
        del_file(cfg_file_name)
        del_file(model_file_name)
        del_file(output_prefix1+'.cmp')
        del_file(output_prefix1+'.info')
        del_file(output_prefix1+'_upmodel.dat')
        del_file(output_prefix2+'.dat')
        del_file(output_prefix2+'_upmodel.dat')



# TODO: random test
