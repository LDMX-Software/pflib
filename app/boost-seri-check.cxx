// Source - https://stackoverflow.com/a/54436971
// Posted by Damian, modified by community. See post 'Timeline' for change history
// Retrieved 2026-08-25, License - CC BY-SA 4.0

#include <iostream>
#include <fstream>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/serialization/binary_object.hpp>
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/vector.hpp>

struct MultiPack {
  std::vector<uint32_t> data;
  std::vector<uint32_t> trig;
  std::vector<uint32_t> algo;

  friend class boost::serialization::access;
  template<class Archive>
  void serialize(Archive& ar, const unsigned int version) {
    ar & data;
    ar & trig;
    ar & algo;
  }
};

int main() {
  {
    std::ofstream f("data.dat", std::ofstream::binary);
    boost::archive::binary_oarchive ar(f);
  
    MultiPack mp;
    mp.data.push_back(1);
    mp.data.push_back(2);
    mp.trig.push_back(3);
    mp.algo.push_back(4);
    mp.algo.push_back(5);
  
    ar << mp;
  }

  {
    std::ifstream f("data.dat", std::ifstream::binary);
    boost::archive::binary_iarchive ar(f);

    MultiPack mp;
    ar >> mp;

    std::cout << "data" << std::endl;
    for (const auto& word : mp.data) {
      std::cout << word << std::endl;
    }
    std::cout << "trig" << std::endl;
    for (const auto& word : mp.trig) {
      std::cout << word << std::endl;
    }
    std::cout << "algo" << std::endl;
    for (const auto& word : mp.algo) {
      std::cout << word << std::endl;
    }
  }
}

