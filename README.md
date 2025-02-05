# PLATO Compression/Decompression Tool

If you find a bug or have a feature request please file an [issue](https://gitlab.phaidra.org/loidoltd15/cmp_tool/-/issues) or send
me an [email](mailto:dominik.loidolt@univie.ac.at?subject=%5BIssue%5D%20cmp_tool).  
Compiled executables can be found [here](https://gitlab.phaidra.org/loidoltd15/cmp_tool/-/releases). The building instructions can be found [here](INSTALL.md).

## Usage

**usage:** `cmp_tool [options] [<argument>]`

### General Options

| Options             | Description                                                                   |
|:--------------------|:------------------------------------------------------------------------------|
| `-h, --help`        | Print some help text and exit                                                 |
| `-o <prefix>`       | Use the `<prefix>` for output files<sup>[1](#fnote1)</sup>                    |
| `-n, --model_cfg`   | Print a default model configuration and exit<sup>[2](#fnote2)</sup>           |
| `--diff_cfg`        | Print a default 1d-differencing configuration and exit<sup>[2](#fnote2)</sup> |
| `-b, --binary`      | Read and write files in binary format                                         |
| `-a, --rdcu_par`    | Add additional RDCU control parameters                                        |
| `-V, --version`     | Print program version and exit                                                |
| `-v, -vv, --verbose`| Print various debugging information, -vv is extra verbose                     |


<a name="fnote1">1</a>) **NOTE:** If the -o option is not used the `<prefix>`
will be set to "OUTPUT".  
<a name="fnote2">2</a>) **NOTE:** In the default configurations the **samples**
and **buffer_length** parameter is set to **0**!

### Compression Options

| Options                     | Description                                                                          |
|:----------------------------|:-------------------------------------------------------------------------------------|
| `-c <file>`                 | File containing the compressing configuration                                        |
| `-d <file>`                 | File containing the data to be compressed                                            |
| `-m <file>`                 | File containing the model of the data to be compressed                               |
| `--no_header`               | Do not add a compression entity header in front of the compressed data               |
| `--rdcu_pkt`                | Generate RMAP packets for an RDCU compression<sup>[3](#fnote3)</sup>                 |
| `--last_info  <.info file>` | Generate RMAP packets for an RDCU compression with parallel read of the last results |

<a name="fnote3">3</a>) **NOTE:** When using the `--rdcu_pkt` option the
configuration of the RMAP parameters can be found in the `.rdcu_pkt_mode_cfg file`.
The generated packets can be found in the `TC_FILES` directory.

### Decompression Options

| Options     | Description                                                                     |
|:----------- |:--------------------------------------------------------------------------------|
| `-d <file>` | File containing the compressed data                                             |
| `-m <file>` | File containing the model of the compressed data                                |
| `-i <file>` | File containing the decompression information (required if --no_header was used)|

### Guessing Options
| Options                 | Description                                                            |
|:------------------------|:-----------------------------------------------------------------------|
| `--guess <rdcu|chunk>`  | Search for a good configuration for compression RDCU or chunk data set |
| `-d <file>`             | File containing the data to be compressed                              |
| `-m <file>`             | File containing the model of the data to be compressed                 |
| `--guess_level <level>` | Set guess level to \<level\> (optional)<sup>[4](#fnote4)</sup>         |

<a name="fnote4">4</a>) **Supported levels:** 

| guess level | Description                     |
|:------------|:--------------------------------|
| `1`         | fast mode                       |
| `2`         | default mode                    |
| `3`         | slow mode (better results)      |

Lower values increase step size (coarser search), while higher values decrease step size (finer search).

#### Examples of Compression Parameter guessing:

```bash
# RDCU data compression guessing
./cmp_tool --guess rdcu -d test_data/test_data1.dat -o rdcu_guess

# Chunk mode guessing
./cmp_tool --guess chunk -d chunk_data.dat -o chunk_guess

# Custom guess level with model
./cmp_tool --guess chunk -d chunk_data.dat -m chunk_model.dat --guess_level 3 -o chunk_guess_3
```

These commands create `.cfg` files for RDCU compression parameters. For chunk mode, a `.par` file is created containing all parameters for chunk compression.

### Data Format
The input data is formatted as hexadecimal numbers.
For example: `12 AB 34 CD` or `12AB34CD`.

### User Manual
You can find the user manual [here](doc).

## How to use the tool

A simple example to show how the compression tool works.
Instructions on how to perform compression without headers can be found [here](how_to_no_header.md).

0. Download the [tool](https://gitlab.phaidra.org/loidoltd15/cmp_tool/-/releases) or [build the tool](INSTALL.md) yourself

1. Create a configuration file
* Create a cfg directory  
    `mkdir cfg`
* To create a default 1d-differencing mode configuration:  
    `./cmp_tool --diff_cfg > cfg/default_config_1d.cfg`
* To create a default model mode configuration:  
    `./cmp_tool --model_cfg > cfg/default_config_model.cfg`

2. Compress data with the default configurations:
* Create a directory for the compressed data  
    `mkdir compressed`
* 1d-differencing mode compression  
   `./cmp_tool -c cfg/default_config_1d.cfg -d test_data/test_data1.dat -o compressed/data1`  
    This creates a files in the compressed directory:  
    `data1.cmp`        -> compressed `test_data1.dat` file  
* Model mode compression  
   `./cmp_tool -c cfg/default_config_model.cfg -d test_data/test_data2.dat -m test_data/test_data1.dat -o compressed/data2`  
    We use `test_data1.dat` as the first model for `test_data2.dat`.  
    Creates these two files in the compressed directory:  
    `data2.cmp `        -> compressed `test_data3.dat` file  
    `data2_upmodel.dat` -> updated model used to **compress** the next data in model mode

3.  Decompress the data  
* Create a directory for the decompressed data  
    `mkdir decompressed` 
* Decompress `data1.cmp`  
    `./cmp_tool -d compressed/data1.cmp -o decompressed/test_data1`  
    This creates a file in the decompressed directory:  
    `test_data1.dat`  -> decompressed `data1.cmp` file
* Decompress `data2.cmp`  
    `./cmp_tool -d compressed/data2.cmp -m decompressed/test_data1.dat -o decompressed/test_data2`  
    As for the compression we use `test_data1.dat` as our model for decompression.  
    Creates these two files in the decompressed directory:  
    `test_data2.dat`   -> decompressed `data2.cmp` file  
    `data2_upmodel.dat` -> updated model used to **decompress** the next data in model mode

4. Bonus: Check if the decompressed data are equal to the original data  
    The moment of truth!  
    `diff test_data/test_data1.dat decompressed/test_data1.dat`  
    `diff test_data/test_data2.dat decompressed/test_data2.dat`  

    And also check if the updated model is the same  
    `diff compressed/data2_upmodel.dat decompressed/test_data2_upmodel.dat`
