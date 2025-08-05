# Changelog
All notable changes to this project will be documented in this file.

## [1.0] - 18-08-2025
### Fixed
- smearing: Change `variance_mean` field to `uint32_t` and update tests
- cmp_tool: improve error handling—empty files now raise Error instead of Warning
- fuzz: fix file handling assertions
- docs: Correct doxygen generation issues

### Added
- cmp_tool: Add samples and buffer length validation

### Changed
- docs: Compression User Manual updated to Issue 1 Revision 1
- build: allow fuzz target builds if corpus download fails
- fuzz: redirect cmp_tool output to /dev/null

## [0.15] - 22-01-2025
### Added
- add fuzzer for cmp_tool
### Changed
- error handling is now consistent with the --binary option when model file size does not match original data size

## [0.14] - 16-01-2025
### Added
- check for model file size mismatch errors

## [0.13] - 08-11-2024
### Added
- added chunk-specific compression parameter guessing functionality
 - added chunk compression parameter file I/O functionality
- added fuzz target for decompression
- added Github action to compile with Werror and run tests
- added unit tests to improve code coverage
### Changed
- improved spillover threshold calculation logic for chunk compression
- downgraded reserved field check from error to warning in decompression
- added const qualifiers for (de)compression parameters
- renamed CMP_ERROR_SMALL_BUF_ to CMP_ERROR_SMALL_BUFFER
- replaced dynamic max_used_bits struct with constant definition
- separated RDCU configuration into its own struct
- consolidated l_fx_variance and l_cob_variance into single l_fx_cob_variance field
- changed return type of compress_like_rdcu from int32_t to uint32_t
- updated Doxygen configuration and comments
 - updated doxygen-awesome-css submodule to latest version
 - fixed spelling errors and grammar in documentation and messages
### Fixed
- corrected size calculation when `dst` is NULL in `compress_chunk()`
- fix be24_to_cpu function for big-endian systems
- fix collection size validation in decompression header
### Removed
- remove deprecated ICU compression interface
- remove (de)compression from fast cadence data products

## [0.12] - 2024-05-02
### Added
- added chunk (de)compression
- added test case for chunk (de)compression
- added -vv flag for extra verbose output
- fuzzing
 - added fuzzing targets configuration in meson build system
 - added GitHub Actions workflow for ClusterFuzzLite integration
- added compression speed test bench
 - added wrapper to execute programmes on a LEON setup
- added examples for software and hardware compression
- updated unit tests
- added compression and decompression for DATA_TYPE_F_CAM_OFFSET and DATA_TYPE_F_CAM_BACKGROUND
- documentation was improved with additional comments and doxygen strings
### Changed
- enhanced error handling in chunk compression functions
 - compression function now returns error code upon failure
 - Added error code checking with cmp_get_error_code()
 - added descriptive strings for each error code
- updated debug_print() for better environment adaptability
- adapted to C standard GNU89 with GCC extensions
- restructured file layout:
 - split lib folder into common, decompress, icu_compress rdcu_compress
 - moved cmp_tool files into programs directory
### Fixed
- Fixed cross-compilation to Windows on Ubuntu/Debian
- Fixed cmp_tool chunk compression entity header
- Fixed model update for NULL dst and worst case model mode
- Fixed unaligned access when compressing imagettes
- Fixed sparc compiler warnings and compatibility issues
- Fixed compiler warning for datetime deprecation
### Removed
- removed unused CMP_MODE_STUFF compression mode
- removed unused files and code

## [0.11] - 2023-04-26
### Added
- add -b or --binary option for read and write files in binary format
- add cross compile file for sparc
- add tests for the compression entity
- add test for cmp_rdcu_cfg.c
- add sparc inttypes.h definitions
### Changed
- refactor function and constants names
- refactor configure check functions
- move the rdcu setup functions to cmp_rdcu_cfg.c
- update Documentation
- update Doxygen setup
### Fixed
- remove memory leaks from tests
- fixed incorrect error message when using rdcu_pkt option without .rdcu_pkt_mode_cfg file
- set the rdcu_par option when using the rdcu_pkt option
- fixed several bug when using the last_info option
- fix a bug in the calculation of the adaptive compression sizes

