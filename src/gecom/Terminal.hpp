
#ifndef GECOM_TERMINAL_HPP
#define GECOM_TERMINAL_HPP

#include <iostream>

namespace gecom {

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

	}

}

#endif // GECOM_TERMINAL_HPP
