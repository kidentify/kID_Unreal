# kID_Unreal
 Unreal demo of kID


This demo uses the First Person Template from Unreal 5, and was built using Unreal 5.4.1.  The demo goes through various k-ID workflows as described in the [k-ID Developer Guide](https://docs.google.com/document/d/12J5mkFZvE8LC6aUsSwuxUvuONAsYrQ0aR1yPdJOXrPE/edit#heading=h.a4uiiic96g9p).  

## Installation
- Clone the repo locally
- Open the file kID_Unreal.uproject in Unreal 5.4.1
- Install Visual Studio Code and add C++ Plugins
- From the Tools menu in Unreal, select Refresh Visual Studio Code Project, then select Open Visual Studio code
- Create a file called apikey.txt in the project root directory with your k-ID API key from your k-ID Product
- Click the Play button in Unreal to enter Play In Editor mode

## Dependencies
Source from an external QR Code generator C++ library called [QR-Code-generator](https://github.com/nayuki/QR-Code-generator) is included in this repo for convenience, but any QR code generation approach can be used as long as it can create a bitmap that can be placed in the Texture2D.  

## Using the Demo
Enter a valid location (e.g. US-CA) into the location field, and click Start Session.  If this location requires an age gate, then an age gate is shown.  If the jurisdiction requires parental consent for certain game features at a certain age, and the player is under this age, then a parental consent challenge is shown.  If an email is submitted, then the generated email will contain a link to the kID parent portal.  The QR code will also lead parents to the same portal.  Here you can validate that you are a parent (a Legal Adult), and can grant permission to the child to play the game.  If the player is a legal adult, or consent is granted by the parent, then a new session starts.  From then on, there is an existing session and no further checking will happen even after restarting the game until the Clear Session button is pressed to clear the previous state.