## [0.09] - 2022-09-30
### Added
- decompression/compression for non-imagette data
- functions to create and configure a compression configuration
- add max_used_bits feature
- add max used bit version field to the compression entity
### Changed
- Change the build system form make to meson
- Change DEFAULT_CFG_MODEL and DEFAULT_CFG_DIFF to CMP_DIF_XXX constants
### Fixed
- now the adaptive compression size (ap1_cmp_size, ap2_cmp_size) is calculate when the --rdcu_par option is used

## [0.08] - 2021-01-19
### Added
- Relax the requirements on the input format
A whitespace (space (0x20), form feed (0x0c), line feed (0x0a), carriage return
(0x0d), horizontal tab (0x09), or vertical tab (0x0b) or several in a sequence
are used as separators. If a string contains more than three hexadecimal
numeric characters (0123456789abcdefABCDEF) in a row without separators, a
separator is added after every second hexadecimal numeric character. Comments
after a '#' symbol until the end of the line are ignored.
E.g. "# comment\n ABCD 1    2\n34B 12\n" are interpreted as {0xAB, 0xCD,
0x01, 0x02, 0x34, 0x0B, 0x12}.
### Changed
- update the header definition according to PLATO-UVIE-PL-UM-0001 Draft 6
    - changed version_id from 16 to 32 bit in the generic header. Add spare bits to the adaptive imagette header and the non-imagette header, so that the compressed data start address is 4 byte-aligned.
### Fixed
- Fix a bug in the definition in imagette header
### Changed
- Rename cmp_tool_lib.c to cmp_io.c

## [0.07] - 2021-12-13
- **NOTE:**  The behaviour of the cmp_tool has changed. From now on, the compressed data will be preceded by a header by default. The old behaviour can be achieved with the `--no_header` option.
- Implement the compression entity header as defined in PLATO-UVIE-PL-UM-0001 for compression and decompression
- small bug fixes

## [0.06] - 2021-11-12
- if samples = 0 the cmp_tool counts the samples in the data file and uses them
- if buffer_length = 0 the 2*samples parameter is used as buffer_length
- added feature to guess the compression configuration
- some code restructuring
- added more detailed error messages
- small bug fixes

## [0.05] - 2021-04-23
### Removed
- discard old file format. Now the only input format is like: 12 AB 23 CD .. ..
### Changed
- change to new compression and decompression library

## [0.05 Beta 2] - 2020-12-04
### Fixed
- packet file number now 4 digits long
- now the mtu size in .rdcu_pkt_mode_cfg file for the data packets

## [0.05 Beta] - 2020-11-17
### Fixed
- fixes small errors when reading the data
### Added
- add frame script to manage multiple compression in a raw
- add last_info option to generate RMAP packets to read the last compression results in parallel

## [0.04] - 2020-08-12
### Fixed
- fixes an error when reading compressed data for decompression when compiled for a 32-bit system where a long integer has 4 bytes
- fixes a bug that generates wrong packets for reading data from the RDCU with the --rdcu_pkt option

## [0.03] - 2020-07-07
### Added
- README: add a note for the --rdcu_pkt option
### Fixed
- if the --rdcu_pkt option is set the program did not create the TC_FILES directory for the tc files
- if the --rdcu_pkt option is set the program did not build the data and model packets correctly.
- if the --rdcu_pkt option is set the program now also sets the RDCU Interrupt Enable bit.
- fix typo in Help and README

## [0.02] - 2020-06-02
### Added
- add --rdcu_pkt option to generate the packets to set a RDCU compression
- add --model_cfg, --diff_cfg option to print default model/1d-diff configuration
- add --rdcu_par option to add the RDCU parameters to compression configuration and decompression information
- check the cfg file and the info file for plausibility
### Changed
- change .info file format
### Fixed
- include fixes of rmap lib
### Removed
- removes comma symbol as indicator for a comment

## [0.01] - 2020-05-06
### Added
- initialte realse
