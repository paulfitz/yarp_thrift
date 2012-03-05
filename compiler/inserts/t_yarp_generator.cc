/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements. See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership. The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied. See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include <string>
#include <fstream>
#include <iostream>
#include <vector>
#include <map>

#include <stdlib.h>
#include <sys/stat.h>
#include <sstream>
#include "t_generator.h"
#include "t_oop_generator.h"
#include "platform.h"
using namespace std;


/**
 * YARP code generator
 *
 * Serialization:
 *   Bottle compatible, why not.
 *   BOTTLE_CODE
 *   LENGTH
 *
 * mostly copy/pasting/tweaking from mcslee's work.
 */
class t_yarp_generator : public t_oop_generator {
 public:
  t_yarp_generator(
      t_program* program,
      const std::map<std::string, std::string>& parsed_options,
      const std::string& option_string)
    : t_oop_generator(program)
  {
    (void) parsed_options;
    (void) option_string;  
    out_dir_base_ = "gen-yarp";
    gen_pure_enums_ = true;
  }

  void generate_program();
  void generate_program_toc();
  void generate_program_toc_row(t_program* tprog);
  void generate_program_toc_rows(t_program* tprog,
         std::vector<t_program*>& finished);
  void generate_index();

  /**
   * Program-level generation functions
   */

  void generate_typedef (t_typedef*  ttypedef);
  void generate_enum    (t_enum*     tenum);
  void generate_const   (t_const*    tconst);
  void generate_struct  (t_struct*   tstruct);
  void generate_service (t_service*  tservice);
  void generate_xception(t_struct*   txception);

  void print_doc        (t_doc* tdoc);
  std::string print_type       (t_type* ttype);
  std::string print_const_value(t_const_value* tvalue);

  std::string function_prototype(t_function *tfn);

  std::string type_name(t_type* ttype, bool in_typedef=false, bool arg=false);
  std::string base_type_name(t_base_type::t_base tbase);
  std::string namespace_prefix(std::string ns);
  std::string namespace_open(std::string ns);
  std::string namespace_close(std::string ns);
  bool is_complex_type(t_type* ttype) {
    ttype = get_true_type(ttype);
    return
      ttype->is_container() ||
      ttype->is_struct() ||
      ttype->is_xception() ||
      (ttype->is_base_type() && (((t_base_type*)ttype)->get_base() == t_base_type::TYPE_STRING));
  }

  std::ofstream f_out_;
  bool gen_pure_enums_;
};





/////////////////////////////////////////////////////////////////////
/// C++ generator code begins
/////////////////////////////////////////////////////////////////////

string t_yarp_generator::type_name(t_type* ttype, bool in_typedef, bool arg) {
  if (ttype->is_base_type()) {
    string bname = base_type_name(((t_base_type*)ttype)->get_base());
    if (!arg) {
      return bname;
    }

    if (((t_base_type*)ttype)->get_base() == t_base_type::TYPE_STRING) {
      return "const " + bname + "&";
    } else {
      return "const " + bname;
    }
  }

  // Check for a custom overloaded C++ name
  if (ttype->is_container()) {
    string cname;

    t_container* tcontainer = (t_container*) ttype;
    if (tcontainer->has_cpp_name()) {
      cname = tcontainer->get_cpp_name();
    } else if (ttype->is_map()) {
      t_map* tmap = (t_map*) ttype;
      cname = "std::map<" +
        type_name(tmap->get_key_type(), in_typedef) + ", " +
        type_name(tmap->get_val_type(), in_typedef) + "> ";
    } else if (ttype->is_set()) {
      t_set* tset = (t_set*) ttype;
      cname = "std::set<" + type_name(tset->get_elem_type(), in_typedef) + "> ";
    } else if (ttype->is_list()) {
      t_list* tlist = (t_list*) ttype;
      cname = "std::vector<" + type_name(tlist->get_elem_type(), in_typedef) + "> ";
    }

    if (arg) {
      return "const " + cname + "&";
    } else {
      return cname;
    }
  }

  string class_prefix;
  if (in_typedef && (ttype->is_struct() || ttype->is_xception())) {
    class_prefix = "class ";
  }

  // Check if it needs to be namespaced
  string pname;
  t_program* program = ttype->get_program();
  if (program != NULL && program != program_) {
    pname =
      class_prefix +
      namespace_prefix(program->get_namespace("cpp")) +
      ttype->get_name();
  } else {
    pname = class_prefix + ttype->get_name();
  }

  if (ttype->is_enum() && !gen_pure_enums_) {
    pname += "::type";
  }

  if (arg) {
    if (is_complex_type(ttype)) {
      return "const " + pname + "&";
    } else {
      return "const " + pname;
    }
  } else {
    return pname;
  }
}

