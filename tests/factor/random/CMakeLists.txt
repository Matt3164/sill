# TODO: discrete_factor_generator

add_executable(alternating_generator alternating_generator.cpp)
add_executable(associative_factor_generator associative_factor_generator.cpp)
add_executable(ising_factor_generator ising_factor_generator.cpp)
add_executable(mixture_generator mixture_generator.cpp)
add_executable(moment_gaussian_generator moment_gaussian_generator.cpp)
add_executable(uniform_factor_generator uniform_factor_generator.cpp)

add_test(alternating_generator alternating_generator)
add_test(associative_factor_generator associative_factor_generator)
add_test(ising_factor_generator ising_factor_generator)
add_test(mixture_generator mixture_generator)
add_test(moment_gaussian_generator moment_gaussian_generator)
add_test(uniform_factor_generator uniform_factor_generator)
