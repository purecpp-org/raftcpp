#include "agent_group.h"
#include "combiner.h"
#include "variable.h"
#include "reducer.h"         
#include <iostream>
int main()
{

	bvar::Adder<int> value;
    value << 1 << 2 << 3 << -4;
    //CHECK_EQ(2, value.get_value());

    std::cout << "value = " << value.get_value() << std::endl;

    //bvar::Adder<double> fp_value;  // ¿ÉÄÜÓĞwarning
    //fp_value << 1.0 << 2.0 << 3.0 << -4.0;

	return 0;
}