# RABERIX: X-Plane integration with Python

## About...
This is a plugin for X-Plane 10 or above which lets You write further X-Plane plugins as Python scripts.

## How to build?
This plugin is compatible with X-Plane ver. 10.00+ (32-bit) or 10.20+ (64-bit).
Although it was compiled against X-Plane SDK 3.0.1, containing XPLM200, XPLM210, XPLM300, and XPLM301 APIs, only the first two are used. Thus, previous versions of SDK may be suitable as well. See https://developer.x-plane.com/sdk/plugin-sdk-downloads/ for further details.

Once X-Plane SDK is downloaded and unzipped (e.g. http://developer.x-plane.com/wp-content/plugins/code-sample-generation/sample_templates/XPSDK301.zip), symbolic links named "Widgets", "Wrappers", "XPLM" should be created to where the directories ```XPSDK301.zip#SDK/CHeaders/Widgets```, ```XPSDK301.zip#SDK/CHeaders/Wrappers```, and ```XPSDK301.zip#SDK/CHeaders/XPLM``` were unzipped to, respectively. Of course, it is also possible to just copy those directories directly into the project.

The Makefile is configured to produce artifact for Linux - ```raberix.xpl```.

## License
The LGPL 3.0 applies. See: ```LICENSE``` file.

## How does it work?
After a successful build, the outcome file ```raberix.xpl``` should be copied to ```$XPLANE_HOME/Resources/plugins```, where ```$XPLANE_HOME``` is Your X-Plane installation directory. There, You can also put Your own ```raberix.py``` file with Your custom plugin code written in Python. An example ```raberix.py``` content is as follows:

```
import raberix

nav1_freq_hz = raberix.find_dataref('sim/cockpit/radios/nav1_freq_hz')

def fhandler():
    freq = raberix.get_dataref(nav1_freq_hz)
    print("NAV 1 frquency: {} kHz".format(freq / 100))
    sys.stdout.flush()
    return -1.0;

raberix.set_flight_loop_handler(fhandler)
```
The above script defines a plugin code where, in each flight simulation iteration (every 0.1 s., by default), the _XPLMDataRef_  named ```'sim/cockpit/radios/nav1_freq_hz'``` is read and printed to the console. This refers to the current (i.e. not standby) frequency of the first VOR receiver (NAV 1). See: https://developer.x-plane.com/datarefs/ for dataref list.

If You plugin Python script is not ```$XPLANE_HOME/Resources/plugins/raberix.py```, You can start X-Plane with environment variable ```$RABERIX_SCRIPT``` set to Your file path.

The embedded Python interpreter tries to discover a Python environment existing in Your host and set ```sys.path``` accordingly, before Your script is called.
Then, Your script may use any library that You have installed in Your environment (e.g. with ```pip install``` or ```conda install``` commands).
In order to correctly recognize Your preferred Python home, the following environment variables are scanned (in that order, first non-empty one applies): ```$RABERIX_PYTHON_HOME```, ```$CONDA_HOME```, ```$PYTHON_HOME```, ```$PYTHONHOME```.
If no home is found in the above way, the plugin will not start its Python embedded interpreter, thus Your Python script will not work.
Be careful about the Python environment - it must contain at least ```encodings``` modules, otherwise Your whole X-Plane will fail to start!

## Python scripts
Another example of a simple script is:
```
import raberix

nav1_freq_hz = raberix.find_dataref('sim/cockpit/radios/nav1_freq_hz')
throttle_up = raberix.find_commandref('sim/engines/throttle_up')
my_command = raberix.create_command('sim/engines/my_command', 'My command...')
toggle = 1

def mycmdhandler(command_id, handler_id, is_before, phase):
    global toggle
    if phase != 0:
        return 1
    freq = raberix.get_dataref(nav1_freq_hz)
    print("[{}] NAV 1 frquency: {} kHz".format(phase, freq / 100))
    sys.stdout.flush()
    if freq > 10800:
        raberix.set_dataref(nav1_freq_hz, freq - 10)
    raberix.do_command(throttle_up, toggle)
    toggle = 3 - toggle
    return 1

raberix.add_command_handler(my_command, mycmdhandler, True)

def fhandler():
    return -1.0;

raberix.set_flight_loop_handler(fhandler)
```

This scripts creates new command 'sim/engines/my_command', which You can map to Your equipment in X-Plane (via "Settings" -> "Joystick & Equipment" -> {"Buttons: Adv", "Keys"}). This command, on pressing (but not holding or releasing it) a button mapped to it, shows the current active fequency of NAV 1, decreases it by 100kHz, and either starts throttling up or stops it. Of course, this simple "business logic" has no real meaning in aviation, but shows a sample of what can be achieved. 
