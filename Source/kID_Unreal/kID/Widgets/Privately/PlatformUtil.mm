#import <Foundation/Foundation.h>
#import <AppKit/AppKit.h>


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