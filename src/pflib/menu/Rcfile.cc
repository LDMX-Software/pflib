#include "pflib/menu/Rcfile.h"

#include <yaml-cpp/yaml.h>

namespace pflib::menu {

Rcmap::Rcmap() {}

Rcmap::~Rcmap() {
  for (auto pnode : nodes_) delete pnode;
}

void Rcmap::addFile(YAML::Node* n) { nodes_.push_back(n); }

/** Does this map have the given key ? */
bool Rcmap::has_key(const std::string& keyname) const {
  for (auto pnode : nodes_)
    if ((*pnode)[keyname]) return true;
  return false;
}

/** Is this key a vector? */
bool Rcmap::is_vector(const std::string& keyname) const {
  for (auto pnode : nodes_) {
    YAML::Node n = (*pnode)[keyname];
    if (n) return n.IsSequence();
  }
  return false;
}

/** Is this key a submap? */
bool Rcmap::is_map(const std::string& keyname) const {
  for (auto pnode : nodes_) {
    YAML::Node n = (*pnode)[keyname];
    if (n) return n.IsMap();
  }
  return false;
}

/** get item as map */
Rcmap Rcmap::getMap(const std::string& keyname) const {
  Rcmap map;
  for (auto pnode : nodes_) {
    YAML::Node n = (*pnode)[keyname];
    if (n) map.addFile(new YAML::Node(n));
  }
  return map;
}

/** get item as vector of strings */
std::vector<std::string> Rcmap::getVString(const std::string& keyname) const {
  for (auto pnode : nodes_) {
    YAML::Node n = (*pnode)[keyname];
    if (n) return n.as<std::vector<std::string>>();
  }
  return std::vector<std::string>();
}

/** get item as vector of ints */
std::vector<int> Rcmap::getVInt(const std::string& keyname) const {
  for (auto pnode : nodes_) {
    YAML::Node n = (*pnode)[keyname];
    if (n) return n.as<std::vector<int>>();
  }
  return std::vector<int>();
}

std::vector<bool> Rcmap::getVBool(const std::string& keyname) const {
  for (auto pnode : nodes_) {
    YAML::Node n = (*pnode)[keyname];
    if (n) return n.as<std::vector<bool>>();
  }
  return std::vector<bool>();
}

/** get item as scalar string */
std::string Rcmap::getString(const std::string& keyname) const {
  for (auto pnode : nodes_) {
    YAML::Node n = (*pnode)[keyname];
    if (n) return n.as<std::string>();
  }
  return "";
}

/** gRcmap::et item as scalar int */
int Rcmap::getInt(const std::string& keyname) const {
  for (auto pnode : nodes_) {
    YAML::Node n = (*pnode)[keyname];
    if (n) return n.as<int>();
  }
  return 0;
}

bool Rcmap::getBool(const std::string& keyname) const {
  for (auto pnode : nodes_) {
    YAML::Node n = (*pnode)[keyname];
    if (n) return n.as<bool>();
  }
  return false;
}

Rcfile::Rcfile() {}

void Rcfile::declareVString(const std::string& key,
                            const std::string& description) {
  HelpInfo pt;
  pt.key = key;
  pt.description = description;
  pt.type = "VSTRING";
  helpInfo_.push_back(pt);
}
void Rcfile::declareString(const std::string& key,
                           const std::string& description) {
  HelpInfo pt;
  pt.key = key;
  pt.description = description;
  pt.type = "STRING";
  helpInfo_.push_back(pt);
}
void Rcfile::declareVBool(const std::string& key,
                          const std::string& description) {
  HelpInfo pt;
  pt.key = key;
  pt.description = description;
  pt.type = "VBOOL";
  helpInfo_.push_back(pt);
}
void Rcfile::declareBool(const std::string& key,
                         const std::string& description) {
  HelpInfo pt;
  pt.key = key;
  pt.description = description;
  pt.type = "BOOL";
  helpInfo_.push_back(pt);
}
void Rcfile::declareVInt(const std::string& key,
                         const std::string& description) {
  HelpInfo pt;
  pt.key = key;
  pt.description = description;
  pt.type = "VINT";
  helpInfo_.push_back(pt);
}
void Rcfile::declareInt(const std::string& key,
                        const std::string& description) {
  HelpInfo pt;
  pt.key = key;
  pt.description = description;
  pt.type = "INT";
  helpInfo_.push_back(pt);
}

void Rcfile::load(const std::string& file) {
  contents_.addFile(new YAML::Node(YAML::LoadFile(file)));
}

void Rcfile::help() {
  for (auto ihelp : helpInfo_) {
    printf(" %s [%s] : %s\n", ihelp.key.c_str(), ihelp.type.c_str(),
           ihelp.description.c_str());
  }
}

}
