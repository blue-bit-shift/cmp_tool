## How to use the tool without header

A simple example to show how the compression tool works without the compression entity header.

0. Download the [cmp_tool](https://gitlab.phaidra.org/loidoltd15/cmp_tool/-/releases) or [build](INSTALL.md) the tool

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
   `./cmp_tool -c cfg/default_config_1d.cfg -d test_data/test_data1.dat --no_header -o compressed/data1`  
    This creates these two files in the compressed directory:  
    `data1.cmp`        -> compressed `test_data1.dat` file  
    `data1.info`       -> decompression information for `data1.cmp`
* Model mode compression  
   `./cmp_tool -c cfg/default_config_model.cfg -d test_data/test_data2.dat -m test_data/test_data1.dat --no_header -o compressed/data2`  
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