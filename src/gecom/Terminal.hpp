
/*
 * GECOM Terminal Utilities Header
 *
 * Allows use of ANSI terminal escape sequences to manipulate color in
 * a portable manner. Redirects stdio to a thread and then forwards or
 * interprets escape sequences if connected to a terminal, suppressing
 * them otherwise. If no redirection mechanism is available, the iostream
 * manipulators in this header do nothing.
 * 
 * Including this header ensures that terminal stdio redirection is correctly
 * initialized before any code that comes after the inclusion.
 *
 * TODO
 * - POSIX stdio redirection
 */

#ifndef GECOM_TERMINAL_HPP
#define GECOM_TERMINAL_HPP

#include <cstdio>
#include <iostream>

namespace gecom {

	class TerminalInit {
	private:
		static size_t refcount;
	public:
		TerminalInit();
		~TerminalInit();
	};

	namespace terminal {

		// reset all attributes
		std::ostream & reset(std::ostream &);

		// reset, then apply regular foreground colors
		std::ostream & black(std::ostream &);
		std::ostream & red(std::ostream &);
		std::ostream & green(std::ostream &);
		std::ostream & yellow(std::ostream &);
		std::ostream & blue(std::ostream &);
		std::ostream & magenta(std::ostream &);
		std::ostream & cyan(std::ostream &);
		std::ostream & white(std::ostream &);

		// reset, then apply bold foreground colors
		std::ostream & boldBlack(std::ostream &);
		std::ostream & boldRed(std::ostream &);
		std::ostream & boldGreen(std::ostream &);
		std::ostream & boldYellow(std::ostream &);
		std::ostream & boldBlue(std::ostream &);
		std::ostream & boldMagenta(std::ostream &);
		std::ostream & boldCyan(std::ostream &);
		std::ostream & boldWhite(std::ostream &);

		// apply regular background colors without reset
		std::ostream & onBlack(std::ostream &);
		std::ostream & onRed(std::ostream &);
		std::ostream & onGreen(std::ostream &);
		std::ostream & onYellow(std::ostream &);
		std::ostream & onBlue(std::ostream &);
		std::ostream & onMagenta(std::ostream &);
		std::ostream & onCyan(std::ostream &);
		std::ostream & onWhite(std::ostream &);

		// width in characters of the terminal behind a FILE pointer
		// if no terminal, returns INT_MAX
		int width(FILE *);

	}

}

namespace {
	gecom::TerminalInit terminal_init_obj;
}

#endif // GECOM_TERMINAL_HPP
