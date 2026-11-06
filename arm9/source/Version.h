#pragma once

/// @brief The version this build says it is. Bumped by hand in the commit that
///        rolls the changelog for a release, so the two always move together.
///        Between releases it names the release being worked towards, and the
///        build hash next to it is what tells one build from another.
#define LAUNCHER_VERSION "1.11.0"

/// @brief Short hash of the commit this was built from, with a "+" after it when
///        the tree had uncommitted changes. Makefile.arm9 passes it in from git
///        and compiles Version.cpp on every build so it is never a build old.
///        Empty when git or the repository was not there at build time.
#ifndef LAUNCHER_BUILD
#define LAUNCHER_BUILD ""
#endif

/// @brief The repository the build came from, as "owner/repo", from the remote
///        the built branch tracks, or the only remote. Empty when git could not say.
#ifndef LAUNCHER_REPO
#define LAUNCHER_REPO ""
#endif

extern const char kLauncherVersion[];
extern const char kLauncherBuild[];
extern const char kLauncherRepo[];

/// @brief Writes "v1.8.0", or "v1.8.0 (f2e5642+)" when asked for the build and
///        there is one, so every screen that names the build spells it the same.
void FormatLauncherVersion(char* dst, unsigned int size, bool withBuild);
