#include "keycode.h"

#if defined(IS_MACOSX)
#include <pthread.h>

static CFMutableDictionaryRef charToCodeDict = NULL;
static pthread_once_t once_control = PTHREAD_ONCE_INIT;

/* Helper function to initialize the keycode dictionary once. */
static void initKeyDictionary(void)
{
	charToCodeDict = CFDictionaryCreateMutable(kCFAllocatorDefault, 128,
											   &kCFCopyStringDictionaryKeyCallBacks, NULL);
	if (charToCodeDict == NULL)
	{
		return;
	}
	for (CGKeyCode i = 0; i < 128; ++i)
	{
		CFStringRef keyString = createStringForKey(i);
		if (keyString != NULL)
		{
			CFNumberRef num = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &i);
			if (num != NULL)
			{
				CFDictionaryAddValue(charToCodeDict, keyString, num);
				CFRelease(num);
			}
			CFRelease(keyString);
		}
	}
}

MMKeyCode keyCodeForChar(const char c)
{
	// Ensure one-time, thread-safe initialization.
	pthread_once(&once_control, initKeyDictionary);

	if (charToCodeDict == NULL)
	{
		return K_NOT_A_KEY;
	}

	CGKeyCode code;
	UniChar character = c;
	CFStringRef charStr = CFStringCreateWithCharacters(kCFAllocatorDefault, &character, 1);
	CFNumberRef num = NULL;

	if (!CFDictionaryGetValueIfPresent(charToCodeDict, charStr, (const void **)&num))
	{
		code = UINT16_MAX; /* Error */
	}
	else
	{
		if (!CFNumberGetValue(num, kCFNumberSInt32Type, &code))
		{
			code = UINT16_MAX; /* Error */
		}
	}
	CFRelease(charStr);

	if (code == UINT16_MAX)
	{
		return K_NOT_A_KEY;
	}
	return (MMKeyCode)code;
}
#elif defined(IS_WINDOWS)
// Windows implementation remains unchanged.
MMKeyCode keyCodeForChar(const char c)
{
	MMKeyCode code;
	code = VkKeyScan(c);
	if (code == 0xFFFF)
	{
		return K_NOT_A_KEY;
	}
	return code;
}
#elif defined(USE_X11)
// X11 implementation remains unchanged.
MMKeyCode keyCodeForChar(const char c)
{
	char buf[2];
	buf[0] = c;
	buf[1] = '\0';

	MMKeyCode code = XStringToKeysym(buf);
	if (code == NoSymbol)
	{
		struct XSpecialCharacterMapping *xs = XSpecialCharacterTable;
		while (xs->name)
		{
			if (c == xs->name)
			{
				code = xs->code;
				break;
			}
			xs++;
		}
	}
	if (code == NoSymbol)
	{
		return K_NOT_A_KEY;
	}
	if (c == 60)
	{
		code = 44;
	}
	return code;
}
#endif
#if defined(IS_MACOSX)
CFStringRef createStringForKey(CGKeyCode keyCode)
{
	TISInputSourceRef currentKeyboard = TISCopyCurrentASCIICapableKeyboardInputSource();
	CFDataRef layoutData = (CFDataRef)TISGetInputSourceProperty(
		currentKeyboard, kTISPropertyUnicodeKeyLayoutData);

	if (layoutData == nil)
	{
		return 0;
	}

	const UCKeyboardLayout *keyboardLayout = (const UCKeyboardLayout *)CFDataGetBytePtr(layoutData);
	UInt32 keysDown = 0;
	UniChar chars[4];
	UniCharCount realLength;

	UCKeyTranslate(keyboardLayout, keyCode, kUCKeyActionDisplay, 0, LMGetKbdType(),
				   kUCKeyTranslateNoDeadKeysBit, &keysDown,
				   sizeof(chars) / sizeof(chars[0]), &realLength, chars);
	CFRelease(currentKeyboard);

	return CFStringCreateWithCharacters(kCFAllocatorDefault, chars, 1);
}
#endif
