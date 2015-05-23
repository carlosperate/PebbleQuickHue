# QuickHue for Pebble
Pebble app to facilitate quick control of a Philips Hue light bulb.

### [Link to QuickHue in the Pebble App Store][1]

The Pebble App will toggle the preselected light ON or OFF as soon as it loads, so it is designed to be registered as a button shortcut ([Quick Launch Pebble feature][2]) for quick light control. The up and down buttons when the app is open will change the brightness of the light.

![QuickHue for Pebble screenshot 1][screenshot_1]
![QuickHue for Pebble screenshot 2][screenshot_2]


## Installing QuickHue
This app can be installed from the Pebble App store: [QuickHue for Pebble][1]

The latest, not for production, app version from the GitHub repository can be installed by [clicking this link on your phone][3]. The Pebble phone app should be able to catch the `.pbw` file and install it in your Pebble smarthwatch. 


## Using QuickHue with your Philips Hue bridge
A future update to the app will allow automatic pairing with the bridge, but at the moment to use the application with your Philips Hue bridge you will need to provide a developer user ID in the settings page.

![QuickHue for Pebble settings screenshot][screenshot_3]

More information about creating a user can be found in the 
[HUE API - Getting Started page][4].


<sub>"Hue Personal Wireless Lighting" is a trademark owned by Koninklijke Philips N.V., see www.meethue.com for more information.</sub>

<sub>This project and its developer/s are in no way affiliated with Koninklijke Philips N.V.</sub>


[1]: https://apps.getpebble.com/applications/5526f89e1c36ea04bd00006b
[2]: http://help.getpebble.com/customer/portal/articles/1407457-firmware-release-notes#2.6
[3]: https://github.com/carlosperate/PebbleQuickHue/releases/download/v0.1/QuickHue.pbw
[4]: http://www.developers.meethue.com/documentation/getting-started

[screenshot_1]: https://raw.githubusercontent.com/carlosperate/PebbleQuickHue/master/screenshots/screenshot_1.png
[screenshot_2]: https://raw.githubusercontent.com/carlosperate/PebbleQuickHue/master/screenshots/screenshot_2.png
[screenshot_3]: https://raw.githubusercontent.com/carlosperate/PebbleQuickHue/master/screenshots/screenshot_config_1_small.png
