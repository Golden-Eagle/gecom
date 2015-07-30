
#ifndef GECOM_TERMINAL_HPP
#define GECOM_TERMINAL_HPP

#include <iostream>
#include <utility>

namespace gecom {

	namespace terminal {

		// color off
		std::ostream & colorOff(std::ostream &);

		// regular colors
		std::ostream & black(std::ostream &);
		std::ostream & red(std::ostream &);
		std::ostream & green(std::ostream &);
		std::ostream & yellow(std::ostream &);
		std::ostream & blue(std::ostream &);
		std::ostream & purple(std::ostream &);
		std::ostream & cyan(std::ostream &);
		std::ostream & white(std::ostream &);

		// bold colors
		std::ostream & boldBlack(std::ostream &);
		std::ostream & boldRed(std::ostream &);
		std::ostream & boldGreen(std::ostream &);
		std::ostream & boldYellow(std::ostream &);
		std::ostream & boldBlue(std::ostream &);
		std::ostream & boldPurple(std::ostream &);
		std::ostream & boldCyan(std::ostream &);
		std::ostream & boldWhite(std::ostream &);

		// stdout/stderr streambufs with color support
		// these interpret or forward ANSI escape codes when appropriate
		std::streambuf * stdoutBuf();
		std::streambuf * stderrBuf();
		std::wstreambuf * wstdoutBuf();
		std::wstreambuf * wstderrBuf();

		// (w,h) of visible area
		std::pair<int, int> screenSize(const std::streambuf *);
		std::pair<int, int> screenSize(const std::wstreambuf *);

	}

}

#endif // GECOM_TERMINAL_HPP
