# fty-common-messagebus
fty-common-messagebus:

* Centralize all methods to address Request/Reply, Publish/Subscribe patterns above malamute message bus. 

## How to build

To build fty-common-messagebus project run:

```bash
./autogen.sh
./configure
make
make check # to run self-test
```

## How to use the dependency in your project

In the project.xml, add following lines:

```bash
<use project = "fty-common-messagebus" libname = "libfty_common_messagebus" header = "fty_common_messagebus.h"
        repository = "https://github.com/42ity/fty-common-messagebus.git"
        release = "master"
        test = "fty_common_messagebus_selftest" />
```

## Howto 

See all samples in src folder.
