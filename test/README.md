# pflib Testing

Since Rogue depends on Boost and we expect to use Rogue in the future,
we will use Boost in pflib. Boost has a unit test framework subcomponent
[Boost.Test](https://www.boost.org/doc/libs/1_85_0/libs/test/doc/html/index.html)
which fulfills our needs.

The tests are built along with `pflib` and do not need to be enabled.
You should run them from within the `build` directory so that the temporary files
are left behind within the `build` directory that can be easily cleaned up.
```
cd build
./test-pflib
```

Boost.Test constructs the main program for us and there are many command line
options available (use `--help` to see a list of the main ones).
I suggest using `-l all` (short for `--log-level all`) so that all test cases
(both passing and failing) are listed. This is helpful when writing your tests
to make sure new tests you are adding are included.
