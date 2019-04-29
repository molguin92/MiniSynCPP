# MiniSynCPP

Reference implementation in C++11 of the MiniSync/TinySync time synchronization algorithms detailed in [1, 2].
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
$> cmake --build . --target MiniSynCPP -- -j 4
```

Note: Even though the project uses the Google Protobuf libraries, it is not necessary to have these installed when 
compiling, as the build chain will download its own copy anyway to statically link them. This is mainly done for 
portability and cross-compilation purposes.

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
