#include "pflib/Elinks.h"
#include <unistd.h>

namespace pflib {

  Elinks::Elinks(int links) : n_links{links} {
    m_linksActive=std::vector<bool>(n_links,true);
  }
  
}