string t_yarp_generator::base_type_name(t_base_type::t_base tbase) {
  switch (tbase) {
  case t_base_type::TYPE_VOID:
    return "void";
  case t_base_type::TYPE_STRING:
    return "std::string";
  case t_base_type::TYPE_BOOL:
    return "bool";
  case t_base_type::TYPE_BYTE:
    return "int8_t";
  case t_base_type::TYPE_I16:
    return "int16_t";
  case t_base_type::TYPE_I32:
    return "int32_t";
  case t_base_type::TYPE_I64:
    return "int64_t";
  case t_base_type::TYPE_DOUBLE:
    return "double";
  default:
    throw "compiler error: no C++ base type name for base type " + t_base_type::t_base_name(tbase);
  }
}


string t_yarp_generator::namespace_prefix(string ns) {
  // Always start with "::", to avoid possible name collisions with
  // other names in one of the current namespaces.
  //
  // We also need a leading space, in case the name is used inside of a
  // template parameter.  "MyTemplate<::foo::Bar>" is not valid C++,
  // since "<:" is an alternative token for "[".
  string result = " ::";

  if (ns.size() == 0) {
    return result;
  }
  string::size_type loc;
  while ((loc = ns.find(".")) != string::npos) {
    result += ns.substr(0, loc);
    result += "::";
    ns = ns.substr(loc+1);
  }
  if (ns.size() > 0) {
    result += ns + "::";
  }
  return result;
}

string t_yarp_generator::namespace_open(string ns) {
  if (ns.size() == 0) {
    return "";
  }
  string result = "";
  string separator = "";
  string::size_type loc;
  while ((loc = ns.find(".")) != string::npos) {
    result += separator;
    result += "namespace ";
    result += ns.substr(0, loc);
    result += " {";
    separator = " ";
    ns = ns.substr(loc+1);
  }
  if (ns.size() > 0) {
    result += separator + "namespace " + ns + " {";
  }
  return result;
}

string t_yarp_generator::namespace_close(string ns) {
  if (ns.size() == 0) {
    return "";
  }
  string result = "}";
  string::size_type loc;
  while ((loc = ns.find(".")) != string::npos) {
    result += "}";
    ns = ns.substr(loc+1);
  }
  result += " // namespace";
  return result;
}

/////////////////////////////////////////////////////////////////////
/// C++ generator code ends
/////////////////////////////////////////////////////////////////////


/**
 * Emits the Table of Contents links at the top of the module's page
 */
void t_yarp_generator::generate_program_toc() {
  f_out_ << "<table><tr><th>Module</th><th>Services</th>"
   << "<th>Data types</th><th>Constants</th></tr>" << endl;
  generate_program_toc_row(program_);
  f_out_ << "</table>" << endl;
}


void t_yarp_generator::generate_program_toc_rows(t_program* tprog,
         std::vector<t_program*>& finished) {
  for (vector<t_program*>::iterator iter = finished.begin();
       iter != finished.end(); iter++) {
    if (tprog->get_path() == (*iter)->get_path()) {
      return;
    }
  }
  finished.push_back(tprog);
  generate_program_toc_row(tprog);
  vector<t_program*> includes = tprog->get_includes();
  for (vector<t_program*>::iterator iter = includes.begin();
       iter != includes.end(); iter++) {
    generate_program_toc_rows(*iter, finished);
  }
}

