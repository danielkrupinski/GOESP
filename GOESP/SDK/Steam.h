#pragma once

#include "VirtualMethod.h"

class SteamFriends {
public:
	VIRTUAL_METHOD(int, getSmallFriendAvatar, 34, (std::uint64_t steamID), (this, steamID))
};

struct SteamAPIContext {
	void* steamClient;
	void* steamUser;
	SteamFriends* steamFriends;
	// SteamUtils* steamUtils;
};
