#pragma once
/*
 * @Snail@ - Code Execution Footprint Tracer & Explorer.
 *
 * Copyright (C) 2018 Haoran Luo
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * @file snailviewer/uid.hpp
 * @author Haoran Luo
 * @brief Unique identity definition for the snail viewer.
 *
 * This file provides facilities for uniquely identifying a component
 * from some modules, either built from the snail or some user provided
 * and loadable modules, while won't wish to consume up too much spaces.
 */
#include <type_traits>
#include <functional>

namespace snailviewer {

/**
 * @brief Definition of different component that would like to be 
 * uniquelly identified.
 */
enum class uidType : char {
	/// The identified component is a module (including the program).
	/// The name field of a module should also be nil. So that the
	/// lower module plus author bits can completely identifies the
	/// modules.
	module = 0,

	/// The identified component is an event type.
	event,

	/// The identified component is a widget (basic UI component).
	widget,

	/// The identified component is a key binding.
	keybind,

	/// The identified component is a color component.
	color,
};

/**
 * @brief Definition of an unique identify.
 *
 * With this facility, it is most likely to distinguish different 
 * components from one another, and without consuming up too much space.
 */
struct uid {
	/**
	 * @brief The name of the module which it is from.
	 *
	 * For example, the snailviewer itself can use {'S', 'N', 'A', 
	 * 'I', 'L'} to identify the module.
	 */
	char module[5];

	/**
	 * @brief The abbreviated name of the author.
	 *
	 * For example, the FirstName M LastName can be abbreviated to 
	 * {'F', 'M', 'L'}. It is used to avoid collision.
	 */
	char author[3];

	/**
	 * @brief The type which the uid is identifying.
	 */
	uidType type;

	/**
	 * @brief The abbreviated name of the identified object.
	 *
	 * For example, the event of update of current footprint index
	 * can be abbreviated to { 'F', 'P', 'N', 'T', 'I', 'D', 'X' }.
	 */
	char name[7];

	/// Compare the less relationship of two unique ids.
	bool operator<(const uid& opponent) const noexcept {
		const long* a = reinterpret_cast<const long*>(this);
		const long* b = reinterpret_cast<const long*>(&opponent);
		if(a[0] == b[0]) return a[1] < b[1];
		else return a[0] < b[0];
	}

	/// Compare the equality relationship of two unique ids.
	bool operator==(const uid& opponent) const noexcept {
		const long* a = reinterpret_cast<const long*>(this);
		const long* b = reinterpret_cast<const long*>(&opponent);
		return a[0] == b[0] && a[1] == b[1];
	}

	/// Judge whether the id has some type.
	bool hasType(uidType expectedType) const noexcept {
		return type == expectedType;
	}
};

// Ensure the behavior of unique identity is the same as expected.
static_assert(sizeof(uid) == 16, "The size of the uid must be 16 bytes.");
static_assert(	std::is_pod<uid>::value &&
		std::is_trivially_constructible<uid>::value,
		"The uid must be a plain old data and can be trivially constructed.");

// The uid representing nulls of some specific types.
constexpr uid null(uidType type) {
	return uid {{0, 0, 0, 0, 0}, {0, 0, 0}, type, {0, 0, 0, 0, 0, 0, 0}};
}

} // namespace snailviewer.

namespace std {

template<> struct hash<snailviewer::uid> {
	size_t operator()(const snailviewer::uid& id) const noexcept {
		const long* v = reinterpret_cast<const long*>(&id);
		return (size_t)(v[0] ^ v[1]);
	}
};

} // namespace std.
