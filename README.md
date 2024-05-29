# kID_Unreal
 Unreal Demo of kID


This demo uses the First Person Template from Unreal 5, and was built using Unreal 5.4.1.  The demo goes through various k-ID workflows as described in the [k-ID Developer Guide](https://docs.google.com/document/d/12J5mkFZvE8LC6aUsSwuxUvuONAsYrQ0aR1yPdJOXrPE/edit#heading=h.a4uiiic96g9p).  

## Installation
- Clone the repo locally
- Open the file kID_Unreal.uproject in Unreal 5.4.1
- Install Visual Studio Code and add C++ Plugins
- From the Tools menu in Unreal, select Refresh Visual Studio Code Project, then select Open Visual Studio code
- Create a file called apikey.txt in the project root directory with your k-ID API key from your k-ID Product
     - **Warning: Do not deploy an API Key in your Unreal binary.  API keys should always be hidden on the server in a real-world implementation to avoid misuse.**
- Click the Play button in Unreal to enter Play In Editor mode

## Using the Demo
Enter a valid location (e.g. US-CA) into the location field at the lower left, and click Start Session.  If this location requires an age gate, then an age gate is shown.  If the jurisdiction requires parental consent for certain game features at a certain age, and the player is under this age, then a parental consent challenge is shown, and the challengeId will be show in the HUD.  If an email is submitted from the challenge window, then the generated email will contain a link to the kID parent portal.  The QR code will also lead parents to the same location.  There you can validate that you are a parent (a Legal Adult), and can grant permission to the child to play the game.  If the player is a legal adult, or consent is granted by the parent, then a new session starts, and the session ID will be visible in the HUD.  From then on, there is an existing session and no further checking will happen even after restarting the game until the Clear Session button is pressed to clear the previous state.

While the challenge window is showing, a Set Challenge Status button appears at the bottom of the screen.  If this is clicked, it is possible to simulate consent, or rejection of consent in the window that appears.  This is using the test/set-challenge-status API and is only for demo purposes.

You can view the series of API calls that are being made ti the kID Engine by looking at the Output Log in Unreal.

## Demo structure
The First Person Unreal sample is unchanged except for integration to k-ID on startup and shutdown in [kID_UnrealGameMode.cpp](Source/kID_Unreal/kID_UnrealGameMode.cpp).  

All source code that implements kID workflows resides in the [kID](Source/kID_Unreal/kID) folder.  All of the logic for kID flows is in the [KidWorkflow.cpp](Source/kID_Unreal/kID/KidWorkflow.cpp) class.  

There are 5 widgets implemented as Unreal Blueprints that can be customized.  Each is in the [Content/FirstPerson/Blueprints/kID](Content/FirstPerson/Blueprints/kID) directory, and can be edited in the Content Drawer in Unreal.  The supporting classes are:
- [PlayerHUDWidget.cpp](Source/kID_Unreal/kID/Widgets/PlayerHUDWidget.cpp): Displays information about the current age status, session id, and challenge id.
- [DemoControlsWidget.cpp](Source/kID_Unreal/kID/Widgets/DemoControlsWidget.cpp): Displays the location field and a submit button as well as the Clear Session button.
- [AgeGateWidget.cpp](Source/kID_Unreal/kID/Widgets/AgeGateWidget.cpp): Displays the age gate.
- [FloatingChallengeWidget.cpp](Source/kID_Unreal/kID/Widgets/FloatingChallengeWidget.cpp): Displays the challenge screen to get parent consent, including the generated QR code.
- [UnavailableWidget.cpp](Source/kID_Unreal/kID/Widgets/UnavailableWidget.cpp): Blocks play when a player fails the consent challenge or is below the minimum age to play the game. 

## Dependencies
Source from an external QR Code generator C++ library called [QR-Code-generator](https://github.com/nayuki/QR-Code-generator) is included in this repo for convenience, but any QR code generation approach can be used as long as it can create a bitmap that can be placed in the 2D Texture in the FloatingChallengeWidget class.  

## Roadmap
While requesting consent for a single feature is implemented in this demo, it is currently not yet supported in the kID Engine in production.  This is a roadmap feature.
