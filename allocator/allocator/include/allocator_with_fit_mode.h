#ifndef MATH_PRACTICE_AND_OPERATING_SYSTEMS_ALLOCATOR_WITH_FIT_MODE_H
#define MATH_PRACTICE_AND_OPERATING_SYSTEMS_ALLOCATOR_WITH_FIT_MODE_H

#include "allocator.h"

class allocator_with_fit_mode:
    public allocator
{

public:
    
    enum class fit_mode
    {
        first_fit,
        the_best_fit,
        the_worst_fit
    };

public:
<<<<<<< HEAD

    inline virtual void set_fit_mode(fit_mode mode) = 0;

=======
    
    inline virtual void set_fit_mode(
        fit_mode mode) = 0;
    
>>>>>>> 5ade9435e0702eaa7d8713a809c05debdb627456
};

#endif //MATH_PRACTICE_AND_OPERATING_SYSTEMS_ALLOCATOR_WITH_FIT_MODE_H