# Changelog
All notable changes to this project will be documented in this file.

## [Unreleased]
### Added
- add -vv flag for extra verbose output

## [0.11] - 26-04-2023
### Added
- add -b or --binary option for read and write files in binary format
- add cross compile file for sparc
- add tests for the compression entity
- add test for cmp_rdcu_cfg.c
- add sparc inttypes.h definitions
###Changed
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

## [0.09] - 30-09-2022
### Added
- decompression/compression for non-imagette data
- functions to create and configure a compression configuration
- add max_used_bits feature
- add max used bit version field to the compression entity
###Changed
- Change the build system form make to meson
- Change DEFAULT_CFG_MODEL and DEFAULT_CFG_DIFF to CMP_DIF_XXX constants
### Fixed
- now the adaptive compression size (ap1_cmp_size, ap2_cmp_size) is calculate when the --rdcu_par option is used

## [0.08] - 19-01-2021
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

## [0.07] - 13-12-2021
- **NOTE:**  The behaviour of the cmp_tool has changed. From now on, the compressed data will be preceded by a header by default. The old behaviour can be achieved with the `--no_header` option.
- Implement the compression entity header as defined in PLATO-UVIE-PL-UM-0001 for compression and decompression
- small bug fixes

## [0.06] - 12-11-2021
- if samples = 0 the cmp_tool counts the samples in the data file and uses them
- if buffer_length = 0 the 2*samples parameter is used as buffer_length
- added feature to guess the compression configuration
- some code restructuring
- added more detailed error messages
- small bug fixes

## [0.05] - 23-04-2021
### Removed
- discard old file format. Now the only input format is like: 12 AB 23 CD .. ..
### Changed
- change to new compression and decompression library

## [0.05 Beta 2] - 04-12-2020
### Fixed
- packet file number now 4 digits long
- now the mtu size in .rdcu_pkt_mode_cfg file for the data packets

## [0.05 Beta] - 17-11-2020
### Fixed
- fixes small errors when reading the data
### Added
- add frame script to mange multiple compression in a raw
- add last_info option to generate RMAP packets to read the last compression results in parallel

## [0.04] - 12-08-2020
### Fixed
- fixes an error when reading compressed data for decompression when compiled for a 32-bit system where a long integer has 4 bytes
- fixes a bug that generates wrong packets for reading data from the RDCU with the --rdcu_pkt option

## [0.03] - 07-07-2020
### Added
- README: add a note for the --rdcu_pkt option
### Fixed
- if the --rdcu_pkt option is set the program did not create the TC_FILES directory for the tc files
- if the --rdcu_pkt option is set the program did not build the data and model packets correctly.
- if the --rdcu_pkt option is set the program now also sets the RDCU Interrupt Enable bit.
- fix typo in Help and README

## [0.02] - 02-06-2020
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

## [0.01] - 06-05-2020
### Added
- initialte realse


