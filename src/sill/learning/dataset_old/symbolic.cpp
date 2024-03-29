#include <sill/learning/dataset_old/symbolic.hpp>

#include <sill/macros_def.hpp>

namespace sill {

  namespace symbolic {

    parameters load_symbolic_summary_options(std::ifstream& f_in) {
      parameters params;
      std::string line;
      std::istringstream is;
      size_t size;
      getline(f_in, line);
      while (!(line.empty()) && !(f_in.eof())) {
        if (line.compare(0,1,"|") == 0) {
          // ignore description
        } else {
          size_t i = line.find_first_of("=");
          if (i == std::string::npos) {
            std::cerr << "Error in data summary file line: " << line
                      << std::endl;
            assert(false);
          }
          std::string option_name(line.substr(0,i));
          std::string option_value(line.substr(i+1));
          if (option_name == "FORMAT") {
	    is.clear();
	    is.str(option_value);
	    if (!(is >> size))
	      assert(false);
            params.format = size;
          } else if (option_name == "CLASS_VARIABLES") {
            is.clear();
            is.str(option_value);
            params.class_variables.clear();
            while(is >> size)
              params.class_variables.push_back(size);
	  } else if (option_name == "SEPARATOR") {
            params.separator = option_value;
          } else if (option_name == "PREFIX") {
            params.prefix = option_value;
          } else if (option_name == "INDEX_BASE") {
            is.clear();
            is.str(option_value);
            if (!(is >> size))
              assert(false);
            params.index_base = size;
          } else if (option_name == "SKIPLINES") {
            is.clear();
            is.str(option_value);
            if (!(is >> size))
              assert(false);
            params.skiplines = size;
          } else if (option_name == "SKIPCOLS") {
            is.clear();
            is.str(option_value);
            if (!(is >> size))
              assert(false);
            params.skipcols = size;
          } else if (option_name == "WEIGHTED") {
            is.clear();
            is.str(option_value);
            if (!(is >> size))
              assert(false);
            switch(size) {
            case 0:
              params.weighted = false;
              break;
            case 1:
              params.weighted = true;
              break;
            default:
              assert(false);
            }
          } else {
            std::cerr << "Error in data file line: " << line << std::endl;
            assert(false);
          }
        }
        getline(f_in, line);
      }
      return params;
    }

  } // namespace symbolic

  symbolic::parameters
  load_symbolic_summary(const std::string& filename, universe& u) {
    using namespace symbolic;

    std::string data_filename;
    finite_var_vector finite_var_order;
    vector_var_vector vector_var_order;
    std::vector<variable::variable_typenames> var_type_order;
    size_t nrecords;

    // Get directory (which will be the root directory used for resolving the
    // data file name).
    size_t size = filename.find_last_of('/');
    std::string directory = filename.substr(0,size);
    std::ifstream f_in;
    f_in.open(filename.c_str());
    if (!(f_in.good())) {
      throw std::runtime_error("symbolic failed to open summary file: "
                               + filename);
    }

    // Read in summary file.
    // dataset name
    std::string line;
    getline(f_in, line);
    std::string dataset_name(line);
    // number of records
    getline(f_in, line);
    std::istringstream is(line);
    if (!(is >> nrecords))
      assert(false);
    assert(nrecords > 0);
    // number of variables
    size_t nvars = 0;
    getline(f_in, line);
    is.clear();
    is.str(line);
    if (!(is >> nvars))
      assert(false);
    assert(nvars > 0);
    for (size_t i = 0; i < nvars; i++) {
      // get variable info
      std::string name = "";
      getline(f_in, line);
      bool is_vector;
      if (line[0] == 'v') {
        is_vector = true;
        line = line.substr(1);
      } else
        is_vector = false;
      is.clear();
      is.str(line);
      // arity
      if (!(is >> size)) {
        std::cerr << "symbolic dataset summary file unable to read size of "
                  << "variable " << i << " (from 0) in file " << filename
                  << " on line:\n"
                  << line << "\n"
                  << "Does the number of variables given at the start of the "
                  << "summary file (" << nvars
                  << ") match the number listed in the file?" << std::endl;
        assert(false);
      }
      // variable name
      if (!(is >> name) || name.empty()) {
        std::ostringstream os;
        os << i;
        name = os.str();
      }
      if (is_vector) {
        vector_var_order.push_back(u.new_vector_variable(name, size));
        var_type_order.push_back(variable::VECTOR_VARIABLE);
      } else {
        // ignore value names
        finite_var_order.push_back(u.new_finite_variable(name, size));
        var_type_order.push_back(variable::FINITE_VARIABLE);
      }
    }
    // data file name
    getline(f_in, line);
    data_filename = directory + "/" + line;
    // Read in options:
    parameters params(load_symbolic_summary_options(f_in));
    f_in.close();
    params.dataset_name = dataset_name;
    params.data_filename = data_filename;
    params.datasource_info.finite_seq = finite_var_order;
    params.datasource_info.vector_seq = vector_var_order;
    params.datasource_info.var_type_order = var_type_order;
    // Set class variables
    size_t f_i(0);
    size_t v_i(0);
    std::set<size_t> class_var_set(params.class_variables.begin(),
                                   params.class_variables.end());
    for (size_t j(0); j < var_type_order.size(); ++j) {
      switch(var_type_order[j]) {
      case variable::FINITE_VARIABLE:
        if (class_var_set.count(j) != 0)
          params.datasource_info.finite_class_vars.push_back
            (finite_var_order[f_i]);
        ++f_i;
        break;
      case variable::VECTOR_VARIABLE:
        if (class_var_set.count(j) != 0)
          params.datasource_info.vector_class_vars.push_back
            (vector_var_order[v_i]);
        ++v_i;
        break;
      default:
        assert(false);
      }
    }
    assert(params.datasource_info.finite_class_vars.size()
           + params.datasource_info.vector_class_vars.size()
           == params.class_variables.size());
    params.nrecords = nrecords;
    return params;
  }

