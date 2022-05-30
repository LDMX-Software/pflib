#include "pftool_bias.h"
using pflib::PolarfireTarget;

void initialize_bias_on_all_boards(PolarfireTarget* pft, const int num_boards) {
    std::cout <<"Initializing bias on all "
              << num_boards
              <<" boards\n";
    for (int board {0}; board < num_boards; ++board) {
      initialize_bias(pft, board);
    }
}
void initialize_bias(PolarfireTarget* pft,
                     const int iboard)
{

    pflib::Bias bias=pft->hcal.bias(iboard);
    bias.initialize();
}
void set_bias_on_all_active_boards(PolarfireTarget* pft)
{
  static int led_sipm {0};
  led_sipm=BaseMenu::readline_int(" SiPM(0) or LED(1)? ", led_sipm);
  static int dac {0};
  dac=BaseMenu::readline_int(" What DAC value? ", dac);
  set_bias_on_all_active_boards(pft, led_sipm == 1, dac);
}
void set_bias_on_all_active_boards(PolarfireTarget* pft,
                                   const bool set_led,
                                   const int dac_value)
{
  std::vector<int> active_boards{get_rocs_with_active_links(pft)};
  for (const auto board : active_boards) {
    const int led_bias{0};
    const int num_connections {16};
    initialize_bias(pft, board);
    for (int connection{0}; connection < num_connections; ++connection) {
      pft->setBiasSetting(board, set_led, connection, dac_value);
    }
  }
}
void set_bias_on_all_connectors(PolarfireTarget* pft,
                                const int num_boards,
                                const bool set_led,
                                const int dac_value) {
  initialize_bias_on_all_boards(pft, num_boards);
    const int num_connections{16};
    for (int board{0}; board < num_boards; ++board) {
        for (int connection{0}; connection< num_connections; ++connection) {
            pft->setBiasSetting(board, set_led, connection, dac_value);
        }
    }
}

void set_bias_on_all_connectors(PolarfireTarget* pft) {
    static int dac_value {0};
    static int led_or_sipm {0};
    const int num_boards {get_num_rocs()};
    led_or_sipm=BaseMenu::readline_int(" SiPM(0) or LED(1)? ",led_or_sipm);
    dac_value = BaseMenu::readline_int("What DAC value?", dac_value);
    if (led_or_sipm < 0 || led_or_sipm > 1) {
        std::cout << "Invalid value for SiPM/LED\n";
        return;
    }
    bool set_led {led_or_sipm == 1};
    set_bias_on_all_connectors(pft, num_boards, set_led, dac_value);
}

void bias( const std::string& cmd, PolarfireTarget* pft )
{
  static int iboard=0;
  if (cmd=="STATUS") {
    iboard=BaseMenu::readline_int("Which board? ",iboard);
    pflib::Bias bias=pft->hcal.bias(iboard);
  }

  if (cmd=="INIT") {
    iboard=BaseMenu::readline_int("Which board? ",iboard);
    initialize_bias(pft, iboard);
  }
  if (cmd=="SET_ALL") {
      set_bias_on_all_connectors(pft);
  }
  if (cmd=="SET") {
    iboard=BaseMenu::readline_int("Which board? ",iboard);
    static int led_sipm=0;
    led_sipm=BaseMenu::readline_int(" SiPM(0) or LED(1)? ",led_sipm);
    int ichan=BaseMenu::readline_int(" Which HDMI connector? ",-1);
    int dac=BaseMenu::readline_int(" What DAC value? ",0);
    if (ichan>=0) {
      pft->setBiasSetting(iboard,led_sipm==1,ichan,dac);
    }
    if (ichan==-1) {
      printf("\n Setting bias on all 16 connectors. \n");
      for(int k = 0; k < 16; k++){
        pft->setBiasSetting(iboard,led_sipm==1,k,dac);
      }
    }
  }
  if (cmd=="LOAD") {
    printf("\n --- This command expects a CSV file with four columns [0=SiPM/1=LED,board,hdmi#,value].\n");
    printf(" --- Line starting with # are ignored.\n");
    std::string fname=BaseMenu::readline("Filename: ");
    if (not pft->loadBiasSettings(fname)) {
      std::cerr << "\n\n  ERROR: Unable to access " << fname << std::endl;
    }
  }
}

namespace {
auto menu_bias = pftool::menu("BIAS","bias voltage settings")
  //->line("STATUS","Read the bias line settings", bias )
  ->line("INIT","Initialize a board", bias )
  ->line("SET","Set a specific bias line setting", bias )
  ->line("SET_ALL", "Set a specific bias line setting to every connector", bias)
  ->line("LOAD","Load bias values from file", bias )
;
}
