//----------------------------  trilinos_20.cc  ---------------------------
//    $Id$
//    Version: $Name$ 
//
//    Copyright (C) 2004, 2005 by the deal.II authors
//
//    This file is subject to QPL and may not be  distributed
//    without copyright and license information. Please refer
//    to the file deal.II/doc/license.html for the  text  and
//    further information on this license.
//
//----------------------------  trilinos_20.cc  ---------------------------


// check TrilinosWrappers::Vector::operator *=

#include "../tests.h"
#include <lac/trilinos_vector.h>    
#include <fstream>
#include <iostream>
#include <vector>


void test (TrilinosWrappers::Vector &v)
{
                                   // set only certain elements of the
                                   // vector. have a bit pattern of where we
                                   // actually wrote elements to
  std::vector<bool> pattern (v.size(), false);
  for (unsigned int i=0; i<v.size(); i+=1+i)
    {
      v(i) = i;
      pattern[i] = true;
    }
  v.compress ();

                                   // multiply v with 5/4
  v *= 5./4.;

                                   // check that the entries are ok
  for (unsigned int i=0; i<v.size(); ++i)
    Assert (((pattern[i] == true) && (v(i) == i*5./4.)
             ||
             (pattern[i] == false) && (v(i) == 0)),
             ExcInternalError());

  deallog << "OK" << std::endl;
}



int main (int argc,char **argv) 
{
  std::ofstream logfile("20/output");
  deallog.attach(logfile);
  deallog.depth_console(0);
  deallog.threshold_double(1.e-10);

  try
    {
      {
        TrilinosWrappers::Vector v (100);
        test (v);
      }
    }
  catch (std::exception &exc)
    {
      std::cerr << std::endl << std::endl
		<< "----------------------------------------------------"
		<< std::endl;
      std::cerr << "Exception on processing: " << std::endl
		<< exc.what() << std::endl
		<< "Aborting!" << std::endl
		<< "----------------------------------------------------"
		<< std::endl;
      
      return 1;
    }
  catch (...) 
    {
      std::cerr << std::endl << std::endl
		<< "----------------------------------------------------"
		<< std::endl;
      std::cerr << "Unknown exception!" << std::endl
		<< "Aborting!" << std::endl
		<< "----------------------------------------------------"
		<< std::endl;
      return 1;
    };
}
