# Data Distribution Service (DDS) Video Streaming Sample Application

This folder contains an application which can be run as either a sender
(publisher) or receiver (subscriber) for a video stream that is written to a
DDS databus.

When run as a sender (`-s` option) the application will use V4L2 to open a
camera device (e.g. a USB webcam) and will write the video frames to the
DDS databus.

When run as a receiver (`-r` option) the application will read the video stream
from the DDS databus and will render the video using Holoviz. Multiple receiver
instances can be active and reading/rendering the video stream simultaneously.
The receiver will also listen for any key value pairs that are written to the
databus, such as those written with the `dds_key_values` application, and will
also overlay any received values onto the rendered video.

## Example

Running this application requires an instance of the OpenDDS `DCPSInfoRepo` to
be running such that its default `repo.ior` file is available in the directory
from which the `dds_video` application is run. This example assumes that OpenDDS
has been installed to `/opt/opendds` and every command will be run from the
directory in which the `dds_video` executable is located.

Each of the following steps should be performed in a new terminal window.

### 1. Start an OpenDDS DCPS Info Repo

```sh
$ /opt/opendds/bin/DCPSInfoRepo
```

### 2. Run a Sender

```sh
$ ./dds_video -s
```

### 3. Run a Receiver

```sh
$ ./dds_video -r
```

A window should appear with the rendered video. Note that multiple receivers can
be launched using this command, and each one will read and render the video
stream to its own window.

### 4. Send Key Value Pairs

The `dds_key_values` application can be used to write values to the databus for
the receivers to read and overlay on top of the video stream. For example:

```sh
$ ./dds_key_values -s -k Name -v Jack
$ ./dds_key_values -s -k HR -v 86
```

Should result in the following overlay being rendered at the top left of the
receiver's video stream:

```
Name: Jack
HR: 86
```