void t_yarp_generator::generate_program_toc_row(t_program* tprog) {
  string fname = tprog->get_name();
  f_out_ << "// " << fname << endl;
  if (!tprog->get_services().empty()) {
    vector<t_service*> services = tprog->get_services();
    vector<t_service*>::iterator sv_iter;
    for (sv_iter = services.begin(); sv_iter != services.end(); ++sv_iter) {
      string name = get_service_name(*sv_iter);
      //printf("//  %s (service)\n", name.c_str());
      f_out_ << "//  service: " << name << endl;
      //map<string,string> fn_yarp;
      vector<t_function*> functions = (*sv_iter)->get_functions();
      vector<t_function*>::iterator fn_iter;
      for (fn_iter = functions.begin(); fn_iter != functions.end(); ++fn_iter) {
        string fn_name = (*fn_iter)->get_name();
	//printf("//    %s (fn)\n", fn_name.c_str());
	f_out_ << "//    fn: " << fn_name << endl;
        //fn_yarp.insert(pair<string,string>(fn_name, yarp));
      }
      //for (map<string,string>::iterator yarp_iter = fn_yarp.begin();
      //yarp_iter != fn_yarp.end(); yarp_iter++) {
      //f_out_ << yarp_iter->second << endl;
      //}
    }
  }
  //map<string,string> data_types;
  if (!tprog->get_enums().empty()) {
    vector<t_enum*> enums = tprog->get_enums();
    vector<t_enum*>::iterator en_iter;
    for (en_iter = enums.begin(); en_iter != enums.end(); ++en_iter) {
      string name = (*en_iter)->get_name();
      f_out_ << "//  enum: " << name << endl;
      //data_types.insert(pair<string,string>(name, yarp));
    }
  }
  if (!tprog->get_typedefs().empty()) {
    vector<t_typedef*> typedefs = tprog->get_typedefs();
    vector<t_typedef*>::iterator td_iter;
    for (td_iter = typedefs.begin(); td_iter != typedefs.end(); ++td_iter) {
      string name = (*td_iter)->get_symbolic();
      //printf("//  %s (typedef)\n", name.c_str());
      f_out_ << "//  typedef: " << name << endl;
      //data_types.insert(pair<string,string>(name, yarp));
    }
  }
  if (!tprog->get_objects().empty()) {
    vector<t_struct*> objects = tprog->get_objects();
    vector<t_struct*>::iterator o_iter;
    for (o_iter = objects.begin(); o_iter != objects.end(); ++o_iter) {
      string name = (*o_iter)->get_name();
      f_out_ << "//  object: " << name << endl;
      //data_types.insert(pair<string,string>(name, yarp));
    }
  }
  //for (map<string,string>::iterator dt_iter = data_types.begin();
  //dt_iter != data_types.end(); dt_iter++) {
  //printf("//  ... %s ...\n", dt_iter->second.c_str());
  //}
  if (!tprog->get_consts().empty()) {
    //map<string,string> const_yarp;
    vector<t_const*> consts = tprog->get_consts();
    vector<t_const*>::iterator con_iter;
    for (con_iter = consts.begin(); con_iter != consts.end(); ++con_iter) {
      string name = (*con_iter)->get_name();
      f_out_ << "//  constant: " << name << endl;
      //string yarp ="<a href=\"" + fname + "#Const_" + name +
      //"\">" + name + "</a>";
      //const_yarp.insert(pair<string,string>(name, yarp));
    }
    //for (map<string,string>::iterator con_iter = const_yarp.begin();
    //con_iter != const_yarp.end(); con_iter++) {
    //printf("//  ... %s ...\n", con_iter->second.c_str());
    //}
  }
}

/**
 * Prepares for file generation by opening up the necessary file output
 * stream.
 */
