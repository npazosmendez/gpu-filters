/* 
	C++ ProgressBar
	~~~~~~~~~~~~~~~

	* Author: Nicolás Pazos-Méndez
	* Created: nov 2018

	* Notes: for this bar to work properly, it should be the only source of output
	during its usage.

*/

#ifndef _PROGRESS_BAR_H_
#define _PROGRESS_BAR_H_

#include <iostream>

#define ASCIIDIGIT(digit) (digit + 48)

class ProgressBar {

public:

	ProgressBar(int total_iterations) : cycles(total_iterations) {
		reset();
	}

	void update(int iterations = 1) {
		// You should call this on every iteration
		if(!printed) print();

		iteration += iterations;
		int percentage = iteration*100.0 / cycles;

		if(percentage == last_percentage || percentage > 100)
			return;

		last_percentage = percentage;

		for(int i = 0; i < percentage*bar_lenght/100; i++)
			bar[i+1] = completed_symbol;

		char hundred	=	ASCIIDIGIT((percentage / 100) % 10);
		char ten		=	ASCIIDIGIT((percentage / 10) % 10);
		char one		=	ASCIIDIGIT(percentage % 10);

		int percentage_index = bar_lenght + 3;
		bar[percentage_index++] = hundred != '0' ? hundred : ' ';
		bar[percentage_index++] = hundred != '0' || ten != '0' ? ten : ' ';
		bar[percentage_index++] = one;


		if (printed)
			for(int i = 0; i < total_length; i++) std::cout << '\b';

		print();

	}

	void print(){
		// Prints the bar. No need to call it explicitly unless you want to force it before any iterations
		for(int i = 0; i < total_length; i++) std::cout << bar[i];
		std::cout << std::flush;
		printed = true;
	}

	void finish(){
		// Breakline and reset the bar to be used again
		std::cout << std::endl;
		reset();
	}

	void reset() {
		// You probably want to call 'finish' and not this
		printed = false;
		iteration = 0;
		last_percentage = 0;

		int i = 0;
		bar[i++] = '[';
		for(int j = 0; j < bar_lenght; j++) bar[i++] = remaining_symbol;
		bar[i++] = ']';
		bar[i++] = ' ';
		bar[i++] = ' ';
		bar[i++] = ' ';
		bar[i++] = '0';
		bar[i++] = '%';
	}

private:
	// Can customize
	const static int bar_lenght = 50;
	const static char completed_symbol = '#';
	const static char remaining_symbol = '-';

	// Total lenght is '[' + bar_lenght + ']' + ' ' + percentage + '%' 
	const static int total_length = 1 + bar_lenght + 1 + 1 + 3 + 1;
	char bar[total_length];
	int iteration;
	int cycles;
	int last_percentage;
	bool printed;
};

#endif
