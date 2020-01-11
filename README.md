## X-Plane 10 plugin stub

# About...
This plugin stub is compatible with X-Plane ver. 10.00+ (32-bit) or 10.20+ (64-bit).
Although it was compiled against X-Plane SDK 3.0.1, containing XPLM200, XPLM210, XPLM300, and XPLM301 APIs, only the first two are used. Thus, previous versions of SDK may be suitable as well. See https://developer.x-plane.com/sdk/plugin-sdk-downloads/ for further details.

Once X-Plane SDK is downloaded and unzipped (e.g. http://developer.x-plane.com/wp-content/plugins/code-sample-generation/sample_templates/XPSDK301.zip), symbolic links named "Widgets", "Wrappers", "XPLM" should be created to where the directories XPSDK301.zip#SDK/CHeaders/Widgets, XPSDK301.zip#SDK/CHeaders/Wrappers, and XPSDK301.zip#SDK/CHeaders/XPLM were unzipped. Of course, it is also possible to just copy those directories directly into the project.

The Makefile is configured to produce artifact for Linux.

# License
The LGPL 3.0 applies.
