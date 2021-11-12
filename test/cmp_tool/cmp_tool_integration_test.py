import subprocess
import shlex
import sys
import os

PATH_CMP_TOOL = "./cmp_tool"
VERSION = "0.06"

EXIT_FAILURE = 1
EXIT_SUCCESS = 0


def call_cmp_tool(args):
    args = shlex.split(PATH_CMP_TOOL + " " + args)

    try:
        with subprocess.Popen(args, stdout=subprocess.PIPE, stderr=subprocess.PIPE, universal_newlines=True) as proc:
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


HELP_STRING = \
"""usage: %s [options] [<argument>]
General Options:
  -h, --help               Print this help text and exit
  -V, --version            Print program version and exit
  -v, --verbose            Print various debugging information
  -n, --model_cfg          Print a default model configuration and exit
  --diff_cfg               Print a default 1d-differencing configuration and exit
  -a, --rdcu_par           Add additional RDCU control parameters
  -o <prefix>              Use the <prefix> for output files
Compression Options:
  -c <file>                File containing the compressing configuration
  -d <file>                File containing the data to be compressed
  -m <file>                File containing the model of the data to be compressed
  --rdcu_pkt               Generate RMAP packets for an RDCU compression
  --last_info <.info file> Generate RMAP packets for an RDCU compression with parallel read of the last results
Decompression Options:
  -i <file>                File containing the decompression information
  -d <file>                File containing the compressed data
  -m <file>                File containing the model of the compressed data
Guessing Options:
  --guess <mode>           Search for a good configuration for compression <mode>
  -d <file>                File containing the data to be compressed
  -m <file>                File containing the model of the data to be compressed
  --guess_level <level>    Set guess level to <level> (optional)
""" % (PATH_CMP_TOOL)


CMP_START_STR = \
"""#########################################################
### PLATO Compression/Decompression Tool Version %s ###
#########################################################

""" % (VERSION)


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
            assert(stderr == "cmp_tool: invalid option -- q\n")
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
        assert(returncode == EXIT_SUCCESS)
        assert(stderr == "")

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
    returncode, stdout, stderr = call_cmp_tool("-d no_data.data")
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
    assert(stdout == CMP_START_STR +
           "Importing configuration file no_cfg.cfg ... FAILED\n")
    assert(stderr == "cmp_tool: no_cfg.cfg: No such file or directory\n")


