#ifdef __APPLE__
#include <Foundation/Foundation.h>
#include <AppKit/AppKit.h>

// macOS-specific implementation
FString GetDefaultBrowser(const FString &URL)
{
    // Convert FString to NSString
    NSString* nsURLString = [NSString stringWithUTF8String:TCHAR_TO_UTF8(*URL)];
    NSURL* nsURL = [NSURL URLWithString:nsURLString];

    if (nsURL)
    {
        CFURLRef appURL = LSCopyDefaultApplicationURLForURL((__bridge CFURLRef)nsURL, kLSRolesAll, NULL);
        if (appURL)
        {
            NSString* appPath = [(__bridge NSURL*)appURL path];
            CFRelease(appURL);
            return FString(appPath.UTF8String);
        }
    }
    return FString();
}

#elif _WIN32
#include <windows.h>
#include <string>

// Windows-specific implementation
FString GetDefaultBrowser(const FString &URL)
{
    // Function to get default browser on Windows
    HKEY hKey;
    TCHAR szBrowser[MAX_PATH];
    DWORD dwBufferSize = sizeof(szBrowser);

    // Open the registry key for the default HTTP handler
    if (RegOpenKeyEx(HKEY_CLASSES_ROOT, TEXT("HTTP\\shell\\open\\command"), 0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        // Get the default value of the key
        if (RegQueryValueEx(hKey, NULL, NULL, NULL, (LPBYTE)szBrowser, &dwBufferSize) == ERROR_SUCCESS)
        {
            RegCloseKey(hKey);

            // Convert TCHAR to FString
            FString browserPath = FString(szBrowser);
            return browserPath;
        }
        RegCloseKey(hKey);
    }

    return FString();
}
#endif