// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include <cstdint>
#include <map>
#include <thread>
#include <semaphore>
#include <functional>
#include <mutex>
#include <bitset>
#include <Core/source/win/Key.h>
#include <Core/source/win/ModSet.h>
#include <CommonUtilities/mt/Thread.h>
#include "../Action.h"

namespace p2c::client::util
{
	class Hotkeys
	{
	public:
		Hotkeys();
        Hotkeys(const Hotkeys&) = delete;
        Hotkeys& operator=(const Hotkeys&) = delete;
		~Hotkeys();
		void BindAction(Action action, win::Key key, win::ModSet mods, std::function<void(bool)> resultCallback);
		void ClearAction(Action action, std::function<void(bool)> resultCallback);
		void SetHandler(std::function<void(Action)> handler);
	private:
		// types
		struct Hotkey
		{
		public:
			Hotkey(win::Key key, win::ModSet mods);
			bool operator<(const Hotkey& rhs) const;
			bool operator==(const Hotkey& rhs) const;
		private:
			win::Key key;
			win::ModSet mods;
		};		
		// functions
		void Kernel_() noexcept;
		void DispatchHotkey_(Action action) const;
		win::ModSet GatherModifiers_() const;
		// data
		std::binary_semaphore startupSemaphore_{ 0 };
		::pmon::util::mt::Thread thread_;
		unsigned long threadId_{};
		void* messageWindowHandle_ = nullptr;
		std::bitset<win::Key::virtualKeyTableSize> pressedKeys_;
		// control access to concurrent memory for key map / handler
		mutable std::mutex mtx_;
		std::function<void(Action)> Handler_;
		std::map<Hotkey, Action> registeredHotkeys_;
	};
}