def test_compression_diff():
    # generate test data
    data = '00 01 00 02 00 03 00 04 00 05 \n'
    data_file_name = 'data.dat'
    cfg_file_name = 'diff.cfg'
    output_prefix = 'diff_cmp'
    try:
        with open(data_file_name, 'w') as f:
            f.write(data)

        # generate test configuration
        with open(cfg_file_name, 'w') as f:
            returncode, stdout, stderr = call_cmp_tool("--diff_cfg")
            assert(returncode == EXIT_SUCCESS)
            assert(stderr == "")
            f.write(stdout)

        # compression
        returncode, stdout, stderr = call_cmp_tool(
            " -c "+cfg_file_name+" -d "+data_file_name + " -o "+output_prefix)

        # check compression results
        assert(returncode == EXIT_SUCCESS)
        assert(stderr == "")
        assert(stdout == CMP_START_STR +
               "Importing configuration file %s ... DONE\n" % (cfg_file_name) +
               "Importing data file %s ... \n" % (data_file_name) +
               "No samples parameter set. Use samples = 5.\n" +
               "... DONE\n" +
               "No buffer_length parameter set. Use buffer_length = 12 as compression buffer size.\n" +
               "Compress data ... DONE\n" +
               "Write compressed data to file %s.cmp ... DONE\n" % (output_prefix) +
               "Write decompression information to file %s.info ... DONE\n" % (output_prefix))
        # check compressed data
        with open(output_prefix+".cmp") as f:
            assert(f.read() == "44 44 40 00 \n")
        # check info file
        with open(output_prefix+".info") as f:
            info = parse_key_value(f.read())
        assert(info['cmp_mode_used'] == '2')
        # assert(info['model_value_used'] == '8')  # not needed for diff
        assert(info['round_used'] == '0')
        assert(info['spill_used'] == '60')
        assert(info['golomb_par_used'] == '7')
        assert(info['samples_used'] == '5')
        assert(info['cmp_size'] == '20')
        assert(info['cmp_err'] == '0')

        # decompression
        returncode, stdout, stderr = call_cmp_tool(
            " -i "+output_prefix+".info -d "+output_prefix+".cmp -o "+output_prefix)

        # check decompression
        assert(returncode == EXIT_SUCCESS)
        assert(stdout == CMP_START_STR +
               "Importing decompression information file %s.info ... DONE\n" % (output_prefix) +
               "Importing compressed data file %s.cmp ... DONE\n" % (output_prefix) +
               "Decompress data ... DONE\n" +
               "Write decompressed data to file %s.dat ... DONE\n" % (output_prefix))
        assert(stderr == "")
        with open(output_prefix+".dat") as f:
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
    data_file_name = 'data.dat'
    model_file_name = 'model.dat'
    cfg_file_name = 'model.cfg'
    output_prefix1 = 'model_cmp'
    output_prefix2 = 'model_decmp'
    try:
        with open(data_file_name, 'w') as f:
            f.write(data)
        with open(model_file_name, 'w') as f:
            f.write(model)

        # generate test configuration
        with open(cfg_file_name, 'w') as f:
            returncode, stdout, stderr = call_cmp_tool("--model_cfg")
            assert(returncode == EXIT_SUCCESS)
            assert(stderr == "")
            cfg = parse_key_value(stdout)
            cfg['cmp_mode'] = 'MODE_MODEL_MULTI'
            cfg["samples"] = '5'
            cfg["buffer_length"] = '2'
            for key, value in cfg.items():
                f.write(key + ' = ' + str(value) + '\n')

        # compression
        returncode, stdout, stderr = call_cmp_tool(
            " -c "+cfg_file_name+" -d "+data_file_name + " -m "+model_file_name+" -o "+output_prefix1)

        # check compression results
        assert(returncode == EXIT_SUCCESS)
        assert(stderr == "")
        assert(stdout == CMP_START_STR +
               "Importing configuration file %s ... DONE\n" % (cfg_file_name) +
               "Importing data file %s ... DONE\n" % (data_file_name) +
               "Importing model file %s ... DONE\n" % (model_file_name) +
               "Compress data ... DONE\n" +
               "Write compressed data to file %s.cmp ... DONE\n" % (output_prefix1) +
               "Write decompression information to file %s.info ... DONE\n" % (output_prefix1) +
               "Write updated model to file %s_upmodel.dat ... DONE\n" % (output_prefix1))
        # check compressed data
        with open(output_prefix1+".cmp") as f:
            assert(f.read() == "49 24 00 00 \n")
        # check info file
        with open(output_prefix1+".info") as f:
            info = parse_key_value(f.read())
        assert(info['cmp_mode_used'] == '3')
        assert(info['model_value_used'] == cfg['model_value'])
        assert(info['round_used'] == cfg['round'])
        assert(info['spill_used'] == cfg['spill'])
        assert(info['golomb_par_used'] == cfg['golomb_par'])
        assert(info['samples_used'] == cfg['samples'])
        assert(info['cmp_size'] == '15')
        assert(info['cmp_err'] == '0')

        # decompression
        returncode, stdout, stderr = call_cmp_tool(
            " -i "+output_prefix1+".info -d "+output_prefix1+".cmp -m "+model_file_name+" -o "+output_prefix2)
        assert(returncode == EXIT_SUCCESS)
        assert(stdout == CMP_START_STR +
               "Importing decompression information file %s.info ... DONE\n" % (output_prefix1) +
               "Importing compressed data file %s.cmp ... DONE\n" % (output_prefix1) +
               "Importing model file %s ... DONE\n" % (model_file_name) +
               "Decompress data ... DONE\n" +
               "Write decompressed data to file %s.dat ... DONE\n" % (output_prefix2) +
               "Write updated model to file %s_upmodel.dat ... DONE\n" % (output_prefix2))
        assert(stderr == "")
        # check compressed data

        with open(output_prefix2+".dat") as f:
            assert(f.read() == data)

        with open(output_prefix1+"_upmodel.dat") as f1:
            with open(output_prefix2+"_upmodel.dat") as f2:
                assert(f1.read() == f2.read() ==
                       '00 00 00 01 00 02 00 03 00 04 \n')
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