void t_yarp_generator::generate_program() {
  // Make output directory
  MKDIR(get_out_dir().c_str());
  string fname = get_out_dir() + program_->get_name() + ".cpp";
  f_out_.open(fname.c_str());
  f_out_ << "// Thrift module: " << program_->get_name() << endl;

  //print_doc(program_);

  //generate_program_toc();

  if (!program_->get_consts().empty()) {
    f_out_ << endl << "// Constants" << endl;
    vector<t_const*> consts = program_->get_consts();
    generate_consts(consts);
  }

  if (!program_->get_enums().empty()) {
    // Generate enums
    f_out_ << endl << "// Enums" << endl;
    vector<t_enum*> enums = program_->get_enums();
    vector<t_enum*>::iterator en_iter;
    for (en_iter = enums.begin(); en_iter != enums.end(); ++en_iter) {
      generate_enum(*en_iter);
    }
  }

  if (!program_->get_typedefs().empty()) {
    // Generate typedefs
    f_out_ << endl << "// Typedefs" << endl;
    vector<t_typedef*> typedefs = program_->get_typedefs();
    vector<t_typedef*>::iterator td_iter;
    for (td_iter = typedefs.begin(); td_iter != typedefs.end(); ++td_iter) {
      generate_typedef(*td_iter);
    }
  }

  if (!program_->get_objects().empty()) {
    // Generate structs and exceptions in declared order
    f_out_ << endl << "// Structures" << endl;
    vector<t_struct*> objects = program_->get_objects();
    vector<t_struct*>::iterator o_iter;
    for (o_iter = objects.begin(); o_iter != objects.end(); ++o_iter) {
      if ((*o_iter)->is_xception()) {
        generate_xception(*o_iter);
      } else {
        generate_struct(*o_iter);
      }
    }
  }

  if (!program_->get_services().empty()) {
    // Generate services
    f_out_ << endl << "// Services" << endl;
    vector<t_service*> services = program_->get_services();
    vector<t_service*>::iterator sv_iter;
    for (sv_iter = services.begin(); sv_iter != services.end(); ++sv_iter) {
      service_name_ = get_service_name(*sv_iter);
      generate_service(*sv_iter);
    }
  }

  f_out_.close();

  generate_index();
}

/**
 * Emits the index.html file for the recursive set of Thrift programs
 */
void t_yarp_generator::generate_index() {
  string index_fname = get_out_dir() + "index.cpp";
  f_out_.open(index_fname.c_str());
  vector<t_program*> programs;
  generate_program_toc_rows(program_, programs);
  f_out_.close();
}

/**
 * If the provided documentable object has documentation attached, this
 * will emit it to the output stream in YARP format.
 */
void t_yarp_generator::print_doc(t_doc* tdoc) {
  if (tdoc->has_doc()) {
    string doc = tdoc->get_doc();
    size_t index;
    while ((index = doc.find_first_of("\r\n")) != string::npos) {
      if (index == 0) {
  f_out_ << "<p/>" << endl;
      } else {
  f_out_ << doc.substr(0, index) << endl;
      }
      if (index + 1 < doc.size() && doc.at(index) != doc.at(index + 1) &&
    (doc.at(index + 1) == '\r' || doc.at(index + 1) == '\n')) {
  index++;
      }
      doc = doc.substr(index + 1);
    }
    f_out_ << doc << "<br/>";
  }
}

/**
 * Prints out the provided type in YARP
 */
string t_yarp_generator::print_type(t_type* ttype) {
  return type_name(ttype);

  // OLD
  string result;
  if (ttype->is_container()) {
    if (ttype->is_list()) {
      result += "list<";
      result += print_type(((t_list*)ttype)->get_elem_type());
      result += ">";
    } else if (ttype->is_set()) {
      result += "set<";
      result += print_type(((t_set*)ttype)->get_elem_type());
      result += ">";
    } else if (ttype->is_map()) {
      result += "map<";
      result += print_type(((t_map*)ttype)->get_key_type());
      result += ", ";
      result += print_type(((t_map*)ttype)->get_val_type());
      result += ">";
    }
  } else if (ttype->is_base_type()) {
    result += (((t_base_type*)ttype)->is_binary() ? "binary" : ttype->get_name());
  } else {
    string prog_name = ttype->get_program()->get_name();
    string type_name = ttype->get_name();
    if (ttype->get_program() != program_) {
      result += prog_name;
      result += ".";
    }
    result += type_name;
  }
  return result;
}

/**
 * Prints out an YARP representation of the provided constant value
 */
string t_yarp_generator::print_const_value(t_const_value* tvalue) {
  string result;
  bool first = true;
  char buf[10000];
  switch (tvalue->get_type()) {
  case t_const_value::CV_INTEGER:
    sprintf(buf,"%d",tvalue->get_integer());
    result += buf;
    break;
  case t_const_value::CV_DOUBLE:
    sprintf(buf,"%g",tvalue->get_double());
    result += buf;
    break;
  case t_const_value::CV_STRING:
    result += string("\"") + get_escaped_string(tvalue) + "\"";
    break;
  case t_const_value::CV_MAP:
    {
      result += "{ ";
      map<t_const_value*, t_const_value*> map_elems = tvalue->get_map();
      map<t_const_value*, t_const_value*>::iterator map_iter;
      for (map_iter = map_elems.begin(); map_iter != map_elems.end(); map_iter++) {
        if (!first) {
          result += ", ";
        }
        first = false;
        result += print_const_value(map_iter->first);
        result += " = ";
        result += print_const_value(map_iter->second);
      }
      result += " }";
    }
    break;
  case t_const_value::CV_LIST:
    {
      result += "{ ";
      vector<t_const_value*> list_elems = tvalue->get_list();;
      vector<t_const_value*>::iterator list_iter;
      for (list_iter = list_elems.begin(); list_iter != list_elems.end(); list_iter++) {
        if (!first) {
          result += ", ";
        }
        first = false;
        result += print_const_value(*list_iter);
      }
      result += " }";
    }
    break;
  default:
    result += "UNKNOWN";
    break;
  }
}

