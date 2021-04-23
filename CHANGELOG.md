# Changelog
All notable changes to this project will be documented in this file.

## [Unreleased]
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


