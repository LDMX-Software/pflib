#pragma once

class reg_and_val{
public:
  reg_and_val(unsigned int r0, unsigned int r1, unsigned int val=0){
    m_val=val;
    m_r0=r0;
    m_r1=r1;
  }
  ~reg_and_val(){;}
  unsigned int val() const { return m_val; }
  unsigned int r0()  const { return m_r0; }
  unsigned int r1()  const { return m_r1; }
  unsigned int address()  const { return m_r0 | m_r1<<8; }

  inline void updateVal( const unsigned int new_val)
  {
    m_val = new_val;
  }
  
  friend bool operator<(const reg_and_val& lhs, const reg_and_val& rhs)
  { 
    return lhs.address() < rhs.address();
  }
  friend bool operator==(const reg_and_val& regA, const reg_and_val& regB)
  {
    return regA.address()==regB.address();
  }
  friend std::ostream& operator<<(std::ostream& out,const reg_and_val& rv)
  {
    out << rv.m_r0 << " " << rv.m_r1 << " " << rv.m_val;
    return out;
  }

private:
  unsigned int m_val;
  unsigned int m_r0;
  unsigned int m_r1;
};

class reg_and_vals{
public:
  reg_and_vals(reg_and_val rv){
    m_vals=std::vector<unsigned int>(1,rv.val());
    m_r0=rv.r0();
    m_r1=rv.r1();
  }
  reg_and_vals(unsigned int r0, unsigned int r1, std::vector<unsigned int> vals){
    m_vals=vals;
    m_r0=r0;
    m_r1=r1;
  }
  ~reg_and_vals(){;}
  std::vector<unsigned int> vals() const { return m_vals; }
  unsigned int address()  const { return m_r0 | m_r1<<8; }

  void update(reg_and_val rv){
    m_vals.insert(m_vals.begin(),rv.val());
    setAddress(rv.r0(),rv.r1());
  }

  unsigned int r0()  const { return m_r0; }
  unsigned int r1()  const { return m_r1; }
  void addVal( unsigned int val )
  {
    m_vals.push_back( val );
  }

  friend std::ostream& operator<<(std::ostream& out,const reg_and_vals& rv)
  {
    out << rv.address() << " ";
    std::for_each( rv.m_vals.begin(), rv.m_vals.end(), [&](const auto& v){ out << v << " ";} );
    return out;
  }

private:
  inline void setAddress(unsigned int r0, unsigned int r1) { m_r0=r0; m_r1=r1; }
  std::vector<unsigned int> m_vals;
  unsigned int m_r0;
  unsigned int m_r1;
};
