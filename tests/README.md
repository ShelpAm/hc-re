After adding a test, you should call `gtest_discover_tests(<test>)` at the end of the file.


## Test

To run the tests of the project, simply run the following after a full build:
```bash
ctest --test-dir build/tests --output-on-failure
```

