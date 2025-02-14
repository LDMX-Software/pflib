#pragma once
#ifndef YAML_HELPER
#define YAML_HELPER

#include <yaml-cpp/yaml.h>
#include <iostream>
#include "spdlogger.hpp"

// taken from : https://gist.github.com/kunaltyagi/ebe13098cc22a717f793684659644f4e

namespace yaml_helper{

  template <class ...Args>
  bool scalar_compare(YAML::Node&, YAML::Node&);
  template<class T, class ...Args>
  inline bool helper_compare(YAML::Node& node1, YAML::Node& node2)
  {
    try
      {
	return node1.as<T>() == node2.as<T>();
      }
    catch(YAML::TypedBadConversion<T>& e)
      {
	return scalar_compare<Args...>(node1, node2);
      }
  }
  template<class ...Args>
  inline bool scalar_compare(YAML::Node& node1, YAML::Node& node2)
  {
    return helper_compare<Args...>(node1, node2);
  }
  template <>
  inline bool scalar_compare<>(YAML::Node&, YAML::Node&)
  {
    return true;
  }

  inline bool compare(YAML::Node node1, YAML::Node node2)
  {
    if (node1.is(node2))
      {
	return true;
      }
    if (node1.Type() != node2.Type() )
      {
	return false;
      }

    switch(node1.Type())
      {
      case YAML::NodeType::Scalar:
	return scalar_compare<bool, int, double>(node1, node2);
      case YAML::NodeType::Null:
	return true;
      case YAML::NodeType::Undefined:
	return false;
      case YAML::NodeType::Map:
      case YAML::NodeType::Sequence:
	if (node1.size() != node2.size())
	  {
	    return false;
	  }
      }

    if (node1.IsMap())
      {
	bool result = true;
	for (YAML::const_iterator it1 = node1.begin(), it2;
	     it1 != node1.end() && result == true; ++it1)
	  {
	    for (it2 = node2.begin(); it2 != node2.end(); ++it2)
	      {
		auto strcompare = it1->first.as<std::string>().compare(it2->first.as<std::string>());
		if (strcompare==0 && compare(it1->first, it2->first))
		  {
		    break;
		  }
	      }
	    if (it2 == node2.end())
	      {
		return false;
	      }
	    result = compare(it1->second, it2->second);
	    //if(result == false)
	    //  std::cout << "diff it1 and it2: \n" << it1->second << "\n" << it2->second << std::endl;
	  }
	return result;
      }

    return std::equal(node1.begin(), node1.end(), node2.begin(), compare);
  }

  inline const YAML::Node & cnode(const YAML::Node &n) {
    return n;
  }

  inline YAML::Node merge_nodes(YAML::Node a, YAML::Node b)
  {
    if (!b.IsMap()) {
      // If b is not a map, merge result is b, unless b is null
      return b.IsNull() ? a : b;
    }
    if (!a.IsMap()) {
      // If a is not a map, merge result is b
      return b;
    }
    if (!b.size()) {
      // If a is a map, and b is an empty map, return a
      return a;
    }
    // Create a new map 'c' with the same mappings as a, merged with b
    auto c = YAML::Node(YAML::NodeType::Map);
    for (auto n : a) {
      if (n.first.IsScalar()) {
	const std::string & key = n.first.Scalar();
	auto t = YAML::Node(cnode(b)[key]);
	if (t) {
	  c[n.first] = merge_nodes(n.second, t);
	  continue;
	}
      }
      c[n.first] = n.second;
    }
    // Add the mappings from 'b' not already in 'c'
    for (auto n : b) {
      if (!n.first.IsScalar() || !cnode(c)[n.first.Scalar()]) {
	c[n.first] = n.second;
      }
    }
    return c;
  }

  struct StringAndVal
  {
    StringAndVal() : 
      mVal(0),
      mStr(""){;}
    StringAndVal(std::string str, uint32_t val) : 
      mVal(val),
      mStr(str){;}
    std::string mStr;
    uint32_t mVal;
  };
  