  symbolic::parameters
  load_symbolic_summary(const std::string& filename,
                        const datasource_info_type& info,
                        bool check_class_vars) {

    using namespace symbolic;

    std::string data_filename;
    size_t nrecords;

    // Get directory (which will be the root directory used for resolving the
    // data file name).
    size_t size = filename.find_last_of('/');
    std::string directory = filename.substr(0,size);
    std::ifstream f_in;
    f_in.open(filename.c_str());
    if (!(f_in.good())) {
      std::cerr << "symbolic failed to open summary file: " << filename
                << std::endl;
      assert(false);
    }

    // Read in summary file.
    // dataset name
    std::string line;
    getline(f_in, line);
    std::string dataset_name(line);
    // number of records
    getline(f_in, line);
    std::istringstream is(line);
    if (!(is >> nrecords))
      assert(false);
    assert(nrecords > 0);
    // number of variables
    size_t nvars = 0;
    getline(f_in, line);
    is.clear();
    is.str(line);
    if (!(is >> nvars))
      assert(false);
    assert(nvars > 0);
    size_t nf = 0, nv = 0;
    for (size_t i = 0; i < nvars; i++) {
      // get variable info
      std::string name = "";
      getline(f_in, line);
      bool is_vector;
      if (line[0] == 'v') {
        is_vector = true;
        line = line.substr(1);
      } else
        is_vector = false;
      is.clear();
      is.str(line);
      // arity
      if (!(is >> size))
        assert(false);
      // ignore variable name
      if (is_vector) {
        assert(info.var_type_order[i] == variable::VECTOR_VARIABLE);
        assert(info.vector_seq.size() > nv);
        assert(info.vector_seq[nv]->size() == size);
        ++nv;
      } else {
        assert(info.var_type_order[i] == variable::FINITE_VARIABLE);
        assert(info.finite_seq.size() > nf);
        if (info.finite_seq[nf]->size() != size) {
          std::cerr << "Error in load_symbolic_summary(): expected finite"
                    << " variable of size " << info.finite_seq[nf]->size()
                    << " but found one of size " << size << std::endl;
          assert(false);
        }
        ++nf;
      }
    }
    // data file name
    getline(f_in, line);
    data_filename = directory + "/" + line;
    // Read in options:
    parameters params(load_symbolic_summary_options(f_in));
    f_in.close();
    params.dataset_name = dataset_name;
    params.data_filename = data_filename;
    params.datasource_info.finite_seq = info.finite_seq;
    params.datasource_info.vector_seq = info.vector_seq;
    params.datasource_info.var_type_order = info.var_type_order;
    params.nrecords = nrecords;
    // Set class variables
    size_t f_i(0);
    size_t v_i(0);
    std::set<size_t> class_var_set(params.class_variables.begin(),
                                   params.class_variables.end());
    for (size_t j(0); j < info.var_type_order.size(); ++j) {
      switch(info.var_type_order[j]) {
      case variable::FINITE_VARIABLE:
        if (class_var_set.count(j) != 0)
          params.datasource_info.finite_class_vars.push_back
            (info.finite_seq[f_i]);
        ++f_i;
        break;
      case variable::VECTOR_VARIABLE:
        if (class_var_set.count(j) != 0)
          params.datasource_info.vector_class_vars.push_back
            (info.vector_seq[v_i]);
        ++v_i;
        break;
      default:
        assert(false);
      }
    }
    assert(params.datasource_info.finite_class_vars.size()
           + params.datasource_info.vector_class_vars.size()
           == params.class_variables.size());
    // (Optionally) check class variables
    if (check_class_vars) {
      assert(params.datasource_info.finite_class_vars ==
             info.finite_class_vars);
      assert(params.datasource_info.vector_class_vars ==
             info.vector_class_vars);
    }

    return params;
  }

} // namespace sill

#include <sill/macros_undef.hpp>