/**
 * Generates a typedef.
 *
 * @param ttypedef The type definition
 */
void t_yarp_generator::generate_typedef(t_typedef* ttypedef) {
  string name = ttypedef->get_name();
  f_out_ << "<div class=\"definition\">";
  f_out_ << "<h3 id=\"Typedef_" << name << "\">Typedef: " << name
   << "</h3>" << endl;
  f_out_ << "<p><strong>Base type:</strong>&nbsp;";
  print_type(ttypedef->get_type());
  f_out_ << "</p>" << endl;
  print_doc(ttypedef);
  f_out_ << "</div>" << endl;
}

/**
 * Generates code for an enumerated type.
 *
 * @param tenum The enumeration
 */
void t_yarp_generator::generate_enum(t_enum* tenum) {
  string name = tenum->get_name();
  f_out_ << "<div class=\"definition\">";
  f_out_ << "<h3 id=\"Enum_" << name << "\">Enumeration: " << name
   << "</h3>" << endl;
  print_doc(tenum);
  vector<t_enum_value*> values = tenum->get_constants();
  vector<t_enum_value*>::iterator val_iter;
  f_out_ << "<br/><table>" << endl;
  for (val_iter = values.begin(); val_iter != values.end(); ++val_iter) {
    f_out_ << "<tr><td><code>";
    f_out_ << (*val_iter)->get_name();
    f_out_ << "</code></td><td><code>";
    f_out_ << (*val_iter)->get_value();
    f_out_ << "</code></td></tr>" << endl;
  }
  f_out_ << "</table></div>" << endl;
}

/**
 * Generates a constant value
 */
void t_yarp_generator::generate_const(t_const* tconst) {
  string name = tconst->get_name();
  f_out_ << "<tr id=\"Const_" << name << "\"><td><code>" << name
   << "</code></td><td><code>";
  print_type(tconst->get_type());
  f_out_ << "</code></td><td><code>";
  print_const_value(tconst->get_value());
  f_out_ << "</code></td></tr>";
  if (tconst->has_doc()) {
    f_out_ << "<tr><td colspan=\"3\"><blockquote>";
    print_doc(tconst);
    f_out_ << "</blockquote></td></tr>";
  }
}

/**
 * Generates a struct definition for a thrift data type.
 *
 * @param tstruct The struct definition
 */
