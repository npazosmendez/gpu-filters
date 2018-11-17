/* Progress Bar for c++ loops with internal running variable
 *
 * Author: Luigi Pertoldi (changed)
 * Created: 3 dic 2016
 *
 * Notes: The bar must be used when there's no other possible source of output
 *        inside the for loop
 *
 *
 */

#ifndef _PROGRESS_BAR_H_
#define _PROGRESS_BAR_H_

class ProgressBar {

public:
	// default destructor
	~ProgressBar()                             = default;

	// delete everything else
	ProgressBar           (ProgressBar const&) = delete;
	ProgressBar& operator=(ProgressBar const&) = delete;
	ProgressBar           (ProgressBar&&)      = delete;
	ProgressBar& operator=(ProgressBar&&)      = delete;

	// default constructor, must call SetNIter later

	ProgressBar(int n) :
			nCycles(n),
			printed(false) {
		Reset();
	}

	// reset bar to use it again
	void Reset() {
		i = 0,
		printed = false;
		savedPerc = 0;
		bar[0] = '[';
		for(int index = 0; index < 100/2; index++) bar[index+1] = '-';
		bar[51] = ']';
		bar[52] = ' ';
		bar[53] = '0';
		bar[54] = '%';
		bar[55] = ' ';
		bar[56] = ' ';
		return;
	}

	// main function
	void Update() {

			// compute percentage
			int perc = i*100. / (nCycles-1);
			++i;

			if ( perc == savedPerc ) return;

			for(int index = 0; index < perc/2; index++) bar[index+1] = '#';
			for(int index = perc/2; index < 100/2; index++) bar[index+1] = '-';

			char centena = (perc / 100)%10 + 48;
			char decena = (perc / 10)%10 + 48;
			char unidad = (perc / 1)%10 + 48;
			if(centena == '0'){
				centena = ' ';
				if(decena == '0') decena = ' ';
			}
			bar[53] = centena;
			bar[54] = decena;
			bar[55] = unidad;
			bar[56] = '%';
			savedPerc = perc;

			if (printed){
				for(int index = 0; index < barLenght; index++) std::cout << '\b';
			}
			Print();
			return;
	}

	void Print(){
		for(int index = 0; index < barLenght; index++) std::cout << bar[index];
		std::cout << std::flush;
		printed = true;
	}

private:
	// internal counter
	int i;
	// total number of iterations
	int nCycles;
	int savedPerc;

	// track first call of Update()
	bool printed;

	const static int barLenght = 2 + 50 + 3 + 2;
	char bar[barLenght];
};

#endif
