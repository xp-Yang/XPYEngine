#include "MacFileDialog.hpp"

#import <AppKit/AppKit.h>
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>

#include <cstring>
#include <string>
#include <vector>

namespace
{
void appendExtensionFromPattern(std::vector<std::string>& extensions, const char* begin, const char* end)
{
	while (begin < end && (*begin == ' ' || *begin == ';' || *begin == ',')) {
		begin++;
	}
	while (end > begin && (*(end - 1) == ' ' || *(end - 1) == ';' || *(end - 1) == ',')) {
		end--;
	}

	if (end - begin >= 3 && begin[0] == '*' && begin[1] == '.') {
		extensions.emplace_back(begin + 2, end);
	}
}

std::vector<std::string> parseFilterExtensions(const char* filter)
{
	std::vector<std::string> extensions;
	if (!filter) {
		return extensions;
	}

	const char* entry = filter;
	while (*entry) {
		const char* pattern = entry + std::strlen(entry) + 1;
		if (!*pattern) {
			break;
		}

		const char* token_begin = pattern;
		for (const char* p = pattern; ; p++) {
			if (*p == ';' || *p == ',' || *p == '\0') {
				appendExtensionFromPattern(extensions, token_begin, p);
				if (*p == '\0') {
					entry = p + 1;
					break;
				}
				token_begin = p + 1;
			}
		}
	}

	return extensions;
}

NSArray<UTType*>* allowedContentTypes(const char* filter)
{
	NSMutableArray<UTType*>* result = [NSMutableArray array];
	for (const std::string& ext : parseFilterExtensions(filter)) {
		NSString* extension = [NSString stringWithUTF8String:ext.c_str()];
		UTType* type = [UTType typeWithFilenameExtension:extension];
		if (type) {
			[result addObject:type];
		}
	}
	return result.count > 0 ? result : nil;
}

std::string pathFromUrl(NSURL* url)
{
	if (!url) {
		return {};
	}

	const char* path = url.path.UTF8String;
	return path ? std::string(path) : std::string();
}
}

std::string MacFileDialog::OpenFile(const char* filter)
{
	@autoreleasepool {
		NSOpenPanel* panel = [NSOpenPanel openPanel];
		panel.canChooseFiles = YES;
		panel.canChooseDirectories = NO;
		panel.allowsMultipleSelection = NO;

		NSArray<UTType*>* content_types = allowedContentTypes(filter);
		if (content_types) {
			panel.allowedContentTypes = content_types;
		}

		if ([panel runModal] == NSModalResponseOK) {
			return pathFromUrl(panel.URL);
		}

		return {};
	}
}

std::string MacFileDialog::SaveFile(const char* filter)
{
	@autoreleasepool {
		NSSavePanel* panel = [NSSavePanel savePanel];

		NSArray<UTType*>* content_types = allowedContentTypes(filter);
		if (content_types) {
			panel.allowedContentTypes = content_types;
		}

		if ([panel runModal] == NSModalResponseOK) {
			return pathFromUrl(panel.URL);
		}

		return {};
	}
}
