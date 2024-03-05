# Data Distribution Service (DDS) Key Values Sample Application

This folder contains an application which can be run as either a sender
(publisher) or receiver (subscriber) for key value pairs written to a DDS
databus.

When run as a sender (`-s` option) the application will write a single key value
pair, using the values provided to for the `-k` and `-v` arguments respectively,
then exit.

When run as a receiver (`-r` option) the application will poll the DDS databus
every second for received key value pairs and will print them to the console.


## Example

Running this application requires an instance of the OpenDDS `DCPSInfoRepo` to
be running such that its default `repo.ior` file is available in the directory
from which the `dds_key_values` application is run. This example assumes that
OpenDDS has been installed to `/opt/opendds` and every command will be run from
the directory in which the `dds_key_values` executable is located.

Each of the following steps should be performed in a new terminal window.

### 1. Start an OpenDDS DCPS Info Repo

```sh
$ /opt/opendds/bin/DCPSInfoRepo
```

### 2. Run a Receiver

```sh
$ ./dds_key_values -r
```

The application should start and should start querying and printing values
every second:

```
[info] [gxf_executor.cpp:1894] Running Graph...
[info] [gxf_executor.cpp:1896] Waiting for completion...
[info] [gxf_executor.cpp:1897] Graph execution waiting. Fragment:
[info] [greedy_scheduler.cpp:190] Scheduling 2 entities
[info] [dds_key_values.cpp:75] Query 0:
[info] [dds_key_values.cpp:75] Query 1:
[info] [dds_key_values.cpp:75] Query 2:
...
```

### 3. Send Values

Use the `-k` and `-v` arguments to specify the key and value, respectively. The
sender can be run many times to provide new key value pairs or to overwrite
existing ones.

```sh
$ ./dds_key_values -s -k Name -v Jack
$ ./dds_key_values -s -k HR -v 86
$ ./dds_key_values -s -k Name -v Jill
```

The output from the receiver should then update with the new values:

```
[info] [dds_key_values.cpp:75] Query 10:
[info] [dds_key_values.cpp:75] Query 11:
[info] [dds_key_values.cpp:77]   Name / Jack
[info] [dds_key_values.cpp:75] Query 12:
[info] [dds_key_values.cpp:77]   Name / Jack
[info] [dds_key_values.cpp:77]   HR / 86
[info] [dds_key_values.cpp:75] Query 23:
[info] [dds_key_values.cpp:77]   Name / Jill
[info] [dds_key_values.cpp:77]   HR / 86
```
