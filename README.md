# MiniSynCPP
[![Build Status](https://travis-ci.org/molguin92/MiniSynCPP.svg?branch=master)](https://travis-ci.org/molguin92/MiniSynCPP)

Reference implementation in C++11 of the MiniSync/TinySync time synchronization algorithms detailed in [\[1, 2\]](#references).
Note that this implementation is still pretty naive and probably should not be used for anything critical (yet).

## Compilation
The only requirements are:

- CMAKE
- C++11 Compiler (gcc, g++, clang).

Compilation is then just a matter of running:
```bash
$> mkdir ./cmake_build 
$> cd ./cmake_build
$> cmake ..
$> cmake --build . --target all -- -j 4
```

Note: Even though the project uses the Google Protobuf libraries, it is not necessary to have these installed when 
compiling, as the build chain will download its own copy anyway to statically link them. This is mainly done for 
portability and cross-compilation purposes.

## Usage

The program includes a help message accessible through the ```-h, --help``` flags.

```bash
$> MiniSynCPP --help
MiniSynCPP v0.4. Standalone demo implementation of the Tiny/MiniSync time synchronization algorithms.
Usage: MiniSynCPP [OPTIONS] SUBCOMMAND

Options:
  -h,--help                   Print this help message and exit

Subcommands:
  REF_MODE                    Start node in reference mode; i.e. other peers synchronize to this node's clock.
  SYNC_MODE                   Start node in synchronization mode.
```

Help is also available per application mode:
```bash
$> MiniSynCPP SYNC_MODE --help
Start node in synchronization mode.
Usage: MiniSynCPP SYNC_MODE [OPTIONS] BIND_PORT ADDRESS PORT

Positionals:
  BIND_PORT UINT REQUIRED     Local UDP port to bind to.
  ADDRESS TEXT REQUIRED       Address of peer to synchronize with.
  PORT UINT REQUIRED          Target UDP Port on peer.

Options:
  -h,--help                   Print this help message and exit
  -v INT=-2                   Set verbosity level.
  -o,--output TEXT            Output stats to file.
  -b,--bandwidth FLOAT        Nominal bandwidth in Mbps, for minimum delay estimation.
  -p,--ping FLOAT             Nominal minimum ICMP ping RTT in milliseconds for better minimum delay estimation.

$> MiniSynCPP REF_MODE --help
Start node in reference mode; i.e. other peers synchronize to this node's clock.
Usage: MiniSynCPP REF_MODE [OPTIONS] BIND_PORT

Positionals:
 BIND_PORT UINT REQUIRED     Local UDP port to bind to.

Options:
 -h,--help                   Print this help message and exit
 -v INT=-2                   Set verbosity level.
```

Basic usage involves:

1. Initializing a node on one client in reference mode. *Reference mode* means that the node will act as a reference 
to other clocks, i.e. its clock remains unaltered and other clocks synchronize to it.

Example: 
```bash
user@ref_node $> MiniSynCPP -v 0 REF_MODE 1338
```

2. Initializing a node on another client in sync mode, and point it to the reference node. 

Example:

```bash
user@sync_node $> MiniSynCPP -v 0 SYNC_MODE 1338 192.168.0.123 1338 --bandwidth 300 --ping 1.20
```

## References
[1] S. Yoon, C. Veerarittiphan, and M. L. Sichitiu. 2007. Tiny-sync: Tight time synchronization for wireless sensor 
networks. ACM Trans. Sen. Netw. 3, 2, Article 8 (June 2007). 
DOI: 10.1145/1240226.1240228 

[2] M. L. Sichitiu and C. Veerarittiphan, "Simple, accurate time synchronization for wireless sensor networks," 2003 
IEEE Wireless Communications and Networking, 2003. WCNC 2003., New Orleans, LA, USA, 2003, pp. 1266-1273 vol.2. DOI: 
10.1109/WCNC.2003.1200555. URL: http://ieeexplore.ieee.org/stamp/stamp.jsp?tp=&arnumber=1200555&isnumber=27029

## Copyright
 [![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

Code is Copyright© (2019 -) of Manuel Olguín Muñoz \<manuel@olguin.se\>, provided under an MIT License.
See [LICENSE](LICENSE) for details.

The TinySync and MiniSync algorithms are owned by the authors of the referenced papers [1, 2].
