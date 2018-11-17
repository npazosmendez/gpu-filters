/* Progress Bar for c++ loops with internal running variable
 *
 * Author: Luigi Pertoldi
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
      ProgressBar() :
          i(0),
          savedPerc(0),
          style('#'),
          showBar(true),
          updateIsCalled(false),
          setNIterIsCalled(true) {}
      ProgressBar(int n, char opt = '#', bool showbar = true) :
          i(0),
          nCycles(n),
          savedPerc(0),
          style(opt),
          showBar(showbar),
          updateIsCalled(false),
          setNIterIsCalled(true) {}

      // reset bar to use it again
      void Reset() {
          i = 0,
          updateIsCalled = false;
          savedPerc = 0;
          return;
      }
     // set number of loop iterations
      void SetNIter(int iter) {
          nCycles = iter;
          setNIterIsCalled = true;
          return;
      }
      // chose between '#' or '->' style
      void SetStyle(char opt) { style = opt; }
      // to show only the percentage
      void ShowBar(bool flag) { showBar = flag; }
      // main function
      void Update() {
          if (!setNIterIsCalled) { std::cerr << "ProgressBar: number of cycles not set!\n"; return; }

          if (!updateIsCalled) {
              if ( showBar == true ) {
                  std::cout << "[--------------------------------------------------] 0" << "%" << std::flush; // 50 '-'
              }
              else std::cout << "0%" << std::flush;
          }
          updateIsCalled = true;

          int perc = 0;

          // compute percentage
          perc = i*100. / (nCycles-1);
          if ( perc < savedPerc ) return;

          // update percentage each unit
          if (perc == savedPerc + 1) {
              // erase the correct  number of characters
              if (perc  < 10)               std::cout << "\b\b"   << perc << "%";
              if (perc == 10)               std::cout << "\b\b"   << perc << "%";
              if (perc  > 10 && perc < 100) std::cout << "\b\b\b" << perc << "%";
              if (perc == 100)              std::cout << "\b\b\b" << perc << "%";
          }
          if ( showBar == true ) {
              // update bar every ten units
              if (perc % 2 == 0) {
                  // erase trailing percentage characters
                  if (perc  < 10)               std::cout << "\b\b\b\b";
                  if (perc == 10)               std::cout << "\b\b\b\b\b";
                  if (perc  > 10 && perc < 100) std::cout << "\b\b\b\b\b";
                  if (perc == 100)              std::cout << "\b\b\b\b\b\b";

                  // erase "-"
                  for (int j = 0; j < 50 - (perc-1) / 2; ++j) std::cout << "\b";

                  if ( style == '#' ) {
                      // add one additional "#"
                      if (perc == 0) std::cout << "-" << std::flush;
                      else           std::cout << "#" << std::flush;
                  }

                  else if ( style == '>' ) {
                      // shift '>' to right
                      if (perc == 0) std::cout << ">" << std::flush;
                      else if (perc == 2) std::cout << "\b\b[->" << std::flush;
                      else                std::cout << "\b->"    << std::flush;
                  }

                  // refill with "-"
                  for (int j = 0; j < 50-(perc-1)/2-1; ++j) std::cout << "-";

                  // readd trailing percentage characters
                  std::cout << "] " << perc << "%";
              }
          }
          savedPerc = perc;
          std::cout << std::flush;

          ++i;
          return;
      }


    private:
      // internal counter
      int i;
      // total number of iterations
      int nCycles;
      int savedPerc;

      char style;
      bool showBar;
      // track first call of Update()
      bool updateIsCalled;
      bool setNIterIsCalled;
};

#endif