def test_guess_option():
    # generate test data
    data = '00 01 00 01 00 01 00 01 00 01 \n'
    model = '00 00 00 00 00 00 00 00 00 00 \n'
    data_file_name = 'data.dat'
    model_file_name = 'model.dat'
    args = [
        ['guess_RDCU_diff',  '--guess RDCU -d %s -o guess --rdcu_par' %
            (data_file_name)],
        ['guess_RDCU_model', '--guess RDCU -d %s -m %s -o guess --rdcu_par' %
            (data_file_name, model_file_name)],
        ['guess_level_3',    '--guess MODE_DIFF_MULTI --guess_level 3 -d %s -o guess' %
            (data_file_name)],
        ['cfg_folder_not_exist', '--guess RDCU -d ' +
            data_file_name+' -o not_exist/guess'],
        ['guess_level_10',    '--guess MODE_DIFF_MULTI --guess_level 10 -d %s -o guess' %
            (data_file_name)],
        ['guess_unknown_mode', '--guess MODE_UNKNOWN -d %s -o guess' %
            (data_file_name)],
        ['guess_model_mode_no_model ', '--guess 1 -d %s -o guess' %
            (data_file_name)],
    ]
    try:
        with open(data_file_name, 'w') as f:
            f.write(data)
        with open(model_file_name, 'w') as f:
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
                        'Importing model file model.dat ... DONE\n', '2', '', '5.33')
                elif sub_test == 'guess_level_3':
                    exp_out = (
                        '', '3', ' 0%... 6%... 13%... 19%... 25%... 32%... 38%... 44%... 50%... 57%... 64%... 72%... 80%... 88%... 94%... 100%', '11.43')
                else:
                    exp_out = ('', '', '')

                assert(stdout == CMP_START_STR +
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
                assert(stdout == CMP_START_STR +
                       "Importing data file %s ... \n" % (data_file_name) +
                       "No samples parameter set. Use samples = 5.\n" +
                       "... DONE\n"
                       "Search for a good set of compression parameters (level: 2) ... DONE\n" +
                       "Write the guessed compression configuration to file not_exist/guess.cfg ... FAILED\n")
            elif sub_test == 'guess_level_10':
                assert(stderr == "cmp_tool: guess level not supported!\n")
                assert(returncode == EXIT_FAILURE)
                assert(stdout == CMP_START_STR +
                       "Importing data file %s ... \n" % (data_file_name) +
                       "No samples parameter set. Use samples = 5.\n" +
                       "... DONE\n"
                       "Search for a good set of compression parameters (level: 10) ... FAILED\n")
            elif sub_test == 'guess_unknown_mode':
                assert(
                    stderr == "cmp_tool: Error: unknown compression mode: MODE_UNKNOWN\n")
                assert(returncode == EXIT_FAILURE)
                assert(stdout == CMP_START_STR +
                       "Importing data file %s ... \n" % (data_file_name) +
                       "No samples parameter set. Use samples = 5.\n" +
                       "... DONE\n"
                       "Search for a good set of compression parameters (level: 2) ... FAILED\n")
            else:
                assert(
                    stderr == "cmp_tool: Error: model mode needs model data (-m option)\n")
                assert(returncode == EXIT_FAILURE)
                assert(stdout == CMP_START_STR +
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
        assert(stdout == CMP_START_STR +
               "Importing configuration file %s ... DONE\n" % (cfg_file_name) +
               "Importing data file %s ... DONE\n" % (data_file_name) +
               "Compress data ... \n"
               "Compression error 0x01\n"
               "... FAILED\n")
        assert(stderr == "cmp_tool: the buffer for the compressed data is too small. Try a larger buffer_length parameter.\n")
        assert(returncode == EXIT_FAILURE)

    finally:
        del_file(data_file_name)
        del_file(cfg_file_name)
# random test
