# kID_Unreal
 Unreal demo of kID


This demo uses the First Person Template from Unreal 5, and was built using Unreal 5.4.1.  The demo goes through various k-ID workflows as described in the [k-ID Developer Guide](https://docs.google.com/document/d/12J5mkFZvE8LC6aUsSwuxUvuONAsYrQ0aR1yPdJOXrPE/edit#heading=h.a4uiiic96g9p).  

## Installation
- Clone the repo locally
- Open the file kID_Unreal.uproject in Unreal 5.4.1
- Create a file called apikey.txt in the project root directory with your k-ID API key from your Product
- Run the game within Unreal

## Dependencies
Source from an external QR Code generator C++ library called [QR-Code-generator](https://github.com/nayuki/QR-Code-generator) is included in this repo for convenience, but any QR code generation approach can be used as long as it can create a bitmap that can be placed in the Texture2D.  