void t_yarp_generator::generate_struct(t_struct* tstruct) {
  string name = tstruct->get_name();
  bool except = tstruct->is_xception();
  vector<t_field*> members = tstruct->get_members();
  vector<t_field*>::iterator mem_iter = members.begin();

  f_out_ << "class " << name << " : public yarp::os::Portable {" << endl;
  f_out_ << "public:" << endl;

  for ( ; mem_iter != members.end(); mem_iter++) {
    string mname = (*mem_iter)->get_name();
    string mtype = print_type((*mem_iter)->get_type());
    f_out_ << "    " << mtype << " " << mname << ";" << endl; 
  }
  mem_iter = members.begin();

  f_out_ << "    bool read(yarp::os::ConnectionReader& connection) {" << endl;
  for ( ; mem_iter != members.end(); mem_iter++) {
    string mname = (*mem_iter)->get_name();
    string mtype = print_type((*mem_iter)->get_type());
    f_out_ << "        // read: " << mtype << " " << mname << ";" << endl; 
  }
  f_out_ << "    }" << endl;
  mem_iter = members.begin();


  f_out_ << "    bool write(yarp::os::ConnectionWriter& connection) {" << endl;
  for ( ; mem_iter != members.end(); mem_iter++) {
    string mname = (*mem_iter)->get_name();
    string mtype = print_type((*mem_iter)->get_type());
    f_out_ << "        // write: " << mtype << " " << mname << ";" << endl; 
  }
  f_out_ << "    }" << endl;
  mem_iter = members.begin();

  f_out_ << "};" << endl;

  /*
  f_out_ << "<table>";
  f_out_ << "<tr><th>Key</th><th>Field</th><th>Type</th><th>Description</th><th>Requiredness</th><th>Default value</th></tr>"
    << endl;
  for ( ; mem_iter != members.end(); mem_iter++) {
    f_out_ << "<tr><td>" << (*mem_iter)->get_key() << "</td><td>";
    f_out_ << (*mem_iter)->get_name();
    f_out_ << "</td><td>";
    print_type((*mem_iter)->get_type());
    f_out_ << "</td><td>";
    f_out_ << (*mem_iter)->get_doc();
    f_out_ << "</td><td>";
    if ((*mem_iter)->get_req() == t_field::T_OPTIONAL) {
      f_out_ << "optional";
    } else if ((*mem_iter)->get_req() == t_field::T_REQUIRED) {
      f_out_ << "required";
    } else {
      f_out_ << "default";
    }
    f_out_ << "</td><td>";
    t_const_value* default_val = (*mem_iter)->get_value();
    if (default_val != NULL) {
      print_const_value(default_val);
    }
    f_out_ << "</td></tr>" << endl;
  }
  f_out_ << "</table><br/>";
  print_doc(tstruct);
  f_out_ << "</div>";
  */
}

/**
 * Exceptions are special structs
 *
 * @param tstruct The struct definition
 */
void t_yarp_generator::generate_xception(t_struct* txception) {
  generate_struct(txception);
}


std::string t_yarp_generator::function_prototype(t_function *tfn) {
  string result = "";
  t_function **fn_iter = &tfn;
  string fn_name = (*fn_iter)->get_name();
  string return_type = print_type((*fn_iter)->get_returntype());
  result += return_type;
  result += string(" ") + fn_name + "(";
  bool first = true;
  vector<t_field*> args = (*fn_iter)->get_arglist()->get_members();
  vector<t_field*>::iterator arg_iter = args.begin();
  if (arg_iter != args.end()) {
    for ( ; arg_iter != args.end(); arg_iter++) {
      if (!first) {
	result += ", ";
      }
      first = false;
      result += type_name((*arg_iter)->get_type(),false,true);
      result += string(" ") + (*arg_iter)->get_name();
      if ((*arg_iter)->get_value() != NULL) {
	result += " = ";
        result += print_const_value((*arg_iter)->get_value());
      }
    }
  }
  result += ")";
  return result;
}

/**
 * Generates the YARP block for a Thrift service.
 *
 * @param tservice The service definition
 */
