# kID_Unreal
## Unreal Sample of k-ID

The sample provides an implementation of the various k-ID workflows as described in the [k-ID Developer Guide](https://docs.google.com/document/d/12J5mkFZvE8LC6aUsSwuxUvuONAsYrQ0aR1yPdJOXrPE/edit#heading=h.a4uiiic96g9p).  This demo leverages the First Person Template from Unreal 5, and was built using Unreal 5.4.1.  
<p align="center">
 <img src="https://github.com/kidentify/kID_Unreal/assets/3493285/71b04185-097c-4ffa-aae3-7857fea87586" />
</p>

## Installation
- Clone the repo locally
- Open the file kID_Unreal.uproject in Unreal 5.4.4
- Install Visual Studio Code and add C++ Plugins
- Configure the Unreal Engine to use Visual Studio Code as the source code editor as described in the [documentation](https://dev.epicgames.com/documentation/en-us/unreal-engine/setting-up-visual-studio-code-for-unreal-engine)
- From the Tools menu in Unreal, select Refresh Visual Studio Code Project, then select Open Visual Studio code
- Create a file called apikey.txt in the project root directory with a Test Mode k-ID API key from your k-ID Product 
     - **Warning: Do not deploy an API Key in your Unreal binary.  API keys should always be hidden on the server in a real-world implementation to avoid misuse.**
- If you will be integrating Privately, see the instructions in the Dependencies section below
- Click the Play button in Unreal to enter Play In Editor mode

## Overview
The k-ID Engine provides the framework to implement the workflow in the diagram below:
![image](https://github.com/kidentify/kID_Unreal/assets/3493285/771251ab-ce69-4509-ad16-426fbbc53d82)

The game user experience is in the control of the game developer, and this sample provides a sample UI implementation using a collection of Unreal Blueprints.

## Getting started with the k-ID API
The [k-ID API Documentation](https://game-api.k-id.com/swagger/) provides tools to try out all of the APIs interactively with a valid API key.  You can observe how different API calls used in the demo are used and try different variations of inputs.  

## Using the Demo
To use the demo, enter a valid location (e.g. US-CA, GB, etc.) into the location field at the lower left, and click Start Session.  If this location requires an age gate, then an age gate will be shown.  If the jurisdiction requires parental consent for certain game features at a certain age, and the player is under this age, then a parental consent challenge is shown, and the challenge ID will be displayed in the HUD.  

From the consent challenge window, if an email address is submitted, then a generated email will be sent to that address that contains a link to the kID parent portal.  The QR code will also lead parents to the same location.  There you can validate that you are a parent (a Legal Adult), and can grant permission to the child to play the game.  

If the player provided a date of birth of a legal adult in a jurisdiction that requires age assurance (e.g. GB), then an age assurance dialog will be shown.  

After this, if the player is a legal adult and has passed age assurance where necessary, or consent has been granted by the parent, then a new kID session starts, and the session ID will be visible in the HUD.  From then on, only session refreshes will happen after restarting the game, or until the Clear Session button is pressed to clear the previous state.  

While the challenge window is showing, a Set Challenge Status button appears at the bottom of the screen.  If this is clicked, it is possible to simulate consent, or rejection of consent in the window that appears.  This is using the `test/set-challenge-status` API and is only for demo purposes.

You can view the series of API calls that are being made to the kID Engine by looking at the Output Log in Unreal.

## Demo structure
The First Person Unreal sample is unchanged except for integration to k-ID on startup and shutdown in [kID_UnrealGameMode.cpp](Source/kID_Unreal/kID_UnrealGameMode.cpp).  

All source code that implements kID workflows resides in the [kID](Source/kID_Unreal/kID) folder.  All of the logic for kID flows is in the [KidWorkflow.cpp](Source/kID_Unreal/kID/KidWorkflow.cpp) class.  

The demo uses a set of widgets implemented as Unreal Blueprints backed by C++ classes.  Each is in the [Content/FirstPerson/Blueprints/kID](Content/kID/Blueprints) directory, and can be found and opened from the Content Drawer in Unreal.  The supporting C++ classes are:
- [PlayerHUDWidget.cpp](Source/kID_Unreal/kID/Widgets/PlayerHUDWidget.cpp): Displays information about the current age status, session id, and challenge id.
- [DemoControlsWidget.cpp](Source/kID_Unreal/kID/Widgets/DemoControlsWidget.cpp): Displays the location field and a submit button as well as the Clear Session button.
- [AgeGateWidget.cpp](Source/kID_Unreal/kID/Widgets/AgeGateWidget.cpp): Displays an age gate that requests date of birth.
- [SliderAgeGateWidget.cpp](Source/kID_Unreal/kID/Widgets/AgeGateWidget.cpp): Displays an age gate that uses a slider to select age in years.
- [FloatingChallengeWidget.cpp](Source/kID_Unreal/kID/Widgets/FloatingChallengeWidget.cpp): Displays the challenge screen to get parent consent, including the generated QR code.
- [UnavailableWidget.cpp](Source/kID_Unreal/kID/Widgets/UnavailableWidget.cpp): Blocks play when a player fails the consent challenge or is below the minimum age to play the game. 
- [TestSetChallengeWidget.cpp](Source/kID_Unreal/kID/Widgets/TestSetChallengeWidget.cpp): Calls the `test/set-challenge-status` API to simulate consent for testing purposes. 
- [AgeAssuranceWidget.cpp](Source/kID_Unreal/kID/Widgets/AgeAssuranceWidget.cpp): Demonstrates age assurance for required jurisdictions (e.g. GB) by presenting a dialog with a yes or no button for testing.
- [SettingsWidget.cpp](Source/kID_Unreal/kID/Widgets/SettingsWidget.cpp): Displays current permissions, and allows users to set them with consent if necessary.

## Dependencies

### Privately
Integration to Privately using an external web browser is available in this demo.  To enable, you will require valid Privately credentials in a file called privately.json that you should create at the root of the project folder.  The format of this file is:

`{ "session_id": "<privately session id>", "session_password": "privately session password"}`

In order to trigger age estimation using Privately facial scanning, the jurisdiction you provide must be one that requires age assurance.  An example location that will trigger age assurance is `GB`.

### QR-Code-generator
Source from an external QR Code generator C++ library called [QR-Code-generator](https://github.com/nayuki/QR-Code-generator) is included in this repo for convenience, but any QR code generation approach can be used as long as it can create a bitmap that can be placed in the 2D Texture in the `FloatingChallengeWidget` class.  

## Roadmap
While requesting consent for a feature is implemented in this demo by checking a feature checkbox in the settings window, it is currently not yet supported in the kID Engine in production.  This is a roadmap feature.
