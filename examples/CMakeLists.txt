project(examples)
link_libraries(sill ${Boost_LIBRARIES})
subdirs(filter graph grid_motion ipf loopy_bp naive_bayes markov_chain)
