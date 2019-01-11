# two-level-tree
The two level tree data structure is commonly used in TSP (travelling salesman problem) solvers to efficiently manipulate a tour. 

According to Ref. [1] page 4, the essential four methods are provided to query and change a tour:

- Next: `get_next`

- Prev: `get_prev`

- Between:  `is_between`

- Flip: `flip`

as well as many other helper methods. More information can be found in section 2.2 of Ref. [1].
## Documentation

Detailed *Doxygen*-style comments accompany the source code. You can also refer to the test code to learn how to use the `TwoLevelTree` class.

This implementation is extensively tested using the [Catch2](https://github.com/catchorg/Catch2) framework. See the below section on how to build and run the tests.

## How to use
- C++ 11 complier is required.
- Add the *include* directory to your INCLUDE path.
- Drop *src/two_level_tree.cpp* into your source code directory.
- `#include "two_level_tree.h"` in your source code.

A code snippet is 
```c++
// create a two-level tree for 100 cities and the first city is labelled 0
tsp::TwoLevelTree tree{ 100, 0 };
// set the city order: path is a vector containing 100 cities in a certain order
tree.set_raw_tour(path); 
```
## Build & run the tests
First change into the *test* directory: `cd test`

Then, create a *build* directory: `mkdir build` and `cd build`

Run CMake: 
​	- On Windows: `cmake -DCMAKE_GENERATOR_PLATFORM=x64 ..`
​	- Others: `cmake ..`

Build the test executable:
​	- On Windows: open the *tsp.sln* generated in the above step with Visual Studio, and then build & run
​	- Others: `make` and 

## Reference
[1] Fredman, Michael L., David S. Johnson, Lyle A. McGeoch, and Gretchen Ostheimer. "Data structures for traveling salesmen." Journal of Algorithms 18, no. 3 (1995): 432-479.