void t_yarp_generator::generate_service(t_service* tservice) {
  {
    f_out_ << "class " << service_name_ << " {" << endl;
    f_out_ << "public:" << endl;
    vector<t_function*> functions = tservice->get_functions();
    vector<t_function*>::iterator fn_iter = functions.begin();
    for ( ; fn_iter != functions.end(); fn_iter++) {
      f_out_ << "    virtual " << function_prototype(*fn_iter) 
	     << " = 0;" << endl;
    }
    f_out_ << "};" << endl;
  }
  
  {
    f_out_ << "class " << service_name_ << "Client : public " << service_name_ << ", public yarp::os::Client {" << endl;
    f_out_ << "public:" << endl;

    vector<t_function*> functions = tservice->get_functions();
    vector<t_function*>::iterator fn_iter = functions.begin();
    for ( ; fn_iter != functions.end(); fn_iter++) {
      f_out_ << "    virtual " << function_prototype(*fn_iter);
      f_out_ << " {" << endl;
      f_out_ << "        yarp::os::ConnectionWriter& writer = getWriter();" << endl;
      f_out_ << "        // proxy action" << endl;
      if ((*fn_iter)->is_oneway()) {
	f_out_ << "        // (one way)" << endl;
      }
      f_out_ << "    }" << endl;
    }
    f_out_ << "};" << endl;
  }

  {
    f_out_ << "class " << service_name_ << "Server : public " << service_name_ << ", public yarp::os::PortReader {" << endl;
    f_out_ << "public:" << endl;
    f_out_ << "    " << service_name_ << " *impl;" << endl;
    f_out_ << "    " << service_name_ << "Server() { impl = 0/*NULL*/; }" << endl;

    vector<t_function*> functions = tservice->get_functions();
    vector<t_function*>::iterator fn_iter = functions.begin();
    for ( ; fn_iter != functions.end(); fn_iter++) {
      f_out_ << "    virtual " << function_prototype(*fn_iter);
      f_out_ << " {" << endl;
      f_out_ << "        ";
      //f_out_ << "if (impl) ";
      if (!(*fn_iter)->get_returntype()->is_void()) {
	f_out_ << "return ";
      }
      f_out_ << "impl->" + (*fn_iter)->get_name() << "(";
      bool first = true;
      vector<t_field*> args = (*fn_iter)->get_arglist()->get_members();
      vector<t_field*>::iterator arg_iter = args.begin();
      if (arg_iter != args.end()) {
	for ( ; arg_iter != args.end(); arg_iter++) {
	  if (!first) {
	    f_out_ << ",";
	  }
	  first = false;
	  f_out_ << (*arg_iter)->get_name();
	}
      }
      f_out_ << ");" << endl;
      f_out_ << "    }" << endl;
    }
    f_out_ << "    virtual bool read(yarp::os::ConnectionReader& reader) {" << endl;
    f_out_ << "        int tag = reader.expectInt();" << endl;
    f_out_ << "        if reader.isError() return false;" << endl;
    f_out_ << "        switch (tag) {" << endl;
    fn_iter = functions.begin();
    for ( ; fn_iter != functions.end(); fn_iter++) {
      f_out_ << "        case ...:" << endl;
      f_out_ << "            // " << (*fn_iter)->get_name() << endl;
      f_out_ << "            break;" << endl;
    }
    f_out_ << "        }" << endl;
    f_out_ << "        return false;" << endl;
    f_out_ << "    }" << endl;
    f_out_ << "};" << endl;
  }



  //if (tservice->get_extends()) {
  //f_out_ << "<div class=\"extends\"><em>extends</em> ";
  //print_type(tservice->get_extends());
  //f_out_ << "</div>\n";
  //}
  //print_doc(tservice);

  /*
  vector<t_function*> functions = tservice->get_functions();
  vector<t_function*>::iterator fn_iter = functions.begin();
  for ( ; fn_iter != functions.end(); fn_iter++) {
    string fn_name = (*fn_iter)->get_name();
    f_out_ << "<div class=\"definition\">";
    f_out_ << "<h4 id=\"Fn_" << service_name_ << "_" << fn_name
      << "\">Function: " << service_name_ << "." << fn_name
      << "</h4>" << endl;
    f_out_ << "<pre>";
    int offset = print_type((*fn_iter)->get_returntype());
    bool first = true;
    f_out_ << " " << fn_name << "(";
    offset += fn_name.size() + 2;
    vector<t_field*> args = (*fn_iter)->get_arglist()->get_members();
    vector<t_field*>::iterator arg_iter = args.begin();
    if (arg_iter != args.end()) {
      for ( ; arg_iter != args.end(); arg_iter++) {
        if (!first) {
          f_out_ << "," << endl;
          for (int i = 0; i < offset; ++i) {
            f_out_ << " ";
          }
        }
        first = false;
        print_type((*arg_iter)->get_type());
        f_out_ << " " << (*arg_iter)->get_name();
        if ((*arg_iter)->get_value() != NULL) {
          f_out_ << " = ";
          print_const_value((*arg_iter)->get_value());
        }
      }
    }
    f_out_ << ")" << endl;
    first = true;
    vector<t_field*> excepts = (*fn_iter)->get_xceptions()->get_members();
    vector<t_field*>::iterator ex_iter = excepts.begin();
    if (ex_iter != excepts.end()) {
      f_out_ << "    throws ";
      for ( ; ex_iter != excepts.end(); ex_iter++) {
        if (!first) {
          f_out_ << ", ";
        }
        first = false;
        print_type((*ex_iter)->get_type());
      }
      f_out_ << endl;
    }
    f_out_ << "</pre>";
    print_doc(*fn_iter);
    f_out_ << "</div>";
  }
  */
}

THRIFT_REGISTER_GENERATOR(yarp, "YARP", "")

