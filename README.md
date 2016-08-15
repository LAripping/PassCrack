# PassCrack 
A personal project which started as an assignment for my "Computer Security" course, developed for almost two semesters.
PassCrack is a powerful passsword cracker written in C/C++ that uses Rainbow Tables and (currently) works for passwords hashed with the _Blake_ algorithm.
It is a molular and configurable enough to demonstrate the _Time-Memory Tradeoff_, originally analyzed by [Phillipe Oechslin](http://lasec.epfl.ch/~oechslin/publications/crypto03.pdf) and to support other hashing algorithms, password alphabets, reductions functions etc.



## Code Layout
File | Purpose
-----|--------
README.txt | The assignment's description, written in greek. Original Documentation.
PassCrack | The application binary, current release. (Compsec16)
main.cpp | The code for the main thread. Parses command line arguments, orchestrates spawned threads, defines some globals and contains the code-backbone
rainbowfuncts.cpp | Implementations of the general-purpose functions used.
generate.cpp | Defines functions regarding table generation.
test.cpp | Testing module
blake.cpp, blake.hpp | Implementation and Interface files for the BLAKE256 hash function used. Based on the [standard](https://131002.net/blake/#dl).
tablemodel.txt, chainmodel.txt | Table and chain demos
makefile | Build script
generator-script.bash | A bash script to automate back2back runs for small tables to avoid memory overuse.
changelog.txt | A diary of major fixes between versions.

__NOTE:__ This programm uses modern libraries, part of the C++11 standard, which the compiler used must support.



## Design & Operations
The operations carried out by this programm can be classified in 2 modes:

1. OFFLINE mode, where the rainbow table is generated or imported and execution parameters are set.
2. ONLINE mode, where given a hashed password, a "Reduce-Lookup-Hash" loop is initiated.

Supposing an environment where the password must be "reversed" (from the hash) in a short period of time, with the password being refreshed and rehashed after the end of the period, an event to halt the programm is required. Consequently, a SIGTSTOP signal (Ctr+Z) stops the loop and a "new hash" prompt is spawned.

Similarly, to terminate the programm a SIGINT (Ctr+C) must be signalled.

### Reduction Functions
The purpose of these routines is to map -as "one-way" as possible"- a 32byte hash to a 6character password from an alphabet of 64 valid characters (!,@, 1,2,...,9, A,B,...,Z, a,b,...,z). Therefore (pseudo-)randomness and entropy are the drivers behind each technique. 
The current version include a set reduction-function-families, but decoupling the _password alphabet_ extra families can be implemented, and instantly used. Each family defines several reduction functions based on the same tecnhique, slightly altered depending on each function's index. The techniques per family are the following:

* __Set #1: "Semi ASCII"__ [32 indices]

  All 32 functions choose 6 bytes from the hash. 
  * The first one picks bytes 0,1,...,5.
  * The second one picks bytes 1,2,...,6,
  
  and so it goes until the 33rd byte is requested where the 1st is given instead. Continuing with a mod32 arithmetic, the number of genuinely different functions is limited to 32.
  For each one of these 6 bytes, after zero-ing out the msb to match the 7bit ASCII code, the values are matched to a character like this:
    
    00 ...... 2F | 30 ...... 39 | 3A ... 40 | 41 ...... 5A | 5B ... 60 | 60 ...... 7A | 7B ... 7F
    -------------|--------------|-----------|--------------|-----------|--------------|----------
    !       | 0, 1, ..., 9  |    @      | A, B, ...,Z  |     !     |  a, b, ...,z  |     @


* __Set #2: "High Kolmogorov Complexity"__ [992 indices]

  All functions choose 6 bytes from the hash and map them using mod64 arithmetic as above. Every function belongs to only one of the 31 "classes". Each class includes the 32 genuinely different functions, all of which start the selection from the same byte. e.g.:
  * Functions of the 1st class start with byte 0.
  * Functions of the 2nd class start with byte 1 etc.
  Then, the next byte for each is selected adding the function's intra-class index (1-32) to the selected byte's number, cicrulating around 32. e.g.:
  * Function #28 of class 2 selects bytes 28, 30, 0, 2, 4, 6.
  
  
* __Set #3: "Mix & Match"__ [32 indices]

  Driven by simplicity and calculation time (in CPU cycles), this technique is based on a mod64 logic, salted by some "magic numbers".
  
  
* __Set #4: "PRNG/Uniform indices"__ [infinite indices]

  Following the common "PRNG & hash" scheme, a generator is seeded with the function's index. After 6 calls, the generated numbers are the selectors for 6 bytes from the hash, the values of which are mapped to characters from the alphabet using mod64 arithmetic.
  
  
* __Set #5: "PRNG/Uniform characters"__ [infinite indices]

  Following the common "PRNG & hash" scheme, a generator is seeded with the value of a byte. The function's index is the selector for this byte. After 6 calls, the generated numbers are mapped to password characters from the alphabet.
  
  
* __Set 36: "Human-like"__ [for future implementation]

  A family of functions that will meet the second Reduction Function Requirement (besides high entropy). That is, to output passwords that a human would probably select, or at least pronεξασφαλιζωounceable (thus memorable!).
  An idea to achieve the above, would be to map bytes against an alphabet of phonems/digramms/syllables that are most commonly used in most languages.
  Another idea is to inject dates (e.g. 1994) inside possible passwords mimicing the human behavior.
  Such behavior was not required for this assignment since the password was randomly chosen.
  
  

## Command Line Options
Make sure you have read and understood the Design Principles listed above, before proceeding.
```bash
  ./PassCrack [online|offline] [-uvcgqQ       ] [-m <chains>  ]
              [-h <load>     ] [-i <intable>  ] [-t <links>   ]
              [-p <threads>  ] [-o <outable>  ] [-l <tables>  ]
              [-r <redfunset>] [-s <outchain> ]
```

Where:
  * No option is mandatory
  * Options can be in any order, as long as the options requiring an argument are followed by this argument.
  * Specifically:

    * ```online```/```offline``` Mode of operation. Online mode requires table input/generation (default).
    * ```-u (usage)``` Print this guide.
    * ```-v (verbose)``` Print extra info during execution. Recommended.
    * ```-c (calculate)``` Calculate success probability for the given parameters using Oechslins formula.
    * ```-g (generator)``` Use the modern, more robust PRNG defined in the C++11 standard for the table generation.
    * ```-q (unique)``` Enable table post-generation processing to discard duplicate chains (with similar endpoints). Option only meaningful if the table is generated and not imported.
    * ```-Q (unique)``` Like ```-q``` but each chain's uniqueness is examined right after creation. Ensures table size equal to the requested (see ```-m``` option) with a cost of many CPU cycles. Option only valid if table is generated by a single thread (see ```-p``` option).
    * ```-r (reduction)``` Use ```<redfunset>``` family for the reduction functions. Defaut: ```3```.
    * ```-h (hashmap)``` Convert the table to a hash map for O(1) Lookup complexity. It is also possible to declare eac bucket's "load factor". Default: ```100.0```.
    * ```-p (parallel)``` Use ```<threads>``` threads to generate the table concurrently. Option only valid if table is not imported (see ```-i``` option). If combined with ```-s``` option, only the first thread's chains are printed for verification.
    * ```-i (input) ```/```-o (output)``` Input/output the table from/to the ```<intable>```/```<outtable>``` file. Files should have the format specified in the [model file](https://github.com/LAripping/PassCrack/blob/master/tablemodel.txt). Both absolute and relative paths are supported. The length of each Reduce-Lookup-Hash loop depends on the ```t``` parameter which should match the table's length, even if imported. Therefore ```-i``` option must be combined with the ```-m``` and ```-t``` ones.
    * ```-s (save) ``` Export the 10 first complete chains in the ```<outchain>``` file for debugging/verification. Not recommended due to the danger of creating massive files. The written file has a format similar to the [model](https://github.com/LAripping/PassCrack/blob/master/chainmodel.txt). Both absolute and relative paths are supported.
    * ```-m``` Explicit definiton of table parameters 'm' (total chains per table) and 't' (total chains per table). Defaut: ```m=100, t=32```.
    * ```-l``` Create ```<tables>``` tables in total. Default and currently only supported value: ```1```.
    
