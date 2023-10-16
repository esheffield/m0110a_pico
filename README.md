# m0110a_pico
Raspberry Pi Pico replacement controller for Mac Plus M0110A keyboard

Note: For whatever reason, at this point this code will NOT work with optimizations turned on. Weird things happen with the row counter (r) variable. It gets incremented BEFORE the loop has completed, and then when looking up the key code from KEYMAP the row dimension index is incorrect.

Additionally, if the variable is printed out at some point BEFORE the column loop, then it's fine. Completely disabling all optimizations fixes the problem as well. So for now build with:

```
cmake -DPICO_DEOPTIMIZED_DEBUG=1 -DCMAKE_BUILD_TYPE=Debug ..
make -d
```