  inline std::vector<StringAndVal> flattenConfig(YAML::Node node, std::string parentKey="", std::string separator="__")
  {
    std::vector<StringAndVal> vec;
    for( auto childnode : node ){
      auto nodeName = childnode.first.as<std::string>();
      std::string newKey = parentKey!="" ? parentKey + separator + nodeName : nodeName;
      switch( node[nodeName].Type() ){
      case YAML::NodeType::Null: 
	break;
      case YAML::NodeType::Scalar:
	{
	  StringAndVal sv(newKey, node[nodeName].as<uint32_t>() );
	  vec.push_back(sv);
	}
      case YAML::NodeType::Sequence:
	{
	  break;
	}
      case YAML::NodeType::Map:
	{
	  auto newVec = flattenConfig( node[nodeName], newKey, separator );
	  vec.insert( vec.end(), newVec.begin(), newVec.end());
	  break;
	}
      case YAML::NodeType::Undefined:
	{
	  break;
	}
      }
    }
    return vec;
  }

  inline YAML::Node prune(YAML::Node read_config, YAML::Node orig_config)
  {
    YAML::Node pruned = read_config;
    YAML::Node toberemoved;
    for ( YAML::iterator it=read_config.begin(); it!=read_config.end(); ++it ){
      auto block_id = it->first.as<std::string>();
      for ( YAML::iterator jt=it->second.begin(); jt!=it->second.end(); ++jt ){
	auto subblock_id = jt->first.as<std::string>();
	for ( YAML::iterator kt=jt->second.begin(); kt!=jt->second.end(); ++kt ){
	  auto pname=kt->first.as<std::string>();
	  if( !orig_config[block_id][subblock_id][pname].IsDefined() ){
	    spdlog::apply_all([&](LoggerPtr l) { l->debug("Node {}.{}.{} was not found in config",block_id,subblock_id,pname); });
	    toberemoved[block_id][subblock_id][pname]=0;
	  }
	}
      }
    }
    for ( YAML::iterator it=toberemoved.begin(); it!=toberemoved.end(); ++it ){
      auto block_id = it->first.as<std::string>();
      for ( YAML::iterator jt=it->second.begin(); jt!=it->second.end(); ++jt ){
	auto subblock_id = jt->first.as<std::string>();
	for ( YAML::iterator kt=jt->second.begin(); kt!=jt->second.end(); ++kt ){
	  auto pname=kt->first.as<std::string>();
	  spdlog::apply_all([&](LoggerPtr l) { l->debug("Node {}.{}.{} will be removed",block_id,subblock_id,pname); });
	  pruned[block_id][subblock_id].remove( pname );
	}
      }
    }
    return pruned;
  }

  inline YAML::Node vtrx_prune(YAML::Node read_config, YAML::Node orig_config)
  {
    YAML::Node pruned = read_config;
    YAML::Node toberemoved;
    for ( YAML::iterator it=read_config.begin(); it!=read_config.end(); ++it ){
      auto block_id = it->first.as<std::string>();
      if( !it->second.IsMap() ) continue;
      for ( YAML::iterator jt=it->second.begin(); jt!=it->second.end(); ++jt ){
	auto pname = jt->first.as<std::string>();
	if( !orig_config[block_id][pname].IsDefined() ){
	  spdlog::apply_all([&](LoggerPtr l) { l->debug("Node {}.{} was not found in config",block_id,pname); });
	  toberemoved[block_id][pname]=0;
	}
      }
    }
    for ( YAML::iterator it=toberemoved.begin(); it!=toberemoved.end(); ++it ){
      auto block_id = it->first.as<std::string>();
      for ( YAML::iterator jt=it->second.begin(); jt!=it->second.end(); ++jt ){
	auto pname = jt->first.as<std::string>();
	spdlog::apply_all([&](LoggerPtr l) { l->debug("Node {}.{} will be removed",block_id,pname); });
	pruned[block_id].remove( pname );
      }
      if( pruned[block_id].size()==0 )
	pruned.remove( block_id );
    }
    return pruned;
  }
  
  inline std::vector<std::string> strToYamlNodeNames(std::string str,std::string delimiter = "__")
  {
    size_t pos = 0;
    std::vector<std::string> strvec;
    while ((pos = str.find(delimiter)) != std::string::npos) {
      strvec.push_back( str.substr(0, pos) );
      str.erase(0, pos + delimiter.length());
    }
    strvec.push_back( str );
    return strvec;
  }

}
#endif
