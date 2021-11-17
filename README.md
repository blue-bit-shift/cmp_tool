# PLATO Compression/Decompression Tool

If you find a bug or have a feature request please file an [issue][1] or send
me an [email][2].  
Compiled executables can be found [here][3].

## Usage

**usage:** `cmp_tool [options] [<argument>]`

### General Options

| Options           | Description                                                                   |
|:------------------|:------------------------------------------------------------------------------|
| `-h, --help`      | Print some help text and exit                                                 |
| `-V, --version`   | Print program version and exit                                                |
| `-v, --verbose`   | Print various debugging information                                           |
| `-n, --model_cfg` | Print a default model configuration and exit<sup>[1](#fnote1)</sup>           |
| `--diff_cfg`      | Print a default 1d-differencing configuration and exit<sup>[1](#fnote1)</sup> |
| `-a, --rdcu_par`  | Add additional RDCU control parameters                                        |
| `-o <prefix>`     | Use the `<prefix>` for output files<sup>[2](#fnote2)</sup>                    |

<a name="fnote1">1</a>) **NOTE:** In the default configurations the **samples**
and **buffer_length** parameter is set to **0**!  
<a name="fnote2">2</a>) **NOTE:** If the -o option is not used the `<prefix>`
will be set to "OUTPUT".

### Compression Options

| Options                     | Description                                                                          |
|:----------------------------|:-------------------------------------------------------------------------------------|
| `-c <file>`                 | File containing the compressing configuration                                        |
| `-d <file>`                 | File containing the data to be compressed                                            |
| `-m <file>`                 | File containing the model of the data to be compressed                               |
| `--rdcu_pkt`                | Generate RMAP packets for an RDCU compression<sup>[3](#fnoot3)</sup>                 |
| `--last_info  <.info file>` | Generate RMAP packets for an RDCU compression with parallel read of the last results |

<a name="foot3">3</a>) **NOTE:** When using the `--rdcu_pkt` option the
configuration of the RMAP parameters can be found in the `.rdcu_pkt_mode_cfg file`.
The generated packets can be found in the `TC_FILES` directory.

### Decompression Options

| Options     | Description                                      |
|:----------- |:-------------------------------------------------|
| `-i <file>` | File containing the decompression information    |
| `-d <file>` | File containing the compressed data              |
| `-m <file>` | File containing the model of the compressed data |

### Guessing Options

| Options                 | Description                                                                     |
|:------------------------|:--------------------------------------------------------------------------------|
| `--guess <mode>`        | Search for a good configuration for compression \<mode\><sup>[4](#fnoot4)</sup> |
| `-d <file>`             | File containing the data to be compressed                                       |
| `-m <file>`             | File containing the model of the data to be compressed                          |
| `--guess_level <level>` | Set guess level to \<level\> (optional)<sup>[5](#fnoot5)</sup>                  |

<a name="fnoot4">4</a>) **NOTE:** \<mode\> can be either the compression mode
number or the keyword: `RDCU`. The RDCU mode automatically selects the correct
RDCU-compatible compression mode depending on if the Model (-m) option is set.  
<a name="fnoot5">5</a>) **Supported levels:** 

| guess level | Description                     |
|:------------|:--------------------------------|
| `1`         | fast mode (not implemented yet) |
| `2`         | default mode                    |
| `3`         | brute force                     |

**Example of Compression Parmeter guessing:**

``./cmp_tool --guess RDCU -d test_data/test_data1.dat -o myguess``

This command creates the file `myguess.cfg` with the guessed compression parameters.

### Data Format

The input data must be formatted as two hex numbers separated by a space.
For example: `12 AB 34 CD`.

## How to use the tool

A simple example to show how the compression tool works.

0. Download the [tool][3] or run `make` to build the tool

1. Create a configuration file
* Create a cfg directory  
    `mkdir cfg`
* To create a default 1d-differencing mode configuration:  
    `./cmp_tool --diff_cfg > cfg/default_config_1d.cfg`
* To create a default model mode configuration:  
    `./cmp_tool --model_cfg > cfg/default_config_model.cfg`
* Change the the **`samples`** and **`buffer_length`** parameters from `0` to `50`
in the `default_config_1d.cfg` and `default_config_model.cfg` files

2. Compress data with the default configurations:
* Create a directory for the compressed data  
    `mkdir compressed`
* 1d-differencing mode compression  
   `./cmp_tool -c cfg/default_config_1d.cfg -d test_data/test_data1.dat -o compressed/data1`  
    This creates these two files in the compressed directory:  
    `data1.cmp`        -> compressed `test_data1.dat` file  
    `data1.info`       -> decompression information for `data1.cmp`
* Model mode compression  
   `./cmp_tool -c cfg/default_config_model.cfg -d test_data/test_data2.dat -m test_data/test_data1.dat -o compressed/data2`  
    We use `test_data1.dat` as the first model for `test_data2.dat`.

    Creates these three files in the compressed directory:  
    `data2.cmp `        -> compressed `test_data3.dat` file  
    `data2.info`        -> decompression information for `data2.cmp`  
    `data2_upmodel.dat` -> updated model used to **compress** the next data in model mode

3.  Decompress the data  
* Create a directory for the decompressed data  
    `mkdir decompressed` 
* Decompress `data1.cmp`  
    `./cmp_tool -i compressed/data1.info -d compressed/data1.cmp -o decompressed/test_data1`  
    Creates this file in the decompressed directory:  
    `test_data1.dat `  -> decompressed `data1.cmp` file
* Decompress `data2.cmp`  
    `./cmp_tool -i compressed/data2.info -d compressed/data2.cmp -m decompressed/test_data1.dat -o decompressed/test_data2`  
    As for the compression we use `test_data1.dat` as our model for decompression.  

    Creates these two files in the decompressed directory:  
    `test_data2.dat `   -> decompressed `data2.cmp` file  
    `data2_upmodel.dat` -> updated model used to **decompress** the next data in model mode

4. Bonus: Check if the decompressed data are equal to the original data  
    The moment of truth!  
    `diff test_data/test_data1.dat decompressed/test_data1.dat`  
    `diff test_data/test_data2.dat decompressed/test_data2.dat`  

    And also check if the updated model is the same  
    `diff compressed/data2_upmodel.dat decompressed/test_data2_upmodel.dat`

[1]: <https://gitlab.phaidra.org/loidoltd15/cmp_tool/-/issues> "issues"
[2]: <mailto:dominik.loidolt@univie.ac.at?subject=%5BIssue%5D%20Cmd_Tool> "email"
[3]: <https://gitlab.phaidra.org/loidoltd15/cmp_tool/-/releases> "release"