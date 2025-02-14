#pragma once

class lpgbt_reg_and_val{
public:
  lpgbt_reg_and_val(unsigned int reg_addr, unsigned int val=0){
    m_val=val;
    m_reg_addr=reg_addr;
  }
  ~lpgbt_reg_and_val(){;}
  unsigned int val() const { return m_val; }
  unsigned int reg_address()  const { return m_reg_addr; }

  friend bool operator<(const lpgbt_reg_and_val& lhs, const lpgbt_reg_and_val& rhs)
  { 
    return lhs.m_reg_addr < rhs.m_reg_addr;
  }
  friend bool operator==(const lpgbt_reg_and_val& regA, const lpgbt_reg_and_val& regB)
  {
    return regA.m_reg_addr==regB.m_reg_addr;
  }
  friend std::ostream& operator<<(std::ostream& out,const lpgbt_reg_and_val& rv)
  {
    out << "Address = " << rv.m_reg_addr << " ; value = " << rv.m_val ;
    return out;
  }
  
private:
  unsigned int m_val;
  unsigned int m_reg_addr;
};

class lpgbt_reg_and_vals{
public:
  lpgbt_reg_and_vals(lpgbt_reg_and_val rv){
    m_vals=std::vector<unsigned int>(1,rv.val());
    m_reg_addr=rv.reg_address();
  }
  lpgbt_reg_and_vals(unsigned int reg_addr, std::vector<unsigned int> vals){
    m_vals=vals;
    m_reg_addr=reg_addr;
  }
  ~lpgbt_reg_and_vals(){;}
  std::vector<unsigned int> vals() const { return m_vals; }
  unsigned int reg_address()  const { return m_reg_addr; }

  void update(lpgbt_reg_and_val rv){
    m_vals.insert(m_vals.begin(),rv.val());
    set_reg_address(rv.reg_address());
  }

  void add_val( unsigned int val )
  {
    m_vals.push_back( val );
  }

  friend std::ostream& operator<<(std::ostream& out,const lpgbt_reg_and_vals& rv)
  {
    out << rv.reg_address() << " : {";
    std::for_each( rv.m_vals.begin(), rv.m_vals.end(), [&](const auto& v){ out << v << " ";} );
    out << "}";
    return out;
  }

private:
  inline void set_reg_address(unsigned int reg_addr) { m_reg_addr=reg_addr; }
  std::vector<unsigned int> m_vals;
  unsigned int m_reg_addr;
};

