#include <array>
#include <bitset>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <vector>

uint32_t get_bits(const std::array<uint32_t, 3>& packet, int loc, int num) {
  if ((loc < 0) || (num < 1) || (num > 31) || (loc + num > 96)) {
    throw std::invalid_argument("get_bits index out of bounds");
  }
  int word = loc / 32;
  if (word != ((loc + num - 1) / 32)) {
    int split = 32 - (loc % 32);
    return (((packet[word] >> (32 - ((loc + split) % 32))) &
             ((1u << split) - 1u))
            << (num - split)) |
           ((packet[word + 1] >> (32 - (num - split))) &
            ((1u << (num - split)) - 1u));
  }
  return (packet[word] >> (32 - ((loc + num) % 32))) & ((1u << num) - 1u);
}

uint64_t decode(uint32_t word) {
  // we are using midpoint decoding, hence we set the bit after the mantissa
  // to 1.
  uint64_t exponent = (word >> 4) & ((1u << 5) - 1u);
  uint64_t mantissa = word & ((1u << 4) - 1u);
  uint64_t l1 = exponent + 3;
  uint64_t output = 0;
  if (exponent > 0) {
    output |= (1ull << l1) | (mantissa << (l1 - 4));
    if (l1 >= 5) {
      output |= (1ull << (l1 - 5));  // midpoint
    }
  } else {
    output = mantissa;
  }
  return output;
}

int main(int argc, char* argv[]) {
  if (argc != 4) {
    std::cout << "\nUsage: ./econt-decoder ./file.csv num_lines decode\n";
    std::cout << "num_lines is the number of lines you would like to examine\n";
    std::cout
        << "decode is 1 to view decoded data and 0 to view raw binary data\n\n";
    return 1;
  }

  std::ifstream file(argv[1]);
  if (!file) {
    std::cerr << "Cannot open file\n";
    return 1;
  }
  int max_lines = std::stoi(argv[2]);
  int decoded = std::stoi(argv[3]);

  std::string line;
  std::getline(file, line);  // skip the header
  int line_count = 0;
  while (std::getline(file, line) && (line_count < max_lines)) {
    std::stringstream ss(line);
    std::string value;
    std::vector<std::string> row;
    std::cout << "Decoding line " << line_count + 1 << "\n";

    while (std::getline(ss, value, ',')) {
      row.push_back(value);
    }
    if (row.size() < 3) {
      std::cout << "Incorrect CSV line length\n";
      return 1;
    }
    std::array<uint32_t, 3> packet;
    for (int i = 0; i < 3; i++) {
      if (std::string(row[i]).size() != 8) {
        std::cout << "Please provide a packet of the correct size\n";
        return 1;
      }
      packet[i] = std::stoul(row[i], nullptr, 16);
    }

    // print the header
    for (size_t i{0}; i < packet.size(); i++) {
      std::cout << "link" << i << ": " << std::bitset<32>(packet[i]) << "\n";
    }
    std::cout << "Header: " << std::bitset<4>(get_bits(packet, 0, 4)) << "\n";

    // print max
    if (decoded == 0) {
      for (int i{0}; i < 8; i++) {
        std::cout << "Max" << i + 1 << ": "
                  << std::bitset<2>(get_bits(packet, 4 + i * 2, 2)) << "\n";
      }
    } else {
      for (int i{0}; i < 8; i++) {
        std::cout << "Max" << i + 1 << ": " << get_bits(packet, 4 + i * 2, 2)
                  << "\n";
      }
    }

    // print STC
    if (decoded == 0) {
      for (int i{0}; i < 8; i++) {
        std::cout << "STC" << i + 1 << ": "
                  << std::bitset<9>(get_bits(packet, 20 + i * 9, 9)) << "\n";
      }
    } else {
      for (int i{0}; i < 8; i++) {
        std::cout << "STC" << i + 1 << ": "
                  << decode(get_bits(packet, 20 + i * 9, 9)) << "\n";
      }
    }
    std::cout << "\n";
    line_count += 1;
  }
  file.close();
  return 0;
}
