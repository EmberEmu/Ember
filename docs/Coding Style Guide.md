
# 🔥 **Ember Style Guide**
---
This guide aims to provide advice on both the formatting of code contributions as well as language usage rules of thumb to keep in mind.

## Sections:
- [Formatting](#formatting)
- [Naming Conventions](#naming-conventions)
- [Language Usage](#language-usage)

## Formatting
Code formatting is highly subjective and the rules presented are not claimed as being superior to alternatives but a unified style is important for code readability. A `clang-format` file is included in the project directory to help maintain a unified style.

### Tabs vs. Spaces
Use tabs for indenting and spaces for alignment. Never mix the two on a single line.

For example:
```cpp
void foo(const Player& source, const Monster& target, const Loot& drops,
         const Spell& cast) {
	source.bar();
	...
}
```

Had tabs and spaces been mixed, correct alignment on the second line would have been reliant upon tabs being set to a fixed width.

### Line Length & Wrapping
Try to stick to lines shorter than 100 characters and no longer than 120. Wrap long lines so arguments line up to the beginning of the argument list on the previous line.

For example:
```cpp
std::bind(&UDPServer::packet_sent, this, std::placeholders::_1, std::placeholders::_2,
          std::placeholders::_3, std::placeholders::_4, packet);
```

### Braces & Indenting
A minor variant of [1TBS](https://en.wikipedia.org/wiki/Indent_style#Variant:_1TBS) is followed where opening braces are never placed on their own line. Note that braces are always added, even when only a single statement makes up the body.

For example:
```cpp
void foo(int bar) {
	if(bar > 10) {
		std::cout << "Greater than 10\n";
	} else {
		std::cout << "Less than or equal to 10\n";
	}
    
	for(int i = 0; i < bar; ++i) {
		std::cout << i << "\n";
	}

	do {
		--bar;
	} while(bar > 0);
}
```

### Namespaces
Namespace bodies should not be indented and a blank line should be added after/before the opening/closing braces respectively. 

For example:
```cpp
namespace foo {

double calculate_area(double radius);
double calculate_pi(int places);

} // foo
```

### Class Definitions
```cpp
class A : public B {
public:
	A(int seed) : gen(seed) {};
	~A();
	int next_random();

private:
	const Generator gen;
	void do_work();
};
```

Observations:

- Place a space before and after colons.
- The access specifiers should be on the same level of indentation as the class name.
- Place the public section before the private section.

### Comments
Use `//` for single line comments and `/* ... */` for multi-line comments.

For example:
```cpp
/*
 * This is a multi-line comment.
 * The function is a placeholder.
 */
void bar(int foo) {
	int res = baz(foo);
    // to do, implement
}
```

### Switch/Case
Indent each case and double-indent each statement.

For example:
```cpp
switch(score) {
	case 1:
		std::cout << "Try harder next time.";
		metrics::player_failed();
		break;
	case 2:
		std::cout << "Could be worse, could be better.";
		break;
	...
}
```

### Argument Ordering
Order inputs first, followed by outputs.
```cpp
void reticulate_splines(const Spline& source, char** destination);
```

### Variable Scope

Minimise the scope of variables as much as possible and initialise them at the point of declaration.

For an example of what ***not*** to do:
```cpp
int overheal; // scope could be reduced, declare & initialise on same line
player.set_health(player.health() + healing);

if(player.health() >= MAX_HEALTH) {
	overheal = player.health() - MAX_HEALTH;
	player.set_health(MAX_HEALTH);
	events.log(healer, Combat::OVERHEAL, overheal);
}       
```

### Miscellaneous
When in doubt have a look for formatting examples in the codebase and follow their lead.

## Naming Conventions

### Functions
All lower-case with words separated by underscores. For example:
```cpp
void send_message(string message);
```
### Constants
All upper-case with words separated by underscores. For example:
```cpp
constexpr int MAX_HEALTH = 100;

enum class Class {
	NECROMANCER, PALADIN, SAGE, TAOIST
};
```
### Variables
All lower-case with words separated by underscores. For example:
```cpp
int low_mana_threshold = (MAX_MANA / 2) + player.regen_rate();
```
### User-defined Types
Use CamelCase. For example:
```cpp
class PlayerStats;
struct ArmourSlots;
```

### Namespaces
All lower-case with words separated by underscores. For example:
```cpp
namespace network_comms {

...

} // network_comms
```

## Language Usage
This section of the guide is not intended to be a tutorial or a technical reference but rather a quick overview of preferred usage of some language features.

### Header Guards
Use the `#pragma once` preprocessor directive over the more traditional `#ifndef`.

### Header Inclusion Order
1. Corresponding .h file(s).
2. Related project headers.
3. Ember library headers.
3. Third-party library headers.
4. Standard library headers.
5. System headers.

To generalise, go from local to global. This ordering can help prevent accidentally hiding/dependencies between header files.

For an example Player.cpp:
```cpp
#include "Player.h"                       // corresponding .h file
#include "PlayerManager.h"                // related project header
#include <logging/Logger.h>               // Ember library header
#include <boost/thread/shared_mutex.hpp>  // third-party library header
#include <memory>                         // standard library header
#include <cstdint>                        // standard library header
#include <Windows.h>                      // system header
```

### Prefer `enum class` over `enum`
```cpp
enum TabardColours { RED, BLUE };
enum ArmourColours { BLUE, PURPLE, MAUVE };

void recolour_armour(ArmourColours colour) {
	switch(colour) {
		case RED: // mistake but compiles O.K.
			...
	}
}

recolour_armour(BLUE); // error: redefinition of BLUE
```

```cpp
enum class TabardColours { RED, BLUE };
enum class ArmourColours { BLUE, PURPLE, MAUVE };

void recolour_armour(ArmourColours colour) {
	switch(colour) {
		case RED: // mistake, caught by compiler
			...
	}
}

recolour_armour(ArmourColours::BLUE); // no problem, no double definition
```

### Argument Passing
As a general rule, pass built-in types by value and user-defined types by const reference.

```cpp
int calculate_damage(const& Actor actor, int damage) {
	int armour_rating = actor.armour();
	
    if(armour_rating < 100) {
    	damage *= 2;
    }
    
    ...
    return damage;
    
}

calculate_damage(player, damage);
```

### Helper Functions
Consider placing helper functions outside of class definitions as free-standing functions if they do not need access to implementation details.
```cpp
class Player { ... };

bool teleport_to(const Player& target, const Player& teleportee) {
	if(teleportee.zone() == target.zone()) {
		...
	}
	...
}
```

### Exceptions
Avoid throwing exceptions in performance-critical code other than to indicate a serious or fatal error.  Outside of performance-critical code, exceptions may be used less sparingly but consider alternatives first.

Exceptions should never be used as flow control to deal with invalid player input (e.g. malformed packets from clients).

### Avoid 'naked' `new`/`delete`
Always opt to use smart pointers (unique_ptr, shared_ptr) over new/delete.

For example:
```cpp
void recombobulate(const Player& player) {
	RecombobulatedPlayer* rp = new RecombobulatedPlayer(player);
	events.log(Events::RECOMBOBULATED, player);
	// exception thrown by .log, rp leaks
	...
	delete rp;
}	
```

In constrast:
```cpp
void recombobulate(const Player& player) {
	std::unique_ptr<RecombobulatedPlayer> rp(std::make_unique<RecombobulatedPlayer>(player));
	events.log(Events::RECOMBOBULATED, player);
	// exception thrown by .log, rp does not leak
	...
}	
```

### Default to `unique_ptr` over `shared_ptr`